//
// Created by divan on 05/01/19.
//

#pragma once

#include <cstdlib>

#define array_size(_arr) ((int)(sizeof(_arr)/sizeof(*(_arr))))

#define raw_alloc_type(type) ((type *) malloc(sizeof(type)))
#define raw_alloc_type_zero(type) ((type *) calloc(1, sizeof(type)))
#define raw_alloc_array(type, size) ((type *) malloc(sizeof(type) * size))
#define raw_alloc_string(size) ((char *) malloc(sizeof(char)*size))
#define raw_alloc_string_zt(size) ((char *) calloc(1, sizeof(char)*(size+1)))
#define raw_free(ptr) free(ptr)