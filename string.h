//
// Created by divan on 05/01/19.
//

#pragma once

#include "memory.h"

struct String {
   int length;
   char *data;
};

inline bool str_blank(String str) {return str.length == 0 || !str.data;}
inline bool str_nonblank(String str) {return str.length > 0 && str.data;}

String str_copy(MemoryArena *arena, const char* str, int length);
String str_copy(MemoryArena *arena, const char* str);

String as_string(char* str);

#define str_prt(s) s.length, s.data