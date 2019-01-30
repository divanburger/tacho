//
// Created by divan on 05/01/19.
//

#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <cassert>

#include "string.h"
#include "util.h"
#include "math.h"

//
// String
//

String str_copy(MemoryArena *arena, const char *str) {
   return str_copy(arena, str, (int) strlen(str));
}

String str_copy(MemoryArena *arena, String str) {
   return str_copy(arena, str.data, str.length);
}

String str_copy(MemoryArena *arena, const char *str, int length) {
   String result;
   result.length = length;
   result.data = alloc_zstring(arena, length);
   memcpy(result.data, str, (size_t) length);
   result.data[length] = 0;
   return result;
}

String as_string(char *str) {
   return String{(int) strlen(str), str};
}

bool str_equal(String a, String b) {
   if (a.length != b.length) return false;
   return (strncmp(a.data, b.data, (size_t)a.length) == 0);
}

int str_cmp(String a, String b) {
   auto prefix_length = min(a.length, b.length);
   if (prefix_length == 0) return b.length - a.length;

   auto result = strncmp(a.data, b.data, (size_t)prefix_length);
   return result == 0 ? a.length - b.length : result;
}

String str_print(MemoryArena *arena, const char *fmt, ...) {
   char buffer[1024];

   va_list v, v2;
   va_start(v, fmt);
   va_copy(v2, v);

   auto res = vsnprintf(buffer, sizeof(buffer), fmt, v);
   va_end(v);

   if (res <= array_size(buffer)) {
      return str_copy(arena, buffer, res);
   } else {
      int big_size = res + 1;

      String result;
      result.length = res;
      result.data = alloc_zstring(arena, res);
      res = vsnprintf(result.data, static_cast<size_t>(big_size), fmt, v2);

      va_end(v2);
      assert(res >= 0);

      return result;
   }
}

//
// String builder
//

void strb_init(StringBuilder *sb, MemoryArena* arena) {
   sb->length = 0;
   sb->capacity = 4096;
   sb->buffer = alloc_array(arena, char, sb->capacity);
   sb->arena = arena;
}

void strb_destroy(StringBuilder *sb) {
}

String strb_done(StringBuilder *sb) {
   sb->buffer[sb->length] = 0;
   return String{sb->length, sb->buffer};
}

void strb_reset(StringBuilder *sb) {
   sb->length = 0;
}

void strb_print(StringBuilder *sb, const char *fmt, ...) {
   char buffer[1024];
   char *big_buffer = nullptr;

   va_list v, v2;
   va_start(v, fmt);
   va_copy(v2, v);

   auto res = vsnprintf(buffer, sizeof(buffer), fmt, v);
   va_end(v);

   if (res <= array_size(buffer)) {
      strb_add(sb, buffer, res);
   } else {
      auto big_size = static_cast<size_t>(res + 1);
      big_buffer = raw_alloc_string(big_size);
      res = vsnprintf(big_buffer, big_size, fmt, v2);
      va_end(v2);
      assert(res >= 0);

      strb_add(sb, big_buffer, res);
      raw_free(big_buffer);
   }
}

void strb_add(StringBuilder *sb, char c) {
   assert(sb->length + 1 < sb->capacity - 1);
   sb->buffer[sb->length] = c;
   sb->length++;
}

void strb_add(StringBuilder *sb, const char *str) {
   strb_add(sb, str, (i32)strlen(str));
}

void strb_add(StringBuilder *sb, String str) {
   strb_add(sb, str.data, str.length);
}

void strb_add(StringBuilder *sb, const char *str, i32 size) {
   assert(sb->length + size < sb->capacity - 1);
   memcpy(sb->buffer + sb->length, str, sizeof(char) * size);
   sb->length += size;
}
