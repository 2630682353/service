#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
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
int file_package_num = 0;
int file_package_current = 0;
int fd;


void * get_input(void *arg)
{
	int sockfd = *(int *)arg;
	char msg_data[MAX_PACKAGE] = {0};
	char c_model[8] = {0};
	int model;
	struct msg_head head;
	int toid = 0;
	int len;
	char buf[MAX_PACKAGE] = {0};
	int datalen;
	char *first = NULL;
	while (1) {
		printf("1 发文件 2 发信息\n");

		fgets(c_model, 8, stdin);
		model = atoi(c_model);
		switch (model) {
		case 1:
			printf("发文件：\n");
			fgets(buf, MAX_PACKAGE, stdin);
			first = strstr(buf, " ");
			if (first == NULL) {
				printf("input error\n");
				continue;
			}
			*first = '\0';
			toid = atoi(buf);
			head.type = 3;
			head.from = userid;
			head.to = toid;
			first[strlen(first+1)] = '\0';
			int fd;
			if ((fd = open(first+1, O_RDONLY)) < 0) {
				printf("open error\n");
				continue;
			}
			struct stat stat1;
			stat(first+1, &stat1);
			int size = (unsigned int)stat1.st_size;
			head.totalnum = 0;
			head.index = 1;
			datalen = MAX_PACKAGE-sizeof(head);
			if (size > datalen) {
				unsigned char temp[datalen*4];
				head.totalnum = size%datalen > 0 ? size/datalen+1:size/datalen;
				
				int n;
				while((n = read(fd, temp, datalen*4)) > 0){
					int i = 0;
					while(n - datalen > 0) {
						head.totallen = MAX_PACKAGE;
						send(sockfd, &head, sizeof(head), 0);
						send(sockfd, &temp[datalen*i], datalen,0);
						i++;
						n = n - datalen;
						head.index++;
					}
					head.totallen = sizeof(head) + n;
					send(sockfd, &head, sizeof(head), 0);
					send(sockfd, &temp[datalen*i], n, 0);
					head.index++;
					usleep(5000);
				}
			} else {
				int n;
				unsigned char temp[datalen];
				if ((n = read(fd, temp, datalen)) > 0) {
					head.totallen = sizeof(head) + n;
					send(sockfd, &head, sizeof(head), 0);
					send(sockfd, temp, n, 0);
				}

			}
			break;
		case 2:
			printf("发信息：\n");
			fgets(buf, MAX_PACKAGE, stdin);
			first = strstr(buf, " ");
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
			break;
		}
	}
}

void * file_send(void *arg)
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
		sleep(30);
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
			tv.tv_sec = 60;
			tv.tv_usec = 0;
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
	int login_msg[7] = {0};
	struct msg_head head = {0, 1, 2, 5, 0, 0};
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
#define MAX_LINE    1400
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
    	if (head.from == 0) {
    		printf("error\n");
    		return 0;
    	}
    	switch (head.type) {
    		case 1:
    			line[ret] = 0;
    			printf("%d msg received: from %d to %d datais:%s\n",userid, head.from, head.to, &line[sizeof(head)]);
    			break;
    		case 3:
    			if (head.totalnum == 0) {
    				fd = open("recevied0", O_RDWR | O_CREAT | O_TRUNC);
    				printf("recevied0\n");
    				write(fd, &line[sizeof(head)], head.totallen - sizeof(head));
    				close(fd);
    			} else {
    				if (head.index == 1)
    					fd = open("recevied", O_RDWR | O_CREAT | O_TRUNC);
    				printf("recevied total:%d current %d size:%d\n", head.totalnum, head.index, head.totallen - sizeof(head));
    				write(fd, &line[sizeof(head)], head.totallen - sizeof(head));
    				if (head.index == head.totalnum)
    					close(fd);
    			}

    	}
    	
    }
}
