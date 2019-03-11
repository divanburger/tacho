//
// Created by divan on 07/03/19.
//

#include "heap_ui.h"
#include "colour.h"
#include "cairo_helpers.h"

HeapUIState heap_state = {};

void heap_update_linear(UIContext *ctx, cairo_t *cr, i32rect area) {
   auto heap = heap_state.heap;

   heap_state.highlighted_page = nullptr;
   heap_state.highlighted_slot = -1;

   Colour chart_background = Colour{0.1, 0.1, 0.1};
   cairo_set_source_rgb(cr, chart_background);
   cairo_rectangle(cr, area.x, area.y, area.w, area.h);
   cairo_fill_preserve(cr);
   cairo_clip(cr);

   if (ctx->click_went_down) {
      heap_state.click_draw_page_start = heap_state.draw_page_start;
   }

   if (ctx->click) {
      heap_state.draw_page_start = (heap_state.click_draw_page_start +
                                    ((ctx->click_mouse_pos.x - ctx->mouse_pos.x) * heap_state.pages_per_width) /
                                    area.w);
   }

   f64 mouse_page = heap_state.draw_page_start + ((ctx->mouse_pos.x - area.x) * heap_state.pages_per_width) / area.w;

   bool inside_area = inside(area, ctx->mouse_pos.x, ctx->mouse_pos.y);
   if (inside_area) {
      f64 zoom_factor = (ctx->mouse_delta_z > 0 ? (1.0 / 1.1) : 1.1);
      int zoom_iters = (ctx->mouse_delta_z > 0 ? ctx->mouse_delta_z : -ctx->mouse_delta_z);
      for (i32 i = 0; i < zoom_iters; i++) {
         heap_state.pages_per_width *= zoom_factor;
      }
      heap_state.draw_page_start = (mouse_page - ((ctx->mouse_pos.x - area.x) * heap_state.pages_per_width) / area.w);
   }

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

      u64 page_id = page->id;

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

         if (x <= ctx->mouse_pos.x && x + page_width > ctx->mouse_pos.x) {
            heap_state.highlighted_page = page;
            i64 slot_index = (ctx->mouse_pos.y - 2) / 2;
            heap_state.highlighted_slot = (slot_index >= 0 && slot_index <= 408) ? slot_index : -1;
         }

         auto colour = Colour{0.33, 0.67, 1.00};

         bool page_referenced = false;
         double brightness = 1.0;
         if (heap_state.active_page) {
            page_referenced = ht_exist(&heap_state.page_references, page_id);
            brightness = page_referenced ? 0.3 : 0.1;
         }

         if (page_width < 2.0) {
            cairo_set_source_rgb(cr, colour * (page->slot_count / 408.0) * brightness);
            cairo_rectangle(cr, x, 2, page_width, 408 * 2);
            cairo_fill(cr);
         } else {
            for (i32 slot_index = 0; slot_index < 408; slot_index++) {
               Object *slot = page->slots[slot_index];
               if (slot) {
                  auto slot_brightness = brightness;

                  if (page_referenced) {
                     if (heap_state.active_page == page && heap_state.active_slot == slot_index) {
                        slot_brightness = 1.0;
                     }

                     ArrayListCursor cursor = {};
                     while (arl_cursor_step(&heap_state.down_references, &cursor)) {
                        if (slot == *arl_cursor_get<Object *>(cursor)) {
                           slot_brightness = 0.7;
                           break;
                        }
                     }

                     cursor = {};
                     while (arl_cursor_step(&heap_state.up_references, &cursor)) {
                        if (slot == *arl_cursor_get<Object *>(cursor)) {
                           slot_brightness = 0.7;
                           break;
                        }
                     }
                  }

                  auto slot_colour = Colour{0.33, 0.67, 1.00};

                  if (heap_state.slot_view == SLOT_VIEW_TYPE) {
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
                  } else if (heap_state.slot_view == SLOT_VIEW_OLD) {
                     if (slot->flags & OBJFLAG_OLD) {
                        slot_colour = Colour{1.00, 0.67, 0.33};
                     }
                  }

                  cairo_set_source_rgb(cr, slot_colour * slot_brightness);
                  cairo_rectangle(cr, x, slot_index * 2 + 2, page_width, 2);
                  cairo_fill(cr);
               }
            }
         }
      }
   }

   if (ctx->click_went_down) {
      heap_state.active_page = heap_state.highlighted_page;
      heap_state.active_slot = heap_state.highlighted_slot;

      arena_temp_end(heap_state.active_temp_section);
      arena_temp_begin(&heap_state.active_allocator);

      auto allocator = (Allocator *) &heap_state.active_allocator;
      ht_init(&heap_state.page_references, allocator);
      arl_init(&heap_state.up_references, allocator);
      arl_init(&heap_state.down_references, allocator);

      auto object = heap_get_object(HeapLocation{heap_state.active_page, heap_state.active_slot});
      if (object) {
         ArrayListCursor cursor = {};
         while (arl_cursor_step(&object->references, &cursor)) {
            auto address = *arl_cursor_get<u64>(cursor);
            auto location = heap_find_object(heap, address);
            auto other = heap_get_object(location);

            if (location.page && !ht_exist(&heap_state.page_references, location.page->id)) {
               ht_add(&heap_state.page_references, location.page->id, location.page);
            }
            arl_push(&heap_state.down_references, other);
         }

         cursor = {};
         while (arl_cursor_step(&object->referenced_by, &cursor)) {
            auto other = *arl_cursor_get<Object*>(cursor);
            auto location = heap_find_object(heap, other->address);

            if (location.page && !ht_exist(&heap_state.page_references, location.page->id)) {
               ht_add(&heap_state.page_references, location.page->id, location.page);
            }
            arl_push(&heap_state.up_references, other);
         }
      }
   }

   cairo_reset_clip(cr);
}

void heap_update_slot_info(UIContext *ctx, cairo_t *cr, i32rect area) {
   double y = 0;

   cairo_font_extents_t font_extents;
   cairo_font_extents(cr, &font_extents);

   if (heap_state.highlighted_page && heap_state.highlighted_slot >= 0) {
      Page *page = heap_state.highlighted_page;
      Object *object = page->slots[heap_state.highlighted_slot];

      if (object) {
         if (str_nonblank(object->value)) {
            cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
            cairo_move_to(cr, area.x + 4, area.y + y + font_extents.ascent);
            cairo_show_text(cr, object->value.data);
            y += font_extents.height;
            y += font_extents.height;
         }

         ArrayListCursor cursor = {};
         while (arl_cursor_step(&object->references, &cursor)) {
            u64 address = *arl_cursor_get<u64>(cursor);

            String address_str = str_print(&ctx->temp, "%lu", address);

            cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
            cairo_move_to(cr, area.x + 4, area.y + y + font_extents.ascent);
            cairo_show_text(cr, address_str.data);
            y += font_extents.height;
         }

         y += 10;

         cursor = {};
         while (arl_cursor_step(&object->referenced_by, &cursor)) {
            Object *other = *arl_cursor_get<Object*>(cursor);

            String address_str = str_print(&ctx->temp, "%lu", other->address);

            cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
            cairo_move_to(cr, area.x + 4, area.y + y + font_extents.ascent);
            cairo_show_text(cr, address_str.data);
            y += font_extents.height;

            if (str_nonblank(other->value)) {
               cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
               cairo_move_to(cr, area.x + 4, area.y + y + font_extents.ascent);
               cairo_show_text(cr, other->value.data);
            }

            y += font_extents.height;
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

      u64 page_id = page->id;
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

   cairo_set_line_width(cr, 1.0);

   if (heap_state.page_view == PAGE_VIEW_NONE) {
      heap_state.page_view = PAGE_VIEW_LINEAR;

      heap_state.active_allocator = arena_make();
   }

   if (ctx->key_went_up[SDL_SCANCODE_F1]) heap_state.page_view = PAGE_VIEW_LINEAR;
   if (ctx->key_went_up[SDL_SCANCODE_F2]) heap_state.page_view = PAGE_VIEW_MAP;

   if (ctx->key_went_up[SDL_SCANCODE_F5]) heap_state.slot_view = SLOT_VIEW_TYPE;
   if (ctx->key_went_up[SDL_SCANCODE_F6]) heap_state.slot_view = SLOT_VIEW_OLD;

   auto area = Rect(0, 0, ctx->width - 400, ctx->height);

   switch (heap_state.page_view) {
      case PAGE_VIEW_LINEAR:
         heap_update_linear(ctx, cr, area);
         break;
      case PAGE_VIEW_MAP:
         heap_update_map(ctx, cr);
         break;
   }

   heap_update_slot_info(ctx, cr, Rect(ctx->width - 400, 0, 400, ctx->height));
}