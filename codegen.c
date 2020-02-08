#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "38cc.h"

int label_id = 0;

bool gen(Node *node);
void gen_num(int num);
void gen_deref(int size);

char *reg64s[] = {
  "rdi", // 0
  "rsi", // 1
  "rdx", // 2
  "rcx", // 3
  "r8",  // 4
  "r9"   // 5
};
char *reg32s[] = {
  "edi", // 0
  "esi", // 1
  "edx", // 2
  "ecx", // 3
  "r8d", // 4
  "r9d"  // 5
};
char *reg16s[] = {
  "di", // 0
  "si", // 1
  "dx", // 2
  "cx", // 3
  "r8w",// 4
  "r9w" // 5
};
char *reg8s[] = {
  "dil", // 0
  "sil", // 1
  "dl",  // 2
  "cl",  // 3
  "r8b", // 4
  "r9b"  // 5
};

void print_header() {
  printf("  .intel_syntax noprefix\n");
  gen_gvars();
  printf("  .global main\n");
}

char* get_args_register(int size, int index) {
  if (index > 5) {
    error("not supported +6 args. if args > 6 then args are stacked.");
  }
  if (size == 1) {
    return reg8s[index];
  } else if (size == 2) {
    return reg16s[index];
  } else if (size == 4) {
    return reg32s[index];
  } else {
    assert(size == 8);
    return reg64s[index];
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
      char *r = get_args_register(8, args);
      printf("  # set arg%d to %s\n", args, r);
      gen((Node *)itr->value);
      printf("  pop %s\n", r);

      itr = itr->next;
      ++args;
    }
  }
  int padding = 0;
  if (padding) {
    printf("  sub rsp, 8\n");
  }
  // reset for return val
  printf("  mov rax, 0\n");
  if (node->val) {
    printf("  call %s@PLT\n", node->ident);
  } else {
    printf("  call %s\n", node->ident);
  }

  if (padding) {
    printf("  add rsp, 8\n");
  }

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

void gen_gvars() {
  for (GVar *var = globals; var; var = var->next) {
    printf("%s:\n", var->name);
    if (*(var->name) == '.') {
      printf("  .string \"%s\"\n", var->str);
      printf("  .text\n");
    } else if (var->type == char_type) {
      printf("  .byte %d\n", var->val);
    } else if (var->type == short_type) {
      printf("  .word %d\n", var->val);
    } else if (var->type == int_type) {
      printf("  .long %d\n", var->val);
    } else {
      assert(var->type == long_type);
      printf("  .quad %d\n", var->val);
    }
  }
}

bool gen_gvar(Node *node) {
  if (node->kind != ND_GVAR) {
    error("node is not gvar");
  }

  // for 64bit
  if (node->type == char_type) {
    printf("  movsx rax, byte ptr %s[rip]\n", node->ident);
  } else if (node->type == short_type) {
    printf("  movsx rax, word ptr %s[rip]\n", node->ident);
  } else if (node->type == int_type) {
    printf("  movsxd rax, dword ptr %s[rip]\n", node->ident);
  } else if (node->type == long_type) {
    printf("  mov rax, %s[rip]\n", node->ident);
  } else if (node->type == str_type) {
    printf("  lea rax, %s[rip]\n", node->ident);
  }
  printf("  push rax\n");
  return true;
}

void gen_defined_function(Node *node) {
  if (node->kind != ND_FUNCTION) {
    error("node is not function");
  }
  // function label
  printf("%s:\n", node->ident);

  printf("  # prologue by %s function\n", node->ident);
  printf("  push rbp\n");
  printf("  mov rbp, rsp\n");

  if (node->val > 0) {
    // allocate local vars
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
      int size = arg->type->size;
      char *r = get_args_register(size, index);
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
    error("node is none. by gen_defined");
  }

  if (node->kind == ND_FUNCTION) {
    gen_defined_function(node);
  } else if (node->kind == ND_GVAR) {
    // gen gvar at gen_gvars
  } else {
    error("node is not supported.");
  }
}

void gen_deref(int size) {
  printf("  pop rax\n");

  if (size == 1) {
    printf("  movsx rax, byte ptr [rax]\n");
  } else if (size == 2) {
    printf("  movsx rax, word ptr [rax]\n");
  } else if (size == 4) {
    printf("  movsxd rax, dword ptr [rax]\n");
  } else {
    assert(size == 8);
    printf("  mov rax, [rax]\n");
  }
  printf("  push rax\n");
}

// push store address
void gen_lval(Node *node) {
  printf("  # gen_lval\n");
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

int max_size(Node* lhs, Node* rhs) {
  int lsize = lhs->type->size;
  int rsize = rhs->type->size;
  if (lsize >= rsize) {
    return lsize;
  } else {
    return rsize;
  }
}

bool gen(Node *node) {
  if (!node) {
    error("node is none. by gen");
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
    if (!type_is_array(node->type)) {
      gen_deref(node->type->size);
    }
    return true;
  }
  if (node->kind == ND_GVAR) {
    return gen_gvar(node);
  }
  if (node->kind == ND_ASSIGN) {
    gen_lval(node->lhs);

    printf("  # assign rhs\n");
    gen(node->rhs);

    printf("  # assign\n");
    printf("  pop rdi\n");
    printf("  pop rax\n");

    int size = node->type->size;
    if (size == 1) {
      printf("  mov [rax], dil\n");
    } else if (size == 2) {
      printf("  mov [rax], di\n");
    } else if (size == 4) {
      printf("  mov [rax], edi\n");
    } else {
      assert(size == 8);
      printf("  mov [rax], rdi\n");
    }
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
    gen(node->lhs);
    gen_deref(node->lhs->type->ptr_to->size);
    return true;
  }

  bool lhs_is_ptr = type_is_ptr(node->lhs->type) || type_is_array(node->lhs->type);
  bool rhs_is_ptr = type_is_ptr(node->rhs->type) || type_is_array(node->rhs->type);

  printf("  # lhs\n");
  gen(node->lhs);
  if (!lhs_is_ptr && rhs_is_ptr) {
    // for pointer calculation
    printf("  pop rax\n");
    printf("  imul rax, %d\n", 8); // sizeof_node(node->rhs));
    printf("  push rax\n");
  }

  printf("  # rhs\n");
  gen(node->rhs);
  if (lhs_is_ptr && !rhs_is_ptr) {
    // for pointer calculation
    printf("  pop rax\n");
    printf("  imul rax, %d\n", 8); // sizeof_node(node->lhs));
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
    printf("  sete al\n");  // al: under 8bit of rax
    printf("  movzb rax, al\n");
  } else if (node->kind == ND_NE) {
    printf("  # ne(!=)\n");
    printf("  cmp rax, rdi\n");
    printf("  setne al\n");  // al: under 8bit of rax
    printf("  movzb rax, al\n");
  } else if (node->kind == ND_LE) {
    printf("  # le(>=)\n");
    printf("  cmp rax, rdi\n");
    printf("  setle al\n");  // al: under 8bit of rax
    printf("  movzb rax, al\n");
  } else if (node->kind == ND_LT) {
    printf("  # lt(>)\n");
    printf("  cmp rax, rdi\n");
    printf("  setl al\n");  // al: under 8bit of rax
    printf("  movzb rax, al\n");
  } else {
    error("failed to calc");
  }
  printf("  push rax\n");
  return true;
}

