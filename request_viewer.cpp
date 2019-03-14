//
// Created by divan on 13/03/19.
//


#include <cstdio>
#include <ctime>
#include "allocator.h"
#include "string.h"
#include "array_list.h"
#include "ui.h"
#include "colour.h"
#include "cairo_helpers.h"

enum ColumnType {
   COL_UNKNOWN,
   COL_INTEGER,
   COL_ENUM
};

const char *column_names[] = {"PID", "method", "path", "format", "controller", "action", "status", "duration",
                              "gc_count", "gc_live_slots", "gc_live_slots_d", "gc_alloc_pages", "gc_alloc_pages_d",
                              "gc_sorted_pages", "gc_sorted_pages_d", "user"};
const ColumnType column_types[] = {COL_ENUM, COL_UNKNOWN, COL_UNKNOWN, COL_UNKNOWN, COL_UNKNOWN, COL_UNKNOWN,
                                   COL_ENUM, COL_INTEGER, COL_INTEGER, COL_INTEGER, COL_INTEGER, COL_INTEGER,
                                   COL_INTEGER, COL_INTEGER, COL_INTEGER, COL_UNKNOWN};

Colour column_colours[] = {{1.0,  0.67, 0.33},
                           {0.33, 1.00, 0.67},
                           {0.67, 0.33, 1.00}};

struct Column {
   const char *name;
   ColumnType type;
   i64 max;
   i64 min;
   bool enabled;

   i32 chosen;
   ArrayList<String> values;
};

struct Value {
   String text;
   i64 integer;
   bool given;
};

struct Request {
   i64 time;
   Value *values;
   bool included;
};

struct Log {
   Allocator *allocator;
   ArrayList<Request> requests;

   i32 column_count;
   Column *columns;

   i64 start_time;
   i64 end_time;
};

struct Reader {
   i64 line_no;
   char *ptr;

   Allocator *allocator;
   Log *log;
   Request request;
};

struct {
   Allocator *allocator;
   Log log;

   double draw_time_start;
   double draw_time_width;

   double click_time;
} state;

void update_chart(UIContext *ctx, cairo_t *cr, i32rect area) {
   if (state.draw_time_width <= 0) {
      state.draw_time_start = state.log.start_time;
      state.draw_time_width = state.log.end_time - state.log.start_time;
   }

   if (ctx->click_went_down) state.click_time = state.draw_time_start;

   if (ctx->click) {
      state.draw_time_start = state.click_time +
                              ((ctx->click_mouse_pos.x - ctx->mouse_pos.x) * state.draw_time_width) / area.w;
   }

   f64 mouse_time = state.draw_time_start + ((ctx->mouse_pos.x - area.x) * state.draw_time_width) / area.w;

   bool inside_area = inside(area, ctx->mouse_pos.x, ctx->mouse_pos.y);
   if (inside_area) {
      f64 zoom_factor = (ctx->mouse_delta_z > 0 ? (1.0 / 1.1) : 1.1);
      int zoom_iters = (ctx->mouse_delta_z > 0 ? ctx->mouse_delta_z : -ctx->mouse_delta_z);
      for (int i = 0; i < zoom_iters; i++) {
         state.draw_time_width *= zoom_factor;
      }
      state.draw_time_start = (i64) (mouse_time - ((ctx->mouse_pos.x - area.x) * state.draw_time_width) / area.w);
   }

   ArrayListCursor cursor = {};
   while (arl_cursor_step(&state.log.requests, &cursor)) {
      auto request = arl_cursor_get<Request>(cursor);
      request->included = true;

      for (i32 index = 0; index < state.log.column_count; index++) {
         Column *column = state.log.columns + index;

         if (column->type == COL_ENUM && column->chosen >= 0) {
            Value *value = request->values + index;

            String name = column->values.first->data[column->chosen];
            if (!str_equal(name, value->text)) request->included = false;
         }
      }
   }

   i32 column_colour_index = 0;

   for (i32 index = 0; index < state.log.column_count; index++) {
      Column *column = state.log.columns + index;
      if (!column->enabled || column->type != COL_INTEGER) continue;

      i64 last_time = -1;

      cairo_new_path(cr);
      cairo_set_source_rgb(cr, column_colours[column_colour_index]);
      column_colour_index = (column_colour_index + 1) % 4;

      ArrayListCursor cursor = {};
      while (arl_cursor_step(&state.log.requests, &cursor)) {
         auto request = arl_cursor_get<Request>(cursor);
         if (!request->included) continue;

         Value *value = request->values + index;
         if (!value->given) continue;

         double x = (request->time - state.draw_time_start) * area.w / state.draw_time_width;
         double y = area.h - (double) (value->integer - column->min) * area.h / (column->max - column->min);

//         if (last_time < request->time - 1) {
//            cairo_move_to(cr, x, y);
//         } else {
            cairo_line_to(cr, x, y);
//         }

         last_time = request->time;
      }

      cairo_stroke(cr);
   }
}

void update_settings(UIContext *ctx, cairo_t *cr, i32rect area) {
   i32 entry_height = 20;

   i64 y = 20;

   Colour background = Colour{0.15, 0.15, 0.15};
   cairo_set_source_rgb(cr, background);
   cairo_rectangle(cr, area.x, area.y, area.w, area.h);
   cairo_fill(cr);

   cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);

   cairo_font_extents_t font_extents;
   cairo_font_extents(cr, &font_extents);

   for (i32 index = 0; index < state.log.column_count; index++) {
      Column *column = state.log.columns + index;

      if (column->type != COL_INTEGER) continue;

      i32rect entry_rect = Rect(area.x, (int) y, area.w, entry_height);
      bool hover = inside(entry_rect, ctx->mouse_pos);
      if (hover && ctx->click_went_up) {
         column->enabled = !column->enabled;
         ctx->dirty = true;
      }

      cairo_rectangle(cr, area.x + 20 + 6, y + 6, 8, 8);

      column->enabled ? cairo_fill(cr) : cairo_stroke(cr);

      cairo_move_to(cr, area.x + 20 + 20, y + (entry_height - font_extents.height) / 2 + font_extents.ascent);
      cairo_show_text(cr, column->name);

      y += entry_height;
   }

   y += entry_height;

   for (i32 index = 0; index < state.log.column_count; index++) {
      Column *column = state.log.columns + index;
      if (column->type != COL_ENUM) continue;

      i32rect entry_rect = Rect(area.x, (int) y, area.w, entry_height);
      bool hover = inside(entry_rect, ctx->mouse_pos);
      if (hover && ctx->click_went_up) {
         column->chosen = -1;
         ctx->dirty = true;
      }

      cairo_move_to(cr, area.x + 20, y + (entry_height - font_extents.height) / 2 + font_extents.ascent);
      cairo_show_text(cr, column->name);
      y += entry_height;

      ArrayListCursor cursor = {};
      while (arl_cursor_step(&column->values, &cursor)) {
         String name = *arl_cursor_get<String>(cursor);

         i32rect entry_rect = Rect(area.x, (int) y, area.w, entry_height);
         bool hover = inside(entry_rect, ctx->mouse_pos);
         if (hover && ctx->click_went_up) {
            column->chosen = cursor.index;
            ctx->dirty = true;
         }

         cairo_new_path(cr);
         cairo_arc(cr, area.x + 20 + 10, y + entry_height / 2, 4, 0, M_PI * 2);
         column->chosen == cursor.index ? cairo_fill(cr) : cairo_stroke(cr);

         cairo_move_to(cr, area.x + 20 + 20, y + (entry_height - font_extents.height) / 2 + font_extents.ascent);
         cairo_show_text(cr, name.data);

         y += entry_height;
      }

      y += entry_height;
   }
}

void update(UIContext *ctx, cairo_t *cr) {
   cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
   cairo_paint(cr);

   cairo_select_font_face(cr, "Source Code Pro", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
   cairo_set_font_size(cr, 10);

   cairo_set_line_width(cr, 1.0);

   update_chart(ctx, cr, Rect(0, 0, ctx->width - 200, ctx->height));
   update_settings(ctx, cr, Rect(ctx->width - 200, 0, 200, ctx->height));
}

bool parse_tag(Reader *reader) {
   auto log = reader->log;

   char *tag_ptr = reader->ptr;
   while ((*reader->ptr >= 'a' && *reader->ptr <= 'z') || (*reader->ptr >= 'A' && *reader->ptr <= 'Z') ||
          *reader->ptr == '_')
      reader->ptr++;
   String tag_name = {(i32) (reader->ptr - tag_ptr), tag_ptr};

   if (*reader->ptr != '=' && *reader->ptr != ':') {
//      printf("%lu: Invalid tag: %s\n", reader->line_no, tag_ptr);
      return false;
   } else {
      reader->ptr++;
   }

   i32 column_index = -1;
   for (i32 i = 0; i < array_size(column_names); i++) {
      if (str_equal(tag_name, column_names[i])) {
         column_index = i;
         break;
      }
   }

   char *value_ptr = reader->ptr;
   i32 hash_depth = 0;
   for (; *reader->ptr; reader->ptr++) {
      if (*reader->ptr == '{') hash_depth++;
      if (*reader->ptr == '}') hash_depth--;
      if (*reader->ptr == ' ' && hash_depth <= 0) break;
   }

   if (column_index == -1) {
//      printf("%lu: Ignored tag: %s\n", reader->line_no, tag_ptr);
      return false;
   }

   if (!reader->request.values) {
      reader->request.values = std_alloc_array_zero(reader->allocator, Value, log->column_count);
   }

   Value *value = reader->request.values + column_index;
   value->given = true;
   value->text = str_copy(reader->allocator, value_ptr, (i32) (reader->ptr - value_ptr));

   Column *column = log->columns + column_index;
   switch (column->type) {
      case COL_INTEGER: {
         if (value->text.length == 0) {
            value->given = false;
         } else {
            value->integer = strtol(value->text.data, nullptr, 10);
            if (column->min > value->integer) column->min = value->integer;
            if (column->max < value->integer) column->max = value->integer;
         }
         break;
      }
      case COL_ENUM: {
         bool found = false;
         ArrayListCursor cursor = {};
         while (arl_cursor_step(&column->values, &cursor)) {
            if (str_equal(*arl_cursor_get<String>(cursor), value->text)) {
               found = true;
               break;
            }
         }

         if (!found) arl_push(&column->values, value->text);
         break;
      }
      default:
         break;
   }

   return true;
}

int main(int argc, char **args) {
   if (argc != 2) {
      printf("  Usage: %s <filename>", args[0]);
      return 1;
   }

   char *filename = args[1];
   FILE *input = fopen(filename, "r");
   if (!input) {
      fprintf(stderr, "Unknown file: %s", filename);
   }

   int buffer_size = 1024 * 1024 * 16;
   char *buffer = std_alloc_array(nullptr, char, buffer_size);

   ArenaAllocator arena = arena_make();
   state.allocator = (Allocator *) &arena;

   arl_init(&state.log.requests, state.allocator);

   state.log.start_time = INT64_MAX;
   state.log.end_time = INT64_MIN;

   state.log.column_count = array_size(column_names);
   state.log.columns = std_alloc_array_zero(nullptr, Column, state.log.column_count);
   for (i32 index = 0; index < state.log.column_count; index++) {
      Column *column = state.log.columns + index;
      column->name = column_names[index];
      column->type = column_types[index];
      column->enabled = false;
      column->min = 0;
      column->max = 0;
      column->chosen = -1;
      arl_init(&column->values, state.allocator);
   }

   Reader reader = {};
   reader.allocator = state.allocator;
   reader.log = &state.log;

   while (fgets(buffer, buffer_size, input)) {
      reader.line_no++;

      reader.request = {};
      reader.ptr = buffer;

      char *start_ptr = reader.ptr;
      while ((*reader.ptr >= '0' && *reader.ptr <= '9') || *reader.ptr == '-') reader.ptr++;

      if (*reader.ptr != ' ') {
//         printf("%lu: Invalid line: %s\n", reader.line_no, buffer);
         continue;
      } else {
         reader.ptr++;
      }

      while ((*reader.ptr >= '0' && *reader.ptr <= '9') || *reader.ptr == ':') reader.ptr++;

      if (*reader.ptr != ' ') {
//         printf("%lu: Invalid line: %s\n", reader.line_no, buffer);
         continue;
      } else {
         reader.ptr++;
      }

      struct tm tm;
      time_t epoch;
      if (strptime(start_ptr, "%Y-%m-%d %H:%M:%S", &tm) != nullptr) {
         reader.request.time = (i64) mktime(&tm);
         if (state.log.start_time > reader.request.time) state.log.start_time = reader.request.time;
         if (state.log.end_time < reader.request.time) state.log.end_time = reader.request.time;
      } else {
         printf("%lu: Invalid timestamp: %s\n", reader.line_no, buffer);
         continue;
      }

      while (*reader.ptr) {
         if (*reader.ptr == ' ') {
            reader.ptr++;
            continue;
         }

         if ((*reader.ptr >= 'a' && *reader.ptr <= 'z') || (*reader.ptr >= 'A' && *reader.ptr <= 'Z')) {
            if (!parse_tag(&reader)) {
               while (*reader.ptr && *reader.ptr != ' ') reader.ptr++;
            }
         } else {
//            printf("%lu: Invalid line: %s\n", reader.line_no, buffer);
            break;
         }
      }

      arl_push(&state.log.requests, reader.request);
   }

   arena_stats_print(&arena);

   std_free(nullptr, buffer);

   ui_run(update);
}