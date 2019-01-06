
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <SDL2/SDL.h>
#include <cairo/cairo.h>

#include "util.h"
#include "string.h"
#include "timeline.h"

struct Context {
   bool running;
   int width;
   int height;

   uint64_t time_counter;
   double real_delta;
   double proc_time;

   int zoom;
   int mouse_x;
   int mouse_y;

   int click_mouse_x;
   int click_mouse_y;
   int64_t click_draw_y;
   int64_t click_draw_start_time;

   bool click;
   bool click_went_up;
   bool click_went_down;
   bool double_click;
   double last_click;

   int64_t draw_y;
   int64_t draw_start_time;
   double draw_time_width;

   TimelineEntry *highlighted_entry;
   TimelineEntry *active_entry;

   MemoryArena temp;
};

void update(Context *ctx, Timeline *timeline, cairo_t *cr) {
   char buffer[4096];

   if (ctx->click) {
      ctx->draw_start_time = (int64_t)(ctx->click_draw_start_time + ((ctx->click_mouse_x - ctx->mouse_x) * ctx->draw_time_width) / ctx->width);
      ctx->draw_y = (int64_t)(ctx->click_draw_y + (ctx->click_mouse_y - ctx->mouse_y));
   }

   double mouse_time = ctx->draw_start_time + (ctx->mouse_x * ctx->draw_time_width) / ctx->width;

   {
      double old_width = ctx->draw_time_width;
      double zoom_factor = (ctx->zoom > 0 ? (1.0 / 1.1) : 1.1);
      int zoom_iters = (ctx->zoom > 0 ? ctx->zoom : -ctx->zoom);
      for (int i = 0; i < zoom_iters; i++) {
         ctx->draw_time_width *= zoom_factor;
      }
      ctx->zoom = 0;
      ctx->draw_start_time = (int64_t) (mouse_time - (ctx->mouse_x * ctx->draw_time_width) / ctx->width);
   }

   if (ctx->draw_start_time < timeline->start_time) {
      ctx->draw_start_time = timeline->start_time;
   }

   if (ctx->draw_time_width > (timeline->end_time - timeline->start_time)) {
      ctx->draw_time_width = timeline->end_time - timeline->start_time;
   }

   if (ctx->draw_y < 0) {
      ctx->draw_y = 0;
   }

   double draw_end_time = ctx->draw_start_time + ctx->draw_time_width;

   cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
   cairo_paint(cr);

   cairo_select_font_face(cr, "Source Code Pro", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
   cairo_set_font_size(cr, 10);

   cairo_font_extents_t font_extents;
   cairo_font_extents(cr, &font_extents);

   cairo_set_line_width(cr, 1.0);

//   int stack_index = 0;
//   TimelineEntry* stack[1024];

   double width_scale = ctx->width / ctx->draw_time_width;

   for (TimelineChunk *chunk = timeline->first; chunk; chunk = chunk->next) {
      for (int index = 0; index < chunk->entry_count; index++) {
         TimelineEntry *entry = chunk->entries + index;

         if (entry->end_time < ctx->draw_start_time || entry->start_time > draw_end_time) continue;

         double x0 = (entry->start_time - ctx->draw_start_time) * width_scale;
         double x1 = (entry->end_time - ctx->draw_start_time) * width_scale;

         if (x0 < 0.0) x0 = 0.0;
         if (x1 > ctx->width) x1 = ctx->width;

         double w = x1 - x0;

         if (w >= 0.1) {
            double y0 = entry->depth * 15 - ctx->draw_y;

            if (ctx->mouse_x >= x0 && ctx->mouse_x <= x1 && ctx->mouse_y >= y0 && ctx->mouse_y < y0 + 15) {
               ctx->highlighted_entry = entry;
            }

            if (w >= 20.0) {
               double b = 0.6;
               if (entry == ctx->active_entry) {
                  b = 1.0;
               } else if (entry == ctx->highlighted_entry) {
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

   if (ctx->highlighted_entry && ctx->click_went_up && (abs(ctx->click_mouse_x - ctx->mouse_x) <= 2)) {
      ctx->active_entry = ctx->highlighted_entry;
      if (ctx->double_click) {
         ctx->draw_start_time = ctx->active_entry->start_time;
         ctx->draw_time_width = ctx->active_entry->end_time - ctx->active_entry->start_time;
      }
   }

   if (ctx->active_entry) {
      snprintf(buffer, array_size(buffer), "%.*s - events: %i - %.*s:%i", str_prt(ctx->active_entry->name), ctx->active_entry->events, str_prt(ctx->active_entry->path), ctx->active_entry->line_no);
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

   Timeline timeline_mem;
   Timeline *timeline = &timeline_mem;
   if (!tm_read_file(timeline, args[1])) {
      printf("Could not read file %s", args[1]);
      return 2;
   }

//   if (!tm_output_html(timeline, "output.html")) {
//      printf("Could not read file %s", args[1]);
//      return 2;
//   }

//   uint64_t allocated, used;
//   arena_stats(&timeline->arena, &allocated, &used);
//   printf("Timeline memory: allocated: %lu MB used: %lu MB\n", (allocated >> 20), (used >> 20));

   if (SDL_Init(SDL_INIT_VIDEO) == 0) {
      SDL_Window *window = nullptr;
      SDL_Renderer *renderer = nullptr;

      Context ctx = {};
      ctx.running = true;
      ctx.width = 640;
      ctx.height = 480;

      ctx.draw_start_time = timeline->start_time;
      ctx.draw_time_width = timeline->end_time - timeline->start_time;

      arena_init(&ctx.temp);

      if (SDL_CreateWindowAndRenderer(ctx.width, ctx.height, SDL_WINDOW_RESIZABLE, &window, &renderer) == 0) {
         auto backbuffer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, ctx.width,
                                             ctx.height);

         while (ctx.running) {
            auto now = SDL_GetPerformanceCounter();
            ctx.real_delta = (double)(now - ctx.time_counter) / SDL_GetPerformanceFrequency();
            if (ctx.real_delta > 1.0) ctx.real_delta = 1.0;

            ctx.time_counter = now;
            ctx.proc_time += ctx.real_delta;

            ctx.click_went_down = false;
            ctx.click_went_up = false;
            ctx.double_click = false;

            SDL_Event event;
            while (SDL_PollEvent(&event)) {
               switch (event.type) {
                  case SDL_QUIT: {
                     ctx.running = false;
                  }
                     break;
                  case SDL_MOUSEWHEEL: {
                     ctx.zoom += event.wheel.y;
                  }
                     break;
                  case SDL_MOUSEMOTION: {
                     ctx.mouse_x = event.motion.x;
                     ctx.mouse_y = event.motion.y;
                  }
                     break;
                  case SDL_MOUSEBUTTONDOWN: {
                     ctx.mouse_x = event.button.x;
                     ctx.mouse_y = event.button.y;
                     ctx.click_mouse_x = event.button.x;
                     ctx.click_mouse_y = event.button.y;
                     ctx.click_draw_start_time = ctx.draw_start_time;
                     ctx.click_draw_y = ctx.draw_y;
                     ctx.click = true;
                     ctx.click_went_down = true;
                  }
                     break;
                  case SDL_MOUSEBUTTONUP: {
                     ctx.mouse_x = event.button.x;
                     ctx.mouse_y = event.button.y;
                     ctx.click = false;
                     ctx.click_went_up = true;

                     if (ctx.last_click >= ctx.proc_time - 0.3) {
                        ctx.double_click = true;
                     }
                     ctx.last_click = ctx.proc_time;
                  }
                     break;
                  case SDL_WINDOWEVENT: {
                     switch (event.window.event) {
                        case SDL_WINDOWEVENT_RESIZED:
                        case SDL_WINDOWEVENT_SIZE_CHANGED: {
                           ctx.width = event.window.data1;
                           ctx.height = event.window.data2;

                           SDL_DestroyTexture(backbuffer);
                           backbuffer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                                          SDL_TEXTUREACCESS_STREAMING, ctx.width, ctx.height);

                        }
                           break;
                        default: {
                           break;
                        }
                     }
                  }
                  default: {
                     break;
                  }
               }
            }

            void *pixels;
            int pitch;
            SDL_LockTexture(backbuffer, nullptr, &pixels, &pitch);
            auto cairo_surface = cairo_image_surface_create_for_data((uint8_t *) pixels, CAIRO_FORMAT_ARGB32, ctx.width,
                                                                     ctx.height, pitch);
            auto cairo = cairo_create(cairo_surface);

            auto temp_section = begin_temp_section(&ctx.temp);

            update(&ctx, timeline, cairo);

            end_temp_section(temp_section);

            SDL_UnlockTexture(backbuffer);

            SDL_RenderCopy(renderer, backbuffer, nullptr, nullptr);
            SDL_RenderPresent(renderer);
         }
      }

      if (renderer) SDL_DestroyRenderer(renderer);
      if (window) SDL_DestroyWindow(window);
   }

   SDL_Quit();
   return 0;
}