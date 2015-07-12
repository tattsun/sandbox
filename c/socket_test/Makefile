socket_test: build build/server build/client

build:
	mkdir build

build/server: server.c
	gcc -o build/server -std=c99 -Wall -O3 ./server.c -levent

build/client: client.c
	gcc -o build/client -std=c99 -Wall -O3 ./client.c -levent
