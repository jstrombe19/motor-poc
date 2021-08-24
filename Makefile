CC=gcc
CFLAGS=-g -Wall -I/opt/picoscope/include
SRC=src
OBJ=obj
SRCS=$(wildcard $(SRC)/*.c)
OBJS=$(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SRCS))
BIN=bin
EXE=$(BIN)/motor-control-cli


all: $(EXE)

$(EXE): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ -lm -pthread -lncurses -L/opt/picoscope/lib/ -lpicohrdl

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) -r $(BIN)/* $(OBJ)/* *dSYM