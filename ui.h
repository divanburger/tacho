//
// Created by divan on 07/01/19.
//

#pragma once

#include <SDL2/SDL.h>
#include <cairo/cairo.h>

#include "memory.h"
#include "math.h"
#include "hash_table.h"

#define MAX_KEYS 512
#define MAX_MOUSE_BUTTONS 3

struct UIScrollable {
   i32 scroll;
   i32rect rect;
   i32vec2 content;
   bool hover;
};

struct UIContext {
   cairo_t *cairo;

   bool running;
   volatile bool dirty;
   int width;
   int height;

   u64 time_counter;
   f64 real_delta;
   f64 proc_time;

   i32vec2 mouse_delta;
   int mouse_delta_z;

   i32vec2 mouse_pos;
   i32vec2 click_mouse_pos[MAX_MOUSE_BUTTONS];
   bool click[MAX_MOUSE_BUTTONS];
   bool click_went_up[MAX_MOUSE_BUTTONS];
   bool click_went_down[MAX_MOUSE_BUTTONS];
   bool click_drag[MAX_MOUSE_BUTTONS];
   bool double_click[MAX_MOUSE_BUTTONS];
   f64 last_click[MAX_MOUSE_BUTTONS];

   bool key_down[MAX_KEYS];
   bool key_went_down[MAX_KEYS];
   bool key_went_up[MAX_KEYS];

   MemoryArena temp;
   MemoryArena permanent;

   HashTable<void*> scrollables;
};

struct ScrollState {
   i32rect rect;
   i32rect screen_area;
   i32vec2 mouse_offset;
};

extern UIContext ui_context;

void ui_run(void (*update)(UIContext *, cairo_t *));

ScrollState ui_scrollable_begin(const char *name, i32rect rect, i32vec2 content = {-1, -1}, i32 scroll_rate = 30);
void ui_scrollable_end(const char *name, i32vec2 content = {-1, -1});

