#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "38cc.h"
#include "vector.h"

Node *new_node(NodeKind kind);
Node *stmt();
Node *mul();
Node *expr();
Type *consume_type();
int sizeof_lvars();
GVar *find_gvar(Token *tok);
GVar *find_or_gen_str_gvar(Token *tok);
Type *find_type(Token *tok);
Function *find_function(Token *tok);

// current token
Token *token;

char *user_input;
Node *code[1000];
LVar *locals;
GVar *globals;
Vector *types;  // Type*
Vector *functions;

Type *char_type;
Type *short_type;
Type *int_type;
Type *long_type;

Type *void_type;
Type *str_type;

int gstr_len = 0; // number of global string variable

Type *new_type(char* name, int len, int size) {
  Type *type;
  type = calloc(1, sizeof(Type));
  type->name = name;
  type->len = len;
  type->size = size;
}

Type *new_ptr_type(Type* type) {
  Type *ptr_type = new_type("*", 1, 8);
  ptr_type->ptr_to = type;
  return ptr_type;
}

Type *new_array_type(Type* type, int len) {
  Type *array_type = new_type("[]", 2, type->size * len);
  array_type->ptr_to = type;
  return array_type;
}

Function *new_function(char* name, int len, Type* ret_type) {
  Function *func = calloc(1, sizeof(Function));
  func->name = name;
  func->len = len;
  func->ret_type = ret_type;
  return func;
}

void init_types() {
  types = calloc(1, sizeof(Vector));
  functions = calloc(1, sizeof(Vector));

  char_type = new_type("char", 4, 1);
  short_type = new_type("short", 5, 2);
  int_type = new_type("int", 3, 4);
  long_type = new_type("long", 4, 8);

  void_type = new_type("void", 4, 8);

  str_type = new_ptr_type(char_type);

  vec_add(types, void_type);
  vec_add(types, int_type);
  vec_add(types, short_type);
  vec_add(types, char_type);
  vec_add(types, long_type);
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
  return type->ptr_to && !type_is_ptr_or_array(type);
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
  if (!lhs->type->ptr_to) {
    error("type is not ptr at new_node_deref");
  }
  Node *node = new_node(ND_DEREF);
  node->lhs = lhs;
  node->type = lhs->type->ptr_to;
  return node;
}

Type *to_type(Type *lhs_type, Type *rhs_type) {
  if (type_is_func(lhs_type)) {
    lhs_type = lhs_type->ptr_to;
  }
  if (type_is_func(rhs_type)) {
    rhs_type = rhs_type->ptr_to;
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
  } else {
    error("parser assertion error");
  }
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
  } else if (kind == ND_ASSIGN) {
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

bool token_is(Token *tok, char *op) {
  return tok && tok->kind == TK_RESERVED && strlen(op) == tok->len &&
         memcmp(op, tok->str, tok->len) == 0;
}

bool consume(char *op) {
  if (!token_is(token, op)) {
    return false;
  }
  token = token->next;
  return true;
}

// read reserved symbol
void expect(char *op) {
  if (!token_is(token, op)) {
    error_at(token->str, "token is '%s' not '%s'. tk-kind: %d",
        substring(token->str, token->len), op, token->kind);
  }
  token = token->next;
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

Node *consume_char() {
  if (token->kind != TK_CHAR) {
    return NULL;
  }
  Node *node = new_node(ND_NUM);
  node->type = char_type;
  node->val = *token->str;
  token = token->next;
  return node;
}

Node *consume_str() {
  if (token->kind != TK_STR) {
    return NULL;
  }
  GVar *gvar = find_or_gen_str_gvar(token);
  token = token->next;

  Node *node = new_node(ND_GVAR);
  node->type = gvar->type;
  node->ident = gvar->name;
  return node;
}

int is_pre_type(Token* tok) {
  return (tok->len == 4 && memcmp(tok->str, "auto", 4) == 0) ||
      (tok->len == 6 && memcmp(tok->str, "static", 6) == 0) ||
      (tok->len == 6 && memcmp(tok->str, "signed", 6) == 0) ||
      (tok->len == 8 && memcmp(tok->str, "unsigned", 8) == 0) ||
      (tok->len == 4 && memcmp(tok->str, "long", 4) == 0) ||
      (tok->len == 5 && memcmp(tok->str, "short", 5) == 0);
}

Type *consume_type() {
  if (consume_kind(TK_CONST)) {
    // skip
  }

  if (token->kind != TK_IDENT) {
    return NULL;
  }

  Type *type;
  char *p0 = token->str;
  for (;;) {
    Type *type1 = find_type(token);
    if (type1 || is_pre_type(token)) {
      type = type1;
      token = token->next;
      continue;
    }
    break;
  }
  if (p0 == token->str) {
    return NULL;
  }
  if (!type) {
    error_at(token->str, "# type is not set");
  }
  while (consume("*")) {
    type = new_ptr_type(type);
  }
  return type;
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
  if (!tok) {
    error_at(token->str, "illegal lvar name");
  }
  if (find_lvar(tok)) {
    error_at(tok->str, "duplicated defined lvar");
  }

  int new_size;
  if (consume("[")) {
    int array_len = expect_number();
    expect("]");

    new_size = type->size * array_len;
    type = new_array_type(type, array_len);
  } else {
    new_size = type->size;
  }

  LVar *lvar;
  lvar = calloc(1, sizeof(LVar));
  lvar->name = tok->str;
  lvar->len = tok->len; // strlen
  lvar->type = type;

  if (locals) {
    lvar->offset = locals->offset + new_size;
  } else {
    lvar->offset = new_size;
  }
  lvar->next = locals;
  locals = lvar;
  return lvar;
}

Vector *defined_args() {
  Vector *args = NULL;
  if (!consume(")")) {
    args = calloc(1, sizeof(Vector));
    for (;;) {
      LVar *lvar = consume_lvar_define();
      if (!lvar) {
        error_at(token->str, "illegal arg");
      }
      Node *node = new_node(ND_LVAR);
      node->ident = substring(lvar->name, lvar->len);
      node->offset = lvar->offset;
      node->type = lvar->type;

      vec_add(args, node);
      if (consume(")")) {
        break;
      }
      expect(",");
    }
  }
  return args;
}

Vector *defined_extern_args() {
  Vector *args = NULL;
  if (!consume(")")) {
    args = calloc(1, sizeof(Vector));
    for (;;) {
      if (consume_kind(TK_VA)) {
        if (consume(")")) {
          break;
        }
        expect(",");
        continue;
      }

      Type *type = consume_type();
      if (!type) {
        error_at(token->str,
            "illegal extern arg type (size: %d)", args->size);
      }

      if (consume_kind(TK_RESTRICT)) {
        // skip __restrict
      }
      if (consume_kind(TK_CONST)) {
        // skip const
      }

      int new_size;
      Token *tok = consume_ident();
      if (tok) {
        if (consume("[")) {
          int array_len = expect_number();
          expect("]");

          new_size = type->size * array_len;
          type = new_array_type(type, array_len);
        } else {
          new_size = type->size;
        }
      }

      Node *node = new_node(ND_LVAR);
      node->type = type;
      node->ident = "";

      vec_add(args, node);
      if (consume(")")) {
        break;
      }
      expect(",");
    }
  }
  return args;
}

int sizeof_node(Node* node) {
  if (node->kind == ND_NUM || node->kind == ND_ADDR || node->kind == ND_DEREF) {
    return node->type->size;
  } else if (node->kind == ND_LVAR) {
    return node->type->size;
  } else {
    return sizeof_node(node->lhs);
  }
}

Node *primary() {

  if (consume("(")) {
    Node *node = expr();
    expect(")");
    return node;
  }

  Node *node;

  node = consume_char();
  if (node) {
    return node;
  }

  node = consume_str();
  if (node) {
    return node;
  }

  Token *tok = consume_ident();
  if (!tok) {
    return new_node_num(expect_number());
  }

  if (consume("(")) {
    Node *node = new_node(ND_CALL);
    node->list = consume_args();
    node->ident = substring(tok->str, tok->len);
    Function *func = find_function(tok);
    if (!func) {
      error_at(tok->str, "func is not found");
    }
    node->type = func->ret_type;
    node->val = func->plt;

    if (!node->type) {
      error(tok->str, "return type of function is not found");
    }
    return node;
  }

  LVar *lvar = find_lvar(tok);
  if (lvar) {
    node = new_node(ND_LVAR);
    node->type = lvar->type;
    node->ident = substring(tok->str, tok->len);
    node->offset = lvar->offset;
  } else {
    GVar *gvar = find_gvar(tok);
    if (!gvar) {
      error_at(tok->str, "undefined gvar");
    }
    node = new_node(ND_GVAR);
    node->type = gvar->type;
    node->ident = substring(tok->str, tok->len);
  }
  if (type_is_ptr_or_array(node->type) && consume("[")) {
    Node *index = expr();
    node = new_node_deref(
        new_node_lr(ND_ADD, node, index));

    expect("]");
  }
  return node;
}

Node *unary() {
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
  if (consume_kind(TK_SIZEOF)) {
    Node *node = unary();
    return new_node_num(sizeof_node(node));
  }
  return primary();
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
  if (consume("=")) {
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
  Node *node = NULL;
  if (consume("{")) {
    node = new_node(ND_BLOCK);
    Vector *stmt_list = calloc(1, sizeof(Vector));
    do {
      Node *sub = stmt();
      vec_add(stmt_list, sub);
    } while (!consume("}"));
    node->list = stmt_list;
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

Node *reduce_node(Node* node) {
  if (node->kind == ND_NUM) {
    return node;
  }
  error("not supported");
}

Node *global() {
  if (consume_kind(TK_TYPEDEF)) {
    int brace = 0;
    Token *tok;

    for (;;) {
      if (brace == 0 && consume(";")) {
        break;
      }
      if (consume("{")) {
        brace++;
        continue;
      } else if (consume("}")) {
        brace--;
        continue;
      }
      tok = token;
      token = token->next;
    }
    if (tok->kind == TK_IDENT) {
      Type *def_type = new_type(tok->str, tok->len, 8);
      vec_add(types, def_type);
    }
    return NULL;
  }

  int is_extern = 0;
  if (consume_kind(TK_EXTERN)) {
    is_extern = 1;
  }
  if (consume_kind(TK_STRUCT)) {
    if (!find_type(token)) {
      Type *struct_type = new_type(token->str, token->len, 8);
      vec_add(types, struct_type);
    }
    token = token->next;

    int brace = 0;
    for (;;) {
      if (brace == 0 && consume(";")) {
        break;
      }
      if (consume("{")) {
        brace++;
        continue;
      } else if (consume("}")) {
        brace--;
        continue;
      }
      token = token->next;
    }
    return NULL;
  }

  Type *type = consume_type();
  if (!type) {
    if (is_extern) {
      printf("# type is not found: %s\n",
          substring(token->str, token->len));
      while (!consume(";")) {
        token = token->next;
      }
      return NULL;
    }
    error_at(token->str, "type is not found");
  }

  if (consume_kind(TK_CONST)) {
  }
  Token *tok = consume_ident();
  if (!tok) {
    error_at(token->str, "illegal defined function ident");
  }
  if (consume("(")) {
    // before parse function statement for recursive call
    Function *func = new_function(tok->str, tok->len, type);
    vec_add(functions, func);

    Node *block;
    Vector *args;

    LVar *tmp_locals = locals;
    locals = NULL;

    if (is_extern) {
      args = defined_extern_args();

      while (!consume(";")) {
        // skip unsupported tokens
        token = token->next;
      }
      func->plt = 1;
    } else {
      args = defined_args();
      block = block_stmt();
      if (!block) {
        error_at(token->str, "illegal defined block");
      }
    }

    Node *node;
    node = new_node(ND_FUNCTION);
    node->list = args;
    node->ident = substring(tok->str, tok->len);
    node->lhs = block;
    node->val = sizeof_args(args) + sizeof_lvars();
    node->type = func->ret_type;

    // local変数を戻す(サイズ計算後)
    locals = tmp_locals;
    return node;

  } else {

    GVar *gvar = find_gvar(tok);
    if (gvar) {
      error_at(tok->str, "duplicated defined gvar");
    }

    gvar = calloc(1, sizeof(GVar));
    gvar->name = substring(tok->str, tok->len);
    gvar->len = tok->len;

    int new_size;
    if (consume("[")) {
      if (token->kind == TK_NUM) {
        // gvar[n]
        int array_len = expect_number();
        expect("]");

        new_size = type->size * array_len;
        type = new_array_type(type, array_len);
      } else {
        // []
        type = new_array_type(type, type->size);
        expect("]");
      }
    } else {
      new_size = type->size;
    }

    gvar->type = type;
    gvar->val = tok->val;

    gvar->next = globals;
    globals = gvar;

    Node *node = new_node(ND_GVAR);
    node->type = gvar->type;
    node->ident = substring(tok->str, tok->len);

    if (consume("=")) {
      gvar->val = reduce_node(equality())->val;
    } else {
      if (node->type != str_type) {
        gvar->val = 0;
      } else {
        printf("# not initialize gvar str\n");
      }
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
    if (type->len == tok->len && memcmp(tok->str, type->name, type->len) == 0) {
      return type;
    }
  }
  return NULL;
}

Function *find_function(Token *tok) {
  for (int i = 0; i < functions->size; ++i) {
    Function *func = vec_get(functions, i);
    if (func->len == tok->len && memcmp(tok->str, func->name, func->len) == 0) {
      return func;
    }
  }
  return NULL;
}

LVar *find_lvar(Token *tok) {
  for (LVar *var = locals; var; var = var->next) {
    if (var->len == tok->len && memcmp(tok->str, var->name, var->len) == 0) {
      return var;
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

GVar *find_or_gen_str_gvar(Token *tok) {
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
  gvar->type = str_type;

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

