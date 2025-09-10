CC = gcc
CFLAGS = -Wall -I src/headers
SRC_DIR = src
BUILDS_DIR = ./builds
OBJS_DIR = $(BUILDS_DIR)/objs

CLIENT_DOWNLOAD_DIR = ./client_download

OBJS = $(OBJS_DIR)/packet.o $(OBJS_DIR)/protocol.o
CLIENT_OBJS = $(OBJS_DIR)/client.o $(OBJS)
SERVER_OBJS = $(OBJS_DIR)/server.o $(OBJS)

all: remove_client_file clean client server

client: $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o $(BUILDS_DIR)/$@.out $(CLIENT_OBJS)

server: $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $(BUILDS_DIR)/$@.out $(SERVER_OBJS)

$(OBJS_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS_DIR)/*.o $(BUILDS_DIR)/client.out $(BUILDS_DIR)/server.out

remove_client_file:
	rm -f $(CLIENT_DOWNLOAD_DIR)/*
