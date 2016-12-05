#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int main()
{
	struct sockaddr_in server_address;
	bzero(&server_address, sizeof(server_address));
	server_address.sin_family = AF_INET;
	inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr);
	server_address.sin_port = htons(8888);
	int sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if (connect(sockfd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
		printf("connection failed\n");
	} else {
		int n = 0;
		do {
			n = send(sockfd, "hello", 5, 0);
			sleep(1);	
		} while (n == 5);
	}
	close(sockfd);
	return 0;
}