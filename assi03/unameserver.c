#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/utsname.h>
#define UDP 2
#define TCP 1
#define ON 1
#define OFF 0
#define BUFFER_LEN 2048

void validateArgv(int argc,char **argv, int *portnum, int *trans_flag);
void exitWithError();
void exitSysWithError(char *call);
void udpServer(char **argv, int *port_flag);
void tcpServer(char **argv, int *port_flag);
void unameCaller(char *buffer);
void validatePortArg(char *buffer);

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
	int server_sockfd, client_sockfd, client_len, ser_len;
	struct sockaddr_in servaddr, client, sin;
	char buffer[BUFFER_LEN];
	server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(server_sockfd == -1) {
        exitSysWithError("socket()");
    }

	servaddr.sin_family = AF_INET;
 	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	if(*port==OFF) {
  		servaddr.sin_port = htons(0);
	}
	if(*port == ON) {
		validatePortArg(argv[3]);
	}
	if(*port == ON && atoi(argv[3]) <= 2000) {

		printf("port=%d\n", *port);
		exitWithError("port number too small. Please input port number greater than 2000");
	}

	if(*port == ON && atoi(argv[3]) > 2000) {
		servaddr.sin_port = htons(atoi(argv[3]));
	}

	//TBD:bind
	int ret = bind(server_sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
	if(ret != 0) {
		exitSysWithError("bind()");
	}


	//TBD:listen()	
	ret = listen(server_sockfd, 8);
	
	if(ret != 0) {
		exitSysWithError("listen()");
	}
	ser_len = sizeof(server_sockfd);
	if(getsockname(server_sockfd, (struct sockaddr*) &servaddr, &ser_len) == -1) {
		exitWithError("getsockname()");
	} 
	if(*port == OFF) {
		printf("Server listening at port number: %u\n", ntohs(servaddr.sin_port));
	}else if(*port == ON) {
		printf("Server listening...\n");
	}
	
	while(1) {
		//TBD: accept()
		client_len = sizeof(client);

		client_sockfd = accept(server_sockfd, (struct sockaddr*) &client, &client_len);
		if(client_sockfd < 0) {
			exitSysWithError("accpet()");
		}

		//TBD: read()/recv()
		while((ret = recv(client_sockfd, buffer, BUFFER_LEN, 0)) > 0) {
			buffer[ret] = '\0';
			printf("received message=%s\n", buffer);
			unameCaller(buffer);
			write(client_sockfd, buffer, strlen(buffer));
		}
	
		if(ret == 0) {
			printf("connection close with client%s...\n", inet_ntoa(client.sin_addr));
		} else if(ret == -1) {
			exitSysWithError("recv()");
		}
	}
	close(server_sockfd);
	exit(0);
}

void unameCaller(char *buffer){
	printf("unameCller()\n");
	struct utsname ustr;
	int ret;
	if(strcmp(buffer, "-s") == 0) {
		ret = uname(&ustr);
		if(ret == -1) {
			exitSysWithError("uname()");
		}
		printf("ustr=%s", ustr.sysname);	
		buffer == ustr.sysname;
	}

}

void validatePortArg(char *buffer) {
	for(int i = 0; buffer[i] != '\0'; i++) {
		if(isdigit(buffer[i]) == 0) {
			exitWithError("we only accept digit argument as port input");
		}
	}
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
	//unameserver -tcp|-udp -port PortNum
	if(argc == 3|| argc > 4 || argc < 2) {
		exitWithError("input arguments either too less/many");
	}
	if(argc == 4) {
		*portnum = ON;	
	}else if(argc == 2) {
		*portnum = OFF;
	}
	if(argc == 4 && strcmp(argv[2], "-port") != 0) {
		exitWithError("unrecognized option for argument 2. We only accept -port");
	}
	
	if(strcmp(argv[1],"-tcp") == 0) {
		*trans_flag = TCP;
	} else if(strcmp(argv[1],"-udp") == 0) {
		*trans_flag = UDP;
	} else {
		exitWithError("unrecognized transport. We only accept [-tcp|-udp]");
	}

	
}
