//
// Created by divan on 05/01/19.
//

#include <cstddef>
#include <cassert>
#include <cstdlib>
#include "memory.h"
#include "util.h"

void arena_clear(MemoryArena *arena) {
   auto block = arena->block;

   while (block) {
      auto prev = block->prev;
      block->prev = arena->free_block;
      arena->free_block = block;
      block = prev;
   }

   arena->free_block_count += arena->block_count;
   arena->block_count = 0;
   arena->block = nullptr;
}

void arena_destroy(MemoryArena *arena) {
   auto block = arena->block;

   while (block) {
      void *mem = block;
      block = block->prev;
      raw_free(mem);
   }
}

void arena_stats_print_(MemoryArena *arena, const char* name) {
   u64 allocated = 0;
   u64 used = 0;

   auto block = arena->block;
   while (block) {
      allocated += block->size;
      used += block->used;
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
      MemoryArenaBlock* freed_block = arena->block;
      arena->block = arena->block->prev;
      arena->block_count--;

      add_freed_block(arena, freed_block);
   }
   assert(arena->block == section.block);

   arena->block->used = section.mark;
   arena->block->temp_count--;
}

void add_freed_block(MemoryArena *arena, MemoryArenaBlock* block) {
   block->prev = arena->free_block;
   arena->free_block = block;
   arena->free_block_count++;
}

void alloc_block(MemoryArena *arena, size_t size) {
   if (arena->min_block_size == 0) arena->min_block_size = 128 * 1024 - sizeof(MemoryArenaBlock);

   size_t required_size = arena->min_block_size > size ? arena->min_block_size : size;

   MemoryArenaBlock* block = nullptr;

   if (arena->free_block && arena->free_block->size >= required_size) {
      block = arena->free_block;
      arena->free_block = arena->free_block->prev;
      block->used = 0;
      arena->free_block_count--;
   } else
   {
      size_t size_with_header = sizeof(MemoryArenaBlock) + required_size;
      auto mem = raw_alloc_array_zero(u8, size_with_header);
      assert(mem);

      block = (MemoryArenaBlock *) mem;
      block->data = mem + sizeof(MemoryArenaBlock);
      block->size = required_size;
   }

   if (arena->block) {
      block->temp_count = arena->block->temp_count;
   }

   block->prev = arena->block;
   arena->block = block;
   arena->block_count++;
}

void *alloc_size(MemoryArena *arena, size_t size) {
   if (!arena->block || (arena->block->used + size > arena->block->size)) {
      alloc_block(arena, size);
   }
   assert(arena->block && arena->block->used + size <= arena->block->size);

   auto mem = arena->block->data + arena->block->used;
   arena->block->used += size;

   return mem;
}

void *alloc_size_zero(MemoryArena *arena, size_t size) {
   u8* mem = (u8*)alloc_size(arena, size);
   for (size_t i = 0; i < size; i++) mem[i] = 0;
   return mem;
}