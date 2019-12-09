/* mcc.h */
#ifndef MCC_H
#define MCC_H


typedef enum {
  ND_ADD,  // +
  ND_SUB,  // -
  ND_MUL,  // *
  ND_DIV,  // /
  ND_MOD,  // %
  ND_EQ,
  ND_NE,
  ND_LT,  // <
  ND_LE,  // <=
  ND_NUM,  // integer number
} NodeKind;

typedef enum {
  TK_RESERVED,  // simbol +, -
  TK_NUM,       // integer number
  TK_EOF,       // Eno of File
} TokenKind;

typedef struct Token Token;

struct Token {
  TokenKind kind;
  Token *next;
  int val;    // integer value if kind == TK_NUM
  char *str;  // length of string
  int len;    // length of token
};

typedef struct Node Node;

struct Node {
  NodeKind kind;
  Node *lhs;  // left-hand side
  Node *rhs;  // right-hand side
  int val;    // only use if kind is ND_NUM
};

// global vars
extern char *user_input;
extern Token *token;

void print_header();
Node *expr();
Node *mul();

void gen(Node *node);
Token *tokenize(char *p);
void print_header();

void error_at(char *loc, char *fmt, ...);
void error(char *fmt, ...);

#endif

