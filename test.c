#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "vector.h"

#include "38cc.h"

//static int debug = 0;
//extern int debug;
int debug = 0;

void assert(char* name, int ret);
void assertInt(char *, int, int);

void assertChar(char *name, char expect, char actual) {
  if (actual == expect) {
    if (debug) {
      printf("%s: OK, expect: '%c', actual: '%c'\n", name, expect, actual);
    }
  } else {
    printf("%s: NG, expect: '%c', actual: '%c'\n", name, expect, actual);
  }
}

void assertStr(char *name, char *expect, char *actual) {
  if (strcmp(actual, expect) == 0) {
    if (debug) {
      printf("%s: OK, expect: %s, actual: %s\n", name, expect, actual);
    }
  } else {
    printf("%s: NG, expect: %s, actual: %s\n", name, expect, actual);
  }
}

typedef enum {
  MAME_A,
  MAME_B,
  MAME_C,
} Mame;

typedef enum {
  SAKE_1,
  SAKE_2,
  SAKE_3,
} Sake;

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

// test for increment operator and decrement operator
void test2() {
  int i = 0;
  assertInt("test2-1", 0, i);
  assertInt("test2-2", 1, ++i);
  assertInt("test2-3", 1, i);
  assertInt("test2-4", 0, --i);
  assertInt("test2-5", 0, i);

  assertInt("test2-6", 0, i++);
  assertInt("test2-7", 1, i);
  assertInt("test2-8", 1, i--);
  assertInt("test2-9", 0, i);
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
  for (int i = 0; i < 5; ++i) {
    sum = sum + i;
  }
  for (int i = 5; i < 10; ++i) {
    sum += i;
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

  char *msg = "Hello, World\n";
  assertChar("test5-1", 'a', 'a');
  assertChar("test5-2", 'b', 'b');
  assertChar("test5-3", 'H', *msg);
  assertChar("test5-4", 'e', *(msg+1));
  assertChar("test5-5", ' ', *(msg+6));
  assertChar("test5-6", 'W', *(msg+7));
  assertChar("test5-7", 'd', *(msg+11));
  assertChar("test5-8", '\n', *(msg+12));
  assertChar("test5-9", '\0', *(msg+13));

  assertChar("test5-10", 'H', *(msg++));
  assertChar("test5-11", 'l', *(++msg));
  assertChar("test5-12", 'e', *(--msg));
  assertChar("test5-13", 'e', *(msg--));
  assertChar("test5-14", 'H', *msg);
}

void test6() {
  Mame a = MAME_A;
  Mame b = MAME_B;
  Mame c = MAME_C;

  assert("test5-1", MAME_A == 0);
  assert("test5-2", MAME_B == 1);
  assert("test5-3", MAME_C == 2);

  assert("test5-4", MAME_A == MAME_A);
  assert("test5-5", MAME_A != MAME_B);
  assert("test5-6", MAME_B != MAME_A);

  assert("test5-7", a != b);
  assert("test5-8", b != a);

  assert("test5-9", a + 2 == c);
  assert("test5-10", b + 1 == c);

  assert("test5-11", MAME_A == SAKE_1);
  assert("test5-12", MAME_A != SAKE_2);
}

struct Char {
  char val;
};
typedef struct Char Char;

typedef struct {
  short val;
} Short;

typedef struct {
  int val;
} Int;

typedef struct {
  short val1;
  char val2;
} ShortChar;

typedef struct {
  int val1;
  char val2;
} IntChar;

typedef struct {
  long val1;
  char val2;
} LongChar;

typedef struct {
  char val1;
  long val2;
  char val3;
} CharLongChar;

typedef struct {
  char val1;
  char val2;
  long val3;
} CharCharLong;

typedef struct {
  short val1;
  int val2;
  short val3;
} ShortIntShort;

typedef struct {
  int val1;
  short val2;
  short val3;
} IntShortShort;

typedef struct {
  long val1;
  long val2;
  long val3;
} LongLongLong;

typedef struct {
  int val1;
  int val2;
  int val3;
} IntIntInt;

void test7() {

  char char_val;
  int int_val;

  char *char_ptr;
  int *int_ptr;

  int int_array_7[7];
  char char_array_7[7];

  assertInt("test7-1", 1, sizeof(char));
  assertInt("test7-2", 1, sizeof(char_val));
  assertInt("test7-3", 8, sizeof(&char_val));

  assertInt("test7-4", 4, sizeof(int));
  assertInt("test7-5", 4, sizeof(int_val));
  assertInt("test7-6", 8, sizeof(&int_val));

  assertInt("test7-7", 8, sizeof(char *));
  assertInt("test7-8", 8, sizeof(char_ptr));
  assertInt("test7-9", 1, sizeof(*char_ptr));

  assertInt("test7-10", 8, sizeof(int *));
  assertInt("test7-11", 8, sizeof(int_ptr));
  assertInt("test7-12", 4, sizeof(*int_ptr));

  assertInt("test7-13", 1 * 7, sizeof(char_array_7));
  assertInt("test7-14", 1, sizeof(*char_array_7));
  assertInt("test7-15", 1, sizeof(char_array_7[1]));

  assertInt("test7-16", 4 * 7, sizeof(int_array_7));
  assertInt("test7-17", 4, sizeof(*int_array_7));
  assertInt("test7-18", 4, sizeof(int_array_7[1]));

  assertInt("test7-19", 1, sizeof(struct Char));
  assertInt("test7-19", 1, sizeof(Char));
  assertInt("test7-20", 2, sizeof(Short));
  assertInt("test7-21", 4, sizeof(Int));

  assertInt("test7-22", 4, sizeof(ShortChar));
  assertInt("test7-23", 8, sizeof(IntChar));
  assertInt("test7-24", 16, sizeof(LongChar));
  assertInt("test7-25", 24, sizeof(CharLongChar));
  assertInt("test7-26", 16, sizeof(CharCharLong));
  assertInt("test7-27", 8, sizeof(IntShortShort));
  assertInt("test7-28", 12, sizeof(ShortIntShort));

  assertInt("test7-29", 24, sizeof(LongLongLong));
  assertInt("test7-30", 12, sizeof(IntIntInt));
}

//struct User User;

void test8() {
  LongLongLong p;

  assertInt("test8-1", 24, sizeof(p));

  p.val2 = 17;
  p.val1 = 13;
  p.val3 = 23;

  assertInt("test8-2", 13, p.val1);
  assertInt("test8-3", 17, p.val2);
  assertInt("test8-4", 23, p.val3);
}

void test9() {
  IntIntInt p;
  assertInt("test9-1", 12, sizeof(p));

  p.val2 = 17;
  p.val1 = 13;
  p.val3 = 23;

  assertInt("test9-2", 13, p.val1);
  assertInt("test9-3", 17, p.val2);
  assertInt("test9-4", 23, p.val3);
}

void testIntIntInt(IntIntInt *p) {
  assertInt("test10-5", 13, p->val1);
  assertInt("test10-6", 17, p->val2);
  assertInt("test10-7", 23, p->val3);
}

void test10() {
  IntIntInt *p = calloc(1, sizeof(IntIntInt));
  assertInt("test10-1", 8, sizeof(p));
  assertInt("test10-2", 12, sizeof(*p));

  p->val2 = 17;
  p->val1 = 13;
  p->val3 = 23;

  assertInt("test10-2", 13, p->val1);
  assertInt("test10-3", 17, p->val2);
  assertInt("test10-4", 23, p->val3);

  testIntIntInt(p);
}

void test11() {
  int v1 = 2;
  int v2 = 3;
  int v3 = 28;
  int v4 = -14;

  assertInt("test11-1", 16, v1 << v2);
  assertInt("test11-2", 12, v2 << v1);

  assertInt("test11-3", 14, v3 >> 1);
  assertInt("test11-4", 1, v2 >> 1);
  assertInt("test11-5", 0, v2 >> v1);
  assertInt("test11-6", 0, v2 >> v3);

  assertInt("test11-7", -28, v4 << 1);
  assertInt("test11-8", -7, v4 >> 1);

  v1 <<= 1;
  assertInt("test11-9", 4, v1);
  v2 <<= v1;
  assertInt("test11-10", (1 << 4) * 3, v2);

  v3 >>= 2;
  assertInt("test11-11", 7, v3);
  v4 >>= 2;
  assertInt("test11-13", -3 - 1, v4);
}

void test12() {
  int i = 0;
  for (;;) {
    if (i++ < 9) {
      continue;
    }
    break;
  }
  assertInt("test12-1", 10, i);

  while (1) {
    if (i < 10) {
      break;
    }
    if (i++ < 19) continue;
    break;
  }
  assertInt("test12-2", 20, i);
}

void test13() {
  int v1 = 3;
  int v2 = 6;
  int v3;

  assertInt("test13-1", 2, v1 & v2);
  assertInt("test13-2", 7, v1 | v2);
  assertInt("test13-3", 5, v1 ^ v2);

  assertInt("test13-4", 2, v2 & v1);
  assertInt("test13-5", 7, v2 | v1);
  assertInt("test13-6", 5, v2 ^ v1);

  v3 = v1;
  v3 &= v2;
  assertInt("test13-7", 2, v3);

  v3 = v1;
  v3 |= v2;
  assertInt("test13-8", 7, v3);

  v3 = v1;
  v3 ^= v2;
  assertInt("test13-9", 5, v3);
}

void test14() {
  int i = 0;
  int j = 0;

  if (i && ++j) {
    i = 1;
  }
  assertInt("test13-1", i, 0);
  assertInt("test13-2", j, 0);

  int k = ++j || ++i;
  assertInt("test13-3", 0, i);
  assertInt("test13-4", 1, j);
  assertInt("test13-5", 1, k);

  k = i && ++j;
  assertInt("test13-6", 0, i);
  assertInt("test13-7", 1, j);
  assertInt("test13-8", 0, k);
}

void test15() {
  assertInt("test15-5", 4, sizeof(!10));
  assertInt("test15-5", 4, sizeof(!0));
  assertInt("test15-5", 1, !0);
  assertInt("test15-5", 0, !10);
}

void test16() {
  int a = 0 ? 1 : 2;
  assertInt("test15-1", 2, a);
  assertInt("test15-2", 2, 1 ? 2 : 3);
  assertInt("test15-3", 1, !0 ? 1 : 2);
  assertInt("test15-4", 3, !1 ? 2 : 3);
} 

typedef struct Nest {
  enum NestType {
    A = 1,
    B = 3,
  } type;
  char *name;
} Nest;

Nest *new_nest(enum NestType type) {
  Nest *nest = calloc(1, sizeof(Nest));
  nest->type = type;
  return nest;
}

void test17() {
  Nest *nest = new_nest(B);
  nest->name = "test17";

  assertInt("test17-1", 16, sizeof(Nest));
  assertInt("test17-2", B, nest->type);
  assertInt("test17-3", 3, nest->type);
  assertStr("test17-4", "test17", nest->name);
}

void test18() {
  int i = 0;
  int a = 0;
  while (i) {
    a = 10;
  }

  int j = 0;
  int b = 0;
  do {
    b = 10;
  } while (j);

  assertInt("test18-1", 0, a);
  assertInt("test18-2", 10, b);
}

void test19() {
  for (int i = 0; i < 10; i++) {
    assertInt("test19-1", -1, i + ~i);
  }
  for (int i = 0; i < 10; i++) {
    assertInt("test19-2", -1, i | ~i);
  }
  for (int i = 0; i < 10; i++) {
    assertInt("test19-3", 0, i & ~i);
  }
}

int v20 = 3;
void test20() {
  v20 = 5;
  assertInt("test20", 5, v20);
}

void test21() {
  assertInt("test21", 5, v20);
}

typedef int *PINT;

void test22() {
  int v1 = 3;
  int *p1 = &v1;
  assertInt("test22-1", 3, *p1);

  int v2 = 7;
  PINT p2 = &v2;
  assertInt("test22-2", 7, *p2);
}

void test23() {
  int a = 1000;
  char v1 = (char) a;
  assertInt("test23-1", -24, v1);

  int b = 2;
  bool v2 = (bool) b;
  assertInt("test23-2", 1, v2);

  int c = -1;
  bool v3 = (bool) c;
  assertInt("test23-3", 1, v3);
}

typedef union {
  char v1;
  int v2;
  short v3;
} IntUnion;

union LongUnion {
  char v1;
  int v2;
  long v3;
};
typedef union LongUnion LongUnion;

void test24() {
  assertInt("test24-1", 4, sizeof(IntUnion));
  assertInt("test24-1", 8, sizeof(union LongUnion));
  assertInt("test24-1", 8, sizeof(LongUnion));
}

void test25() {
  Vector *v = new_vector();
  assertInt("test25-1", 8, sizeof(v));

  int v1 = 7;
  int v2 = 17;
  int v3 = 29;

  int *p1;
  int *p2;
  int *p3;

  p1 = &v1;
  p2 = &v2;
  p3 = &v3;

  vec_add(v, p1);
  vec_add(v, p2);
  vec_add(v, p3);

  int *r0 = (int*) vec_get(v, 0);
  assertInt("test25-2", 7, *r0);

  int *r1 = (int*) vec_get(v, 1);
  assertInt("test25-3", 17, *r1);

  int *r2 = (int*) vec_get(v, 2);
  assertInt("test25-4", 29, *r2);
  assertInt("test25-5", 3 , v->size);
}

Char *new_char(char c) {
  Char *ins = calloc(1, sizeof(Char));
  ins->val = c;
  return ins;
}

Token *new_token(TokenKind kind, Token *cur, char *str) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  cur->next = tok;
  return tok;
}

ShortIntShort *new_short_int_short() {
  return calloc(1, sizeof(ShortIntShort));
}

void test26() {
  Char *c = new_char('Z');
  assertInt("test26-1", 8, sizeof(c));
  assertChar("test26-2", 'Z', c->val);

  Token head;
  char *str = "{}";
  Token *token = new_token(TK_RESERVED, &head, str);
  token->kind = TK_RESERVED;
  token->str = str;
  assertInt("test26-3", 8, sizeof(token));
  assertInt("test26-4", TK_RESERVED, token->kind);

  ShortIntShort *e = new_short_int_short();
  assertInt("test26-5", 8, sizeof(e));

  e->val1 = 10;
  e->val2 = 131;
  e->val3 = -7;

  assertInt("test26-6", 10, e->val1);
  assertInt("test26-7", 131, e->val2);
  assertInt("test26-8", -7, e->val3);
}

char *ptr_str = "HeLlo"; // 0
char ary_str[] = "HelLo"; // 0

char *strings[2] = {
  "Hello", // 0
  "World" // 1
};

int nums[] = {7, 11, 13};

void test27() {
  assertStr("test27-1", "HeLlo", ptr_str);
  assertStr("test27-2", "HelLo", ary_str);

  assertStr("test27-3", "Hello", strings[0]);
  assertStr("test27-4", "World", strings[1]);

  assertInt("test27-5", 7, nums[0]);
  assertInt("test27-6", 11, nums[1]);
  assertInt("test27-7", 13, nums[2]);
}

void test28() {
  int a[2];
  a[0] = 3;
  assertInt("test28-1", 3, a[0]);
}

int main() {
  test1();
  test2();
  test3();
  test4();
  test5();
  test6();
  test7();
  test8();
  test9();
  test10();
  test11();
  test12();
  test13();
  test14();
  test15();
  test16();
  test17();
  test18();
  test19();
  test20();
  test21();
  test22();
  test23();
  test24();
  test25();
  test26();
  test27();
  test28();
  return 0;
}

int add(int v1, int v2) {
  return v1 + v2;
}

typedef int (*intint2int) (int, int);
typedef int intint2int_2 (int, int);

/*
int fn1(intint2int f) {
  return f(7, 11);
}

int fn2(intint2int_2 *f) {
  return f(3, 5);
}

void test11() {
  intint2int f1 = add;
  assertInt("test11-1", 18, fn1(f1));

  intint2int_2 *f2 = add;
  assertInt("test11-1", 8, fn2(f2));
}
*/

void assert(char* name, int ret) {
  if (ret) {
    if (debug) {
      printf("%s: OK\n", name);
    }
  } else {
    printf("%s: NG\n", name);
  }
}

void assertInt(char *name, int expect, int actual) {
  if (actual == expect) {
    if (debug) {
      printf("%s: OK, expect: %d, actual: %d\n", name, expect, actual);
    }
  } else {
    printf("%s: NG, expect: %d, actual: %d\n", name, expect, actual);
  }
}
