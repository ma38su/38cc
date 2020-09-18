# 38cc

38cc is a compiler for the C programming language. The compiler is able to compile itself. 

The compiler supports x86-64 Linux only. It's tested on Ubuntu-18.04(WSL2).

### Build

Run make to build:
```
make
```

To build and test:
```
make test
```
or
```
mkdir tmp
make shtest
```

To test up to 2nd generation 38cc:
```
make test2
```

To test up to 3nd generation 38cc:
```
make test3
```

To compile by 38cc:
```
cpp source.c -o tmp.c
./38cc tmp.c > tmp.s
gcc tmp.s -o tmp
```

Note:
- It requires preprocessor 'cpp' to compile codes.
- It also requires libc.

### Reference

- 低レイヤを知りたい人のためのCコンパイラ作成入門 https://www.sigbus.info/compilerbook
