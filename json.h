//
// Created by divan on 07/03/19.
//

#pragma once

#include "array_list.h"
#include "types.h"
#include "string.h"

enum JsonNumberType : u8 {
   JNUM_UNKNOWN,
   JNUM_INTEGER,
   JNUM_UNSIGNED,
   JNUM_FLOAT
};

struct JsonNumber {
   JsonNumberType type;
   union {
      i64 i;
      u64 u;
      double f;
   } value;
};

enum JsonTokType : u8 {
   JTOK_EOF,
   JTOK_STRING,
   JTOK_INTEGER,
   JTOK_UNSIGNED,
   JTOK_FLOAT,
   JTOK_LBRACE,
   JTOK_RBRACE,
   JTOK_LBRACKET,
   JTOK_RBRACKET,
   JTOK_COLON,
   JTOK_COMMA,
   JTOK_NAME
};

struct JsonTok {
   JsonTokType type;
   u32 index;
   String text;
   JsonNumber number;
};

struct JsonLexer {
   Allocator *allocator;

   const char *start;
   const char *data;
   ArrayList<JsonTok> tokens;

   ArrayListCursor<JsonTok> token_cursor;
};

using JsonParserFunc = void (*)(void *, JsonTok);

struct JsonParser {
   Allocator *allocator;

   JsonLexer lexer;
   JsonTok token;

   void *user_data;
   JsonParserFunc on_object_start;
   JsonParserFunc on_object_end;
   JsonParserFunc on_array_start;
   JsonParserFunc on_array_end;
   JsonParserFunc on_key;
   JsonParserFunc on_literal;
};

JsonNumber json_parse_number(const char **data);

bool json_lexer(JsonLexer *lexer, const char *data, Allocator *allocator);

JsonTok json_lexer_next(JsonLexer *lexer);

bool json_parse(JsonParser *parser, const char *data, Allocator *allocator);