#include <stdio.h>
#include <stdbool.h>
#include "mcc.h"

int label_id = 0;

bool gen(Node *node);
void gen_num(int num);
void gen_deref();

void print_header() {
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
}

char* get_args_register(int args) {
  if (args == 0) {
    return "rdi";
  } else if (args == 1) {
    return "rsi";
  } else if (args == 2) {
    return "rdx";
  } else if (args == 3) {
    return "rcx";
  } else if (args == 4) {
    return "r8";
  } else if (args == 5) {
    return "r9";
  } else {
    error("not supported +6 args. if args > 6 then args are stacked.");
  }
}

void gen_addr(Node* node) {
  if (node->kind != ND_LVAR) {
    error("not lvar: %d", node->kind);
  }
  printf("  # &%s\n", node->ident);
  printf("  mov rax, rbp\n");
  printf("  sub rax, %d\n", node->offset);
  printf("  push rax\n");
}

void gen_function_call(Node *node) {
  if (node->kind != ND_CALL) {
    error("not function call");
  }

  if (node->list) {

    printf("  # set args %d to call %s\n",
        node->list->size, node->ident);

    VNode *itr = node->list->head;
    int args = 0;
    while (itr != NULL) {
      char *r = get_args_register(args);
      printf("  # set arg%d to %s\n", args, r);
      gen((Node *)itr->value);
      printf("  pop %s\n", r);

      itr = itr->next;
      ++args;
    }
  }
  printf("  call %s\n", node->ident);

  // after called, return value is stored rax
  printf("  push rax\n");
}

void gen_block(Node *node) {
  if (node->kind != ND_BLOCK) {
    error("not block");
  }

  printf("  # block begin\n\n");
  VNode *itr = node->list->head;
  Node *n;
  while (itr != NULL) {
    n = (Node *)itr->value;
    if (gen(n)) {
      printf("  pop rax\n");
      printf("  # end line\n\n");
    }
    itr = itr->next;
  }
  printf("\n");
  printf("  # block end\n");
}

bool gen_while(Node *node) {
  if (node->kind != ND_WHILE) {
    error("not while");
  }
  printf(".Lbegin%03d:\n", label_id);
  gen(node->lhs);
  printf("  pop rax # while\n");
  printf("  cmp rax, 0\n");
  printf("  je  .Lend%03d\n", label_id);
  gen(node->rhs);
  printf("  jmp .Lbegin%03d\n", label_id);
  printf(".Lend%03d:\n", label_id);
  label_id++;
  return false;
}

bool gen_for(Node *node) {
  if (node->kind != ND_FOR) {
    error("not while");
  }
  if (node->lhs) {
    gen(node->lhs);
  }
  return gen_while(node->rhs);
}

bool gen_if(Node *node) {
  if (node->kind != ND_IF) {
    error("not if");
  }
  gen(node->lhs);
  printf("  pop rax # if\n");
  printf("  cmp rax, 0\n");
  if (node->rhs->kind != ND_ELSE) {
    printf("  je  .Lend%03d\n", label_id);
    gen(node->rhs);
    printf(".Lend%03d:\n", label_id);
  } else {
    printf("  je  .Lelse%03d\n", label_id);
    gen(node->rhs->lhs);
    printf("  jmp .Lend%03d\n", label_id);
    printf(".Lelse%03d:\n", label_id);
    gen(node->rhs->rhs);
    printf(".Lend%03d:\n", label_id);
  }
  label_id++;
  return false;
}

bool gen_return(Node *node) {
  if (node->kind != ND_RETURN) {
    error("not return");
  }
  gen(node->lhs);
  printf("  pop rax\n");

  printf("  # epilogue by return\n");
  printf("  mov rsp, rbp\n");
  printf("  pop rbp\n");
  printf("  ret\n");  // return rax value
  return false;
}

void gen_defined_function(Node *node) {
  if (node->kind != ND_FUNCTION) {
    error("node is not function");
  }
  // function label
  printf("%s:\n", node->ident);

  // allocate memory for 26 variables
  printf("  # prologue by %s function\n", node->ident);
  printf("  push rbp\n");
  printf("  mov rbp, rsp\n");

  if (node->val > 0) {
    printf("  sub rsp, %d # args+lvar\n",
        node->val);
  }

  // extract args
  if (node->list) {
    int index = node->list->size;

    VNode *itr = node->list->tail;
    while (itr != NULL) {
      Node *arg = (Node *) itr->value;
      printf("  # extract arg \"%s\"\n", arg->ident);
      gen_addr(arg);
      printf("  pop rax\n");

      --index;
      char *r = get_args_register(index);
      printf("  mov [rax], %s\n", r);
      itr = itr->prev;
    }
  }

  gen_block(node->lhs);

  printf("  # epilogue by %s function\n", node->ident);
  printf("  mov rsp, rbp\n");
  printf("  pop rbp\n");
  printf("  ret\n");  // return rax value
}

void gen_defined(Node *node) {
  if (!node) {
    error("node is none");
  }

  if (node->kind != ND_FUNCTION) {
    error("node is not function");
  }

  gen_defined_function(node);
}

void gen_deref() {
  printf("  pop rax\n");
  printf("  mov rax, [rax]\n");
  printf("  push rax\n");
}

// push store address
void gen_lval(Node *node) {
  if (node->kind == ND_DEREF) {
    gen(node->lhs);
  } else {
    gen_addr(node);
  }
}

void gen_num(int num) {
  printf("  # num %d\n", num);
  printf("  push %d\n", num);
}

bool gen(Node *node) {
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
    return true;
  }

  if (node->kind == ND_IF) {
    return gen_if(node);
  }
  if (node->kind == ND_FOR) {
    return gen_for(node);
  }
  if (node->kind == ND_WHILE) {
    return gen_while(node);
  }
  if (node->kind == ND_RETURN) {
    return gen_return(node);
  }

  if (node->kind == ND_NUM) {
    gen_num(node->val);
    return true;
  }
  if (node->kind == ND_LVAR) {
    gen_addr(node);
    gen_deref();
    return true;
  }
  if (node->kind == ND_ASSIGN) {
    gen_lval(node->lhs);

    printf("  # assign rhs\n");
    gen(node->rhs);

    printf("  # assign\n");
    printf("  pop rdi\n");
    printf("  pop rax\n");
    printf("  mov [rax], rdi\n");
    printf("  push rdi\n");
    return true;
  }
  if (node->kind == ND_BLOCK) {
    gen_block(node);
    return false;
  }
  if (node->kind == ND_CALL) {
    gen_function_call(node);
    return true;
  }
  if (node->kind == ND_ADDR) {
    gen_addr(node->lhs);
    return true;
  }
  if (node->kind == ND_DEREF) {
    //printf("  # deref\n"); //*%s\n", node->lhs->ident);
    gen(node->lhs);
    gen_deref();
    return true;
  }

  int lhs_is_ptr = 0;//(node->lhs->kind == ND_PTR || node->lhs->kind == ND_ARRAY);
  int rhs_is_ptr = 0;//(node->rhs->kind == ND_PTR || node->rhs->kind == ND_ARRAY);

  printf("  # lhs\n");
  gen(node->lhs);
  if (!lhs_is_ptr && rhs_is_ptr) {
    printf("  pop rax\n");
    printf("  imul rax, 8\n");
    printf("  push rax\n");
  }
  printf("  # rhs\n");
  gen(node->rhs);
  if (lhs_is_ptr && !rhs_is_ptr) {
    printf("  pop rax\n");
    printf("  imul rax, 8\n");
    printf("  push rax\n");
  }
  printf("  pop rdi\n");
  printf("  pop rax\n");
  if (node->kind == ND_ADD) {
    printf("  # add(+)\n");
    printf("  add rax, rdi\n");
  } else if (node->kind == ND_SUB) {
    printf("  # sub(-)\n");
    printf("  sub rax, rdi\n");
  } else if (node->kind == ND_MUL) {
    printf("  # mul(*)\n");
    printf("  imul rax, rdi\n");
  } else if (node->kind == ND_DIV) {
    printf("  # div(/)\n");
    printf("  cqo\n");
    printf("  idiv rdi\n");  // divide by (rdx << 64 | rax) / rdi => rax, rdx
  } else if (node->kind == ND_MOD) {
    printf("  # mod(%%)\n");
    printf("  cqo\n");
    printf("  idiv rdi\n");  // divide by (rdx << 64 | rax) / rdi => rax, rdx
    printf("  mov rax, rdx\n");
  } else if (node->kind == ND_EQ) {
    printf("  # eq(==)\n");
    printf("  cmp rax, rdi\n");
    printf("  sete al\n");  // raxの下8bit
    printf("  movzb rax, al\n");
  } else if (node->kind == ND_NE) {
    printf("  # ne(!=)\n");
    printf("  cmp rax, rdi\n");
    printf("  setne al\n");  // raxの下8bit
    printf("  movzb rax, al\n");
  } else if (node->kind == ND_LE) {
    printf("  # le(>=)\n");
    printf("  cmp rax, rdi\n");
    printf("  setle al\n");  // raxの下8bit
    printf("  movzb rax, al\n");
  } else if (node->kind == ND_LT) {
    printf("  # lt(>)\n");
    printf("  cmp rax, rdi\n");
    printf("  setl al\n");  // raxの下8bit
    printf("  movzb rax, al\n");
  } else {
    error("failed to calc");
  }
  printf("  push rax\n");
  return true;
}
