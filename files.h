//
// Created by divan on 07/01/19.
//

#pragma once

#include "string.h"

enum FileType {
   FILE_TYPE_NOT_EXIST,
   FILE_TYPE_OTHER,
   FILE_TYPE_FILE,
   FILE_TYPE_DIRECTORY
};

struct File {
   FileType type;
   String name;
};

struct DirectoryBlock {
   int  count;
   File files[64];

   DirectoryBlock* prev;
   DirectoryBlock* next;
};

struct DirectoryList {
   DirectoryBlock* first;
   DirectoryBlock* last;
   MemoryArena arena;
};

File file_stat(String name);

void file_list_init(DirectoryList* list);
void file_list_free(DirectoryList* list);

File* file_list_add(DirectoryList* list, FileType type, String name);

void file_read_directory(DirectoryList* list, String path);