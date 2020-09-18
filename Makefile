CFLAGS=-Wall -std=c11 -g -static
SRCS=main.c token.c parser.c codegen.c reader.c debug.c vector.c
OBJS=main.o token.o parser.o codegen.o reader.o debug.o vector.o
#SRCS=$(wildcard *.c)
#OBJS=$(SRCS:.c=.o)

FILE = test

38cc: $(OBJS)
	$(CC) -o 38cc $(OBJS) $(LDFLAGS)

38cc2: main.s reader.s token.s parser.s codegen.s debug.s vector.s
	$(CC) main.s reader.s token.s parser.s codegen.s debug.s vector.s -o 38cc2 $(LDFLAGS)

38cc3: main2.s reader2.s token2.s parser2.s codegen2.s debug.s vector2.s
	$(CC) main2.s reader2.s token2.s parser2.s codegen2.s debug.s vector2.s -o 38cc3 $(LDFLAGS)

$(OBJS): 38cc.h $(SRCS)

shtest: 38cc test.sh
	./test.sh

test: 38cc test.s test_gcc.s vector.s
	$(CC) test.s test_gcc.s vector.s -o test
	./test

test-gcc: test.c test_gcc.c
	$(CC) -o test_gcc test.c test_gcc.c vector.c
	./test_gcc

test2: 38cc2 test.s test2.s test_gcc.s vector2.s
	$(CC) test2.s test_gcc.s vector2.s -o test2
	diff test.s test2.s
	./test2

test3: 38cc3 test2 test3.s test_gcc.s vector3.s
	$(CC) test3.s test_gcc.s vector3.s -o test3
	diff test2.s test3.s
	./test3

vector_.c: vector.c
	cpp vector.c vector_.c

main_.c: main.c
	cpp main.c -o main_.c

reader_.c: reader.c
	cpp reader.c -o reader_.c

token_.c: token.c
	cpp token.c -o token_.c

parser_.c: parser.c
	cpp parser.c -o parser_.c

codegen_.c: codegen.c
	cpp codegen.c -o codegen_.c

codegen_sub_.c: codegen_sub.c
	cpp codegen_sub.c -o codegen_sub_.c

debug_.c: debug.c
	cpp debug.c -o debug_.c

test_.c: test.h test.c
	cpp test.c -o test_.c

test_gcc_.c: test.h test_gcc.c
	cpp test_gcc.c -o test_gcc_.c

sample_.c: sample.c
	cpp sample.c -o sample_.c

sample: 38cc 38cc2 sample_.c
	$(CC) -O0 -S -masm=intel sample.c -o sample-gcc.s
	gcc -o sample-gcc sample-gcc.s
	./sample-gcc Hello

	./38cc sample_.c > sample-38cc.s
	gcc -o sample-38cc sample-38cc.s
	./sample-38cc Hello

	./38cc2 sample_.c > sample-38cc2.s
	gcc -o sample-38cc2 sample-38cc2.s
	./sample-38cc2 Hello

main.s: 38cc main_.c
	./38cc main_.c > main.s

main2.s: 38cc2 main_.c main.s
	./38cc2 main_.c > main2.s
	diff main.s main2.s

vector.s: 38cc vector_.c
	./38cc vector_.c > vector.s

vector2.s: 38cc2 vector_.c vector.s
	./38cc2 vector_.c > vector2.s
	diff vector.s vector2.s

vector3.s: 38cc3 vector_.c vector2.s
	./38cc3 vector_.c > vector3.s
	diff vector2.s vector3.s

reader.s: 38cc reader_.c
	./38cc reader_.c > reader.s

reader2.s: 38cc2 reader_.c reader.s
	./38cc2 reader_.c > reader2.s
	diff reader.s reader2.s

token.s: 38cc token_.c
	./38cc token_.c > token.s

token2.s: 38cc2 token_.c token.s
	./38cc2 token_.c > token2.s
	diff token.s token2.s

parser.s: 38cc parser_.c
	./38cc parser_.c > parser.s

parser2.s: 38cc2 parser_.c parser.s
	./38cc2 parser_.c > parser2.s
	diff parser.s parser2.s

codegen.s: 38cc codegen_.c
	./38cc codegen_.c > codegen.s

codegen2.s: 38cc2 codegen_.c codegen.s
	./38cc2 codegen_.c > codegen2.s
	diff codegen.s codegen2.s

debug.s: 38cc debug.c debug_.c
	$(CC) -S -masm=intel debug.c -o debug.s
	#./38cc debug_.c > debug.s

test.s: 38cc test_.c
	./38cc test_.c > test.s

test2.s: 38cc2 test_.c test.s
	./38cc2 test_.c > test2.s
	diff test.s test2.s

test3.s: 38cc3 test_.c test2.s
	./38cc3 test_.c > test3.s
	diff test2.s test3.s

test_gcc.s: 38cc test_gcc_.c
	$(CC) -S -masm=intel test_gcc_.c -o test_gcc.s

sample-s: sample-gcc.s sample-38cc.s
	gcc -o sample-gcc sample-gcc.s
	./sample-gcc Hello
	gcc -o sample-38cc sample-38cc.s
	./sample-38cc Hello

sample.s: 38cc sample.c .sample.c
	./38cc .sample.c > sample.s

maptest:
	gcc map_test.c map.c -o map_test
	./map_test

clean:
	rm -f 38cc 38cc2 38cc3 test test2 test3 *.s .*.s *.o *~ tmp/* *_.c

all: 38cc

.PHONY: test clean
