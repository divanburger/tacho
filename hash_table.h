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

struct HashTableSlot {
   uint64_t key;
   uint64_t hash;
};

struct HashTable {
   int32_t pow2_size;
   int32_t size;
   uint32_t hash_mask;
   int32_t count;
   HashTableSlot *slots;
   void **items;
};

union HashValue {
   struct {
      uint64_t i64[2];
   };
   __m128i i128;
};

static const uint8_t hash_constant[16] = {0xe5, 0xbd, 0xe8, 0xe1, 0xd4, 0xf1, 0xfd, 0x75, 0xf1, 0x10, 0xbc, 0xce, 0x74,
                                          0xd4, 0x07, 0x5d};

inline uint64_t ht_hash(uint64_t key) {
   int64_t values[] = {(int64_t) key, (int64_t) key};

   __m128i constant = *(__m128i *) hash_constant;
   __m128i input = _mm_loadu_si128((__m128i *) values);

   __m128i result = _mm_aesdec_si128(constant, input);
   result = _mm_aesdec_si128(result, input);
   result = _mm_aesdec_si128(result, input);

   HashValue output;
   output.i128 = result;
   return output.i64[0];
}

inline int32_t ht_count(HashTable *table) {
   return table->count;
}


void ht_init(HashTable *table);
void *ht_find(HashTable *table, uint64_t key);
void ht__add(HashTable *table, uint64_t key, uint64_t hash, void *entry);
void ht_grow(HashTable *table);
void ht_add(HashTable *table, uint64_t key, void *entry);

inline void *ht_find(HashTable *table, void *ptr_key) {
   return ht_find(table, (uint64_t) ptr_key);
}

inline void ht_add(HashTable *table, void *ptr_key, void *entry) {
   ht_add(table, (uint64_t) ptr_key, entry);
}

int32_t th_start_cursor(HashTable *table);
bool th_valid_cursor(HashTable *table, int32_t cursor);
int32_t th_step_cursor(HashTable *table, int32_t cursor);

inline uint64_t th_key(HashTable *table, int32_t cursor) {
   return table->slots[cursor].key;
}

inline void *th_item(HashTable *table, int32_t cursor) {
   return table->items[cursor];
}

void test_hash();
void test_hash_table();