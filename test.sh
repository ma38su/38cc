#!/bin/bash

try() {
  expected="$1"
  input="$2"

  ./mcc "$input" > tmp.s
  cat tmp.s
  gcc -o tmp tmp.s
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual is expected"
    echo ""
  else
    echo "$input => $expected is expected, but actual is => $actual"
    echo ""
    exit 1
  fi
}

try 0 '0;'
try 42 '42;'
try 21 '5+20-4;'
try 21 '5 + 20 - 4;'
try 41 ' 12 + 34 - 5;'
try 26 ' 2*3 + 4 * 5;'
try 70 ' 2*(3 + 4) * 5;'
try 7 '-3+10;'
try 1 '1 == 1;'
try 0 '1 == 2;'
try 0 '1 != 1;'
try 1 '2 != 1;'
try 1 '1 != 2;'
try 1 '2 < 3;'
try 0 '3 < 2;'
try 1 '2 <= 2;'
try 1 '2 <= 3;'
try 0 '3 <= 2;'
try 1 '2 >= 2;'
try 1 '3 >= 2;'
try 0 '2 >= 3;'
try 0 '6 % 3;'
try 1 '3 % 2;'
try 5 '41 % 9;'
try 10 'test=10;'
try 10 'c=10;'
try 11 'c=a=5;c+a+1;'
try 11 'ca=ab=5;ca+ab+1;'
try 5 'return 5; return 8;'
try 13 'return 2 + 11; return 3 + 21;'

try -3 '-3;'

echo OK

