//
// Created by divan on 07/03/19.
//

#include "heap_ui.h"
#include "colour.h"
#include "cairo_helpers.h"

HeapUIState heap_state = {};

void heap_update_linear(UIContext *ctx, cairo_t *cr) {
   auto heap = heap_state.heap;

   i32rect area = Rect(0, 0, ctx->width, ctx->height);

   if (ctx->click_went_down) {
      heap_state.click_draw_page_start = heap_state.draw_page_start;
   }

   if (ctx->click) {
      heap_state.draw_page_start = (heap_state.click_draw_page_start +
                                    ((ctx->click_mouse_pos.x - ctx->mouse_pos.x) * heap_state.pages_per_width) /
                                    area.w);
   }

   f64 mouse_page = heap_state.draw_page_start + ((ctx->mouse_pos.x - area.x) * heap_state.pages_per_width) / area.w;

   f64 zoom_factor = (ctx->mouse_delta_z > 0 ? (1.0 / 1.1) : 1.1);
   int zoom_iters = (ctx->mouse_delta_z > 0 ? ctx->mouse_delta_z : -ctx->mouse_delta_z);
   for (i32 i = 0; i < zoom_iters; i++) {
      heap_state.pages_per_width *= zoom_factor;
   }
   heap_state.draw_page_start = (mouse_page - ((ctx->mouse_pos.x - area.x) * heap_state.pages_per_width) / area.w);

   if (heap_state.draw_page_start < 0.0) {
      heap_state.draw_page_start = 0.0;
   }

   if (heap_state.pages_per_width <= 0) {
      heap_state.pages_per_width = 100;
   }

   f64 page_width = area.w / heap_state.pages_per_width;

   u64 last_page_id = 0;
   i64 offset = 0;

   for (size_t page_index = 0; page_index < heap->page_count; page_index++) {
      auto page = heap->pages[page_index];

      u64 page_id = (page->address >> 14U);

      i64 skip = 0;
      if (page_id > last_page_id + 10000) {
         skip = (i64) (page_id - last_page_id) - 1000;
      }

      if (skip > 0) {
         double x1 = (last_page_id + 1 + offset - heap_state.draw_page_start) * page_width;
         double x2 = (page_id - 1 + offset - skip - heap_state.draw_page_start) * page_width;

         x1 = (x1 + x2) * 0.5 - 10;
         x2 = x1 + 10;

         if (x1 < ctx->width && x2 >= 0.0) {
            cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
            cairo_rectangle(cr, x1, 2, x2 - x1, 411);
            cairo_fill(cr);
         }
      }

      last_page_id = page_id;
      offset -= skip;

      double x = (page_id + offset - heap_state.draw_page_start) * page_width;

      if (x < ctx->width && x + page_width >= 0.0) {
         auto colour = Colour{0.33, 0.67, 1.00};
         cairo_set_source_rgb(cr, colour * (page->slot_count / 408.0));

         if (page_width < 2.0) {
            cairo_rectangle(cr, x, 2, page_width, 408);
            cairo_fill(cr);
         } else {
            for (i32 slot_index = 0; slot_index < 408; slot_index++) {
               Object *slot = page->slots[slot_index];
               if (slot) {

                  auto slot_colour = Colour{0.33, 0.67, 1.00};
                  switch (slot->type) {
                     case HO_ROOT:
                     case HO_CLASS:
                     case HO_MODULE:
                     case HO_ICLASS:
                     case HO_IMEMO:
                        slot_colour = Colour{1.00, 0.33, 0.67};
                        break;
                     case HO_STRING:
                        slot_colour = Colour{0.33, 1.00, 0.67};
                        break;
                     case HO_HASH:
                        slot_colour = Colour{1.00, 0.67, 0.33};
                        break;
                     default:
                        break;
                  }

                  cairo_set_source_rgb(cr, slot_colour);
                  cairo_rectangle(cr, x, slot_index + 2, page_width, 1);
                  cairo_fill(cr);
               }
            }
         }
      }
   }
}

void heap_update_map(UIContext *ctx, cairo_t *cr) {
   auto heap = heap_state.heap;

   i64 x = 0;
   i64 y = 0;

   u64 last_row = 0;
   u64 offset = 0;

   u64 page_width = 8;
   u64 page_height = 8;
   u64 pages_per_row = ctx->width / page_width;

   for (size_t page_index = 0; page_index < heap->page_count; page_index++) {
      auto page = heap->pages[page_index];

      u64 page_id = (page->address >> 14U);
      u64 row = page_id / pages_per_row;

      if (row > last_row + 5) {
         offset += (row - last_row - 3) * page_height;

         cairo_set_source_rgb(cr, 0.3, 0.3, 0.3);
         cairo_rectangle(cr, 0.0, round((last_row + 1.5) * page_height) - 0.5, ctx->width, 1.0);
         cairo_fill(cr);
      }

      last_row = row;

      x = (page_id % pages_per_row) * page_width;
      y = row * page_height - offset;

      if (y < ctx->height) {
         auto colour = Colour{0.33, 0.67, 1.00};
         cairo_set_source_rgb(cr, colour * (page->slot_count / 408.0));

         cairo_rectangle(cr, x, y, page_width - 1, page_height - 1);
         cairo_fill(cr);
      }
   }
}

void heap_update(UIContext *ctx, cairo_t *cr) {
   cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
   cairo_paint(cr);

   cairo_select_font_face(cr, "Source Code Pro", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
   cairo_set_font_size(cr, 10);

   cairo_font_extents_t font_extents;
   cairo_font_extents(cr, &font_extents);

   cairo_set_line_width(cr, 1.0);

   heap_state.page_view = PAGE_VIEW_LINEAR;

   switch (heap_state.page_view) {
      case PAGE_VIEW_MAP:
         heap_update_map(ctx, cr);
         break;
      case PAGE_VIEW_LINEAR:
         heap_update_linear(ctx, cr);
         break;
   }

}