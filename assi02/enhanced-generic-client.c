#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>


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
int ValidateOption(char* option)
{
	if(strcmp(option, "-m") == 0 || 
		strcmp(option, "-h") == 0 || 
		strcmp(option, "-s") == 0 || 
		strcmp(option, "-p") == 0) {
		return 0;
	}

	return -1;
}

void ValidateOpts(char* argv[], int argc)
{
	for(int i = 1; i < argc; i+=2) {
        if(ValidateOption(argv[i]) == -1) {
            printf("Check your args, we only support -m, -h, -s, -p four options\n");
            exitWithError();
        }
    }
}

void SetOpts(char* arr[],int argc, int *msg, int *hn, int *sn, int *protocol)
{
	for(int i = 1; i < argc; i+=2) {
		if(strcmp(arr[i], "-m") == 0) {
			*msg = i + 1;
		} else if(strcmp(arr[i], "-h") == 0) {
			*hn = i + 1;
		} else if(strcmp(arr[i], "-s") == 0) {
			*sn = i + 1;
		} else if(strcmp(arr[i], "-p") == 0){
			*protocol = i + 1;
		}
	}
}


int main(int argc, char* argv[]) {
	//struct hostent *he;
	//he = gethostbyname("google.com");
	//printf("IP address=%s\n", inet_ntoa(*(struct in_addr*) he->h_addr));
	if (argc != 9) {
		printf("Error:Inputs either too less or too many\n");
		printf("Please follow format: ./echo_client YourMsg 127.0.0.1 -tcp/-udp\n");
		exitWithError();
	}
	

	//check the command inputs for -m, -s, -p, -h
	ValidateOpts(argv, argc);


	int sn, msg, hn, protocol; // varible holders for server name, message, hostname, and protocol
	SetOpts(argv, argc, &sn, &msg, &hn, &protocol);
	printf("sn=%d\n",sn);
	printf("msg=%d\n",msg);
	printf("hn=%d\n",hn);
	printf("protocol=%d\n",protocol);
	struct in_addr ina;
	char* snp = argv[sn];
	int ret = inet_aton(snp, &ina);

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
