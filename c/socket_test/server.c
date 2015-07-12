#include <event2/event.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

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

void read_handler(struct bufferevent *bufev, void *ctx) {
  size_t readlen;
  char buf[BUF_SIZE];
  readlen = bufferevent_read(bufev, buf, BUF_SIZE);
  printf("Recv: ");
  fwrite(buf, readlen, 1, stdout);
  printf("\n");
}

void event_handler(struct bufferevent* bufev, short ev_type, void *ctx) {
  if (ev_type & BEV_EVENT_ERROR) perror("event error");
  if (ev_type & (BEV_EVENT_EOF|BEV_EVENT_ERROR)) {
    bufferevent_free(bufev);
    num_clients--;
    show_num_clients();
  }
}

void accept_handler(struct evconnlistener* listener, evutil_socket_t fd,
                    struct sockaddr* addr, int socklen, void *ctx) {
  struct event_base* ev_base = evconnlistener_get_base(listener);
  struct bufferevent* bufev = bufferevent_socket_new(ev_base,
                                                     fd, BEV_OPT_CLOSE_ON_FREE);
  bufferevent_setcb(bufev, read_handler, NULL, event_handler, NULL);
  bufferevent_enable(bufev, EV_READ|EV_WRITE);
  bufferevent_write(bufev, "Write", 6);
  num_clients++;
  show_num_clients();
}

void accept_error_handler(struct evconnlistener* listener, void *ctx) {
  struct event_base* ev_base = evconnlistener_get_base(listener);
  int err = EVUTIL_SOCKET_ERROR();
  fprintf(stderr, "accept_error: %d(%s)\n", err, evutil_socket_error_to_string(err));
  event_base_loopexit(ev_base, NULL);
}

struct evconnlistener* open_server(struct event_base* ev_base, int port) {
  struct sockaddr_in sin;
  struct evconnlistener* listener;

  memset((void*)&sin , 0, sizeof(struct sockaddr_in));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = htonl(INADDR_ANY);
  sin.sin_port = htons(port);

  listener = evconnlistener_new_bind(ev_base, accept_handler, NULL,
                                     LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE, -1,
                                     (struct sockaddr*)&sin, sizeof(sin));
  if(!listener) die("evconnlistener_new_bind");
  evconnlistener_set_error_cb(listener, accept_error_handler);
  return listener;
}

int main() {
  struct event_base* ev_base = event_base_new();

  open_server(ev_base, 8000);

  if (event_base_dispatch(ev_base) != 0) die("event_base_dispatch");
  event_base_free(ev_base);
  return 0;
}
