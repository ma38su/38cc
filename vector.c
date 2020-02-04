#include <stdlib.h>

#include "vector.h"

void vec_add(Vector *vector, void *value) {
  VNode *node = calloc(1, sizeof(VNode));
  node->value = value;
  if (vector->tail) {
    node->prev = vector->tail;
    vector->tail->next = node;
    vector->tail = node;
  } else {
    vector->head = node;
    vector->tail = node;
  }
  vector->size++;
}

