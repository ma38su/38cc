#include <stdio.h>

int a;
int b;
char c;

char* s = "Hello\n";
char t[] = "World\n";

int main() {
  printf(s);
  printf(t);
  printf("!!!\n");
  int i;
  for (i = 0; i < 10; i = i + 1) {
    int j = i + 1;
    printf("1 + %d = %d\n", i, j);
    //printf("1 + %d = %d\n", i, i + 1);
  }
  return 0;
}

