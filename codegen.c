#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "38cc.h"

int label_id = 0;
int current_lid = -1;

int sizeof_type(Type *type);
int max_size(Node* lhs, Node* rhs);

bool gen(Node *node);
void gen_lval(Node *node);
bool gen_gvar(Node *node);
void gen_gvars();
void gen_gvars_uninit();
void gen_defined(Node *node);
void gen_defined_function(Node *node);
void gen_num(int num);
void gen_deref(Node *node);
void gen_deref_type(Type *type);
void truncate(Type *type);
int type_is_struct_ref(Type* type);
char* get_args_register(int size, int index);

void codegen();
void gen_deref(Node *node);
void gen_deref_type(Type *type);
void gen_addr(Node* node);
bool gen_return(Node *node);

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
  gen(node->cnd);
  printf("  pop rax # while\n");
  printf("  cmp rax, 0\n");
  printf("  je  .L.end.%d\n", lid);
  gen(node->thn);
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
  gen(node->thn);
  if (node->stp) gen(node->stp);
  printf("  jmp .L.begin.%d\n", lid);
  printf(".L.end.%d:\n", lid);
  current_lid = prev_lid;
  return false;
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

void gen_gvars_uninit() {
  for (Var *var = globals; var; var = var->next) {
    if (var->init || var->extn || var->type->kind == TY_FUNCTION) {
      continue;
    }
    int size = sizeof_type(var->type);
    // TODO alignment size
    printf("  .comm   %s,%d,%d\n", substring(var->name, var->len), size, size);
  }
}

// self NG
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
    char *name = substring(var->name, var->len);
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
      printf("# unsupported type: %s: %s\n", name, var->type->name);
    }
  }
}

// self NG
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

// self NG
bool gen(Node *node) {
  if (!node) {
    error("node is NULL");
  }
  if (node->kind == ND_COMMENT) {
    printf("  ### %s\n", node->ident);
    return gen(node->lhs);
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
      gen_deref_type(node->type);
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
      if (lhs->type == bool_type) {
        printf("  cmp rax, 0\n");
        printf("  setne al\n");
        printf("  movzb rax, al\n");
      }

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
      
      if (node->lhs->type == bool_type) {
        printf("  cmp rdi, 0\n");
        printf("  setne dil\n");
        printf("  movzb rdi, dil\n");
      }
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
    gen_deref(node->lhs);
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

