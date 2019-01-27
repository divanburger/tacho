//
// Created by divan on 07/01/19.
//

#pragma once

#include <SDL2/SDL.h>
#include <cairo/cairo.h>

#include "memory.h"
#include "math.h"

struct Context {
   cairo_t* cairo;

   bool running;
   bool dirty;
   int width;
   int height;

   uint64_t time_counter;
   double real_delta;
   double proc_time;

   i32vec2 mouse_delta;
   int mouse_delta_z;

   i32vec2 mouse_pos;
   i32vec2 click_mouse_pos;

   bool click;
   bool click_went_up;
   bool click_went_down;
   bool double_click;
   double last_click;

   MemoryArena temp;
};

void ui_run(void (*update)(Context *, cairo_t *));