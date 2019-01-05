//
// Created by divan on 05/01/19.
//

#pragma once

#include <cstdint>
#include "memory.h"
#include "string.h"

struct TimelineEntry {
   int64_t start_time;
   int64_t end_time;

   int depth;
   int line_no;
   String name;
   String path;
};

struct TimelineChunk {
   int64_t start_time;
   int64_t end_time;

   TimelineChunk* prev;
   TimelineChunk* next;

   int entry_count;
   int entry_capacity;
   TimelineEntry* entries;
};

struct Timeline {
   TimelineChunk* first;
   TimelineChunk* last;

   int64_t start_time;
   int64_t end_time;

   MemoryArena arena;
};

void tm_init(Timeline* timeline);

TimelineEntry* tm_add(Timeline* timeline, int depth, char* name, char* path, int line_no);