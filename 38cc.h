/* mcc.h */
#ifndef MCC_H
#define MCC_H

#include "vector.h"

typedef enum {
  ND_NUM,         // 0 integer number
  ND_STR,         // 1 string literal
  ND_LVAR,        // 2 local variable
  ND_GVAR,        // 3 global variable
  ND_DEREF,       // 4
  ND_ADDR,        // 5
  ND_MEMBER,      // 6 . (struct member access)
  ND_EQ,          // 7 ==
  ND_NE,          // 8 !=
  ND_LT,          // 9 <
  ND_LE,          // 10 <=
  ND_BLOCK,       // 11 {}
  ND_IF,          // 12 if
  ND_WHILE,       // 13 while
  ND_FOR,         // 14 for
  ND_ADD,         // 15 +
  ND_SUB,         // 16 -
  ND_MUL,         // 17 *
  ND_DIV,         // 18 /
  ND_MOD,         // 19 %
  ND_SHL,         // 20 >>
  ND_SAR,         // 21 <<
  ND_NOT,         // 22 !
  ND_ASSIGN,      // 23 =
  ND_ASSIGN_POST, // 24 for i++ and i--
  ND_RETURN,      // 25 return
  ND_CONTINUE,    // 26 continue
  ND_BREAK,       // 27 break
  ND_ENUM,        // 28 enum
  ND_STRUCT,      // 29 struct
  ND_UNION,       // 30 union
  ND_FUNCTION,    // 31
  ND_CALL,        // 32
  ND_BITAND,      // 34 &
  ND_BITXOR,      // 35 ^
  ND_BITOR,       // 36 |
  ND_BITNOT,      // 37 ~
  ND_AND,         // 38 &&
  ND_OR,          // 39 ||
  ND_CAST,        // 40 
  ND_TERNARY,     // 41 ?:
  ND_SWITCH,      // 42 switch
  ND_DO,          // 43 do
  ND_LABEL,       // 44
  ND_PTR_ADD,
  ND_PTR_SUB,
  ND_PTR_DIFF,

  ND_COMMENT,

} NodeKind;

typedef enum {
  TK_RESERVED,  // simbol +, -
  TK_NUM,       // integer number
  TK_CHAR,      // 1 character literal
  TK_STR,       // 1 string literal
  TK_IDENT,     // identity
  TK_EOF,       // End of File
} TokenKind;

typedef enum {
  TY_VOID,
  TY_PRM,
  TY_PTR,
  TY_ARRAY,
  TY_FUNCTION,
  TY_ENUM,
  TY_UNION,
  TY_STRUCT,
  TY_TYPEDEF,
} TypeKind;

typedef struct Type Type;
typedef struct Token Token;
typedef struct Node Node;
typedef struct Function Function;
typedef struct Enum Enum;
typedef struct Member Member;
typedef struct Var Var;
typedef struct InitVal InitVal;

struct Type {

  TypeKind kind;

  char *name;
  int len;  // length of string
  int size; // size of type

  int is_unsigned;
  // ptr or array. *char = * -> char,
  // fp: return type
  Type *to;
  
  Type *ret;

  Type *def;

  Vector *members;
};

struct Token {
  TokenKind kind;
  Token *next;
  int val;    // integer value if kind == TK_NUM
  char *str;  // string of token
  int len;    // length of token
};

struct Node {
  NodeKind kind;

  Node *lhs;    // left-hand side
  Node *rhs;    // right-hand side

  // if, ?:
  Node *ini;
  Node *cnd;    // condition
  Node *stp;

  Node *thn;    // then
  Node *els;    // else

  char *ident;
  int len;

  Vector *list;
  long val;      // for ND_NUM or ND_CALL(extern)
  Type *type;   // for lvar
  long offset;   // for lvar
};

struct Var {
  Var *next;
  Type *type;
  char *name;
  int len;

  int offset; // local variable

  int extn;
  int is_static;

  InitVal *init;  // <InitVal>
};

struct InitVal {
  int n;

  char *str;
  int strlen;

  char *ident;
  int len;

  InitVal *next;
};

struct Enum {
  char *tag; // for debug

  char *name;
  int len;  // length of string

  int val;
};

struct Member {
  char *name;
  int len;
  Type *type;
  int offset;
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
extern Var *locals;
extern Var *globals;

extern Type *bool_type;
extern Type *char_type;
extern Type *short_type;
extern Type *int_type;
extern Type *long_type;
extern Type *float_type;
extern Type *double_type;

extern Type *str_type;

int type_is_array(Type *type);
int type_is_ptr(Type *type);
int sizeof_node(Node* node);
Type *raw_type(Type *type);

char *substring(char *str, int len);
char *line(char *p0);

char *read_file(char *path);

Var *find_var(Token *tok);
Var *find_lvar(Token *tok);

Token *tokenize();
void program();
void codegen();

void message_at(char *loc, char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
void error(char *fmt, ...);

#endif
