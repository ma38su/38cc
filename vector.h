#ifndef VECTOR_H 
#define VECTOR_H 

typedef struct Vector Vector;

struct Vector {
  void **_array;
  int _len;
  int size;
};

Vector *new_vector();

void vec_add(Vector *vector, void *value);
void *vec_get(Vector *vector, int index);

#endif
