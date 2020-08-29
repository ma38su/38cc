#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "38cc.h"

int label_id = 0;

void gen_gvars();
void gen_gvars_uninit();

void gen_defined(Node *node);

bool gen(Node *node);
void gen_num(int num);
void gen_deref(Type *type);

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

void codegen() {
  printf("  .intel_syntax noprefix\n");
  gen_gvars_uninit();
  //printf("  .section  .rodata\n");
  gen_gvars();

  printf("  .global main\n");
  for (int i = 0; code[i]; ++i) {
    gen_defined(code[i]);
  }
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
  } else if (size == 8) {
    return reg64s[index];
  } else {
    error("illegal arg size: %d", size);
    return NULL;
  }
}

void gen_addr(Node* node) {
  /*
  if (node->kind != ND_LVAR) {
    error("not lvar: %d", node->kind);
  }
  */

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

    printf("  # push %d args\n", node->list->size);
    for (int i = 0; i < node->list->size; ++i) {
      gen((Node *) vec_get(node->list, i));
    }
    printf("  # set %d args\n", node->list->size);
    for (int i = node->list->size - 1; i >= 0; --i) {
      char *r = get_args_register(8, i);
      printf("  pop %s\n", r);
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
  for (int i = 0; i < node->list->size; ++i) {
    Node *n = (Node *) vec_get(node->list, i);
    if (gen(n)) {
      printf("  pop rax\n");
      printf("  # end line\n\n");
    }
  }
  printf("\n");
  printf("  # block end\n");
}

bool gen_while(Node *node) {
  if (node->kind != ND_WHILE) {
    error("not while");
  }
  int lid = label_id++;
  printf(".Lbegin%03d:\n", lid);
  gen(node->lhs);
  printf("  pop rax # while\n");
  printf("  cmp rax, 0\n");
  printf("  je  .Lend%03d\n", lid);
  gen(node->rhs);
  printf("  jmp .Lbegin%03d\n", lid);
  printf(".Lend%03d:\n", lid);
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
  int lid = label_id++;
  if (node->rhs->kind != ND_ELSE) {
    printf("  je  .Lend%03d\n", lid);
    gen(node->rhs);
    printf(".Lend%03d:\n", lid);
  } else {
    printf("  je  .Lelse%03d\n", lid);
    gen(node->rhs->lhs);
    printf("  jmp .Lend%03d\n", lid);
    printf(".Lelse%03d:\n", lid);
    gen(node->rhs->rhs);
    printf(".Lend%03d:\n", lid);
  }
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
  printf("  # return rax value\n");
  printf("  ret\n");
  return false;
}

void gen_gvars_uninit() {
  for (GVar *var = globals; var; var = var->next) {
    if (var->init || var->extn) {
      continue;
    }
    printf("  .comm   %s,%d,%d\n", var->name, var->type->size, var->type->size);
  }
}
void gen_gvars() {
  for (GVar *var = globals; var; var = var->next) {
    if (!var->init || var->extn) {
      continue;
    }
    if (var->type == char_type) {
      printf("%s:\n", var->name);
      printf("  .byte %d\n", var->val);
    } else if (var->type == short_type) {
      printf("%s:\n", var->name);
      printf("  .word %d\n", var->val);
    } else if (var->type == int_type) {
      printf("%s:\n", var->name);
      printf("  .long %d\n", var->val);
    } else if (var->type == long_type) {
      printf("%s:\n", var->name);
      printf("  .quad %d\n", var->val);
    } else if (*(var->name) == '.') {
      printf("%s:\n", var->name);
      printf("  .string \"%s\"\n", var->str);
    } else if (type_is_ptr(var->type)) {
      printf("%s:\n", var->name);
      printf("  .quad %s\n", var->str);
    } else if (type_is_array(var->type)) {
      printf("%s:\n", var->name);
      printf("  .string \"%s\"\n", var->str);
    } else {
      printf("# unsupported type: %s: %s\n", var->name, var->type->name);
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
  } else if (type_is_array(node->type) && node->type->to == char_type) {
    printf("  lea rax, %s[rip]\n", node->ident);
  } else if (type_is_ptr(node->type) && node->type->to == char_type) {
    printf("  mov rax, qword ptr %s[rip]\n", node->ident);
  } else {
    printf("  # not support gvar %s, type: %s\n", node->ident, node->type->name);
  }
  printf("  push rax\n");
  return true;
}

void gen_defined_function(Node *node) {
  if (node->kind != ND_FUNCTION) {
    error("node is not function");
  }
  if (!node->lhs) {
    // skip extern function;
    return;
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

    for (int i = node->list->size - 1; i >= 0; --i) {
      Node *arg = (Node *) vec_get(node->list, i);
      printf("  # extract arg \"%s\"\n", arg->ident);
      gen_addr(arg);
      printf("  pop rax\n");

      --index;
      int size = arg->type->size;
      char *r = get_args_register(size, index);
      printf("  mov [rax], %s\n", r);
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

void gen_deref(Type *type) {
  if (type->kind == TY_STRUCT || type->kind == TY_TYPEDEF) {
    return;
  }

  int size = type->size;
  printf("  pop rax\n");

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
      gen_deref(node->type);
    }
    return true;
  }
  if (node->kind == ND_GVAR) {
    return gen_gvar(node);
  }

  if (node->kind == ND_ASSIGN || node->kind == ND_ASSIGN_POST) {

    if (node->kind == ND_ASSIGN_POST) {
      gen(node->lhs);
    }

    gen_lval(node->lhs);

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
    } else if (size == 8) {
      printf("  mov [rax], rdi\n");
    } else {
      char *type_name = substring(node->type->name, node->type->len);
      error("illegal assign defref: sizeof(%s) = %d", type_name, size);
    }

    if (node->kind != ND_ASSIGN_POST) {
      printf("  push rdi\n");
    }
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
    gen_deref(node->lhs->type->to);
    return true;
  }

  bool lhs_is_ptr = type_is_ptr(node->lhs->type) || type_is_array(node->lhs->type);
  bool rhs_is_ptr = type_is_ptr(node->rhs->type) || type_is_array(node->rhs->type);

  printf("  # LHS\n");
  gen(node->lhs);
  if (!lhs_is_ptr && rhs_is_ptr) {
    // for pointer calculation
    printf("  pop rax\n");
    printf("  imul rax, %d\n", node->rhs->type->to->size);
    printf("  push rax\n");
  }

  printf("  # RHS\n");
  gen(node->rhs);
  if (lhs_is_ptr && !rhs_is_ptr) {
    // for pointer calculation
    printf("  pop rax\n");
    printf("  imul rax, %d\n", node->lhs->type->to->size);
    printf("  push rax\n");
  }

  if (node->kind == ND_SHL) {
    printf("  # SHL (<<)\n");
    printf("  pop rcx\n");
    printf("  pop rax\n");
    printf("  shl rax, cl\n");
    printf("  push rax\n");
    return true;
  } else if (node->kind == ND_SAR) {
    printf("  # SAL (>>)\n");
    printf("  pop rcx\n");
    printf("  pop rax\n");
    printf("  sar rax, cl\n");
    printf("  push rax\n");
    return true;
  }  

  printf("  pop rdi\n");
  printf("  pop rax\n");
  if (node->kind == ND_ADD) {
    printf("  # ADD (+)\n");
    printf("  add rax, rdi\n");
  } else if (node->kind == ND_SUB) {
    printf("  # SUB (-)\n");
    printf("  sub rax, rdi\n");
  } else if (node->kind == ND_MUL) {
    printf("  # MUL (*)\n");
    printf("  imul rax, rdi\n");
  } else if (node->kind == ND_DIV) {
    printf("  # DIV (/)\n");
    printf("  cqo\n");
    printf("  idiv rdi\n");  // divide by (rdx << 64 | rax) / rdi => rax, rdx
  } else if (node->kind == ND_MOD) {
    printf("  # MOD (%%)\n");
    printf("  cqo\n");
    printf("  idiv rdi\n");  // divide by (rdx << 64 | rax) / rdi => rax, rdx
    printf("  mov rax, rdx\n");
  } else if (node->kind == ND_EQ) {
    printf("  # EQ (==)\n");
    printf("  cmp rax, rdi\n");
    printf("  sete al\n");  // al: under 8bit of rax
    printf("  movzb rax, al\n");
  } else if (node->kind == ND_NE) {
    printf("  # NE (!=)\n");
    printf("  cmp rax, rdi\n");
    printf("  setne al\n");  // al: under 8bit of rax
    printf("  movzb rax, al\n");
  } else if (node->kind == ND_LE) {
    printf("  # LE (>=)\n");
    printf("  cmp rax, rdi\n");
    printf("  setle al\n");  // al: under 8bit of rax
    printf("  movzb rax, al\n");
  } else if (node->kind == ND_LT) {
    printf("  # LT (>)\n");
    printf("  cmp rax, rdi\n");
    printf("  setl al\n");  // al: under 8bit of rax
    printf("  movzb rax, al\n");
  } else {
    error("failed to calc");
  }
  printf("  push rax\n");
  return true;
}

