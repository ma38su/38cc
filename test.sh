#!/bin/bash


try() {
  expected="$1"
  input="$2"

  #echo "./mcc \"$input\" > tmp.s"
  ./mcc "$input" > tmp.s
  cat tmp.s
  
  gcc -o tmp tmp.s
  ./tmp
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

try 0 'main() {return 0;}'

try 42 'main() {return 42;}'

try 21 'main() {return 5+20-4;}'

try 26 'main() {return 2*3 + 4 * 5;}'
try 70 'main() {return 2*(3 + 4) * 5;}'
try 7 'main() {return -3+10;}'
try 1 'main() {return 1 == 1;}'
try 0 'main() {return 1 == 2;}'
try 0 'main() {return 1 != 1;}'
try 1 'main() {return 2 != 1;}'
try 1 'main() {return 1 != 2;}'
try 1 'main() {return 2 < 3;}'
try 0 'main() {return 3 < 2;}'
try 1 'main() {return 2 <= 2;}'
try 1 'main() {2 <= 3;}'
try 0 'main() {3 <= 2;}'
try 1 'main() {2 >= 2;}'
try 1 'main() {3 >= 2;}'
try 0 'main() {2 >= 3;}'
try 0 'main() {6 % 3;}'
try 1 'main() {3 % 2;}'
try 5 'main() {41 % 9;}'
try 10 'main() {return test=10;}'
try 10 'main() {return c=10;}'
try 11 'main() {c=a=5;return c+a+1;}'
try 11 'main() {ca=ab=5;ca+ab+1;}'
try 5 'main() {return 5; return 8;}'

try 13 'main() {return 2 + 11; return 3 + 21;}'
try 9 'main() {if (1) return 9; return 7;}'
try 13 'main() {if (0) return 11; return 13;}'
try 5 'main() {if (0) return 3; else return 5;}'
try 3 'main() {if (1) return 3; else return 5;}'
try 8 'main() {i = 3; i = i + 5; return i;}'
try 10 'main() {i = 1; while (i < 10) i = i + 1; return i;}'
try 1 'main() {i = 1; while (i < 10) return i;}'
try 5 'main() {for (i = 5; i < 10; i = i + 1) return i;}'
try 45 '
main() {
  sum = 0;
  for (i = 1; i < 10; i = i + 1)
    sum = sum + i;
  return sum;
}
'

# block
try 5 '
main() {
  sum = 0;
  for (i = 5; i < 10; i = i + 1) {
    sum = sum + i;
    return sum;
  }
}
'

try 21 '
foo() {
  return 21;
}
main() {
  return foo();
}
'

try 45 '
main() {
  sum = 0;
  for (i = 1; i < 10; i = i + 1) {
    sum = sum + i;
  }
  return sum;
}
'

try 45 '
sum() {
  sum = 0;
  for (i = 1; i < 10; i = i + 1) {
    sum = sum + i;
  }
  return sum;
}
main() {
  sum = sum();
  return sum;
}
'

try 15 '
add(a, b) {
  return a + b;
}
main() {
  return add(9, 6);
}
'

try 55 '
fib(a) {
  if (a <= 0) {
    return 0;
  }
  if (a == 1) {
    return 1;
  }
  return fib(a - 1) + fib(a - 2);
}
main() {
  return fib(10);
}
'

try $((0xff - 4 + 1)) 'main() {return -4;}'

exit

