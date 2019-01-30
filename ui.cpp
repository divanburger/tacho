//
// Created by divan on 07/01/19.
//

#include "ui.h"
#include "cairo_helpers.h"

UIContext ui_context = {};

void ui_run(void (*update)(UIContext *, cairo_t *)) {
   if (SDL_Init(SDL_INIT_VIDEO) == 0) {
      SDL_Window *window = nullptr;
      SDL_Renderer *renderer = nullptr;

      ui_context.running = true;
      ui_context.width = 640;
      ui_context.height = 480;
      ui_context.dirty = true;

      arena_init(&ui_context.temp);

      window = SDL_CreateWindow("Tacho", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, ui_context.width, ui_context.height,
            SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

      if (window) renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);

      if (window && renderer) {
         auto backbuffer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, ui_context.width,
                                             ui_context.height);

         while (ui_context.running) {
            auto time_start = SDL_GetPerformanceCounter();
            ui_context.real_delta = (f64) (time_start - ui_context.time_counter) / SDL_GetPerformanceFrequency();
            if (ui_context.real_delta > 1.0) ui_context.real_delta = 1.0;

            auto old_mouse_pos = ui_context.mouse_pos;

            ui_context.time_counter = time_start;
            ui_context.proc_time += ui_context.real_delta;

            ui_context.mouse_delta = {};
            ui_context.mouse_delta_z = 0;
            ui_context.click_went_down = false;
            ui_context.click_went_up = false;
            ui_context.f64_click = false;

            SDL_Event event;
            while (SDL_PollEvent(&event)) {
               switch (event.type) {
                  case SDL_QUIT: {
                     ui_context.running = false;
                  }
                     break;
                  case SDL_MOUSEWHEEL: {
                     ui_context.mouse_delta_z += event.wheel.y;
                     ui_context.dirty = true;
                  }
                     break;
                  case SDL_MOUSEMOTION: {
                     ui_context.mouse_pos = {event.motion.x, event.motion.y};
                     ui_context.dirty = true;
                  }
                     break;
                  case SDL_MOUSEBUTTONDOWN: {
                     ui_context.mouse_pos = {event.button.x, event.button.y};
                     ui_context.click_mouse_pos = ui_context.mouse_pos;
                     ui_context.click = true;
                     ui_context.click_went_down = true;
                     ui_context.dirty = true;
                  }
                     break;
                  case SDL_MOUSEBUTTONUP: {
                     ui_context.mouse_pos = {event.button.x, event.button.y};
                     ui_context.click = false;
                     ui_context.click_went_up = true;

                     if (ui_context.last_click >= ui_context.proc_time - 0.3) {
                        ui_context.f64_click = true;
                     }
                     ui_context.last_click = ui_context.proc_time;
                     ui_context.dirty = true;
                  }
                     break;
                  case SDL_WINDOWEVENT: {
                     switch (event.window.event) {
                        case SDL_WINDOWEVENT_RESIZED:
                        case SDL_WINDOWEVENT_SIZE_CHANGED: {
                           ui_context.width = event.window.data1;
                           ui_context.height = event.window.data2;

                           SDL_DestroyTexture(backbuffer);
                           backbuffer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                                          SDL_TEXTUREACCESS_STREAMING, ui_context.width, ui_context.height);
                           ui_context.dirty = true;

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

            ui_context.mouse_delta = ui_context.mouse_pos - old_mouse_pos;

            bool dirty = ui_context.dirty;
            if (ui_context.dirty) {
               ui_context.dirty = false;

               void *pixels;
               int pitch;
               SDL_LockTexture(backbuffer, nullptr, &pixels, &pitch);
               auto cairo_surface = cairo_image_surface_create_for_data((u8 *) pixels, CAIRO_FORMAT_ARGB32,
                                                                        ui_context.width,
                                                                        ui_context.height, pitch);
               ui_context.cairo = cairo_create(cairo_surface);

               auto temp_section = begin_temp_section(&ui_context.temp);

               update(&ui_context, ui_context.cairo);
               cairo_destroy(ui_context.cairo);

               end_temp_section(temp_section);

               SDL_UnlockTexture(backbuffer);

               SDL_RenderCopy(renderer, backbuffer, nullptr, nullptr);
               SDL_RenderPresent(renderer);
            }
            auto time_end = SDL_GetPerformanceCounter();
            auto time_taken = ((f64) (time_end - time_start) / SDL_GetPerformanceFrequency());
//            printf("%1.6f\n", time_taken);

            auto wait_time = (int)((1000.0 / 100.0) - time_taken);
            if (!dirty && !ui_context.dirty && wait_time >= 1) SDL_Delay(wait_time);
         }
      }

      if (renderer) SDL_DestroyRenderer(renderer);
      if (window) SDL_DestroyWindow(window);
   }

   SDL_Quit();
}

irect ui_scrollable_begin(const char *name, i32rect rect, i32vec2 content, i32 scroll_rate) {
   auto cr = ui_context.cairo;

   auto scrollable = (UIScrollable*)ht_find(&ui_context.scrollables, (void*)name);
   if (!scrollable) {
      scrollable = alloc_type(&ui_context.permenant, UIScrollable);
      ht_add(&ui_context.scrollables, (void*)name, (void*)scrollable);
   }

   scrollable->rect = rect;
   scrollable->content = content;
   scrollable->hover = inside(rect, ui_context.mouse_pos);

   if (scrollable->hover) {
      scrollable->scroll -= ui_context.mouse_delta_z * scroll_rate;
   }

   i32 scroll_max = content.y - rect.h;
   if (scrollable->scroll > scroll_max) scrollable->scroll = scroll_max;
   if (scrollable->scroll < 0) scrollable->scroll = 0;

   cairo_save(cr);
   cairo_rectangle(cr, rect.x, rect.y, rect.w - 10, rect.h);
   cairo_clip(cr);

   return irect{rect.x, rect.y - scrollable->scroll, rect.w - 10, rect.h};
}

void ui_scrollable_end(const char *name) {
   auto cr = ui_context.cairo;

   auto scrollable = (UIScrollable*)ht_find(&ui_context.scrollables, (void*)name);
   assert(scrollable);
   cairo_restore(ui_context.cairo);

   auto content = scrollable->content;
   auto rect = scrollable->rect;

   i32 track_w = 10;
   i32 track_x = rect.x + rect.w - track_w;

   bool hovered = (inside(Rect(track_x, rect.y, track_w, rect.h), ui_context.mouse_pos));

   hovered ? cairo_set_source_rgb(cr, 0.3, 0.3, 0.3) : cairo_set_source_rgb(cr, 0.2, 0.2, 0.2);
   cairo_rectangle(cr, track_x, rect.y, track_w, rect.h);
   cairo_fill(cr);

   i32 track_start = 11;
   i32 track_height = rect.h - 22;

   i32 scroll_max = content.y - rect.h;
   f64 scroll_perc = min(content.y > 0 ? (f64)rect.h / content.y : 1.0, 1.0);
   f64 scroll_height = track_height * scroll_perc;

   if (hovered && ui_context.click) {
      auto perc = ((ui_context.mouse_pos.y - rect.y - track_start - scroll_height * 0.5) / (track_height - scroll_height));
      perc = clamp(perc, 0.0, 1.0);
      scrollable->scroll = (i32)(perc * scroll_max);
   }

   f64 scroll_start = track_height * (1.0 - scroll_perc) * ((f64)scrollable->scroll / scroll_max);

   cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
   cairo_rectangle(cr, track_x + 2, rect.y + track_start + scroll_start, 6, scroll_height);
   cairo_fill(cr);

   cairo_set_source_rgb(cr, 0.8, 0.8, 0.8);
   cairo_move_to(cr, track_x + 5, rect.y + 2);
   cairo_line_to(cr, track_x + 2, rect.y + 8);
   cairo_line_to(cr, track_x + 8, rect.y + 8);
   cairo_close_path(cr);
   cairo_fill(cr);

   cairo_move_to(cr, track_x + 5, rect.y + rect.h - 2);
   cairo_line_to(cr, track_x + 2, rect.y + rect.h - 8);
   cairo_line_to(cr, track_x + 8, rect.y + rect.h - 8);
   cairo_close_path(cr);
   cairo_fill(cr);
}
