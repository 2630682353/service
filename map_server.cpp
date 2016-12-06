#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <arpa/inet.h>
#include <string.h>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <map>
#define LISTEN_PORT 9999
#define LISTEN_BACKLOG 32
# define bzero(S1, LEN)     ((void)memset(S1,  0, LEN))
using namespace std;

void do_accept(evutil_socket_t listener, short event, void *arg);
void read_cb(struct bufferevent *bev, void *arg);
void error_cb(struct bufferevent *bev, short event, void *arg);
void write_cb(struct bufferevent *bev, void *arg);

map<int, struct bufferevent*> mapStudent;  
map<int, struct bufferevent*>::iterator iter;  
  
struct msg_head{
    int type;
    int from;
    int to;
    int totallen;
};


int main(int argc, char *argv[])
{
    int ret;
    evutil_socket_t listener;
    listener = socket(AF_INET, SOCK_STREAM, 0);
    assert(listener > 0);
    evutil_make_listen_socket_reuseable(listener);

    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = inet_addr("127.0.0.1");
    sin.sin_port = htons(LISTEN_PORT);

    if (bind(listener, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        perror("bind");
        return 1;
    }

    if (listen(listener, LISTEN_BACKLOG) < 0) {
        perror("listen");
        return 1;
    }

    printf ("Listening...\n");

    evutil_make_socket_nonblocking(listener);

    struct event_base *base = event_base_new();
    assert(base != NULL);
    struct event *listen_event;
    listen_event = event_new(base, listener, EV_READ|EV_PERSIST, do_accept, (void*)base);
    event_add(listen_event, NULL);
    event_base_dispatch(base);

    printf("The End.");
    return 0;
}

void do_accept(evutil_socket_t listener, short event, void *arg)
{
    struct event_base *base = (struct event_base *)arg;
    evutil_socket_t fd;
    struct sockaddr_in sin;
    socklen_t slen;
    fd = accept(listener, (struct sockaddr *)&sin, &slen);
    if (fd < 0) {
        perror("accept");
        return;
    }
    if (fd > FD_SETSIZE) {
        perror("fd > FD_SETSIZE\n");
        return;
    }

    printf("ACCEPT: fd = %u\n", fd);

    struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(bev, read_cb, NULL, error_cb, arg);
    bufferevent_enable(bev, EV_READ|EV_WRITE|EV_PERSIST);
}

void read_cb(struct bufferevent *bev, void *arg)
{
#define MAX_LINE    512
    char line[MAX_LINE+1];
    int n;
 //   evutil_socket_t fd = bufferevent_getfd(bev);
    msg_head head;
    bzero(&head, sizeof(head));
    int st = 0;
    int ret;
    do{
        switch(st){
            case 0:
                ret = bufferevent_read(bev, &head, sizeof(head));
                if(ret > 0){
                    st = 1;
                }
                break;
            case 1:               
                ret = bufferevent_read(bev, line + sizeof(head), head.totallen - sizeof(head));
                if(ret > 0){
                    memcpy(line, &head, sizeof(head));
                    ret = ret + sizeof(head);
                    st = 2;
                }            
                break;
            default:
                break;
        }
    }while(ret>0 && st != 2);
    if (st == 2) {
        switch (head.type) {
        case 0:
            iter = mapStudent.find(head.from);
            if(iter != mapStudent.end()) {
                head.to = head.from;
                head.from = 0;
                head.totallen = sizeof(head) + 4;
                int val = -1;
                bufferevent_write(bev, &head, sizeof(head));
                bufferevent_write(bev, &val, 4);
            } else  {
                mapStudent.insert(pair<int,struct bufferevent*>(head.from, bev));
                head.to = head.from;
                head.from = 0;
                head.totallen = sizeof(head) + 4;
                int val = 0;
                bufferevent_write(bev, &head, sizeof(head));
                bufferevent_write(bev, &val, 4);
            } 
            break;
        case 1:
            iter = mapStudent.find(head.to);
            if(iter != mapStudent.end()) {         
                bufferevent_write((struct bufferevent*)(iter->second), line, ret);
            } else {
                head.to = head.from;
                head.from = 0;
                head.totallen = sizeof(head) + 4;
                int val = -2;
                bufferevent_write(bev, &head, sizeof(head));
                bufferevent_write(bev, &val, 4);
            }
            break;
        }
    }
}

void write_cb(struct bufferevent *bev, void *arg) {}

void error_cb(struct bufferevent *bev, short event, void *arg)
{
    evutil_socket_t fd = bufferevent_getfd(bev);
    printf("fd = %u, ", fd);
    if (event & BEV_EVENT_TIMEOUT) {
        printf("Timed out\n"); //if bufferevent_set_timeouts() called
    }
    else if (event & BEV_EVENT_EOF) {
        printf("connection closed\n");
    }
    else if (event & BEV_EVENT_ERROR) {
        printf("some other error\n");
    }
    bufferevent_free(bev);
}