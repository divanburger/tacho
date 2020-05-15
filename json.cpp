//
// Created by divan on 07/03/19.
//

#include "json.h"

JsonNumber json_parse_number(const char **data) {
   JsonNumber number = {};
   number.type = JNUM_INTEGER;

   const char *ptr = *data;
   if (*ptr == '0' && *(ptr + 1) == 'x') {
      number.type = JNUM_UNSIGNED;
      ptr += 2;

      while (true) {
         int value = 0;
         if (*ptr >= '0' && *ptr <= '9') {
            value = (*ptr - '0');
         } else if (*ptr >= 'A' && *ptr <= 'F') {
            value = (*ptr - 'A' + 10);
         } else if (*ptr >= 'a' && *ptr <= 'f') {
            value = (*ptr - 'a' + 10);
         } else {
            break;
         }

         number.value.i = number.value.i * 16 + value;
         ptr++;
      }
   } else {
      while (*ptr >= '0' && *ptr <= '9') {
         number.value.i = number.value.i * 10 + (*ptr - '0');
         ptr++;
      }
      if (**data == '-') number.value.i *= -1;
   }
   assert(*data != ptr);

   *data = ptr;
   return number;
}

bool json_lexer_number(JsonLexer *lexer) {
   const char *start = lexer->data;

   JsonTok token = {};
   token.index = (lexer->data - lexer->start);
   token.type = JTOK_INTEGER;

   token.number = json_parse_number(&lexer->data);

   token.text = str_copy(lexer->allocator, start, (int) (lexer->data - start));
   arl_push(&lexer->tokens, token);
   return true;
}

bool json_lexer_name(JsonLexer *lexer) {
   const char *start = lexer->data;

   while ((*lexer->data >= 'a' && *lexer->data <= 'z') || (*lexer->data >= 'A' && *lexer->data <= 'Z')) lexer->data++;

   JsonTok token;
   token.index = (start - lexer->start);
   token.text = str_copy(lexer->allocator, start, (int) (lexer->data - start));
   token.type = JTOK_NAME;
   arl_push(&lexer->tokens, token);
   return true;
}

bool json_lexer_string(JsonLexer *lexer) {
   if (*lexer->data != '"') return false;
   lexer->data++;

   const char *start = lexer->data;
   while (*lexer->data) {
      if (*lexer->data == '"') {
         break;
      } else if (*lexer->data == '\\') {
         lexer->data++;
         if (*lexer->data == '"') {
            lexer->data++;
         } else if (*lexer->data == '\\') {
            lexer->data++;
         }
      } else {
         lexer->data++;
      }
   }

   if (*lexer->data != '"') return false;
   lexer->data++;

   JsonTok token = {};
   token.index = start - lexer->start;
   token.text = str_copy(lexer->allocator, start, (int) (lexer->data - start - 1));
   token.type = JTOK_STRING;
   arl_push(&lexer->tokens, token);
   return true;
}

void json_init() {
   json_first_char[' '] = JFCA_SKIP;
   json_first_char['\r'] = JFCA_SKIP;
   json_first_char['\n'] = JFCA_SKIP;
   json_first_char['\t'] = JFCA_SKIP;
   json_first_char['{'] = JFCA_TOKEN;
   json_first_char['}'] = JFCA_TOKEN;
   json_first_char['['] = JFCA_TOKEN;
   json_first_char[']'] = JFCA_TOKEN;
   json_first_char[':'] = JFCA_TOKEN;
   json_first_char[','] = JFCA_TOKEN;
   json_first_char['"'] = JFCA_STRING;
   json_first_char['-'] = JFCA_NUMBER;
   for (char c = '0'; c <= '9'; c++) json_first_char[c] = JFCA_NUMBER;
   for (char c = 'a'; c <= 'z'; c++) json_first_char[c] = JFCA_KEYWORD;
   for (char c = 'A'; c <= 'Z'; c++) json_first_char[c] = JFCA_KEYWORD;
}

bool json_lexer(JsonLexer *lexer, const char *data, Allocator *allocator) {
   arl_init(&lexer->tokens, allocator);
   lexer->allocator = allocator;
   lexer->start = data;
   lexer->data = data;

   JsonTok token = {};

   while (*lexer->data) {
      switch (json_first_char[*lexer->data]) {
         case JFCA_TOKEN:
            token.index = (lexer->data - lexer->start);
            token.text = str_copy(lexer->allocator, lexer->data, 1);
            token.type = (JsonTokType) *lexer->data;
            arl_push(&lexer->tokens, token);
         case JFCA_SKIP:
            lexer->data++;
            break;
         case JFCA_STRING:
            if (!json_lexer_string(lexer)) return false;
            break;
         case JFCA_NUMBER:
            if (!json_lexer_number(lexer)) return false;
            break;
         case JFCA_KEYWORD:
            if (!json_lexer_name(lexer)) return false;
            break;
         default:
            printf("%u: Unknown character: %c\n", (u32) (lexer->data - lexer->start) + 1, *lexer->data);
            return false;
      }
   }

   token.index = lexer->data - lexer->start;
   token.text = String{};
   token.type = JTOK_EOF;
   arl_push(&lexer->tokens, token);

   lexer->token_cursor = {};
   return true;
}

JsonTok json_lexer_next(JsonLexer *lexer) {
   JsonTok result = {};

   if (arl_cursor_step(&lexer->tokens, &lexer->token_cursor)) result = *arl_cursor_get<JsonTok>(lexer->token_cursor);

   return result;
}

void json_next(JsonParser *parser) {
   parser->token = json_lexer_next(&parser->lexer);
}

bool json_expect(JsonParser *parser, JsonTokType type) {
   if (parser->token.type == type) return true;
   printf("%u: Unexpected token type=%i '%.*s', expected %i\n", parser->token.index + 1, parser->token.type,
          str_prt(parser->token.text), type);
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
      case JTOK_INTEGER:
      case JTOK_UNSIGNED:
         if (parser->on_literal) parser->on_literal(parser->user_data, parser->token);
         json_next(parser);
         return true;
      case JTOK_NAME:
         if (str_equal(parser->token.text, const_as_string("true")) ||
             str_equal(parser->token.text, const_as_string("false")) ||
             str_equal(parser->token.text, const_as_string("null"))) {
            if (parser->on_literal) parser->on_literal(parser->user_data, parser->token);
            json_next(parser);
            return true;
         } else {
            printf("Unknown name '%.*s'\n", str_prt(parser->token.text));
         }
      case JTOK_LBRACE:
         return json_parse_object(parser);
      case JTOK_LBRACKET:
         return json_parse_array(parser);
      default:
         printf("%u: Unexpected token type=%i '%.*s', expected a string, literal, number or object\n",
                parser->token.index + 1, parser->token.type, str_prt(parser->token.text));
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

bool json_parse(JsonParser *parser, const char *data, Allocator *allocator) {
   parser->allocator = allocator;
   if (!json_lexer(&parser->lexer, data, allocator)) return false;

//   for (auto cursor = arl_cursor_start(&parser->lexer.tokens); arl_cursor_valid(cursor); arl_cursor_step(&cursor)) {
//      auto token = *arl_cursor_get(cursor);
//      printf("token: %u: type=%i text=%.*s\n", token.index, token.type, str_prt(token.text));
//   }

   json_next(parser);
   auto result = json_parse_value(parser);
   return result;
}