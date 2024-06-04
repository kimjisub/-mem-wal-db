CC = gcc

CFLAGS = -Wall -g

TARGET = memwaldb

SRCDIR = src
HDRDIR = include
SRCS = $(SRCDIR)/main.c $(SRCDIR)/core.c $(SRCDIR)/hash_table.c
HDRS = $(HDRDIR)/main.h $(HDRDIR)/core.h $(HDRDIR)/hash_table.h

OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

rebuild: clean all