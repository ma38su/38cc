#!/bin/bash

try() {
  expected="$1"
  input="$2"

  #echo "./mcc \"$input\" > tmp.s"
  ./mcc "$input" > tmp.s
  cat -n tmp.s
  
  gcc -g -o tmp tmp.s
  #gdb ./tmp
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "\"$input\" => $actual is expected"
    echo ""
  else
    echo "\"$input\" => $actual is not expected. $expected is expected,"
    echo ""
    exit 1
  fi
}

try 3 '
int main() {
  int a;
  a = 3;
  return a;
}
'

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

try 11 '
int main() {
  int x;
  int *y;
  y = &x;
  *y = 11;
  return x;
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
  *a = 0;
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

exit

