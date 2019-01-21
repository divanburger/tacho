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

   tm_grow_method_table(&timeline->method_table);
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
      thread->event_count = 0;
      thread->thread_id = thread_id;
      thread->fiber_id = fiber_id;
      thread->arena = &timeline->arena;

      timeline->thread_count++;
   }

   return thread;
}


TimelineEvent *tm_push_event(ThreadInfo *thread_info) {
   TimelineEventChunk *chunk = thread_info->last;

   if (!chunk || chunk->entry_count == array_size(chunk->entries)) {
      TimelineEventChunk *new_chunk = raw_alloc_type_zero(TimelineEventChunk);

      new_chunk->next = nullptr;
      thread_info->last = new_chunk;

      if (chunk) {
         chunk->next = new_chunk;
      } else {
         thread_info->first = new_chunk;
      }

      chunk = new_chunk;
   }

   assert(chunk);
   assert(chunk->entry_count < array_size(chunk->entries));
   return chunk->entries + (chunk->entry_count++);
}

bool tm_read_file(Timeline *timeline, const char *filename) {
   tm_init(timeline);

   FILE *input = fopen(filename, "rb");
   if (!input) return false;

   ThreadInfo thread_infos[64];

   {
      char name_buffer[4096];
      char filename_buffer[4096];
      char method_buffer[4096];

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

            String name = str_copy(thread->arena, method_buffer);
            String path = str_copy(thread->arena, name_buffer);

            auto method = tm_find_or_create_method(timeline, name, path, call_body.line_no);
            method->method_id = call_body.method_id;

            auto entry = tm_push_event(thread_info);
            entry->method = method;
            entry->depth = thread_info->stack_index;
            entry->thread_index = thread->index;
            entry->start_time = time;

            if (entry->depth > thread->deepest_level) {
               thread->deepest_level = entry->depth;
            }

            thread->event_count++;

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

               if (entry->method->method_id != method_id) {
                  printf("Method ID mismatch : %i != %i : %.*s\n", entry->method->method_id, method_id,
                         str_prt(entry->method->name));
               }

               auto test_entry = entry;
               while (stack_index > 0) {
                  if (test_entry->method->method_id == method_id) {
                     found = true;
                     break;
                  }
                  stack_index--;
                  test_entry = thread_info->stack[stack_index];
               }

               if (found) {
                  thread_info->stack_index--;

                  while (entry->method->method_id != method_id) {
                     entry->end_time = thread_info->last_time;

                     thread_info->stack_index--;
                     entry = thread_info->stack[thread_info->stack_index];
                     printf(" - Closed method %i : %.*s\n", entry->method->method_id, str_prt(entry->method->name));
                  }

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
   }


   for (int thread_index = 0; thread_index < timeline->thread_count; thread_index++) {
      auto thread = timeline->threads + thread_index;
      auto thread_info = thread_infos + thread_index;

      thread->events = raw_alloc_array(TimelineEvent, thread->event_count);

      int32_t stack[1024] = {};
      int32_t stack_index = 0;

      int32_t event_index = 0;
      for (auto chunk = thread_info->first; chunk; chunk = chunk->next) {
         for (int i = 0; i < chunk->entry_count; i++) {
            auto event = chunk->entries + i;
            thread->events[event_index] = *event;

            if (stack[event->depth] != 0) {
               thread->events[stack[event->depth]].next_sibling_index = event_index;
            }
            stack[event->depth] = event_index;

            while (stack_index > event->depth) {
               stack[stack_index--] = 0;
            }
            stack_index = event->depth;

            event_index++;
         }
      }
   }

//   for (int i = 0; i < timeline->method_table.count; i++) {
//      auto method = timeline->method_table.methods[i];
//      method->self_time = method->total_time - method->child_time;
//
//      if (method->total_time > timeline->highest_method_total_time) {
//         timeline->highest_method_total_time = method->total_time;
//      }
//   }
//
//   qsort(timeline->method_table.methods, (size_t)timeline->method_table.count, sizeof(TimelineMethod*), [](const void* a_ptr, const void * b_ptr) -> int {
//      auto a = *(TimelineMethod**)a_ptr;
//      auto b = *(TimelineMethod**)b_ptr;
//      if (a->total_time < b->total_time) {
//         return 1;
//      } else if (a->total_time > b->total_time) {
//         return - 1;
//      }
//      return 0;
//   });

//   printf("Longest method: %lins\n", timeline->highest_method_total_time);
//   printf("Method count: %i\n", timeline->method_table.count);
//   for (int i = 0; i < timeline->method_table.count; i++) {
//      auto method = timeline->method_table.methods[i];
//      printf("%6li %10lins %10lins %10lins %.*s\n",
//            method->calls, method->total_time, method->self_time, method->child_time, str_prt(method->name));
//   }

   return true;
}

uint64_t tm_hash_call(String name, String path, int line_no) {
   uint64_t result = (uint64_t) line_no * name.length * path.length;
   if (result == 0) result = 1; // Hash may not be zero
   return result;
}

TimelineMethod *tm_find_or_create_method(Timeline *timeline, String name, String path, int line_no) {
   TimelineMethod *result;

   auto table = &timeline->method_table;

   uint64_t hash = tm_hash_call(name, path, line_no);

   for (int64_t i = 0; i < table->count; i++) {
      if (table->hashes[i] == hash) {
         result = table->methods[i];
         if (result->line_no == line_no &&
             str_equal(result->name, name) &&
             str_equal(result->path, path)) {
            return result;
         }
      }
   }

   if (table->count == table->capacity) {
      tm_grow_method_table(table);
   }
   assert(table->count < table->capacity);

   auto index = table->count++;
   table->hashes[index] = hash;

   result = alloc_type(&timeline->arena, TimelineMethod);
   table->methods[index] = result;

   result->hash = hash;
   result->line_no = line_no;
   result->name = name;
   result->path = path;
   return result;
}

void tm_grow_method_table(TimelineMethodTable *method_table) {
   if (method_table->hashes) free(method_table->hashes);

   auto old_methods = method_table->methods;

   method_table->capacity = method_table->capacity * 2;
   if (method_table->capacity < 64) method_table->capacity = 64;

   method_table->hashes = raw_alloc_array_zero(uint64_t, method_table->capacity);
   method_table->methods = raw_alloc_array_zero(TimelineMethod*, method_table->capacity);

   if (old_methods) {
      for (int64_t i = 0; i < method_table->count; i++) {
         TimelineMethod *method = old_methods[i];
         method_table->hashes[i] = method->hash;
         method_table->methods[i] = method;
      }
      free(old_methods);
   }
}

void tm_calculate_statistics(Timeline *timeline, TimelineStatistics *statistics, int64_t start_time, int64_t end_time, int32_t start_depth) {
   auto table = &statistics->method_statistics;
   statistics->time_span = end_time - start_time;

   arena_destroy(&statistics->arena);

   arena_init(&statistics->arena);
   ht_init(table);

   for (int16_t thread_index = 0; thread_index < timeline->thread_count; thread_index++) {
      auto thread = timeline->threads + thread_index;
      
      for (int32_t event_index = 0; event_index < thread->event_count; event_index++) {
         auto event = thread->events + event_index;

         if (event->depth < start_depth) continue;
         if (event->end_time < start_time) continue;
         if (event->start_time > end_time) continue;

         int64_t clipped_start_time = start_time > event->start_time ? start_time : event->start_time;
         int64_t clipped_end_time = event->end_time > end_time ? end_time : event->end_time;

         if (clipped_start_time < clipped_end_time) {
            auto method_statistics = (TimelineMethodStatistics *) ht_find(table, (void *) event->method);
            if (!method_statistics) {
               method_statistics = alloc_type(&statistics->arena, TimelineMethodStatistics);
               method_statistics->method = event->method;
               method_statistics->total_time = 0;
               method_statistics->self_time = 0;
               method_statistics->child_time = 0;
               method_statistics->calls = 0;
               ht_add(table, event->method, method_statistics);
            }

            int64_t event_time = clipped_end_time - clipped_start_time;

            method_statistics->total_time += event_time;
            method_statistics->calls++;
         }
      }
   }

   statistics->calculated = true;
}
