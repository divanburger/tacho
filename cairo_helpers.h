//
// Created by divan on 12/01/19.
//

#pragma once

inline void cairo_set_source_rgb(cairo_t *cr, Colour c) {
   cairo_set_source_rgb(cr, c.r, c.g, c.b);
}