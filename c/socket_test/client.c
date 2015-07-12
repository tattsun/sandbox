#include <stdio.h>
#include <event2/event.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_CONNECTIONS 25000
#define WAIT_SECS 5
#define BUF_SIZE 128

int num_conn = 0;
#define show_num_conn() printf("number of connections: %d\n", num_conn);

struct timeval* timer_duration;

void die(const char* msg) {
  perror(msg);
  exit(1);
}

typedef struct {
  struct event_base* ev_base;
  struct event* ev;
  struct event* timer;
} connection_t;

connection_t* new_connection_t(struct event_base* ev_base) {
  connection_t* cont = (connection_t*)malloc(sizeof(connection_t));
  cont->ev_base = ev_base;
  cont->ev = NULL;
  cont->timer = NULL;
  return cont;
}

void free_connection_t(connection_t* cont) {
  event_free(cont->ev);
  if (cont->timer) evtimer_del(cont->timer);
  free(cont);
}

void close_handler(int s, short e, void* args) {
  connection_t* cont = (connection_t*)args;
  free_connection_t(cont);
  num_conn--;
  show_num_conn();
  printf("Connection closed by timer.");
}

void recv_handler(int sock, short ev_type, void* args) {
  connection_t* cont = (connection_t*)args;
  size_t recvlen;
  char buf[BUF_SIZE];

  if (ev_type & EV_READ) {
    recvlen = recv(sock, buf, BUF_SIZE, 0);

    if (recvlen == 0) {
      printf("Closed by server.\n");
      free_connection_t(cont);
      close(sock);
      num_conn--;
      show_num_conn();
    } else {
      printf("Data received.\n");
      cont->timer = evtimer_new(cont->ev_base, close_handler, cont);
      if (evtimer_add(cont->timer, timer_duration) != 0) die("evtimer_add");
    }
  }
}

void connect_server(struct event_base* ev_base, const char* addr, int port) {
  struct sockaddr_in servaddr;
  struct event* recv_ev;
  connection_t* cont = new_connection_t(ev_base);
  int sock = socket(AF_INET, SOCK_STREAM, 0);

  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(port);
  servaddr.sin_addr.s_addr = inet_addr(addr);

  if (connect(sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0) die("connect");
  recv_ev = event_new(ev_base, sock, EV_READ | EV_PERSIST, recv_handler, cont);
  cont->ev = recv_ev;
  if (event_add(recv_ev, 0) != 0) die("event_add");

  num_conn++;
  show_num_conn();
}

void init_timer_duration() {
  timer_duration = (struct timeval*)malloc(sizeof(struct timeval));
  timer_duration->tv_sec = WAIT_SECS;
  timer_duration->tv_usec = 0;
}

int main(int argc, const char* args[]) {
  if (argc < 2) {
    printf("client SERVER_ADDR SERVER_PORT");
    exit(1);
  }

  struct event_base* ev_base = event_base_new();
  init_timer_duration();

  const char* host = args[1];
  int port = atoi(args[2]);

  for(int i=0; i < MAX_CONNECTIONS; i++) {
    connect_server(ev_base, host, port);
  }

  event_base_dispatch(ev_base);

  event_base_free(ev_base);
}
