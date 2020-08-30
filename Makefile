CFLAGS=-Wall -std=c11 -g -static
SRCS=main.c token.c parser.c codegen.c reader.c debug.c vector.c
OBJS=main.o token.o parser.o codegen.o reader.o debug.o vector.o
#SRCS=$(wildcard *.c)
#OBJS=$(SRCS:.c=.o)

FILE = test

38cc: $(OBJS)
	$(CC) -o 38cc $(OBJS) $(LDFLAGS)

$(OBJS): 38cc.h $(SRCS)

self-main: 38cc main.c
	cpp main.c -o .main.c
	./38cc .main.c > .main.s

self-reader: 38cc reader.c
	cpp reader.c -o .reader.c
	./38cc .reader.c > .reader.s

self-token: 38cc token.c
	cpp token.c -o .token.c
	./38cc .token.c > .token.s

self-parser: 38cc parser.c
	cpp parser.c -o .parser.c
	./38cc .parser.c > .parser.s

self-codegen: 38cc codegen.c
	cpp codegen.c -o .codegen.c
	./38cc .codegen.c > .codegen.s

self-debug: 38cc debug.c
	cpp debug.c -o .debug.c
	./38cc .debug.c > .debug.s

self-vector: 38cc vector.c
	cpp vector.c -o .vector.c
	./38cc .vector.c > .vector.s


.test.c: 38cc test.c
	cpp test.c -o .test.c

.test.s: 38cc .test.c
	./38cc .test.c > .test.s

test: 38cc .test.s
	$(CC) .test.s -o test
	./test

test-gcc: test.c
	$(CC) -o test_gcc test.c
	./test_gcc

test-gcc-debug: 38cc test.c
	$(CC) -S -masm=intel test.c -o test_gcc.s
	cat test_gcc.s

shtest: 38cc test.c
	./_test.sh

maptest:
	gcc map_test.c map.c -o map_test
	./map_test

clean:
	rm -f 38cc *.s *.o *~ tmp/* .*.c

.PHONY: test clean
