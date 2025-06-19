CC = gcc
CFLAGS = -Wall -pthread
LDFLAGS = -lpcre -lm


ifeq ($(shell uname), Darwin)
    BREW_PREFIX = $(shell brew --prefix)
    CFLAGS += -I$(BREW_PREFIX)/include
    LDFLAGS += -L$(BREW_PREFIX)/lib
endif


COMMON_DIR = common
CLIENT_DIR = client
SERVER_DIR = server
ADMIN_DIR = admin


COMMON_OBJ = $(COMMON_DIR)/nlp.o $(COMMON_DIR)/protocol.o
CLIENT_OBJ = $(CLIENT_DIR)/client.o
SERVER_OBJ = $(SERVER_DIR)/server.o
ADMIN_OBJ = $(ADMIN_DIR)/admin_client.o


CLIENT_BIN = client_bin
SERVER_BIN = server_bin
ADMIN_BIN = admin_bin

all: $(CLIENT_BIN) $(SERVER_BIN) $(ADMIN_BIN)


$(COMMON_DIR)/%.o: $(COMMON_DIR)/%.c $(COMMON_DIR)/%.h
	$(CC) $(CFLAGS) -c $< -o $@

$(CLIENT_DIR)/%.o: $(CLIENT_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(SERVER_DIR)/%.o: $(SERVER_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(ADMIN_DIR)/%.o: $(ADMIN_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@


$(CLIENT_BIN): $(CLIENT_OBJ) $(COMMON_OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(SERVER_BIN): $(SERVER_OBJ) $(COMMON_OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(ADMIN_BIN): $(ADMIN_OBJ) $(COMMON_OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)


clean:
	rm -f $(COMMON_DIR)/*.o $(CLIENT_DIR)/*.o $(SERVER_DIR)/*.o $(ADMIN_DIR)/*.o
	rm -f $(CLIENT_BIN) $(SERVER_BIN) $(ADMIN_BIN)


$(shell mkdir -p $(COMMON_DIR) $(CLIENT_DIR) $(SERVER_DIR) $(ADMIN_DIR))