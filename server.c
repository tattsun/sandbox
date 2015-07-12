#include <event2/event.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUF_SIZE 128

int num_clients = 0;

#define show_num_clients() printf("number of clients: %d\n", num_clients);

typedef struct {
  struct event* ev;
} client_t;

client_t* new_client() {
  client_t* cli = (client_t*)malloc(sizeof(client_t));
  return cli;
}

void free_client(client_t* cli) {
  event_free(cli->ev);
  free(cli);
}

void die(const char* msg) {
  perror(msg);
  exit(1);
}

int open_server(int port) {
  struct sockaddr_in sin;
  int fd;

  fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  memset((void*)&sin , 0, sizeof(struct sockaddr_in));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = htonl(INADDR_ANY);
  sin.sin_port = htons(port);

  if (bind(fd, (struct sockaddr*)&sin, sizeof(struct sockaddr)) != 0)
    die("bind");
  if (listen(fd, -1) != 0)
    die("listen");

  printf("Server listening on %d\n", port);

  return fd;
}

void recv_handler(int csock, short ev_type, void* args) {
  char buf[BUF_SIZE];
  int recvlen;
  client_t* cli = (client_t*)args;

  if (ev_type & EV_READ) {
    recvlen = recv(csock, buf, BUF_SIZE, 0);
    if (recvlen == 0) {
      free_client(cli);
      close(csock);
      num_clients--;
      show_num_clients();
    } else {
      printf("received: ");
      fwrite(buf, recvlen, 1, stdout);
      printf("\n");
    }
  }
}

void accept_handler(int ssock, short ev_type, void* args) {
  printf("Client accepted.\n");
  struct event_base* ev_base = (struct event_base*)args;
  struct event* ev = NULL;
  struct sockaddr_in addr;
  int csock;
  socklen_t addrlen = sizeof(addr);
  client_t* cli;
  const char* msg = "Welcome";

  if (ev_type & EV_READ) {
    cli = new_client();
    csock = accept(ssock, (struct sockaddr*)&addr, &addrlen);
    ev = event_new(ev_base, csock, EV_READ | EV_PERSIST, recv_handler, (void*)cli);
    cli->ev = ev;
    if (event_add(ev, 0) != 0) die("event_add");
    send(csock, msg, strlen(msg), 0);
    num_clients++;
    show_num_clients();
  }
}

int main() {
  struct event_base* ev_base = event_base_new();

  int sock = open_server(8000);
  struct event* accept_ev = event_new(ev_base, sock, EV_READ | EV_PERSIST, accept_handler,
                                      ev_base);
  if (event_add(accept_ev, 0) != 0) die("event_add");

  if (event_base_dispatch(ev_base) != 0) die("event_base_dispatch");
}
