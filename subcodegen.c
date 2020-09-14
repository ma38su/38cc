#include <stdio.h>
#include "38cc.h"

extern int label_id;
extern int current_lid;

bool gen(Node *node);
int sizeof_type(Type *type);
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

void gen_gvar(Node *node) {
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
    if (raw_type(raw_type(node->type)->to) == char_type) {
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
}

void gen_return(Node *node) {
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
}


void gen_function_call(Node *node) {
  if (node->kind != ND_CALL) error("not function call");

  if (node->list) {
    for (int i = 0; i < node->list->size; ++i) {
      Node *n = (Node *) vec_get(node->list, i);
      gen(n);
    }
    for (int i = node->list->size - 1; i >= 0; --i) {
      char *r = get_args_register(8, i);
      printf("  pop %s  # arg %d\n", r, i);
    }
  }

  int padding = 0;
  if (padding) {
    int lid = label_id++;
    printf("  mov rax, rsp\n");
    printf("  and rax, 15\n");
    printf("  jnz .L.call.%d\n", lid);

    // reset for return val
    printf("  mov rax, 0\n");
    if (node->val) {
      printf("  call %s@PLT\n", node->ident);
    } else {
      printf("  call %s\n", node->ident);
    }
    printf("  jmp .L.end.%d\n", lid);

    printf(".L.call.%d:\n", lid);
    printf("  sub rsp, 8  # align rsb to 16 byte boundary\n");

    // reset for return val
    printf("  mov rax, 0\n");
    if (node->val) {
      printf("  call %s@PLT\n", node->ident);
    } else {
      printf("  call %s\n", node->ident);
    }
    printf("  add rsp, 8  # turn back rsb\n");

    printf(".L.end.%d:\n", lid);
  } else {
    // reset for return val
    printf("  mov rax, 0\n");
    if (node->val) {
      printf("  call %s@PLT\n", node->ident);
    } else {
      printf("  call %s\n", node->ident);
    }
  }

  // after called, return value is stored rax
  printf("  push rax\n");
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
    truncate(type->def);
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
  } else if (type->is_unsigned) {
    if (size == 1) {
      printf("  movzx rax, al\n");
    } else if (size == 2) {
      printf("  movzx eax, ax\n");
    }
  }
  printf("  push rax\n");
}

void gen_if(Node *node) {
  if (node->kind != ND_IF) error("not if");

  gen(node->cnd);
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
  printf(".L.begin.%d:\n", lid);
  gen(node->cnd);
  printf("  pop rax # while\n");
  printf("  cmp rax, 0\n");
  printf("  je  .L.end.%d\n", lid);
  if (gen(node->thn)) {
    printf("  pop rax # skip\n");
  }
  printf("  jmp .L.begin.%d\n", lid);
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
  gen(node->cnd);
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
  if (node->ini) {
    gen(node->ini);
  }

  int prev_lid = current_lid;
  int lid = current_lid = label_id++;
  printf(".L.begin.%d:\n", lid);
  gen(node->cnd);
  printf("  pop rax # while\n");
  printf("  cmp rax, 0\n");
  printf("  je  .L.end.%d\n", lid);
  if (gen(node->thn)) {
    printf("  pop rax # skip\n");
  }
  if (node->stp && gen(node->stp)) {
    printf("  pop rax # skip\n");
  }
  printf("  jmp .L.begin.%d\n", lid);
  printf(".L.end.%d:\n", lid);
  current_lid = prev_lid;
}

void gen_deref(Node *node) {
  if (node->kind == ND_ADDR) {
    gen(node->lhs);
    return;
  }
  gen(node);
  gen_deref_type(raw_type(node->type)->to);
}

void gen_deref_type(Type *type) {
  if (type->kind == TY_STRUCT) {
    printf("  # deref struct\n");
    return;
  }
  if (type->kind == TY_TYPEDEF || type->kind == TY_ENUM) {
    gen_deref_type(type->def);
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
    printf("  sub rax, %ld\n", node->offset);
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

void gen_ternary(Node *node) {
  int lid = label_id++;
  printf("  # TERNAY (?:)\n");
  gen(node->cnd);
  printf("  pop rax\n");
  printf("  cmp rax, 0\n");
  printf("  je  .L.false.%d\n", lid);
  gen(node->thn);
  printf("  jmp .L.end.%d\n", lid);
  printf(".L.false.%d:\n", lid);
  gen(node->els);
  printf(".L.end.%d:\n", lid);
}


void gen_gvar_declaration(Var *var) {
  char *name = substring(var->name, var->len);

  if (var->is_static) {
    printf("  .global %s\n", name);
  }
  if (var->type == char_type) {
    printf("%s:\n", name);
    printf("  .byte %d\n", var->init->n);
  } else if (var->type == short_type) {
    printf("%s:\n", name);
    printf("  .word %d\n", var->init->n);
  } else if (var->type == int_type) {
    printf("%s:\n", name);
    printf("  .long %d\n", var->init->n);
  } else if (var->type == long_type) {
    printf("%s:\n", name);
    printf("  .quad %d\n", var->init->n);
  } else if (*name == '.') {
    printf("%s:\n", name);
    if (!var->init || !var->init->str) error("char* null: %s", name);
    printf("  .string \"%s\"\n", substring(var->init->str, var->init->strlen));
  } else if (type_is_array(var->type)) {
    Type *t = var->type->to;

    if (t == char_type) {
      printf("%s:\n", name);
      if (!var->init || !var->init->str) error("char[] null: %s %s", name, var->init->ident);
      printf("  .string \"%s\"\n", substring(var->init->str, var->init->strlen));
    } else if (type_is_ptr(t)) {
      printf("%s:\n", name);
      printf(" # DEBUG1\n");
      if (t->to == char_type) {
        for (InitVal *v = var->init; v; v = v->next) {
          if (!v->ident) error("illegal gvars");
          printf("  .quad %s\n", substring(v->ident, v->len));
        }
      } else {
        error("unsupported initialization");
      }
    } else {
      printf("%s:\n", name);
      if (t) {
        if (t->name) {
          printf(" # DEBUG3 %d %s\n", t->kind);
        } else {
          printf(" # DEBUG4 %d\n", t->kind);
        }
      } else {
        printf(" # DEBUG2\n");
      }
      if (t == char_type) {
        for (InitVal *v = var->init; v; v = v->next) {
          printf("  .byte %d\n", v->n);
        }
      } else if (t == short_type) {
        for (InitVal *v = var->init; v; v = v->next) {
          printf("  .word %d\n", v->n);
        }
      } else if (t == int_type) {
        for (InitVal *v = var->init; v; v = v->next) {
          printf("  .long %d\n", v->n);
        }
      } else if (t == long_type) {
        for (InitVal *v = var->init; v; v = v->next) {
          printf("  .quad %d\n", v->n);
        }
      } else if (type_is_ptr(t)) {
        for (InitVal *v = var->init; v; v = v->next) {
          printf("  .quad %s\n", v->ident);
        }
      }
    }
  } else if (type_is_ptr(var->type)) {
    printf("%s:\n", name);
    if (!var->init || !var->init->ident)
      error("char[] null: %s", name);
    printf("  .quad %s\n", var->init->ident);
  } else {
    error("unsupported type");
    //printf("# unsupported type: %s: %s\n", name, var->type->name);
  }
}

void codegen() {
  printf("  .intel_syntax noprefix\n");
  gen_gvars_uninit();
  gen_gvar_declarations();

  printf("\n");
  printf("  .text\n");
  for (int i = 0; code[i]; ++i) {
    gen_defined(code[i]);
  }
}

int type_is_struct_ref(Type* type) {
  type = raw_type(type);
  if (type->kind == TY_PTR) {
    return type_is_struct_ref(type->to);
  }
  if (type->kind == TY_STRUCT) {
    return 1;
  } else {
    return 0;
  }
}
