#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
  ND_ADD,  // +
  ND_SUB,  // -
  ND_MUL,  // *
  ND_DIV,  // /
  ND_MOD,  // %
  ND_NUM,  // integer number
} NodeKind;

typedef enum {
  TK_RESERVED,  // simbol +, -
  TK_NUM,       // integer number
  TK_EOF,       // Eno of File
} TokenKind;

typedef struct Node Node;
typedef struct Token Token;

struct Token {
  TokenKind kind;
  Token *next;
  int val;
  char *str;
};

struct Node {
  NodeKind kind;
  Node *lhs;  // left-hand side
  Node *rhs;  // right-hand side
  int val;    // only use if kind is ND_NUM
};

void print_header();
Node *expr();
Node *mul();

// current token
char *user_input;
Token *token;

void error_at(char *loc, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  int pos = loc - user_input;
  fprintf(stderr, "%s\n", user_input);
  fprintf(stderr, "%*s", pos, "");
  fprintf(stderr, "^");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

bool consume(char op) {
  if (token->kind != TK_RESERVED || token->str[0] != op) {
    return false;
  }
  token = token->next;
  return true;
}

// read reserved symbol
void expect(char op) {
  if (token->kind != TK_RESERVED || token->str[0] != op) {
    error("token is not '%c'", op);
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

Token *new_token(TokenKind kind, Token *cur, char *str) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  cur->next = tok;
  return tok;
}

Token *tokenize(char *p) {
  Token head;
  head.next = NULL;
  Token *cur = &head;

  int cnt = *p;

  while (*p) {
    if (isspace(*p)) {
      p++;
      continue;
    }

    if (*p == '(' || *p == ')') {
      cur = new_token(TK_RESERVED, cur, p++);
      continue;
    }
    if (*p == '+' || *p == '-') {
      cur = new_token(TK_RESERVED, cur, p++);
      continue;
    }
    if (*p == '*' || *p == '/') {
      cur = new_token(TK_RESERVED, cur, p++);
      continue;
    }
    if (*p == '%') {
      cur = new_token(TK_RESERVED, cur, p++);
      continue;
    }

    if (isdigit(*p)) {
      cur = new_token(TK_NUM, cur, p);
      cur->val = strtol(p, &p, 10);
      continue;
    }

    error_at(p, "unexpected token");
  }
  new_token(TK_EOF, cur, p);
  return head.next;
}

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
  if (consume('(')) {
    Node *node = expr();
    expect(')');
    return node;
  }
  return new_node_num(expect_number());
}

Node *unary() {
  if (consume('-')) {
    return new_node(ND_SUB, new_node_num(0), primary());
  }
  if (consume('+')) {
    return primary();
  }
  return primary();
}

Node *mul() {
  Node *node = unary();
  for (;;) {
    if (consume('*')) {
      node = new_node(ND_MUL, node, unary());
    } else if (consume('/')) {
      node = new_node(ND_DIV, node, unary());
    } else if (consume('%')) {
      node = new_node(ND_MOD, node, unary());
    } else {
      return node;
    }
  }
  return node;
}

Node *expr() {
  Node *node = mul();
  for (;;) {
    if (consume('+')) {
      node = new_node(ND_ADD, node, mul());
    } else if (consume('-')) {
      node = new_node(ND_SUB, node, mul());
    } else {
      return node;
    }
  }
}

void print_header() {
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");
}

void gen(Node *node) {
  if (node->kind == ND_NUM) {
    printf("  push %d\n", node->val);
    return;
  }

  gen(node->lhs);
  gen(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");
  if (node->kind == ND_ADD) {
    printf("  add rax, rdi\n");
  } else if (node->kind == ND_SUB) {
    printf("  sub rax, rdi\n");
  } else if (node->kind == ND_MUL) {
    printf("  imul rax, rdi\n");
  } else if (node->kind == ND_DIV) {
    printf("  cqo\n");
    printf("  idiv rdi\n");  // divide by (rdx << 64 | rax) / rdi => rax, rdx
  } else if (node->kind == ND_MOD) {
    printf("  cqo\n");
    printf("  idiv rdi\n");  // divide by (rdx << 64 | rax) / rdi => rax, rdx
    printf("  mov rax, rdx\n");
  } else {
    error("failed to calc");
  }
  printf("  push rax\n");
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Illegal num of argmuments.\n");
    return 1;
  }

  user_input = argv[1];
  token = tokenize(user_input);

  print_header();

  Node *node = expr();
  gen(node);

  printf("  pop rax\n");
  printf("  ret\n");
  return 0;
}
