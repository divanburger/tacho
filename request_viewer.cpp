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
#include "array.h"

enum ColumnType {
   COL_UNKNOWN,
   COL_INTEGER,
   COL_ENUM
};

enum GroupByTime {
   GROUPBY_NONE,
   GROUPBY_SECOND,
   GROUPBY_MINUTE,
   GROUPBY_HOUR
};

const char *column_names[] = {"PID", "method", "path", "format", "controller", "action", "status", "duration",
                              "gc_count", "gc_count_d", "gc_live_slots", "gc_live_slots_d", "gc_alloc_pages", "gc_alloc_pages_d",
                              "gc_old_objects", "user"};
const ColumnType column_types[] = {COL_ENUM, COL_ENUM, COL_UNKNOWN, COL_ENUM, COL_ENUM, COL_ENUM, COL_ENUM, COL_INTEGER,
                                   COL_INTEGER, COL_INTEGER, COL_INTEGER, COL_INTEGER, COL_INTEGER, COL_INTEGER,
                                   COL_INTEGER, COL_UNKNOWN};

Colour column_colours[] = {{0.67, 1.00, 0.33},
                           {1.0,  0.33, 0.67},
                           {1.0,  0.67, 0.33},
                           {0.33, 0.67, 1.00},
                           {0.33, 1.00, 0.67},
                           {0.67, 0.33, 1.00}};

using ColumnMask = u32;

struct Column {
   String name;
   ColumnType type;
   i64 max;
   i64 min;
   bool enabled;

   i32 chosen;
   String *values;
};

union Value {
   String text;
   i64 enum_id;
   i64 integer;
};

struct Request {
   i64 time;
   Value *values;
   u32 given;
   bool included;
};

struct Log {
   Allocator *allocator;

   Request *requests;

   i32 column_count;
   Column *columns;

   i64 start_time;
   i64 end_time;
};

struct Reader {
   char* filename;
   volatile bool follow;

   i64 line_no;
   char *ptr;

   Log *log;
   Request request;
};

struct {
   Allocator *allocator;
   Log log;

   GroupByTime group_by;

   double draw_time_start;
   double draw_time_width;

   double click_time;

   i32 settings_height;

   volatile bool refilter;
   volatile bool initialized;
} state;

void update_chart(UIContext *ctx, cairo_t *cr, i32rect area) {
   cairo_rectangle(cr, area);
   cairo_clip(cr);

   if (state.draw_time_width <= 0) {
      state.group_by = GROUPBY_SECOND;
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

   if (state.draw_time_start < state.log.start_time) state.draw_time_start = state.log.start_time;

   if (state.refilter) {
      state.refilter = false;

      for (i32 request_index = 0; request_index < alen(state.log.requests); request_index++) {
         auto request = state.log.requests + request_index;
         request->included = true;

         for (i32 column_index = 0; column_index < state.log.column_count; column_index++) {
            Column *column = state.log.columns + column_index;

            if (column->type == COL_ENUM && column->chosen >= 0) {
               Value *value = request->values + column_index;
               if (value->enum_id != column->chosen) {
                  request->included = false;
                  break;
               }
            }
         }
      }
   }

   // TODO: @Speed Convert these to binary searches
   i32 draw_start_index, draw_end_index;
   {
      for (draw_start_index = 0; draw_start_index < alen(state.log.requests); draw_start_index++) {
         if (state.log.requests[draw_start_index].time >= state.draw_time_start) break;
      }

      draw_start_index--;
      if (draw_start_index < 0) draw_start_index = 0;

      for (draw_end_index = draw_start_index; draw_end_index < alen(state.log.requests); draw_end_index++) {
         if (state.log.requests[draw_end_index].time > state.draw_time_start + state.draw_time_width) break;
      }

      draw_end_index++;
      if (draw_end_index >= alen(state.log.requests)) draw_end_index = alen(state.log.requests) - 1;
   }

   for (i32 column_index = 0; column_index < state.log.column_count; column_index++) {
      Column *column = state.log.columns + column_index;
      ColumnMask column_mask = (1UL << (ColumnMask) column_index);
      if (!column->enabled || column->type != COL_INTEGER) continue;

      cairo_new_path(cr);
      cairo_set_source_rgb(cr, column_colours[column_index % array_size(column_colours)]);

      i64 last_time = 0;
      u32 count = 0;

      double x_factor = area.w / state.draw_time_width;
      double y_factor = (double) area.h / (column->max - column->min);
      double x = 0.0, y = 0.0;

      for (i32 request_index = draw_start_index; request_index < draw_end_index; request_index++) {
         auto request = state.log.requests + request_index;
         if (!request->included) continue;
         if ((request->given & column_mask) == 0) continue;

         if (last_time + 1 < request->time) {
            if (count == 1) {
               cairo_arc(cr, x, y, 1.5, 0, M_PI*2);
               cairo_fill(cr);
            } else {
               cairo_stroke(cr);
            }
            count = 0;
         }
         last_time = request->time;

         Value *value = request->values + column_index;
         x = (request->time - state.draw_time_start) * x_factor;
         y = area.h - (value->integer - column->min) * y_factor;

         count ? cairo_line_to(cr, x, y) : cairo_move_to(cr, x, y);
         count++;
      }

      if (count == 1) {
         cairo_arc(cr, x, y, 1.5, 0, M_PI*2);
         cairo_fill(cr);
      } else {
         cairo_stroke(cr);
      }
   }

   cairo_reset_clip(cr);
}

void update_settings(UIContext *ctx, cairo_t *cr, i32rect area) {
   i32 entry_height = 20;

   Colour background = Colour{0.15, 0.15, 0.15};
   cairo_set_source_rgb(cr, background);
   cairo_rectangle(cr, area.x, area.y, area.w, area.h);
   cairo_fill(cr);


   cairo_font_extents_t font_extents;
   cairo_font_extents(cr, &font_extents);

   auto scroll_area = ui_scrollable_begin("settings", area, Vec(area.w, state.settings_height));

   i32 y = 20 + scroll_area.y;

   for (i32 index = 0; index < state.log.column_count; index++) {
      Column *column = state.log.columns + index;

      if (column->type != COL_INTEGER) continue;

      i32rect entry_rect = Rect(scroll_area.x, (int) y, scroll_area.w, entry_height);
      bool hover = inside(entry_rect, ctx->mouse_pos);
      if (hover && ctx->click_went_up) {
         column->enabled = !column->enabled;
         ctx->dirty = true;
         state.refilter = true;
      }

      cairo_set_source_rgb(cr, column_colours[index % array_size(column_colours)]);
      cairo_rectangle(cr, scroll_area.x + 20 + 6, y + 6, 8, 8);

      column->enabled ? cairo_fill(cr) : cairo_stroke(cr);

      cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
      cairo_move_to(cr, scroll_area.x + 20 + 20, y + (entry_height - font_extents.height) / 2 + font_extents.ascent);
      cairo_show_text(cr, column->name.data);

      y += entry_height;
   }

   y += entry_height;

   for (i32 index = 0; index < state.log.column_count; index++) {
      Column *column = state.log.columns + index;
      if (column->type != COL_ENUM) continue;

      i32rect entry_rect = Rect(scroll_area.x, (int) y, scroll_area.w, entry_height);
      bool hover = inside(entry_rect, ctx->mouse_pos);
      if (hover && ctx->click_went_up) {
         column->chosen = -1;
         ctx->dirty = true;
         state.refilter = true;
      }

      cairo_move_to(cr, scroll_area.x + 20, y + (entry_height - font_extents.height) / 2 + font_extents.ascent);
      cairo_show_text(cr, column->name.data);
      y += entry_height;

      for (i32 value_index = 0; value_index < alen(column->values); value_index++) {
         String name = column->values[value_index];

         i32rect option_rect = Rect(scroll_area.x, (int) y, scroll_area.w, entry_height);
         bool option_hover = inside(option_rect, ctx->mouse_pos);
         if (option_hover && ctx->click_went_up) {
            column->chosen = value_index;
            ctx->dirty = true;
            state.refilter = true;
         }

         cairo_new_path(cr);
         cairo_arc(cr, scroll_area.x + 20 + 10, y + entry_height / 2, 4, 0, M_PI * 2);
         column->chosen == value_index ? cairo_fill(cr) : cairo_stroke(cr);

         cairo_move_to(cr, scroll_area.x + 20 + 20, y + (entry_height - font_extents.height) / 2 + font_extents.ascent);
         cairo_show_text(cr, name.data);

         y += entry_height;
      }

      y += entry_height;
   }

   ui_scrollable_end("settings");

   state.settings_height = y - scroll_area.y;
}

void update(UIContext *ctx, cairo_t *cr) {
   cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
   cairo_paint(cr);

   cairo_select_font_face(cr, "Source Code Pro", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
   cairo_set_font_size(cr, 10);

   cairo_set_line_width(cr, 1.0);

   if (state.initialized) {
      update_settings(ctx, cr, Rect(ctx->width - 300, 0, 300, ctx->height));
      update_chart(ctx, cr, Rect(0, 0, ctx->width - 300, ctx->height));
   }
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
      if (str_equal(tag_name, reader->log->columns[i].name)) {
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
   String value_text = {(i32) (reader->ptr - value_ptr), value_ptr};

   if (column_index == -1) {
//      printf("%lu: Ignored tag: %s\n", reader->line_no, tag_ptr);
      return false;
   }

   if (!reader->request.values) {
      reader->request.values = std_alloc_array_zero(reader->log->allocator, Value, log->column_count);
   }

   Column *column = log->columns + column_index;
   ColumnMask column_mask = 1UL << (ColumnMask) column_index;

   Value *value = reader->request.values + column_index;
   reader->request.given |= column_mask;

   switch (column->type) {
      case COL_INTEGER: {
         if (value_text.length == 0) {
            reader->request.given &= ~column_mask;
         } else {
            value->integer = strtol(value_text.data, nullptr, 10);
            if (column->min > value->integer) column->min = value->integer;
            if (column->max < value->integer) column->max = value->integer;
         }
         break;
      }
      case COL_ENUM: {
         i32 found = -1;
         for (i32 value_index = 0; value_index < alen(column->values); value_index++) {
            if (str_equal(column->values[value_index], value_text)) {
               found = value_index;
               break;
            }
         }

         if (found == -1) {
            found = alen(column->values);
            auto text = str_copy(reader->log->allocator, value_text);
            apush(column->values, text);
         }

         value->enum_id = found;
         break;
      }
      default: {
         value->text = str_copy(reader->log->allocator, value_text);
         break;
      }
   }

   return true;
}

int read_file(void* data) {
   auto reader = (Reader*)data;

   FILE *input = nullptr;

   if (strcmp(reader->filename, "-") == 0) {
      input = stdin;
   } else {
      input = fopen(reader->filename, "r");
      if (!input) {
         fprintf(stderr, "Unknown file: %s\n", reader->filename);
         return 2;
      }
   }

   int buffer_size = 1024 * 1024 * 16;
   char *buffer = std_alloc_array(nullptr, char, buffer_size);

   bool reading = true;
   bool new_requests = false;

   while (reading) {
      auto read_start_time = time(nullptr);
      while (fgets(buffer, buffer_size, input)) {
         reader->line_no++;

         reader->request = {};
         reader->ptr = buffer;

         char *start_ptr = reader->ptr;
         while ((*reader->ptr >= '0' && *reader->ptr <= '9') || *reader->ptr == '-') reader->ptr++;

         if (*reader->ptr != ' ') {
//         printf("%lu: Invalid line: %s\n", reader->line_no, buffer);
            continue;
         } else {
            reader->ptr++;
         }

         while ((*reader->ptr >= '0' && *reader->ptr <= '9') || *reader->ptr == ':') reader->ptr++;

         if (*reader->ptr != ' ') {
//         printf("%lu: Invalid line: %s\n", reader->line_no, buffer);
            continue;
         } else {
            reader->ptr++;
         }

         struct tm tm;
         time_t epoch;
         if (strptime(start_ptr, "%Y-%m-%d %H:%M:%S", &tm) != nullptr) {
            reader->request.time = (i64) mktime(&tm);
            if (state.log.start_time > reader->request.time) state.log.start_time = reader->request.time;
            if (state.log.end_time < reader->request.time) state.log.end_time = reader->request.time;
         } else {
            printf("%lu: Invalid timestamp: %s\n", reader->line_no, buffer);
            continue;
         }

         while (*reader->ptr) {
            if (*reader->ptr == ' ') {
               reader->ptr++;
               continue;
            }

            if ((*reader->ptr >= 'a' && *reader->ptr <= 'z') || (*reader->ptr >= 'A' && *reader->ptr <= 'Z')) {
               if (!parse_tag(reader)) {
                  while (*reader->ptr && *reader->ptr != ' ') reader->ptr++;
               }
            } else {
//            printf("%lu: Invalid line: %s\n", reader.line_no, buffer);
               break;
            }
         }

         apush(state.log.requests, reader->request);
         new_requests = true;
      }

      if (ferror(input)) {
         printf("Read failed\n");
         break;
      }

      if (new_requests) {
         printf("Read time: %lis\n", time(nullptr) - read_start_time);
         printf("Requests: %i\n", alen(state.log.requests));

         state.refilter = true;
         state.initialized = true;
         ui_context.dirty = true;
      }

      new_requests = false;
      if (!reader->follow) break;
      SDL_Delay(250);
   }

   std_free(nullptr, buffer);
   fclose(input);
}

int main(int argc, char **args) {
   if (argc != 2) {
      printf("  Usage: %s <filename>\n\n", args[0]);
      return 1;
   }

   ArenaAllocator arena = arena_make();
   state.initialized = false;
   state.allocator = (Allocator *) &arena;
   ainit(state.log.requests, nullptr);

   state.log.start_time = INT64_MAX;
   state.log.end_time = INT64_MIN;

   state.log.column_count = array_size(column_names);
   state.log.columns = std_alloc_array_zero(nullptr, Column, state.log.column_count);
   for (i32 index = 0; index < state.log.column_count; index++) {
      Column *column = state.log.columns + index;
      column->name = str_copy(state.allocator, column_names[index]);
      column->type = column_types[index];
      column->enabled = false;
      column->min = 0;
      column->max = 0;
      column->chosen = -1;
      ainit(column->values, state.allocator);
   }

   Reader reader = {};
   reader.filename = args[1];
   reader.follow = true;
   reader.log = &state.log;

   auto reader_thread = SDL_CreateThread(read_file, "Reader", &reader);

   ui_run(update);

   reader.follow = false;

   int result = 0;
   SDL_WaitThread(reader_thread, &result);
}