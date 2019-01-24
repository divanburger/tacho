//
// Created by divan on 11/01/19.
//

#pragma once

struct Colour {
   double r;
   double g;
   double b;

   Colour operator*=(double s) { r*=s; g*=s; b*=s; return *this; }
};

inline Colour operator*(Colour c, double s) { return Colour {c.r*s, c.g*s, c.b*s}; }
inline Colour operator+(Colour a, Colour b) { return Colour {a.r+b.r, a.g+b.g, a.b+b.b}; }
inline Colour operator-(Colour a, Colour b) { return Colour {a.r-b.r, a.g-b.g, a.b-b.b}; }

inline Colour lerp(double f, Colour a, Colour b) { return a + (b - a) * f; }