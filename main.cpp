
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <SDL2/SDL.h>
#include <cairo/cairo.h>

#include "timeline_ui.h"
#include "heap_ui.h"
#include "json.h"
#include "heap.h"

void show_help(const char *exe_name) {
   printf("Usage:\t%s COMMAND\n", exe_name);
   printf("\n\tprofile FILE\n");
   printf("\t  Show profile\n");
   printf("\n\twatch DIRECTORY\n");
   printf("\t  Watch directory containing profiles\n");
   printf("\n\theap FILE\n");
   printf("\t  Show heap\n");
   printf("\n\thelp\n");
   printf("\t  Shows this help\n");
   printf("\n");
}

int main(int argc, char **args) {
   if (argc <= 1) {
      show_help(args[0]);
      return 1;
   }

   if (strcmp(args[1], "profile") == 0) {
      if (argc != 3) {
         printf("Missing filename\n\n");
         show_help(args[0]);
         return 1;
      }

      String arg_file_name = str_copy(&state.memory, args[2]);
      File arg_file = file_stat(arg_file_name);

      if (arg_file.type != FILE_TYPE_FILE) {
         printf("Could not show profile: %s is not a file\n", args[2]);
         return 1;
      }

      printf("TimelineEvent size: %li\n", sizeof(TimelineEvent));

      state.active_timeline = raw_alloc_type(Timeline);

      printf("Reading file: %s\n", args[2]);
      if (!tm_read_file(state.active_timeline, args[2])) {
         printf("Could not read file %s\n", args[2]);
         return 2;
      }

      tm_calculate_statistics(state.active_timeline, &state.selection_statistics, 0, state.active_timeline->end_time);

      ui_run(update);

   } else if (strcmp(args[1], "watch") == 0) {
      if (argc != 3) {
         printf("Missing directory\n\n");
         show_help(args[0]);
         return 1;
      }

      String arg_file_name = str_copy(&state.memory, args[2]);
      File arg_file = file_stat(arg_file_name);

      if (arg_file.type != FILE_TYPE_DIRECTORY) {
         printf("Could not watch: %s is not a directory\n", args[2]);
         return 1;
      }

      printf("Watching directory: %s\n", args[2]);
      state.watch_path = arg_file_name;

      ui_run(update);

   } else if (strcmp(args[1], "heap") == 0) {
      if (argc != 3) {
         printf("Missing filename\n\n");
         show_help(args[0]);
         return 1;
      }

      String arg_file_name = str_copy(&state.memory, args[2]);
      File arg_file = file_stat(arg_file_name);

      if (arg_file.type != FILE_TYPE_FILE) {
         printf("Could not show heap: %s is not a file\n", args[2]);
         return 1;
      }

      printf("Object size: %li\n", sizeof(Object));
      printf("Page size: %li\n", sizeof(Page));

      Heap heap = {};
      heap_read(&heap, args[2]);

      heap_state.heap = &heap;
      ui_run(heap_update);

   } else {
      if (strcmp(args[1], "help") != 0) printf("Unknown command: %s\n\n", args[1]);
      show_help(args[0]);
      return 2;
   }

//   printf("sizeof(TimelineEvent) = %li\n", sizeof(TimelineEvent));

   return 0;
}