# 38cc

38cc is a compiler for the C programming language. The compiler supports only limited features.

The compiler supports x86-64 Linux only. It's tested on Ubuntu-18.04(WSL2).

### Build

Run make to build:
```
make
```

Run make to build and test:
```
mkdir tmp
make shtest
```
or
```
make test
```

Run 38cc to compile:
```
cpp source.c -o tmp.c
./38cc tmp.c > tmp.s
gcc tmp.s -o tmp
```


Note: It doesn't include preprocessor. It requires proprocessor 'cpp' to compile a following code that includes stdio.h.

```
#include <stdio.h>

int main() {
  printf("Hello, World!!\n");
}
```

### Author
ma38su

### Reference

- 低レイヤを知りたい人のためのCコンパイラ作成入門 https://www.sigbus.info/compilerbook
