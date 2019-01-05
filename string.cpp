//
// Created by divan on 05/01/19.
//

#include <cstring>

#include "string.h"

String str_copy(MemoryArena *arena, const char *str) {
   return str_copy(arena, str, (int)strlen(str));
}

String str_copy(MemoryArena *arena, const char *str, int length) {
   String result;
   result.length = length;
   result.data = alloc_zstring(arena, length);
   memcpy(result.data, str, (size_t)length);
   result.data[length] = 0;
   return result;
}

