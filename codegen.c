#include <stdio.h>
#include <string.h>
#include "38cc.h"

int label_id = 0;
int current_lid = -1;

long sizeof_type(Type *type);
int max_size(Node* lhs, Node* rhs);

bool gen(Node *node);
void gen_to_stack(Node *node);

void gen_lval(Node *node);
void gen_gvar(Node *node);
void gen_gvar_declarations();
void gen_gvar_declaration(Var *var);
void gen_gvars_uninit();
void gen_defined(Node *node);
void gen_defined_function(Node *node);
void gen_num(long num);
void gen_deref(Node *node);
void gen_deref_type(Type *type);
void gen_ternary(Node *node);
void truncate(Type *type);
char* args_register(int size, int index);

void codegen();
void gen_addr(Node* node);
void gen_return(Node *node);
void gen_if(Node *node);
void gen_do_while(Node *node);
void gen_while(Node *node);
void gen_for(Node *node);
void gen_assign(Node *node);
void gen_block(Node *node);
void gen_function_call(Node *node);

// self NG
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

void dump_gvars() {
  Var *var;
  for (var = globals; var; var = var->next) {
    if (is_debug) {
      fprintf(stderr, "ptr %x\n", var);
      fprintf(stderr, " ptr %.*s\n", var->len, var->name);
    }
    if (is_debug) fprintf(stderr, "call dump_gvars() %.*s\n", var->len, var->name);
  }
}

void gen_gvars_uninit() {

  if (is_debug) fprintf(stderr, "call gen_gvars_uninit()\n");
  dump_gvars();
  if (is_debug) fprintf(stderr, "call gen_gvars_uninit() 2\n");

  Var *var;
  for (var = globals; var; var = var->next) {
    if (var->init || var->is_extern || var->type->kind == TY_FUNCTION) {
      continue;
    }
    long size = sizeof_type(var->type);
    char *name = substring(var->name, var->len);
    if (var->is_static) {
      printf("  .global %s\n", name);
    }
    // TODO alignment size
    printf("  .comm   %s,%d,%d\n", name, size, size);
  }
  if (is_debug) fprintf(stderr, "call gen_gvars_uninit() end\n");
}

int n_gvars() {
  int count = 0;
  for (Var *var = globals; var; var = var->next) {
    if (!var->init || var->is_extern) continue;
    count++;
  }
  return count;
}

// self NG
void gen_gvar_declarations() {
  printf("  .data\n");
  for (Var *var = globals; var; var = var->next) {
    if (!var->init || var->is_extern) continue;
    if (is_debug) fprintf(stderr, "    gvar %s\n", substring(var->name, var->len));
    gen_gvar_declaration(var);
  }
}

// self NG segv
void gen_defined_function(Node *node) {
  if (node->kind != ND_FUNCTION) {
    error("node is not function");
  }
  if (!node->lhs) return; // skip extern function;

  // function label
  printf("  .global %s\n", node->ident);
  printf("%s:\n", node->ident);
  printf("  push rbp\n");
  printf("  mov rbp, rsp\n"); // prologue end

  int offset = node->offset;
  if (offset > 0) {
    // allocate local vars
    printf("  sub rsp, %d\n", offset);
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
      Type *type = raw_type(arg->type);
      long size = sizeof_type(type);
      char *r = args_register(size, index);
      printf("  mov [rax], %s\n", r);
    }
  }

  gen_block(node->lhs);

  printf("  mov rsp, rbp  # epilogue by %s function\n", node->ident);
  printf("  pop rbp\n");
  printf("  ret           # return rax value\n");
}

// self NG
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

    char *name = substring(lhs->ident, lhs->len);
    if (lhs_type->kind == TY_PRM) {
      if (lhs_type->size == 1) {
        printf("  mov %s[rip], dil\n", name);
      } else if (lhs_type->size == 2) {
        printf("  mov %s[rip], di\n", name);
      } else if (lhs_type->size == 4) {
        printf("  mov %s[rip], edi\n", name);
      } else if (lhs_type->size == 8) {
        printf("  mov %s[rip], rdi\n", name);
      } else {
        printf("  # not support gvar");
      }
    } else {
      printf("  # not support gvar %s, type: %s\n", name, lhs->type->name);
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
      char *type_name = substring(node->type->name, node->type->len);
      error("illegal assign defref: sizeof(%s) = %d", type_name, size);
    }
  }
  if (node->kind != ND_ASSIGN_POST) {
    printf("  push rdi\n");
  }
}

void gen_binary(Node *node) {
  gen_to_stack(node->lhs);
  gen_to_stack(node->rhs);

  if (node->kind == ND_SHL) {
    printf("  pop rcx\n");
    printf("  pop rax\n");
    printf("  shl rax, cl\n");
    printf("  push rax\n");
    return;
  } else if (node->kind == ND_SAR) {
    printf("  pop rcx\n");
    printf("  pop rax\n");
    printf("  sar rax, cl\n");
    printf("  push rax\n");
    return;
  }  

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
  } else {
    if (node->ident) {
      error_at(node->ident, "Unsupported operator");
    } else {
      error("Not calculatable: %d", node->kind);
    }
  }
  printf("  push rax\n");
}

// self NG
bool gen(Node *node) {
  if (!node) {
    error("node is NULL");
  }

  if (node->kind == ND_IF) {
    gen_if(node);
    return false;
  }
  if (node->kind == ND_FOR) {
    gen_for(node);
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
    if (!type_is_array(node->type) && raw_type(node->type)->kind != TY_STRUCT) {
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

void gen_gvar_declaration(Var *var) {
  char *name = substring(var->name, var->len);

  Type *type = raw_type(var->type);

  if (var->is_static) {
    printf("  .global %s\n", name);
  }
  if (*name == '.') {
    printf("%s:\n", name);
    if (!var->init || !var->init->str) error("char* null: %s", name);
    printf("  .string \"%s\"\n", substring(var->init->str, var->init->strlen));
  } else if (type->kind == TY_PRM) {
    if (type->size == 1) {
      printf("%s:\n", name);
      printf("  .byte %ld\n", var->init->n);
    } else if (type->size == 2) {
      printf("%s:\n", name);
      printf("  .word %ld\n", var->init->n);
    } else if (type->size == 4) {
      printf("%s:\n", name);
      printf("  .long %ld\n", var->init->n);
    } else if (type->size == 8) {
      printf("%s:\n", name);
      printf("  .quad %ld\n", var->init->n);
    } else {
      error("unsupported type");
    }
  } else if (type_is_array(type)) {
    Type *t = raw_type(type->to);

    if (is_debug)
      fprintf(stderr, "      type %s %s\n",
          substring(t->name, t->len),
          substring(type->name, type->len));

    if (t == char_type) {
      printf("%s:\n", name);
      if (!var->init || !var->init->str) error("char[] null: %s %s", name, var->init->ident);
      printf("  .string \"%s\"\n", substring(var->init->str, var->init->strlen));
    } else if (type_is_ptr(t)) {
      printf("%s:\n", name);
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
            fprintf(stderr, "      type ptr %s %s\n",
                substring(t->name, t->len),
                substring(type->name, type->len));

          printf("  .quad %s\n", v->ident);
        }
      }
    }
  } else if (type_is_ptr(type)) {
    printf("%s:\n", name);
    if (!var->init || !var->init->ident)
      error("char[] null: %s", name);
    printf("  .quad %s\n", var->init->ident);
  } else {
    error("unsupported type");
  }
}

void gen_function_call(Node *node) {
  if (node->kind != ND_CALL) error("not function call");

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


void gen_gvar(Node *node) {
  if (node->kind != ND_GVAR) {
    error("node is not gvar");
  }

  // for 64bit
  Type *type = raw_type(node->type);
  if (type->kind == TY_PRM) {
    if (type->is_unsigned) {
      if (type->size == 1) {
        printf("  movzx eax, byte ptr %s[rip]\n", node->ident);
      } else if (type->size == 2) {
        printf("  movzx eax, word ptr %s[rip]\n", node->ident);
      } else if (type->size == 4) {
        printf("  mov eax, dword ptr %s[rip]\n", node->ident);
      } else if (type->size == 8) {
        printf("  mov rax, %s[rip]\n", node->ident);
      } else {
        error("unsupported");
      }
    } else {
      if (type->size == 1) {
        printf("  movsx rax, byte ptr %s[rip]\n", node->ident);
      } else if (type->size == 2) {
        printf("  movsx rax, word ptr %s[rip]\n", node->ident);
      } else if (type->size == 4) {
        printf("  movsxd rax, dword ptr %s[rip]\n", node->ident);
      } else if (type->size == 8) {
        printf("  mov rax, %s[rip]\n", node->ident);
      } else {
        error("unsupported");
      }
    }
  } else if (type_is_array(type)) {
    printf("  lea rax, %s[rip]\n", node->ident);
  } else if (type_is_ptr(type)) {
    printf("  mov rax, qword ptr %s[rip]\n", node->ident);
  } else {
    printf("  # not support gvar %s, type: %s\n", node->ident, type->name);
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

  if (type->kind == TY_STRUCT) {
    //printf("  # deref struct\n");
    error("TODO deref struct: %s %s", substring(type->name, type->len));
    return;
  }

  printf("  pop rax\n");
  long size = sizeof_type(type);
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
      char *type_name = substring(type->name, type->len);
      error("illegal defref size: sizeof(%s) = %ld %d", type_name, size, type->kind);
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
      char *type_name = substring(type->name, type->len);
      error("illegal defref size: sizeof(%s) = %ld %d", type_name, size, type->kind);
    }
  }

  printf("  push rax\n");
}

