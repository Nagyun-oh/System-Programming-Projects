CC           = gcc
FLAGS       = -Wall -g
SRC       	= -lcrypto
OBJS_SERVER  = proxy_cache.c
EXEC_SERVER  = proxy_cache

all: $(EXEC_SERVER)

$(EXEC_SERVER): $(OBJS_SERVER)
	$(CC) $(FLAGS) -o $(EXEC_SERVER) $(OBJS_SERVER) $(SRC)

clean:
	rm -f $(EXEC_SERVER)

.PHONY: all clean
