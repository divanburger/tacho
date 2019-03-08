//
// Created by divan on 07/03/19.
//

#include "json.h"

bool json_lexer_number(JsonLexer *lexer) {
   const char *start = lexer->data;

   while ((*lexer->data >= '0' && *lexer->data <= '9') || *lexer->data == '.') lexer->data++;

   JsonTok token;
   token.text = str_copy(lexer->arena, start, (int) (lexer->data - start));
   token.type = JTOK_NUMBER;
   arl_push(&lexer->tokens, token);
   return true;
}

bool json_lexer_name(JsonLexer *lexer) {
   const char *start = lexer->data;

   while ((*lexer->data >= 'a' && *lexer->data <= 'z') || (*lexer->data >= 'A' && *lexer->data <= 'Z')) lexer->data++;

   JsonTok token;
   token.text = str_copy(lexer->arena, start, (int) (lexer->data - start));
   token.type = JTOK_NAME;
   arl_push(&lexer->tokens, token);
   return true;
}

bool json_lexer_string(JsonLexer *lexer) {
   if (*lexer->data != '"') return false;
   lexer->data++;

   const char *start = lexer->data;
   while (*lexer->data && *lexer->data != '"') lexer->data++;

   if (*lexer->data != '"') return false;
   lexer->data++;

   JsonTok token;
   token.text = str_copy(lexer->arena, start, (int) (lexer->data - start - 1));
   token.type = JTOK_STRING;
   arl_push(&lexer->tokens, token);
   return true;
}

bool json_lexer(JsonLexer *lexer, const char *data, MemoryArena *arena) {
   arl_init(&lexer->tokens);
   lexer->arena = arena;
   lexer->data = data;

   JsonTok token;

   while (*lexer->data) {
      switch (*lexer->data) {
         case ' ':
         case '\r':
         case '\n':
         case '\t':
            lexer->data++;
            break;
         case '{':
            token.text = str_copy(lexer->arena, lexer->data, 1);
            token.type = JTOK_LBRACE;
            arl_push(&lexer->tokens, token);
            lexer->data++;
            break;
         case '}':
            token.text = str_copy(lexer->arena, lexer->data, 1);
            token.type = JTOK_RBRACE;
            arl_push(&lexer->tokens, token);
            lexer->data++;
            break;
         case '[':
            token.text = str_copy(lexer->arena, lexer->data, 1);
            token.type = JTOK_LBRACKET;
            arl_push(&lexer->tokens, token);
            lexer->data++;
            break;
         case ']':
            token.text = str_copy(lexer->arena, lexer->data, 1);
            token.type = JTOK_RBRACKET;
            arl_push(&lexer->tokens, token);
            lexer->data++;
            break;
         case ':':
            token.text = str_copy(lexer->arena, lexer->data, 1);
            token.type = JTOK_COLON;
            arl_push(&lexer->tokens, token);
            lexer->data++;
            break;
         case ',':
            token.text = str_copy(lexer->arena, lexer->data, 1);
            token.type = JTOK_COMMA;
            arl_push(&lexer->tokens, token);
            lexer->data++;
            break;
         case '"':
            if (!json_lexer_string(lexer)) return false;
            break;
         default:
            if (*lexer->data >= '0' && *lexer->data <= '9') {
               if (!json_lexer_number(lexer)) return false;
            } else if ((*lexer->data >= 'a' && *lexer->data <= 'z') || (*lexer->data >= 'A' && *lexer->data <= 'Z')) {
               if (!json_lexer_name(lexer)) return false;
            } else {
               printf("Unknown character: %c\n", *lexer->data);
               return false;
            }
            break;
      }
   }

   lexer->token_cursor = arl_cursor_start(&lexer->tokens);
   return true;
}

JsonTok json_lexer_next(JsonLexer *lexer) {
   JsonTok result = {};

   if (arl_cursor_valid(lexer->token_cursor)) result = *arl_cursor_get(lexer->token_cursor);

   arl_cursor_step(&lexer->token_cursor);
   return result;
}

void json_next(JsonParser *parser) {
   parser->token = json_lexer_next(&parser->lexer);
}

bool json_expect(JsonParser *parser, JsonTokType type) {
   if (parser->token.type == type) return true;
   printf("Unexpected token %i, expected %i\n", parser->token.type, type);
   return false;
}

bool json_expect_and_next(JsonParser *parser, JsonTokType type) {
   if (json_expect(parser, type)) {
      json_next(parser);
      return true;
   }
   return false;
}

JsonTokType json_tok_type(JsonParser *parser) {
   return parser->token.type;
}

bool json_parse_array(JsonParser *parser);
bool json_parse_object(JsonParser *parser);

bool json_parse_value(JsonParser *parser) {
   switch (json_tok_type(parser)) {
      case JTOK_STRING:
      case JTOK_NAME:
      case JTOK_NUMBER:
         if (parser->on_literal) parser->on_literal(parser->user_data, parser->token);
         json_next(parser);
         return true;
      case JTOK_LBRACE:
         return json_parse_object(parser);
      case JTOK_LBRACKET:
         return json_parse_array(parser);
      default:
         printf("Unexpected token %i, expected a string, literal, number or object\n", parser->token.type);
         return false;
   }
}

bool json_parse_array(JsonParser *parser) {
   if (!json_expect_and_next(parser, JTOK_LBRACKET)) return false;

   if (parser->on_array_start) parser->on_array_start(parser->user_data, parser->token);

   while (true) {
      if (!json_parse_value(parser)) return false;
      if (json_tok_type(parser) != JTOK_COMMA) break;
      json_next(parser);
   }

   if (parser->on_array_end) parser->on_array_end(parser->user_data, parser->token);

   return json_expect_and_next(parser, JTOK_RBRACKET);
}

bool json_parse_object(JsonParser *parser) {
   if (!json_expect_and_next(parser, JTOK_LBRACE)) return false;

   if (parser->on_object_start) parser->on_object_start(parser->user_data, parser->token);

   while (true) {
      if (!json_expect(parser, JTOK_STRING)) return false;

      if (parser->on_key) parser->on_key(parser->user_data, parser->token);
      json_next(parser);

      if (!json_expect_and_next(parser, JTOK_COLON)) return false;

      if (!json_parse_value(parser)) return false;

      if (json_tok_type(parser) != JTOK_COMMA) break;
      json_next(parser);
   }

   if (parser->on_object_end) parser->on_object_end(parser->user_data, parser->token);

   return json_expect_and_next(parser, JTOK_RBRACE);
}

void json_parse(JsonParser *parser, const char *data, MemoryArena *arena) {
   parser->arena = arena;
   json_lexer(&parser->lexer, data, arena);
   json_next(parser);
   json_parse_value(parser);
}