# Usage:
# make        # compile all binary
# make clean  # remove ALL binaries and objects

.PHONY = all clean

CC = gcc# compiler to use

LINKERFLAG = -lm

SOURCES := $(wildcard *.c)
HTTP_PARSERS_SOURCES := $(wildcard HTTP_parsers/*.c)
OBJECTS := $(SOURCES:%.c=%.o)
HTTP_PARSERS_OBJECTS := $(HTTP_PARSERS_SOURCES:%.c=%.o)
BINARIES := $(SOURCES:%.c=%.out)
HTTP_BINARIES := $(HTTP_PARSERS_SOURCES:%.c=%.out)

complete: clean all

all: OBJECTS
	@echo "Checking..."
	${CC} ${LINKERFLAG} $(OBJECTS) $(HTTP_PARSERS_OBJECTS) -o http.out

OBJECTS: 
	@echo "Creating object..."
	${CC} $(SOURCES) $(HTTP_PARSERS_SOURCES) -o http.out

clean:
	@echo "Cleaning up..."
	rm -rvf *.o ${BINARIES} ${HTTP_PARSER_BINARIES}