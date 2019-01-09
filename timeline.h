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

   uint32_t thread_index;
   uint32_t method_id;

   uint64_t events;
   int depth;
   int line_no;
   String name;
   String path;
};

struct TimelineChunk {
   int64_t start_time;
   int64_t end_time;

   TimelineChunk *prev;
   TimelineChunk *next;

   int entry_count;
   int entry_capacity;
   TimelineEntry *entries;
};

struct TimelineThread {
   uint32_t index;
   uint32_t thread_id;
   uint32_t fiber_id;

   uint64_t events;

   TimelineChunk *first;
   TimelineChunk *last;

   MemoryArena *arena;
};

struct Timeline {
   uint32_t thread_count;
   TimelineThread threads[64];

   int64_t start_time;
   int64_t end_time;

   String name;
   MemoryArena arena;
};

struct CallBody {
   uint32_t line_no;
   uint32_t method_id;
   uint16_t method_name_length;
   uint16_t filename_offset;
   uint16_t filename_length;
};
#define CALL_BODY_BYTES 14

struct ThreadSwitch {
   uint32_t thread_id;
   uint32_t fiber_id;
};

struct ThreadInfo {
   uint64_t last_time;
   int32_t stack_index;
   TimelineEntry *stack[1024];
};

bool tm_read_file(Timeline *timeline, const char *filename);

void tm_init(Timeline *timeline);

bool tm_output_html(Timeline *timeline, const char *output_filename);

TimelineThread *tm_find_or_create_thread(Timeline *timeline, uint32_t thread_id, uint32_t fiber_id);

TimelineEntry *tm_add(TimelineThread *thread, int depth, const char *name, const char *path, int line_no);