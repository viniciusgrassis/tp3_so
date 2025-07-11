CC = gcc

CFLAGS = -Wall -g

EXEC = fatsim

SRCS = main.c fat_commands.c fat_helpers.c

OBJS = $(SRCS:.c=.o)

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $(EXEC) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(EXEC)

run: all
	./$(EXEC)