//
// Created by divan on 11/01/19.
//

#pragma once

struct Colour {
   f64 r;
   f64 g;
   f64 b;

   Colour operator*=(f64 s) { r*=s; g*=s; b*=s; return *this; }
};

inline Colour operator*(Colour c, f64 s) { return Colour {c.r*s, c.g*s, c.b*s}; }
inline Colour operator+(Colour a, Colour b) { return Colour {a.r+b.r, a.g+b.g, a.b+b.b}; }
inline Colour operator-(Colour a, Colour b) { return Colour {a.r-b.r, a.g-b.g, a.b-b.b}; }

inline Colour lerp(f64 f, Colour a, Colour b) { return a + (b - a) * f; }