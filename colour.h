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

Colour operator*(Colour c, double s) { return Colour {c.r*s, c.g*s, c.b*s}; }