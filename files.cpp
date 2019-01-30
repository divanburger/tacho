//
// Created by divan on 07/01/19.
//

#include "files.h"
#include "util.h"

#include <dirent.h>
#include <sys/stat.h>
#include <cstring>

File file_stat(String name) {
   struct stat s = {};
   File result = {};
   result.name = name;

   if (stat(name.data, &s) == 0) {
      switch (s.st_mode & S_IFMT) {
         case S_IFREG: {
            result.type = FILE_TYPE_FILE;
         } break;
         case S_IFDIR: {
            result.type = FILE_TYPE_DIRECTORY;
         } break;
         default: {
            result.type = FILE_TYPE_OTHER;
         } break;
      }
   }

   return result;
}

void file_list_init(DirectoryList *list) {
   list->last = nullptr;
   list->first = nullptr;
}

void file_list_free(DirectoryList *list) {
   arena_destroy(&list->arena);
   list->last = nullptr;
   list->first = nullptr;
}

File *file_list_add(DirectoryList *list, FileType type, String name) {
   DirectoryBlock *block = list->last;
   if (!block || block->count == array_size(block->files)) {
      DirectoryBlock *new_block = alloc_type(&list->arena, DirectoryBlock);

      new_block->prev = list->last;
      new_block->next = nullptr;
      list->last = new_block;

      if (block) {
         new_block->prev->next = new_block;
      } else {
         list->first = new_block;
      }

      block = new_block;
   }

   File *file = block->files + (block->count++);
   file->type = type;
   file->name = name;
   return file;
}

void file_read_directory(DirectoryList *list, String path) {
   DIR *d = opendir(path.data);
   struct dirent *dir;
   if (d) {
      while ((dir = readdir(d)) != nullptr) {
         if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) {
            continue;
         }

         String name = str_copy(&list->arena, dir->d_name);
         FileType type = FILE_TYPE_OTHER;
         if (dir->d_type == DT_REG) {
            type = FILE_TYPE_FILE;
         } else if (dir->d_type == DT_DIR) {
            type = FILE_TYPE_DIRECTORY;
         }
         file_list_add(list, type, name);
      }
      closedir(d);
   }
}

i64 file_list_count(DirectoryList *list) {
   i64 total = 0;
   for (DirectoryBlock* block = list->first; block; block = block->next) {
      total += block->count;
   }
   return total;
}
