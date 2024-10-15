# Makefile

CC = gcc
CFLAGS = -Wall -g
OBJDIR = ../obj
SRCDIR = ../src
INCDIR = ../include
BUILDDIR = .
BINDIR = ./binary

OBJS = $(OBJDIR)/main.o $(OBJDIR)/task.o
EXEC = $(BINDIR)/todo

all: $(BINDIR) $(EXEC)

$(BINDIR):
	mkdir -p $(BINDIR)

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ -lncurses -lpthread

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -I $(INCDIR) -c $< -o $@

clean:
	rm -f $(OBJDIR)/*.o $(EXEC)
	rm -rf $(BINDIR)

install: $(EXEC)
	install -m 755 $(EXEC) /usr/local/bin/todo

uninstall:
	rm -f /usr/local/bin/todo

.PHONY: all clean install uninstall
