CFLAGS=-Wall -std=c11 -g -static
SRCS=main.c token.c newtoken.c subtoken.c parser.c codegen.c reader.c debug.c vector.c
OBJS=main.o token.o newtoken.o subtoken.o parser.o codegen.o reader.o debug.o vector.o
#SRCS=$(wildcard *.c)
#OBJS=$(SRCS:.c=.o)

FILE = test

38cc: $(OBJS)
	$(CC) -o 38cc $(OBJS) $(LDFLAGS)

$(OBJS): 38cc.h $(SRCS)

.vector.c: vector.c
	cpp vector.c .vector.c

.main.c: main.c
	cpp main.c -o .main.c

.reader.c: reader.c
	cpp reader.c -o .reader.c

.token.c: token.c
	cpp token.c -o .token.c

.subtoken.c: subtoken.c
	cpp subtoken.c -o .subtoken.c

.newtoken.c: newtoken.c
	cpp newtoken.c -o .newtoken.c

.parser.c: parser.c
	cpp parser.c -o .parser.c

.codegen.c: codegen.c
	cpp codegen.c -o .codegen.c

.test.c: test.c
	cpp test.c -o .test.c

self: 38cc main.s token.s newtoken.s subtoken.s parser.s codegen.s reader.s debug.s vector.s .test.c
	$(CC) -o 38cc2 main.s token.s newtoken.s subtoken.s parser.s codegen.s reader.s debug.s vector.s $(LDFLAGS)

	./38cc2 .main.c > main2.s
	./38cc2 .test.c > test2.s
	./38cc2 .vector.c > vector2.s
	./38cc2 .reader.c > reader2.s
	./38cc2 .subtoken.c > subtoken2.s

	$(CC) -o 38cc3 main.s token.s newtoken.s subtoken2.s parser.s codegen.s reader.s debug.s vector.s $(LDFLAGS)

	./38cc3 .vector.c > vector3.s
	./38cc3 .reader.c > reader3.s
	./38cc3 .subtoken.c > subtoken3.s
	diff reader2.s reader3.s
	diff subtoken2.s subtoken3.s
	diff vector2.s vector3.s

	$(CC) test2.s vector2.s -o test2
	./test2

main.s: 38cc main.c .main.c
	$(CC) -S -masm=intel main.c -o main.s
	#./38cc .main.c > main.s

reader.s: 38cc reader.c .reader.c
	#$(CC) -S -masm=intel reader.c -o reader.s
	./38cc .reader.c > reader.s

token.s: 38cc token.c .token.c
	$(CC) -S -masm=intel token.c -o token.s
	#./38cc .token.c > token.s

subtoken.s: 38cc subtoken.c .subtoken.c
	#$(CC) -S -masm=intel subtoken.c -o subtoken.s
	./38cc .subtoken.c > subtoken.s

newtoken.s: 38cc newtoken.c .newtoken.c
	$(CC) -S -masm=intel newtoken.c -o newtoken.s
	#./38cc .newtoken.c > newtoken.s

parser.s: 38cc parser.c .parser.c
	$(CC) -S -masm=intel parser.c -o parser.s
	#./38cc .parser.c > parser.s

codegen.s: 38cc codegen.c .codegen.c
	$(CC) -S -masm=intel codegen.c -o codegen.s
	#./38cc .codegen.c > codegen.s

debug.s: debug.c
	$(CC) -S -masm=intel debug.c -o debug.s

vector.s: 38cc vector.c .vector.c
	$(CC) -S -masm=intel vector.c -o vector.s
	#./38cc .vector.c > vector.s

self-main: 38cc .main.c
	./38cc .main.c > main.s

self-reader: 38cc .reader.c
	./38cc .reader.c > reader.s

self-token: 38cc token.c .token.c
	./38cc .token.c > token.s

self-parser: 38cc .parser.c
	./38cc .parser.c > parser.s

self-codegen: 38cc codegen.c
	cpp codegen.c -o .codegen.c
	./38cc .codegen.c > codegen.s

self-debug: 38cc debug.c
	cpp debug.c -o .debug.c
	./38cc .debug.c > debug.s

self-vector: 38cc vector.c
	cpp vector.c -o .vector.c
	./38cc .vector.c > vector.s

test.s: 38cc .test.c
	./38cc .test.c > test.s

test: 38cc test.s vector.s
	$(CC) test.s vector.s -o test
	./test

test-gcc: test.c
	$(CC) -o test_gcc test.c vector.c
	./test_gcc

test-gcc-debug: 38cc test.c
	$(CC) -S -masm=intel test.c -o test_gcc.s
	cat test_gcc.s

sample: 38cc sample.c

	$(CC) -S -masm=intel sample.c -o sample-gcc.s
	gcc -o sample-gcc sample-gcc.s
	./sample-gcc

	cpp sample.c .sample.c
	./38cc .sample.c > sample-38cc.s
	gcc -o sample-38cc sample-38cc.s
	./sample-38cc

sample-s: sample-gcc.s sample-38cc.s
	gcc -o sample-gcc sample-gcc.s
	./sample-gcc
	gcc -o sample-38cc sample-38cc.s
	./sample-38cc

shtest: 38cc test.c
	./_test.sh

maptest:
	gcc map_test.c map.c -o map_test
	./map_test

clean:
	rm -f 38cc 38cc2 test test2 *.s .*.s *.o *~ tmp/* .*.c

.PHONY: test clean
