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
#include "string.h"

template<typename K>
struct HashTableSlot {
   K key;
   u64 hash;
};

template<typename K>
struct HashTable {
   Allocator* allocator;
   i32 pow2_size;
   i32 size;
   u32 hash_mask;
   i32 count;
   HashTableSlot<K> *slots;
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

inline u64 ht_hash(void *key) {
   return ht_hash((u64) key);
}

inline u64 ht_hash(String key) {
   __m128i constant = *(__m128i *) hash_constant;
   __m128i input;
   __m128i result = constant;

   for (i32 index = 0; index <= key.length - 16; index += 16) {
      input = _mm_loadu_si128((__m128i *) (key.data + index));
      result = _mm_aesdec_si128(result, input);
   }

   i32 left = key.length & 0xFU;
   if (left) {
      i8 values[16] = {};
      i32 start = key.length & (~0xFU);
      for (i32 i = 0; i < left; i++) values[i] = key.data[start + i];
      input = _mm_loadu_si128((__m128i *) values);
      result = _mm_aesdec_si128(result, input);
   }

   HashValue output;
   output.i128 = result;
   return output.i64[0] ^ output.i64[1];
}

inline bool ht_key_eq(u64 a, u64 b) {
   return a == b;
}

inline bool ht_key_eq(void *a, void *b) {
   return a == b;
}

inline bool ht_key_eq(String a, String b) {
   return str_equal(a, b);
}

template<typename K>
inline i32 ht_count(HashTable<K> *table) {
   return table->count;
}

template<typename K>
void ht_init(HashTable<K> *table, Allocator* allocator = nullptr) {
   table->allocator = allocator;
   table->pow2_size = 0;
   table->size = 0;
   table->count = 0;
}

template<typename K>
void ht_destroy(HashTable<K> *table) {
   std_free(table->allocator, table->slots);
   std_free(table->allocator, table->items);
}

template<typename K>
bool ht_exist(HashTable<K> *table, K key) {
   if (table->size == 0) return false;

   auto hash = ht_hash(key);
   auto index = hash & table->hash_mask;

   while (true) {
      auto slot = table->slots + index;

      if (!slot->hash) return false;
      if (ht_key_eq(slot->key, key)) return true;

      index = (index + 1) & table->hash_mask;
   }
}

template<typename K>
void *ht_find(HashTable<K> *table, K key) {
   if (table->size == 0) return nullptr;

   auto hash = ht_hash(key);
   auto index = hash & table->hash_mask;

   while (true) {
      auto slot = table->slots + index;

      if (!slot->hash) return nullptr;
      if (ht_key_eq(slot->key, key)) return table->items[index];

      index = (index + 1) & table->hash_mask;
   }
}

template<typename K>
void ht__add(HashTable<K> *table, K key, u64 hash, void *entry) {
   auto index = hash & table->hash_mask;

   while (true) {
      auto slot = table->slots + index;

      if (!slot->hash) {
         slot->hash = hash;
         slot->key = key;
         table->items[index] = entry;
         return;
      }

      assert(!ht_key_eq(slot->key, key));
      index = (index + 1) & table->hash_mask;
   }
}

template<typename K>
void ht_grow(HashTable<K> *table) {
   auto old_size = table->size;
   auto old_slots = table->slots;
   auto old_entries = table->items;

   table->pow2_size = (table->pow2_size >= 6) ? (table->pow2_size + 1) : 6;
   table->size = 1U << table->pow2_size;
   table->hash_mask = (u32) table->size - 1U;

   table->slots = std_alloc_array_zero(table->allocator, HashTableSlot<K>, table->size);
   table->items = std_alloc_array(table->allocator, void*, table->size);

   for (u32 index = 0; index < old_size; index++) {
      auto slot = old_slots + index;
      ht__add(table, slot->key, slot->hash, old_entries[index]);
   }

   std_free(table->allocator, old_slots);
   std_free(table->allocator, old_entries);
}

template<typename K>
void ht_add(HashTable<K> *table, K key, void *entry) {
   if (table->size == 0 || table->count * 2 > table->size) ht_grow(table);
   ht__add(table, key, ht_hash(key), entry);
   table->count++;
}

template<typename K>
i32 th_cursor_start(HashTable<K> *table) {
   i32 result = 0;
   while (result < table->size && table->slots[result].hash == 0) result++;
   return result;
}

template<typename K>
bool th_cursor_valid(HashTable<K> *table, i32 cursor) {
   return cursor < table->size;
}

template<typename K>
i32 th_cursor_step(HashTable<K> *table, i32 cursor) {
   i32 result = cursor + 1;
   while (result < table->size && table->slots[result].hash == 0) result++;
   return result;
}

template<typename K>
inline u64 th_key(HashTable<K> *table, i32 cursor) {
   return table->slots[cursor].key;
}

template<typename K>
inline void *th_item(HashTable<K> *table, i32 cursor) {
   return table->items[cursor];
}

void test_hash();

void test_hash_table();