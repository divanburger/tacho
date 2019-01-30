//
// Created by divan on 19/01/19.
//

#pragma once

#include <cstdint>
#include <cassert>
#include <xmmintrin.h>
#include <wmmintrin.h>
#include <smmintrin.h>

#include "util.h"
#include "types.h"

struct HashTableSlot {
   u64 key;
   u64 hash;
};

struct HashTable {
   i32 pow2_size;
   i32 size;
   u32 hash_mask;
   i32 count;
   HashTableSlot *slots;
   void **items;
};

union HashValue {
   struct {
      u64 i64[2];
   };
   __m128i i128;
};

static const u8 hash_constant[16] = {0xe5, 0xbd, 0xe8, 0xe1, 0xd4, 0xf1, 0xfd, 0x75, 0xf1, 0x10, 0xbc, 0xce, 0x74,
                                          0xd4, 0x07, 0x5d};

inline u64 ht_hash(u64 key) {
   i64 values[] = {(i64) key, (i64) key};

   __m128i constant = *(__m128i *) hash_constant;
   __m128i input = _mm_loadu_si128((__m128i *) values);

   __m128i result = _mm_aesdec_si128(constant, input);
   result = _mm_aesdec_si128(result, input);
   result = _mm_aesdec_si128(result, input);

   HashValue output;
   output.i128 = result;
   return output.i64[0];
}

inline i32 ht_count(HashTable *table) {
   return table->count;
}


void ht_init(HashTable *table);
void *ht_find(HashTable *table, u64 key);
void ht__add(HashTable *table, u64 key, u64 hash, void *entry);
void ht_grow(HashTable *table);
void ht_add(HashTable *table, u64 key, void *entry);

inline void *ht_find(HashTable *table, void *ptr_key) {
   return ht_find(table, (u64) ptr_key);
}

inline void ht_add(HashTable *table, void *ptr_key, void *entry) {
   ht_add(table, (u64) ptr_key, entry);
}

i32 th_start_cursor(HashTable *table);
bool th_valid_cursor(HashTable *table, i32 cursor);
i32 th_step_cursor(HashTable *table, i32 cursor);

inline u64 th_key(HashTable *table, i32 cursor) {
   return table->slots[cursor].key;
}

inline void *th_item(HashTable *table, i32 cursor) {
   return table->items[cursor];
}

void test_hash();
void test_hash_table();