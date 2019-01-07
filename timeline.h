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

   int events;
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

   String name;
   MemoryArena arena;
};

struct CallBody {
   uint32_t line_no;
   uint16_t method_name_length;
   uint16_t filename_offset;
   uint16_t filename_length;
};
#define CALL_BODY_BYTES 10

bool tm_read_file(Timeline* timeline, const char* filename);

void tm_init(Timeline* timeline);

bool tm_output_html(Timeline *timeline, const char* output_filename);

TimelineEntry* tm_add(Timeline* timeline, int depth, const char* name, const char* path, int line_no);