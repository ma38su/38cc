#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "38cc.h"

int label_id = 0;
int current_lid = -1;

void gen_gvars();
void gen_gvars_uninit();

void gen_defined(Node *node);

bool gen(Node *node);
void gen_num(int num);
void gen_deref(Type *type);
int sizeof_type(Type *type);

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

  printf("\n");
  printf("  .text\n");
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
  if (node->offset != 0) {
    printf("  mov rax, rbp\n");
    printf("  sub rax, %d\n", node->offset);
    printf("  push rax\n");
  } else {
    printf("  push rbp\n");
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

void gen_function_call(Node *node) {
  if (node->kind != ND_CALL) error("not function call");

  if (node->list) {
    for (int i = 0; i < node->list->size; ++i) {
      gen((Node *) vec_get(node->list, i));
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

// cast
void truncate(Type *type) {
  if (type->kind == TY_TYPEDEF || type->kind == TY_ENUM) {
    truncate(type->to);
    return;
  }

  if (type->kind != TY_PRM) return;
  printf("  pop rax\n");
  if (type == bool_type) {
    printf("  cmp rax, 0\n");
    printf("  setne al\n");
  }

  int size = sizeof_type(type);
  if (size == 1) {
    printf("  movsx rax, al\n");
  } else if (size == 2) {
    printf("  movsx rax, ax\n");
  } else if (size == 4) {
    printf("  movsxd rax, eax\n");
  } else {
    fprintf(stderr, "warn: unsupported case? - size: %d", size);
  }
  printf("  push rax\n");
}

void gen_block(Node *node) {
  if (node->kind != ND_BLOCK) {
    error("not block");
  }

  for (int i = 0; i < node->list->size; ++i) {
    Node *n = (Node *) vec_get(node->list, i);
    if (!gen(n)) continue;
    printf("  pop rax\n");
  }
}

bool gen_while(Node *node) {
  if (node->kind != ND_WHILE) {
    error("not while");
  }

  int prev_lid = current_lid;
  int lid = current_lid = label_id++;
  printf(".L.begin.%d:\n", lid);
  gen(node->lhs);
  printf("  pop rax # while\n");
  printf("  cmp rax, 0\n");
  printf("  je  .L.end.%d\n", lid);
  gen(node->rhs);
  printf("  jmp .L.begin.%d\n", lid);
  printf(".L.end.%d:\n", lid);
  current_lid = prev_lid;
  return false;
}

bool gen_do_while(Node *node) {
  if (node->kind != ND_DO) {
    error("not do while");
  }

  int prev_lid = current_lid;
  int lid = current_lid = label_id++;
  printf(".L.begin.%d: # do while\n", lid);
  gen(node->thn);
  gen(node->cnd);
  printf("  pop rax\n");
  printf("  cmp rax, 0\n");
  printf("  je  .L.end.%d\n", lid);
  printf("  jmp .L.begin.%d\n", lid);
  printf(".L.end.%d:\n", lid);
  current_lid = prev_lid;
  return false;
}

bool gen_for(Node *node) {
  if (node->kind != ND_FOR) {
    error("not for");
  }
  if (node->lhs) {
    gen(node->lhs);
  }
  return gen_while(node->rhs);
}

bool gen_if(Node *node) {
  if (node->kind != ND_IF) error("not if");

  gen(node->cnd);
  printf("  pop rax # if\n");
  printf("  cmp rax, 0\n");
  int lid = label_id++;
  if (!node->els) {
    printf("  je  .L.end.%d\n", lid);
    gen(node->thn);
    printf(".L.end.%d:\n", lid);
  } else {
    printf("  je  .L.else.%d\n", lid);
    gen(node->thn);
    printf("  jmp .L.end.%d\n", lid);
    printf(".L.else.%d:\n", lid);
    gen(node->els);
    printf(".L.end.%d:\n", lid);
  }
  return false;
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

void gen_gvars_uninit() {
  for (Var *var = globals; var; var = var->next) {
    if (var->init || var->extn || var->type->kind == TY_FUNCTION) {
      continue;
    }
    int size = sizeof_type(var->type);
    // TODO alignment size
    printf("  .comm   %s,%d,%d\n", var->name, size, size);
  }
}

void gen_gvars() {
  int data_count = 0;
  for (Var *var = globals; var; var = var->next) {
    if (!var->init || var->extn) continue;
    data_count++;
  }
  if (data_count == 0) {
    return;
  }

  printf("  .data\n");
  for (Var *var = globals; var; var = var->next) {
    if (!var->init || var->extn) continue;
    if (var->type == char_type) {
      printf("%s:\n", var->name);
      printf("  .byte %d\n", var->init->n);
    } else if (var->type == short_type) {
      printf("%s:\n", var->name);
      printf("  .word %d\n", var->init->n);
    } else if (var->type == int_type) {
      printf("%s:\n", var->name);
      printf("  .long %d\n", var->init->n);
    } else if (var->type == long_type) {
      printf("%s:\n", var->name);
      printf("  .quad %d\n", var->init->n);
    } else if (*(var->name) == '.') {
      printf("%s:\n", var->name);
      if (!var->init || !var->init->str) error("char* null: %s", var->name);
      printf("  .string \"%s\"\n", substring(var->init->str, var->init->strlen));
    } else if (type_is_array(var->type)) {
      Type *t = var->type->to;
      if (t == char_type) {
        printf("%s:\n", var->name);
        if (!var->init || !var->init->str) error("char[] null: %s %s", var->name, var->init->ident);
        printf("  .string \"%s\"\n", substring(var->init->str, var->init->strlen));
      } else if (type_is_ptr(t)) {
        printf("%s:\n", var->name);
        if (t->to == char_type) {
          for (InitVal *v = var->init; v; v = v->next) {
            if (!v->ident) error("illegal gvars");
            printf("  .quad %s\n", substring(v->ident, v->len));
          }
        } else {
          error("unsupported initialization");
        }
      } else {
        printf("%s:\n", var->name);
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
      printf("%s:\n", var->name);
      if (!var->init || !var->init->ident)
        error("char[] null: %s", var->name);
      printf("  .quad %s\n", var->init->ident);
    } else {
      printf("# unsupported type: %s: %s\n", var->name, var->type->name);
    }
  }
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
  printf("\n");
  printf("  .global %s\n", node->ident);
  printf("%s:\n", node->ident);
  printf("  push rbp\n");
  printf("  mov rbp, rsp\n\n"); // prologue end

  int offset = node->val;
  if (offset > 0) {
    if (offset % 16 != 0) {
      offset = (offset / 16 + 1) * 16;
    }
    // allocate local vars
    printf("  sub rsp, %d # args+lvar\n", offset);
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
      int size = sizeof_type(arg->type);
      if (size == 1) {
        char *r = get_args_register(size, index);
        printf("  mov [rax], %s\n", r);
      } else if (size == 2) {
        char *r = get_args_register(size, index);
        printf("  mov [rax], %s\n", r);
      } else if (size == 4) {
        char *r = get_args_register(size, index);
        printf("  mov [rax], %s\n", r);
        //printf("  mov dword ptr [rax], %s\n", r);
      } else if (size == 8) {
        char *r = get_args_register(size, index);
        printf("  mov [rax], %s\n", r);
      } else {
        error("unsupported arg - sizeof(%s) = %d", substring(arg->ident, arg->len), size);
      }
    }
  }

  gen_block(node->lhs);

  printf("  mov rsp, rbp  # epilogue by %s function\n", node->ident);
  printf("  pop rbp\n");
  printf("  ret           # return rax value\n");
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

void gen_deref(Type *type) {
  if (type->kind == TY_STRUCT) return;
  if (type->kind == TY_TYPEDEF || type->kind == TY_ENUM) {
    gen_deref(type->to);
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

// push store address
void gen_lval(Node *node) {
  if (node->kind == ND_DEREF) {
    gen(node->lhs);
  } else {
    gen_addr(node);
  }
}

void gen_num(int num) {
  printf("  push %d  # num %d\n", num, num);
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
    error("node is NULL");
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
  if (node->kind == ND_DO) {
    return gen_do_while(node);
  }
  if (node->kind == ND_RETURN) {
    return gen_return(node);
  }
  if (node->kind == ND_CONTINUE) {
    printf("  # CONTINUE\n");
    printf("  jmp .L.begin.%d\n", current_lid);
    return false;
  }
  if (node->kind == ND_BREAK) {
    printf("  # BREAK\n");
    printf("  jmp .L.end.%d\n", current_lid);
    return false;
  }
  if (node->kind == ND_CAST) {
    gen(node->lhs);
    truncate(node->type);
    return true;
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

    int size = sizeof_type(node->type);
    if (node->lhs->kind == ND_GVAR) {

      gen(node->rhs);
      char *name = substring(node->lhs->ident, node->lhs->len);
      printf("  pop rax\n");

      Node *lhs = node->lhs;
      if (lhs->type == char_type) {
        printf("  movsx byte ptr %s[rip], rax\n", name);
      } else if (lhs->type == short_type) {
        printf("  movsx word ptr %s[rip], rax\n", name);
      } else if (lhs->type == int_type) {
        printf("  mov dword ptr %s[rip], eax\n", name);
      } else if (lhs->type == long_type) {
        printf("  mov %s[rip], rax\n", name);
      } else {
        printf("  # not support gvar %s, type: %s\n", name, lhs->type->name);
      }
      printf("  push rax\n");
    } else {
      gen_lval(node->lhs);
      gen(node->rhs);

      printf("  # assign\n");
      printf("  pop rdi\n");
      printf("  pop rax\n");

      if (size == 1) {
        printf("  mov [rax], dil\n");
        //printf("  mov byte ptr [rax], dil\n");
      } else if (size == 2) {
        printf("  mov [rax], di\n");
        //printf("  mov word ptr [rax], di\n");
      } else if (size == 4) {
        printf("  mov [rax], edi\n");
        //printf("  mov dword ptr [rax], edi\n");
      } else if (size == 8) {
        printf("  mov [rax], rdi\n");
        //printf("  mov qword ptr [rax], rdi\n");
      } else {
        char *type_name = substring(node->type->name, node->type->len);
        error("illegal assign defref: sizeof(%s) = %d", type_name, size);
      }

      if (node->kind != ND_ASSIGN_POST) {
        printf("  push rdi\n");
      }
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

  if (node->kind == ND_TERNARY) {
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
    return true;
  }

  if (node->kind == ND_AND) {
    int lid = label_id++;
    printf("  # AND (&&)\n");
    gen(node->lhs);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  je  .L.false.%d\n", lid);
    gen(node->rhs);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  je  .L.false.%d\n", lid);
    printf("  push 1\n");
    printf("  jmp .L.end.%d\n", lid);
    printf(".L.false.%d:\n", lid);
    printf("  push 0\n");
    printf(".L.end.%d:\n", lid);
    return true;
  }
  if (node->kind == ND_OR) {
    int lid = label_id++;
    printf("  # OR (||)\n");
    gen(node->lhs);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  jne .L.true.%d\n", lid);
    gen(node->rhs);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  jne .L.true.%d\n", lid);
    printf("  push 0\n");
    printf("  jmp .L.end.%d\n", lid);
    printf(".L.true.%d:\n", lid);
    printf("  push 1\n");
    printf(".L.end.%d:\n", lid);
    return true;
  }
  if (node->kind == ND_NOT) {
    printf("  # NOT (!)\n");
    gen(node->lhs);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  sete al\n");
    printf("  movzb rax, al\n");
    printf("  push rax\n");
    return true;
  }
  if (node->kind == ND_BITNOT) {
    printf("  # BITNOT (~)\n");
    gen(node->lhs);
    printf("  pop rax\n");
    printf("  not rax\n");
    printf("  push rax\n");
    return true;
  }

  if (!node->lhs || !node->rhs) {
    if (node->ident) {
      error_at(node->ident, "unsupported node");
    } else {
      error("unsupported node: %d", node->kind);
    }
  }

  bool lhs_is_ptr = type_is_ptr(node->lhs->type) || type_is_array(node->lhs->type);
  bool rhs_is_ptr = type_is_ptr(node->rhs->type) || type_is_array(node->rhs->type);

  gen(node->lhs);
  if (!lhs_is_ptr && rhs_is_ptr) {
    // TODO ptr calc
    if (raw_type(raw_type(node->rhs->type)->to)->kind != TY_STRUCT) {
      int rhs_size = sizeof_type(node->rhs->type->to);
      if (rhs_size == 0) {
        error("no rhs ptr size: %s",
          substring(node->lhs->type->to->name, node->lhs->type->to->len));
      }
      // for pointer calculation
      printf("  pop rax\n");
      printf("  imul rax, %d\n", rhs_size);
      printf("  push rax\n");
    }
  }

  gen(node->rhs);
  if (lhs_is_ptr && !rhs_is_ptr) {
    // TODO ptr calc
    if (raw_type(raw_type(node->lhs->type)->to)->kind != TY_STRUCT) {
      int lhs_size = sizeof_type(node->lhs->type->to);
      if (lhs_size == 0) {
        error("no lhs ptr size %s",
          substring(node->lhs->type->to->name, node->lhs->type->to->len));
      }
      // for pointer calculation
      printf("  pop rax\n");
      printf("  imul rax, %d\n", lhs_size);
      printf("  push rax\n");
    }
  }

  if (node->kind == ND_SHL) {
    printf("  pop rcx\n");
    printf("  pop rax\n");
    printf("  shl rax, cl\n");
    printf("  push rax\n");
    return true;
  } else if (node->kind == ND_SAR) {
    printf("  pop rcx\n");
    printf("  pop rax\n");
    printf("  sar rax, cl\n");
    printf("  push rax\n");
    return true;
  }  

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
  } else if (node->kind == ND_EQ) {
    printf("  cmp rax, rdi\n");
    printf("  sete al\n");  // al: under 8bit of rax
    printf("  movzb rax, al\n");
  } else if (node->kind == ND_NE) {
    printf("  cmp rax, rdi\n");
    printf("  setne al\n");  // al: under 8bit of rax
    printf("  movzb rax, al\n");
  } else if (node->kind == ND_LE) {
    printf("  cmp rax, rdi\n");
    printf("  setle al\n");  // al: under 8bit of rax
    printf("  movzb rax, al\n");
  } else if (node->kind == ND_LT) {
    printf("  cmp rax, rdi\n");
    printf("  setl al\n");  // al: under 8bit of rax
    printf("  movzb rax, al\n");
  } else if (node->kind == ND_BITAND) {
    printf("  and rax, rdi\n");
  } else if (node->kind == ND_BITXOR) {
    printf("  xor rax, rdi\n");
  } else if (node->kind == ND_BITOR) {
    printf("  or rax, rdi\n");
  } else {
    if (node->ident) {
      error_at(node->ident, "Unsupported operator");
    } else {
      error("Not calculatable: %d", node->kind);
    }
  }
  printf("  push rax\n");
  return true;
}

