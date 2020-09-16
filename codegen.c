#include <stdio.h>
#include <string.h>
#include "38cc.h"

int label_id = 0;
int current_lid = -1;

int sizeof_type(Type *type);
int max_size(Node* lhs, Node* rhs);

bool gen(Node *node);
void gen_to_stack(Node *node);

void gen_lval(Node *node);
bool gen_gvar(Node *node);
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
int type_is_struct_ref(Type* type);
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

void gen_gvars_uninit() {
  for (Var *var = globals; var; var = var->next) {
    if (var->init || var->is_extern || var->type->kind == TY_FUNCTION) {
      continue;
    }
    int size = sizeof_type(var->type);
    char *name = substring(var->name, var->len);
    if (var->is_static) {
      printf("  .global %s\n", name);
    }
    // TODO alignment size
    printf("  .comm   %s,%d,%d\n", name, size, size);
  }
}

int n_gvars() {
  int data_count = 0;
  for (Var *var = globals; var; var = var->next) {
    if (!var->init || var->is_extern) continue;
    data_count++;
  }
  return data_count;
}

// self NG
void gen_gvar_declarations() {
  int data_count = n_gvars();
  if (data_count == 0) {
    return;
  }

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
      int size = sizeof_type(arg->type);
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

  int size = sizeof_type(node->type);
  if (node->lhs->kind == ND_GVAR) {
    gen_to_stack(node->rhs);
    printf("  pop rax\n");

    Node *lhs = node->lhs;
    Type *lhs_type = lhs->type;
    if (lhs_type == bool_type) {
      printf("  cmp rax, 0\n");
      printf("  setne al\n");
      printf("  movzb rax, al\n");
    }

    char *name = substring(lhs->ident, lhs->len);
    if (lhs_type == char_type || lhs_type == unsigned_char_type) {
      printf("  movsx byte ptr %s[rip], rax\n", name);
    } else if (lhs_type == short_type || lhs_type == unsigned_short_type) {
      printf("  movsx word ptr %s[rip], rax\n", name);
    } else if (lhs_type == int_type || lhs_type == unsigned_int_type) {
      printf("  mov dword ptr %s[rip], eax\n", name);
    } else if (lhs_type == long_type || lhs_type == unsigned_long_type) {
      printf("  mov %s[rip], rax\n", name);
    } else {
      printf("  # not support gvar %s, type: %s\n", name, lhs->type->name);
    }
    if (node->kind != ND_ASSIGN_POST) {
      printf("  push rax\n");
    }
  } else {
    gen_lval(node->lhs);
    gen_to_stack(node->rhs);

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

