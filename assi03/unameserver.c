#define _GNU_SOURCE
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

struct UnameMap{
	int sbit;
	int abit;
	int nbit;
	int rbit;
	int vbit;
	int mbit;
	int pbit;
	int ibit;
	int obit;
	int errorbit;
};

void validateArgv(int argc,char **argv, int *portnum, int *trans_flag);
void exitWithError();
void exitSysWithError(char *call);
void udpServer(char **argv, int *port_flag);
void tcpServer(char **argv, int *port_flag);
void unameCaller(char *buffer);
void validatePortArg(char *buffer);
void validateUnameArg(char *buffer, struct UnameMap *m);

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
	int sockfd, len;
    struct sockaddr_in servaddr, clientaddr;
    char buffer[BUFFER_LEN];
    
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd == -1) {
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
    int ret = bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
    if(ret != 0) {
        exitSysWithError("bind()");
    }
	
	// stdio output the port in terminal
	len = sizeof(sockfd);
    if(getsockname(sockfd, (struct sockaddr*) &servaddr, &len) == -1) {
        exitWithError("getsockname()");
    }

    if(*port == OFF) {
        printf("Server listening at port number: %u\n", ntohs(servaddr.sin_port));
    }else if(*port == ON) {
        printf("Server listening...\n");
    }


	while(1) {
        len = sizeof(clientaddr);

		//TBD: recvfrom()
		ret = recvfrom(sockfd, buffer, BUFFER_LEN, 0, &clientaddr, &len);
        buffer[ret] = '\0';
        unameCaller(buffer);
		//TBD: sendto()
        sendto(sockfd, buffer, strlen(buffer), 0, &clientaddr, len);
    }
	
	//TBD: close()
    close(sockfd);
    exit(0);
	

		
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
		memset(buffer, 0, sizeof buffer);
		while((ret = recv(client_sockfd, buffer, BUFFER_LEN, 0)) > 0) {
			buffer[strlen(buffer)] = '\0';
			printf("strlen=%ld\n", strlen(buffer));
			printf("received message=%s\n", buffer);
			printf("ret=%d\n",ret);
			unameCaller(buffer);
			printf("buffer=%s\n",buffer);
			write(client_sockfd, buffer, strlen(buffer));
		}
	
		if(ret == 0) {
			printf("connection close with client: %s...\n", inet_ntoa(client.sin_addr));
		} else if(ret == -1) {
			exitSysWithError("recv()");
		}
	}
	close(server_sockfd);
	exit(0);
}

void unameCaller(char *buffer){

	struct utsname ustr;
	char *temp;
	int ret;
	struct UnameMap m;
	int len = 0;
	

	memset(&m, 0, sizeof(m));
	ret = uname(&ustr);
    

	if(ret == -1) {
		exitSysWithError("uname()");
	}

	validateUnameArg(buffer, &m);
	memset(buffer, 0, sizeof buffer);

	/*if we don't return right here that means all inputs are validate
	*	else we return a error message to the client notifying that client's input arg
	* 	is not one of a|s|n|r|v|m|p|o.
	*/
	if(m.errorbit == 1) {
		strcpy(buffer, "we only accept a|s|n|r|v|m|p|o arg inputs\0");
		printf("errorbit:%s\n", buffer);
		return;
	}

	/*1)if we reach here, it means that client's args are all validated
	* 2)we check if use's args have 'a'. If we see a-bit is on then we fill buffer with 
	*   all info return by uname syscall and return. Bsically a means return all.
	* 
	*/
	if(m.abit == 1) {
		/*concatinating system name to the buffer*/
        strcat(buffer,ustr.sysname);
		strcat(buffer, " ");
		printf("buffer=%s\n", buffer);
		printf("len=%d\n", len);


		/*concatinating server name to the buffer*/
        strcat(buffer,ustr.nodename);
		strcat(buffer, " ");
		printf("buffer=%s\n", buffer);
        printf("len=%d\n", len);
		
		/*concatinating release versiov to the buffer*/
        strcat(buffer, ustr.release);
		strcat(buffer, " ");
		printf("buffer=%s\n", buffer);
        printf("len=%d\n", len);


		/*concatinating os distro to the buffer*/
        strcat(buffer, ustr.version);
		strcat(buffer, " ");
		printf("buffer=%s\n", buffer);
        printf("len=%d\n", len);

		/*concatinating machine to the buffer
        len += strlen(ustr.machine);
        strcat(buffer, ustr.machine);
        buffer[len++] = ' ';
		*/
	
		
		/*concatinating processor to the buffer
        len += strlen(ustr.machine);
        strcat(buffer, ustr.machine);
        buffer[len] = ' ';*/

		/*concatinating hardware archit to the buffer*/
        strcat(buffer, ustr.machine);
		strcat(buffer, " ");
		printf("buffer=%s\n", buffer);
        printf("len=%d\n", len);

		/*concatinating os to the buffer*/
        strcat(buffer, ustr.sysname);
		strcat(buffer, "\0");
		printf("buffer=%s\n", buffer);
        printf("len=%d\n", len);


		return;
	}
	

	/*if we reach here, it means that we don't have 'a' as a input arg.
	* We need to fill in whatever the input args are in the buffer.
	*/
	if(m.sbit == 1) {
		strcat(buffer,ustr.sysname);
        strcat(buffer, " ");
        printf("buffer=%s\n", buffer);
        printf("len=%d\n", len);
	}

	if(m.nbit == 1) {
        strcat(buffer, ustr.nodename);
		strcat(buffer, " ");
		printf("buffer=%s\n", buffer);
        printf("len=%d\n", len);
	}
	if(m.rbit == 1) {
		strcat(buffer, ustr.release);
		strcat(buffer, " ");
		printf("buffer=%s\n", buffer);
        printf("len=%d\n", len);
	}
	if(m.vbit == 1) {
        strcat(buffer, ustr.version);
		strcat(buffer, " ");
		printf("buffer=%s\n", buffer);
        printf("len=%d\n", len);
    }
	if(m.mbit == 1) {
        strcat(buffer, ustr.machine);
		strcat(buffer, " ");
		printf("buffer=%s\n", buffer);
        printf("len=%d\n", len);
    }
	if(m.pbit == 1) {
        strcat(buffer, ustr.machine);
		strcat(buffer, " ");
		printf("buffer=%s\n", buffer);
        printf("len=%d\n", len);
    }
	if(m.ibit == 1) {
        strcat(buffer, ustr.machine);
		strcat(buffer, " ");
		printf("buffer=%s\n", buffer);
        printf("len=%d\n", len);
    }
	if(m.obit == 1) {
        strcat(buffer,ustr.sysname);
		strcat(buffer, " ");
		buffer[len] = ' ';
		printf("buffer=%s\n", buffer);
        printf("len=%d\n", len);
	}
	strcat(buffer, "\0");
	printf("len=%d\n", len);
	printf("buffer=%s\n", buffer);


}

void validateUnameArg(char *b, struct UnameMap *m) {

	for(int i = 0; b[i] != '\0'; i++) {
		char c = b[i];
		printf("b[%d]=%s\n",i, &c);
		printf("strcmp=%d\n", strcmp(&c, "n"));
		if('a' == c) {
			printf("strcmp=%d\n",strcmp("a", &c));
			m->abit = 1;
		} else if(c == 's') {
			printf("strcmp=%d\n",strcmp("s", &c));
			m->sbit = 1;
		} else if(c == 'n') {
			printf("strcmp=%d\n",strcmp("n", &c));
            m->nbit = 1;
        } else if(c == 'r') {
			printf("strcmp=%d\n",strcmp("r", &c));
            m->rbit = 1;
        } else if(c == 'v') {
			printf("strcmp=%d\n",strcmp("v", &c));
            m->vbit = 1;
        } else if(c == 'm') {
			printf("strcmp=%d\n",strcmp("m", &c));
            m->mbit = 1;
        } else if(c == 'p') {
			printf("strcmp=%d\n",strcmp("p", &c));
            m->pbit = 1;
        } else if(c == 'o') {
			printf("strcmp=%d\n",strcmp("o", &c));
            m->obit = 1;
        } else if(c == 'i') {
			printf("strcmp=%d\n",strcmp("i", &c));
            m->ibit = 1;
		} else if(c != ' '){
			return;
		} else {
			m->errorbit=1;
			return;
		}
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
