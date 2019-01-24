//
// Created by divan on 05/01/19.
//

#pragma once

#include <cstdlib>
#include <cstdio>

#define array_size(_arr) ((int)(sizeof(_arr)/sizeof(*(_arr))))

#define DEBUG_ALLOCATIONS 1
#undef DEBUG_ALLOCATIONS

#ifdef DEBUG_ALLOCATIONS

inline void* raw_alloc_size(size_t size) {
   printf("MALLOC: %li\n", size);
   return malloc(size);
}

inline void* raw_alloc_size_zero(size_t size) {
   printf("CALLOC: %li\n", size);
   return calloc(1, size);
}

inline void raw_free(void* ptr) {
   printf("FREE\n");
   free(ptr);
}

#else

#define raw_alloc_size(size) (malloc(size))
#define raw_alloc_size_zero(size) (calloc(1, size))
#define raw_free(ptr) free(ptr)

#endif

#define raw_alloc_type(type) ((type *) raw_alloc_size(sizeof(type)))
#define raw_alloc_type_zero(type) ((type *) raw_alloc_size_zero(sizeof(type)))
#define raw_alloc_array(type, size) ((type *) raw_alloc_size(sizeof(type) * size))
#define raw_alloc_array_zero(type, size) ((type *) raw_alloc_size_zero(sizeof(type) * size))
#define raw_alloc_string(size) ((char *) raw_alloc_size(sizeof(char)*size))
#define raw_alloc_string_zt(size) ((char *) raw_alloc_size_zero(sizeof(char)*(size+1)))