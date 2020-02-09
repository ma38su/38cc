#include <stdlib.h>

#include "linkedlist.h"

void ll_add(LinkedList *list, void *value) {
  LLNode *node = calloc(1, sizeof(LLNode));
  node->value = value;
  if (list->tail) {
    node->prev = list->tail;
    list->tail->next = node;
    list->tail = node;
  } else {
    list->head = node;
    list->tail = node;
  }
  list->size++;
}

