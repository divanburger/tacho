//
// Created by divan on 12/01/19.
//

#pragma once

#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)<(b)?(b):(a))

//
// Vector
//

union i32vec2 {
   struct {
      int32_t x, y;
   };
   int32_t e[2];
};

inline i32vec2 Vec(int32_t x, int32_t y) {
   i32vec2 result;
   result.x = x;
   result.y = y;
   return result;
}

inline i32vec2 operator+(i32vec2 a, i32vec2 b) {return i32vec2{a.x+b.x, a.y+b.y};}
inline i32vec2 operator-(i32vec2 a, i32vec2 b) {return i32vec2{a.x-b.x, a.y-b.y};}
inline i32vec2 operator*(i32vec2 a, i32vec2 b) {return i32vec2{a.x*b.x, a.y*b.y};}
inline i32vec2 operator/(i32vec2 a, i32vec2 b) {return i32vec2{a.x/b.x, a.y/b.y};}

using ivec2 = i32vec2;

//
// Rect
//

union i32rect {
   struct {
      int32_t x, y, w, h;
   };
   struct {
      i32vec2 pos, size;
   };
};

inline i32rect Rect(int32_t x, int32_t y, int32_t w, int32_t h) {
   i32rect result;
   result.x = x;
   result.y = y;
   result.w = w;
   result.h = h;
   return result;
}

inline bool inside(i32rect r, int32_t x, int32_t y) {
   return (r.x <= x && r.x + r.w >= x && r.y <= y && r.y + r.h >= y);
}

inline bool inside(i32rect r, i32vec2 p) {
   return (r.x <= p.x && r.x + r.w >= p.x && r.y <= p.y && r.y + r.h >= p.y);
}

using irect = i32rect;