//
// Created by divan on 07/03/19.
//

#include <cstdio>

#include "heap.h"
#include "util.h"
#include "memory.h"
#include "json.h"

struct HeapReader {
   Object object;
};

void heap_read(Heap *heap, const char *filename) {
   FILE *input;
   char buffer[1024 * 16];

   input = fopen(filename, "r");

   MemoryArena arena = {};

   HeapReader reader;

   while (fgets(buffer, array_size(buffer), input)) {

      auto section = begin_temp_section(&arena);

      bool valid = heap_read_object(&reader, buffer, &arena);

      end_temp_section(section);
   }

   fclose(input);
}

void heap_on_key(void* object_ptr, JsonTok token) {
   Object
}

bool heap_read_object(HeapReader *reader, const char *str, MemoryArena* arena) {


   JsonParser parser = {};
   parser
   parser.on_key =

   json_parse(&parser, str, arena);

   return false;
}