//
// Created by divan on 07/01/19.
//

#pragma once

#include <SDL2/SDL.h>
#include <cairo/cairo.h>

#include "memory.h"

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

   bool click;
   bool click_went_up;
   bool click_went_down;
   bool double_click;
   double last_click;

   MemoryArena temp;
};

void ui_run(void (*update)(Context *, cairo_t *));