//
// Created by divan on 07/03/19.
//

#include <cstdio>

#include "heap.h"
#include "util.h"
#include "memory.h"
#include "json.h"

enum HeapReaderKey {
   HRK_UNKNOWN,
   HRK_ADDRESS,
   HRK_TYPE,
   HRK_OLD,
   HRK_EMBEDDED,
   HRK_MARKED,
   HRK_REFERENCES,
   HRK_VALUE
};

struct HeapReader {
   Allocator *allocator;
   Heap *heap;

   Object object;
   HeapReaderKey key;

   ArrayList<Object> objects;
   HashTable<u64> page_table;
};

void heap_on_key(void *user_data, JsonTok token) {
   auto reader = (HeapReader *) user_data;
//   printf("key: %.*s\n", token.text);

   reader->key = HRK_UNKNOWN;
   if (str_equal(token.text, const_as_string("address"))) {
      reader->key = HRK_ADDRESS;
   } else if (str_equal(token.text, const_as_string("type"))) {
      reader->key = HRK_TYPE;
   } else if (str_equal(token.text, const_as_string("marked"))) {
      reader->key = HRK_MARKED;
   } else if (str_equal(token.text, const_as_string("embedded"))) {
      reader->key = HRK_EMBEDDED;
   } else if (str_equal(token.text, const_as_string("old"))) {
      reader->key = HRK_OLD;
   } else if (str_equal(token.text, const_as_string("references"))) {
      reader->key = HRK_REFERENCES;
   } else if (str_equal(token.text, const_as_string("value"))) {
      reader->key = HRK_VALUE;
   } else {
      reader->key = HRK_UNKNOWN;
   }
}

void heap_on_literal(void *user_data, JsonTok token) {
   auto reader = (HeapReader *) user_data;
//   printf("literal: %.*s\n", token.text);

   switch (reader->key) {
      case HRK_ADDRESS: {
         const char *ptr = token.text.data;
         JsonNumber number = json_parse_number(&ptr);
         reader->object.address = number.value.u;
         break;
      }
      case HRK_TYPE: {
         if (str_equal(token.text, const_as_string("STRING"))) {
            reader->object.type = ObjectType::HO_STRING;
         } else if (str_equal(token.text, const_as_string("HASH"))) {
            reader->object.type = ObjectType::HO_HASH;
         } else if (str_equal(token.text, const_as_string("OBJECT"))) {
            reader->object.type = ObjectType::HO_OBJECT;
         } else if (str_equal(token.text, const_as_string("ARRAY"))) {
            reader->object.type = ObjectType::HO_ARRAY;
         } else if (str_equal(token.text, const_as_string("DATA"))) {
            reader->object.type = ObjectType::HO_DATA;
         } else if (str_equal(token.text, const_as_string("CLASS"))) {
            reader->object.type = ObjectType::HO_CLASS;
         } else if (str_equal(token.text, const_as_string("IMEMO"))) {
            reader->object.type = ObjectType::HO_IMEMO;
         } else if (str_equal(token.text, const_as_string("STRUCT"))) {
            reader->object.type = ObjectType::HO_STRUCT;
         } else if (str_equal(token.text, const_as_string("RATIONAL"))) {
            reader->object.type = ObjectType::HO_RATIONAL;
         } else if (str_equal(token.text, const_as_string("MATCH"))) {
            reader->object.type = ObjectType::HO_MATCH;
         } else if (str_equal(token.text, const_as_string("REGEXP"))) {
            reader->object.type = ObjectType::HO_REGEXP;
         } else if (str_equal(token.text, const_as_string("SYMBOL"))) {
            reader->object.type = ObjectType::HO_SYMBOL;
         } else if (str_equal(token.text, const_as_string("ICLASS"))) {
            reader->object.type = ObjectType::HO_ICLASS;
         } else if (str_equal(token.text, const_as_string("FILE"))) {
            reader->object.type = ObjectType::HO_FILE;
         } else if (str_equal(token.text, const_as_string("BIGNUM"))) {
            reader->object.type = ObjectType::HO_BIGNUM;
         } else if (str_equal(token.text, const_as_string("FLOAT"))) {
            reader->object.type = ObjectType::HO_FLOAT;
         } else if (str_equal(token.text, const_as_string("COMPLEX"))) {
            reader->object.type = ObjectType::HO_COMPLEX;
         } else if (str_equal(token.text, const_as_string("MODULE"))) {
            reader->object.type = ObjectType::HO_MODULE;
         } else if (str_equal(token.text, const_as_string("NODE"))) {
            reader->object.type = ObjectType::HO_NODE;
         } else if (str_equal(token.text, const_as_string("ZOMBIE"))) {
            reader->object.type = ObjectType::HO_ZOMBIE;
         } else if (str_equal(token.text, const_as_string("ROOT"))) {
            reader->object.type = ObjectType::HO_ROOT;
         } else {
            printf("UNKNOWN: %.*s\n", str_prt(token.text));
         }
         break;
      }
      case HRK_MARKED:
         if (str_equal(token.text, const_as_string("true"))) reader->object.flags |= OBJFLAG_MARKED;
         break;
      case HRK_OLD:
         if (str_equal(token.text, const_as_string("true"))) reader->object.flags |= OBJFLAG_OLD;
         break;
      case HRK_EMBEDDED:
         if (str_equal(token.text, const_as_string("true"))) reader->object.flags |= OBJFLAG_EMDEDDED;
         break;
      case HRK_REFERENCES: {
         const char *ptr = token.text.data;
         JsonNumber number = json_parse_number(&ptr);
         arl_push(&reader->object.references, number.value.u);
         break;
      }
      case HRK_VALUE:
         reader->object.value = str_copy(reader->allocator, token.text);
         break;
      default:
         break;
   }
}

HeapLocation heap_find_object(Heap *heap, u64 address) {
   HeapLocation location = {};

   u64 page_id = address >> 14U;
   Page *page = nullptr;

   u64 low = 0;
   u64 high = heap->page_count - 1;

   while (low < high) {
      auto mid = (high + low) >> 1UL;
      auto cur_page = heap->pages[mid];
      auto cur_page_id = cur_page->id;

      if (cur_page_id < page_id) {
         low = mid + 1;
      } else if (cur_page_id > page_id) {
         high = mid - 1;
      } else {
         page = cur_page;
         break;
      }
   }

   if (page) {
      location.page = page;
      location.slot_index = (address - location.page->slot_start_address) / 40;
   }
   return location;
}

bool heap_read_object(HeapReader *reader, const char *str, Allocator *temp_allocator) {
   reader->object = {};
   arl_init(&reader->object.referenced_by, reader->allocator);
   arl_init(&reader->object.references, reader->allocator);

   JsonParser parser = {};
   parser.user_data = reader;
   parser.on_key = heap_on_key;
   parser.on_literal = heap_on_literal;
   return json_parse(&parser, str, temp_allocator);
}

void heap_read(Heap *heap, const char *filename) {
   ArenaAllocator heap_allocator = arena_make();
   HeapReader reader = {};
   reader.allocator = (Allocator *) &heap_allocator;

   heap->allocator = (Allocator *) &heap_allocator;
   arl_init(&heap->objects, heap->allocator);
   ht_init(&reader.page_table);

   FILE *input;

   size_t buffer_size = 1024 * 1024 * 16;
   char *buffer = (char *) malloc(buffer_size);

   input = fopen(filename, "r");

   ArenaAllocator arena = arena_make();
   auto allocator = (Allocator *) &arena;

   while (fgets(buffer, buffer_size, input)) {
      auto section = arena_temp_begin(&arena);

      if (!heap_read_object(&reader, buffer, allocator)) {
//         printf("%s\n", buffer);
         printf("PARSING FAILED!\n");
         arena_temp_end(section);
         break;
      }

      Object *object = arl_push(&heap->objects, reader.object);
//      printf("object: address=0x%08lx type=%s\n", reader.object.address, object_type_names[reader.object.type]);

      if (object->address > 0) {
         u64 page_id = (object->address >> 14U);
         auto page = (Page *) ht_find(&reader.page_table, page_id);
         if (!page) {
            page = std_alloc_type(heap->allocator, Page);
            page->id = page_id;

            page->slot_start_address = (page->id << 14U) + 8;
            auto delta = page->slot_start_address % 40;
            if (delta > 0) page->slot_start_address += 40 - delta;

            page->slot_count = 0;
            ht_add(&reader.page_table, page_id, page);
         }

         u64 slot_index = (object->address - page->slot_start_address) / 40;
         page->slots[slot_index] = object;
         page->slot_count++;
      }

      arena_temp_end(section);
   }

   arena_free(&arena);

   arena_stats_print_(&heap_allocator, "before pages");

   heap->pages = std_alloc_array(heap->allocator, Page*, reader.page_table.count);
   for (auto cursor = th_cursor_start(&reader.page_table); th_cursor_valid(&reader.page_table,
                                                                           cursor); cursor = th_cursor_step(
         &reader.page_table, cursor)) {
      heap->pages[heap->page_count++] = (Page *) reader.page_table.items[cursor];
   }

   qsort(heap->pages, heap->page_count, sizeof(Page *), [](const void *pa, const void *pb) -> int {
      auto a = *(Page **) pa, b = *(Page **) pb;
      if (a->id < b->id) return -1;
      if (a->id > b->id) return 1;
      return 0;
   });

   u64 last_page_id = 0;
   u64 xpos = 0;
   for (i64 page_index = 0; page_index < heap->page_count; page_index++) {
      auto page = heap->pages[page_index];
      auto page_id = page->id;

      if (last_page_id == 0) last_page_id = page_id;

      u64 offset = page_id - last_page_id;
      if (offset > 1000) offset = 1000;
      xpos += offset;

      last_page_id = page_id;
      page->xpos = xpos;
   }
   heap->max_xpos = xpos;

   arena_stats_print_(&heap_allocator, "before referenced_by");

   ArrayListCursor cursor = {};
   while (arl_cursor_step(&heap->objects, &cursor)) {
      auto object = arl_cursor_get<Object>(cursor);

      ArrayListCursor ref_cur = {};
      while (arl_cursor_step(&object->references, &ref_cur)) {
         auto address = *arl_cursor_get<u64>(ref_cur);
         auto location = heap_find_object(heap, address);
         if (location.page && location.slot_index >= 0) {
            auto other = location.page->slots[location.slot_index];
            if (other) arl_push(&other->referenced_by, object);
         }
      }
   }

   arena_stats_print_(&heap_allocator, "finished");

   fclose(input);
   free(buffer);
}