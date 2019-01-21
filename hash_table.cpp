//
// Created by divan on 19/01/19.
//

#include <cstdio>
#include "hash_table.h"

void ht_init(HashTable *table) {
   table->pow2_size = 0;
   table->size = 0;
   table->count = 0;
}

void *ht_find(HashTable *table, uint64_t key) {
   if (table->size == 0) return nullptr;

   auto hash = ht_hash(key);
   auto index = hash & table->hash_mask;

   while (true) {
      auto slot = table->slots + index;

      if (!slot->hash) return nullptr;
      if (slot->key == key) return table->items[index];

      index = (index + 1) & table->hash_mask;
   }
}

void ht__add(HashTable *table, uint64_t key, uint64_t hash, void *entry) {
   auto index = hash & table->hash_mask;

   while (true) {
      auto slot = table->slots + index;

      if (!slot->hash) {
         slot->hash = hash;
         slot->key = key;
         table->items[index] = entry;
         return;
      }

      assert(slot->key != key);
      index = (index + 1) & table->hash_mask;
   }
}

void ht_grow(HashTable *table) {
   auto old_size = table->size;
   auto old_slots = table->slots;
   auto old_entries = table->items;

   table->pow2_size = (table->pow2_size >= 6) ? (table->pow2_size + 1) : 6;
   table->size = 1U << table->pow2_size;
   table->hash_mask = (uint32_t) table->size - 1U;

   table->slots = raw_alloc_array_zero(HashTableSlot, table->size);
   table->items = raw_alloc_array(void*, table->size);

   for (uint32_t index = 0; index < old_size; index++) {
      auto slot = old_slots + index;
      ht__add(table, slot->key, slot->hash, old_entries[index]);
   }

   raw_free(old_slots);
   raw_free(old_entries);
}

void ht_add(HashTable *table, uint64_t key, void *entry) {
   if (table->size == 0 || table->count * 2 > table->size) ht_grow(table);
   ht__add(table, key, ht_hash(key), entry);
   table->count++;
}

void test_hash() {
   uint64_t a = 113;
   uint64_t b = 322;
   uint64_t c = 531;

   printf("%8lu -> %8lu\n", a, ht_hash(a));
   printf("%8lu -> %8lu\n", b, ht_hash(b));
   printf("%8lu -> %8lu\n", c, ht_hash(c));

   for (uint64_t i = 0; i < 200; i++) {
      printf("%8lu -> %8lu\n", i, ht_hash(i));
   }
}

void test_hash_table() {
   int a = 113;
   int b = 322;
   int c = 531;

   HashTable table = {};

   ht_init(&table);

   for (int i = 0; i < 100; i++) {
      ht_add(&table, static_cast<uint64_t>(3 + i * 8), &a);
      ht_add(&table, static_cast<uint64_t>(5 + i * 8), &b);
      ht_add(&table, static_cast<uint64_t>(7 + i * 8), &c);
   }

   auto res = (int*)ht_find(&table, 3);
   assert(res && *res == 113);

   res = (int*)ht_find(&table, 4);
   assert(!res);

   res = (int*)ht_find(&table, 7);
   assert(res && *res == 531);

   res = (int*)ht_find(&table, 5);
   assert(res && *res == 322);

   res = (int*)ht_find(&table, 6);
   assert(!res);

   ht_add(&table, 6, &b);

   for (int i = 100; i < 1000; i++) {
      ht_add(&table, static_cast<uint64_t>(3 + i * 8), &a);
      ht_add(&table, static_cast<uint64_t>(5 + i * 8), &b);
      ht_add(&table, static_cast<uint64_t>(7 + i * 8), &c);
   }

   res = (int*)ht_find(&table, 6);
   assert(res && *res == 322);

   res = (int*)ht_find(&table, 3);
   assert(res && *res == 113);

   res = (int*)ht_find(&table, 4);
   assert(!res);

   res = (int*)ht_find(&table, 7);
   assert(res && *res == 531);

   res = (int*)ht_find(&table, 5);
   assert(res && *res == 322);
}

int32_t th_start_cursor(HashTable *table) {
   int32_t result = 0;
   while (result < table->size && table->slots[result].hash == 0) result++;
   return result;
}

bool th_valid_cursor(HashTable *table, int32_t cursor) {
   return cursor < table->size;
}

int32_t th_step_cursor(HashTable *table, int32_t cursor) {
   int32_t result = cursor + 1;
   while (result < table->size && table->slots[result].hash == 0) result++;
   return result;
}
