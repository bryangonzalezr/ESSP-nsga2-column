# Makefile for compiling NSGA-II source code
CC=gcc
LD=gcc
RM=rm -f
CFLAGS=-Wall -ansi -pedantic -g -std=c99
OBJS:=$(patsubst %.c,%.o,$(wildcard *.c))
MAIN=nsga2r
all:$(MAIN)
$(MAIN):$(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $(MAIN) -lm -std=c99
%.o: %.c global.h rand.h
	$(CC) $(CFLAGS) -c $<
clean:
	$(RM) $(OBJS)

