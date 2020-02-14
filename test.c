#include <stdio.h>

int a;
int b;
char c;

char* s = "Hello";
char t[] = "World";

int sum() {
  int sum = 0;
  int i;
  for (i = 0; i < 10; i = i + 1) {
    sum = sum + i;
  } 
  return sum;
}

void test1() {
  int v = sum();
  if (v == 45) {
    printf("OK\n");
  } else {
    printf("NG: actual is %d, expected is %d.\n", v, 45);
  }
}

void test2() {
  printf("printf(char*)  %s\n", s);
  printf("printf(char[]) %s\n", t);
}


int fib(int a) {                                                                                                                                                               
  if (a <= 0) {
    return 0;
  }
  if (a == 1) {
    return 1;
  }
  return fib(a - 1) + fib(a - 2);
}

void test3() {
  if (fib(10) == 55) {
    printf("OK\n");
  } else {
    printf("NG\n");
  }
}

void test4() {
  int i;
  for (i = 0; i < 10; i = i + 1) {
    printf("1 + %d = %d\n", i, i + 1);
  }
}

void test5() {
  if (fib(10) == 55) {
    printf("OK\n");
  } else {
    printf("NG\n");
  }
}

int main() {
  test1();
  test2();
  test3();
  test4();

  return 0;
}

