//
// Created by divan on 07/03/19.
//

#pragma once

#include "types.h"
#include "memory.h"

enum ObjectType {
   UNKNOWN,
   STRING,
   HASH,
   ARRAY,
   OBJECT
};

struct Object {
   u64 address;
   ObjectType type;
};

struct Page {
   u64 address;
};

struct Heap {
   i64 pages;
};

void heap_read(Heap *heap, const char *filename);

bool heap_read_object(Object *object, const char *str, MemoryArena* arena);