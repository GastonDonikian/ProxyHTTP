# Usage:
# make        # compile all binary
# make clean  # remove ALL binaries and objects

.PHONY = all clean

CC = gcc# compiler to use

LINKERFLAG = -lm

SOURCES := $(wildcard *.c)
OBJECTS := $(SOURCES:%.c=%.o)
BINARIES := $(SOURCES:%.c=%.out)

complete: clean all

all:
	@echo "Creating object..."
	${CC} -c -g $(SOURCES)


clean:
	@echo "Cleaning up..."
	rm -rvf *.o ${BINARIES}