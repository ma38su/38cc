CFLAGS=-Wall -std=c11 -g -static
SRCS=main.c token.c parser.c codegen.c reader.c debug.c vector.c
OBJS=main.o token.o parser.o codegen.o reader.o debug.o vector.o
#SRCS=$(wildcard *.c)
#OBJS=$(SRCS:.c=.o)

FILE = test

38cc: $(OBJS)
	$(CC) -o 38cc $(OBJS) $(LDFLAGS)

$(OBJS): 38cc.h $(SRCS)

test: 38cc test.c
	cpp test.c -o .test.c
	ulimit -c unlimited
	./38cc .test.c > test.s
	$(CC) test.s -o test
	./test

test-s: test.s
	$(CC) test.s -o test
	./test

test-gcc: test.c
	$(CC) -o test_gcc test.c
	./test_gcc

test-debug: 38cc test.c
	ulimit -c unlimited
	cpp test.c -o .test.c
	./38cc .test.c > test.s
	#cat .test.c
	#$(CC) -S -masm=intel .test.c -o gcc_test.s
	#cat gcc_test.s
	cat test.s
	$(CC) test.s -o test
	./test

test-gcc-debug: 38cc test.c
	$(CC) -S -masm=intel test.c -o test_gcc.s
	cat test_gcc.s

shtest: 38cc test.c
	./_test.sh

maptest:
	gcc map_test.c map.c -o map_test
	./map_test

selfhost: 38cc vector.c
	cat vector.c | cpp > .vector.c
	./38cc .vector.c > vector.s
	cat vector.s

debug: 38cc
	./_debug.sh

asm:
	./38cc tmp/tmp.s > tmp/38cc.s
	gcc -o tmp/38cc_exe tmp/38cc.s
	./tmp/38cc_exe

clean:
	rm -f 38cc *.s *.o *~ tmp/* .*.c

gcc:
	gcc -S -masm=intel ${FILE}.c -o ${FILE}.s
	cat ${FILE}.s
	gcc ${FILE}.s -o ${FILE}
	./${FILE}

.PHONY: test clean

