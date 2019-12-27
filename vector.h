#ifndef VECTOR_H
#define VECTOR_H 

typedef struct Vector Vector;
typedef struct VNode VNode;

struct VNode {
  void *value;
  VNode *next;
};

struct Vector {
  VNode *head;
  VNode *tail;
};

void add_last(Vector *vector, void *value);

#endif
