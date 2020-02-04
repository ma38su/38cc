/* mcc.h */
#ifndef MCC_H
#define MCC_H

#include "vector.h"

typedef enum {
  ND_NUM,     // 0 integer number
  ND_LVAR,    // 1 local variable
  ND_GVAR,    // 2 global variable
  ND_DEREF,   // 3
  ND_ADDR,    // 4
  ND_EQ,      // 5 ==
  ND_NE,      // 6 !=
  ND_LT,      // 7 <
  ND_LE,      // 8 <=
  ND_BLOCK,   // 9 {}
  ND_IF,      // 10 return
  ND_ELSE,    // 11 return
  ND_WHILE,   // 12 return
  ND_NONE,    // 13 return
  ND_FOR,     // 14 return
  ND_ADD,     // 15 +
  ND_SUB,     // 16 -
  ND_MUL,     // 17 *
  ND_DIV,     // 18 /
  ND_MOD,     // 19 %
  ND_ASSIGN,  // 20 =
  ND_RETURN,  // 21 return
  ND_FUNCTION,// 22
  ND_CALL,    // 23
} NodeKind;

typedef enum {
  TK_SIZEOF,    // sizeof
  TK_RESERVED,  // simbol +, -
  TK_IDENT,     // identity
  TK_NUM,       // integer number
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
