#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#define UDP 2
#define TCP 1
#define ON 1
#define OFF 0

void validateArgv(int argc,char **argv, int *portnum, int *trans_flag);
void exitWithError();
void exitSysWithError(char *call);
void udpServer(char **argv, int *port_flag);
void tcpServer(char **argv, int *port_flag);

int main(int argc, char **argv) {

	int port_flag; //check if we have port number input
	int trans_flag; //flag to check which transport specified 

	//TBD: check input argvi
	validateArgv(argc, argv, &port_flag, &trans_flag);
	printf("port_number=%d\n",port_flag);
	printf("trans_flag=%d\n",trans_flag);

	if(trans_flag == UDP) {
		udpServer(argv, &port_flag);
	}else if(trans_flag == TCP) {
		tcpServer(argv, &port_flag);
	} else {
		exit(-1);
	}
	return 0;
}
void udpServer(char **argv, int *port) {
	
}

void tcpServer(char **argv, int *port) {
	int sockfd;
	struct sockaddr_in servaddr;
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd == -1) {
        exitSysWithError("socket()");
    }

	servaddr.sin_family = AF_INET;
 	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	if(*port==OFF) {
  		servaddr.sin_port = htons(0);
	}
	

	printf("port=%d\n", *port);
	printf("portNum=%d\n", atoi(argv[2]));
	if(*port == ON && atoi(argv[2]) <= 2000) {

		printf("port=%d\n", *port);
		exitWithError("port number too small. Please input port number greater than 2000");
	}

	if(*port == ON && atoi(argv[2]) > 2000) {
		servaddr.sin_port = htons(atoi(argv[2]));
	}
	
	int ret = bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
	if(ret != 0) {
		exitSysWithError("bind()");
	}
	
	ret = listen(sockfd, 8);

	if(ret != 0) {
		exitSysWithError("listen()");
	}

	printf("Server listening at port:%d\n", ntohs(servaddr.sin_port));
	
	close(sockfd);
	exit(0);
	
	

	
}




void exitSysWithError(char *call){
    fprintf(stderr, "Syscall %s failed with errno=%d\n", call, errno);
    exit(-1);
}
void exitWithError(char *call) {
	printf("Exit with error: %s\n", call);
    exit(-1);
}

void validateArgv(int argc, char **argv, int *portnum, int *trans_flag) {
	
	//unameserver -tcp|-udp portnumber
	if(argc > 3 || argc < 2) {
		exitWithError("input arguments either too less/many");
	}

	if(argc == 3) {
		*portnum = ON;	
	}else if(argc == 2) {
		*portnum = OFF;
	}
	
	if(strcmp(argv[1],"-tcp") == 0) {
		*trans_flag = TCP;
	} else if(strcmp(argv[1],"-udp") == 0) {
		*trans_flag = UDP;
	} else {
		exitWithError("unrecognized transport. We only accpte [-tcp|-udp]");
	}

	
}
