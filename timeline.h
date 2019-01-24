//
// Created by divan on 05/01/19.
//

#pragma once

#include <cstdint>
#include "memory.h"
#include "string.h"
#include "hash_table.h"

struct TimelineMethod {
   uint64_t hash;

   uint32_t method_id;

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

   TimelineMethod *method;

   int16_t thread_index;
   int16_t depth;

   int32_t next_sibling_index;
};

struct TimelineEventChunk {
   TimelineEventChunk *next;
   TimelineEvent entries[1023];
   int entry_count;
   uint32_t padding[5];
};


struct TimelineThread {
   uint32_t thread_id;
   uint32_t fiber_id;

   int32_t deepest_level;
   int16_t index;
   int32_t event_count;

   TimelineEvent *events;

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
   int16_t thread_count;
   TimelineThread threads[64];

   int64_t start_time;
   int64_t end_time;

   String name;
   MemoryArena arena;

   TimelineMethodTable method_table;
};

struct TimelineStatistics {
   bool calculated;

   int64_t time_span;

   int32_t method_count;
   TimelineMethodStatistics *method_statistics;
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
   int64_t last_time;
   int32_t stack_index;
   TimelineEvent *stack[1024];

   TimelineEventChunk *first;
   TimelineEventChunk *last;
};

enum class MethodSortOrder : int8_t {
   SELF_TIME,
   TOTAL_TIME,
   CALLS,
   NAME,
   NONE
};

bool tm_read_file(Timeline *timeline, const char *filename);

void tm_init(Timeline *timeline);

TimelineThread *tm_find_or_create_thread(Timeline *timeline, uint32_t thread_id, uint32_t fiber_id);

TimelineEvent *tm_push_event(ThreadInfo *thread_info);

TimelineMethod *tm_find_or_create_method(Timeline *timeline, String name, String path, int line_no);

void tm_grow_method_table(TimelineMethodTable *method_table);

uint64_t tm_hash_call(TimelineEvent *call_entry);

void tm_calculate_statistics(Timeline *timeline, TimelineStatistics *statistics, int64_t start_time, int64_t end_time,
                             int32_t start_depth = 0, MethodSortOrder order = MethodSortOrder::SELF_TIME);

void tm_sort_statistics(TimelineStatistics *statistics, MethodSortOrder order);