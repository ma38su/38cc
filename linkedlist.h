#ifndef LINKEDLIST_H
#define LINKEDLIST_H 

typedef struct LinkedList LinkedList;
typedef struct LLNode LLNode;

struct LLNode {
  void *value;
  LLNode *next;
  LLNode *prev;
};

struct LinkedList {
  LLNode *head;
  LLNode *tail;
  int size;
};

void ll_add(LinkedList *list, void *value);

#endif
