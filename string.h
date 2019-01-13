//
// Created by divan on 05/01/19.
//

#pragma once

#include "memory.h"

//
// String
//

struct String {
   int length;
   char *data;
};

inline bool str_blank(String str) {return str.length == 0 || !str.data;}
inline bool str_nonblank(String str) {return str.length > 0 && str.data;}

String str_copy(MemoryArena *arena, const char* str, int length);
String str_copy(MemoryArena *arena, const char* str);

bool str_equal(String a, String b);

String str_print(MemoryArena *arena, const char *fmt, ...);

String as_string(char* str);

#define str_prt(s) s.length, s.data

//
// String builder
//

struct StringBuilder {
   int32_t capacity;
   int32_t length;
   char *buffer;
   MemoryArena* arena;
};

void strb_init(StringBuilder *sb, MemoryArena* arena);
void strb_destroy(StringBuilder *sb);
String strb_done(StringBuilder *sb);
void strb_reset(StringBuilder *sb);
void strb_print(StringBuilder *sb, const char *fmt, ...);
void strb_add(StringBuilder *sb, char str);
void strb_add(StringBuilder *sb, const char *str);
void strb_add(StringBuilder *sb, String str);
void strb_add(StringBuilder *sb, const char *str, int32_t size);