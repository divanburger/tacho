
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <SDL2/SDL.h>
#include <cairo/cairo.h>

#include "util.h"
#include "string.h"
#include "timeline.h"
#include "files.h"
#include "ui.h"

struct {
   MemoryArena memory;

   DirectoryList profile_file_list;
   double last_file_check;

   int64_t click_draw_y;
   int64_t click_draw_start_time;

   int64_t draw_y;
   int64_t draw_start_time;
   double draw_time_width;

   Timeline timeline;
   TimelineEntry *highlighted_entry;
   TimelineEntry *active_entry;

   String  watch_path;
   int64_t watch_panel_width;
   bool    watch_panel_open;
} state = {};

void tch_update(Context *ctx, cairo_t *cr) {
   char buffer[4096];

   Timeline* timeline = &state.timeline;
   int64_t timeline_view_x = 0;
   int64_t timeline_view_width = ctx->width;

   if (str_nonblank(state.watch_path)) {
      state.watch_panel_open = true;
      state.watch_panel_width = 300;
      timeline_view_x = state.watch_panel_width;
      timeline_view_width -= state.watch_panel_width;

      if (state.last_file_check <= 0.0 || state.last_file_check < ctx->proc_time - 3.0) {
         state.last_file_check = ctx->proc_time;

         printf("Scanning directory\n");
         file_list_free(&state.profile_file_list);
         file_list_init(&state.profile_file_list);
         file_read_directory(&state.profile_file_list, state.watch_path);
      }
   } else {
      state.watch_panel_open = false;
   }

   if (state.draw_time_width == 0) {
      state.draw_start_time = timeline->start_time;
      state.draw_time_width = timeline->end_time - timeline->start_time;
   }

   if (ctx->click_went_down) {
      state.click_draw_start_time = state.draw_start_time;
      state.click_draw_y = state.draw_y;
   }

   if (ctx->click) {
      state.draw_start_time = (int64_t)(state.click_draw_start_time + ((ctx->click_mouse_x - ctx->mouse_x) * state.draw_time_width) / timeline_view_width);
      state.draw_y = (int64_t)(state.click_draw_y + (ctx->click_mouse_y - ctx->mouse_y));
   }

   double mouse_time = state.draw_start_time + (ctx->mouse_x * state.draw_time_width) / timeline_view_width;

   {
      double zoom_factor = (ctx->zoom > 0 ? (1.0 / 1.1) : 1.1);
      int zoom_iters = (ctx->zoom > 0 ? ctx->zoom : -ctx->zoom);
      for (int i = 0; i < zoom_iters; i++) {
         state.draw_time_width *= zoom_factor;
      }
      ctx->zoom = 0;
      state.draw_start_time = (int64_t) (mouse_time - (ctx->mouse_x * state.draw_time_width) / timeline_view_width);
   }

   if (state.draw_start_time < timeline->start_time) {
      state.draw_start_time = timeline->start_time;
   }

   if (state.draw_time_width > (timeline->end_time - timeline->start_time)) {
      state.draw_time_width = timeline->end_time - timeline->start_time;
   }

   if (state.draw_y < 0) {
      state.draw_y = 0;
   }

   double draw_end_time = state.draw_start_time + state.draw_time_width;

   cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
   cairo_paint(cr);

   cairo_select_font_face(cr, "Source Code Pro", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
   cairo_set_font_size(cr, 10);

   cairo_font_extents_t font_extents;
   cairo_font_extents(cr, &font_extents);

   cairo_set_line_width(cr, 1.0);

   double width_scale = timeline_view_width / state.draw_time_width;

   for (TimelineChunk *chunk = timeline->first; chunk; chunk = chunk->next) {
      for (int index = 0; index < chunk->entry_count; index++) {
         TimelineEntry *entry = chunk->entries + index;

         if (entry->end_time < state.draw_start_time || entry->start_time > draw_end_time) continue;

         double x0 = (entry->start_time - state.draw_start_time) * width_scale;
         double x1 = (entry->end_time - state.draw_start_time) * width_scale;

         if (x0 < 0.0) x0 = 0.0;
         if (x1 > timeline_view_width) x1 = timeline_view_width;

         double w = x1 - x0;

         x0 += timeline_view_x;
         x1 += timeline_view_x;

         if (w >= 0.1) {
            double y0 = entry->depth * 15 - state.draw_y;

            if (ctx->mouse_x >= x0 && ctx->mouse_x <= x1 && ctx->mouse_y >= y0 && ctx->mouse_y < y0 + 15) {
               state.highlighted_entry = entry;
            }

            if (w >= 20.0) {
               double b = 0.6;
               if (entry == state.active_entry) {
                  b = 1.0;
               } else if (entry == state.highlighted_entry) {
                  b = 0.8;
               }

               cairo_set_source_rgb(cr, 0.17 * b, 0.33 * b, 0.5 * b);
               cairo_rectangle(cr, x0, y0, w, 14);
               cairo_fill_preserve(cr);

               cairo_set_source_rgb(cr, 0.33 * b, 0.66 * b, 1.0 * b);
               cairo_stroke(cr);

               double text_x = x0 + 2, text_w = w - 4;
               if (text_x < 2.0) {
                  text_x = 2.0;
                  text_w = x1 - text_x - 4;
               }

               cairo_rectangle(cr, text_x, y0, text_w, 15);
               cairo_clip(cr);

               cairo_text_extents_t extents;
               cairo_text_extents(cr, entry->name.data, &extents);

               cairo_set_source_rgb(cr, 0.9, 0.9, 0.9);
               cairo_move_to(cr, text_x, y0 + font_extents.ascent);
               cairo_show_text(cr, entry->name.data);

               text_x += extents.x_advance + 4.0;

               double spent = (entry->end_time - entry->start_time);
               if (spent < 2000.0)
                  snprintf(buffer, array_size(buffer), "%.2f ns", spent);
               else if (spent < 2000000.0)
                  snprintf(buffer, array_size(buffer), "%.2f us", spent / 1000.0);
               else if (spent < 2000000000.0)
                  snprintf(buffer, array_size(buffer), "%.2f ms", spent / 1000000.0);
               else
                  snprintf(buffer, array_size(buffer), "%.2f s", spent / 1000000000.0);

               cairo_set_source_rgb(cr, 0.6, 0.6, 0.6);
               cairo_move_to(cr, text_x, y0 + font_extents.ascent);
               cairo_show_text(cr, buffer);

               cairo_reset_clip(cr);
            } else {
               cairo_set_source_rgb(cr, 0.1, 0.2, 0.3);
               cairo_rectangle(cr, x0, y0, w, 15);
               cairo_fill(cr);
            }
         }
      }
   }

   if (state.watch_panel_open) {
      cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
      cairo_rectangle(cr, 0, 0, state.watch_panel_width, ctx->height);
      cairo_fill_preserve(cr);
      cairo_clip(cr);

      double cur_y = 2.0 + font_extents.ascent;

      for (auto block = state.profile_file_list.first; block; block = block->next) {
         for (int i = 0; i < block->count; i++) {
            File *file = block->files + i;

            cairo_move_to(cr, 2.0, cur_y);
            cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
            cairo_show_text(cr, file->name.data);

            cur_y += font_extents.height * 2 + 4;
         }
      }

      cairo_set_source_rgb(cr, 0.9, 0.9, 0.9);
      cairo_move_to(cr, state.watch_panel_width, 0.0);
      cairo_line_to(cr, state.watch_panel_width, ctx->height);
      cairo_stroke(cr);
      cairo_reset_clip(cr);
   }

   if (state.highlighted_entry && ctx->click_went_up && (abs(ctx->click_mouse_x - ctx->mouse_x) <= 2)) {
      state.active_entry = state.highlighted_entry;
      if (ctx->double_click) {
         state.draw_start_time = state.active_entry->start_time;
         state.draw_time_width = state.active_entry->end_time - state.active_entry->start_time;
      }
   }

   if (state.active_entry) {
      snprintf(buffer, array_size(buffer), "%.*s - events: %i - %.*s:%i", str_prt(state.active_entry->name), state.active_entry->events, str_prt(state.active_entry->path), state.active_entry->line_no);
      cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
      cairo_move_to(cr, 2, ctx->height - 2 - font_extents.descent);
      cairo_show_text(cr, buffer);
   }
}

int main(int argc, char **args) {
   if (argc != 2) {
      printf("Usage: %s <filename>\n", args[0]);
      return 1;
   }

   Timeline *timeline = &state.timeline;

   String arg_file_name = str_copy(&state.memory, args[1]);
   File arg_file = file_stat(arg_file_name);

   if (arg_file.type == FILE_TYPE_FILE) {
      printf("Reading file: %s\n", args[1]);
      if (!tm_read_file(timeline, args[1])) {
         printf("Could not read file %s\n", args[1]);
         return 2;
      }
   } else if (arg_file.type == FILE_TYPE_DIRECTORY) {
      tm_init(timeline);
      printf("Watching directory: %s\n", args[1]);
      state.watch_path = arg_file_name;
   } else {
      printf("Unknown file type %s\n", args[1]);
      return 2;
   }

   ui_run(tch_update);
   return 0;
}