/* mcc.h */
#ifndef MCC_H
#define MCC_H

#include "vector.h"

typedef enum {
  ND_NUM,     // 0 integer number
  ND_STR,     // 1 string literal
  ND_LVAR,    // 2 local variable
  ND_GVAR,    // 3 global variable
  ND_DEREF,   // 4
  ND_ADDR,    // 5
  ND_EQ,      // 6 ==
  ND_NE,      // 7 !=
  ND_LT,      // 8 <
  ND_LE,      // 9 <=
  ND_BLOCK,   // 10 {}
  ND_IF,      // 11 return
  ND_ELSE,    // 12 return
  ND_WHILE,   // 13 return
  ND_NONE,    // 14 return
  ND_FOR,     // 15 return
  ND_ADD,     // 16 +
  ND_SUB,     // 17 -
  ND_MUL,     // 18 *
  ND_DIV,     // 19 /
  ND_MOD,     // 20 %
  ND_ASSIGN,  // 21 =
  ND_RETURN,  // 22 return
  ND_FUNCTION,// 23
  ND_CALL,    // 24
} NodeKind;

typedef enum {
  TK_RESERVED,  // simbol +, -
  TK_NUM,       // integer number
  TK_STR,       // 1 string literal
  TK_SIZEOF,    // sizeof
  TK_IDENT,     // identity
  TK_RETURN,    // return
  TK_IF,        // return
  TK_ELSE,      // return
  TK_FOR,       // return
  TK_WHILE,     // return
  TK_EOF,       // End of File
} TokenKind;

typedef struct Type Type;
typedef struct Token Token;
typedef struct Node Node;
typedef struct LVar LVar;
typedef struct GVar GVar;
typedef struct Function Function;

struct Type {
  char *name;
  int len;
  int size;
  Type *ptr_to;
};

struct Token {
  TokenKind kind;
  Token *next;
  int val;    // integer value if kind == TK_NUM
  char *str;  // length of string
  int len;    // length of token
};

struct Node {
  NodeKind kind;
  Node *lhs;    // left-hand side
  Node *rhs;    // right-hand side
  char *ident;  // for function
  Vector *list;
  int val;      // for ND_NUM
  Type *type;    // for lvar
  int offset;   // for lvar
};

struct LVar {
  LVar *next;
  Type *type;
  char *name;
  int len;
  int offset;
};

struct GVar {
  GVar *next;
  Type *type;
  char *name;
  int len;
};

struct Function {
  Type *ret_type;
  char *name;
  int len;
  int size;
};

// global vars
extern char *user_input;
extern Token *token;
extern Node *code[];
extern LVar *locals;
extern GVar *globals;

extern Type *type_int;
extern Type *type_char;


void print_header();
Node *expr();
int type_is_array(Type *type);
int type_is_ptr(Type *type);
int sizeof_node(Node* node);

void gen_defined(Node *node);
char *substring(char *str, int len);

Token *tokenize(char *p);
void print_header();

LVar *find_var(Token *tok);
LVar *find_lvar(Token *tok);
Type *find_type(Token *tok);
void error_at(char *loc, char *fmt, ...);
void error(char *fmt, ...);
void program();

#endif
