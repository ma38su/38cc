#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "mcc.h"

// current token
char *user_input;
Token *token;
Node *code[100];
LVar *locals;

Token *new_token(TokenKind kind, Token *cur, char *str) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  cur->next = tok;
  return tok;
}

int is_alnum(char c) {
  return ('a' <= c && c <= 'z')
      || ('A' <= c && c <= 'Z')
      || ('0' <= c && c <= '9')
      || (c == '_');
}

int lvar_len(char *p0) {
  char *p;
  p = p0;
  if (('a' <= *p && *p <= 'z')
    || ('A' <= *p && *p <= 'Z')) {
    while (is_alnum(*p)) {
      p++;
    }
  }
  return p - p0;
}

void tokenize(char *p) {
  Token head;
  head.next = NULL;
  Token *cur = &head;

  int cnt = *p;

  while (*p) {
    if (isspace(*p)) {
      p++;
      continue;
    }

    if (memcmp(p, "==", 2) == 0 || memcmp(p, "!=", 2) == 0 
     || memcmp(p, "<=", 2) == 0 || memcmp(p, ">=", 2) == 0) {
      cur = new_token(TK_RESERVED, cur, p);
      cur->len = 2;
      p += 2;
      continue;
    }

    if (strchr("=()<>+-*/%^;", *p)) {
      cur = new_token(TK_RESERVED, cur, p++);
      cur->len = 1;
      continue;
    }

    if (isdigit(*p)) {
      cur = new_token(TK_NUM, cur, p);
      cur->val = strtol(p, &p, 10);
      continue;
    }

    int l = lvar_len(p);
    if (l > 0) {
      if (l == 6 && memcmp(p, "return", l) == 0) {
        cur = new_token(TK_RETURN, cur, p);
      } else {
        cur = new_token(TK_IDENT, cur, p);
      }
      p += l;
      cur->len = l;

      continue;
    }

    error_at(p, "unexpected token");
  }
  new_token(TK_EOF, cur, p);
  
  token = head.next;
}

bool consume(char *op) {
  if (token->kind != TK_RESERVED || strlen(op) != token->len || memcmp(op, token->str, token->len) != 0) {
    return false;
  }
  token = token->next;
  return true;
}

bool consume_return() {
  if (token->kind != TK_RETURN) {
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

// read reserved symbol
void expect(char* op) {
  if (token->kind != TK_RESERVED || strlen(op) != token->len || memcmp(op, token->str, token->len) != 0) {
    Token *t;
    for (t = token; t; t = t->next) {
      if (t->kind == TK_RETURN) {
        printf("  token: %s = return\n", t->str);
      } else {
        printf("  token: %s\n", t->str);
      }
    }
    error("token is '%s' not '%s'", token->str, op);
  }
  token = token->next;
}

// read reserved integer number
int expect_number() {
  if (token->kind != TK_NUM) {
    error("not number");
  }
  int val = token->val;
  token = token->next;
  return val;
}

bool at_eof() { return token->kind == TK_EOF; }

Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

Node *new_node_num(int val) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_NUM;
  node->val = val;
  return node;
}

Node *primary() {
  if (consume("(")) {
    Node *node = expr();
    expect(")");
    return node;
  }

  Token *tok = consume_ident();
  if (tok) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_LVAR;
    
    LVar *lvar = find_lvar(tok);
    if (lvar) {
      node->offset = lvar->offset;
    } else {
      lvar = calloc(1, sizeof(LVar));
      lvar->name = tok->str;
      lvar->len = tok->len;
      if (locals) {
        lvar->offset = locals->offset + 8;
      } else {
        lvar->offset = 8;
      }
      node->offset = lvar->offset;
      lvar->next = locals;
      locals = lvar;
    }
    return node;
  }

  return new_node_num(expect_number());
}

Node *unary() {
  if (consume("-")) {
    return new_node(ND_SUB, new_node_num(0), primary());
  }
  if (consume("+")) {
    return primary();
  }
  return primary();
}

Node *mul() {
  Node *node = unary();
  for (;;) {
    if (consume("*")) {
      node = new_node(ND_MUL, node, unary());
    } else if (consume("/")) {
      node = new_node(ND_DIV, node, unary());
    } else if (consume("%")) {
      node = new_node(ND_MOD, node, unary());
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
      node = new_node(ND_ADD, node, mul());
    } else if (consume("-")) {
      node = new_node(ND_SUB, node, mul());
    } else {
      return node;
    }
  }
}

Node *relational() {
  Node *node = add();
  for (;;) {
    if (consume("<")) {
      node = new_node(ND_LT, node, add());
    } else if (consume("<=")) {
      node = new_node(ND_LE, node, add());
    } else if (consume(">=")) {
      node = new_node(ND_LE, add(), node);
    } else if (consume(">")) {
      node = new_node(ND_LT, add(), node);
    } else {
      return node;
    }
  }
}

Node *equality() {
  Node *node = relational();
  for (;;) {
    if (consume("==")) {
      node = new_node(ND_EQ, node, relational());
    } else if (consume("!=")) {
      node = new_node(ND_NE, node, relational());
    } else {
      return node;
    }
  }
}

Node *assign() {
  Node *node = equality();
  if (consume("=")) {
    node = new_node(ND_ASSIGN, node, assign());
  }
  return node;
}

Node *expr() {
  return assign();
}

Node *stmt() {
  Node *node;
  if (consume_return()) {
    node = calloc(1, sizeof(Node));
    node->kind = ND_RETURN;
    node->lhs = expr();
  } else {
    node = expr();
  }
  expect(";");
  return node;
}


void program() {
  int i = 0;
  while (!at_eof()) {
    code[i++] = stmt();
  }
  code[i] = NULL;
}

LVar *find_lvar(Token *tok) {
  for (LVar *var = locals; var; var = var->next) {
    if (var->len == tok->len && !memcmp(tok->str, var->name, var->len)) {
      return var;
    }
  }
  return NULL;
}
