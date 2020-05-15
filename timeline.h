//
// Created by divan on 05/01/19.
//

#pragma once

#include <cstdint>
#include "memory.h"
#include "str.h"
#include "hash_table.h"

enum SectionType : i16 {
   SECTION_METHOD,
   SECTION_CUSTOM
};

struct TimelineSection {
   u32 method_id;
   int line_no;
   String name;
   String path;
   SectionType type;
};

struct TimelineSectionStatistics {
   TimelineSection *method;

   i64 calls;
   i64 total_time;
   i64 child_time;
   i64 self_time;
};

struct TimelineEvent {
   i64 start_time;
   i64 end_time;

   i16 thread_index;
   i16 depth;

   i32 next_sibling_index;

   TimelineSection *section;
};

struct TimelineEventChunk {
   TimelineEventChunk *next;
   TimelineEvent entries[1023];
   int entry_count;
   u32 padding[5];
};


struct TimelineThread {
   u32 thread_id;
   u32 fiber_id;

   i32 deepest_level;
   i16 index;
   i32 event_count;

   TimelineEvent *events;

   MemoryArena *arena;
};

struct TimelineMethodTable {
   i32 capacity;
   i32 count;

   u64 *hashes;
   TimelineSection **methods;
};

struct Timeline {
   String filename;
   i16 version;
   bool loaded;
   bool header_only;

   i16 thread_count;
   TimelineThread threads[64];

   i64 start_time;
   i64 end_time;

   String name;
   MemoryArena arena;

   HashTable<TimelineSection> section_table;
};

struct TimelineStatistics {
   i64 time_span;
   i32 method_count;
   TimelineSectionStatistics *method_statistics;
   MemoryArena arena;
};

struct CallBody {
   u32 line_no;
   u32 method_id;
   u16 method_name_length;
   u16 filename_offset;
   u16 filename_length;
};
#define CALL_BODY_BYTES 14

struct ThreadSwitch {
   u32 thread_id;
   u32 fiber_id;
};

struct ThreadInfo {
   i64 last_time;
   i32 stack_index;
   TimelineEvent *stack[1024];

   TimelineEventChunk *first;
   TimelineEventChunk *last;
};

enum class MethodSortOrder : i8 {
   SELF_TIME,
   TOTAL_TIME,
   CALLS,
   NAME,
   NONE
};

u64 ht_hash(TimelineSection *method);

bool ht_key_eq(TimelineSection *a, TimelineSection *b);

bool tm_read_file_header(Timeline *timeline, const char *filename);

bool tm_read_file(Timeline *timeline, const char *filename);

void tm_init(Timeline *timeline);

TimelineThread *tm_find_or_create_thread(Timeline *timeline, u32 thread_id, u32 fiber_id);

TimelineEvent *tm_push_event(ThreadInfo *thread_info);

TimelineSection *tm_find_or_create_section(Timeline *timeline, SectionType type, String name, String path, int line_no);

void tm_calculate_statistics(Timeline *timeline, TimelineStatistics *statistics, i64 start_time, i64 end_time,
                             i32 start_depth = 0, i16 thread_index = -1,
                             MethodSortOrder order = MethodSortOrder::SELF_TIME);

void tm_sort_statistics(TimelineStatistics *statistics, MethodSortOrder order);