#include <stdio.h>
#include <stdbool.h>
#include "38cc.h"

extern int label_id;
extern int current_lid;
bool gen(Node *node);
int sizeof_type(Type *type);
void gen_addr(Node *node);
void gen_gvars_uninit();
void gen_gvars();
void gen_defined(Node *code);
void gen_deref_type(Type *type);
void gen_defined_function(Node *node);
void gen_function_call(Node *node);
bool gen_if(Node *node);
bool gen_while(Node *node);
bool gen_for(Node *node);
bool gen_do_while(Node *node);
bool gen_block(Node *node);

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
  } else if (size == 8) {
    return reg64s[index];
  } else {
    error("illegal arg size: %d", size);
    return NULL;
  }
}

// push store address
void gen_lval(Node *node) {
  if (node->kind == ND_DEREF) {
    gen(node->lhs);
  } else {
    gen_addr(node);
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
  } else if (type_is_array(node->type)) {
    if (node->type->to == char_type) {
      printf("  lea rax, %s[rip]\n", node->ident);
    } else {
      printf("  lea rax, %s[rip]\n", node->ident);
    }
  //} else if (type_is_ptr(node->type) && node->type->to == char_type) {
  } else if (type_is_ptr(node->type)) {
    printf("  mov rax, qword ptr %s[rip]\n", node->ident);
  } else {
    printf("  # not support gvar %s, type: %s\n", node->ident, node->type->name);
  }
  printf("  push rax\n");
  return true;
}


bool gen_return(Node *node) {
  if (node->kind != ND_RETURN) {
    error("not return");
  }
  if (node->lhs) {
    // set return value to rax
    gen(node->lhs);
    printf("  pop rax\n");
  }
  printf("  mov rsp, rbp  # epilogue\n");
  printf("  pop rbp\n");
  printf("  ret           # return rax value\n");
  return false;
}

void gen_defined(Node *node) {
  if (!node) {
    error("node is none by gen_defined");
  }

  if (node->kind == ND_FUNCTION) {
    gen_defined_function(node);
  } else if (node->kind == ND_GVAR) {
    // gen gvar at gen_gvars
  } else {
    error("node is not supported.");
  }
}

void gen_num(int num) {
  printf("  push %d  # num %d\n", num, num);
}

// cast
void truncate(Type *type) {
  if (type->kind == TY_TYPEDEF || type->kind == TY_ENUM) {
    truncate(type->to);
    return;
  }

  printf("  pop rax\n");
  if (type == bool_type) {
    printf("  cmp rax, 0\n");
    printf("  setne al\n");
    printf("  movzb rax, al\n");
  }

  int size = sizeof_type(type);
  if (type->kind == TY_PRM) {
    if (size == 1) {
      printf("  movsx rax, al\n");
    } else if (size == 2) {
      printf("  movsx rax, ax\n");
    } else if (size == 4) {
      printf("  movsxd rax, eax\n");
    }
  } else if (type->kind == TY_UNSIGNED) {
    if (size == 1) {
      printf("  movzx rax, al\n");
    } else if (size == 2) {
      printf("  movzx rax, ax\n");
    } else if (size == 4) {
      printf("  movzx rax, eax\n");
    }
  }
  printf("  push rax\n");
}

void gen_deref(Node *node) {
  if (node->kind == ND_ADDR) {
    gen(node->lhs);
    return;
  }
  gen(node);
  gen_deref_type(node->type->to);
}

void gen_deref_type(Type *type) {
  if (type->kind == TY_STRUCT) {
    printf("  # deref struct\n");
    return;
  }
  if (type->kind == TY_TYPEDEF || type->kind == TY_ENUM) {
    gen_deref_type(type->to);
    return;
  }

  printf("  pop rax\n");
  int size = sizeof_type(type);
  if (size == 1) {
    printf("  movsx rax, byte ptr [rax]\n");
  } else if (size == 2) {
    printf("  movsx rax, word ptr [rax]\n");
  } else if (size == 4) {
    printf("  movsxd rax, dword ptr [rax]\n");
  } else if (size == 8) {
    printf("  mov rax, [rax]\n");
  } else {
    char *type_name = substring(type->name, type->len);
    error("illegal defref size: sizeof(%s) = %d %d", type_name, size, type->kind);
  }
  printf("  push rax\n");
}

void gen_addr(Node* node) {
  if (node->kind == ND_DEREF) {
    gen(node->lhs);
    return;
  }
  if (node->offset != 0) {
    printf("  mov rax, rbp\n");
    printf("  sub rax, %d\n", node->offset);
    printf("  push rax\n");
  } else {
    printf("  push rbp\n");
  }
}

int max_size(Node* lhs, Node* rhs) {
  int lsize = sizeof_type(lhs->type);
  int rsize = sizeof_type(rhs->type);
  if (lsize >= rsize) {
    return lsize;
  } else {
    return rsize;
  }
}

void codegen() {
  printf("  .intel_syntax noprefix\n");
  gen_gvars_uninit();
  //printf("  .section  .rodata\n");
  gen_gvars();

  printf("\n");
  printf("  .text\n");
  for (int i = 0; code[i]; ++i) {
    gen_defined(code[i]);
  }
}

int type_is_struct_ref(Type* type) {
  if (type->kind == TY_PTR) {
    return type_is_struct_ref(type->to);
  }
  if (type->kind == TY_STRUCT) {
    return 1;
  } else {
    return 0;
  }
}