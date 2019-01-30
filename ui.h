//
// Created by divan on 07/01/19.
//

#pragma once

#include <SDL2/SDL.h>
#include <cairo/cairo.h>

#include "memory.h"
#include "math.h"
#include "hash_table.h"

struct UIContext {
   cairo_t *cairo;

   bool running;
   bool dirty;
   int width;
   int height;

   u64 time_counter;
   f64 real_delta;
   f64 proc_time;

   i32vec2 mouse_delta;
   int mouse_delta_z;

   i32vec2 mouse_pos;
   i32vec2 click_mouse_pos;

   bool click;
   bool click_went_up;
   bool click_went_down;
   bool f64_click;
   f64 last_click;

   MemoryArena temp;
   MemoryArena permenant;

   HashTable scrollables;
};

extern UIContext ui_context;

struct UIScrollable {
   i32 scroll;
   i32rect rect;
   i32vec2 content;
   bool hover;
};

void ui_run(void (*update)(UIContext *, cairo_t *));

irect ui_scrollable_begin(const char *name, i32rect rect, i32vec2 content, i32 scroll_rate = 30);
void ui_scrollable_end(const char *name);

