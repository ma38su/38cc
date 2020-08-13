#include <stdio.h>

/*
int strcmp(char* m1, char* m2) {
  int i = 0;
  while (1) {
    if (m1[i] == m2[i]) {
      if (m1[i] == '\0') {
        return 1;
      }
      i++;
      continue;
    }
    return 0;
  }
}
*/
//#include <string.h>

void assert(char* name, int ret) {
  if (ret) {
    printf("%s: OK\n", name);
  } else {
    printf("%s: NG\n", name);
  }
}

void assertInt(char *name, int expect, int actual) {
  if (actual == expect) {
    printf("%s: OK, expect: %d, actual: %d\n", name, expect, actual);
  } else {
    printf("%s: NG, expect: %d, actual: %d\n", name, expect, actual);
  }
}

void assertStr(char *name, char *expect, char *actual) {
  puts("Unimplemented");
  /*
  if (strcmp(actual, expect) == 0) {
    printf("%s: OK, expect: %s, actual: %s\n", name, expect, actual);
  } else {
    printf("%s: NG, expect: %s, actual: %s\n", name, expect, actual);
  }
  */
}

typedef enum {
  MAME_A,
  MAME_B,
  MAME_C,
} Mame;

int a;
int b;
char c;

char* s = "Hello";
char t[] = "World";

// 四則演算
void test1() {
  assertInt("test1-1", 5, 2 + 3);
  assertInt("test1-1", 2 + 3, 3 + 2);
  assertInt("test1-2", 6, 2 * 3);
  assertInt("test1-3", 2 * 3, 3 * 2);
  assertInt("test1-3", -21, 7 * -3);
  assertInt("test1-3", 7 * -3, -3 * 7);
}

int test_func(int a, int b) {
  if (a == 1) {
    if (b == 2) {
      return 1;
    }
  }
  return 0;
}

int sum() {
  int sum = 0;
  int i;
  for (i = 0; i < 10; i = i + 1) {
    sum = sum + i;
  } 
  return sum;
}

// 関数呼び出し
void test2() {
  assert("test2-1", test_func(1, 2));
  assertInt("test2-2", 45, sum());
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
  assertInt("test3", 55, fib(10));
}

void test4() {
  int i;

  char buf[100];
  i = 0;
  sprintf(buf, "1 + %d = %d", i, i + 1);
  assertStr("test4-1", buf, "1 + 0 = 1");

  i = 3;
  sprintf(buf, "1 + %d = %d", i, i + 1);
  assertStr("test4-2", buf, "1 + 3 = 4");

  i = 9;
  sprintf(buf, "1 + %d = %d", i, i + 1);
  assertStr("test4-3", buf, "1 + 9 = 10");

  assertStr("test4-4", s, "Hello");
  assertStr("test4-5", t, "World");

  sprintf(buf, "%s, %s\n", s, t);
  assertStr("test4-4", buf, "Hello, World\n");
}

void test5() {
  Mame a = MAME_A;
  Mame b = MAME_B;
  Mame c = MAME_A;

  assert("test5-1", a != b);
  assert("test5-2", b != a);
  assert("test5-3", a == c);
  assert("test5-4", c == a);
}

int main() {
  test1();
  test2();
  test3();
  test4();
  test5();

  return 0;
}

