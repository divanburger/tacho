This is the viewer application for the [tacho ruby gem](https://github.com/divanburger/tacho-ruby).

# Code style

Even if the application is written in C++, we use classic C-style with some C++ features.
Notably we use the following C++ features:

* operator overloading
* method overloading
* auto keyword (type inference)

# Libraries

1. SDL2
2. Cairo

# Building

1. Goto repository directory
2. $ mkdir build
3. $ cd build
4. $ cmake ..
5. $ make
6. $ ./tacho