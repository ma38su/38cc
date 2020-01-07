#!/bin/bash

try() {
  expected="$1"
  input="$2"

  #echo "./mcc \"$input\" > tmp.s"
  ./mcc "$input" > tmp.s
  cat -n tmp.s
  
  gcc -g -o tmp tmp.s
  gdb ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "\"$input\" => $actual is expected"
    echo ""
  else
    echo "\"$input\" => $expected is expected, but actual is => $actual"
    echo ""
    exit 1
  fi
}

try 3 '
int main() {
  int x;
  int *y;
  y = &x;
  *y = 3;
  return x;
}
'

exit

