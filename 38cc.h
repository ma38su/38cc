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
  ND_NONE,        // 14 return
  ND_FOR,         // 15 for
  ND_ADD,         // 16 +
  ND_SUB,         // 17 -
  ND_MUL,         // 18 *
  ND_DIV,         // 19 /
  ND_MOD,         // 20 %
  ND_SHL,         // 21 >>
  ND_SAR,         // 22 <<
  ND_NOT,         // 23 !
  ND_ASSIGN,      // 24 =
  ND_ASSIGN_POST, // 25 for i++ and i--
  ND_RETURN,      // 26 return
  ND_CONTINUE,    // 27 continue
  ND_BREAK,       // 28 break
  ND_ENUM,        // 29 enum
  ND_STRUCT,      // 30 struct
  ND_UNION,       // 31 union
  ND_FUNCTION,    // 32
  ND_CALL,        // 33
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

} NodeKind;

typedef enum {
  TK_RESERVED,  // simbol +, -
  TK_NUM,       // integer number
  TK_CHAR,      // 1 character literal
  TK_STR,       // 1 string literal
  TK_IDENT,     // identity
  TK_EOF,       // End of File
} TokenKind;

typedef struct Type Type;
typedef struct Token Token;
typedef struct Node Node;
typedef struct LVar LVar;
typedef struct GVar GVar;
typedef struct Function Function;
typedef struct Enum Enum;
typedef struct Member Member;

struct Type {
  enum {
    TY_VOID,
    TY_PRM,
    TY_PTR,
    TY_ARRAY,
    TY_FUNCTION,
    TY_STRUCT,
    TY_TYPEDEF,
  } kind;

  char *name;
  int len;  // length of string
  int size; // size of type

  // ptr or array. *char = * -> char,
  // fp: return type
  Type *to;

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
  Node *cnd;    // condition
  Node *thn;    // then
  Node *els;    // else

  char *ident;
  int len;

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

  int level; // scope level
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
extern LVar *locals;
extern GVar *globals;

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

char *substring(char *str, int len);
char *line(char *p0);

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
