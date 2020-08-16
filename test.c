#include <stdio.h>

//#include <string.h>
int strcmp(char* m1, char* m2) {
  char* tmp_1 = m1;
  char* tmp_2 = m2;

  int i = 0;

  while (1) {
    if (m1[i] != m2[i]) {
      if (m1[i] < m2[i]) {
        return 1;
      } else {
        return -1;
      }
    }

    if (m1[i] == '\0') {
      return 0;
    }
    ++i;
  }
}

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

void assertChar(char *name, char expect, char actual) {
  if (actual == expect) {
    printf("%s: OK, expect: '%c', actual: '%c'\n", name, expect, actual);
  } else {
    printf("%s: NG, expect: '%c', actual: '%c'\n", name, expect, actual);
  }
}

void assertStr(char *name, char *expect, char *actual) {
  if (strcmp(actual, expect) == 0) {
    printf("%s: OK, expect: %s, actual: %s\n", name, expect, actual);
  } else {
    printf("%s: NG, expect: %s, actual: %s\n", name, expect, actual);
  }
}

typedef enum {
  MAME_A,
  MAME_B,
  MAME_C,
} Mame;

int a;
int b;
char c;

char s1[] = "Hello";
char *s2 = "World";

// 四則演算
void test1() {
  assertInt("test1-1", 5, 2 + 3);
  assertInt("test1-2", 2 + 3, 3 + 2);
  assertInt("test1-3", 6, 2 * 3);
  assertInt("test1-4", 2 * 3, 3 * 2);
  assertInt("test1-5", -21, 7 * -3);
  assertInt("test1-6", 7 * -3, -3 * 7);
}

void test2() {
  char *msg = "Hello, World\n";
  assertChar("test2-1", 'a', 'a');
  assertChar("test2-2", 'b', 'b');
  assertChar("test2-3", 'H', *msg);
  assertChar("test2-4", 'e', *(msg+1));
  assertChar("test2-5", 'l', *(msg+2));
  assertChar("test2-6", 'l', *(msg+3));
  assertChar("test2-7", 'o', *(msg+4));
  assertChar("test2-8", ',', *(msg+5));
  assertChar("test2-9", ' ', *(msg+6));
  assertChar("test2-10", 'W', *(msg+7));
  assertChar("test2-11", 'o', *(msg+8));
  assertChar("test2-12", 'r', *(msg+9));
  assertChar("test2-13", 'l', *(msg+10));
  assertChar("test2-14", 'd', *(msg+11));
  assertChar("test2-15", '\n', *(msg+12));
  assertChar("test2-16", '\0', *(msg+13));

  assertChar("test2-17", 'H', *(msg++));
  assertChar("test2-18", 'l', *(++msg));

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
  for (i = 0; i < 10; ++i) {
    sum = sum + i;
  } 
  return sum;
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

// 関数呼び出し
void test3() {
  assert("test3-1", test_func(1, 2));
  assertInt("test3-2", 45, sum());
  assertInt("test3-3", 55, fib(10));
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

  assertStr("test4-4", s1, "Hello");
  assertStr("test4-5", s2, "World");

  sprintf(buf, "Hello, %s", s2);
  assertStr("test4-6", buf, "Hello, World");

  sprintf(buf, "%s, World", s1);
  assertStr("test4-7", buf, "Hello, World");

    sprintf(buf, "%s, %s", s1, s2);
  assertStr("test4-8", buf, "Hello, World");
}

void test5() {
  Mame a = MAME_A;
  Mame b = MAME_B;
  Mame c = MAME_A;

  assert("test5-1", MAME_A == MAME_A);
  assert("test5-1", MAME_A != MAME_B);
  assert("test5-2", MAME_B != MAME_A);

  assert("test5-3", a != b);
  assert("test5-4", b != a);
  assert("test5-5", a == c);
  assert("test5-6", c == a);
}

void test6() {
  int i = 0;
  assertInt("test6-1", 0, i);
  assertInt("test6-2", 1, ++i);
  assertInt("test6-3", 1, i);
  assertInt("test6-4", 0, --i);
  assertInt("test6-5", 0, i);

  assertInt("test6-6", 0, i++);
  assertInt("test6-7", 1, i);
  assertInt("test6-8", 1, i--);
  assertInt("test6-9", 0, i);
}

int main() {

  test1();
  test2();
  test3();
  test4();
  test5();
  test6();

  return 0;
}

