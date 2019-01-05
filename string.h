//
// Created by divan on 05/01/19.
//

#pragma once

#include "memory.h"

struct String {
   int length;
   char *data;
};

String str_copy(MemoryArena *arena, const char* str, int length);
String str_copy(MemoryArena *arena, const char* str);

#define str_prt(s) s.length, s.data