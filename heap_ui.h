//
// Created by divan on 07/03/19.
//

#pragma once

#include "ui.h"
#include "heap.h"

#include <cairo/cairo.h>

enum PageViewType {
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

   f64 click_draw_page_start;
   f64 draw_page_start;
   f64 pages_per_width;

   Page *highlighted_page;
   i16 highlighted_slot;
};

extern HeapUIState heap_state;

void heap_update(UIContext *ctx, cairo_t *cr);