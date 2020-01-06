#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "mcc.h"
#include "vector.h"

Node *new_node(NodeKind kind);
Node *stmt();
Type *consume_type();

// current token
char *user_input;
Node *code[100];
LVar *locals;
Vector *types;  // Type*
Token *token;

Type *new_type(char* name, int len, int size) {
  Type *type;
  type = calloc(1, sizeof(Type));
  type->name = name;
  type->len = len;
  type->size = size;
}

void init_types() {
  types = calloc(1, sizeof(Vector));
  add_last(types, new_type("int", 3, 4));
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
    error("token is '%s' not '%s'", substring(token->str, token->len), op);
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
  Type *type;
  type = find_type(token);
  if (!type) {
    return NULL;
  }
  token = token->next;
  return type;
}

// read reserved integer number
int expect_number() {
  if (token->kind != TK_NUM) {
    error("token is '%s' not number: %d", substring(token->str, token->len), token->kind);
  }
  int val = token->val;
  token = token->next;
  return val;
}

bool at_eof() {
  return token->kind == TK_EOF;
}

Node *new_node(NodeKind kind) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  return node;
}

Node *new_node_lr(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = new_node(kind);
  node->kind = kind;
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

Node *new_node_num(int val) {
  Node *node = new_node(ND_NUM);
  node->val = val;
  return node;
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
      add_last(args, expr());
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
    error("illegal type");
  }

  Type *type;
  type = find_type(tok);
  if (!type) {
    error("undefined type: %s", substring(tok->str, tok->len));
  }
  return type;
}

// defined local variable
LVar *consume_defined_lvar() {
  Type *type = consume_type();
  if (!type) {
    return NULL;
  }

  while (consume("*")) {
    Type *ptr_type = new_type("*", 1, 8);
    ptr_type->ptr_to = type;
    type = ptr_type;
  }

  Token *tok = consume_ident();
  if (!tok) {
    error("illegal lvar name");
  }
  if (find_lvar(tok)) {
    error("duplicated defined lvar %s", substring(tok->str, tok->len));
  }

  LVar *lvar;
  lvar = calloc(1, sizeof(LVar));
  lvar->name = tok->str;
  lvar->len = tok->len;
  if (locals) {
    lvar->offset = locals->offset + 8;
  } else {
    lvar->offset = 8;
  }
  lvar->type = type;

  lvar->next = locals;
  locals = lvar;
  return lvar;
}

Vector *defined_args() {
  Vector *args = NULL;
  if (!consume(")")) {
    args = calloc(1, sizeof(Vector));
    for (;;) {
      LVar *lvar = consume_defined_lvar();
      if (!lvar) {
        error("illegal lvar");
      }
      Node *node = new_node(ND_LVAR);
      node->ident = substring(lvar->name, lvar->len);
      node->offset = lvar->offset;

      add_last(args, node);
      if (consume(")")) {
        break;
      }
      expect(",");
    }
  }
  return args;
}

Node *primary() {
  if (consume("(")) {
    Node *node = expr();
    expect(")");
    return node;
  }

  Token *tok = consume_ident();
  if (tok) {
    if (consume("(")) {
      Node *node = new_node(ND_CALL);
      node->list = consume_args();
      node->ident = substring(tok->str, tok->len);
      return node;
    }

    Node *node = new_node(ND_LVAR);
    
    // for debug
    node->ident = substring(tok->str, tok->len);

    LVar *lvar = find_lvar(tok);
    if (!lvar) {
      error("undefined lvar: %s", substring(tok->str, tok->len));
    }
    node->offset = lvar->offset;
    return node;
  }

  return new_node_num(expect_number());
}

Node *unary() {
  if (consume("-")) {
    return new_node_lr(ND_SUB, new_node_num(0), primary());
  }
  if (consume("+")) {
    return primary();
  }
  if (consume("*")) {
    Node *node = new_node(ND_DEREF);
    node->lhs = unary();
    return node;
  }
  if (consume("&")) {
    Node *node = new_node(ND_ADDR);
    node->lhs = unary();
    return node;
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
      add_last(stmt_list, sub);
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
    LVar *lvar = consume_defined_lvar();
    if (lvar) {
      expect(";");
      Node *node = new_node(ND_LVAR_DECLARED);
      node->offset = lvar->offset;
      return node;
    }

    node = expr();
    expect(";");
  }
  return node;
}

Node *block_stmt() {
  Node *node;
  if (consume("{")) {
    node = new_node(ND_BLOCK);
    Vector *stmt_list = calloc(1, sizeof(Vector));
    do {
      Node *sub = stmt();
      add_last(stmt_list, sub);
    } while (!consume("}"));
    node->list = stmt_list;
  }
  return node;
}

int sizeof_args(Vector *args) {
  return args ? args->size * 1 : 0; // 8byer/lvar
}

int sizeof_block_lvar(Node *block) {
  int sizeof_lvar = 0;
  VNode *itr = block->list->head;
  while (itr) {
    Node *node = itr->value;
    if (node->kind == ND_LVAR_DECLARED) {
      sizeof_lvar += 1;
    }
    itr = itr->next;
  }
  return sizeof_lvar;
}

Node *defined_function() {
  Token *tok;

  Node *node;
  Node *block;
  Type *ret_type;
  Vector *args;

  ret_type = expect_type();

  tok = consume_ident();
  if (!tok) {
    error("illegal defined function ident");
  }

  expect("(");
  LVar *tmp_locals = locals;
  locals = NULL;

  args = defined_args();
  block = block_stmt();
  if (!block) {
    error("illegal defined block");
  }

  // local変数を戻す
  locals = tmp_locals;

  node = new_node(ND_FUNCTION);
  node->list = args;
  node->ident = substring(tok->str, tok->len);
  node->lhs = block;
  node->val = sizeof_args(args) + sizeof_block_lvar(block);
  return node;
}

void program() {

  init_types();

  int i = 0;
  while (!at_eof()) {
    code[i++] = defined_function();
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

LVar *find_lvar(Token *tok) {
  for (LVar *var = locals; var; var = var->next) {
    if (var->len == tok->len && !memcmp(tok->str, var->name, var->len)) {
      return var;
    }
  }
  return NULL;
}
