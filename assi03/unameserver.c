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
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
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

void validateArgv(int argc,char **argv, int *portnum, int *trans_flag);
void exitWithError();
void exitSysWithError(char *call);
void udpServer(char **argv, int *port_flag);
void tcpServer(char **argv, int *port_flag);
int unameCaller(char *buffer);
void validatePortArg(char *buffer);
int validateUnameArg(char *buffer, struct UnameMap *m);
void removeSpace(char *buffer);
void signal_handler(int sig);
int tcp_sock_handler(int sock, char *buffer);
int udp_sock_handler(int sock, char *buffer, struct sockaddr_in *clientaddr, int len, int unameret);

int main(int argc, char **argv) {

	/*set to ON if we have input after -port else OFF*/
	int port_flag;
	/*set to UDP|TCP*/
	int trans_flag; //flag to check which transport specified 

	//TBD: check input argvi
	validateArgv(argc, argv, &port_flag, &trans_flag);

	
	/*calling the server function to do work base on trans_flag*/
	if(trans_flag == UDP) {
		udpServer(argv, &port_flag);
	}else if(trans_flag == TCP) {
		tcpServer(argv, &port_flag);
	} else {
		exit(-1);
	}
	return 0;
}

int tcp_sock_handler(int sock, char *buffer) {
	//TBD: read()/recv()
        memset(buffer, 0, sizeof buffer);
        int unameret, ret;
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
		return 1;
}
int udp_sock_handler(int sockfd, char *buffer, struct sockaddr_in *clientaddr, int len, int unameret) {

	removeSpace(buffer);
    unameret = unameCaller(buffer);
    //TBD: sendto()
    sendto(sockfd, buffer, strlen(buffer), 0, clientaddr, len);
    if(unameret == 0) {
		exit(0);
    }
	close(sockfd);
	return 0;
}


void signal_handler(int sig) {
	int status;
	while(wait3(&status, WNOHANG, (struct rusage*) 0) > 0);
	return;
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

	int unameret;
	
	signal(SIGCHLD, signal_handler);
	while(1) {
        len = sizeof(clientaddr);

		//TBD: recvfrom()
		ret = recvfrom(sockfd, buffer, BUFFER_LEN, 0, &clientaddr, &len);
        buffer[ret] = '\0';

		switch (fork()) {
            case 0:     /* child */
                exit(udp_sock_handler(sockfd, buffer, &clientaddr, len, unameret));
            default:    /* parent */
                break;
            case -1:
                exitSysWithError("fork()");
        }
    }
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

	/*clean up zombie children*/
	signal(SIGCHLD, signal_handler);
	
	while(1) {
		//TBD: accept()
		client_len = sizeof(client);

		client_sockfd = accept(server_sockfd, (struct sockaddr*) &client, &client_len);
		if(client_sockfd < 0) {
			if(errno==EINTR) {
				continue;
			}
			exitSysWithError("accpet()");
		}
		
		switch (fork()) {
			case 0:		/* child */
				close(server_sockfd);
				exit(tcp_sock_handler(client_sockfd, buffer));
			default:	/* parent */
				close(client_sockfd);
				break;
			case -1:
				exitSysWithError("fork()");
		}
	}
	close(server_sockfd);	
	exit(0);
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
