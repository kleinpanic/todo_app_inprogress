CC = gcc
CFLAGS = -Wall -g
OBJDIR = ../obj
SRCDIR = ../src
INCDIR = ../include
BUILDDIR = .
BINDIR = .

OBJS = $(OBJDIR)/main.o $(OBJDIR)/task.o
EXEC = $(BINDIR)/todo

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ -lncurses

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -I $(INCDIR) -c $< -o $@

clean:
	rm -f $(OBJDIR)/*.o $(EXEC)

.PHONY: all clean
