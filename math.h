//
// Created by divan on 12/01/19.
//

#pragma once

#include "types.h"

#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)<(b)?(b):(a))
#define clamp(v,l,h) min(max((v),(l)),(h))

//
// Vector
//

union i32vec2 {
   struct {
      i32 x, y;
   };
   i32 e[2];
};

inline i32vec2 Vec(i32 x, i32 y) {
   i32vec2 result;
   result.x = x;
   result.y = y;
   return result;
}

inline i32vec2 operator+(i32vec2 a, i32vec2 b) {return i32vec2{a.x+b.x, a.y+b.y};}
inline i32vec2 operator-(i32vec2 a, i32vec2 b) {return i32vec2{a.x-b.x, a.y-b.y};}
inline i32vec2 operator*(i32vec2 a, i32vec2 b) {return i32vec2{a.x*b.x, a.y*b.y};}
inline i32vec2 operator/(i32vec2 a, i32vec2 b) {return i32vec2{a.x/b.x, a.y/b.y};}


union i64vec2 {
   struct {
      i64 x, y;
   };
   i64 e[2];
};

inline i64vec2 Vec(i64 x, i64 y) {
   i64vec2 result;
   result.x = x;
   result.y = y;
   return result;
}

inline i64vec2 operator+(i64vec2 a, i64vec2 b) {return i64vec2{a.x+b.x, a.y+b.y};}
inline i64vec2 operator-(i64vec2 a, i64vec2 b) {return i64vec2{a.x-b.x, a.y-b.y};}
inline i64vec2 operator*(i64vec2 a, i64vec2 b) {return i64vec2{a.x*b.x, a.y*b.y};}
inline i64vec2 operator/(i64vec2 a, i64vec2 b) {return i64vec2{a.x/b.x, a.y/b.y};}

using ivec2 = i32vec2;

//
// Rect
//

union i32rect {
   struct {
      i32 x, y, w, h;
   };
   struct {
      i32vec2 pos, size;
   };
};

inline i32rect Rect(i32 x, i32 y, i32 w, i32 h) {
   i32rect result;
   result.x = x;
   result.y = y;
   result.w = w;
   result.h = h;
   return result;
}

inline bool inside(i32rect r, i32 x, i32 y) {
   return (r.x <= x && r.x + r.w > x && r.y <= y && r.y + r.h > y);
}

inline bool inside(i32rect r, i32vec2 p) {
   return (r.x <= p.x && r.x + r.w > p.x && r.y <= p.y && r.y + r.h > p.y);
}

union i64rect {
   struct {
      i64 x, y, w, h;
   };
   struct {
      i64vec2 pos, size;
   };
};

inline i64rect Rect(i64 x, i64 y, i64 w, i64 h) {
   i64rect result;
   result.x = x;
   result.y = y;
   result.w = w;
   result.h = h;
   return result;
}

inline bool inside(i64rect r, i64 x, i64 y) {
   return (r.x <= x && r.x + r.w > x && r.y <= y && r.y + r.h > y);
}

inline bool inside(i64rect r, i64vec2 p) {
   return (r.x <= p.x && r.x + r.w > p.x && r.y <= p.y && r.y + r.h > p.y);
}

using irect = i32rect;