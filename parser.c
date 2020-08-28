#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "38cc.h"
#include "vector.h"

Type *consume_type();
Node *consume_member();
int sizeof_lvars();
GVar *find_gvar(Token *tok);
GVar *find_or_gen_gstr(Token *tok);
Type *find_type(Token *tok);
Type *find_or_gen_struct_type(Token *tok);
Type *find_struct_type(Token *tok);
Member *find_member(Vector *members, Token *tok);

Enum *find_enum(Token *tok);

Node *new_node(NodeKind kind);
Node *stmt();
Node *expr();
Node *equality();
Node *mul();
Node *unary();

int reduce_node(Node* node);

// current token
Token *token;

char *user_input;
Node *code[1000];
LVar *locals;
GVar *globals;

Vector *enums;

Vector *types;  // Type*
Vector *functions;

Type *char_type;
Type *short_type;
Type *int_type;
Type *long_type;
Type *float_type;
Type *double_type;

Type *void_type;
Type *ptr_char_type;

int gstr_len = 0; // number of global string variable

Type *new_type(char* name, int len, int size) {
  Type *type;
  type = calloc(1, sizeof(Type));
  type->name = name;
  type->len = len;
  type->size = size;
  return type;
}

Type *new_ptr_type(Type* type) {
  Type *ptr_type = new_type("*", 1, 8);
  ptr_type->kind = TY_PTR;
  ptr_type->to = type;
  return ptr_type;
}

Type *new_array_type(Type* type, int len) {
  Type *ary_type = new_type("[]", 2, type->size * len);
  ary_type->kind = TY_ARRAY;
  ary_type->to = type;
  return ary_type;
}

Type *new_fn_type(Type* ret_type) {
  Type *fn_type = new_type("(*)()", 5, 0);
  fn_type->kind = TY_FUNCTION;
  fn_type->to = ret_type;
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

LVar *new_lvar(Token *tok, Type *type) {
  LVar *lvar = calloc(1, sizeof(LVar));
  lvar->name = tok->str;
  lvar->len = tok->len; // strlen
  lvar->type = type;
  return lvar;
}

GVar *new_gvar(Token *tok, Type *type) {
}

GVar *new_function(Token *tok, Type* ret_type) {
  GVar *var = calloc(1, sizeof(GVar));
  var->name = substring(tok->str, tok->len);
  var->len = tok->len; // strlen
  var->type = new_fn_type(ret_type);
  return var;
}

void init_types() {
  enums = calloc(1, sizeof(Vector));
  types = calloc(1, sizeof(Vector));
  functions = calloc(1, sizeof(Vector));

  char_type = new_type("char", 4, 1);
  char_type->kind = TY_PRM;

  short_type = new_type("short", 5, 2);
  short_type->kind = TY_PRM;

  int_type = new_type("int", 3, 4);
  int_type->kind = TY_PRM;

  long_type = new_type("long", 4, 8);
  long_type->kind = TY_PRM;

  float_type = new_type("float", 5, 4);
  float_type->kind = TY_PRM;

  double_type = new_type("double", 6, 8);
  double_type->kind = TY_PRM;

  void_type = new_type("void", 4, 8);
  void_type->kind = TY_VOID;

  Type *builtin_va_list = new_type("__builtin_va_list", 17, 8);
  builtin_va_list->kind = TY_VOID;

  ptr_char_type = new_ptr_type(char_type);

  vec_add(types, void_type);
  vec_add(types, int_type);
  vec_add(types, short_type);
  vec_add(types, char_type);
  vec_add(types, long_type);
  vec_add(types, float_type);
  vec_add(types, double_type);

  vec_add(types, builtin_va_list);
}

int type_is_ptr_or_array(Type *type) {
  return type_is_ptr(type) || type_is_array(type);
}

int type_is_array(Type *type) {
  return type->len == 2 && memcmp(type->name, "[]", 2) == 0;
}

int type_is_ptr(Type *type) {
  return type->len == 1 && memcmp(type->name, "*", 1) == 0;
}

bool type_is_func(Type *type) {
  return type->to && !type_is_ptr_or_array(type);
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
  if (!lhs->type) {
    error("no type at new_node_deref %d", lhs->kind);
  }
  if (!lhs->type->to) {
    error("type is not ptr at new_node_deref: %s", lhs->type->name);
  }
  Node *node = new_node(ND_DEREF);
  node->lhs = lhs;
  node->type = lhs->type->to;
  return node;
}

Type *to_type(Type *lhs_type, Type *rhs_type) {
  if (type_is_func(lhs_type)) {
    lhs_type = lhs_type->to;
  }
  if (type_is_func(rhs_type)) {
    rhs_type = rhs_type->to;
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

  if (lhs_type == long_type || rhs_type == long_type) {
    return long_type;
  } else if (lhs_type == int_type || rhs_type == int_type) {
    return int_type;
  } else if (lhs_type == short_type || rhs_type == short_type) {
    return short_type;
  } else if (lhs_type == char_type || rhs_type == char_type) {
    return char_type;
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

Node *new_node_num(int val) {
  Node *node = new_node(ND_NUM);
  node->val = val;
  node->type = int_type;
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

// read reserved symbol
void expect(char *op) {
  if (!consume(op)) {
    error_at(token->str, "token is '%s' not '%s'. tk-kind: %d",
        substring(token->str, token->len), op, token->kind);
  }
}

bool consume_kind(TokenKind kind) {
  if (token->kind != kind) {
    return false;
  }
  token = token->next;
  return true;
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
  consume_kind(TK_CONST);

  Type *type;
  if (consume_kind(TK_STRUCT)) {
    if (token->kind != TK_IDENT) return NULL;

    type = find_struct_type(token);
    if (!type) return NULL;
    token = token->next;
  } else {
    if (token->kind != TK_IDENT) return NULL;

    char *p0 = token;
    while (1) {
      Type *type1 = find_type(token);
      if (is_pre_type(token)) {
        type = type1;
        token = token->next;
        continue;
      }
      if (type1) {
        type = type1;
        token = token->next;
      }
      break;
    }
    if (p0 == token) return NULL;
    if (!type) error_at(token->str, "type is not set");
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

Node *consume_char() {
  if (token->kind != TK_CHAR) {
    return NULL;
  }
  Node *node = new_node(ND_NUM);
  node->type = char_type;
  node->val = token->val;
  token = token->next;
  return node;
}

Node *consume_str() {
  if (token->kind != TK_STR) {
    return NULL;
  }
  GVar *gvar = find_or_gen_gstr(token);
  token = token->next;

  Node *node = new_node(ND_GVAR);
  node->type = gvar->type;
  node->ident = gvar->name;
  node->len = gvar->len;
  return node;
}

Node *consume_enum() {
  if (!consume_kind(TK_ENUM)) {
    return NULL;
  }

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
    //evar->tag = substring(tag->str, tag->len);
    
    evar->name = substring(tok->str, tok->len);
    evar->len = tok->len;

    evar->val = enum_id++;

    vec_add(enums, evar);

    if (!consume(",")) {
      expect("}");
      break;
    }
  }
  
  Node *node = new_node(ND_ENUM);
  node->type = int_type;

  if (tag) {
    node->ident = substring(tag->str, tag->len);
    node->len = tag->len;
  }
  return node;
}

int fixed_members_offset(Vector *members) {
  int unit = 1;
  int offset = 0;
  for (int i = 0; i < members->size; ++i) {
    Member *mem = vec_get(members, i);
    int size_mem = mem->type->size;
    if (unit < size_mem) {
      if (size_mem > 8) {
        unit = 8;
      } else {
        unit = size_mem;
      }
    }

    mem->offset = offset;
    if ((offset % size_mem) == 0) {
      offset += size_mem;
    } else {
      offset = (offset / size_mem + 2) * size_mem;
    }
  }
  
  if ((offset % unit) != 0) {
    offset = (offset / unit + 1) * unit;
  }
  return offset;
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

Node *consume_struct() {
  if (!consume_kind(TK_STRUCT)) {
    return NULL;
  }

  Node *node = new_node(ND_STRUCT);
  Token *tag = consume_ident();

  if (consume("{")) {
    Vector *members = new_vector();
    
    while (!consume("}")) {
      Node *mem = consume_member();
      Member *member = to_member(mem);
      vec_add(members, member);
    }

    int size_struct = fixed_members_offset(members);
    if (tag) {
      node->type = find_or_gen_struct_type(tag);
      if (node->type->size != 0) error("override type");

      node->type->size = size_struct;
    } else {
      node->type = new_type("_", 1, size_struct); // TODO
      node->type->kind = TY_STRUCT;
    }
    node->type->members = members;
  } else {
    if (!tag) error_at(token->str, "illegal struct");
    node->type = find_or_gen_struct_type(tag);
  }

  return node;
}

Node *consume_union() {
  if (!consume_kind(TK_UNION)) {
    return NULL;
  }

  Node *node = new_node(ND_UNION);
  Token *tag = consume_ident();
  if (consume("{")) {
    Vector *members = new_vector();
    while (!consume("}")) {
      Node *mem = consume_member();
      Member *member = to_member(mem);
      vec_add(members, member);
    }
    node->list = members;
  }
  return node;
}

Node *consume_member() {
  Node *node;

  if (node = consume_enum()) {
    Token *ident = consume_ident();
  } else if (node = consume_union()) {
    Token *ident = consume_ident();
    if (ident) {
      node->ident = substring(ident->str, ident->len);
      node->len = ident->len;
    }
    node->type = new_type("union", 5, 8); // TODO
  } else if (node = consume_struct()) {
    while (consume("*")) {
      node->type = new_ptr_type(node->type);
    }
    Token *ident = consume_ident();
    if (ident) {
      node->ident = substring(ident->str, ident->len);
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
    int array_len = reduce_node(equality());
    expect("]");

    node->type = new_array_type(node->type, array_len);
  }
  expect(";");

  return node;
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
      if (consume_kind(TK_VA)) {
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

Type *expect_type() {
  Token *tok;
  tok = consume_ident();
  if (!tok) {
    error_at(token->str, "illegal type name");
  }

  Type *type;
  type = find_type(tok);
  if (!type) {
    error_at(token->str, "undefined type");
  }

  return type;
}

// defined local variable
LVar *consume_lvar_define() {
  Type *type = consume_type();
  if (!type) {
    return NULL;
  }

  Token *tok = consume_ident();
  if (!tok)
    error_at(token->str, "illegal lvar name");
  if (find_lvar(tok))
    error_at(tok->str, "duplicated defined lvar");

  int new_size;
  if (consume("[")) {
    int array_len = expect_number();
    expect("]");

    new_size = type->size * array_len;
    type = new_array_type(type, array_len);
  } else {
    new_size = type->size;
  }

  LVar *lvar = new_lvar(tok, type);
  if (locals) {
    lvar->offset = locals->offset + new_size;
  } else {
    lvar->offset = new_size;
  }
  lvar->next = locals;
  locals = lvar;
  return lvar;
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

  Vector *args = calloc(1, sizeof(Vector));
  for (;;) {
    if (consume_kind(TK_VA)) {
      if (consume(")")) break;
      expect(",");
      continue;
    }

    Type *type = consume_type();
    if (!type) {
      error_at(token->str,
          "illegal arg type (size: %d)", args->size);
    }

    // skip const
    consume_kind(TK_CONST);
    consume("*");

    Token *tok;
    // some arg var names are ommited.
    if (tok = consume_ident()) {
      if (consume("[")) {
        if (consume("]")) {
          type = new_ptr_type(type);
        } else {
          int array_len = expect_number();
          expect("]");
          type = new_array_type(type, array_len);
        }
      }
    } else if (tok = consume_fp()) {
      expect("(");
      while (!consume(")")) {
        token = token->next;
      }
      type = new_type("fn", 2, 8);
      type->kind = TY_PTR;
    }

    Node *node = new_node(ND_LVAR);
    node->type = type;

    if (tok) {
      LVar *lvar = new_lvar(tok, type);
      if (locals) {
        lvar->offset = locals->offset + type->size;
      } else {
        lvar->offset = type->size;
      }
      lvar->next = locals;
      locals = lvar;

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

Node *consume_sizeof() {
  if (!consume_kind(TK_SIZEOF)) {
    return NULL;
  }

  expect("(");
  Type *type = consume_type();
  if (type) {
    expect(")");
    return new_node_num(type->size);
  }

  Node *node = equality();
  expect(")");
  return new_node_num(sizeof_node(node));
}

Type *typeof(Node* node) {
  if (node->kind == ND_NUM
      || node->kind == ND_ADDR
      || node->kind == ND_DEREF
      || node->kind == ND_STRUCT // TODO
      || node->kind == ND_LVAR) {
    return node->type;
  }
  return typeof(node->lhs);
}

int sizeof_node(Node* node) {
  Type *type = typeof(node);
  return type->size;
}

Node *primary() {

  Node *node;
  Token *tok;

  if (consume("(")) {
    Type *type = consume_type();
    if (type) {
      expect(")");

      // TODO cast (not supported)
      node = expr();

    } else {
      node = expr();
      expect(")");
    }
    return node;
  }

  node = consume_char();
  if (node) {
    return node;
  }

  node = consume_str();
  if (node) {
    return node;
  }

  tok = consume_ident();
  if (!tok) {
    return new_node_num(expect_number());
  }

  // function call
  if (consume("(")) {
    GVar *gvar = find_gvar(tok);
    if (gvar) {
      node = new_node(ND_CALL);

      node->type = gvar->type->to;
      node->val = gvar->extn;
    } else {
      LVar *lvar = find_lvar(tok);
      if (lvar) {
        node = new_node(ND_CALL);
        node->type = lvar->type->to;
        node->val = 0;
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

  LVar *lvar = find_lvar(tok);
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
      GVar *gvar = find_gvar(tok);
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
  Type *ty = type;
  while (ty->kind == TY_TYPEDEF) {
    ty = ty->to;
  }
  if (!ty) error_at(token->str, "no type");

  Vector *members = ty->members;
  if (!members)
    error_at(token->str, "no members: %s",
        substring(ty->name, ty->len));

  Member *member = find_member(members, ident);
  if (!member) {
    Member *tmp = vec_get(members, 1);
    error_at(token->str, "no member %s-%s",
        substring(ident->str, ident->len),
        substring(tmp->name, tmp->len));
  }
  return member;
}

Node *unary() {
  // pre ++
  if (consume("++")) {
    Node *node = primary();
    return new_node_lr(ND_ASSIGN, node, new_node_lr(ND_ADD, node, new_node_num(1)));
  }
  // pre -- 
  if (consume("--")) {
    Node *node = primary();
    return new_node_lr(ND_ASSIGN, node, new_node_lr(ND_SUB, node, new_node_num(1)));
  }
  if (consume("-")) {
    return new_node_lr(ND_SUB, new_node_num(0), primary());
  }
  if (consume("+")) {
    return primary();
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

  Node *node = consume_sizeof();
  if (node) {
    return node;
  }

  node = primary();

  while (1) {
    // post ++ or --
    if (consume("++")) {
      node = new_node_lr(ND_ASSIGN_POST, node, new_node_lr(ND_ADD, node, new_node_num(1)));
      continue;
    }
    if (consume("--")) {
      node = new_node_lr(ND_ASSIGN_POST, node, new_node_lr(ND_SUB, node, new_node_num(1)));
      continue;
    }

    if (consume("[")) {
      if (!type_is_ptr_or_array(node->type)) {
        error_at((token->str - 1), "Not ptr");
      }
      Node *index = expr();
      expect("]");
      node = new_node_deref(new_node_lr(ND_ADD, node, index));
      continue;
    }

    if (consume(".")) {
      Token *ident = consume_ident();
      if (!ident) error_at(token->str, "no ident");
      Member *member = get_member(node->type, ident);
      int new_offset = node->offset - member->offset;

      node = new_node(ND_LVAR);
      node->offset = new_offset;
      node->type = member->type;

      continue;
    }

    if (consume("->")) {
      Token *ident = consume_ident();
      if (!ident) error_at(token->str, "no ident");
      Member *member = get_member(node->type->to, ident);

      int new_offset = node->offset - member->offset;

      Node *addr = new_node_lr(ND_SUB, node, new_node_num(member->offset));
      addr->type = new_ptr_type(member->type);
      node = new_node_deref(addr);
      continue;
    }

    return node;
  }
}

Node *mul() {
  Node *node = unary();
  for (;;) {
    if (consume("*")) {
      node = new_node_lr(ND_MUL, node, unary());
    } else if (consume("/")) {
      node = new_node_lr(ND_DIV, node, unary());
    } else if (consume("%")) {
      node = new_node_lr(ND_MOD, node, unary());
    } else {
      return node;
    }
  }
  return node;
}

Node *add() {
  Node *node = mul();
  for (;;) {
    if (consume("+")) {
      node = new_node_lr(ND_ADD, node, mul());
    } else if (consume("-")) {
      node = new_node_lr(ND_SUB, node, mul());
    } else {
      return node;
    }
  }
}

Node *relational() {
  Node *node = add();
  for (;;) {
    if (consume("<")) {
      node = new_node_lr(ND_LT, node, add());
    } else if (consume("<=")) {
      node = new_node_lr(ND_LE, node, add());
    } else if (consume(">=")) {
      node = new_node_lr(ND_LE, add(), node);
    } else if (consume(">")) {
      node = new_node_lr(ND_LT, add(), node);
    } else {
      return node;
    }
  }
}

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

Node *assign() {
  Node *node = equality();
  if (consume("+=")) {
    node = new_node_lr(ND_ASSIGN, node, new_node_lr(ND_ADD, node, assign()));
  } else if (consume("-=")) {
    node = new_node_lr(ND_ASSIGN, node, new_node_lr(ND_SUB, node, assign()));
  } else if (consume("*=")) {
    node = new_node_lr(ND_ASSIGN, node, new_node_lr(ND_MUL, node, assign()));
  } else if (consume("/=")) {
    node = new_node_lr(ND_ASSIGN, node, new_node_lr(ND_DIV, node, assign()));
  } else if (consume("%=")) {
    node = new_node_lr(ND_ASSIGN, node, new_node_lr(ND_MOD, node, assign()));
  } else if (consume("=")) {
    node = new_node_lr(ND_ASSIGN, node, assign());
  }
  return node;
}

Node *expr() { return assign(); }

Node *stmt() {
  Node *node;
  if (consume("{")) {
    node = new_node(ND_BLOCK);
    Vector *stmt_list = calloc(1, sizeof(Vector));
    do {
      Node *sub = stmt();
      vec_add(stmt_list, sub);
    } while (!consume("}"));
    node->list = stmt_list;
  } else if (consume_kind(TK_IF)) {
    node = new_node(ND_IF);

    expect("(");
    node->lhs = expr();
    expect(")");

    Node *tmp = stmt();
    if (!consume_kind(TK_ELSE)) {
      node->rhs = tmp;
    } else {
      Node *sub = new_node(ND_ELSE);
      node->rhs = sub;

      sub->lhs = tmp;
      sub->rhs = stmt();
    }
  } else if (consume_kind(TK_WHILE)) {
    node = new_node(ND_WHILE);
    expect("(");
    node->lhs = expr();
    expect(")");
    node->rhs = stmt();
  } else if (consume_kind(TK_FOR)) {
    node = new_node(ND_FOR);
    expect("(");

    if (!consume(";")) {
      node->lhs = expr();
      expect(";");
    }

    Node *node_while = new_node(ND_WHILE);
    node->rhs = node_while;

    if (!consume(";")) {
      node_while->lhs = expr();
      expect(";");
    }
    if (consume(")")) {
      node_while->rhs = stmt();
    } else {
      Node *sub = new_node(ND_NONE);
      sub->rhs = expr();
      expect(")");
      sub->lhs = stmt();

      node_while->rhs = sub;
    }
  } else if (consume_kind(TK_RETURN)) {
    node = new_node(ND_RETURN);
    node->lhs = expr();
    expect(";");
  } else {
    LVar *lvar = consume_lvar_define();
    if (lvar) {
      node = new_node(ND_LVAR);
      node->offset = lvar->offset;
      node->type = lvar->type;
      if (consume("=")) {
        node = new_node_lr(ND_ASSIGN, node, assign());
      }
    } else {
      node = expr();
    }
    expect(";");
  }
  return node;
}

Node *block_stmt() {
  if (!consume("{")) return NULL;

  Node *node = new_node(ND_BLOCK);
  node->list = calloc(1, sizeof(Vector));

  while (!consume("}")) {
    vec_add(node->list, stmt());
  }
  return node;
}

int sizeof_args(Vector *args) {
  if (!args) {
    return 0;
  }
  int size = 0;
  for (int i = 0; i < args->size; ++i) {
    Node *node = vec_get(args, i);
    size += node->type->size;
  }
  return size;
}

int reduce_node(Node* node) {
  if (node->kind == ND_ADD) {
    return reduce_node(node->lhs) + reduce_node(node->rhs);
  }
  if (node->kind == ND_SUB) {
    return reduce_node(node->lhs) - reduce_node(node->rhs);
  }
  if (node->kind == ND_MUL) {
    return reduce_node(node->lhs) * reduce_node(node->rhs);
  }
  if (node->kind == ND_DIV) {
    return reduce_node(node->lhs) / reduce_node(node->rhs);
  }
  if (node->kind == ND_MOD) {
    return reduce_node(node->lhs) % reduce_node(node->rhs);
  }
  if (node->kind != ND_NUM) {
    error_at(token->str, "not supported %d", node->kind);
  }
  return node->val;
}

Node *global() {

  if (consume_kind(TK_TYPEDEF)) {
    Node *node;
    if (node = consume_struct()) {
      while (consume("*")) {
        // TODO
        //node->type = new_ptr_type(node->type);
      }

      Token *ident = consume_ident();
      if (!ident) error_at(token->str, "Illegal typedef struct");

      if (ident->kind == TK_IDENT) {
        // TODO to support type alias

        if (!node->type) error_at(token->str, "No defined type");

        int size_t = node->type->size;
        Type *struct_type = new_type(ident->str, ident->len, size_t);
        struct_type->kind = TY_TYPEDEF;
        struct_type->to = node->type;

        vec_add(types, struct_type);
      }
      expect(";");
      return NULL;
    }
    if (node = consume_enum()) {
      Token *ident = consume_ident();
      if (!ident) {
        error_at(token->str, "Illegal typedef enum");
      }
      if (ident->kind == TK_IDENT) {
        // TODO to support type alias
        Type *enum_type = new_type(ident->str, ident->len, 4);
        enum_type->kind = TY_PRM;

        vec_add(types, enum_type);
      }
      expect(";");
      return NULL;
    }
    if (node = consume_union()) {
      Token *ident = consume_ident();
      if (!ident) {
        error_at(token->str, "Illegal typedef union");
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
    if (ident = consume_ident()) {
      if (consume("(")) {
        expect_defined_args();
      } else {
        //fprintf(stderr, "typedef: %s\n", substring(ident->str, ident->len));

        Type *def_type = new_type(ident->str, ident->len, type->size);
        def_type->kind = TY_TYPEDEF;
        def_type->to = type;
        vec_add(types, def_type);
      }
    } else if (ident = consume_fp()) {
      if (consume("(")) {
        expect_defined_args();
      }

      //fprintf(stderr, "typedef(*fp): %s\n", substring(ident->str, ident->len));

      Type *def_type = new_type(ident->str, ident->len, type->size);
      def_type->kind = TY_TYPEDEF;
      def_type->to = type;
      vec_add(types, def_type);
    }
    expect(";");

    return NULL;
  }

  int is_extern = 0;
  if (consume_kind(TK_EXTERN)) {
    is_extern = 1;
  }

  Node *node;
  if (node = consume_struct()) {
    if (is_extern) {
      // 変数宣言
      Type *t = node->type;
      while (consume("*")) {
         t = new_ptr_type(t);
      }
      Token *ident = consume_ident();
      // TODO
    }
    expect(";");
    return NULL;
  }
  if (node = consume_enum()) {
    if (is_extern) {
      Token *ident = consume_ident();
    }
    expect(";");
    return NULL;
  }
  if (node = consume_union()) {
    if (is_extern) {
      Token *ident = consume_ident();
    }
    expect(";");
    return NULL;
  }

  Type *type = consume_type();
  if (!type) error_at(token->str, "type is not found");

  // skip const
  consume_kind(TK_CONST);

  Token *tok = consume_ident();
  if (!tok) {
    error_at(token->str, "illegal defined function ident");
  }
  if (consume("(")) {
    // before parse function statement for recursive call
    GVar *func = new_function(tok, type);
    func->extn = is_extern;
    
    func->next = globals;
    globals = func;
    
    fprintf(stderr, "fn: %s\n", substring(tok->str, tok->len));

    Node *block = NULL;
    Vector *args;

    LVar *tmp_locals = locals;
    locals = NULL;

    args = expect_defined_args();
    block = block_stmt();
    if (!block) {
      while (!consume(";")) {
        // skip unsupported tokens
        token = token->next;
      }
    }

    Node *node;
    node = new_node(ND_FUNCTION);
    node->list = args;
    node->ident = substring(tok->str, tok->len);
    node->len = tok->len;
    node->lhs = block;
    node->val = sizeof_args(args) + sizeof_lvars();
    node->type = func->type->to;

    // local変数を戻す(サイズ計算後)
    locals = tmp_locals;
    return node;

  } else {

    GVar *gvar = find_gvar(tok);
    if (gvar) {
      if (gvar->extn && !is_extern) {
        error_at(tok->str, "duplicated defined gvar");
      }
      gvar->extn = 0;
    } else {
      gvar = calloc(1, sizeof(GVar));
      gvar->extn = is_extern;
    }

    gvar->name = substring(tok->str, tok->len);
    gvar->len = tok->len;

    if (consume("[")) {
      if (token->kind == TK_NUM) {
        // gvar[n]
        int array_len = expect_number();
        expect("]");
        type = new_array_type(type, array_len);
      } else {
        // gvar[]
        type = new_array_type(type, type->size);
        expect("]");
      }
    }

    gvar->type = type;
    gvar->val = tok->val;

    gvar->next = globals;
    globals = gvar;

    Node *node = new_node(ND_GVAR);
    node->ident = substring(tok->str, tok->len);
    node->len = tok->len;
    node->type = gvar->type;
    if (consume("=")) {
      if (type_is_array(type) && type->to == char_type) {
        if (token->kind != TK_STR) {
          error("expected string literal");
        }
        gvar->str = substring(token->str, token->len);
        token = token->next;
      } else if (type_is_ptr(type) && type->to == char_type) {
        Node* subnode = equality();
        if (subnode->kind != ND_GVAR) {
          error("expected gvar (string literal)");
        }
        gvar->str = subnode->ident;
      } else {
        gvar->val = reduce_node(equality());
      }
      gvar->init = 1;
    } else {
      if (node->type != ptr_char_type) {
        gvar->val = 0;
      } else {
        printf("# not initialize gvar str\n");
      }
      gvar->init = 0;
    }
    expect(";");
    return node;
  }
}


void program() {
  init_types();

  int i = 0;
  while (!at_eof()) {
    Node* n = global();
    if (n) {
      code[i++] = n;
    }
  }
  code[i] = NULL;
  printf("# program loaded %dsteps\n", i);
}

Type *find_type(Token *tok) {
  for (int i = 0; i < types->size; ++i) {
    Type *type = vec_get(types, i);
    if (type->kind == TY_STRUCT) continue;
    if (type->len == tok->len && memcmp(tok->str, type->name, type->len) == 0) {
      return type;
    }
  }
  return NULL;
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

Type *find_or_gen_struct_type(Token *tok) {
  Type *type = find_struct_type(tok);
  if (type) return type;

  type = new_type(tok->str, tok->len, 0);
  type->kind = TY_STRUCT;

  vec_add(types, type);
  return type;
}

LVar *find_lvar(Token *tok) {
  for (LVar *var = locals; var; var = var->next) {
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

GVar *find_gvar(Token *tok) {
  for (GVar *var = globals; var; var = var->next) {
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

GVar *find_or_gen_gstr(Token *tok) {
  if (globals) {
    for (GVar *var = globals; var; var = var->next) {
      if (*(var->name) == '.'
          && var->len == tok->len
          && memcmp(tok->str, var->str, var->len) == 0) {
        return var;
      }
    }
  }

  GVar *gvar = calloc(1, sizeof(GVar));
  gvar->name = gen_gstr_name(gstr_len++);
  gvar->type = new_array_type(char_type, tok->len);
  gvar->init = 1;

  gvar->str = substring(tok->str, tok->len);
  gvar->len = tok->len;

  gvar->next = globals;
  globals = gvar;

  return gvar;
}

int sizeof_lvars() {
  int size = 0;
  for (LVar *var = locals; var; var = var->next) {
    size += var->type->size;
  }
  return size;
}

