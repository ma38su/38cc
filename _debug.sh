#!/bin/bash

try() {
  expected="$1"
  input="$2"

  #echo "./38cc \"$input\" > tmp.s"
  echo "$input" > tmp/tmp.c

  ./38cc tmp/tmp.c > tmp/38cc.s
  gcc -g -o tmp/exe_38cc tmp/38cc.s
  ./tmp/exe_38cc
  actual="$?"
  echo "\"$input\" => $actual is not expected. $expected is expected,"

  echo "--- 38cc ---"
  cat -n tmp/38cc.s
  echo ""

  if [ "$actual" = "$expected" ]; then
    echo "\"$input\" => $actual is expected"
    echo ""
  else

    gcc -S -masm=intel -S tmp/tmp.c -o tmp/gcc.s
    echo "--- gcc ---"
    cat -n tmp/gcc.s
    echo ""
  
    gcc -g -o tmp/exe_gcc tmp/gcc.s
    ./tmp/exe_gcc
    gcc_ret="$?"
    echo "gcc output is \"$gcc_ret\"."
    echo ""

    exit 1
  fi
}


try 0 '
#include <stdio.h>

int main() {
  puts("Hello");
  return 0;
}
'

exit

try 1 '
int main() {
  char *hello = "hello";
  //char h = '\''h'\'';
  //char a = hello[1];
  //return a == h;
  return hello[0];
}
'
