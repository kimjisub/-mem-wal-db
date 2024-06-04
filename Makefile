CC = gcc

CFLAGS = -Wall -g

TARGET = memwaldb

SRCDIR = src
SRCS = $(SRCDIR)/main.c $(SRCDIR)/core.c $(SRCDIR)/hash_table.c

OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

rebuild: clean all