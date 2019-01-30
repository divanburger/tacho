//
// Created by divan on 19/01/19.
//

#include <cstdio>
#include "hash_table.h"

void test_hash() {
   u64 a = 113;
   u64 b = 322;
   u64 c = 531;

   printf("%8lu -> %8lu\n", a, ht_hash(a));
   printf("%8lu -> %8lu\n", b, ht_hash(b));
   printf("%8lu -> %8lu\n", c, ht_hash(c));

   for (u64 i = 0; i < 200; i++) {
      printf("%8lu -> %8lu\n", i, ht_hash(i));
   }

   String sa = const_as_string("test");
   String sb = const_as_string("test1234abcdABCD");
   String sc = const_as_string("test12345abcdeABCDE");
   String sd = const_as_string("this is a test string that is supposed to be very long");
   String se = const_as_string("is it really that different from the other case before");

   printf("%.*s -> %8lu\n", str_prt(sa), ht_hash(sa));
   printf("%.*s -> %8lu\n", str_prt(sb), ht_hash(sb));
   printf("%.*s -> %8lu\n", str_prt(sc), ht_hash(sc));
   printf("%.*s -> %8lu\n", str_prt(sd), ht_hash(sd));
   printf("%.*s -> %8lu\n", str_prt(se), ht_hash(se));
}

void test_hash_table() {
   int a = 113;
   int b = 322;
   int c = 531;

   HashTable<u64> table = {};

   ht_init(&table);

   for (int i = 0; i < 100; i++) {
      ht_add(&table, static_cast<u64>(3 + i * 8), &a);
      ht_add(&table, static_cast<u64>(5 + i * 8), &b);
      ht_add(&table, static_cast<u64>(7 + i * 8), &c);
   }

   auto res = (int*)ht_find(&table, 3UL);
   assert(res && *res == 113);

   res = (int*)ht_find(&table, 4UL);
   assert(!res);

   res = (int*)ht_find(&table, 7UL);
   assert(res && *res == 531);

   res = (int*)ht_find(&table, 5UL);
   assert(res && *res == 322);

   res = (int*)ht_find(&table, 6UL);
   assert(!res);

   ht_add(&table, 6UL, &b);

   for (int i = 100; i < 1000; i++) {
      ht_add(&table, static_cast<u64>(3 + i * 8), &a);
      ht_add(&table, static_cast<u64>(5 + i * 8), &b);
      ht_add(&table, static_cast<u64>(7 + i * 8), &c);
   }

   res = (int*)ht_find(&table, 6UL);
   assert(res && *res == 322);

   res = (int*)ht_find(&table, 3UL);
   assert(res && *res == 113);

   res = (int*)ht_find(&table, 4UL);
   assert(!res);

   res = (int*)ht_find(&table, 7UL);
   assert(res && *res == 531);

   res = (int*)ht_find(&table, 5UL);
   assert(res && *res == 322);
}
