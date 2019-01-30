//
// Created by divan on 05/01/19.
//

#include <cassert>
#include <cstdio>
#include <cstring>
#include "timeline.h"
#include "util.h"

TimelineThread *tm_find_or_create_thread(Timeline *timeline, u32 thread_id, u32 fiber_id) {
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

bool tm_read_file_header(Timeline *timeline, const char *filename) {
   bool valid = true;

   FILE *input = fopen(filename, "rb");
   if (!input) return false;

   timeline->filename = str_copy(&timeline->arena, filename);

   char name_buffer[4096];

   while (!feof(input)) {
      u8 type;
      u64 time = 0;
      size_t read = 0;

      read = fread(&type, sizeof(u8), 1, input);
      if (read != 1) {
         valid = false;
         break;
      }
      read = fread(&time, sizeof(u64) - sizeof(u8), 1, input);
      if (read != 1) {
         valid = false;
         break;
      }

      if (type == 'S') {
         u32 name_length;
         read = fread(&name_length, sizeof(u32), 1, input);
         if (read != 1) {
            valid = false;
            break;
         }

         read = fread(&name_buffer, name_length, 1, input);
         if (read != 1) {
            valid = false;
            break;
         }

         timeline->name = str_copy(&timeline->arena, name_buffer, name_length);
         break;
      } else {
         valid = false;
         break;
      }
   }

   fclose(input);

   timeline->header_only = true;
   timeline->loaded = valid;
   return valid;
}

bool tm_read_file(Timeline *timeline, const char *filename) {
   FILE *input = fopen(filename, "rb");
   if (!input) return false;

   timeline->filename = str_copy(&timeline->arena, filename);

   ThreadInfo thread_infos[64] = {};

   {
      char name_buffer[4096];
      char filename_buffer[4096];
      char method_buffer[4096];

      TimelineThread *thread = nullptr;
      u32 thread_id = 0;
      u32 fiber_id = 0;

      while (!feof(input)) {
         u8 type;
         u64 time = 0;
         u32 method_id = 0;

         fread(&type, sizeof(u8), 1, input);
         if (type != 'T') {
            fread(&time, sizeof(u64) - sizeof(u8), 1, input);
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
            fread(&method_id, sizeof(u32), 1, input);

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
            u32 name_length;
            fread(&name_length, sizeof(u32), 1, input);

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

      i32 stack[1024] = {};
      i32 stack_index = 0;

      i32 event_index = 0;
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

   timeline->loaded = true;
   return true;
}

u64 tm_hash_call(String name, String path, int line_no) {
   u64 result = (u64) line_no * name.length * path.length;
   if (result == 0) result = 1; // Hash may not be zero
   return result;
}

TimelineMethod *tm_find_or_create_method(Timeline *timeline, String name, String path, int line_no) {
   TimelineMethod *result;

   auto table = &timeline->method_table;

   u64 hash = tm_hash_call(name, path, line_no);

   for (i64 i = 0; i < table->count; i++) {
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
   if (method_table->hashes) raw_free(method_table->hashes);

   auto old_methods = method_table->methods;

   method_table->capacity = method_table->capacity * 2;
   if (method_table->capacity < 64) method_table->capacity = 64;

   method_table->hashes = raw_alloc_array_zero(u64, method_table->capacity);
   method_table->methods = raw_alloc_array_zero(TimelineMethod*, method_table->capacity);

   if (old_methods) {
      for (i64 i = 0; i < method_table->count; i++) {
         TimelineMethod *method = old_methods[i];
         method_table->hashes[i] = method->hash;
         method_table->methods[i] = method;
      }
      raw_free(old_methods);
   }
}

void tm_calculate_statistics(Timeline *timeline, TimelineStatistics *statistics, i64 start_time, i64 end_time,
                             i32 start_depth, MethodSortOrder order) {
   statistics->time_span = end_time - start_time;

   MemoryArena temp = {};

   arena_clear(&statistics->arena);

   HashTable<void*> method_statistics_table = {};
   auto table = &method_statistics_table;
   ht_init(table);

   TimelineMethodStatistics *stack[1024];
   i32 stack_index = 0;

   for (i16 thread_index = 0; thread_index < timeline->thread_count; thread_index++) {
      auto thread = timeline->threads + thread_index;

      for (i32 event_index = 0; event_index < thread->event_count; event_index++) {
         auto event = thread->events + event_index;

         if (event->depth < start_depth) continue;
         if (event->end_time < start_time) continue;
         if (event->start_time > end_time) continue;

         i64 clipped_start_time = start_time > event->start_time ? start_time : event->start_time;
         i64 clipped_end_time = event->end_time > end_time ? end_time : event->end_time;

         if (clipped_start_time < clipped_end_time) {
            auto method_statistics = (TimelineMethodStatistics *) ht_find(table, (void*)event->method);
            if (!method_statistics) {
               method_statistics = alloc_type(&temp, TimelineMethodStatistics);
               method_statistics->method = event->method;
               method_statistics->total_time = 0;
               method_statistics->self_time = 0;
               method_statistics->child_time = 0;
               method_statistics->calls = 0;
               ht_add(table, (void*)event->method, method_statistics);
            }

            i64 event_time = clipped_end_time - clipped_start_time;

            if (event->depth > start_depth) {
               // Within self
               for (i32 index = start_depth; index < event->depth; index++) {
                  if (stack[index] && stack[index]->method == event->method) {
                     method_statistics->total_time -= event_time;
                     method_statistics->child_time -= event_time;
                     break;
                  }
               }

               TimelineMethodStatistics *parent_stat = stack[event->depth - 1];
               if (parent_stat) {
                  parent_stat->child_time += event_time;
               }
            }

            method_statistics->total_time += event_time;
            method_statistics->calls++;

            stack[event->depth] = method_statistics;

            while (stack_index > event->depth) {
               stack[stack_index--] = nullptr;
            }
            stack_index = event->depth;
         }
      }
   }

   statistics->calculated = true;

   statistics->method_count = table->count;
   statistics->method_statistics = alloc_array(&statistics->arena, TimelineMethodStatistics, table->count);

   TimelineMethodStatistics *ptr = statistics->method_statistics;
   for (auto cursor = th_cursor_start(table); th_cursor_valid(table, cursor); cursor = th_cursor_step(table, cursor)) {
      auto item = *(TimelineMethodStatistics *) th_item(table, cursor);
      item.self_time = item.total_time - item.child_time;
      *(ptr++) = item;
   }

   tm_sort_statistics(statistics, order);

   arena_destroy(&temp);
}

void tm_sort_statistics(TimelineStatistics *statistics, MethodSortOrder order) {
   comparison_fn_t comparison_function;

   switch (order) {
      case MethodSortOrder::CALLS: {
         comparison_function = [](const void *a_ptr, const void *b_ptr) -> int {
            auto a = (TimelineMethodStatistics *) a_ptr;
            auto b = (TimelineMethodStatistics *) b_ptr;

            if (a->calls < b->calls) return 1;
            if (a->calls > b->calls) return -1;
            return 0;
         };
      }
         break;
      case MethodSortOrder::SELF_TIME: {
         comparison_function = [](const void *a_ptr, const void *b_ptr) -> int {
            auto a = (TimelineMethodStatistics *) a_ptr;
            auto b = (TimelineMethodStatistics *) b_ptr;

            if (a->self_time < b->self_time) return 1;
            if (a->self_time > b->self_time) return -1;
            return 0;
         };
      }
         break;
      case MethodSortOrder::TOTAL_TIME: {
         comparison_function = [](const void *a_ptr, const void *b_ptr) -> int {
            auto a = (TimelineMethodStatistics *) a_ptr;
            auto b = (TimelineMethodStatistics *) b_ptr;

            if (a->total_time < b->total_time) return 1;
            if (a->total_time > b->total_time) return -1;
            return 0;
         };
      }
         break;
      case MethodSortOrder::NAME: {
         comparison_function = [](const void *a_ptr, const void *b_ptr) -> int {
            auto a = (TimelineMethodStatistics *) a_ptr;
            auto b = (TimelineMethodStatistics *) b_ptr;
            return str_cmp(a->method->name, b->method->name);
         };
      }
         break;
   }

   qsort(statistics->method_statistics, (size_t) statistics->method_count, sizeof(TimelineMethodStatistics),
         comparison_function);
}
