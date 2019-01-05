//
// Created by divan on 05/01/19.
//

#include <cassert>
#include "timeline.h"
#include "util.h"

void tm_init(Timeline *timeline) {
   timeline->first = nullptr;
   timeline->last = nullptr;

   timeline->start_time = 0x7FFFFFFFFFFFFFFF;
   timeline->end_time = 0;
}

TimelineEntry *tm_add(Timeline *timeline, int depth, char* name, char* path, int line_no) {
   TimelineChunk* chunk = timeline->last;

   if (!chunk || chunk->entry_count == chunk->entry_capacity) {
      TimelineChunk* new_chunk = raw_alloc_type_zero(TimelineChunk);

      new_chunk->entry_capacity = 1024;
      new_chunk->entries = raw_alloc_array(TimelineEntry, new_chunk->entry_capacity);

      new_chunk->prev = timeline->last;
      new_chunk->next = nullptr;
      timeline->last = new_chunk;

      if (chunk) {
         new_chunk->prev->next = new_chunk;
      } else {
         timeline->first = new_chunk;
      }

      chunk = new_chunk;
   }

   assert(chunk);
   assert(chunk->entry_count < chunk->entry_capacity);
   TimelineEntry* entry = chunk->entries + (chunk->entry_count++);

   entry->depth = depth;
   entry->name = str_copy(&timeline->arena, name);
   entry->path = str_copy(&timeline->arena, path);
   entry->line_no = line_no;
   return entry;
}

