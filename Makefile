CFLAGS=-Wall -std=c11 -g -static
SRCS=main.c main_sub.c token.c token_sub.c parser.c codegen.c codegen_sub.c reader.c debug.c vector.c
OBJS=main.o main_sub.o token.o token_sub.o parser.o codegen.o codegen_sub.o reader.o debug.o vector.o
#SRCS=$(wildcard *.c)
#OBJS=$(SRCS:.c=.o)

FILE = test

38cc: $(OBJS)
	$(CC) -o 38cc $(OBJS) $(LDFLAGS)

38cc2: main.s main_sub.s token.s token_sub.s parser.s codegen.s codegen_sub.s reader.s debug.s vector.s
	$(CC) main.s main_sub.s token.s token_sub.s parser.s codegen.s codegen_sub.s reader.s debug.s vector.s -o 38cc2 $(LDFLAGS)

$(OBJS): 38cc.h $(SRCS)

.vector.c: vector.c
	cpp vector.c .vector.c

.main.c: main.c
	cpp main.c -o .main.c

.main_sub.c: main_sub.c
	cpp main_sub.c -o .main_sub.c

.reader.c: reader.c
	cpp reader.c -o .reader.c

.token.c: token.c
	cpp token.c -o .token.c

.token_sub.c: token_sub.c
	cpp token_sub.c -o .token_sub.c

.parser.c: parser.c
	cpp parser.c -o .parser.c

.codegen.c: codegen.c
	cpp codegen.c -o .codegen.c

.codegen_sub.c: codegen_sub.c
	cpp codegen_sub.c -o .codegen_sub.c

.debug.c: debug.c
	cpp debug.c -o .debug.c

test_.c: test.h test.c
	cpp test.c -o test_.c

test_gcc_.c: test.h test_gcc.c
	cpp test_gcc.c -o test_gcc_.c

sample_.c: sample.c
	cpp sample.c -o _sample.c

sample: 38cc 38cc2 sample.c
	$(CC) -O0 -S -masm=intel sample.c -o sample-gcc.s
	gcc -o sample-gcc sample-gcc.s
	./sample-gcc Hello

	cpp sample.c sample_.c
	./38cc sample_.c > sample-38cc.s
	gcc -o sample-38cc sample-38cc.s
	./sample-38cc Hello

	./38cc2 sample_.c > sample-38cc2.s
	gcc -o sample-38cc2 sample-38cc2.s
	./sample-38cc2 Hello

sample-s: sample-gcc.s sample-38cc.s sample2.s
	gcc -o sample-gcc sample-gcc.s sample2.s
	./sample-gcc
	gcc -o sample-38cc sample-38cc.s sample2.s
	./sample-38cc

sample.s: 38cc sample.c .sample.c
	./38cc .sample.c > sample.s

reader.s: 38cc reader.c .reader.c
	#$(CC) -S -masm=intel reader.c -o reader.s
	./38cc .reader.c > reader.s

token_sub.s: 38cc token_sub.c .token_sub.c
	#$(CC) -S -masm=intel token_sub.c -o token_sub.s
	./38cc .token_sub.c > token_sub.s

vector.s: 38cc vector.c .vector.c
	#$(CC) -S -masm=intel vector.c -o vector.s
	./38cc .vector.c > vector.s

codegen_sub.s: 38cc codegen_sub.c .codegen_sub.c
	$(CC) -S -masm=intel codegen_sub.c -o codegen_sub.s
	#./38cc .codegen_sub.c > codegen_sub.s

main.s: 38cc main.c .main.c
	$(CC) -S -masm=intel main.c -o main.s
	#./38cc .main.c > main.s

main_sub.s: 38cc main_sub.c .main_sub.c
	$(CC) -S -masm=intel main_sub.c -o main_sub.s
	#./38cc .main_sub.c > main_sub.s

token.s: 38cc token.c .token.c
	$(CC) -S -masm=intel token.c -o token.s
	#./38cc .token.c > token.s

parser.s: 38cc parser.c .parser.c
	$(CC) -S -masm=intel parser.c -o parser.s
	#./38cc .parser.c > parser.s

codegen.s: 38cc codegen.c .codegen.c
	$(CC) -S -masm=intel codegen.c -o codegen.s
	#./38cc .codegen.c > codegen.s

debug.s: 38cc debug.c .debug.c
	$(CC) -S -masm=intel debug.c -o debug.s
	#./38cc .debug.c > debug.s

self-main: 38cc .main.c
	./38cc .main.c > main.s

self-reader: 38cc .reader.c
	./38cc .reader.c > reader.s

self-token: 38cc token.c .token.c
	./38cc .token.c > token.s

self-parser: 38cc .parser.c
	./38cc .parser.c > parser.s

self-codegen: 38cc .codegen.c
	./38cc .codegen.c > codegen.s

self-debug: 38cc .debug.c
	./38cc .debug.c > debug.s

self-vector: 38cc .vector.c
	./38cc .vector.c > vector.s

test.s: 38cc test_.c
	./38cc test_.c > test.s

test_gcc.s: 38cc test_gcc_.c
	$(CC) -S -masm=intel test_gcc_.c -o test_gcc.s

shtest: 38cc test.sh
	./test.sh

test: 38cc test.s test_gcc.s vector.s
	$(CC) test.s test_gcc.s vector.s -o test
	./test

test-gcc: test.c test_gcc.c
	$(CC) -o test_gcc test.c test_gcc.c vector.c
	./test_gcc

self: 38cc2 test test_.c test_gcc_.c

	./38cc2 test_.c > test2.s
	./38cc2 test_gcc_.c > test2_gcc.s

	./38cc2 .vector.c > vector2.s
	diff vector.s vector2.s

	./38cc2 .reader.c > reader2.s
	diff reader.s reader2.s

	./38cc2 .token_sub.c > token_sub2.s
	diff token_sub.s token_sub2.s

	#./38cc2 .codegen_sub.c > codegen_sub2.s
	#diff codegen_sub.s codegen_sub2.s

	#./38cc2 .token.c > token2.s
	#diff token.s token2.s

	$(CC) test2.s test2_gcc.s vector2.s -o test2
	./test2

#	./38cc2 .sample.c > sample2.s
#	./38cc2 .main.c > main2.s
#	./38cc2 .test.c > test2.s
#	./38cc2 .vector.c > vector2.s
#	./38cc2 .reader.c > reader2.s
#	./38cc2 .subtoken.c > subtoken2.s

#	$(CC) -o 38cc3 main.s token.s subtoken2.s parser.s codegen.s reader.s debug.s vector.s $(LDFLAGS)

#	./38cc3 .vector.c > vector3.s
#	./38cc3 .reader.c > reader3.s
#	./38cc3 .subtoken.c > subtoken3.s
#	diff reader2.s reader3.s
#	diff subtoken2.s subtoken3.s
#	diff vector2.s vector3.s

#	$(CC) test2.s vector2.s -o test2
#	./test2

asm:
	gcc -o .asm asm.s
	./.asm

maptest:
	gcc map_test.c map.c -o map_test
	./map_test

clean:
	rm -f 38cc 38cc2 test test2 *.s .*.s *.o *~ tmp/* .*.c

.PHONY: test clean
