//
// Created by divan on 27/01/19.
//

#pragma once

#include <cstdint>

template<typename T>
struct Array {
   int64_t count;
   int64_t capacity;
   T *data;
};

template<typename T>
void array_init(Array<T> *array) {
   array->count = 0;
   array->data = 0;
}

template<typename T>
void array_destroy(Array<T>* array, T item) {
   raw_free(array->data);
   array->count = 0;
   array->capacity = 0;
}

template<typename T>
void array_push(Array<T>* array, T item) {
   if (array->length >= array->capacity) array_grow(array);
   array->data[array->length++] = item;
}

template<typename T>
void array_grow(Array<T>* array) {
   auto old_count = array->count;
   auto old_data = array->data;

   array->capacity = min(array->capacity * 2, 8);
   array->data = ((T *) raw_alloc_size(sizeof(T) * array->capacity));

   for (int64_t index = 0; index < old_count; index++) array->data[index] = old_data[index];

   free(old_data);
}