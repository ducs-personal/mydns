SRC = $(wildcard *.c )
OBJ = $(patsubst %.c, %, $(SRC))

CC=$(CCPRE)gcc

CFLAGS = -g -Wall -fPIC  -I../incl/  
LDFLAGS += -lpthread -L../src/ -lzhttp


all:$(OBJ)
$(OBJ):%:%.c
	$(CC)  $^ -o $@  $(CFLAGS) $(LDFLAGS)

clean:
	rm *.o *.a $(OBJ)  -fr
