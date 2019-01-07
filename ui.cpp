//
// Created by divan on 07/01/19.
//

#include "ui.h"

void ui_run(void (*update)(Context*, cairo_t*)) {
   if (SDL_Init(SDL_INIT_VIDEO) == 0) {
      SDL_Window *window = nullptr;
      SDL_Renderer *renderer = nullptr;

      Context ctx = {};
      ctx.running = true;
      ctx.width = 640;
      ctx.height = 480;

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

            update(&ctx, cairo);

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
}