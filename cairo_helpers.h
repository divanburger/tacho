//
// Created by divan on 12/01/19.
//

#pragma once

inline void cairo_set_source_rgb(cairo_t *cr, Colour c) {
   cairo_set_source_rgb(cr, c.r, c.g, c.b);
}

inline void cairo_set_source_rgba(cairo_t *cr, Colour c, double alpha) {
   cairo_set_source_rgba(cr, c.r, c.g, c.b, alpha);
}

inline void cairo_rectangle(cairo_t *cr, irect r) {
   cairo_rectangle(cr, r.x, r.y, r.w, r.h);
}