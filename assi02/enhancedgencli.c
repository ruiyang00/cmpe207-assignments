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
int optionCheck(char* option)
{
	if(strcmp(option, "-m") == 0 || 
		strcmp(option, "-h") == 0 || 
		strcmp(option, "-s") == 0 || 
		strcmp(option, "-p") == 0) {
		return 0;
	}

	return -1;

}

/*
void setOpts(char* arr[], char* msg, char* hn, char* sn, char* protocol, int argn)
{
	for(int i = 1; i < argn; i+=2) {
		if(strcmp(arr[i] == "-m") == 0) {
			msg = arr[i+1];
		} else if(strcmp(arr[i], "-h") == 0) {
			hn = argv[i+1];
		} else if(strcmpe(arr[i], "-s") == 0) {
			sn = arr[i+1];
		} else if(strcmp(arr[i], "-p") == 0){
			protocol = arr[i+1];
		}
	}
}*/
int main(int argc, char* argv[]) {

	if (argc != 9) {
		printf("Error:Inputs either too less or too many\n");
		printf("Please follow format: ./echo_client YourMsg 127.0.0.1 -tcp/-udp\n");
		exitWithError();
	}

	
	//if options are not correct we exit here
	/*
    if(optionCheck(argv[1]) == -1 || optionCheck(argv[3]) == -1
        || optionCheck(argv[5]) == -1 || optionCheck(argv[7]) == -1) {
		printf("Check your args, we only support -m, -h, -s, -p four options\n");
        exitWithError();
    }*/


	for(int i = 1; i < argc; i+=2) {
		if(optionCheck(argv[i]) == -1) {
			printf("Check your args, we only support -m, -h, -s, -p four options\n");
        	exitWithError();
		}
        //printf("opt=%s, ret=%d\n",argv[i], optionCheck(argv[i]));
    }

	for(int i = 0; i < argc; i++) {
        printf("argv[%d]=%s\n",i, argv[i]);
    }
	/*

	//char* sn, msg, hn, protocol; // varible holders for server name, message, hostname, and protocol
	//setOpts(argv, sn, msg, hn, protocol);
	
	struct in_addr ina;
	int ret = inet_aton(sn, &ina);

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
	*/

	exit(0);
}
