CC = gcc
CFLAGS = -Wall

SERVER_SRC = src/server.c
CLIENT_SRC = src/client.c

SERVER_TARGET = server.exe
CLIENT_TARGET = client.exe

all: $(SERVER_TARGET) $(CLIENT_TARGET)

$(SERVER_TARGET): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $(SERVER_TARGET) $(SERVER_SRC) -lws2_32

$(CLIENT_TARGET): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $(CLIENT_TARGET) $(CLIENT_SRC) -lws2_32

clean:
ifeq ($(OS),Windows_NT)
	del $(SERVER_TARGET) $(CLIENT_TARGET)
else
	rm -f $(SERVER_TARGET) $(CLIENT_TARGET)
endif