LIB_SRC = $(wildcard *.c )  
#LIB_OBJ = $(patsubst %.c,%.o,$(notdir $(LIB_SRC))) 
LIB_OBJ = $(patsubst %.c,%.o,$(LIB_SRC))

LIB_TARGET = libzhttp.a

CC=$(CCPRE)gcc
AR=$(CCPRE)ar

CFLAGS =  -fPIC -I../incl
LDFLAGS += -lpthread
#all:
#	@echo $(LIB_SRC)
#	@echo $(LIB_OBJ)
#
$(LIB_TARGET): $(LIB_OBJ)
	$(AR) -r $@ $^
%.o:%.c
	@$(CC) $(CFLAGS) -o $@ -c $^ $(LDFLAGS) -static

clean:
	rm *.o *.a  
