#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/utsname.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/select.h>
#define UDP 2
#define TCP 1
#define ON 1
#define OFF 0
#define BUFFER_LEN 4096

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

struct Request{
	int sockfd;
	struct sockaddr_in incoming;
	int len;
};


void validateArgv(int argc,char **argv, int *portnum);
void exitWithError();
void exitSysWithError(char *call);
int passiveUDP(char **argv, int *port_flag, uint16_t *n);
int passiveTCP(char **argv, int *port_flag, uint16_t *n);
int unameCaller(char *buffer);
void validatePortArg(char *buffer);
int validateUnameArg(char *buffer, struct UnameMap *m);
void removeSpace(char *buffer);
void tcp_sock_handler(int sock);
void udp_sock_handler(struct Request *req);
int MAX(int a, int b);

int main(int argc, char **argv) {
	/*set to ON if we have input after -port else OFF*/
	int port_flag;
	struct sockaddr_in incoming;
	uint16_t n; 					/*varible to hold the default port number*/
	char   	 buffer[BUFFER_LEN];
	int	   alen;
	int    tmsock;                /* TCP master socket  */
    int	   umsock;                /* UDP master socket  */
	int	   nfds;
    fd_set rfds;                 /* readable file descriptors */
	pthread_t tid;
	pthread_attr_t ta;
	//TBD: check input argvi
	validateArgv(argc, argv, &port_flag);


	struct Request *req = malloc(sizeof(struct Request));


	//create two passive sock descriptors for both tcp&udp
	tmsock = passiveTCP(argv,&port_flag, &n);
    umsock = passiveUDP(argv, &port_flag, &n);

	
	/* bit number of max fd */
    nfds = MAX(tmsock, umsock) + 1;
    FD_ZERO(&rfds);
	

	pthread_attr_init(&ta);
	pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);
	
	while(1) {
		FD_SET(tmsock, &rfds);
        FD_SET(umsock, &rfds);
        if(select(nfds, &rfds, (fd_set *)0, (fd_set *)0, (struct timeval *)0) < 0){
			exitSysWithError("select()");
		}

		if(FD_ISSET(tmsock, &rfds)) {
            int tssock;
			alen = sizeof(incoming);
			tssock=accept(tmsock, (struct sockaddr *)&incoming, &alen);
			if(tssock < 0) {
				exitSysWithError("accept()");
			}

			
			/*create another thread to handle it*/
			if(pthread_create(&tid, &ta, (void * (*)(void *))tcp_sock_handler, (void *) tssock) < 0){
				exitSysWithError("pthread_creat()");
			}
			
		}

		if(FD_ISSET(umsock, &rfds)) {
			req->sockfd = umsock;
			req->incoming = incoming; 
			if(pthread_create(&tid, &ta, (void * (*)(void *))udp_sock_handler, req) < 0){
        		exitSysWithError("pthread_creat()");
			}
		}
	}

	close(tmsock);
	close(umsock);
	return 0;
}

int MAX(int a, int b) {
	return a > b ? a : b; 
}

void tcp_sock_handler(int sock) {
		char buffer[BUFFER_LEN];
        int unameret, ret;
		memset(buffer, 0, sizeof buffer);
        while((ret = recv(sock, buffer, BUFFER_LEN, 0)) > 0) {
            removeSpace(buffer);
            unameret = unameCaller(buffer);
            write(sock, buffer, strlen(buffer));
            memset(buffer, 0, sizeof buffer);
            if (unameret == 0) {
                exit(0);
            }
        }

        if(ret == -1) {
            exitSysWithError("recv()");
        }
		close(sock);
}
void udp_sock_handler(struct Request *req) {
	int unameret;
	int alen;
	char buffer[BUFFER_LEN];
	alen = sizeof(req->incoming);
	if(recvfrom(req->sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&req->incoming, &alen) < 0){
		exitSysWithError("recvfrom()");
	}	
	removeSpace(buffer);

    unameret = unameCaller(buffer);
    //TBD: sendto()
    sendto(req->sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&req->incoming, alen);

    if(unameret == 0) {
		exit(0);
	}
}


int passiveUDP(char **argv, int *port, uint16_t *n) {
	int sockfd, len;
    struct sockaddr_in servaddr;
	uint16_t smport = *n;
    
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd == -1) {
        exitSysWithError("socket()");
    }

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(*port==OFF) {
        servaddr.sin_port = htons(smport);
	}

    if(*port == ON) {
        validatePortArg(argv[2]);
    }
    if(*port == ON && atoi(argv[2]) <= 2000) {

        printf("port=%d\n", *port);
        exitWithError("port number too small. Please input port number greater than 2000");
    }

    if(*port == ON && atoi(argv[2]) > 2000) {
        servaddr.sin_port = htons(atoi(argv[2]));
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
        printf("UDP Server listening at port number: %u\n", ntohs(servaddr.sin_port));
    }else if(*port == ON) {
        printf("UDP Server is listening...\n");
    }
	
	return sockfd;
	
}

int passiveTCP(char **argv, int *port, uint16_t *n) {

	int server_sockfd, ser_len;
	struct sockaddr_in servaddr;
	
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
		validatePortArg(argv[2]);
	}
	if(*port == ON && atoi(argv[2]) <= 2000) {

		printf("port=%d\n", *port);
		exitWithError("port number too small. Please input port number greater than 2000");
	}

	if(*port == ON && atoi(argv[2]) > 2000) {
		servaddr.sin_port = htons(atoi(argv[2]));
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
	
	//printf() the current port that the server is listening at
	ser_len = sizeof(server_sockfd);
	if(getsockname(server_sockfd, (struct sockaddr*) &servaddr, &ser_len) == -1) {
		exitWithError("getsockname()");
	} 
	if(*port == OFF) {
		*n = ntohs(servaddr.sin_port);
		printf("TCP Server listening at port number: %u\n", ntohs(servaddr.sin_port));
	}else if(*port == ON) {
		printf("TCP Server listening...\n");
	}
	return server_sockfd;
		
}

int unameCaller(char *buffer){

	struct utsname ustr;
	char *temp;
	int ret;
	struct UnameMap m;
	int len = 0;
	char *argvBuffer;
	
	
	memset(&m, 0, sizeof(m));
	ret = uname(&ustr);
    

	if(ret == -1) {
		exitSysWithError("uname()");
	}

	ret = validateUnameArg(buffer, &m);
	if(ret == 1) {
		return 0;
	}
	memset(buffer, 0, sizeof buffer);

	/*if we don't eturn right here that means all inputs are validate
	*	else we return a error message to the client notifying that client's input arg
	* 	is not one of a|s|n|r|v|m|p|o.
	*/
	if(m.errorbit == 1) {
		strcat(buffer, "we cannot recognize your input, we only accept a|s|n|r|v|m|p|o\n");
		strcat(buffer, "Please recconnect to the remote uname server with arg help\n");
		return 1;
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


		/*concatinating server name to the buffer*/
        strcat(buffer,ustr.nodename);
		strcat(buffer, " ");
		
		/*concatinating release versiov to the buffer*/
        strcat(buffer, ustr.release);
		strcat(buffer, " ");


		/*concatinating os distro to the buffer*/
        strcat(buffer, ustr.version);
		strcat(buffer, " ");

		/*concatinating machine to the buffer*/
        strcat(buffer, ustr.machine);
		strcat(buffer, " ");
	
	
		
		/*concatinating processor to the buffer
        strcat(buffer, ustr.machine);
		strcat(buffer, " ");
        printf("buffer=%s\n", buffer);*/
     

		/*concatinating hardware archit to the buffer
        strcat(buffer, ustr.machine);
		strcat(buffer, " ");
		printf("buffer=%s\n", buffer);*/

		/*concatinating os to the buffer*/
        strcat(buffer, ustr.sysname);
		strcat(buffer, "\0");

		return 1;
	}
	

	/*if we reach here, it means that we don't have 'a' as a input arg.
	* We need to fill in whatever the input args are in the buffer.
	*/
	if(m.sbit == 1) {
		strcat(buffer,ustr.sysname);
        strcat(buffer, " ");
	}

	if(m.nbit == 1) {
        strcat(buffer, ustr.nodename);
		strcat(buffer, " ");
	}
	if(m.rbit == 1) {
		strcat(buffer, ustr.release);
		strcat(buffer, " ");
	}
	if(m.vbit == 1) {
        strcat(buffer, ustr.version);
		strcat(buffer, " ");
    }
	if(m.mbit == 1) {
        strcat(buffer, ustr.machine);
		strcat(buffer, " ");
    }
	if(m.pbit == 1) {
        strcat(buffer, ustr.machine);
		strcat(buffer, " ");
    }
	if(m.ibit == 1) {
        strcat(buffer, ustr.machine);
		strcat(buffer, " ");
    }
	if(m.obit == 1) {
        strcat(buffer,ustr.sysname);
		strcat(buffer, " ");
	}
	strcat(buffer, "\0");

	return 1;
}

void removeSpace(char* b) {
	int a = 0;
	for(int n = 0; b[n] != '\0'; ++n) {
		if(isalpha(b[n]))
			b[a++] = b[n];
	}
	b[a] = '\0';
}

int validateUnameArg(char *b, struct UnameMap *m) {

	if(strcmp("help", b) == 0) {
		memset(b, 0, sizeof b);
		strcat(b," -------------------------------------------------- \n");
		strcat(b,"| Manual Page for Remote Unameserver: version beta |\n");
		strcat(b," -------------------------------------------------- \n");
		strcat(b," 1) s: Prints the kernel name\n");
		strcat(b," 2) n: Prints the system’s node name (hostname)\n");
		strcat(b," 3) r: Prints the kernel release\n");
		strcat(b," 4) v: Prints the kernel version\n");
		strcat(b," 5) m: Prints the name of the machine’s hardware name\n");
		strcat(b," 6) p: Prints the architecture of the processor\n");
		strcat(b," 7) i: Prints the hardware platform\n");
		strcat(b," 8) o: Print the name of the operating system(Linux)\n");
		strcat(b," 9) a: Same as snrvmo options\n");
		return 1;
	}

	if(strcmp("version", b) == 0) {
		memset(b, 0, sizeof b);
		strcat(b, "Remote Uname Server Beta 1.0.0\n");
		strcat(b, "Copyright Rui Yang\n");
		strcat(b, "GitHub Link: https://github.com/ruiyang00/cmpe207-assignments/tree/master/assi03\n");
		strcat(b, "This is free software: you are free to change and redistribute it.\n");
		strcat(b, "\n");
		strcat(b, "Written by Rui Yang for the remote uname server assignment of cmpe207\n");
		return 1;
	}

	for(int i = 0; b[i] != '\0'; i++) {
		char c = b[i];
		if('a' == c) {
			m->abit = 1;
		} else if(c == 's') {
			m->sbit = 1;
		} else if(c == 'n') {
            m->nbit = 1;
        } else if(c == 'r') {
            m->rbit = 1;
        } else if(c == 'v') {
            m->vbit = 1;
        } else if(c == 'm') {
            m->mbit = 1;
        } else if(c == 'p') {
            m->pbit = 1;
        } else if(c == 'o') {
            m->obit = 1;
        } else if(c == 'i') {
            m->ibit = 1;
		} else if(c == ' ') {
			return 0;
		} else {
			m->errorbit=1;
			return 0;
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
    fprintf(stderr, "Syscall %s failed with errno=%d, msg=%s\n", call, errno, strerror(errno));
    exit(-1);
}
void exitWithError(char *call) {
	printf("Exit with error: %s\n", call);
    exit(-1);
}

void validateArgv(int argc, char **argv, int *portnum) {
	//unameserver -tcp|-udp -port PortNum

	if(argc == 3) {
		*portnum = ON;	
	}else if(argc == 1) {
		*portnum = OFF;
	} else {
		exitWithError("input arguments either too less/many");	
	}
	if(argc == 3 && strcmp(argv[1], "-port") != 0) {
		exitWithError("unrecognized option for argument 1. We only accept -port");
	}
}	
