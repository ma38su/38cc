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

try 11 '
int a = 11;
int main() {
  return a;
}
'

try 1 '
int main() {
  char* str = "Hello";
  return 1;
}
'

exit

