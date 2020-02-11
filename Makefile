CFLAGS=-Wall -std=c11 -g -static
SRCS=main.c token.c parser.c codegen.c reader.c debug.c vector.c
OBJS=main.o token.o parser.o codegen.o reader.o debug.o vector.o
#SRCS=$(wildcard *.c)
#OBJS=$(SRCS:.c=.o)

38cc: $(OBJS)
	$(CC) -g -o 38cc $(OBJS) $(LDFLAGS)

$(OBJS): 38cc.h $(SRCS)

test: 38cc test.c
	ulimit -c unlimited
	cpp test.c -o .test.c
	./38cc .test.c > test.s
	cat .test.c
	cat test.s
	$(CC) test.s -o test
	./test
	$(CC) -S -masm=intel .test.c -o gcc_test.s
	cat gcc_test.s

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

clean:
	rm -f 38cc *.s *.o *~ tmp/* .*.c

.PHONY: test clean

