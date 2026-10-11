#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t code_point;

typedef enum {
  JSON3_UNDEFINED = 0,
  JSON3_OBJECT = 1 << 0,
  JSON3_ARRAY = 1 << 1,
  JSON3_STRING = 1 << 2,
  JSON3_PRIMITIVE = 1 << 3
} json3_token_type;

enum json3_parser_error {
  /* Not enough tokens were provided */
  JSON3_PARSER_ERROR_NOMEM = -1,
  /* Invalid code point for the current parser state */
  JSON3_PARSER_ERROR_INVAL = -2,
  /* The input is not a complete JSON text, more code points expected */
  JSON3_PARSER_ERROR_PART = -3
};

typedef struct json3_token {
  json3_token_type type;
  int start;
  int end;
  int size;
  int parent;
} json3_token_t;

typedef struct json3_parser {
  struct json3_token *tokens;   /* caller-provided token buffer */
  unsigned int num_tokens;      /* size of the token buffer */
  unsigned int toknext;         /* next token to allocate */
  int toksuper;                 /* stack top: superior token index, or -1 */
  int state;                    /* state machine state */
  int pos;                      /* index of the next input code point */
  int error;                    /* sticky error code, 0 while ok */
} json3_parser_t;

void json3_parser_init(json3_parser_t *parser, json3_token_t *tokens,
                       unsigned int num_tokens);

int json3_parser_feed(json3_parser_t *parser, code_point cp);

int json3_parser_done(json3_parser_t *parser);

#ifdef __cplusplus
}
#endif
