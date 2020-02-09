CFLAGS=-std=c11 -g -static
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

38cc: $(OBJS)
	$(CC) -g -o 38cc $(OBJS) $(LDFLAGS)

$(OBJS): 38cc.h

test: 38cc
	./_test.sh

debug: 38cc
	./_debug.sh

clean:
	rm -f 38cc *.o *~ tmp/*

.PHONY: test clean

