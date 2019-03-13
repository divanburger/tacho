//
// Created by divan on 27/01/19.
//

#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstddef>

#include "types.h"
#include "allocator.h"

struct ArrayHeader {
   Allocator *allocator;
   u32 len;
   u32 cap;
   char *buffer[1];
};

#define ahdr(array) ((ArrayHeader *)((char *)(array) - offsetof(ArrayHeader, buffer)))
#define alen(array) ((array) ? ahdr((array))->len : 0)
#define acap(array) ((array) ? ahdr((array))->cap : 0)

void* ainit_(void *array, Allocator* allocator = nullptr);
void afree_(void *array);
void *afit_(void *array, u32 count, u32 element_size);
void *asetcap_(void *array, u32 new_cap, u32 element_size);
void *acat_(void *array, void* other, u32 element_size);

#define ainit(array, ...) ((array) = (decltype(array))ainit_(array, __VA_ARGS__))
#define afree(array) (afree_((array)), (array) = nullptr)
#define asetcap(array, new_cap) (asetcap_((array), (new_cap), sizeof(*(array))))
#define ashrink(array) ((array) = (decltype(array))asetcap_((array), alen((array)), sizeof(*(array))))
#define apush(array, item) ((array) = (decltype(array))afit_((array), alen(array) + 1, sizeof(*array)), array[ahdr(array)->len++] = (item))
#define adel(array, index) (alen((array)) > (index) ? ((array)[(index)] = (array)[--ahdr((array))->len]) : 0)
#define acat(array, other) ((array) = (decltype(array))acat_((array), (other), sizeof(*array)))

inline void test_array() {
   int *numbers = nullptr;
   int *others = nullptr;

   apush(numbers, 2);
   apush(numbers, 4);
   apush(numbers, 6);
   apush(numbers, 1);
   adel(numbers, 1);
   adel(numbers, 10);
   apush(numbers, 2);
   adel(numbers, 3);
   apush(numbers, 3);
   ashrink(numbers);
   apush(numbers, 10);
   apush(numbers, 11);
   apush(numbers, 12);
   ashrink(numbers);
   apush(numbers, 13);

   apush(others, 100);
   apush(others, 200);
   apush(others, 300);

   for (int i = 0; i < alen(numbers); i++) printf("%i, ", numbers[i]);
   printf("\n");

   for (int i = 0; i < alen(others); i++) printf("%i, ", others[i]);
   printf("\n");

   acat(numbers, others);

   for (int i = 0; i < alen(numbers); i++) {
      printf("%i, ", numbers[i]);
   }
   printf("\n");

   afree(numbers);
   afree(others);
   afree(numbers);
}
