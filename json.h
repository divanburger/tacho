//
// Created by divan on 07/03/19.
//

#pragma once

#include "array_list.h"
#include "types.h"
#include "string.h"

enum JsonTokType : u16 {
   JTOK_EOF,
   JTOK_STRING,
   JTOK_NUMBER,
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
   String text;
};

struct JsonLexer {
   MemoryArena *arena;
   const char *data;
   ArrayList<JsonTok> tokens;

   ArrayListCursor<JsonTok> token_cursor;
};

using JsonParserFunc = void (*)(void*, JsonTok);

struct JsonParser {
   MemoryArena *arena;

   JsonLexer lexer;
   JsonTok token;

   void* user_data;
   JsonParserFunc on_object_start;
   JsonParserFunc on_object_end;
   JsonParserFunc on_array_start;
   JsonParserFunc on_array_end;
   JsonParserFunc on_key;
   JsonParserFunc on_literal;
};

bool json_lexer(JsonLexer *lexer, const char *data, MemoryArena *arena);

JsonTok json_lexer_next(JsonLexer *lexer);

void json_parse(JsonParser *parser, const char *data, MemoryArena *arena);