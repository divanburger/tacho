//
// Created by divan on 05/01/19.
//

#pragma once

#include <cstdint>
#include "memory.h"
#include "string.h"

struct TimelineMethod {
   uint64_t hash;

   int line_no;
   String name;
   String path;
};

struct TimelineMethodStatistics {
   TimelineMethod *method;

   int64_t calls;
   int64_t total_time;
   int64_t child_time;
   int64_t self_time;
};

struct TimelineEvent {
   int64_t start_time;
   int64_t end_time;

   TimelineEvent *parent;
   TimelineMethod *method;

   int32_t thread_index;
   uint32_t method_id;
   int32_t depth;

   uint64_t events;
};

struct TimelineEventChunk {
   int64_t start_time;
   int64_t end_time;

   TimelineEventChunk *prev;
   TimelineEventChunk *next;

   int entry_count;
   int entry_capacity;
   TimelineEvent *entries;
};

struct TimelineThread {
   int32_t index;
   uint32_t thread_id;
   uint32_t fiber_id;

   int32_t deepest_level;
   int64_t events;

   TimelineEventChunk *first;
   TimelineEventChunk *last;

   MemoryArena *arena;
};

struct TimelineMethodTable {
   int32_t capacity;
   int32_t count;

   uint64_t *hashes;
   TimelineMethod **methods;

   MemoryArena arena;
};

struct Timeline {
   int32_t thread_count;
   TimelineThread threads[64];

   int64_t start_time;
   int64_t end_time;

   String name;
   MemoryArena arena;

   uint64_t highest_method_total_time;

   TimelineMethodTable method_table;
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
   int64_t last_time;
   int32_t stack_index;
   TimelineEvent *stack[1024];
};

bool tm_read_file(Timeline *timeline, const char *filename);
void tm_end_event(TimelineEvent *event, ThreadInfo *info, uint64_t time);

void tm_init(Timeline *timeline);

TimelineThread *tm_find_or_create_thread(Timeline *timeline, uint32_t thread_id, uint32_t fiber_id);

TimelineEvent *tm_create_event(TimelineThread *thread);

TimelineMethod *tm_find_or_create_method(Timeline *timeline, String name, String path, int line_no);

void tm_grow_method_table(TimelineMethodTable *method_table);

uint64_t tm_hash_call(TimelineEvent *call_entry);