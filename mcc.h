/* mcc.h */
#ifndef MCC_H
#define MCC_H

#include "vector.h"

typedef enum {
  ND_FUNCTION,
  ND_LVAR,   // local variable
  ND_NUM,    // integer number
  ND_EQ,     // ==
  ND_NE,     // !=
  ND_LT,     // <
  ND_LE,     // <=
  ND_BLOCK,  // {}
  ND_IF,     // return
  ND_ELSE,   // return
  ND_WHILE,  // return
  ND_NONE,   // return
  ND_FOR,    // return
  ND_ADD,    // +
  ND_SUB,    // -
  ND_MUL,    // *
  ND_DIV,    // /
  ND_MOD,    // %
  ND_ASSIGN, // =
  ND_RETURN, // return
} NodeKind;

typedef enum {
  TK_RESERVED,  // simbol +, -
  TK_IDENT,     // identity
  TK_NUM,       // integer number
  TK_RETURN,    // return
  TK_IF,    // return
  TK_ELSE,    // return
  TK_FOR,    // return
  TK_WHILE,    // return
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
  Node *lhs;   // left-hand side
  Node *rhs;   // right-hand side
  char *ident; // for function
  Vector *list;
  int val;    // only use if kind is ND_NUM
  int offset;
};

typedef struct LVar LVar;
struct LVar {
  LVar *next;
  char *name;
  int len;
  int offset;
};

// global vars
extern char *user_input;
extern Token *token;
extern Node *code[];
extern LVar *locals;

void print_header();
Node *expr();
Node *mul();

void gen(Node *node);
void tokenize(char *p);
void print_header();

LVar *find_lvar(Token *tok);
void error_at(char *loc, char *fmt, ...);
void error(char *fmt, ...);
void program();

#endif

