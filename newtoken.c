#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "38cc.h"

Token *new_token(TokenKind kind, Token *cur, char *str);

Token *new_token(TokenKind kind, Token *cur, char *str) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  cur->next = tok;
  return tok;
}
