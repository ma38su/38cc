#!/bin/bash

ulimit -c unlimited

try() {
  expected="$1"
  input="$2"

  #echo "./38cc \"$input\" > tmp.s"
  echo "$input" | cpp > tmp/tmp.c

  ./38cc tmp/tmp.c > tmp/38cc.s
  #gcc -no-pie -g -o tmp/exe_38cc tmp/38cc.s
  gcc -g -o tmp/exe_38cc tmp/38cc.s
  ./tmp/exe_38cc
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "--- 38cc ---"
    cat -n tmp/38cc.s
    echo ""

    echo "\"$input\" => $actual is expected"
    echo ""
  else
    echo "--- 38cc ---"
    ./38cc tmp/tmp.c
    echo ""

    echo "\"$input\" => $actual is not expected. $expected is expected."


    gcc -S -masm=intel tmp/tmp.c -o tmp/gcc.s
    echo "--- gcc ---"
    cat -n tmp/gcc.s
    echo ""
  
    gcc -o tmp/exe_gcc tmp/gcc.s
    ./tmp/exe_gcc
    gcc_ret="$?"
    echo "gcc output is \"$gcc_ret\". $expected is expected."
    echo ""

    exit 1
  fi
}

try 1 '
#include <stdio.h>

int main() {
  int i = 1;
  printf("1 + 10 = %d\n", i + 10);
  return 1;
}
'
