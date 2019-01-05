//
// Created by divan on 05/01/19.
//

#pragma once

#include <cstdint>
#include <cstdlib>

struct MemoryArenaBlock {
   size_t used;
   size_t size;
   MemoryArenaBlock *prev;

   uint8_t temp_count;
   uint8_t *data;
};

struct MemoryArena {
   size_t min_block_size = 8 * 1024 * 1024;
   MemoryArenaBlock *block = nullptr;
};

struct TempSection {
   MemoryArena *arena;
   MemoryArenaBlock *block;
   size_t mark;
};

void arena_init(MemoryArena *arena);
void arena_destroy(MemoryArena *arena);
void arena_stats(MemoryArena *arena, uint64_t* allocated_ptr, uint64_t* used_ptr);

TempSection begin_temp_section(MemoryArena *arena);
void end_temp_section(TempSection section);

void alloc_block(MemoryArena *arena, size_t size);
void *alloc_size_(MemoryArena *arena, size_t size);

#define alloc_size(arena, size) (alloc_size_(arena, size))
#define alloc_type(arena, type) ((type*)alloc_size_(arena, sizeof(type)))
#define alloc_array(arena, type, count) ((type*)alloc_size_(arena, sizeof(type) * (count)))
#define alloc_zstring(arena, size) ((char *)alloc_size_(arena, size + 1))