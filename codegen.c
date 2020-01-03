#include <stdio.h>
#include "mcc.h"

void print_header() {
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
}

void gen_lval(Node *node) {
  if (node->kind != ND_LVAR) {
    error("left side hand is not variable");
  }
  printf("  mov rax, rbp\n");
  printf("  sub rax, %d\n", node->offset);
  printf("  push rax\n");
}

void gen_function_call(Node *node) {
  if (node->kind != ND_CALL) {
    error("not function call");
  }
  if (node->list) {
    VNode *itr = node->list->head;
    int args = 0;
    while (itr != NULL) {
      gen((Node *)itr->value);

      char *r = NULL;
      if (args == 0) {
        r = "rdi";
      } else if (args == 1) {
        r = "rsi";
      } else if (args == 2) {
        r = "rdx";
      } else if (args == 3) {
        r = "rcx";
      } else if (args == 4) {
        r = "r8";
      } else if (args == 5) {
        r = "r9";
      }
      if (r) {
        printf("  pop %s\n", r);
      }

      itr = itr->next;
      ++args;
    }
  }
  printf("  call %s\n", node->ident);
  printf("  push rax\n");
}

void gen_block(Node *node) {
  if (node->kind != ND_BLOCK) {
    error("not block");
  }

  VNode *itr = node->list->head;
  while (itr != NULL) {
    Node *node = (Node *)itr->value;
    gen(node);
    itr = itr->next;
  }
}

void gen_while(Node *node) {
  if (node->kind != ND_WHILE) {
    error("not while");
  }
  printf(".LbeginXXX:\n");
  gen(node->lhs);
  printf("  pop rax # while\n");
  printf("  cmp rax, 0\n");
  printf("  je  .LendXXX\n");
  gen(node->rhs);
  printf("  jmp .LbeginXXX\n");
  printf(".LendXXX:\n");
}

void gen_for(Node *node) {
  if (node->kind != ND_FOR) {
    error("not while");
  }
  if (node->lhs) {
    gen(node->lhs);
  }
  gen_while(node->rhs);
}

void gen_if(Node *node) {
  if (node->kind != ND_IF) {
    error("not if");
  }
  gen(node->lhs);
  printf("  pop rax # if\n");
  printf("  cmp rax, 0\n");
  if (node->rhs->kind != ND_ELSE) {
    printf("  je  .LendXXX\n");
    gen(node->rhs);
    printf(".LendXXX:\n");
  } else {
    printf("  je  .LelseXXX\n");
    gen(node->rhs->lhs);
    printf("  jmp .LendXXX\n");
    printf(".LelseXXX:\n");
    gen(node->rhs->rhs);
    printf(".LendXXX:\n");
  }
}

void gen_return(Node *node) {
  if (node->kind != ND_RETURN) {
    error("not return");
  }
  gen(node->lhs);

  // stack to rax
  printf("  pop rax # return\n");
  printf("  mov rsp, rbp\n");
  printf("  pop rbp\n");

  // return rax value
  printf("  ret\n");
}

void gen_defined(Node *node) {
  if (!node) {
    error("node is none");
  }
  if (node->kind == ND_FUNCTION) {
    // function label
    printf("%s:\n", node->ident);

    // allocate memory for 26 variables
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    //printf("  sub rsp, 0\n");

    gen_block(node->lhs);

    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");

    // return rax value
    printf("  ret\n");

    return;
  }
}

void gen(Node *node) {
  if (!node) {
    error("node is none");
  }
  if (node->kind == ND_NONE) {
    if (node->lhs) {
      gen(node->lhs);
    }
    if (node->rhs) {
      gen(node->rhs);
    }
    return;
  }
  if (node->kind == ND_IF) {
    gen_if(node);
    return;
  }
  if (node->kind == ND_FOR) {
    gen_for(node);
    return;
  }
  if (node->kind == ND_WHILE) {
    gen_while(node);
    return;
  }
  if (node->kind == ND_RETURN) {
    gen_return(node);
    return;
  }
  if (node->kind == ND_NUM) {
    printf("  push %d\n", node->val);
    return;
  }
  if (node->kind == ND_LVAR) {
    gen_lval(node);
    printf("  pop rax # lvar\n");
    printf("  mov rax, [rax]\n");
    printf("  push rax\n");
    return;
  }
  if (node->kind == ND_ASSIGN) {
    gen_lval(node->lhs);
    gen(node->rhs);
    printf("  pop rdi\n");
    printf("  pop rax # assign\n");
    printf("  mov [rax], rdi\n");
    printf("  push rdi\n");
    return;
  }
  if (node->kind == ND_BLOCK) {
    gen_block(node);
    return;
  }
  if (node->kind == ND_CALL) {
    gen_function_call(node);
    return;
  }

  gen(node->lhs);
  gen(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax # gen\n");
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
  } else if (node->kind == ND_EQ) {
    printf("  cmp rax, rdi\n");
    printf("  sete al\n");  // raxの下8bit
    printf("  movzb rax, al\n");
  } else if (node->kind == ND_NE) {
    printf("  cmp rax, rdi\n");
    printf("  setne al\n");  // raxの下8bit
    printf("  movzb rax, al\n");
  } else if (node->kind == ND_LE) {
    printf("  cmp rax, rdi\n");
    printf("  setle al\n");  // raxの下8bit
    printf("  movzb rax, al\n");
  } else if (node->kind == ND_LT) {
    printf("  cmp rax, rdi\n");
    printf("  setl al\n");  // raxの下8bit
    printf("  movzb rax, al\n");
  } else {
    error("failed to calc");
  }
  printf("  push rax\n");
}
