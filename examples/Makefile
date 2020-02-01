LD_FLAGS = -L../core -lumtp -lpthread -lssl -lcrypto
ifeq "$(origin INCLUDE_DIR)" "undefined"
	INCLUDE_DIR = ../include
endif

ifeq "$(origin CFLAGS)" "undefined"
	CFLAGS = -Wall -O3 -I$(INCLUDE_DIR)
endif

all:simple_server simple_client

SERVER_TARGET := simple_server
CLIENT_TARGET := simple_client

SERVER_SRC := simple_server.c
CLIENT_SRC := simple_client.c

SERVER_OBJS := $(patsubst %.c, %.o, $(SERVER_SRC))
CLIENT_OBJS := $(patsubst %.c, %.o, $(CLIENT_SRC))

simple_server:$(SERVER_OBJS)
	@$(CC) $(SERVER_TARGET).o -o $(SERVER_TARGET) $(LD_FLAGS)

simple_client:$(CLIENT_OBJS)
	@$(CC) $(CLIENT_TARGET).o -o $(CLIENT_TARGET) $(LD_FLAGS)

clean:
	@$(RM) $(SERVER_TARGET) $(CLIENT_TARGET) $(SERVER_OBJS) $(CLIENT_OBJS)

%.o:%.c
	@echo "   [CC] " $@;
	@$(CC) $(CFLAGS) -c -o $@ $<
