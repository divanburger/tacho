//
// Created by divan on 07/03/19.
//

#pragma once

#include "types.h"
#include "memory.h"
#include "allocator.h"
#include "array_list.h"
#include "hash_table.h"

enum ObjectType {
   HO_UNKNOWN,
   HO_STRING,
   HO_HASH,
   HO_ARRAY,
   HO_OBJECT,
   HO_DATA,
   HO_CLASS,
   HO_IMEMO,
   HO_STRUCT,
   HO_RATIONAL,
   HO_MATCH,
   HO_REGEXP,
   HO_SYMBOL,
   HO_ICLASS,
   HO_FILE,
   HO_BIGNUM,
   HO_FLOAT,
   HO_COMPLEX,
   HO_MODULE,
   HO_NODE,
   HO_ZOMBIE,
   HO_ROOT,
   HO_LAST
};

static const char *const object_type_names[HO_LAST] = {
      "UNKNOWN",
      "STRING",
      "HASH",
      "ARRAY",
      "OBJECT",
      "DATA",
      "CLASS",
      "IMEMO",
      "STRUCT",
      "RATIONAL",
      "MATCH",
      "REGEXP",
      "SYMBOL",
      "ICLASS",
      "FILE",
      "BIGNUM",
      "FLOAT",
      "COMPLEX",
      "MODULE",
      "NODE",
      "ZOMBIE",
      "ROOT",
};

enum ObjectFlags : u64 {
   OBJFLAG_NONE,
   OBJFLAG_OLD = 1UL << 0UL,
   OBJFLAG_EMDEDDED = 1UL << 1UL,
   OBJFLAG_MARKED = 1UL << 2UL,
};

struct Object {
   u64 address;
   u64 flags;
   ObjectType type;
   String value;

   ArrayList<Object *> referenced_by;
   ArrayList<u64> references;
};

struct Page {
   u64 id;
   u64 slot_start_address;
   u64 xpos;
   Object *slots[408];
   i16 slot_count = 0;
};

struct Heap {
   Allocator *allocator;

   i64 page_count;
   u64 max_xpos;

   ArrayList<Object> objects;

   Page **pages;
};

struct HeapLocation {
   Page *page;
   i16 slot_index;
};

void heap_read(Heap *heap, const char *filename);
HeapLocation heap_find_object(Heap *heap, u64 address);

inline Object *heap_get_object(HeapLocation location) {
   return location.page && location.slot_index >= 0 ? location.page->slots[location.slot_index] : nullptr;
}