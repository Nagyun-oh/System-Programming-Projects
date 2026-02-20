OBJS = proxy_cache.c
CC = gcc
EXEC = proxy_cache
SRC = -lcrypto

all: $(OBJS)
	$(CC) -o $(EXEC) $(OBJS) $(SRC)
clean:
	rm -rf $(EXEC)

