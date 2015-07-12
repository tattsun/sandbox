socket_test: build build/server build/client

build:
	mkdir build

build/server: server.c
	gcc -o build/server -levent -Wall -O3 ./server.c

build/client: client.c
	gcc -o build/client -levent -Wall -O3 ./client.c
