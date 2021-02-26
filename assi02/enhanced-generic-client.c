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
#define ON 1
#define BUFFER_LEN 2056


void exitSysWithError(char *call){
    fprintf(stderr, "Syscall %s failed with errno=%d\n", call, errno);
    exit(-1);
}
void exitWithError() {
    exit(-1);
}

void tcpClient(struct in_addr ina, char* msg, int port_n) {
	int sockfd;
	struct sockaddr_in servaddr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1) {
        exitSysWithError("socket()");
    }

	servaddr.sin_family = AF_INET; 
    servaddr.sin_addr = ina; 
    servaddr.sin_port = htons(port_n); 

	// TBD: connect()
	int ret = connect(sockfd, (SA*) &servaddr, sizeof(servaddr));
	if(ret == -1) {
		exitSysWithError("connect()");
	}
	// TBD: send()
	send(sockfd, msg, strlen(msg), 0);

	int n = 0;
	int len = 0, maxlen = 100;
	char buffer[maxlen];
	char* pbuffer = buffer;

	// will remain open until the server terminates the connection
	n = recv(sockfd, pbuffer, maxlen, 0);
	while ( n < 0) {
		pbuffer += n;
		maxlen -= n;
		len += n;

		n = recv(sockfd, pbuffer, maxlen, 0);	
	}
	buffer[n] = '\0';	
	printf("%s\n", buffer);

	// TBD: close()
	close(sockfd);
	exit(0);
}
void udpClient(struct in_addr ina, char *msg, int port_n) {
	
	int sockfd;
	struct sockaddr_in servaddr;	
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd == -1) {
		exitSysWithError("socket()");
	}
	
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr = ina; 
	servaddr.sin_port = htons(port_n); 
	
	int n, len;
	// TBD: sendto()
	sendto(sockfd, msg, strlen(msg), 0,(SA*) &servaddr, sizeof(servaddr));
	
	// TBD: recvfrom()
	char buffer[BUFFER_LEN];
	len = sizeof(servaddr);
	
	n = recvfrom(sockfd,(char *) buffer, 1024, 0,(SA*) &servaddr, &len);


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
	
	if (argc != 9) {
		printf("Error:Inputs either too less or too many\n");
		printf("Please follow format: ./echo_client -m msg -h hostname(google.com)|ipv4(127.0.0.1) -s servicename(echo|time|daytime|etc..) -p tcp|udp\n");
		printf("The program currently only support exactly 8 args except ./enhanced-generic-client \n");
		exitWithError();
	}
	

	/*check the command inputs for -m, -s, -p, -h 
	if input commands not matching one of these we exit*/
	ValidateOpts(argv, argc);


	int sn, msg, hn, protocol; // varible holders for servicename, message, hostname, and protocol
	SetOpts(argv, argc, &msg, &hn, &sn, &protocol);//set them to the index positions of the current
	
	struct hostent *he;
	/*exit here if the input for -h isn't validated input*/
	he = gethostbyname(argv[hn]);
	if(he == NULL) {
		exitSysWithError("gethostbyname()");
	}
	
	/*exit if input value for service name isn't validated*/
	struct servent *svent;
	svent=getservbyname(argv[sn], argv[protocol]);
	if(svent == NULL) {
		printf("check with your -p&-s args\n");
		exitSysWithError("gethservbyname()");
	}


	/*socket address and port converting*/
	struct in_addr ina;
	int ret = inet_aton(inet_ntoa(*(struct in_addr*) he->h_addr), &ina);
	if(ret == 0) {
		exitSysWithError("inet_aton()");
	}
	int port_int = ntohs(svent->s_port);



	
	/*check for -p arg*/
	if (strcmp(argv[protocol], "tcp") == 0) {
		tcpClient(ina, argv[msg], port_int);
	} else if (strcmp(argv[protocol], "udp") == 0) {
		udpClient(ina, argv[msg], port_int);
	} else {
		exitWithError();
	}

	exit(0);
}
