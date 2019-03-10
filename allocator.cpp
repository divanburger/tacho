//
// Created by divan on 08/03/19.
//

#include <cassert>
#include <cstring>

#include "allocator.h"

void arena_alloc_block(ArenaAllocator *arena, size_t size) {
   if (arena->min_block_size == 0) arena->min_block_size = 128 * 1024;

   size_t required_size = arena->min_block_size > size ? arena->min_block_size : size;

   ArenaAllocatorBlock* block = nullptr;

   if (arena->free_block && arena->free_block->size >= required_size) {
      block = arena->free_block;
      arena->free_block = arena->free_block->prev;
      block->used = 0;
      arena->free_block_count--;
   } else
   {
      size_t size_with_header = sizeof(ArenaAllocatorBlock) + required_size;
      auto mem = (u8*)std_alloc(arena->allocator, size_with_header);
      assert(mem);

      block = (ArenaAllocatorBlock *) mem;
      block->data = mem + sizeof(ArenaAllocatorBlock);
      block->size = required_size;
      block->used = 0;
   }

   if (arena->block) {
      block->temp_count = arena->block->temp_count;
   }

   block->prev = arena->block;
   arena->block = block;
   arena->block_count++;
}

void *arena_alloc(void *allocator, size_t size) {
   auto arena = (ArenaAllocator*)allocator;

   if (!arena->block || (arena->block->used + size > arena->block->size)) {
      arena_alloc_block(arena, size);
   }
   assert(arena->block && arena->block->used + size <= arena->block->size);

   auto mem = arena->block->data + arena->block->used;
   arena->block->used += size;

   return mem;
}

ArenaAllocator arena_make(Allocator *allocator) {
   ArenaAllocator arena = {};
   arena.base.alloc = arena_alloc;
   arena.base.free = noop_free;
   arena.allocator = allocator;
   arena.min_block_size = 128 * 1024;
   return arena;
}

void arena_free(ArenaAllocator* arena) {
   auto block = arena->block;

   while (block) {
      void *mem = block;
      block = block->prev;
      std_free(arena->allocator, mem);
   }
}

ArenaTempSection arena_temp_begin(ArenaAllocator *arena) {
   ArenaTempSection section = {};

   if (!arena->block) {
      arena_alloc_block(arena, 0);
   }

   auto block = arena->block;
   block->temp_count++;

   section.arena = arena;
   section.block = block;
   section.mark = block->used;
   return section;
}

void arena_temp_end(ArenaTempSection section) {
   ArenaAllocator *arena = section.arena;

   while (arena->block != section.block) {
      assert(arena->block);
      assert(arena->block->temp_count == 1);
      ArenaAllocatorBlock* freed_block = arena->block;
      arena->block = arena->block->prev;
      arena->block_count--;

      // Add freed block
      freed_block->prev = arena->free_block;
      arena->free_block = freed_block;
      arena->free_block_count++;
   }
   assert(arena->block == section.block);

   arena->block->used = section.mark;
   arena->block->temp_count--;
}

void arena_stats_print_(ArenaAllocator *arena, const char* name) {
   u64 allocated = 0;
   u64 used = 0;

   auto block = arena->block;
   while (block) {
      allocated += block->size;
      used += block->used;
      block = block->prev;
   }

   block = arena->free_block;
   while (block) {
      allocated += block->size;
      block = block->prev;
   }

   printf("%s: allocated=", name);

   if (allocated > 2097152)
      printf("%.2fMB", allocated / 1048576.0);
   else if (allocated > 2048)
      printf("%.2fkB", allocated / 1024.0);
   else
      printf("%luB", allocated);

   printf(", used=");

   if (used > 2097152)
      printf("%.2fMB", used / 1048576.0);
   else if (used > 2048)
      printf("%.2fkB", used / 1024.0);
   else
      printf("%luB", used);

   printf(", used_blocks=%li, free_blocks=%li\n", arena->block_count, arena->free_block_count);
}