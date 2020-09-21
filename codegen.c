#include <stdio.h>
#include <string.h>
#include "38cc.h"

int label_id = 0;
int current_lid = -1;

long sizeof_type(Type *type);
int max_size(Node* lhs, Node* rhs);
char* args_register(int size, int index);

void codegen();

bool gen(Node *node);
void gen_to_stack(Node *node);

void gen_num(long num);
void gen_lval(Node *node);
void gen_gvar(Node *node);
void gen_gvar_declarations();
void gen_gvar_declaration(Var *var);
void gen_gvars_uninit();
void gen_defined(Node *node);
void gen_defined_function(Node *node);
void gen_defined_function(Node *node);

void gen_if(Node *node);
void gen_switch(Node *node);
void gen_case(Node *node);
void gen_default(Node *node);
void gen_for(Node *node);
void gen_while(Node *node);
void gen_do_while(Node *node);
void gen_block(Node *node);

void gen_assign(Node *node);
void gen_addr(Node* node);
void gen_deref(Node *node);
void gen_deref_type(Type *type);
void gen_ternary(Node *node);
void truncate(Type *type);

void gen_function_call(Node *node);
void gen_return(Node *node);

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

void gen_gvars_uninit() {
  printf("  .text\n");
  for (Var *var = globals; var; var = var->next) {
    if (var->init || var->is_extern || var->type->kind == TY_FUNCTION) {
      continue;
    }
    if (var->is_static) {
      printf("  .global %.*s\n", var->len, var->name);
    }
    long size = sizeof_type(var->type);
    // TODO alignment size
    printf("  .comm   %.*s,%ld,%ld\n", var->len, var->name, size, size);
  }
}

void gen_gvar_declarations() {
  printf("  .data\n");
  for (Var *var = globals; var; var = var->next) {
    if (!var->init || var->is_extern) continue;
    if (is_debug) fprintf(stderr, "    gvar %.*s\n", var->len, var->name);
    gen_gvar_declaration(var);
  }
}

void gen_gvar_declaration(Var *var) {
  Type *type = raw_type(var->type);

  if (var->is_static) {
    printf("  .global %.*s\n", var->len, var->name);
  }
  printf("%.*s:\n", var->len, var->name);
  if (*var->name == '.') {
    if (!var->init || !var->init->str) error("char* null: %.*s", var->len, var->name);
    printf("  .string \"%.*s\"\n", var->init->strlen, var->init->str);
  } else if (type->kind == TY_PRM) {
    if (type->size == 1) {
      printf("  .byte %ld\n", var->init->n);
    } else if (type->size == 2) {
      printf("  .word %ld\n", var->init->n);
    } else if (type->size == 4) {
      printf("  .long %ld\n", var->init->n);
    } else if (type->size == 8) {
      printf("  .quad %ld\n", var->init->n);
    } else {
      error("unsupported type");
    }
  } else if (type_is_array(type)) {
    Type *t = raw_type(type->to);
    if (is_debug)
      fprintf(stderr, "      type %.*s %.*s\n",
          t->len, t->name,
          type->len, type->name);

    if (t == char_type) {
      if (!var->init || !var->init->str) error("char[] null: %.*s %s", var->len, var->name, var->init->ident);
      printf("  .string \"%.*s\"\n", var->init->strlen, var->init->str);
    } else if (type_is_ptr(t)) {
      if (t->to == char_type) {
        for (InitVal *v = var->init; v; v = v->next) {
          if (!v->ident) error("illegal gvars");
          printf("  .quad %.*s\n", v->len, v->ident);
        }
      } else {
        error("unsupported initialization");
      }
    } else {
      if (t->kind == TY_PRM) {
        if (t->size == 1) {
          for (InitVal *v = var->init; v; v = v->next) {
            printf("  .byte %ld\n", v->n);
          }
        } else if (t->size == 2) {
          for (InitVal *v = var->init; v; v = v->next) {
            printf("  .word %ld\n", v->n);
          }
        } else if (t->size == 4) {
          for (InitVal *v = var->init; v; v = v->next) {
            printf("  .long %ld\n", v->n);
          }
        } else if (t->size == 8) {
          for (InitVal *v = var->init; v; v = v->next) {
            printf("  .quad %ld\n", v->n);
          }
        } else {
          error("unsupported type");
        }
      } else if (type_is_ptr(t)) {
        for (InitVal *v = var->init; v; v = v->next) {
          if (is_debug)
            fprintf(stderr, "      type ptr %.*s %.*s\n",
                t->len, t->name,
                type->len, type->name);

          printf("  .quad %.*s\n", v->len, v->ident);
        }
      }
    }
  } else if (type_is_ptr(type)) {
    if (!var->init || !var->init->ident)
      error("char[] null: %.*s", var->len, var->name);
    printf("  .quad %.*s\n", var->init->len, var->init->ident);
  } else {
    error("unsupported type");
  }
}

void gen_defined_function(Node *node) {
  if (node->kind != ND_FUNCTION) {
    error("node is not function");
  }
  if (!node->lhs) return; // skip extern function;

  // function label
  printf("  .global %.*s\n", node->len, node->ident);
  printf("%.*s:\n", node->len, node->ident);
  printf("  push rbp\n");
  printf("  mov rbp, rsp\n"); // prologue end

  int offset = node->offset;
  if (offset > 0) {
    // allocate local vars
    printf("  sub rsp, %d\n", offset);
  }
  // extract args
  if (node->list) {
    for (int i = node->list->size - 1; i >= 0; --i) {
      Node *arg = (Node *) vec_get(node->list, i);
      gen_addr(arg);
      printf("  pop rax\n");

      Type *type = raw_type(arg->type);
      long size = sizeof_type(type);
      char *reg = args_register(size, i);
      printf("  mov [rax], %s # arg \"%.*s\"\n", reg, arg->len, arg->ident);
    }
  }

  gen_block(node->lhs);

  printf("  mov rsp, rbp  # epilogue by call %.*s\n", node->len, node->ident);
  printf("  pop rbp\n");
  printf("  ret           # return rax value\n");
}

void gen_assign(Node *node) {
  if (node->kind == ND_ASSIGN_POST) {
    gen_to_stack(node->lhs);
  }

  long size = sizeof_type(node->type);
  Node *lhs = node->lhs;
  Type *lhs_type = lhs->type;

  if (lhs->kind == ND_GVAR) {
    gen_to_stack(node->rhs);
    printf("  pop rdi\n");

    if (lhs_type == bool_type) {
      printf("  cmp rdi, 0\n");
      printf("  setne dil\n");
      printf("  movzb rdi, dil\n");
    }

    if (lhs_type->kind == TY_PRM) {
      if (lhs_type->size == 1) {
        printf("  mov %.*s[rip], dil\n", lhs->len, lhs->ident);
      } else if (lhs_type->size == 2) {
        printf("  mov %.*s[rip], di\n", lhs->len, lhs->ident);
      } else if (lhs_type->size == 4) {
        printf("  mov %.*s[rip], edi\n", lhs->len, lhs->ident);
      } else if (lhs_type->size == 8) {
        printf("  mov %.*s[rip], rdi\n", lhs->len, lhs->ident);
      } else {
        printf("  # not support gvar");
      }
    } else if (lhs_type->kind == TY_PTR) {
      printf("  mov %.*s[rip], rdi\n", lhs->len, lhs->ident);
    }
  } else {
    gen_lval(lhs);
    gen_to_stack(node->rhs);

    printf("  pop rdi\n");
    printf("  pop rax\n");

    if (lhs_type == bool_type) {
      printf("  cmp rdi, 0\n");
      printf("  setne dil\n");
      printf("  movzb rdi, dil\n");
    }
    if (size == 1) {
      printf("  mov [rax], dil\n");
    } else if (size == 2) {
      printf("  mov [rax], di\n");
    } else if (size == 4) {
      printf("  mov [rax], edi\n");
    } else if (size == 8) {
      printf("  mov [rax], rdi\n");
    } else {
      error("illegal assign deref: sizeof(%.*s) = %d",
          node->type->len, node->type->name, size);
    }
  }
  if (node->kind != ND_ASSIGN_POST) {
    printf("  push rdi\n");
  }
}

void gen_binary(Node *node) {
  gen_to_stack(node->lhs);
  gen_to_stack(node->rhs);
  printf("  pop rdi\n");
  printf("  pop rax\n");

  if (node->kind == ND_PTR_ADD) {
    int ptr_size = sizeof_type(node->lhs->type->to);
    printf("  imul rdi, %d\n", ptr_size);
    printf("  add rax, rdi\n");
  } else if (node->kind == ND_PTR_SUB) {
    int ptr_size = sizeof_type(node->lhs->type->to);
    printf("  imul rdi, %d\n", ptr_size);
    printf("  sub rax, rdi\n");
  } else if (node->kind == ND_PTR_DIFF) {
    int ptr_size = sizeof_type(node->lhs->type->to);
    printf("  sub rax, rdi\n");
    printf("  cqo\n");
    printf("  mov rdi, %d\n", ptr_size);
    printf("  idiv rdi\n");
  } else if (node->kind == ND_ADD) {
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
  } else if (node->kind == ND_SAR) {
    printf("  mov cl, dil\n");
    printf("  sar rax, cl\n");
  } else if (node->kind == ND_SHL) {
    printf("  mov cl, dil\n");
    printf("  shl rax, cl\n");
  } else {
    if (node->ident) {
      error_at(node->ident, "Unsupported operator");
    } else {
      error("Not calculatable: %d", node->kind);
    }
  }
  printf("  push rax\n");
}

void gen_to_stack(Node *node) {
  if (!gen(node)) error("illegal gen");
}

bool gen(Node *node) {
  if (!node) {
    error("node is NULL");
  }

  if (node->kind == ND_DECLR) {
    for (int i = 0; i < node->list->size; ++i) {
      Node *n = vec_get(node->list, i);
      if (gen(n)) {
        printf("  pop rax\n");
      }
    }
    return false;
  }
  if (node->kind == ND_IF) {
    gen_if(node);
    return false;
  }
  if (node->kind == ND_FOR) {
    gen_for(node);
    return false;
  }
  if (node->kind == ND_SWITCH) {
    gen_switch(node);
    return false;
  }
  if (node->kind == ND_CASE) {
    gen_case(node);
    return false;
  }
  if (node->kind == ND_DEFAULT) {
    gen_default(node);
    return false;
  }
  if (node->kind == ND_WHILE) {
    gen_while(node);
    return false;
  }
  if (node->kind == ND_DO) {
    gen_do_while(node);
    return false;
  }
  if (node->kind == ND_RETURN) {
    gen_return(node);
    return false;
  }
  if (node->kind == ND_CONTINUE) {
    printf("  jmp .L.continue.%d # CONTINUE\n", current_lid);
    return false;
  }
  if (node->kind == ND_BREAK) {
    printf("  jmp .L.end.%d # BREAK\n", current_lid);
    return false;
  }
  if (node->kind == ND_CAST) {
    gen_to_stack(node->lhs);
    truncate(node->type);
    return true;
  }
  if (node->kind == ND_NUM) {
    gen_num(node->val);
    return true;
  }
  if (node->kind == ND_LVAR) {
    gen_addr(node);
    if (!type_is_array(node->type) && !type_is_struct(node->type)) {
      gen_deref_type(node->type);
    }
    return true;
  }
  if (node->kind == ND_GVAR) {
    gen_gvar(node);
    return true;
  }

  if (node->kind == ND_ASSIGN || node->kind == ND_ASSIGN_POST) {
    gen_assign(node);
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
    gen_deref(node->lhs);
    return true;
  }
  if (node->kind == ND_TERNARY) {
    gen_ternary(node);
    return true;
  }

  if (node->kind == ND_AND) {
    int lid = label_id++;
    printf("  # AND (&&)\n");
    gen_to_stack(node->lhs);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  je  .L.false.%d\n", lid);
    gen_to_stack(node->rhs);
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
    gen_to_stack(node->lhs);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  jne .L.true.%d\n", lid);
    gen_to_stack(node->rhs);
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
    gen_to_stack(node->lhs);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  sete al\n");
    printf("  movzb rax, al\n");
    printf("  push rax\n");
    return true;
  }
  if (node->kind == ND_BITNOT) {
    printf("  # BITNOT (~)\n");
    gen_to_stack(node->lhs);
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

  gen_binary(node);

  return true;
}

void codegen() {
  if (is_debug) fprintf(stderr, "call codegen...\n");

  printf("  .file \"%s\"\n", filename);
  printf("  .intel_syntax noprefix\n");

  gen_gvars_uninit();

  if (is_debug) fprintf(stderr, "  gvar declarations\n");
  gen_gvar_declarations();

  printf("\n");
  printf("  .text\n");
  for (int i = 0; code[i]; ++i) {
    if (is_debug) fprintf(stderr, "  %d\n", i);
    gen_defined(code[i]);
  }
}

int max_size(Node* lhs, Node* rhs) {
  long lsize = sizeof_type(lhs->type);
  long rsize = sizeof_type(rhs->type);
  if (lsize >= rsize) {
    return lsize;
  } else {
    return rsize;
  }
}

void gen_function_call(Node *node) {
  if (node->kind != ND_CALL) error("not function call");

  if (memcmp(node->ident, "__builtin_va_start", 18) == 0) {
    printf("  pop rax\n");
    printf("  mov edi, dword ptr [rbp-8]\n");
    printf("  mov dword ptr [rax], 0\n");
    printf("  mov dword ptr [rax+4], 0\n");
    printf("  mov qword ptr [rax+8], rdi\n");
    printf("  mov qword ptr [rax+16], 0\n");
    return;
  }

  if (node->list) {
    for (int i = 0; i < node->list->size; ++i) {
      Node *n = (Node *) vec_get(node->list, i);
      gen_to_stack(n);
    }
    for (int i = node->list->size - 1; i >= 0; --i) {
      char *r = args_register(8, i);
      printf("  pop %s  # arg %d\n", r, i);
    }
  }

  // reset for return val
  printf("  mov rax, 0\n");
  if (node->val) {
    printf("  call %.*s@PLT\n", node->len, node->ident);
  } else {
    printf("  call %.*s\n", node->len, node->ident);
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

void gen_gvar(Node *node) {
  if (node->kind != ND_GVAR) {
    error("node is not gvar");
  }

  // for 64bit
  Type *type = raw_type(node->type);
  if (type->kind == TY_PRM) {
    if (type->is_unsigned) {
      if (type->size == 1) {
        printf("  movzx eax, byte ptr %.*s[rip]\n", node->len, node->ident);
      } else if (type->size == 2) {
        printf("  movzx eax, word ptr %.*s[rip]\n", node->len, node->ident);
      } else if (type->size == 4) {
        printf("  mov eax, dword ptr %.*s[rip]\n", node->len, node->ident);
      } else if (type->size == 8) {
        printf("  mov rax, %.*s[rip]\n", node->len, node->ident);
      } else {
        error("unsupported");
      }
    } else {
      if (type->size == 1) {
        printf("  movsx rax, byte ptr %.*s[rip]\n", node->len, node->ident);
      } else if (type->size == 2) {
        printf("  movsx rax, word ptr %.*s[rip]\n", node->len, node->ident);
      } else if (type->size == 4) {
        printf("  movsxd rax, dword ptr %.*s[rip]\n", node->len, node->ident);
      } else if (type->size == 8) {
        printf("  mov rax, %.*s[rip]\n", node->len, node->ident);
      } else {
        error("unsupported");
      }
    }
  } else if (type_is_array(type)) {
    printf("  lea rax, %.*s[rip]\n", node->len, node->ident);
  } else if (type_is_ptr(type)) {
    printf("  mov rax, %.*s[rip]\n", node->len, node->ident);
  } else {
    printf("  # not support gvar %.*s, type: %.*s\n", node->len, node->ident, type->len, type->name);
  }
  printf("  push rax\n");
}

// push store address
void gen_lval(Node *node) {
  gen_addr(node);
}

void gen_deref(Node *node) {
  if (node->kind == ND_ADDR) {
    gen_to_stack(node->lhs);
    return;
  }
  gen_to_stack(node);
  gen_deref_type(raw_type(node->type)->to);
}

void gen_deref_type(Type *type) {
  type = raw_type(type);
  if (type->kind == TY_ARRAY || type->kind == TY_STRUCT) {
    error("illegal state");
  }

  printf("  pop rax\n");
  long size = sizeof_type(type);
  if (type->kind == TY_PRM) {
    if (type->is_unsigned) {
      if (size == 1) {
        printf("  movzx eax, byte ptr [rax]\n");
      } else if (size == 2) {
        printf("  movzx eax, word ptr [rax]\n");
      } else if (size == 4) {
        printf("  mov eax, [rax]\n");
      } else if (size == 8) {
        printf("  mov rax, [rax]\n");
      } else {
        error("illegal deref type: sizeof(%.*s) = %ld", type->len, type->name, size);
      }
    } else {
      if (size == 1) {
        printf("  movsx rax, byte ptr [rax]\n");
      } else if (size == 2) {
        printf("  movsx rax, word ptr [rax]\n");
      } else if (size == 4) {
        printf("  movsxd rax, dword ptr [rax]\n");
      } else if (size == 8) {
        printf("  mov rax, [rax]\n");
      } else {
        error("illegal deref unsigned type: sizeof(%.*s) = %ld", type->len, type->name, size);
      }
    }
  } else {
    printf("  mov rax, [rax]\n");
  }
  printf("  push rax\n");
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

Vector *pick_cases(Node *node) {
  if (node->kind != ND_BLOCK) error("not block");

  Vector *ret = new_vector();

  Vector *vec = node->list;
  for (int i = 0; i < vec->size; ++i) {
    Node *n = vec_get(vec, i);
    if (n->kind == ND_CASE || n->kind == ND_DEFAULT) {
      vec_add(ret, n);
    }
  }
  return ret;
}

void gen_switch(Node *node) {
  if (node->kind != ND_SWITCH) {
    error("not switch");
  }

  int prev_lid = current_lid;
  int lid = current_lid = label_id++;

  gen_to_stack(node->cnd);
  printf("  pop rax\n");

  Vector *case_list = pick_cases(node->thn);
  for (int i = 0; i < case_list->size; ++i) {
    int case_id = label_id++;
    Node *n = vec_get(case_list, i);
    if (n->kind == ND_CASE) {
      printf("  cmp	eax, %ld\n", n->val);
      printf("  je  .L%d\n", case_id);
    } else if (n->kind == ND_DEFAULT) {
      printf("  jmp  .L%d\n", case_id);
    } else {
      error("not supported");      
    }
    n->val = case_id;
  }
  gen_block(node->thn);

  printf(".L.end.%d:\n", lid);
  current_lid = prev_lid;
}

void gen_case(Node *node) {
  if (node->kind != ND_CASE) error("not case");
  printf(".L%ld:\n", node->val);
}

void gen_default(Node *node) {
  if (node->kind != ND_DEFAULT) error("not default");
  printf(".L%ld:\n", node->val);
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
