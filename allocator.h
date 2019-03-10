//
// Created by divan on 08/03/19.
//

#pragma once

#include <cstddef>
#include <cstdlib>

#include "types.h"
#include "util.h"

using AllocatorAllocFunc = void *(*)(void *allocator, size_t size);
using AllocatorFreeFunc = void (*)(void *allocator, void *ptr);

struct Allocator {
   AllocatorAllocFunc alloc;
   AllocatorFreeFunc free;
};

inline void *std_alloc(Allocator *allocator, size_t size) {
   if (allocator) {
      return allocator->alloc(allocator, size);
   } else {
      return raw_alloc_size(size);//calloc(1, size);
   }
}

inline void std_free(Allocator *allocator, void *ptr) {
   if (allocator) {
      return allocator->free(allocator, ptr);
   } else {
      return raw_free(ptr);
   }
}

#define std_alloc_type(allocator, type) ((type*)std_alloc(allocator, sizeof(type)))
#define std_alloc_array(allocator, type, count) ((type*)std_alloc(allocator, sizeof(type) * (count)))
#define std_alloc_zstring(allocator, size) ((char *)std_alloc(allocator, size + 1))

inline void noop_free(void *allocator, void *ptr) {}

struct ArenaAllocatorBlock {
   size_t used;
   size_t size;
   ArenaAllocatorBlock *prev;

   u8 temp_count;
   u8 *data;
};

struct ArenaAllocator {
   Allocator base;
   Allocator *allocator;

   size_t min_block_size;
   i64 block_count;
   i64 free_block_count;
   ArenaAllocatorBlock *block;
   ArenaAllocatorBlock *free_block;
};

struct ArenaTempSection {
   ArenaAllocator *arena;
   ArenaAllocatorBlock *block;
   size_t mark;
};

ArenaAllocator arena_make(Allocator *allocator = nullptr);
void arena_free(ArenaAllocator *arena);
ArenaTempSection arena_temp_begin(ArenaAllocator *arena);
void arena_temp_end(ArenaTempSection section);

void arena_stats_print_(ArenaAllocator *arena, const char* name);
#define arena_stats_print(arena) arena_stats_print_(arena, #arena)