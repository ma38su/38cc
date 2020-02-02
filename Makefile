CFLAGS=-std=c11 -g -static
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

mcc: $(OBJS)
	$(CC) -g -o mcc $(OBJS) $(LDFLAGS)

$(OBJS): mcc.h

test: mcc
	./_test.sh

debug: mcc
	./_debug.sh

clean:
	rm -f mcc *.o *~ tmp*

.PHONY: test clean

