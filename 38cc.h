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
  ND_ENUM,    // 23
  ND_STRUCT,  // 24 struct
  ND_UNION,   // 25 union
  ND_FUNCTION,// 26
  ND_CALL,    // 27
} NodeKind;

typedef enum {
  TK_RESERVED,  // simbol +, -
  TK_NUM,       // integer number
  TK_CHAR,      // 1 character literal
  TK_STR,       // 1 string literal
  TK_ENUM,      // enum
  TK_STRUCT,    // struct
  TK_UNION,     // union
  TK_CONST,     // const
  TK_VA,        // ...
  TK_EXTERN,
  TK_TYPEDEF,
  TK_SIZEOF,    // sizeof
  TK_IDENT,     // identity
  TK_RETURN,    // return
  TK_IF,        // if
  TK_ELSE,      // else
  TK_FOR,       // for
  TK_WHILE,     // while
  TK_EOF,       // End of File
} TokenKind;

typedef struct Type Type;
typedef struct Token Token;
typedef struct Node Node;
typedef struct LVar LVar;
typedef struct GVar GVar;
typedef struct Function Function;

struct Type {
  enum {
    TY_VOID,
    TY_PRM,
    TY_PTR,
    TY_ARRAY,
    TY_STRUCT,
  } kind;

  char *name;
  int len;
  int size;

  Type *to; // ptr or array. *char = * -> char
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
  int val;      // for ND_NUM or ND_CALL(extern)
  Type *type;   // for lvar
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
  int val;   // int
  char *str; // string
  int extn;
  int init;   // initialized 1 or not 0.
};

struct Function {
  Type *ret_type;
  char *name;
  int len;
  int extn;
};

// global vars
extern char *filename;
extern char *user_input;
extern Token *token;
extern Node *code[];
extern LVar *locals;
extern GVar *globals;

extern Type *char_type;
extern Type *short_type;
extern Type *int_type;
extern Type *long_type;

extern Type *str_type;

int type_is_array(Type *type);
int type_is_ptr(Type *type);
int sizeof_node(Node* node);

char *substring(char *str, int len);

char *read_file(char *path);

LVar *find_var(Token *tok);
LVar *find_lvar(Token *tok);
Type *find_type(Token *tok);

Token *tokenize(char *p);
void program();
void codegen();

void message_at(char *loc, char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
void error(char *fmt, ...);

#endif
