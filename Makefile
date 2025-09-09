CC = gcc
CFLAGS = -Wall -I src/headers
SRC_DIR = src
BUILDS_DIR = ./builds

OBJS = $(BUILDS_DIR)/objs/packet.o $(BUILDS_DIR)/objs/protocol.o
CLIENT_OBJS = $(BUILDS_DIR)/objs/client.o $(OBJS)
SERVER_OBJS = $(BUILDS_DIR)/objs/server.o $(OBJS)

all: client server

client: $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o $(BUILDS_DIR)/$@.out $(CLIENT_OBJS)

server: $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $(BUILDS_DIR)/$@.out $(SERVER_OBJS)

$(BUILDS_DIR)/objs/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(BUILDS_DIR)/objs/*.o $(BUILDS_DIR)/client.out $(BUILDS_DIR)/server.out
