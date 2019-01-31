//
// Created by divan on 05/01/19.
//

#pragma once

#include <cstdint>
#include <cstdlib>

#include "types.h"

struct MemoryArenaBlock {
   size_t used;
   size_t size;
   MemoryArenaBlock *prev;

   u8 temp_count;
   u8 *data;
};

struct MemoryArena {
   size_t min_block_size;
   i64 block_count;
   i64 free_block_count;
   MemoryArenaBlock *block;
   MemoryArenaBlock *free_block;
};

struct TempSection {
   MemoryArena *arena;
   MemoryArenaBlock *block;
   size_t mark;
};

void arena_clear(MemoryArena *arena);
void arena_destroy(MemoryArena *arena);
void arena_stats_print_(MemoryArena *arena, const char* name);

#define arena_stats_print(arena) arena_stats_print_(arena, #arena)

TempSection begin_temp_section(MemoryArena *arena);
void end_temp_section(TempSection section);

void add_freed_block(MemoryArena *arena, MemoryArenaBlock* block);
void alloc_block(MemoryArena *arena, size_t size);
void *alloc_size(MemoryArena *arena, size_t size);
void *alloc_size_zero(MemoryArena *arena, size_t size);

#define alloc_type(arena, type) ((type*)alloc_size(arena, sizeof(type)))
#define alloc_type_zero(arena, type) ((type*)alloc_size_zero(arena, sizeof(type)))
#define alloc_array(arena, type, count) ((type*)alloc_size(arena, sizeof(type) * (count)))
#define alloc_zstring(arena, size) ((char *)alloc_size(arena, size + 1))