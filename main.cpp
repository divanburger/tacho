
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

   String arg_file_name = str_copy(&state.memory, args[1]);
   File arg_file = file_stat(arg_file_name);

   if (arg_file.type == FILE_TYPE_FILE) {
      state.active_timeline = raw_alloc_type(Timeline);

      printf("Reading file: %s\n", args[1]);
      if (!tm_read_file(state.active_timeline, args[1])) {
         printf("Could not read file %s\n", args[1]);
         return 2;
      }

      tm_calculate_statistics(state.active_timeline, &state.selection_statistics, 0, state.active_timeline->end_time);

   } else if (arg_file.type == FILE_TYPE_DIRECTORY) {
      printf("Watching directory: %s\n", args[1]);
      state.watch_path = arg_file_name;
   } else {
      printf("Unknown file type %s\n", args[1]);
      return 2;
   }

   printf("sizeof(TimelineEvent) = %li\n", sizeof(TimelineEvent));

   ui_run(update);
   return 0;
}