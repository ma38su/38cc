#ifndef VECTOR_H
#define VECTOR_H 

typedef struct Vector Vector;
typedef struct VNode VNode;

struct VNode {
  void *value;
  VNode *next;
  VNode *prev;
};

struct Vector {
  VNode *head;
  VNode *tail;
  int size;
};

void vec_add(Vector *vector, void *value);

#endif
