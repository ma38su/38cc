#include <stdio.h>
#include "38cc.h"

extern int label_id;
extern int current_lid;

bool gen(Node *node);
void gen_to_stack(Node *node);

long sizeof_type(Type *type);
void gen_addr(Node *node);
void gen_gvars_uninit();
void gen_gvar_declarations();
void gen_defined(Node *code);
void gen_deref_type(Type *type);
void gen_defined_function(Node *node);
void gen_function_call(Node *node);
void gen_if(Node *node);
void gen_while(Node *node);
void gen_for(Node *node);
void gen_do_while(Node *node);
void gen_block(Node *node);
void gen_for(Node *node);


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


char* args_register(int size, int index) {
  if (index > 5) {
    error("not supported +6 args. if args > 6 then args are stacked.");
  }
  if (size == 1) {
    return reg8s[index];
  } else if (size == 2) {
    return reg16s[index];
  } else if (size == 4) {
    return reg32s[index];
  } else if (size == 8) {
    return reg64s[index];
  } else {
    error("illegal arg size: %d", size);
    return NULL;
  }
}

void gen_to_stack(Node *node) {
  if (!gen(node)) error("illegal gen");
}


// cast
void truncate(Type *type) {

  type = raw_type(type);
  printf("  pop rax\n");
  if (type == bool_type) {
    printf("  cmp rax, 0\n");
    printf("  setne al\n");
    printf("  movzb rax, al\n");
  }

  long size = sizeof_type(type);
  if (type->kind == TY_PRM) {
    if (type->is_unsigned) {
      if (size == 1) {
        printf("  movzx rax, al\n");
      } else if (size == 2) {
        printf("  movzx rax, ax\n");
      } else if (size == 4) {
        printf("  mov eax, eax\n");
      }
    } else {
      if (size == 1) {
        printf("  movsx rax, al\n");
      } else if (size == 2) {
        printf("  movsx rax, ax\n");
      } else if (size == 4) {
        printf("  movsxd rax, eax\n");
      }
    }
  }
  printf("  push rax\n");
}

void gen_num(long num) {
  printf("  push %ld  # num %ld\n", num, num);
}

void gen_addr(Node* node) {
  if (node->kind == ND_DEREF) {
    gen_to_stack(node->lhs);
    return;
  }
  if (node->offset != 0) {
    printf("  mov rax, rbp\n");
    printf("  sub rax, %ld\n", node->offset);
    printf("  push rax\n");
  } else {
    printf("  push rbp\n");
  }
}

void gen_ternary(Node *node) {
  int lid = label_id++;
  printf("  # TERNAY (?:)\n");
  gen_to_stack(node->cnd);
  printf("  pop rax\n");
  printf("  cmp rax, 0\n");
  printf("  je  .L.false.%d\n", lid);
  gen_to_stack(node->thn);
  printf("  jmp .L.end.%d\n", lid);
  printf(".L.false.%d:\n", lid);
  gen_to_stack(node->els);
  printf(".L.end.%d:\n", lid);
}

void gen_if(Node *node) {
  if (node->kind != ND_IF) error("not if");

  gen_to_stack(node->cnd);
  printf("  pop rax # if\n");
  printf("  cmp rax, 0\n");
  int lid = label_id++;
  if (!node->els) {
    printf("  je  .L.end.%d\n", lid);
    if (gen(node->thn)) {
      printf("  pop rax # skip\n");
    }
    printf(".L.end.%d:\n", lid);
  } else {
    printf("  je  .L.else.%d\n", lid);
    if (gen(node->thn)) {
      printf("  pop rax # skip\n");
    }
    printf("  jmp .L.end.%d\n", lid);
    printf(".L.else.%d:\n", lid);
    if (gen(node->els)) {
      printf("  pop rax # skip\n");
    }
    printf(".L.end.%d:\n", lid);
  }
}

void gen_while(Node *node) {
  if (node->kind != ND_WHILE) {
    error("not while");
  }

  int prev_lid = current_lid;
  int lid = current_lid = label_id++;
  printf(".L.continue.%d:\n", lid);
  gen_to_stack(node->cnd);
  printf("  pop rax # while\n");
  printf("  cmp rax, 0\n");
  printf("  je  .L.end.%d\n", lid);
  if (gen(node->thn)) {
    printf("  pop rax # skip\n");
  }
  printf("  jmp .L.continue.%d\n", lid);
  printf(".L.end.%d:\n", lid);
  current_lid = prev_lid;
}

void gen_do_while(Node *node) {
  if (node->kind != ND_DO) {
    error("not do while");
  }

  int prev_lid = current_lid;
  int lid = current_lid = label_id++;
  printf(".L.begin.%d: # do while\n", lid);
  if (gen(node->thn)) {
    printf("  pop rax # skip\n");
  }
  printf(".L.continue.%d: # do while\n", lid);
  gen_to_stack(node->cnd);
  printf("  pop rax\n");
  printf("  cmp rax, 0\n");
  printf("  je  .L.end.%d\n", lid);
  printf("  jmp .L.begin.%d\n", lid);
  printf(".L.end.%d:\n", lid);
  current_lid = prev_lid;
}

void gen_for(Node *node) {
  if (node->kind != ND_FOR) {
    error("not for");
  }
  if (node->ini && gen(node->ini)) {
    printf("  pop rax # skip\n");
  }

  int prev_lid = current_lid;
  int lid = current_lid = label_id++;
  printf(".L.begin.%d:\n", lid);
  gen_to_stack(node->cnd);
  printf("  pop rax # while\n");
  printf("  cmp rax, 0\n");
  printf("  je  .L.end.%d\n", lid);
  if (gen(node->thn)) {
    printf("  pop rax # skip\n");
  }
  printf(".L.continue.%d:\n", lid);
  if (node->stp && gen(node->stp)) {
    printf("  pop rax # skip\n");
  }
  printf("  jmp .L.begin.%d\n", lid);
  printf(".L.end.%d:\n", lid);
  current_lid = prev_lid;
}

void gen_return(Node *node) {
  if (node->kind != ND_RETURN) {
    error("not return");
  }
  if (node->lhs) {
    // set return value to rax
    gen_to_stack(node->lhs);
    printf("  pop rax\n");
  } else {
    printf("  mov rax, 0 # no return\n");
  }
  printf("  mov rsp, rbp  # epilogue\n");
  printf("  pop rbp\n");
  printf("  ret           # return rax value\n");
}
