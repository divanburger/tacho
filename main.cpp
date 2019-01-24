
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <SDL2/SDL.h>
#include <cairo/cairo.h>

#include "timeline_ui.h"

int main(int argc, char **args) {
   if (argc != 2) {
      printf("Usage: %s <filename>\n", args[0]);
      return 1;
   }

   Timeline *timeline = &state.timeline;

   String arg_file_name = str_copy(&state.memory, args[1]);
   File arg_file = file_stat(arg_file_name);

   if (arg_file.type == FILE_TYPE_FILE) {
      printf("Reading file: %s\n", args[1]);
      if (!tm_read_file(timeline, args[1])) {
         printf("Could not read file %s\n", args[1]);
         return 2;
      }

   } else if (arg_file.type == FILE_TYPE_DIRECTORY) {
      tm_init(timeline);
      printf("Watching directory: %s\n", args[1]);
      state.watch_path = arg_file_name;
   } else {
      printf("Unknown file type %s\n", args[1]);
      return 2;
   }

   printf("sizeof(TimelineEvent) = %li\n", sizeof(TimelineEvent));

   tm_calculate_statistics(timeline, &state.selection_statistics, 0, timeline->end_time);

   ui_run(update);
   return 0;
}