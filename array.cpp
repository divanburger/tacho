//
// Created by divan on 27/01/19.
//

#include <cassert>
#include "array.h"

void* ainit_(void *array, Allocator *allocator) {
   auto hdr = (ArrayHeader *) malloc(sizeof(ArrayHeader));
   hdr->allocator = allocator;
   hdr->len = 0;
   hdr->cap = 0;
   return &hdr->buffer;
}

void afree_(void *array) {
   if (array) std_free(ahdr(array)->allocator, ahdr(array));
}

void *asetcap_(void *array, u32 new_cap, u32 element_size) {
   auto hdr = array ? ahdr(array) : nullptr;

   u32 new_len = hdr ? (hdr->len < new_cap ? hdr->len : new_cap) : 0;
   u32 mem_size = sizeof(ArrayHeader) + new_cap * element_size;

   if (array) {
      auto old_hdr = hdr;
      hdr = (ArrayHeader *) std_alloc(old_hdr->allocator, mem_size);
      hdr->allocator = old_hdr->allocator;
      hdr->len = new_len;
      hdr->cap = new_cap;
      memcpy(&hdr->buffer, array, new_len * element_size);
      std_free(hdr->allocator, old_hdr);
   } else {
      hdr = (ArrayHeader *) malloc(sizeof(ArrayHeader) + new_cap * element_size);
      hdr->allocator = nullptr;
      hdr->len = new_len;
      hdr->cap = new_cap;
   }

   return &hdr->buffer;
}

void *afit_(void *array, u32 count, u32 element_size) {
   u32 capacity = acap(array);
   if (count <= capacity) return array;

   u32 new_capacity = capacity >= 6 ? (capacity + (capacity >> 1U)) : 8;
   assert(new_capacity >= count);

   return asetcap_(array, new_capacity, element_size);
}

void *acat_(void *array, void *other, u32 element_size) {
   if (!other) return array;
   if (!array) return other;

   auto len = ahdr(array)->len;
   auto other_len = ahdr(other)->len;
   array = afit_(array, len + other_len, element_size);
   memcpy((char *) array + len * element_size, other, other_len * element_size);
   ahdr(array)->len += other_len;
   return array;
}