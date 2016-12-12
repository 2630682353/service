#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
struct msg_head{
    int type;
    int from;
    int to;
    int totallen;
    int totalnum;
    int index;
};
#define MAX_PACKAGE 1400
int login(int sockfd);
int userid = -1;

void * get_input(void *arg)
{
	int sockfd = *(int *)arg;
	char msg_data[512] = {0};
	struct msg_head head;
	int toid = 0;
	int len;
	char buf[512] = {0};
	while (1) {
		fgets(buf, 512, stdin);
		char *first = strstr(buf, " ");
		if (first == NULL) {
			printf("input error\n");
			continue;
		}
		*first = '\0';
		toid = atoi(buf);
		head.type = 1;
		head.from = userid;
		head.to = toid;
		head.totallen = sizeof(head) + strlen(first+1);
		memcpy(msg_data, &head, sizeof(head));
		memcpy(&msg_data[sizeof(head)], first+1, strlen(first+1));
		send(sockfd, msg_data, head.totallen, 0);
	}
}
void * heart_send(void *arg) {
	int sockfd = *(int *)arg;
	char heart_data[64] = {0};
	struct msg_head head;
	int toid = 0;
	int len;
	while (1) {
		head.type = 2;
		head.from = userid;
		head.to = toid;
		head.totallen = sizeof(head) + 4;
		memcpy(heart_data, &head, sizeof(head));
		send(sockfd, heart_data, head.totallen, 0);
		sleep(5);
	}
}
int main(int argc, char *argv[])
{
	userid = atoi(argv[1]);
	struct sockaddr_in server_address;
	bzero(&server_address, sizeof(server_address));
	server_address.sin_family = AF_INET;
	inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr);
	server_address.sin_port = htons(9999);
	int sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if (connect(sockfd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
		printf("connection failed\n");
	} else {
		login(sockfd);
		pthread_t tid1;
		pthread_t tid2;
		pthread_create(&tid1, NULL, get_input, &sockfd);
		pthread_create(&tid2, NULL, heart_send, &sockfd);
		struct timeval tv;
		tv.tv_sec = 60;
		tv.tv_usec = 0;
		fd_set fds;
		while (1) {
			FD_SET(sockfd, &fds);
			if (select(sockfd + 1, &fds, NULL, NULL, &tv)) {
				if (errno == EINTR || errno == EAGAIN)
					continue;
				if (FD_ISSET(sockfd, &fds))
					get_msg(sockfd);
			}
		}
		/*
		int max_fd = 0;
		int n = 0;
		do {
			n = send(sockfd, "hello", 5, 0);
			sleep(1);	
		} while (n == 5);
		*/
	}
	close(sockfd);
	return 0;
}
int login(int sockfd)
{
	char data[256] = {0};
	int login_msg[5] = {0};
	struct msg_head head = {0, 1, 2, 5};
	head.from = userid;
	memcpy(login_msg, &head, sizeof(head));
	send(sockfd, login_msg, sizeof(login_msg), 0);
	recv(sockfd, &head, sizeof(head), 0);
	recv(sockfd, data, head.totallen - sizeof(head), 0);
	if (head.type == 0 && *(int *)(&data[0]) == 0) {
		printf("login success\n");
	} else if (head.type == 0 && *(int *)(&data[0]) == -1) {
		printf("aready login\n");
	}
	return 0;
}
int get_msg(int sockfd)
{
#define MAX_LINE    512
    char line[MAX_LINE+1];
    int n;
    struct msg_head head;
    bzero(&head, sizeof(head));
    int st = 0;
    int ret;
    do{
        switch(st){
            case 0:
                ret = recv(sockfd, &head, sizeof(head), 0);
                if(ret > 0){
                    st = 1;
                }
                break;
            case 1:               
                ret = recv(sockfd, line + sizeof(head), head.totallen - sizeof(head), 0);
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
    	switch (head.from) {
    		case 0:
    			printf("error\n");
    			break;
    		default:
    			line[ret] = 0;
    			printf("%d msg received: from %d to %d datais:%s\n",userid, head.from, head.to, &line[sizeof(head)]);
    	}
    	
    }
}
