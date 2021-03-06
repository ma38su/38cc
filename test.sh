#!/bin/bash

try() {
  expected="$1"
  input="$2"

  echo "$input" | cpp > tmp/tmp.c
  ./38cc tmp/tmp.c > tmp/38cc.s
  #cat -n tmp_38cc.s
  gcc -g -o tmp/exe tmp/38cc.s
  ./tmp/exe
  actual="$?"

  if [ "$actual" != "$expected" ]; then
    #echo "\"$input\" => $actual is expected"
    #echo ""
  #else
    echo "\"$input\" => $actual is not expected. $expected is expected,"
    echo ""
    exit 1
  fi
}

try 0 'int main() {return 0;}'

try 42 'int main() {return 42;}'

try 21 'int main() {return 5+20-4;}'

try 26 'int main() {return 2*3 + 4 * 5;}'
try 70 'int main() {return 2*(3 + 4) * 5;}'
try 7 'int main() {return -3+10;}'
try 1 'int main() {return 1 == 1;}'
try 0 'int main() {return 1 == 2;}'
try 0 'int main() {return 1 != 1;}'
try 1 'int main() {return 2 != 1;}'
try 1 'int main() {return 1 != 2;}'
try 1 'int main() {return 2 < 3;}'
try 0 'int main() {return 3 < 2;}'
try 1 'int main() {return 2 <= 2;}'
try 1 'int main() {2 <= 3;}'
try 0 'int main() {3 <= 2;}'
try 1 'int main() {2 >= 2;}'
try 1 'int main() {3 >= 2;}'
try 0 'int main() {2 >= 3;}'
try 0 'int main() {6 % 3;}'
try 1 'int main() {3 % 2;}'
try 5 'int main() {41 % 9;}'
try 10 '
int main() {
  int test;
  return test=10;
}'
try 11 '
int main() {
  int ca;
  int ab;
  ca=ab=5;
  return ca+ab+1;
}'
try 5 'int main() {return 5; return 8;}'

try 13 'int main() {return 2 + 11; return 3 + 21;}'

try 9 'int main() {if (1) return 9; return 7;}'

try 13 'int main() {if (0) return 11; return 13;}'

try 5 'int main() {if (0) return 3; else return 5;}'

try 3 'int main() {if (1) return 3; else return 5;}'

try 8 '
int main() {
  int i;
  i = 3;
  i = i + 5;
  return i;
}'

try 1 '
int main() {
  int i = 0;
  i += 1;
  return i;
}'

try 4 '
int main() {
  int i = 5;
  i -= 1;
  return i;
}'

try 65 '
int main() {
  int i = 5;
  i *= 13;
  return i;
}'

try 3 '
int main() {
  int i = 12;
  i /= 4;
  return i;
}'

try 3 '
int main() {
  int i = 21;
  i /= 6;
  return i;
}'

try 10 '
int main() {
  int i;
  i = 1;
  while (i < 10) i += 1;
  return i;
}'

try 1 '
int main() {
  int i;
  i = 1;
  while (i < 10) return i;
}'

try 5 '
int main() {
  int i;
  for (i = 5; i < 10; ++i) return i;
}'

try 45 '
int main() {
  int sum;
  int i;
  sum = 0;
  for (i = 1; i < 10; i = i + 1)
    sum = sum + i;
  return sum;
}
'

try 4 '
int main() {
  int i = 3;
  return ++i;
}
'

try 4 '
int main() {
  int i = 3;
  ++i;
  return i;
}
'

try 4 '
int main() {
  int i = 5;
  --i;
  return i;
}
'

try 4 '
int main() {
  int i = 5;
  return --i;
}
'

# block
try 5 '
int main() {
  int i;
  int sum;
  sum = 0;
  for (i = 5; i < 10; i = i + 1) {
    sum = sum + i;
    return sum;
  }
}
'

try 21 '
int foo() {
  return 21;
}
int main() {
  return foo();
}
'

try 45 '
int main() {
  int sum;
  int i;
  sum = 0;
  for (i = 1; i < 10; i = i + 1) {
    sum = sum + i;
  }
  return sum;
}
'

try 45 '
int sum() {
  int i;
  int sum;
  sum = 0;
  for (i = 1; i < 10; i = i + 1) {
    sum = sum + i;
  }
  return sum;
}
int main() {
  return sum();
}
'

try 15 '
int add(int a, int b) {
  return a + b;
}
int main() {
  return add(9, 6);
}
'

try 55 '
int fib(int a) {
  if (a <= 0) {
    return 0;
  }
  if (a == 1) {
    return 1;
  }
  return fib(a - 1) + fib(a - 2);
}
int main() {
  return fib(10);
}
'

try 6 '
int sum(int a, int b, int c) {
  return a + b + c;
}
int main() {
  int a;
  a = sum(1, 2, 3);
  return a;
}
'

try 11 '
int main() {
  int x;
  int *y;
  x = 5;
  y = &x;
  *y = 11;
  return *y;
}
'

try 11 '
int main() {
  int x;
  int *y;
  x = 3;
  y = &x;
  *y = 11;
  return x;
}
'

try 4 '
int main() {
  int x;
  return sizeof(x);
}
'

try 4 '
int main() {
  int x;
  return sizeof(x+3);
}
'

try 8 '
int main() {
  int *x;
  return sizeof(x);
}
'

try 8 '
int main() {
  int *x;
  return sizeof(x+3);
}
'

try 4 '
int main() {
  int *x;
  return sizeof(*x);
}
'

try 8 '
int main() {
  int x;
  x = 10;
  return sizeof(&x);
}
'

try 4 '
int main() {
  return sizeof(1);
}
'

try 8 '
int main() {
  return sizeof(sizeof(1));
}
'

try 40 '
int main() {
  int a[10];
  return sizeof(a);
}
'

try 3 '
int main() {
  int a[2];
  *a = 3;
  return *a;
}
'

try 3 '
int main() {
  int a[2];
  a[0] = 3;
  return *a;
}
'

try 3 '
int main() {
  int a[2];
  *a = 3;
  return a[0];
}
'


try 3 '
int main() {
  int a[2];
  a[0] = 3;
  return a[0];
}
'

try 3 '
int main() {
  int a[2];
  a[0] = 5;
  a[1] = 3;
  return a[1];
}
'

try 8 '
int main() {
  int a[2];
  a[0] = 5;
  a[1] = 3;
  return a[1] + a[0];
}
'

try 1 '
int main() {
  int a[2];
  *a = 1;
  *(a + 1) = 2;
  return a[0];
}
'

try 2 '
int main() {
  int a[2];
  *(a + 1) = 2;
  return a[1];
}
'

try 3 '
int main() {
  int a[2];
  *a = 1;
  *(a + 1) = 2;
  int *p;
  p = a;
  return *p + *(p + 1);
}
'

try 29 '
int a = 11;
char b = 8;
short c = 10;
int main() {
  return a + b + c;
}
'

try 6 '
int main() {
  int x[3];
  x[0] = 1;
  x[1] = 2;
  int y;
  y = 4;
  return x[1] + 4;
}
'

try 3 '
int main() {
  // line comment test * test
  char x[3];
  x[0] = -1;
  x[1] = 2;
  /* block * / * /* comment */
  int y;
  y = 4;
  return x[0] + y;
}
'

try 104 '
int main() {
  char a = '\''h'\'';
  return a;
}
'

try 1 '
#include <stdio.h>

int main() {
  char* str0 = "Hello0";
  char* str1 = "Hello1";
  char* str2 = "Hello2";
  char* str3 = "Hello3";
  char* str4 = "Hello4";
  char* str5 = "Hello5";
  char* str6 = "Hello6";
  char* str7 = "Hello7";
  char* str8 = "Hello8";
  char* str9 = "Hello9";
  char* str10 = "Hello10";
  char* str11 = "Hello11";
  char* str12 = "Hello12";
  char* str13 = "Hello13";
  char* str14 = "Hello14";
  return 1;
}
'

exit

try $((0xff - 4 + 1)) 'int main() {return -4;}'


