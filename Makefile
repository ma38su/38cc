CFLAGS=-std=c11 -g -static
SRCS=$(winldcard *.c)
OBJS=$(SRC:.c=.o)

mcc: $(OBJS)
		$(CC) -o mcc $(OBJS) $(LDFLAGS)

$(OBJS): mcc.h

test: mcc
		./test.sh

clean:
		rm -f mcc *.o *~ tmp*

.PHONY: test clean

