# Usage:
# make        # compile all binary
# make clean  # remove ALL binaries and objects

.PHONY = all clean

CC = gcc# compiler to use
CCFLAGS = -fsanitize=address -Wall -std=c11 -D_POSIX_C_SOURCE=200112L -lcrypto 
LINKERFLAG = -lm

SOURCES := $(wildcard *.c)
HTTP_PARSERS_SOURCES := $(wildcard parsers/HTTP_parsers/*.c)
PARSERS_SOURCES := $(wildcard parsers/*.c)
OBJECTS := $(SOURCES:%.c=%.o)
HTTP_PARSERS_OBJECTS := $(HTTP_PARSERS_SOURCES:%.c=%.o)

BINARIES := $(SOURCES:%.c=%.out)
HTTP_BINARIES := $(HTTP_PARSERS_SOURCES:%.c=%.out)

complete: clean all

all:
	@echo "Creating object..."
	${CC}  $(SOURCES) $(HTTP_PARSERS_SOURCES) $(PARSERS_SOURCES) $(CCFLAGS)-o httpd

clean:
	@echo "Cleaning up..."
	rm -rvf *.o proxy.log
