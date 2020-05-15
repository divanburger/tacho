//
// Created by divan on 07/03/19.
//

#pragma once

#include "ui.h"
#include "heap.h"

#include <cairo/cairo.h>

#define SLOT_ALL -1

enum PageViewType {
   PAGE_VIEW_NONE,
   PAGE_VIEW_LINEAR,
   PAGE_VIEW_MAP
};

enum SlotViewType {
   SLOT_VIEW_TYPE,
   SLOT_VIEW_OLD
};

struct HeapUIState {
   Heap *heap;
   PageViewType page_view;
   SlotViewType slot_view;

   f64 pages_per_width;

   f64 scroll_x;
   f64 scroll_y;
   f64 click_scroll_x;
   f64 click_scroll_y;

   HeapLocation highlighted;
   HeapLocation highlighted_new;
   HeapLocation active_start;
   HeapLocation active_end;

   ArenaAllocator active_allocator;
   ArenaTempSection active_temp_section;

   HashTable<u64> page_references;
   ArrayList<Object*> up_references;
   ArrayList<Object*> down_references;
};

extern HeapUIState heap_state;

void heap_update(UIContext *ctx, cairo_t *cr);