//
// Created by divan on 24/01/19.
//

#pragma once

#include "util.h"
#include "string.h"
#include "timeline.h"
#include "files.h"
#include "ui.h"
#include "math.h"
#include "colour.h"
#include "cairo_helpers.h"
#include "hash_table.h"

struct TimelineUIState {
   MemoryArena memory;

   DirectoryList profile_file_list;
   double last_file_check;

   int64_t click_draw_y;
   int64_t click_draw_start_time;

   int64_t draw_y;
   int64_t draw_start_time;
   double draw_time_width;

   Timeline timeline;
   TimelineThread *thread;

   TimelineEvent *highlighted_event;
   TimelineEvent *active_event;

   TimelineMethod *highlighted_method;
   TimelineMethod *active_method;

   TimelineStatistics selection_statistics;

   String watch_path;
   int64_t watch_panel_width;
   bool watch_panel_open;

   MethodSortOrder method_sort_order_highlighted;
   MethodSortOrder method_sort_order_active;
   int64_t methods_scroll;

   double tooltip_w = 0.0;
   double tooltip_h = 0.0;
};

extern TimelineUIState state;

String timeline_scaled_time_str(MemoryArena *arena, int64_t time);
String timeline_full_time_str(MemoryArena *arena, int64_t time);
void timeline_chart_update(Context *ctx, cairo_t *cr, Timeline *timeline, i32rect area);
void timeline_methods_update(Context *ctx, cairo_t *cr, Timeline *timeline, i32rect area);
void timeline_update(Context *ctx, cairo_t *cr, Timeline *timeline, i32rect area);
void update(Context *ctx, cairo_t *cr);