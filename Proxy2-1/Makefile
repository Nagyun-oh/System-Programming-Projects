OBJS_SERVER     = server.c
OBJS_CLIENT     = client.c
CC      = gcc
EXEC_SERVER     = server
EXEC_CLIENT     = client
SRC     = -lcrypto

all:    server client

server: $(OBJS_SERVER)
	$(CC) -o $(EXEC_SERVER) $(OBJS_SERVER) $(SRC)

client: $(OBJS_CLIENT)
	$(CC) -o $(EXEC_CLIENT) $(OBJS_CLIENT) $(SRC)

clean:
	rm -rf $(EXEC_SERVER)
	rm -rf $(EXEC_CLIENT)

.PHONY: all clean

