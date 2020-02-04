/* mcc.h */
#ifndef MCC_H
#define MCC_H

#include "vector.h"

typedef enum {
  ND_NUM,     // integer number
  ND_LVAR,    // local variable
  ND_GVAR,    // global variable
  ND_DEREF,
  ND_ADDR,
  ND_EQ,      // ==
  ND_NE,      // !=
  ND_LT,      // <
  ND_LE,      // <=
  ND_BLOCK,   // {}
  ND_IF,      // return
  ND_ELSE,    // return
  ND_WHILE,   // return
  ND_NONE,    // return
  ND_FOR,     // return
  ND_ADD,     // +
  ND_SUB,     // -
  ND_MUL,     // *
  ND_DIV,     // /
  ND_MOD,     // %
  ND_ASSIGN,  // =
  ND_RETURN,  // return
  ND_FUNCTION,
  ND_CALL,
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

void print_header();
Node *expr();
int is_array(Type *type);
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
