#!/bin/bash

try() {
  expected="$1"
  input="$2"

  #echo "./38cc \"$input\" > tmp.s"
  ./38cc "$input" > tmp.s
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

try 1 '
int main() {
  char *hello = "Hello";
  char h = '\''h'\'';
  char a = hello[0];
  return a == h;
}
'

try 0 '
int main() {
  puts("Hello");
  return 0;
}
'

exit

