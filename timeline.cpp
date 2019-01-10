//
// Created by divan on 05/01/19.
//

#include <cassert>
#include <cstdio>
#include <cstring>
#include "timeline.h"
#include "util.h"

void tm_init(Timeline *timeline) {
   timeline->thread_count = 0;

   timeline->start_time = 0;
   timeline->end_time = 0;
}

TimelineThread *tm_find_or_create_thread(Timeline *timeline, uint32_t thread_id, uint32_t fiber_id) {
   TimelineThread *thread = nullptr;

   for (int thread_index = 0; thread_index < timeline->thread_count; thread_index++) {
      TimelineThread *cur_thread = timeline->threads + thread_index;
      if (cur_thread->thread_id == thread_id && cur_thread->fiber_id == fiber_id) {
         thread = cur_thread;
         break;
      }
   }

   if (!thread) {
      thread = timeline->threads + timeline->thread_count;
      thread->index = timeline->thread_count;
      thread->thread_id = thread_id;
      thread->fiber_id = fiber_id;
      thread->arena = &timeline->arena;

      timeline->thread_count++;
   }

   return thread;
}


TimelineEntry *
tm_add(TimelineThread *thread, int depth, const char *name, const char *path, int line_no) {
   TimelineChunk *chunk = thread->last;

   if (!chunk || chunk->entry_count == chunk->entry_capacity) {
      TimelineChunk *new_chunk = raw_alloc_type_zero(TimelineChunk);

      new_chunk->entry_capacity = 1024;
      new_chunk->entries = raw_alloc_array(TimelineEntry, new_chunk->entry_capacity);

      new_chunk->prev = thread->last;
      new_chunk->next = nullptr;
      thread->last = new_chunk;

      if (chunk) {
         new_chunk->prev->next = new_chunk;
      } else {
         thread->first = new_chunk;
      }

      chunk = new_chunk;
   }

   assert(chunk);
   assert(chunk->entry_count < chunk->entry_capacity);

   thread->events++;

   TimelineEntry *entry = chunk->entries + (chunk->entry_count++);
   entry->thread_index = thread->index;
   entry->events = 0;
   entry->depth = depth;
   entry->name = str_copy(thread->arena, name);
   entry->path = str_copy(thread->arena, path);
   entry->line_no = line_no;
   return entry;
}

bool tm_read_file(Timeline *timeline, const char *filename) {
   tm_init(timeline);

   FILE *input = fopen(filename, "rb");
   if (!input) return false;

   char name_buffer[4096];
   char filename_buffer[4096];
   char method_buffer[4096];

   ThreadInfo thread_infos[64];

   TimelineThread *thread = nullptr;
   uint32_t thread_id = 0;
   uint32_t fiber_id = 0;

   while (!feof(input)) {
      uint8_t type;
      uint64_t time = 0;

      uint32_t method_id = 0;

      fread(&type, sizeof(uint8_t), 1, input);
      if (type != 'T') {
         fread(&time, sizeof(uint64_t) - sizeof(uint8_t), 1, input);
      }

      if (type == 'C') {
         CallBody call_body;
         fread(&call_body, CALL_BODY_BYTES, 1, input);

         fread(&method_buffer, call_body.method_name_length, 1, input);
         method_buffer[call_body.method_name_length] = 0;

         fread(&filename_buffer, call_body.filename_length, 1, input);
         filename_buffer[call_body.filename_length] = 0;

         memcpy(name_buffer + call_body.filename_offset, filename_buffer, call_body.filename_length);
         name_buffer[call_body.filename_offset + call_body.filename_length] = 0;

         if (!thread) thread = tm_find_or_create_thread(timeline, 0, 0);
         ThreadInfo *thread_info = thread_infos + thread->index;

         auto entry = tm_add(thread, thread_info->stack_index, method_buffer, name_buffer, call_body.line_no);
         entry->start_time = time;
         entry->method_id = call_body.method_id;

         for (int i = 0; i < thread_info->stack_index; i++) {
            thread_info->stack[i]->events++;
         }

         thread_info->last_time = time;
         thread_info->stack[thread_info->stack_index] = entry;
         thread_info->stack_index++;
         assert(thread_info->stack_index < array_size(thread_info->stack));
      } else if (type == 'R') {
         fread(&method_id, sizeof(uint32_t), 1, input);

         if (!thread) thread = tm_find_or_create_thread(timeline, 0, 0);
         ThreadInfo *thread_info = thread_infos + thread->index;

         int stack_index = thread_info->stack_index;

         if (stack_index > 0) {
            stack_index--;

            bool found = false;
            auto entry = thread_info->stack[stack_index];
            if (entry->method_id != method_id) {
               printf("Method ID mismatch : %i != %i : %.*s\n", entry->method_id, method_id, str_prt(entry->name));
            }

            if (entry->method_id != method_id) {
               auto test_entry = entry;
               while (stack_index > 0) {
                  if (test_entry->method_id == method_id) {
                     found = true;
                     break;
                  }
                  stack_index--;
                  test_entry = thread_info->stack[stack_index];
               }

               if (found) {
                  thread_info->stack_index--;

                  while (entry->method_id != method_id) {
                     entry->end_time = thread_info->last_time;

                     thread_info->stack_index--;
                     entry = thread_info->stack[thread_info->stack_index];
                     printf(" - Closed method %i : %.*s\n", entry->method_id, str_prt(entry->name));
                  }
               } else {
                  printf("Correction failed - Could not find method\n");
               }
            } else {
               thread_info->stack_index--;

               found = true;
            }

            if (found) {
               entry->end_time = time;
               thread_info->last_time = time;
            }
         }
      } else if (type == 'T') {
         ThreadSwitch thread_switch;
         fread(&thread_switch, sizeof(ThreadSwitch), 1, input);
         thread_id = thread_switch.thread_id;
         fiber_id = thread_switch.fiber_id;

         thread = tm_find_or_create_thread(timeline, thread_id, fiber_id);

      } else if (type == 'S') {
         uint32_t name_length;
         fread(&name_length, sizeof(uint32_t), 1, input);

         fread(&name_buffer, name_length, 1, input);
         timeline->name = str_copy(&timeline->arena, name_buffer, name_length);
      } else if (type == 'F') {
         timeline->end_time = time;
         break;
      } else {
         printf("Unknown event type: '%c'\n", type);
         fclose(input);
         return false;
      }
   }

   // Make sure all events end at least at timeline end
   for (int thread_index = 0; thread_index < timeline->thread_count; thread_index++) {
      auto thread_info = thread_infos + thread_index;

      for (--thread_info->stack_index; thread_info->stack_index >= 0; --thread_info->stack_index) {
         thread_info->stack[thread_info->stack_index]->end_time = timeline->end_time;
      }
   }

   fclose(input);
   return true;
}