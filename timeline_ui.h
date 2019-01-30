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
#include "array.h"
#include "array_list.h"

struct TimelineUIState {
   MemoryArena memory;

   DirectoryList profile_file_list;
   f64 last_file_check;

   i64 click_draw_y;
   i64 click_draw_start_time;

   i64 draw_y;
   i64 draw_start_time;
   f64 draw_time_width;

   ArrayList<Timeline> timelines;
   HashTable<String> timelines_table;
   Timeline* highlighted_timeline;
   Timeline* active_timeline;

   TimelineEvent *highlighted_event;
   TimelineEvent *active_event;

   TimelineMethod *highlighted_method;
   TimelineMethod *active_method;

   TimelineStatistics selection_statistics;

   String watch_path;
   i64 watch_panel_width;
   bool watch_panel_open;

   MethodSortOrder method_sort_order_highlighted;
   MethodSortOrder method_sort_order_active;

   f64 tooltip_w = 0.0;
   f64 tooltip_h = 0.0;
};

extern TimelineUIState state;

void switch_timeline(Timeline* timeline);

String timeline_scaled_time_str(MemoryArena *arena, i64 time);
String timeline_full_time_str(MemoryArena *arena, i64 time);
void timeline_chart_update(UIContext *ctx, cairo_t *cr, Timeline *timeline, i32rect area);
void timeline_methods_update(UIContext *ctx, cairo_t *cr, Timeline *timeline, i32rect area);
void timeline_update(UIContext *ctx, cairo_t *cr, Timeline *timeline, i32rect area);
void update(UIContext *ctx, cairo_t *cr);