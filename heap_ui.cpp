//
// Created by divan on 07/03/19.
//

#include "heap_ui.h"
#include "colour.h"
#include "cairo_helpers.h"

HeapUIState heap_state = {};

void add_objects_references(Object *object) {
   auto heap = heap_state.heap;

   ArrayListCursor cursor = {};
//   while (arl_cursor_step(&object->references, &cursor)) {
//      auto address = *arl_cursor_get<u64>(cursor);
//      auto location = heap_find_object(heap, address);
//      auto other = heap_get_object(location);
//
//      if (location.page && !ht_exist(&heap_state.page_references, location.page->id)) {
//         ht_add(&heap_state.page_references, location.page->id, location.page);
//      }
//      arl_push(&heap_state.down_references, other);
//   }
//
//   cursor = {};
   while (arl_cursor_step(&object->referenced_by, &cursor)) {
      auto other = *arl_cursor_get<Object *>(cursor);
      auto location = heap_find_object(heap, other->address);

      if (location.page && !ht_exist(&heap_state.page_references, location.page->id)) {
         ht_add(&heap_state.page_references, location.page->id, location.page);
      }
      arl_push(&heap_state.up_references, other);
   }
}

void make_highlighted_slot_active() {
   if (!heap_state.highlighted.page) return;

   heap_state.active_start = heap_state.highlighted;
   heap_state.active_end = heap_state.highlighted;

   arena_temp_end(heap_state.active_temp_section);
   arena_temp_begin(&heap_state.active_allocator);

   auto allocator = (Allocator *) &heap_state.active_allocator;
   ht_init(&heap_state.page_references, allocator);
   arl_init(&heap_state.up_references, allocator);
   arl_init(&heap_state.down_references, allocator);

   if (heap_state.active_start.slot_index >= 0) {
      auto object = heap_get_object(heap_state.active_start);
      if (object) add_objects_references(object);
   } else {
      for (i16 slot_index = 0; slot_index < 408; slot_index++) {
         auto object = heap_state.active_start.page->slots[slot_index];
         if (object) add_objects_references(object);
      }
   }
}

void heap_update_linear(UIContext *ctx, cairo_t *cr, i32rect area) {
   auto heap = heap_state.heap;

   double minimap_height = 30;
   i32rect top_area = area;
   top_area.h -= minimap_height;
   i32rect minimap_area = Rect(area.x, area.y + area.h - minimap_height, area.w, minimap_height);

   Colour chart_background = Colour{0.1, 0.1, 0.1};
   cairo_set_source_rgb(cr, chart_background);
   cairo_rectangle(cr, area.x, area.y, area.w, area.h);
   cairo_fill_preserve(cr);
   cairo_clip(cr);

   if (heap_state.pages_per_width <= 0) {
      heap_state.pages_per_width = 100;
   }

   f64 zoom = top_area.w / heap_state.pages_per_width;

   if (ctx->click_went_down[0]) {
      heap_state.click_scroll_x = heap_state.scroll_x;
      heap_state.click_scroll_y = heap_state.scroll_y;
   }

   if (ctx->click[0] && inside(top_area, ctx->click_mouse_pos[0])) {
      heap_state.scroll_x = heap_state.click_scroll_x + (ctx->click_mouse_pos[0].x - ctx->mouse_pos.x) / zoom;
      heap_state.scroll_y = heap_state.click_scroll_y + (ctx->click_mouse_pos[0].y - ctx->mouse_pos.y) / zoom;
   }

   f64 page_header_height = 32.0;

   f64 mouse_page = heap_state.scroll_x + (ctx->mouse_pos.x - top_area.x) / zoom;
   f64 mouse_slot = heap_state.scroll_y + (ctx->mouse_pos.y - top_area.y - page_header_height) / zoom;

   bool inside_top_area = inside(top_area, ctx->mouse_pos);
   if (inside_top_area) {
      f64 zoom_factor = (ctx->mouse_delta_z > 0 ? 1.1 : (1.0 / 1.1));
      int zoom_iters = (ctx->mouse_delta_z > 0 ? ctx->mouse_delta_z : -ctx->mouse_delta_z);
      for (i32 i = 0; i < zoom_iters; i++) {
         zoom *= zoom_factor;
      }

      heap_state.pages_per_width = top_area.w / zoom;
      heap_state.scroll_x = mouse_page - (ctx->mouse_pos.x - top_area.x) / zoom;
      heap_state.scroll_y = mouse_slot - (ctx->mouse_pos.y - top_area.y - page_header_height) / zoom;
   }

   if (heap_state.scroll_x < 0.0) {
      heap_state.scroll_x = 0.0;
   }

   if (heap_state.scroll_y < 0.0) {
      heap_state.scroll_y = 0.0;
   }

   f64 page_width = zoom;
   f64 slot_height = zoom;

   for (size_t page_index = 0; page_index < heap->page_count; page_index++) {
      auto page = heap->pages[page_index];

      double x = (page->xpos - heap_state.scroll_x) * page_width;

      if (x < ctx->width && x + page_width >= 0.0) {

         if (inside_top_area && x <= ctx->mouse_pos.x && x + page_width > ctx->mouse_pos.x) {
            i64 slot_index = (double)(ctx->mouse_pos.y - page_header_height) / slot_height + heap_state.scroll_y;
            if ((slot_index >= 0 && slot_index <= 408)) {
               heap_state.highlighted_new = HeapLocation{page, (i16)slot_index};
            } else if (slot_index < 0) {
               heap_state.highlighted_new = HeapLocation{page, SLOT_ALL};
            }
         }

         auto colour = Colour{0.33, 0.67, 1.00};

         bool page_referenced = false;
         double brightness = 0.6;
         if (heap_state.active_start.page) {
            page_referenced = heap_state.active_start.page == page || ht_exist(&heap_state.page_references, page->id);
            brightness = page_referenced ? 0.3 : 0.1;
         }

         cairo_set_source_rgb(cr, lerp(brightness, chart_background, colour * (page->slot_count / 408.0)));

         if (page_width < 1.5) {
            cairo_rectangle(cr, x, 0, page_width, page_header_height + 408 * slot_height - heap_state.scroll_y);
            cairo_fill(cr);
         } else {
            cairo_rectangle(cr, x, 0, page_width, page_header_height - heap_state.scroll_y);
            cairo_fill(cr);

            for (i32 slot_index = 0; slot_index < 408; slot_index++) {
               Object *slot = page->slots[slot_index];
               if (slot) {
                  double y = page_header_height + slot_height * slot_index - heap_state.scroll_y * zoom;
                  auto slot_brightness = brightness;

                  bool slot_referenced = false;
                  if (page_referenced) {
                     if (heap_state.active_start.page == page &&
                         (heap_state.active_start.slot_index == SLOT_ALL || heap_state.active_start.slot_index == slot_index)) {
                        slot_brightness = 1.0;
                        slot_referenced = true;
                     }

                     if (!slot_referenced) {
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
                  }

                  if (heap_state.highlighted.page == page && heap_state.highlighted.slot_index == slot_index) {
                     slot_brightness = 1.0;
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

                  cairo_set_source_rgb(cr, lerp(slot_brightness, chart_background, slot_colour));
                  cairo_rectangle(cr, x, y, page_width, slot_height);
                  cairo_fill(cr);
               }
            }
         }
      }
   }

   double factor = (double)minimap_area.w / heap->max_xpos;

   cairo_set_source_rgb(cr, Colour{0.3, 0.3, 0.3});
   cairo_rectangle(cr, heap_state.scroll_x * factor, minimap_area.y, heap_state.pages_per_width * factor, minimap_height);
   cairo_fill(cr);

   bool referenced = false;
   bool in_view = false;
   i64 last_x_pixel = -1;

   for (size_t page_index = 0; page_index < heap->page_count; page_index++) {
      auto page = heap->pages[page_index];
      double x = page->xpos * factor;
      i64 x_pixel = (i64)x;

      if ((page->xpos >= heap_state.scroll_x) && (page->xpos <= heap_state.scroll_x + heap_state.pages_per_width)) {
         in_view = true;
      }

      if (heap_state.active_start.page) {
         referenced = referenced || heap_state.active_start.page == page || ht_exist(&heap_state.page_references, page->id);
      }

      if (last_x_pixel != x_pixel) {
         double brightness = in_view ? 1.0 : 0.7;
         if (heap_state.active_start.page) {
            brightness *= referenced ? 1.0 : 0.6;
         }
         cairo_set_source_rgb(cr, lerp(brightness, chart_background, Colour{1.0, 1.0, 1.0}));
         cairo_rectangle(cr, x_pixel, minimap_area.y + 4, 1, minimap_height - 8);
         cairo_fill(cr);

         referenced = false;
         in_view = false;
         last_x_pixel = x_pixel;
      }
   }

   cairo_reset_clip(cr);
}

void heap_update_slot_info(UIContext *ctx, cairo_t *cr, i32rect area) {
   auto heap = heap_state.heap;

   double y = 0;

   cairo_font_extents_t font_extents;
   cairo_font_extents(cr, &font_extents);

   Colour hover_colour = Colour{0.2, 0.3, 0.4};
   Colour heading_colour = Colour{1.0, 1.0, 0.9};
   Colour text_colour = Colour{0.8, 0.8, 0.8};

   auto scroll = ui_scrollable_begin("info", area);

   bool inside_info = inside(scroll.screen_area, ctx->mouse_pos);

   if (heap_state.active_start.page && heap_state.active_start.slot_index >= 0) {
      Object *object = heap_get_object(heap_state.active_start);

      if (object) {
         cairo_set_source_rgb(cr, heading_colour);
         cairo_move_to(cr, scroll.rect.x + 4, scroll.rect.y + y + font_extents.ascent);
         cairo_show_text(cr, "Information");
         y += font_extents.height;

         cairo_move_to(cr, scroll.rect.x + 4, scroll.rect.y + y + font_extents.ascent);
         if (object->type == HO_IMEMO) {
            String type_str = str_print(&ctx->temp, "%s: %s", object_type_names[object->type], object_memo_type_names[object->imemo_type]);
            cairo_show_text(cr, type_str.data);
         } else {
            cairo_show_text(cr, object_type_names[object->type]);
         }
         y += font_extents.height;

         String address_str = str_print(&ctx->temp, "%lu", object->address);
         cairo_move_to(cr, scroll.rect.x + 4, scroll.rect.y + y + font_extents.ascent);
         cairo_show_text(cr, address_str.data);
         y += font_extents.height;

         if (str_nonblank(object->value)) {
            cairo_set_source_rgb(cr, text_colour);
            cairo_move_to(cr, scroll.rect.x + 4, scroll.rect.y + y + font_extents.ascent);
            cairo_show_text(cr, object->value.data);
            y += font_extents.height;
         }

         y += 10;

         cairo_set_source_rgb(cr, heading_colour);
         cairo_move_to(cr, scroll.rect.x + 4, scroll.rect.y + y + font_extents.ascent);
         cairo_show_text(cr, "References");
         y += font_extents.height;

         ArrayListCursor cursor = {};
         while (arl_cursor_step(&object->references, &cursor)) {
            u64 address = *arl_cursor_get<u64>(cursor);
            auto location = heap_find_object(heap, address);
            Object* ref_object = heap_get_object(location);

            auto height = font_extents.height;
            bool hovered = inside_info && (ctx->mouse_pos.y + scroll.mouse_offset.y >= y && ctx->mouse_pos.y + scroll.mouse_offset.y < y + height);
            if (hovered) {
               cairo_set_source_rgb(cr, hover_colour);
               cairo_rectangle(cr, scroll.rect.x, scroll.rect.y + y, scroll.rect.w, height);
               cairo_fill(cr);

               heap_state.highlighted_new = location;
            }

            double x = scroll.rect.x + 4;
            String address_str = str_print(&ctx->temp, "%lu", address);
            cairo_set_source_rgb(cr, text_colour);
            cairo_move_to(cr, x, scroll.rect.y + y + font_extents.ascent);
            cairo_show_text(cr, address_str.data);

            x += 100;
            if (ref_object) {
               cairo_move_to(cr, x, scroll.rect.y + y + font_extents.ascent);
               cairo_show_text(cr, object_type_names[ref_object->type]);

               x += 60;
               if (str_nonblank(ref_object->value)) {
                  cairo_move_to(cr, x, scroll.rect.y + y + font_extents.ascent);
                  cairo_show_text(cr, ref_object->value.data);
               } else if (ref_object->type == HO_IMEMO) {
                  cairo_move_to(cr, x, scroll.rect.y + y + font_extents.ascent);
                  cairo_show_text(cr, object_memo_type_names[ref_object->imemo_type]);
               }
            } else {
               cairo_move_to(cr, x, scroll.rect.y + y + font_extents.ascent);
               cairo_show_text(cr, "<can not find object in heap>");
            }

            y += height;
         }

         y += 10;

         cairo_set_source_rgb(cr, heading_colour);
         cairo_move_to(cr, scroll.rect.x + 4, scroll.rect.y + y + font_extents.ascent);
         cairo_show_text(cr, "Referenced by");
         y += font_extents.height;

         cursor = {};
         while (arl_cursor_step(&object->referenced_by, &cursor)) {
            Object *ref_object = *arl_cursor_get<Object *>(cursor);
            auto location = heap_find_object(heap, ref_object->address);

            auto height = font_extents.height;
            bool hovered = inside_info && (ctx->mouse_pos.y + scroll.mouse_offset.y >= y && ctx->mouse_pos.y + scroll.mouse_offset.y < y + height);
            if (hovered) {
               cairo_set_source_rgb(cr, hover_colour);
               cairo_rectangle(cr, scroll.rect.x, scroll.rect.y + y, scroll.rect.w, height);
               cairo_fill(cr);

               heap_state.highlighted_new = location;
            }

            double x = scroll.rect.x + 4;
            String address_str = str_print(&ctx->temp, "%lu", ref_object->address);
            cairo_set_source_rgb(cr, text_colour);
            cairo_move_to(cr, scroll.rect.x + 4, scroll.rect.y + y + font_extents.ascent);
            cairo_show_text(cr, address_str.data);

            x += 100;
            cairo_move_to(cr, x, scroll.rect.y + y + font_extents.ascent);
            cairo_show_text(cr, object_type_names[ref_object->type]);

            x += 60;
            if (str_nonblank(ref_object->value)) {
               cairo_move_to(cr, x, scroll.rect.y + y + font_extents.ascent);
               cairo_show_text(cr, ref_object->value.data);
            } else if (ref_object->type == HO_IMEMO) {
               cairo_move_to(cr, x, scroll.rect.y + y + font_extents.ascent);
               cairo_show_text(cr, object_memo_type_names[ref_object->imemo_type]);
            }

            y += height;
         }
      }
   }

   ui_scrollable_end("info", {0, (int)ceil(y)});
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

   heap_state.highlighted_new = HeapLocation{nullptr, SLOT_ALL};
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

   heap_state.highlighted = heap_state.highlighted_new;
   if (ctx->click_went_up[0] && !ctx->click_drag[0]) {
      make_highlighted_slot_active();
      ctx->dirty = true;
   }
   if (ctx->click_went_up[2]) {
      heap_state.active_start = HeapLocation{nullptr, SLOT_ALL};
      heap_state.active_end = HeapLocation{nullptr, SLOT_ALL};
      ctx->dirty = true;
   }
}