//
// Created by divan on 05/01/19.
//

#include <cstddef>
#include <cassert>
#include <cstdlib>
#include "memory.h"

void arena_init(MemoryArena *arena) {

}

void arena_destroy(MemoryArena *arena) {

}

void arena_stats(MemoryArena *arena, uint64_t *allocated_ptr, uint64_t *used_ptr) {
   uint64_t allocated = 0;
   uint64_t used = 0;

   auto block = arena->block;
   while (block) {
      allocated += block->size;
      used += block->used;
      block = block->prev;
   }

   *allocated_ptr = allocated;
   *used_ptr = used;
}

TempSection begin_temp_section(MemoryArena *arena) {
   TempSection section = {};

   if (!arena->block) {
      alloc_block(arena, 0);
   }

   auto block = arena->block;
   block->temp_count++;

   section.arena = arena;
   section.block = block;
   section.mark = block->used;
   return section;
}

void end_temp_section(TempSection section) {
   MemoryArena *arena = section.arena;

   while (arena->block != section.block) {
      assert(arena->block);
      assert(arena->block->temp_count == 1);
      void* mem = arena->block;
      arena->block = arena->block->prev;
      free(mem);
   }
   assert(arena->block == section.block);

   arena->block->used = section.mark;
   arena->block->temp_count--;
}

void alloc_block(MemoryArena *arena, size_t size) {
   size_t required_size = arena->min_block_size > size ? arena->min_block_size : size;

   size_t size_with_header = sizeof(MemoryArenaBlock) + required_size;
   auto mem = (uint8_t *) calloc(1, size_with_header);
   auto header = (MemoryArenaBlock *) mem;
   header->data = mem + sizeof(MemoryArenaBlock);
   header->size = required_size;

   if (arena->block) {
      header->temp_count = arena->block->temp_count;
   }

   header->prev = arena->block;
   arena->block = header;
}

void *alloc_size_(MemoryArena *arena, size_t size) {
   if (!arena->block || (arena->block->used + size > arena->block->size)) {
      alloc_block(arena, size);
   }
   assert(arena->block && arena->block->used + size <= arena->block->size);

   auto mem = arena->block->data + arena->block->used;
   arena->block->used += size;

   return mem;
}