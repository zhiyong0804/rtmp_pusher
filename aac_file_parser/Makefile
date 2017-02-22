
CC=gcc
LD=gcc
AR=ar

CXX_OBJ=$(patsubst %.c,%.o,$(wildcard *.c))
SOURCES=$(shell find . -maxdepth 1 -name "*.o")
LIB=libAacFileParser.a

CFLAGS=$(INC) -Wall -Wno-narrowing -Wno-unused -g
ARFLAGS=rcs


#all: $(CXX_OBJ) $(LIB) 
all: $(CXX_OBJ) $(LIB) $(BIN)

build: clean all

clean:
	-rm *.o libAacFileParser.a $(BIN)
.c.o:
	@echo "CC " $<
	@$(CC) $(CFLAGS) -c $<
$(LIB):
	@echo "AR " $(LIB)
	$(AR) $(ARFLAGS) $(LIB) $(CXX_OBJ) $(LIB_A)

