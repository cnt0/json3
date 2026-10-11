#include "json3.h"

#include <string.h>
#include <stdio.h>


int main(void) {
  const char* s = "[ { \"a\": \"b 'd\", \"c\": \"d\" }, {} ]";
  struct json3_token tokens[65536] = {0};

  json3_parser_t scanner;
  json3_parser_init(&scanner, tokens, 65536);

  for (int i = 0; i < strlen(s); ++i) {
    json3_parser_feed(&scanner, s[i]);
  }
  int used = json3_parser_done(&scanner);
  for (int i = 0; i < used; ++i) {
    for (int j = tokens[i].start; j < tokens[i].end; ++j) {
      printf("%c", s[j]);
    }
    puts("");
  }
}
