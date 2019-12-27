CFLAGS=-std=c11 -g -static
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

mcc: $(OBJS)
	$(CC) -o mcc $(OBJS) $(LDFLAGS)

$(OBJS): mcc.h

test: mcc
	echo "!!! basic !!!"
	./test.sh
	echo "!!! link !!!"
	./link-test.sh

clean:
	rm -f mcc *.o *~ tmp*

.PHONY: test clean

