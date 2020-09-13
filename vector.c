#include <stdlib.h>

#include "vector.h"

Vector *new_vector() {
  Vector* vector = calloc(1, sizeof(Vector));
  vector->_len = 0;
  vector->size = 0;
  return vector;
}

void _ensure_array(Vector *vector) {
  int new_len;
  if (vector->_len == 0) {
    new_len = 8;
  } else {
    new_len = vector->_len << 1;
  }

  void **new_array = calloc(new_len, sizeof(void*));
  for (int i = 0; i < vector->size; ++i) {
    new_array[i] = vector->_array[i];
  }
  free(vector->_array);
  vector->_array = new_array;
  vector->_len = new_len;
}

void vec_add(Vector *vector, void *value) {
  if (vector->size >= vector->_len)
    _ensure_array(vector);
  vector->_array[vector->size++] = value;
}

void vec_set(Vector *vector, int i, void *value) {
  if (i >= vector->size) {
    //fprintf(stderr, "array out of exception: %d >= %d", i, vector->size);
    return;
  }
  vector->_array[i] = value;
}

void *vec_get(Vector *vector, int index) {
  return vector->_array[index];
}

int vec_contains(Vector *vector, void *value) {
  for (int i = 0; i < vector->size; ++i) {
    if (vector->_array[i] == value) {
      return 1;
    }
  }
  return 0;
}
