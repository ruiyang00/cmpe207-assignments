#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/socket.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <netdb.h>

#define BUFFER_LEN 1024
#define NAME_OPTCODE 1
#define TEXT_OPTCODE 2
#define BYE_OPTCODE 3
#define ACTIVE 1
#define PASSIVE 2
char *my_name;
char *peer_name;
int client_sock = 0;
int server_sock=0;




void ExitSysWithError(char *call);
void ExitWithOptError();
int GetTCPSock();
void SendMsg();
void PassiveChat();
void ActiveChat();
void ActiveRecvMessage();
void PassiveRecvMessage();
void SetUpName();


int main(int argc, char **argv){
    char *peer;
    int activeflag = 0, passiveflag=0, opt=0, portnum=0;
    int tcpsock;
    char name;

    static struct option longopts[] = {
        { "active", no_argument, NULL, 'a' },
        { "passive", no_argument, NULL, 'b' },
        {"port", required_argument, NULL, 'c'},
        {"peer", required_argument, NULL, 'd'},
        {0, 0, 0, 0}
    };



    while( (opt = getopt_long(argc, argv, "abc:d:", longopts, NULL)) != -1) {
        switch(opt) {
            case 'a':
                activeflag=1;
                break;
            case 'b':
                passiveflag=1;
                break;
            case 'c':
                portnum=atoi(optarg);
                break;
            case 'd':
                peer=optarg;
                break;
            case '?':
                ExitWithOptError();
                break;
            case ':':
                ExitWithOptError();
                break;
            default:
                ExitSysWithError("getopt_long()");
        }
    }

    if(peer == NULL) {
        printf("peer=Null\n");
    }

    printf("peerN=%s\n", peer);
    printf("activeflag=%d\n", activeflag);
    printf("passiveflag=%d\n", passiveflag);
    printf("portnum=%d\n", portnum);

    if((passiveflag == 1 && activeflag == 1) || (activeflag == 1 && argc !=6) || (passiveflag == 1 && argc != 4)){
        ExitWithOptError();
    }


    if(activeflag == 1) {
        ActiveChat(portnum, peer);
    }

    if(passiveflag == 1) {
        PassiveChat(portnum);
    }

    return 0;
}

void ActiveChat(int port, char *peer){
    struct sockaddr_in servaddr;
    struct hostent *he;
    char user_input[BUFFER_LEN];

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    printf("server_sockfd=%d\n", server_sock);
    if(server_sock == -1) {
        ExitSysWithError("socket()");
    }

    he = gethostbyname(peer);
    if(he == NULL) {
        ExitSysWithError("gethostbyname()");
    }

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port); 
    memcpy(&servaddr.sin_addr, he->h_addr_list[0], he->h_length);

    printf("peer=%s\n", peer);
    printf("addr=%s\n", inet_ntoa(servaddr.sin_addr));
    
    // cache chat name
    SetUpName();

    int ret = connect(server_sock, (struct sockaddr*) &servaddr, sizeof(servaddr));
    if(ret == -1) {
        ExitSysWithError("connect()");
    }
    printf("PassiveChat() server_sock=%d\n", server_sock);

    pthread_t input_thread;

    if(pthread_create(&input_thread, NULL, (void * (*)(void *))SendMsg, NULL) < 0){
        ExitSysWithError("pthread_creat()");
    } else {
        pthread_detach(input_thread);
    }


    int n;
    char read_buffer[BUFFER_LEN];
    while((n = recv(server_sock, read_buffer, BUFFER_LEN, 0)) > 0) {
        printf("%s\n",read_buffer);
        memset(read_buffer, 0, sizeof(read_buffer));
    }
    if(n == -1) {
        ExitSysWithError("recv()");
    }

    //ActiveRecvMessage(server_sock);
    exit(0);
}

void ActiveRecvMessage(int server_sock){
    int n;
    char read_buffer[BUFFER_LEN];
    
    printf("Active() server_sock=%d\n",server_sock);
    while((n = recv(server_sock, read_buffer, BUFFER_LEN, 0)) > 0) {
    }
    printf("buffer=%s\n",read_buffer);
    if(n == -1) {
        ExitSysWithError("recv()");
    }
    exit(0);
}


void PassiveChat(int port){
    int server_sock, ret, len;
    char read_buffer[BUFFER_LEN];
    struct sockaddr_in client;
    char *pbuffer;


    // set up the chat name on passive side: my_name
    SetUpName(); 

    // passive sock
    server_sock = GetTCPSock(port);





    pbuffer=read_buffer;

    while(1) {
        len = sizeof(client);
        client_sock = accept(server_sock, (struct sockaddr*) &client, &len);
        printf("Passive() client_sock=%d\n", client_sock);
        pthread_t input_thread;
        pthread_create(&input_thread, NULL, (void * (*)(void *))SendMsg, NULL);
        pthread_detach(input_thread);
        printf("after accept()\n");
        memset(read_buffer, 0, sizeof(read_buffer));
        while((ret = recv(client_sock, read_buffer, BUFFER_LEN, 0)) > 0){
            printf("%s\n", read_buffer);
            memset(read_buffer, 0, sizeof(read_buffer));
        }

        printf("after recv()\n");
        if(ret == -1) {
            ExitSysWithError("recv()");
        }
        close(client_sock);
        client_sock = 0;
        printf("terminating the channle, reading to accept new connection....\n");
    }
    exit(0); 
}


void PassiveRecvMessage(int server_sock){
    int sock,len, ret_val;
    struct sockaddr_in client;
    char read_buffer[BUFFER_LEN];

    while(1) {
        len = sizeof(client);
        client_sock = accept(server_sock, (struct sockaddr*) &client, &len);
        memset(read_buffer, 0, sizeof(read_buffer));
        while((ret_val = recv(client_sock, read_buffer, BUFFER_LEN, 0)) > 0){

        }
        printf("buffer=%s\n",read_buffer);
        if(ret_val == -1) {
            ExitSysWithError("recv()");
        }
        close(client_sock);

    }
    client_sock = 0;
    exit(0);
}

void SendMsg(void *ptr){
    char write_buffer[BUFFER_LEN];
    // exchange names

    printf("inside SendMsg()\n");
    printf("SendMsg() server_sock=%d\n", server_sock);
    printf("SendMsg() client_sock=%d\n", client_sock);

    while(1) {
        memset(write_buffer, 0, sizeof(write_buffer));
        fgets(write_buffer, sizeof(write_buffer), stdin);
        write_buffer[strlen(write_buffer) - 1] = '\0';

        //send to peer
        if(client_sock != 0) {
            write(client_sock, write_buffer,strlen(write_buffer));
        }

        if(server_sock != 0) {
            write(server_sock, write_buffer,strlen(write_buffer));
        }
        printf("after write()\n");
    }
}

void SetUpName(){
    char read_buffer[BUFFER_LEN];
    printf("Enter Your Name: ");
    fgets(read_buffer, sizeof(read_buffer), stdin);
    printf("fgets()=%s\n", read_buffer);
    read_buffer[strlen(read_buffer) - 1] = '\0';
    my_name = read_buffer;
    printf("Name=%s\n", my_name);
}

int GetTCPSock(int port) {

    int server_sockfd, ser_len;
    struct sockaddr_in servaddr;

    server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_sockfd == -1) {
        ExitSysWithError("socket()");
    }

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    //TBD:bind
    int ret = bind(server_sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
    if(ret != 0) {
        ExitSysWithError("bind()");
    }


    //TBD:listen()
    ret = listen(server_sockfd, 8);

    if(ret != 0) {
        ExitSysWithError("listen()");
    }

    //printf() the current port that the server is listening at
    ser_len = sizeof(server_sockfd);
    if(getsockname(server_sockfd, (struct sockaddr*) &servaddr, &ser_len) == -1) {
        ExitSysWithError("getsockname()");
    }

    printf("Passive Server Initiated\n");
    printf("Listening at port: %u\n", ntohs(servaddr.sin_port));
    return server_sockfd;
}


void ExitWithOptError() {
    printf("We accpet (--active | --passive) --port XXXX [--peer [IPADDRS | DNSNAME]] in any order\n");
    printf("If initiating Active: --active --port xxxx --peer 127.0.0.1 \n");
    printf("If initiating Passive: --passive --port xxxx\n");
    exit(1);
}

void ExitSysWithError(char *call){
    fprintf(stderr, "Syscall %s failed with errno=%d, msg=%s\n", call, errno, strerror(errno));
    exit(-1);
}


