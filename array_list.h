//
// Created by divan on 30/01/19.
//

#pragma once

template<typename T>
struct ArrayListBlock {
   T data[64];
   i64 count;
   ArrayListBlock<T> *prev;
   ArrayListBlock<T> *next;
};

template<typename T>
struct ArrayListCursor {
   ArrayListBlock<T> *block;
   i64 index;
};

template<typename T>
struct ArrayList {
   i64 count;
   ArrayListBlock<T> *first;
   ArrayListBlock<T> *last;
};

template<typename T>
void arl_init(ArrayList<T> *arl) {
   arl->count = 0;
   arl->first = nullptr;
   arl->last = nullptr;
}

template<typename T>
void arl_grow(ArrayList<T> *arl) {
   ArrayListBlock<T> *new_block = raw_alloc_type_zero(ArrayListBlock<T>);

   if (arl->last) {
      arl->last->next = new_block;
      new_block->prev = arl->last;
      arl->last = new_block;
   } else {
      arl->first = new_block;
      arl->last = new_block;
   }
}

template<typename T>
T* arl_push(ArrayList<T> *arl, T item) {
   if (!arl->last || arl->last->count == array_size(arl->last->data)) arl_grow(arl);
   assert(arl->last->count < array_size(arl->last->data));

   arl->count++;
   auto ptr = arl->last->data + (arl->last->count++);
   *ptr = item;
   return ptr;
}

template<typename T>
T* arl_push(ArrayList<T> *arl) {
   if (!arl->last || arl->last->count == array_size(arl->last->data)) arl_grow(arl);
   assert(arl->last->count < array_size(arl->last->data));

   arl->count++;
   return arl->last->data + (arl->last->count++);
}

template <typename T>
bool arl_pop(ArrayList<T> *arl) {
   if (!arl->last) return false;

   if (arl->last->count > 0) {
      arl->last->count--;
   } else {
      auto block = arl->last;
      arl->last = block->prev;

      if (arl->last) {
         arl->last->next = nullptr;
      } else {
         arl->first = nullptr;
      }

      raw_free(block);
   }
}

template<typename T>
ArrayListCursor<T> arl_cursor_start(ArrayList<T> *arl) {
   ArrayListCursor<T> cursor;
   cursor.block = arl->first;
   cursor.index = 0;
   return cursor;
}

template<typename T>
bool arl_cursor_valid(ArrayListCursor<T> cursor) {
   return cursor.block;
}

template<typename T>
bool arl_cursor_step(ArrayListCursor<T> *cursor) {
   if (!cursor->block) return false;

   cursor->index++;
   if (cursor->index >= array_size(cursor->block->data)) {
      cursor->block = cursor->block->next;
      cursor->index = 0;
   }

   return cursor->block;
}

template<typename T>
T* arl_cursor_get(ArrayListCursor<T> cursor) {
   if (!cursor.block) return nullptr;
   return cursor.block->data + cursor.index;
}