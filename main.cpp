
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cassert>

#include "util.h"
#include "string.h"
#include "timeline.h"

int main(int argc, char **args) {
   if (argc != 2) {
      printf("Usage: %s <filename>\n", args[0]);
      return 1;
   }

   Timeline timeline;
   tm_init(&timeline);

   {
      FILE *input = fopen(args[1], "rb");
      if (!input) {
         printf("Could not open file \"%s\" for reading\n", args[1]);
         return 2;
      }

      char name_buffer[4096];
      char buffer[4096];

      int stack_index = 0;
      TimelineEntry *stack[1024];

      while (fgets(buffer, array_size(buffer), input)) {
         char type = buffer[0];
         if (type == 'C' || type == 'R') {
            int64_t secs;
            int64_t nsecs;
            sscanf(buffer + 1, "%li.%li", &secs, &nsecs);
            int64_t time = secs * 1000000000 + nsecs;

            if (type == 'C') {
               char *filename_ptr = buffer;
               while (*filename_ptr != ' ') filename_ptr++;
               *filename_ptr = 0;
               filename_ptr++;

               char *lineno_ptr = filename_ptr;
               while (*lineno_ptr != ':') lineno_ptr++;
               *lineno_ptr = 0;
               lineno_ptr++;

               char *method_name_ptr = lineno_ptr;
               while (*method_name_ptr != ' ') method_name_ptr++;
               *method_name_ptr = 0;
               method_name_ptr++;

               char *end_ptr = method_name_ptr;
               while (*end_ptr != '\n' && *end_ptr != 0) end_ptr++;
               *end_ptr = 0;

               char *name = name_buffer;

               if (*filename_ptr == '$') {
                  size_t filename_len = strlen(filename_ptr) - 1;
                  memcpy(name_buffer, filename_ptr + 1, filename_len);
                  name_buffer[filename_len] = 0;
               } else {
                  int64_t offset = 0;
                  sscanf(filename_ptr, "%li", &offset);
                  assert(offset > 0);

                  while (*filename_ptr != '@') filename_ptr++;
                  filename_ptr++;

                  size_t filename_len = strlen(filename_ptr);
                  memcpy(name_buffer + offset, filename_ptr + 1, filename_len);
                  name_buffer[offset + filename_len] = 0;
               }

               auto entry = tm_add(&timeline, stack_index, method_name_ptr, name_buffer, atoi(lineno_ptr));
               entry->start_time = time;

               if (time < timeline.start_time) {
                  timeline.start_time = time;
               }
               else if (time > timeline.end_time) {
                  timeline.end_time = time;
               }

               stack[stack_index] = entry;
               stack_index++;
            } else if (stack_index > 0) {
               stack_index--;

               auto entry = stack[stack_index];
               entry->end_time = time;

               if (time > timeline.end_time) {
                  timeline.end_time = time;
               }
            }
         }
      }

      for (; stack_index >= 0; stack_index--) {
         stack[stack_index]->end_time = timeline.end_time;
      }

      fclose(input);
   }

   {
      FILE *output = fopen("output.html", "w");
      if (!output) {
         printf("Could not open file \"%s\" for writing\n", "output.html");
         return 2;
      }

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
      TimelineEntry* stack[1024];

      for (TimelineChunk *chunk = timeline.first; chunk; chunk = chunk->next) {
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
   }

   return 0;
}