#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "38cc.h"
#include "vector.h"

Node *new_node(NodeKind kind);
Node *stmt();
Node *mul();
Type *consume_type();
int sizeof_lvars();
GVar *find_gvar(Token *tok);
Type *find_type(Token *tok);
Type *find_function(Token *tok);

// current token
Token *token;

char *user_input;
Node *code[100];
LVar *locals;
GVar *globals;
Vector *types;  // Type*
Vector *functions;

Type *int_type;
Type *char_type;

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

Type *new_function_type(char* name, int len, Type* ret_type) {
  Type *func_type = new_type(name, len, 8);
  func_type->ptr_to = ret_type;
  return func_type;
}

void init_types() {
  types = calloc(1, sizeof(Vector));
  functions = calloc(1, sizeof(Vector));

  int_type = new_type("int", 3, 8);
  char_type = new_type("char", 4, 8);

  vec_add(types, int_type);
  vec_add(types, char_type);
}

int type_is_array(Type *type) {
  return memcmp(type->name, "[]", type->len) == 0;
}

int type_is_ptr(Type *type) {
  return memcmp(type->name, "*", type->len) == 0;
}

bool type_is_func(Type *type) {
  return type->ptr_to && !type_is_ptr(type) && !type_is_array(type);
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

Node *new_node_lr(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = new_node(kind);
  node->kind = kind;
  node->lhs = lhs;
  node->rhs = rhs;

  if (!lhs->type) {
    error("lhs no type: %d -> %d", node->kind, lhs->kind);
  }
  if (kind == ND_ASSIGN) {
    node->type = lhs->type;
    return node;
  }

  if (!rhs->type) {
    error("rhs no type: %d -> %d", node->kind, rhs->kind);
  }
  Type *lhs_type = lhs->type;
  Type *rhs_type = rhs->type;

  if (type_is_func(lhs_type)) {
    lhs_type = lhs_type->ptr_to;
  }
  if (type_is_func(rhs_type)) {
    rhs_type = rhs_type->ptr_to;
  }

  if (lhs_type == rhs_type) {
    node->type = lhs_type;
  } else if ((lhs_type == int_type || lhs_type == char_type)
      && (rhs_type == int_type || rhs_type == char_type)) {
    node->type = int_type;
  } else if (kind == ND_EQ
      || kind == ND_NE
      || kind == ND_LT
      || kind == ND_LE) {
    node->type = int_type;
  } else if (kind == ND_ADDR) {
    node->type = new_ptr_type(lhs_type);
  } else {
    int lhs_ptr = type_is_ptr(lhs_type) || type_is_array(lhs_type);
    int rhs_ptr = type_is_ptr(rhs_type) || type_is_array(rhs_type);
    if (lhs_ptr && rhs_ptr) {
      node->type = int_type;
    } else if (lhs_ptr) {
      node->type = lhs_type;
    } else if (rhs_ptr) {
      node->type = lhs_type;
    }
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
    error("token is '%s' not '%s'. tk-kind: %d",
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
  Token *tmp;
  tmp = token;
  token = token->next;
  return tmp;
}

Type *consume_type() {
  if (token->kind != TK_IDENT) {
    return NULL;
  }
  Type *type = find_type(token);
  if (!type) {
    return NULL;
  }
  token = token->next;

  while (consume("*")) {
    type = new_ptr_type(type);
  }
  return type;
}

// read reserved integer number
int expect_number() {
  if (token->kind != TK_NUM) {
    error("token is '%s' not number. tk-kind: %d",
        substring(token->str, token->len), token->kind);
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
    args = calloc(1, sizeof(Vector));
    for (;;) {
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
    error("illegal type: %s", substring(token->str, token->len));
  }

  Type *type;
  type = find_type(tok);
  if (!type) {
    error("undefined type: %s", substring(tok->str, tok->len));
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
    error("illegal lvar name");
  }
  if (find_lvar(tok)) {
    error("duplicated defined lvar %s", substring(tok->str, tok->len));
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
        error("illegal lvar");
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

int sizeof_node(Node* node) {
  if (node->kind == ND_NUM) {
    return 8;
  } else if (node->kind == ND_ADDR) {
    return 8;
  } else if (node->kind == ND_DEREF) {
    return sizeof_node(node->lhs);
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

  Token *tok = consume_ident();
  if (!tok) {
    return new_node_num(expect_number());
  }

  if (consume("(")) {
    Node *node = new_node(ND_CALL);
    node->list = consume_args();
    node->ident = substring(tok->str, tok->len);
    node->type = find_function(tok);
    if (!node->type) {
      error("func not found: %s", node->ident);
    }
    return node;
  }

  Node *node;
  LVar *lvar = find_lvar(tok);
  if (lvar) {
    node = new_node(ND_LVAR);
    node->type = lvar->type;
    node->ident = substring(tok->str, tok->len);
    node->offset = lvar->offset;
  } else {
    GVar *gvar = find_gvar(tok);
    if (!gvar) {
      error("undefined lvar: %s", substring(tok->str, tok->len));
    }
    node = new_node(ND_GVAR);
    node->type = gvar->type;
    node->ident = substring(tok->str, tok->len);
  }

  if (type_is_array(node->type) && consume("[")) {
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
  VNode *itr = args->head;
  while (itr) {
    Node *node = itr->value;
    size += node->type->size;
    itr = itr->next;
  }
  return size;
}

Node *global() {

  Type *type = expect_type();
  Token *tok = consume_ident();
  if (!tok) {
    error("illegal defined function ident");
  }

  Node *node;
  if (consume("(")) {

    // before parse function statement for recursive call
    Type *func_type = new_function_type(tok->str, tok->len, type);
    vec_add(functions, func_type);

    Node *block;
    Vector *args;

    LVar *tmp_locals = locals;
    locals = NULL;

    args = defined_args();
    block = block_stmt();
    if (!block) {
      error("illegal defined block");
    }

    node = new_node(ND_FUNCTION);
    node->list = args;
    node->ident = substring(tok->str, tok->len);
    node->lhs = block;
    node->val = sizeof_args(args) + sizeof_lvars();
    node->type = func_type;

    // local変数を戻す(サイズ計算後)
    locals = tmp_locals;
    return node;

  } else {

    GVar *gvar = find_gvar(tok);
    if (gvar) {
      error("duplicated defined gvar: %s", substring(tok->str, tok->len));
    }
    gvar = calloc(1, sizeof(GVar));
    gvar->name = tok->str;
    gvar->len = tok->len; // strlen

    int new_size;
    if (consume("[")) {
      int array_len = expect_number();
      expect("]");

      new_size = type->size * array_len;
      type = new_array_type(type, array_len);
    } else {
      new_size = type->size;
    }

    gvar->type = type;

    gvar->next = globals;
    globals = gvar;

    Node *node = new_node(ND_GVAR);
    node->type = gvar->type;
    node->ident = substring(tok->str, tok->len);

    if (consume("=")) {
      node->lhs = equality();
    }
    expect(";");
    return node;
  }
}

void program() {
  init_types();

  int i = 0;
  while (!at_eof()) {
    code[i++] = global();
  }
  code[i] = NULL;
}

Type *find_type(Token *tok) {
  Type *type;
  VNode *itr = types->head;
  while (itr != NULL) {
    type = (Type*) itr->value;
    if (type->len == tok->len && !memcmp(tok->str, type->name, type->len)) {
      return type;
    }
    itr = itr->next;
  }
  return NULL;
}

Type *find_function(Token *tok) {
  Type *type;
  VNode *itr = functions->head;
  while (itr != NULL) {
    type = (Type*) itr->value;
    if (type->len == tok->len && !memcmp(tok->str, type->name, type->len)) {
      return type;
    }
    itr = itr->next;
  }
  return NULL;
}

LVar *find_lvar(Token *tok) {
  for (LVar *var = locals; var; var = var->next) {
    if (var->len == tok->len && !memcmp(tok->str, var->name, var->len)) {
      return var;
    }
  }
  return NULL;
}

GVar *find_gvar(Token *tok) {
  for (GVar *var = globals; var; var = var->next) {
    if (var->len == tok->len && !memcmp(tok->str, var->name, var->len)) {
      return var;
    }
  }
  return NULL;
}

int sizeof_lvars() {
  int size = 0;
  for (LVar *var = locals; var; var = var->next) {
    size += var->type->size;
  }
  return size;
}

