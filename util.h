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

#define raw_alloc_size(size) (printf("MALLOC(%i): %s:%i\n", (size), __FILE__, __LINE__), malloc(size))
#define raw_alloc_size_zero(size) (printf("CALLOC(%i): %s:%i\n", (size), __FILE__, __LINE__), calloc(1, size))
#define raw_free(ptr) (printf("FREE: %s:%i\n", __FILE__, __LINE__), free(ptr))

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