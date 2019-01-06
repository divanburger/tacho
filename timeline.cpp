//
// Created by divan on 05/01/19.
//

#include <cassert>
#include <cstdio>
#include <cstring>
#include "timeline.h"
#include "util.h"

void tm_init(Timeline *timeline) {
   timeline->first = nullptr;
   timeline->last = nullptr;

   timeline->start_time = 0;
   timeline->end_time = 0;
}

TimelineEntry *tm_add(Timeline *timeline, int depth, const char *name, const char *path, int line_no) {
   TimelineChunk *chunk = timeline->last;

   if (!chunk || chunk->entry_count == chunk->entry_capacity) {
      TimelineChunk *new_chunk = raw_alloc_type_zero(TimelineChunk);

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
   TimelineEntry *entry = chunk->entries + (chunk->entry_count++);

   entry->events = 0;
   entry->depth = depth;
   entry->name = str_copy(&timeline->arena, name);
   entry->path = str_copy(&timeline->arena, path);
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

   char* name_bfr = name_buffer;
   char* filename_bfr = filename_buffer;
   char* method_bfr = method_buffer;

   int stack_index = 0;
   TimelineEntry *stack[1024];

   while (!feof(input)) {
      uint64_t header;
      fread(&header, sizeof(uint64_t), 1, input);

      char type = (char) (header & 0xFF);
      int64_t time = (header >> 8);

      if (type == 'C') {
         CallBody call_body;
         fread(&call_body, CALL_BODY_BYTES, 1, input);

         fread(&method_buffer, call_body.method_name_length, 1, input);
         method_buffer[call_body.method_name_length] = 0;

         fread(&filename_buffer, call_body.filename_length, 1, input);
         filename_buffer[call_body.filename_length] = 0;

         memcpy(name_buffer + call_body.filename_offset, filename_buffer, call_body.filename_length);
         name_buffer[call_body.filename_offset + call_body.filename_length] = 0;

         auto entry = tm_add(timeline, stack_index, method_buffer, name_buffer, call_body.line_no);
         entry->start_time = time;

         for (int i = 0; i < stack_index; i++) {
            stack[i]->events++;
         }

         stack[stack_index] = entry;
         stack_index++;
      } else if (type == 'R' && stack_index > 0) {
         stack_index--;

         auto entry = stack[stack_index];
         entry->end_time = time;
      } else if (type == 'F') {
         timeline->end_time = time;
         break;
      }
   }

   for (; stack_index >= 0; stack_index--) {
      stack[stack_index]->end_time = timeline->end_time;
   }

   fclose(input);
   return true;
}

bool tm_output_html(Timeline *timeline, const char *output_filename) {
   FILE *output = fopen(output_filename, "w");
   if (!output) return false;

   fprintf(output, R"(<html>
<head>
<style>
  .e {
    background: #FFF;
    padding-left: 8px;
    border-top: 1px solid #333;
    border-left: 1px solid #333;
  }

  .t {
    color: #666;
  }

  .f {
    display: none;
  }

  .h:hover > .f {
    display: inline;
  }

  .e:hover {
    background: #FCC;
  }

</style>
</head>
<body>)");

   int stack_index = 0;
   TimelineEntry *stack[1024];

   for (TimelineChunk *chunk = timeline->first; chunk; chunk = chunk->next) {
      for (int index = 0; index < chunk->entry_count; index++) {
         TimelineEntry *entry = chunk->entries + index;

         for (; stack_index > entry->depth; stack_index--) {
            fprintf(output, "</div>");
         }

         double spent = ((entry->end_time - entry->start_time) / 1000000000.0) * 1000.0;

         if (spent > 0.5) {
            stack[stack_index++] = entry;
            fprintf(output,
                    "<div class=\"e\"><div class=\"h\"><span>%.*s</span> <span class=\"t\">%.1f ms</span> <span class=\"f\">%.*s:%i</span></div>",
                    spent, str_prt(entry->name), str_prt(entry->path), entry->line_no);
         }
      }
   }

   fprintf(output, "</body></html>");

   fclose(output);
   return true;
}

