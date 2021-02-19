#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>


#define SA struct sockaddr
#define PORT 7
#define BUFFER_LEN 2056

void exitSysWithError(char *call){
    fprintf(stderr, "Syscall %s failed with errno=%d", call, errno);
    exit(-1);
}
void exitWithError() {
    exit(-1);
}

void tcpClient(struct in_addr ina, char* msg) {
	int sockfd;
	struct sockaddr_in servaddr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1) {
        exitSysWithError("socket()");
    }

	servaddr.sin_family = AF_INET; 
    servaddr.sin_addr = ina; 
    servaddr.sin_port = htons(PORT); 

	// TBD: connect()
	int ret = connect(sockfd, (SA*) &servaddr, sizeof(servaddr));
	if(ret == -1) {
		exitWithError();
	}
	// TBD: send()
	send(sockfd, msg, strlen(msg), 0);
	
	// TBD: read()
	char buffer[BUFFER_LEN] = {0};
	read(sockfd, buffer, sizeof(buffer));
	printf("%s\n", buffer);
	// TBD: close()
	close(sockfd);
	exit(0);
}
void udpClient(struct in_addr ina, char *msg) {
	
	int sockfd;
	struct sockaddr_in servaddr;	
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd == -1) {
		exitSysWithError("socket()");
	}
	
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr = ina; 
	servaddr.sin_port = htons(PORT); 
	
	int n, len;
	// TBD: sendto()
	sendto(sockfd, msg, strlen(msg), 0,(SA*) &servaddr, sizeof(servaddr));
	
	// TBD: recvfrom()
	char buffer[BUFFER_LEN];
	len = sizeof(servaddr);
	
	n = recvfrom(sockfd,(char *) buffer, BUFFER_LEN, 0,(SA*) &servaddr, &len);


	buffer[n] = '\0';
	printf("%s\n", buffer);
	// TBD: close()
	close(sockfd);
	exit(0);
}
	

int main(int argc, char* argv[]) {

	if (argc != 4) {
		printf("Error:Inputs either too less or too many\n");
		printf("Please follow format: ./echo_client YourMsg 127.0.0.1 -tcp/-udp\n");
		exitWithError();
	}
	
	struct in_addr ina;
	int ret = inet_aton(argv[2], &ina);

	if(ret == 0) {
		exitWithError();
	}
	
	if (strcmp(argv[3], "-tcp") == 0) {
		tcpClient(ina, argv[1]);
	} else if (strcmp(argv[3], "-udp") == 0) {
		udpClient(ina, argv[1]);
	} else {
		exitWithError();
	}

	exit(0);
}
