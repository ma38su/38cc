#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "38cc.h"
#include "vector.h"

int sizeof_type(Type *type);
int sizeof_lvars();
int gvar_has_circular();
int align(int offset, int size);
int eval_node(Node* node);
bool is_alpbar(char c);

Type *consume_type();
Member *consume_member();
Var *find_gvar(Token *tok);
Var *find_gstr_or_gen(Token *tok);

Type *find_type(Token *tok);

Type *find_struct_type_or_gen(Token *tok);
Type *find_struct_type(Token *tok);
Member *find_member(Vector *members, Token *tok);

Type *find_union_type_or_gen(Token *tok);
Type *find_union_type(Token *tok);

Enum *find_enum(Token *tok);
Type *find_enum_type(Token *tok);
Type *find_enum_type_or_gen(Token *tok);

Node *new_node(NodeKind kind);
Node *stmt();
Node *stmt_or_block();
Node *expr();
Node *equality();
Node *mul();
Node *shift();
Node *unary();
Node *ternay();
int expect_number();

// current token
Token *token;

char *user_input;
Node *code[1000];
Var *locals;
Var *globals;

Vector *enums;
Vector *types;  // Type*
Vector *inlines;

Type *bool_type;
Type *char_type;
Type *short_type;
Type *int_type;
Type *long_type;
Type *float_type;
Type *double_type;

Type *unsigned_char_type;
Type *unsigned_short_type;
Type *unsigned_int_type;
Type *unsigned_long_type;

Type *void_type;
Type *ptr_char_type;

int gstr_len = 0; // number of global string variable

Type *new_type(char* name, int size) {
  Type *type;
  type = calloc(1, sizeof(Type));
  type->name = name;
  type->len = strlen(name);
  type->size = size;
  return type;
}

Type *new_prm_type(char* name, int size) {
  Type *type;
  type = calloc(1, sizeof(Type));
  type->kind = TY_PRM;
  type->name = name;
  type->len = strlen(name);
  type->size = size;
  return type;
}

Type *new_ptr_type(Type* type) {
  Type *ptr_type = new_type("*", 8);
  ptr_type->kind = TY_PTR;
  ptr_type->to = type;
  ptr_type->is_unsigned = 1;
  return ptr_type;
}

Type *new_array_type(Type* type, int len) {
  Type *ary_type = new_type("[]", sizeof_type(type) * len);
  ary_type->kind = TY_ARRAY;
  ary_type->to = type;
  ary_type->is_unsigned = 1;
  return ary_type;
}

Type *new_fn_type(Type* ret_type) {
  Type *fn_type = new_type("(*)()", 0);
  fn_type->kind = TY_FUNCTION;
  fn_type->ret = ret_type;
  return fn_type;
}

int is_pre_type(Token* tok) {
  return (tok->len == 4 && memcmp(tok->str, "auto", 4) == 0)
      || (tok->len == 6 && memcmp(tok->str, "static", 6) == 0)
      || (tok->len == 8 && memcmp(tok->str, "volatile", 8) == 0)
      || (tok->len == 6 && memcmp(tok->str, "signed", 6) == 0)
      || (tok->len == 8 && memcmp(tok->str, "unsigned", 8) == 0)
      || (tok->len == 4 && memcmp(tok->str, "long", 4) == 0)
      || (tok->len == 5 && memcmp(tok->str, "short", 5) == 0);
}

Var *new_var(Token *tok, Type *type) {
  Var *var = calloc(1, sizeof(Var));
  var->name = tok->str;
  var->len = tok->len;
  var->type = type;
  return var;
}

Var *new_lvar(Token *tok, Type *type) {
  Var *lvar = new_var(tok, type);

  int size = sizeof_type(type);
  if (locals) {
    lvar->offset = locals->offset + size;
  } else {
    lvar->offset = size;
  }

  lvar->next = locals;
  locals = lvar;
  return lvar;
}

Var *new_gvar(Token *tok, Type* type) {
  Var *gvar = new_var(tok, type);
  gvar->next = globals;
  globals = gvar;
  if (gvar_has_circular()) error("gvar circuit");
  return gvar;
}

Var *new_function_var(Token *tok, Type* type) {
  return new_gvar(tok, new_fn_type(type));
}

void add_gvar_function(char *name, Type *ret_type) {
  Token *tok = calloc(1, sizeof(Token));
  tok->str = name;
  tok->len = strlen(name);
  Var *func = new_function_var(tok, ret_type);
  func->is_extern = true;
}

void init_builtin() {
  add_gvar_function("__builtin_va_start", void_type);
  add_gvar_function("__builtin_bswap64", long_type); // unsigned long
  add_gvar_function("__builtin_bswap32", int_type); // unsigned long
}

void init_types() {
  enums = new_vector();
  types = new_vector();
  inlines = new_vector();
  
  bool_type = new_prm_type("_Bool", 1);
  char_type = new_prm_type("char", sizeof(char));
  short_type = new_prm_type("short", sizeof(short));
  int_type = new_prm_type("int", sizeof(int));
  long_type = new_prm_type("long", sizeof(long));
  float_type = new_prm_type("float", sizeof(float));
  double_type = new_prm_type("double", sizeof(double));

  unsigned_char_type = new_prm_type("unsigned char", sizeof(unsigned char));
  unsigned_char_type->is_unsigned = 1;

  unsigned_short_type = new_prm_type("unsigned short", sizeof(unsigned short));
  unsigned_short_type->is_unsigned = 1;

  unsigned_int_type = new_prm_type("unsigned int", sizeof(unsigned int));
  unsigned_int_type->is_unsigned = 1;

  unsigned_long_type = new_prm_type("unsigned long", sizeof(unsigned long));
  unsigned_long_type->is_unsigned = 1;

  ptr_char_type = new_ptr_type(char_type);

  Type *long_int_type = new_prm_type("long int", sizeof(long int));
  vec_add(types, long_int_type);

  Type *short_int_type = new_prm_type("short int", sizeof(short int));
  vec_add(types, short_int_type);

  Type *long_long_int_type = new_prm_type("long long int", sizeof(long long int));
  vec_add(types, long_long_int_type);

  Type *unsigned_long_int_type = new_prm_type("unsigned long int", sizeof(unsigned long int));
  unsigned_long_int_type->is_unsigned = 1;
  vec_add(types, unsigned_long_int_type);

  Type *long_unsigned_int_type = new_prm_type("long unsigned int", sizeof(long unsigned int));
  long_unsigned_int_type->is_unsigned = 1;
  vec_add(types, long_unsigned_int_type);
  
  Type *unsigned_short_int_type = new_prm_type("unsigned short int", sizeof(unsigned short int));
  unsigned_short_int_type->is_unsigned = 1;
  vec_add(types, unsigned_short_int_type);

  Type *long_double_type = new_prm_type("long double", sizeof(long double));
  vec_add(types, long_double_type);

  Type *unsigned_long_long_int_type = new_prm_type("unsigned long long int", sizeof(unsigned long long int));
  vec_add(types, unsigned_long_long_int_type);

  vec_add(types, bool_type);
  vec_add(types, char_type);
  vec_add(types, short_type);
  vec_add(types, int_type);
  vec_add(types, long_type);
  vec_add(types, float_type);
  vec_add(types, double_type);

  vec_add(types, unsigned_char_type);
  vec_add(types, unsigned_short_type);
  vec_add(types, unsigned_int_type);
  vec_add(types, unsigned_long_type);

  void_type = new_type("void", sizeof(void));
  void_type->kind = TY_VOID;
  vec_add(types, void_type);

  Type *builtin_va_list_type = new_type("__builtin_va_list", sizeof(__builtin_va_list));
  builtin_va_list_type->kind = TY_VOID;
  vec_add(types, builtin_va_list_type);
}

int type_is_ptr_or_array(Type *type) {
  return type_is_ptr(type) || type_is_array(type);
}

int type_is_array(Type *type) {
  type = raw_type(type);
  return type->len == 2 && memcmp(type->name, "[]", 2) == 0;
}

int type_is_ptr(Type *type) {
  type = raw_type(type);
  return type->len == 1 && memcmp(type->name, "*", 1) == 0;
}

bool type_is_func(Type *type) {
  type = raw_type(type);
  return type->ret;
}

Node *new_node(NodeKind kind) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  return node;
}

Node *new_node_deref(Node *lhs) {
  if (!lhs) {
    error("no node at new_node_deref");
  }
  Type *type = raw_type(lhs->type);
  if (!type) {
    error("no type at new_node_deref %d", lhs->kind);
  }
  if (!type->to) {
    error("type is not ptr at new_node_deref");
  }
  Node *node = new_node(ND_DEREF);
  node->lhs = lhs;
  node->type = type->to;
  return node;
}

Type *to_type(Type *lhs_type, Type *rhs_type) {
  if (type_is_func(lhs_type)) {
    lhs_type = lhs_type->ret;
  }
  if (type_is_func(rhs_type)) {
    rhs_type = rhs_type->ret;
  }

  int lhs_ptr = type_is_ptr_or_array(lhs_type);
  int rhs_ptr = type_is_ptr_or_array(rhs_type);
  if (lhs_ptr && rhs_ptr) {
    return int_type;
  } else if (lhs_ptr) {
    return lhs_type; 
  } else if (rhs_ptr) {
    return rhs_type;
  }

  if (lhs_type == rhs_type) {
    return lhs_type;
  }

  if (lhs_type->size < rhs_type->size) {
    return rhs_type;
  } else if (lhs_type->size > rhs_type->size) {
    return lhs_type;
  } else if (lhs_type->is_unsigned) {
    return lhs_type;
  } else if (rhs_type->is_unsigned) {
    return rhs_type;
  } else {
    return lhs_type;
  }
  
  error("parser assertion error");
  return NULL;
}

Node *new_node_lr(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = new_node(kind);
  node->kind = kind;
  node->lhs = lhs;
  node->rhs = rhs;

  if (kind == ND_EQ
      || kind == ND_NE
      || kind == ND_LT
      || kind == ND_LE) {
    node->type = int_type;
  } else if (kind == ND_ASSIGN || kind == ND_ASSIGN_POST) {
    node->type = lhs->type;
  } else {
    if (!lhs->type) {
      error("lhs no type: %d -> %d", node->kind, lhs->kind);
    }
    if (!rhs->type) {
      error("rhs no type: %d -> %d", node->kind, rhs->kind);
    }
    node->type = to_type(lhs->type, rhs->type);
  }

  if (!node->type) {
    error("not defined: kind: %d, lhs: %d, rhs: %d, type: %s",
       kind, lhs->kind, rhs->kind, lhs->type->name);
  }
  return node;
}

Node *new_int_node(int val) {
  Node *node = new_node(ND_NUM);
  node->val = val;
  node->type = int_type;
  return node;
}

Node *new_long_node(int val) {
  Node *node = new_node(ND_NUM);
  node->val = val;
  node->type = long_type;
  return node;
}

bool token_is_reserved(Token *tok, char *op) {
  return tok && tok->kind == TK_RESERVED && strlen(op) == tok->len &&
      memcmp(op, tok->str, tok->len) == 0;
}

// for only reserved
bool consume(char *op) {
  if (!token_is_reserved(token, op)) {
    return false;
  }
  token = token->next;
  return true;
}

bool peek(char *op) {
  return token_is_reserved(token, op);
}

// read reserved symbol
void expect(char *op) {
  if (!consume(op)) {
    error_at(token->str, "token is '%s' not '%s'. tk-kind: %d",
        substring(token->str, token->len), op, token->kind);
  }
}

int distance(Token *tok1, Token *tok2) {
  int len = (int) (tok2->str - tok1->str) + (tok2->len);
  int len2 = tok1->len + 1 + tok2->len;
  if (len != len2) {
    return 0;
  }
  return len;
}

Token *consume_ident() {
  if (token->kind != TK_IDENT) {
    return NULL;
  }
  Token *tmp = token;
  token = token->next;
  return tmp;
}

Type *consume_type() {
  // skip const
  consume("const");

  Type *type;
  if (consume("struct")) {
    if (token->kind != TK_IDENT) return NULL;

    type = find_struct_type(token);
    if (!type) return NULL;
    token = token->next;
  } else if (consume("union")) {
    if (token->kind != TK_IDENT) return NULL;

    type = find_union_type(token);
    if (!type) return NULL;
    token = token->next;
  } else if (consume("enum")) {
    if (token->kind != TK_IDENT) return NULL;

    type = find_enum_type(token);
    if (!type) return NULL;
    token = token->next;
  } else {
    if (token->kind != TK_IDENT) return NULL;

    Token *p0 = token;

    type = find_type(token);
    if (!type && !is_pre_type(token)) return NULL;

    while (token->next) {
      if (!is_alpbar(*token->next->str)) {
        token = token->next;
        break;
      }

      int len = token->len;
      int new_len = distance(token, token->next);
      token->len = new_len;

      Type *type1 = find_type(token);
      if (type1) {
        //fprintf(stderr, "found %s\n", substring(token->str, token->len));
        type = type1;
        token->next = token->next->next;
      } else if (is_pre_type(token->next)) {
        type = NULL;
        token->next = token->next->next;
      } else {
        //fprintf(stderr, "break: %s\n", substring(token->str, token->len));
        token->len = len; // rollback
        token = token->next;
        break;
      }
    }
    if (p0 == token) return NULL;
    if (!type) error_at(token->str, "type is not set(null) %s", substring(token->str, token->len));
  }

  while (consume("*")) {
    type = new_ptr_type(type);
  }
  return type;
}

Token *consume_fp() {
  if (!consume("(")) return NULL;
  
  expect("*");
  Token *tok = token;
  if (tok->kind != TK_IDENT)
    error_at(token->str, "illegal function pointer");

  token = token->next;
  expect(")");
  return tok;
}

Node *consume_char_node() {
  if (token->kind != TK_CHAR) {
    return NULL;
  }
  Node *node = new_node(ND_NUM);
  node->type = char_type;
  node->val = token->val;
  token = token->next;
  return node;
}

Token *consume_string() {
  if (token->kind != TK_STR) {
    return NULL;
  }
  Token *cur = token;
  token = token->next;
  return cur;
}

Node *consume_string_node() {
  if (token->kind != TK_STR) {
    return NULL;
  }

  Var *gvar = find_gstr_or_gen(token);
  token = token->next;

  Node *node = new_node(ND_GVAR);
  node->type = gvar->type;
  node->ident = gvar->name;
  node->len = gvar->len;
  return node;
}

Node *consume_enum_node() {
  if (!consume("enum")) return NULL;

  Node *node = new_node(ND_ENUM);
  node->type = int_type;
  Token *tag = consume_ident();

  int enum_id = 0;
  expect("{");
  while (!consume("}")) {
    Token *tok = consume_ident();

    Enum *evar = find_enum(tok);
    if (evar) {
      error_at(tok->str, "duplicated defined gvar");
    }
    evar = calloc(1, sizeof(Enum));
    evar->name = substring(tok->str, tok->len);
    evar->len = tok->len;

    if (consume("=")) {
      evar->val = eval_node(expr());
      enum_id = evar->val + 1;
    } else {
      evar->val = enum_id++;
    }
    vec_add(enums, evar);

    if (!consume(",")) {
      expect("}");
      break;
    }
  }

  if (tag) {
    node->ident = substring(tag->str, tag->len);
    node->len = tag->len;
    if (find_enum_type(tag)) error("override enum type");
    node->type = find_enum_type_or_gen(tag);
  } else {
    node->type = int_type;
  }

  return node;
}

int align(int offset, int size) {
  if ((offset & (size - 1)) == 0)
    return offset;
  return ((offset / size) + 1) * size;
}

int fixed_members_offset(Vector *members) {
  int unit = 1;
  int offset = 0;
  for (int i = 0; i < members->size; ++i) {
    Member *mem = vec_get(members, i);
    int size_mem = sizeof_type(mem->type);
    if (unit < size_mem) unit = size_mem;
    mem->offset = offset = align(offset, size_mem);
    offset += size_mem;
  }
  offset = align(offset, unit);
  return offset;
}

Node *consume_struct_node() {
  if (!consume("struct")) return NULL;

  Node *node = new_node(ND_STRUCT);
  Token *tag = consume_ident();

  if (!consume("{")) {
    if (!tag) error_at(token->str, "illegal struct");

    node->type = find_struct_type_or_gen(tag);
    return node;
  }

  Vector *members = new_vector();
  while (!consume("}")) {
    Member *member = consume_member();
    vec_add(members, member);
  }

  int size_struct = fixed_members_offset(members);
  if (tag) {
    node->type = find_struct_type_or_gen(tag);
    if (node->type->size != 0) error("override struct type");
    node->type->size = size_struct;
  } else {
    node->type = new_type("_", size_struct); // no tag
    node->type->kind = TY_STRUCT;
    // 検索できないので追加もしない
  }
  node->type->members = members;
  return node;
}

Node *consume_union_node() {
  if (!consume("union")) return NULL;

  Node *node = new_node(ND_UNION);
  Token *tag = consume_ident();

  if (!consume("{")) {
    if (!tag) error_at(token->str, "illegal union");

    node->type = find_union_type_or_gen(tag);
    return node;
  }

  Vector *members = new_vector();
  while (!consume("}")) {
    Member *member = consume_member();
    vec_add(members, member);
  }

  int size_union = 0;
  for (int i = 0; i < members->size; ++i) {
    Member *m = vec_get(members, i);
    int size = sizeof_type(m->type);
    if (size_union < size) size_union = size;
  }

  if (tag) {
    node->type = find_union_type_or_gen(tag);
    if (node->type->size != 0) error("override union type");
    node->type->size = size_union;
  } else {
    node->type = new_type("_", size_union); // TODO no tag
    node->type->kind = TY_UNION;
  }
  node->type->members = members;
  return node;
}

Type *raw_type(Type *type) {
  while (type->kind == TY_TYPEDEF || type->kind == TY_ENUM) {
    type = type->def;
  }
  return type;
}

int sizeof_type(Type *type) {
  return raw_type(type)->size;
}

Node *consume_member_node() {
  Node *node;
  if (peek("enum")) {
    node = consume_enum_node();
    Token *ident = consume_ident();
    if (ident) {
      node->ident = ident->str;
      node->len = ident->len;
    }
    node->type = int_type;
  } else if (peek("union")) {
    node = consume_union_node();
    Token *ident = consume_ident();
    if (ident) {
      node->ident = ident->str;
      node->len = ident->len;
    }
  } else if (peek("struct")) {
    node = consume_struct_node();
    while (consume("*")) {
      node->type = new_ptr_type(node->type);
    }
    Token *ident = consume_ident();
    if (ident) {
      node->ident = ident->str;
      node->len = ident->len;
    }
  } else {
    Type *type = consume_type();
    Token *member = consume_ident();
    node = new_node(ND_LVAR);
    node->ident = member->str;
    node->len = member->len;
    node->type = type;
  }

  while (consume("[")) {
    int array_len = eval_node(equality());
    expect("]");
    node->type = new_array_type(node->type, array_len);
  }
  expect(";");
  return node;
}

Member *to_member(Node *node) {
  if (!node)
    error_at(token->str, "no member");
  if (!node->type)
    error_at(token->str, "no member type");

  Member *member = calloc(1, sizeof(Member));
  member->name = node->ident;
  member->len = node->len;
  member->type = node->type;

  return member;
}

Member *consume_member() {
  return to_member(consume_member_node());
}

// read reserved integer number
int expect_number() {
  if (token->kind != TK_NUM) {
    error_at(token->str, "token is not number. (tk-kind: %d)", token->kind);
  }
  int val = token->val;
  token = token->next;
  return val;
}

bool at_eof() {
  return token->kind == TK_EOF;
}

char *substring(char *str, int len) {
  char *sub = calloc(len + 1, sizeof(char));
  strncpy(sub, str, len);
  return sub;
}

Vector *consume_args() {
  Vector *args = NULL;
  if (!consume(")")) {
    args = new_vector();
    for (;;) {
      if (consume("...")) {
        expect(")");
        break;
      }

      vec_add(args, expr());
      if (consume(")")) {
        break;
      }
      expect(",");
    }
  }
  return args;
}

int consume_void_args() {
  if (consume(")")) {
    return 1;
  }
  if (token->kind == TK_IDENT
      && token->len == 4 && memcmp(token->str, "void", 4) == 0
      && token_is_reserved(token->next, ")")) {
    token = token->next->next;
    return 1;
  }
  return 0;
}

Vector *expect_defined_args() {
  if (consume_void_args()) {
    return NULL;
  }

  Vector *args = new_vector();
  for (;;) {
    if (consume("...")) {
      if (consume(")")) break;
      expect(",");
      continue;
    }

    //consume("const");
    Type *type = consume_type();
    if (!type) {
      error_at(token->str,
          "illegal arg type (size: %d)", args->size);
    }

    // skip const
    consume("const");
    consume("*");

    Token *tok = NULL;
    // some arg var names are ommited.
    if (token->kind == TK_IDENT) {
      tok = consume_ident();
      if (consume("[")) {
        if (consume("]")) {
          type = new_ptr_type(type);
        } else {
          int array_len = expect_number();
          expect("]");
          type = new_array_type(type, array_len);
        }
      }
    } else if (peek("(")) {
      tok = consume_fp();
      expect("(");
      while (!consume(")")) {
        token = token->next;
      }
      type = new_type("fn", 8);
      type->kind = TY_PTR;
      type->is_unsigned = 1;
    }

    Node *node = new_node(ND_LVAR);
    node->type = type;

    if (tok) {
      Var *lvar = new_lvar(tok, type);
      node->ident = substring(tok->str, tok->len);
      node->len = tok->len;
      node->offset = lvar->offset;
    } else {
      node->ident = "";
      node->len = 0;
    }

    vec_add(args, node);

    if (consume(")")) {
      break;
    }
    expect(",");
  }
  return args;
}

Node *consume_sizeof_node() {
  if (!consume("sizeof")) {
    return NULL;
  }

  expect("(");
  Type *type = consume_type();
  if (type) {
    expect(")");
    return new_long_node(sizeof_type(type));
  }

  Node *node = equality();
  expect(")");
  return new_long_node(sizeof_node(node));
}

Type *get_type(Node *node) {
  if (node->kind == ND_NUM
      || node->kind == ND_ADDR
      || node->kind == ND_DEREF
      || node->kind == ND_STRUCT
      || node->kind == ND_LVAR) {
    Type *type = node->type;
    return type;
  }
  return get_type(node->lhs);
}

int sizeof_node(Node* node) {
  return sizeof_type(get_type(node));
}

InitVal *gvar_init_val(Type *type) {
  type = raw_type(type);
  if (type->kind == TY_PTR) {
    Type *type_to = raw_type(type->to);
    if (type_to != char_type) {
      error("unsupported init. (1)");
    }

    // char*
    if (token->kind != TK_STR) {
      error("unsupported init.");
    }
    Var *var = find_gstr_or_gen(token);
    token = token->next;
    InitVal *v = calloc(1, sizeof(InitVal));
    v->ident = var->name;
    v->len = var->len;
    return v;
  }
  if (type->kind == TY_ARRAY) {
    Type *type_to = raw_type(type->to);
    if (type_to == char_type) {
      // char*[]
      if (token->kind != TK_STR) {
        error("TODO unsupported init. (2)");
      }
      InitVal *v = calloc(1, sizeof(InitVal));
      Token *tok = token;
      token = token->next;
      v->str = tok->str;
      v->strlen = tok->len;
      return v;
    }
    if (!consume("{")) {
      error("illegal expression.");
    }

    if (consume("}")) error("TODO !!");

    if (type_to->kind == TY_PTR) {
      InitVal head;
      head.next = NULL;
      InitVal *cur = &head;
      while (1) {
        InitVal *v = calloc(1, sizeof(InitVal));
        if (token->kind == TK_NUM) error("illegal token");

        Var *var;
        if (token->kind == TK_STR) {
          var = find_gstr_or_gen(token);
          token = token->next;
        } else if (token->kind == TK_IDENT) {
          Token *ident = consume_ident();
          var = find_gvar(ident);
        } else {
          error("illegal token");
        }
        v->ident = var->name;
        v->len = var->len;
        cur->next = v;
        cur = v;
        if (consume("}")) break;
        expect(",");
      }
      return head.next;
    } else if (type_to->kind == TY_PRM) {
      InitVal head;
      head.next = NULL;
      InitVal *cur = &head;
      while (1) {
        InitVal *v = calloc(1, sizeof(InitVal));
        if (token->kind != TK_NUM) error("illegal token");
        v->n = expect_number();
        cur->next = v;
        cur = v;
        if (consume("}")) break;
        expect(",");
      }
      return head.next;
    }
  }
  Node *node = expr();

  InitVal *v = calloc(1, sizeof(InitVal));
  v->n = eval_node(node);
  return v;

  error_at(token->str, "unsupported state");
  return NULL;
}

Node *primary() {

  Node *node;
  Token *tok;

  node = consume_sizeof_node();
  if (node) {
    return node;
  }

  if (consume("(")) {
    Type *type = consume_type();
    if (type) error_at(token->str, "illegal cast");

    node = expr();
    expect(")");
    return node;
  }

  node = consume_char_node();
  if (node) {
    return node;
  }

  node = consume_string_node();
  if (node) {
    return node;
  }

  tok = consume_ident();
  if (!tok) {
    return new_int_node(expect_number());
  }

  // function call
  if (consume("(")) {
    Var *gvar = find_gvar(tok);
    if (gvar) {
      node = new_node(ND_CALL);
      node->type = gvar->type->ret;
      node->val = gvar->is_extern;
    } else {
      Var *lvar = find_lvar(tok);
      if (lvar) {
        node = new_node(ND_CALL);
        node->type = lvar->type->ret;
        node->val = 0;
      } else {
        error_at(tok->str, "function is not found");
      }
    }

    if (!node) {
      error_at(tok->str, "function(%s): not found", substring(tok->str, tok->len));
    }

    node->list = consume_args();
    node->ident = substring(tok->str, tok->len);
    node->len = tok->len;

    if (!node->type) {
      error_at(tok->str, "function(%s): not found return type", substring(tok->str, tok->len));
    }
    return node;
  }

  Var *lvar = find_lvar(tok);
  if (lvar) {
    node = new_node(ND_LVAR);
    node->type = lvar->type;

    node->ident = substring(tok->str, tok->len);
    node->len = tok->len;

    node->offset = lvar->offset;
  } else {
    Enum *evar = find_enum(tok);
    if (evar) {
      node = new_node(ND_NUM); // ENUM
      node->type = int_type;
      node->ident = substring(tok->str, tok->len);
      node->len = tok->len;
      node->val = evar->val;
    } else {
      Var *gvar = find_gvar(tok);
      if (!gvar) {
        error_at(tok->str, "undefined ident: %s", substring(tok->str, tok->len));
      }
      node = new_node(ND_GVAR);
      node->type = gvar->type;
      node->ident = substring(tok->str, tok->len);
      node->len = tok->len;
    }
  }

  return node;
}

Member *get_member(Type *type, Token *ident) {
  Type *ty = raw_type(type);
  if (!ty) error_at(token->str, "no type");

  Vector *members = ty->members;
  if (!members)
    error_at(token->str, "no members: %s",
        substring(ty->name, ty->len));

  Member *member = find_member(members, ident);
  if (!member) {
    error_at(ident->str, "no members (type: %s)", substring(ty->name, ty->len));
  }
  return member;
}

Node *new_comment(Node *node, char *comment) {
  Node *n = new_node(ND_COMMENT);
  n->lhs = node;
  n->type = node->type;
  n->ident = comment;
  return n;
}

Node *postfix() {

  Node *node = primary();

  for (;;) {
    // post ++ or --
    if (consume("++")) {
      Type *type = raw_type(node->type);
      if (type->to) {
        node = new_node_lr(ND_ASSIGN_POST, node, new_node_lr(ND_PTR_ADD, node, new_int_node(1)));
      } else {
        node = new_node_lr(ND_ASSIGN_POST, node, new_node_lr(ND_ADD, node, new_int_node(1)));
      }
      continue;
    }
    if (consume("--")) {
      Type *type = raw_type(node->type);
      if (type->to) {
        node = new_node_lr(ND_ASSIGN_POST, node, new_node_lr(ND_PTR_SUB, node, new_int_node(1)));
      } else {
        node = new_node_lr(ND_ASSIGN_POST, node, new_node_lr(ND_SUB, node, new_int_node(1)));
      }
      continue;
    }

    if (consume("[")) {
      if (!type_is_ptr_or_array(node->type)) {
        error_at((token->str - 1), "Not ptr");
      }
      Node *index = expr();
      expect("]");

      node = new_node_deref(new_node_lr(ND_PTR_ADD, node, index));
      continue;
    }

    if (consume(".")) {
      Token *ident = consume_ident();
      if (!ident) error_at(token->str, "no ident");
      Member *member = get_member(node->type, ident);
      int new_offset = node->offset - member->offset;

      node = new_node(ND_LVAR);
      node->ident = substring(ident->str, ident->len);
      node->offset = new_offset;
      node->type = member->type;
      continue;
    }

    if (consume("->")) {
      Token *ident = consume_ident();
      if (!ident) error_at(token->str, "no ident");

      Member *member = get_member(raw_type(node->type)->to, ident);
      Node *addr = new_node_lr(ND_ADD, node, new_int_node(member->offset));
      addr->type = new_ptr_type(member->type);
      node = new_node_deref(addr);
      continue;
    }
    return node;
  }
}

Node *unary() {
  // pre ++
  if (consume("++")) {
    Node *node = primary();
    Type *type = raw_type(node->type);

    int size = type->to ? sizeof_type(type->to) : 1;
    return new_node_lr(ND_ASSIGN, node, new_node_lr(ND_ADD, node, new_long_node(size)));
  }
  // pre -- 
  if (consume("--")) {
    Node *node = primary();
    Type *type = raw_type(node->type);

    int size = type->to ? sizeof_type(type->to) : 1;
    return new_node_lr(ND_ASSIGN, node, new_node_lr(ND_SUB, node, new_long_node(size)));
  }
  if (consume("-")) {
    Node *node = primary();
    return new_node_lr(ND_SUB, new_int_node(0), node);
  }
  if (consume("+")) {
    return primary();
  }
  if (consume("~")) {
    Node *node = new_node(ND_BITNOT);
    node->lhs = unary();
    node->type = int_type;
    return node;
  }
  if (consume("!")) {
    Node *node = new_node(ND_NOT);
    node->lhs = unary();
    node->type = int_type;
    return node;
  }
  if (consume("*")) {
    return new_node_deref(unary());
  }
  if (consume("&")) {
    Node *node = new_node(ND_ADDR);
    node->lhs = unary();
    node->type = new_ptr_type(node->lhs->type);
    return node;
  }
  return postfix();
}

Node *consume_cast_node() {
  Token *tmp = token;
  if (!consume("(")) return NULL;

  Type *type = consume_type();
  if (!type) {
    token = tmp; // rollback
    return NULL;
  } 
  expect(")");

  Node *node = new_node(ND_CAST);
  node->type = type;
  return node;
}

Node *cast() {
  Node *node = consume_cast_node();
  if (node) {
    node->lhs = unary();
    return node;
  }
  return unary();
}

// L <- R
Node *mul() {
  Node *node = cast();
  for (;;) {
    if (consume("*")) {
      node = new_node_lr(ND_MUL, node, cast());
    } else if (consume("/")) {
      node = new_node_lr(ND_DIV, node, cast());
    } else if (consume("%")) {
      node = new_node_lr(ND_MOD, node, cast());
    } else {
      return node;
    }
  }
}

// L -> R
Node *add() {
  Node *node = mul();
  for (;;) {
    if (consume("+")) {
      Node *node2 = mul();

      Type *type = raw_type(node->type);
      Type *type2 = raw_type(node2->type);
      if (type->to) {
        if (type2->to) error("illegal");
        node = new_node_lr(ND_PTR_ADD, node, node2);
      } else if (type2->to) {
        node = new_node_lr(ND_PTR_ADD, node2, node);
      } else {
        node = new_node_lr(ND_ADD, node, node2);
      }
    } else if (consume("-")) {
      Node *node2 = mul();
      Type *type = raw_type(node->type);
      Type *type2 = raw_type(node2->type);
      if (type->to && type2->to) {
        fprintf(stderr, "DEBUG ptr-diff\n");
        node = new_node_lr(ND_PTR_DIFF, node, node2);
      } else if (type->to) {
        node = new_node_lr(ND_PTR_SUB, node, node2);
      } else if (type2->to) {
        node = new_node_lr(ND_SUB, new_long_node(0), new_node_lr(ND_PTR_SUB, node2, node));
      } else {
        node = new_node_lr(ND_SUB, node, node2);
      }
    } else {
      return node;
    }
  }
}

// L -> R
Node *shift() {
  Node *node = add();
  for (;;) {
    if (consume("<<")) {
      node = new_node_lr(ND_SHL, node, add());
    } else if (consume(">>")) {
      node = new_node_lr(ND_SAR, node, add());
    } else {
      return node;
    }
  }
}

// L -> R
Node *relational() {
  Node *node = shift();
  for (;;) {
    if (consume("<")) {
      node = new_node_lr(ND_LT, node, shift());
    } else if (consume("<=")) {
      node = new_node_lr(ND_LE, node, shift());
    } else if (consume(">=")) {
      node = new_node_lr(ND_LE, shift(), node);
    } else if (consume(">")) {
      node = new_node_lr(ND_LT, shift(), node);
    } else {
      return node;
    }
  }
}

// L -> R
Node *equality() {
  Node *node = relational();
  for (;;) {
    if (consume("==")) {
      node = new_node_lr(ND_EQ, node, relational());
    } else if (consume("!=")) {
      node = new_node_lr(ND_NE, node, relational());
    } else {
      return node;
    }
  }
}

Node *bitand() {
  Node *node = equality();
  for (;;) {
    char *ident = token->str;
    if (consume("&")) {
      node = new_node_lr(ND_BITAND, node, equality());
      node->ident = ident;
      node->len = 1;
    } else {
      return node;
    }
  }
}

// L -> R
Node *bitxor() {
  Node *node = bitand();
  for (;;) {
    char *ident = token->str;
    if (consume("^")) {
      node = new_node_lr(ND_BITXOR, node, bitand());
      node->ident = ident;
      node->len = 1;
    } else {
      return node;
    }
  }
}

Node *bitor() {
  Node *node = bitxor();
  for (;;) {
    char *ident = token->str;
    if (consume("|")) {
      node = new_node_lr(ND_BITOR, node, bitxor());
      node->ident = ident;
      node->len = 1;
    } else {
      return node;
    }
  }
}

Node *lgand() {
  Node *node = bitor();
  for (;;) {
    char *ident = token->str;
    if (consume("&&")) {
      node = new_node_lr(ND_AND, node, bitor());
      node->ident = ident;
      node->len = 2;
    } else {
      return node;
    }
  }
}

Node *lgor() {
  Node *node = lgand();
  for (;;) {
    if (consume("||")) {
      node = new_node_lr(ND_OR, node, lgand());
    } else {
      return node;
    }
  }
}

Node *ternay() {
  Node *node = lgor();
  if (consume("?")) {
    Node *ternary = new_node(ND_TERNARY);
    ternary->cnd = node;
    ternary->thn = expr();
    expect(":");
    ternary->els = ternay();
    node = ternary;
  }
  return node;
}

// L <- R
Node *assign() {
  Node *node = ternay();
  if (consume("+=")) {
    Type *type = raw_type(node->type);
    if (type->to) {
      node = new_node_lr(ND_ASSIGN, node, new_node_lr(ND_PTR_ADD, node, assign()));
    } else {
      node = new_node_lr(ND_ASSIGN, node, new_node_lr(ND_ADD, node, assign()));
    }
  } else if (consume("-=")) {
    Type *type = raw_type(node->type);
    if (type->to) {
      node = new_node_lr(ND_ASSIGN, node, new_node_lr(ND_PTR_SUB, node, assign()));
    } else {
      node = new_node_lr(ND_ASSIGN, node, new_node_lr(ND_SUB, node, assign()));
    }
  } else if (consume("*=")) {
    node = new_node_lr(ND_ASSIGN, node, new_node_lr(ND_MUL, node, assign()));
  } else if (consume("/=")) {
    node = new_node_lr(ND_ASSIGN, node, new_node_lr(ND_DIV, node, assign()));
  } else if (consume("%=")) {
    node = new_node_lr(ND_ASSIGN, node, new_node_lr(ND_MOD, node, assign()));
  } else if (consume("<<=")) {
    node = new_node_lr(ND_ASSIGN, node, new_node_lr(ND_SHL, node, assign()));
  } else if (consume(">>=")) {
    node = new_node_lr(ND_ASSIGN, node, new_node_lr(ND_SAR, node, assign()));
  } else if (consume("&=")) {
    node = new_node_lr(ND_ASSIGN, node, new_node_lr(ND_BITAND, node, assign()));
  } else if (consume("^=")) {
    node = new_node_lr(ND_ASSIGN, node, new_node_lr(ND_BITXOR, node, assign()));
  } else if (consume("|=")) {
    node = new_node_lr(ND_ASSIGN, node, new_node_lr(ND_BITOR, node, assign()));
  } else if (consume("=")) {
    node = new_node_lr(ND_ASSIGN, node, assign());
  }
  return node;
}

Node *expr(void) {
  return assign();
}


// defined local variable
Node *declaration(void) {
  int is_static = consume("static");

  Type *type = consume_type();
  if (!type) {
    return NULL;
  }

  Token *tok = consume_ident();
  if (!tok) {
    if (type) {
      error_at(token->str, "illegal var name1 %d", type->kind);
    } else {
      error_at(token->str, "illegal var name2");
    }
  }
  if (find_lvar(tok))
    error_at(tok->str, "duplicated defined lvar");

  if (consume("[")) {
    int array_len = expect_number();
    expect("]");

    type = new_array_type(type, array_len);
  }

  if (is_static) {
    Var *var = new_gvar(tok, type);

    Node *node = new_node(ND_GVAR);
    node->ident = substring(var->name, var->len);
    node->type = var->type;
    if (consume("=")) {
      var->init = gvar_init_val(var->type);
    }
    return node;
  } else {
    Var *var = new_lvar(tok, type);
    Node *node = new_node(ND_LVAR);
    node->ident = substring(var->name, var->len);
    node->offset = var->offset;
    node->type = var->type;
    if (consume("=")) {
      node = new_node_lr(ND_ASSIGN, node, assign());
    }
    return node;
  }
}

Node *block_stmt() {
  if (!consume("{")) return NULL;

  Node *node = new_node(ND_BLOCK);
  Vector *stmt_list = new_vector();
  while (!consume("}")) {
    if (at_eof()) error("block_stmt: no more token");
    vec_add(stmt_list, stmt());
  }
  node->list = stmt_list;
  return node;
}

Node *stmt() {
  // scope in
  Var *tmp_locals = locals;

  Node *node = block_stmt();
  if (node) {
    // scope out
    locals = tmp_locals;
    return node;
  }
  if (consume("return")) {
    node = new_node(ND_RETURN);
    if (consume(";")) return node;
    node->lhs = expr();
    expect(";");
    return node;
  }
  
  if (consume("continue")) {
    node = new_node(ND_CONTINUE);
    expect(";");
    return node;
  }
  if (consume("break")) {
    node = new_node(ND_BREAK);
    expect(";");
    return node;
  }

  if (consume("if")) {
    node = new_node(ND_IF);
    expect("(");
    node->cnd = expr();
    expect(")");
    node->thn = stmt();
    if (consume("else")) {
      node->els = stmt();
    }
    return node;
  }

  if (consume("do")) {
    node = new_node(ND_DO);
    node->thn = block_stmt();
    expect("while");
    expect("(");
    node->cnd = expr();
    expect(")");
    expect(";");
    return node;
  }
  if (consume("while")) {
    node = new_node(ND_WHILE);
    expect("(");
    node->cnd = expr();
    expect(")");
    node->thn = stmt();
    return node;
  }
  
  if (consume("for")) {
    node = new_node(ND_FOR);
    expect("(");

    if (!consume(";")) {
      Node *d = declaration();
      if (d) {
        node->ini = d;
      } else {
        node->ini = expr();
      }
      expect(";");
    }

    if (consume(";")) {
      node->cnd = new_int_node(1);
    } else {
      node->cnd = expr();
      expect(";");
    }
    if (!consume(")")) {
      node->stp = expr();
      expect(")");
      node->thn = stmt();
    } else {
      node->thn = stmt();
    }

    // scope out
    locals = tmp_locals;
    return node;
  }

  node = declaration();
  if (!node) {
    node = expr();
  }
  expect(";");
  return node;
}

int sizeof_args(Vector *args) {
  if (!args) {
    return 0;
  }
  int size = 0;
  for (int i = 0; i < args->size; ++i) {
    Node *node = vec_get(args, i);
    size += sizeof_type(node->type);
  }
  return size;
}

// pre calculation
int eval_node(Node* node) {
  if (node->kind == ND_NUM) {
    return node->val;
  }
  if (node->kind == ND_TERNARY) {
    return eval_node(node->cnd) ? eval_node(node->thn) : eval_node(node->els);
  }

  int lhs_val = eval_node(node->lhs);

  if (node->kind == ND_CAST) {
    if (node->type == bool_type) {
      return lhs_val ? 1 : 0;
    }
    if (node->type == char_type) {
      return (char) lhs_val;
    }
    if (node->type == short_type) {
      return (char) lhs_val;
    }
    if (node->type == int_type) {
      return (int) lhs_val;
    }
    if (node->type == float_type || node->type == double_type) {
      // TODO cast operation, ex. float, double
      error("unsupported float/double cast");
    }
    return lhs_val;
  }

  if (node->kind == ND_NOT) {
    return !lhs_val;
  }
  if (node->kind == ND_BITNOT) {
    return ~lhs_val;
  }

  if (!node->rhs) {
    if (node->ident) {
      error("no rhs: %s", node->ident);
    } else {
      error("no rhs");
    }
  }

  if (node->kind == ND_AND) {
    return lhs_val && eval_node(node->rhs);
  }
  if (node->kind == ND_OR) {
    return lhs_val || eval_node(node->rhs);
  }

  int rhs_val = eval_node(node->rhs);
  if (node->kind == ND_EQ) {
    return lhs_val == rhs_val;
  }
  if (node->kind == ND_NE) {
    return lhs_val != rhs_val;
  }
  if (node->kind == ND_LT) {
    return lhs_val < rhs_val;
  }
  if (node->kind == ND_LE) {
    return lhs_val <= rhs_val;
  }
  if (node->kind == ND_ADD) {
    return lhs_val + rhs_val;
  }
  if (node->kind == ND_SUB) {
    return lhs_val - rhs_val;
  }
  if (node->kind == ND_MUL) {
    return lhs_val * rhs_val;
  }
  if (node->kind == ND_DIV) {
    return lhs_val / rhs_val;
  }
  if (node->kind == ND_MOD) {
    return lhs_val % rhs_val;
  }

  if (node->kind == ND_BITAND) {
    return lhs_val & rhs_val;
  }
  if (node->kind == ND_BITOR) {
    return lhs_val | rhs_val;
  }
  if (node->kind == ND_BITXOR) {
    return lhs_val ^ rhs_val;
  }
  if (node->kind == ND_SHL) {
    return lhs_val << rhs_val;
  }
  if (node->kind == ND_SAR) {
    return lhs_val >> rhs_val;
  }

  error_at(token->str, "not supported %d", node->kind);
  return -1;
}

Node *new_gvar_node(Token *tok, Type *t, int is_extern) {
  Var *gvar = find_gvar(tok);
  Type *type = raw_type(t);
  if (gvar) {
    if (!is_extern) {
      if (!gvar->is_extern) {
        error_at(tok->str, "duplicated defined gvar");
      }
      gvar->is_extern = false;
      gvar->is_static = true;
    }
  } else {
    gvar = new_gvar(tok, type);
    gvar->is_extern = is_extern;
    if (!is_extern) {
      gvar->is_static = 1;
    }
  }

  if (consume("[")) {
    if (token->kind == TK_NUM) {
      // gvar[n]
      int array_len = expect_number();
      expect("]");
      type = new_array_type(type, array_len);
    } else {
      // gvar[]
      type = new_array_type(type, sizeof_type(type));
      expect("]");
    }
  }

  gvar->type = type;

  Node *node = new_node(ND_GVAR);
  node->ident = substring(tok->str, tok->len);
  node->len = tok->len;
  node->type = gvar->type;

  if (consume("=")) {
    gvar->init = gvar_init_val(gvar->type);
  } else {
    gvar->init = NULL;
  }
  expect(";");
  return node;
}

void fixed_lvar_offset(Node *node, int frame_size) {
  if (!node) return;
  if (node->kind == ND_LVAR) {
    char *node_ident = substring(node->ident, node->len);
    fprintf(stderr, "%s offset1: %ld\n", node_ident, node->offset);
    node->offset = frame_size - node->offset;
    fprintf(stderr, "%s offset2: %ld\n", node_ident, node->offset);

    Token t;
    t.str = node->ident;
    t.len = node->len;
    Var *var = find_lvar(&t);
    if (var) {
      fprintf(stderr, "offset1: %ld\n", node->offset);
      var->offset = frame_size - var->offset;
      fprintf(stderr, "offset2: %ld\n", node->offset);
    }
  } else if (node->kind == ND_FUNCTION) {
    Vector *list = node->list;
    if (list) {
      for (int i = 0; i < list->size; ++i) {
        Node *n = vec_get(list, i);
        fixed_lvar_offset(n, frame_size);
      }
    }
    fixed_lvar_offset(node->lhs, frame_size);
  } else if (node->kind == ND_BLOCK) {
    Vector *list = node->list;
    for (int i = 0; i < list->size; ++i) {
      Node *n = vec_get(list, i);
      fixed_lvar_offset(n, frame_size);
    }
  } else if (node->kind == ND_IF) {
    fixed_lvar_offset(node->thn, frame_size);
    fixed_lvar_offset(node->els, frame_size);
  } else if (node->kind == ND_DO || node->kind == ND_WHILE) {
    fixed_lvar_offset(node->thn, frame_size);
  } else if (node->kind == ND_FOR) {
    fixed_lvar_offset(node->ini, frame_size);
    fixed_lvar_offset(node->thn, frame_size);
  }
}

Node *global() {

  if (consume("typedef")) {
    if (peek("struct")) {
      Node *node = consume_struct_node();
      Type *type = node->type;
      if (!type || type->kind != TY_STRUCT)
        error_at(token->str, "No defined struct type");

      while (consume("*")) {
        type = new_ptr_type(type);
      }

      Token *ident = consume_ident();
      if (!ident || ident->kind != TK_IDENT)
        error_at(token->str, "Illegal typedef struct");

      int size_t = sizeof_type(type);
      Type *struct_type = new_type(substring(ident->str, ident->len), size_t);
      struct_type->kind = TY_TYPEDEF;
      struct_type->def = type;
      vec_add(types, struct_type);

      expect(";");
      return NULL;
    }
    if (peek("union")) {
      Node *node = consume_union_node();
      Type *type = node->type;
      if (!type || type->kind != TY_UNION)
        error_at(token->str, "No defined union type");

      Token *ident = consume_ident();
      if (!ident || ident->kind != TK_IDENT)
        error_at(token->str, "Illegal typedef union");

      int size_t = sizeof_type(type);
      Type *union_type = new_type(substring(ident->str, ident->len), size_t);
      union_type->kind = TY_TYPEDEF;
      union_type->def = type;
      vec_add(types, union_type);

      expect(";");
      return NULL;
    }
    if (peek("enum")) {
      Node *node = consume_enum_node();
      Type *type = node->type;
      if (!type || (type != int_type && type->kind != TY_ENUM))
        error_at(token->str, "No defined enum type: %s", substring(type->name, type->len));

      Token *ident = consume_ident();
      if (!ident) {
        error_at(token->str, "Illegal typedef enum");
      }
      if (ident->kind == TK_IDENT) {
        Type *enum_type = new_type(substring(ident->str, ident->len), 4);
        enum_type->kind = TY_PRM;
        vec_add(types, enum_type);
      }
      expect(";");
      return NULL;
    }

    Type *type = consume_type();
    if (!type) {
      fprintf(stderr, "NOT FOUND: %s\n", substring(token->str, token->len));
      while (!consume(";")) {
        token = token->next;
      }
      return NULL;
    }

    Token *ident;

    ident = consume_ident();
    if (ident) {
      if (consume("(")) {
        expect_defined_args();
      } else {
        Type *def_type = new_type(substring(ident->str, ident->len), sizeof_type(type));
        def_type->kind = TY_TYPEDEF;
        def_type->def = type;
        vec_add(types, def_type);
      }
      expect(";");
      return NULL;
    }

    ident = consume_fp();
    if (ident) {
      if (consume("(")) {
        expect_defined_args();
      }

      Type *def_type = new_type(substring(ident->str, ident->len), sizeof_type(type));
      def_type->kind = TY_TYPEDEF;
      def_type->def = new_fn_type(type);
      vec_add(types, def_type);

      expect(";");
      return NULL;
    }
    expect(";");
    return NULL;
  }

  int is_extern = consume("extern");
  
  Node *node;
  node = consume_struct_node();
  if (node) {
    if (is_extern) {
      // declared variable
      Type *t = node->type;
      while (consume("*")) {
        t = new_ptr_type(t);
      }
      Token *ident = consume_ident();
      if (!ident) return NULL;
      return new_gvar_node(ident, t, is_extern);
    }
    expect(";");
    return NULL;
  }

  if (consume_enum_node()) {
    expect(";");
    return NULL;
  }
  if (consume_union_node()) {
    expect(";");
    return NULL;
  }


  consume("static");
  int is_inline = consume("__inline") ? 1 : 0;

  //consume("const");
  Type *type = consume_type();
  if (!type) error_at(token->str, "type is not found");

  // skip const
  consume("const");

  Token *tok = consume_ident();
  if (!tok) error_at(token->str, "illegal defined function ident");

  if (consume("(")) {
    // before parse function statement for recursive call
    Var *func = new_function_var(tok, type);
    func->is_extern = true; // declare function

    Node *block = NULL;
    Vector *args;

    Var *tmp_locals = locals;
    locals = NULL;

    args = expect_defined_args();
    block = block_stmt();
    if (!block) {
      while (!consume(";")) {
        if (!token->next) error_at(token->str, "no more token");
        // skip unsupported tokens
        token = token->next;
      }
    }

    int size_lvars = sizeof_lvars();
    node = new_node(ND_FUNCTION);
    node->list = args;
    node->ident = substring(tok->str, tok->len);
    node->len = tok->len;
    node->lhs = block;
    node->offset = size_lvars;
    node->type = func->type->ret;
    
    locals = tmp_locals;

    if (is_inline) {
      vec_add(inlines, node);
      return NULL;
    }
    return node;
  } else {
    return new_gvar_node(tok, type, is_extern);
  }
}



void dump_types() {
  fprintf(stderr, "-- dump types begin --\n");
  for (int i = 0; i < types->size; ++i) {
    Type *type = vec_get(types, i);
    fprintf(stderr, "  find %s\n", substring(type->name, type->len));
  }
  fprintf(stderr, "-- dump types end --\n");
}

void program() {
  init_types();
  init_builtin();
  //dump_types();

  int i = 0;
  while (!at_eof()) {
    Node* n = global();
    if (n) {
      code[i++] = n;
    }
  }
  code[i] = NULL;
}

Type *find_type(Token *tok) {
  for (int i = 0; i < types->size; ++i) {
    Type *type = vec_get(types, i);
    if (type->kind == TY_STRUCT) continue;
    if (type->kind == TY_UNION) continue;
    if (type->kind == TY_ENUM) continue;
    if (type->len == tok->len && memcmp(tok->str, type->name, type->len) == 0) {

      if (type->kind != TY_PRM && type->kind != TY_VOID && type->kind != TY_TYPEDEF) {
        error("illegal init types %d", type->kind);
      }
      return type;
    }
  }
  return NULL;
}

Type *find_enum_type(Token *tok) {
  for (int i = 0; i < types->size; ++i) {
    Type *type = vec_get(types, i);
    if (type->kind != TY_ENUM) continue;
    if (type->len == tok->len && memcmp(tok->str, type->name, type->len) == 0) {
      return type;
    }
  }
  return NULL;
}

Type *find_enum_type_or_gen(Token *tok) {
  Type *type = find_enum_type(tok);
  if (type) return type;

  type = new_type(substring(tok->str, tok->len), int_type->size);
  type->kind = TY_ENUM;
  type->def = int_type;

  vec_add(types, type);
  return type;
}

Type *find_union_type(Token *tok) {
  for (int i = 0; i < types->size; ++i) {
    Type *type = vec_get(types, i);
    if (type->kind != TY_UNION) continue;
    if (type->len == tok->len && memcmp(tok->str, type->name, type->len) == 0) {
      return type;
    }
  }
  return NULL;
}

Type *find_union_type_or_gen(Token *tok) {
  Type *type = find_union_type(tok);
  if (type) return type;

  type = new_type(substring(tok->str, tok->len), 0);
  type->kind = TY_UNION;

  vec_add(types, type);
  return type;
}

Type *find_struct_type(Token *tok) {
  for (int i = 0; i < types->size; ++i) {
    Type *type = vec_get(types, i);
    if (type->kind != TY_STRUCT) continue;
    if (type->len == tok->len && memcmp(tok->str, type->name, type->len) == 0) {
      return type;
    }
  }
  return NULL;
}

Type *find_struct_type_or_gen(Token *tok) {
  Type *type = find_struct_type(tok);
  if (type) return type;

  type = new_type(substring(tok->str, tok->len), 0);
  type->kind = TY_STRUCT;

  vec_add(types, type);
  return type;
}

Var *find_lvar(Token *tok) {
  for (Var *var = locals; var; var = var->next) {
    if (var->len == tok->len && memcmp(tok->str, var->name, var->len) == 0) {
      return var;
    }
  }
  return NULL;
}

Member *find_member(Vector *members, Token *tok) {
  for (int i = 0; i < members->size; ++i) {
    Member *m = vec_get(members, i);
    if (m->len == tok->len && memcmp(tok->str, m->name, m->len) == 0) {
      return m;
    }
  }
  return NULL;
}

void dump_gvar_circular() {
  Vector *vec = new_vector();
  Var *var = globals;
  while (var) {
    fprintf(stderr, "gvar: %s\n", var->name);
    if (vec_contains(vec, var)) {
      return;
    }
    vec_add(vec, var);
    var = var->next;
  }
}

int gvar_has_circular() {
  Vector *vec = new_vector();
  Var *var = globals;
  int i = 0;
  while (var) {
    i++;
    if (vec_contains(vec, var)) {
      fprintf(stderr, "gvar: %d\n", i);
      dump_gvar_circular();
      return 1;
    }
    vec_add(vec, var);
    var = var->next;
  }
  return 0;
}

Var *find_gvar(Token *tok) {
  if (gvar_has_circular()) error("gvar has circuit");
  for (Var *var = globals; var; var = var->next) {
    if (tok->len == var->len && memcmp(tok->str, var->name, var->len) == 0) {
      return var;
    }
  }
  return NULL;
}

Enum *find_enum(Token *tok) {
  for (int i = 0; i < enums->size; ++i) {
    Enum *var = vec_get(enums, i);
    if (var->len == tok->len && memcmp(tok->str, var->name, var->len) == 0) {
      return var;
    }
  }
  return NULL;
}

int to_digit(int n) {
  int digit = 1;
  while (n >= 10) {
    n /= 10;
    digit++;
  }
  return digit;
}

char *gen_gstr_name(int n) {
  int digit = to_digit(n);
  int name_len = 3 + digit + 1;
  char* name = calloc(name_len, sizeof(char));
  sprintf(name, ".LC%d", n);
  return name;
}

Var *find_gstr_or_gen(Token *tok) {
  if (globals) {
    for (Var *var = globals; var; var = var->next) {
      if (*(var->name) != '.') continue;

      InitVal *v = var->init;
      if (!v) continue;
      if (tok->len == v->strlen && memcmp(tok->str, v->str, v->strlen) == 0) {
        return var;
      }
    }
  }

  Var *gvar = calloc(1, sizeof(Var));
  gvar->name = gen_gstr_name(gstr_len++);
  gvar->len = strlen(gvar->name);
  gvar->type = new_array_type(char_type, tok->len);
  gvar->init = calloc(1, sizeof(InitVal));
  gvar->init->str = tok->str;
  gvar->init->strlen = tok->len;

  gvar->next = globals;
  globals = gvar;
  if (gvar_has_circular()) error("gvar circlar1");

  return gvar;
}

int sizeof_lvars() {
  int size = 0;
  //int offset = 0;
  for (Var *var = locals; var; var = var->next) {
    size += sizeof_type(var->type);
    /*
    int size = sizeof_type(var->type);
    offset = align(offset, size);
    */
  }
  return align(size, 16);
}

