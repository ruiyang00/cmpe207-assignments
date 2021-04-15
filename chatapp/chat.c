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
#define ACTIVE 1
#define PASSIVE 2
char NAME_OPTCODE='1';
char TEXT_OPTCODE='2';
char BYE_OPTCODE='3';
char my_name[100];
char peer_name[100];
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
void ExchangeName();


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


    // cache chat name
    SetUpName();

    int ret = connect(server_sock, (struct sockaddr*) &servaddr, sizeof(servaddr));
    if(ret == -1) {
        ExitSysWithError("connect()");
    }


    pthread_t input_thread;

    if(pthread_create(&input_thread, NULL, (void * (*)(void *))SendMsg, NULL) < 0){
        ExitSysWithError("pthread_creat()");
    } else {
        pthread_detach(input_thread);
    }


    int n;
    uint16_t l;
    char opt;
    char text[BUFFER_LEN];
    char read_buffer[BUFFER_LEN];


    memset(read_buffer, 0, sizeof(read_buffer));
    memset(text, 0, sizeof(text));
    ExchangeName();
    while((n = recv(server_sock, read_buffer, sizeof read_buffer, 0)) > 0) {
        memset(text, 0, sizeof(text));
        memcpy(&l, read_buffer, sizeof(l));
        memcpy(&opt, read_buffer + sizeof(l), sizeof(opt));
        memcpy(text, read_buffer + sizeof(l) + sizeof(opt), ntohs(l) - sizeof(char));


        if(opt == NAME_OPTCODE){
            memcpy(peer_name, text, strlen(text));
        } else if(opt == TEXT_OPTCODE) {
            printf("%s>%s\n", peer_name, text);
        } else if(opt == BYE_OPTCODE) {
            break;
        }

    }
    if(n == -1) {
        ExitSysWithError("recv()");
    }

    exit(0);
}


void PassiveChat(int port){
    int server_sock, ret, len;
    struct sockaddr_in client;

    // set up the chat name on passive side: my_name
    SetUpName(); 

    // passive sock
    server_sock = GetTCPSock(port);


    uint16_t l;
    char opt;
    char text[BUFFER_LEN];
    char read_buffer[BUFFER_LEN];
    while(1) {
        len = sizeof(client);
        client_sock = accept(server_sock, (struct sockaddr*) &client, &len);
        pthread_t input_thread;
        pthread_create(&input_thread, NULL, (void * (*)(void *))SendMsg, NULL);
        pthread_detach(input_thread);
        memset(read_buffer, 0, sizeof(read_buffer));
        memset(text, 0, sizeof(text));
        ExchangeName();
        while((ret = recv(client_sock, read_buffer, sizeof read_buffer, 0)) > 0){

            memset(text, 0, sizeof(text));
            memcpy(&l, read_buffer, sizeof(l));
            memcpy(&opt, read_buffer + sizeof(l), sizeof(opt));
            memcpy(text, read_buffer + sizeof(l) + sizeof(opt), ntohs(l) - sizeof(char));
            if(opt == NAME_OPTCODE){
                memcpy(peer_name, text, strlen(text));
            } else if(opt == TEXT_OPTCODE) {
                printf("%s>%s\n", peer_name, text);
            } else if(opt == BYE_OPTCODE) {
                break;
            }


            /*
               if(opt == NAME_OPTCODE){
               printf("inside opt=NAME_OPTCODE\n");
               printf("peer_name=%s\n", peer_name);
               printf("m.message=%s\n", m.message);
            //memcpy(peer_name, &m.message, sizeof(m.message));
            printf("peer_name=%s\n", peer_name);
            } 



            else if(opt == TEXT_OPTCODE) {
            printf("%s>%s", peer_name, m.message);
            } else if(opt == BYE_OPTCODE) {
            break;
            } else {
            continue;
            }*/
            //memset(read_buffer, 0, sizeof(read_buffer));
        }

        if(ret == -1) {
            ExitSysWithError("recv()");
        }
        close(client_sock);
        client_sock = 0;
        printf("terminating the chat with %s, ready to accept new connection....\n", peer_name);
        memset(peer_name, 0, sizeof(peer_name));
    }
    exit(0); 
}


void ExchangeName() {
    char buffer[BUFFER_LEN];
    int len;
    char opcode;
    uint16_t temp;

    len = sizeof(char) + strlen(my_name);
    temp = htons(len);
    opcode = NAME_OPTCODE;
    // copy bytes(len, opcode, text) into buffer
    memset(buffer, 0, sizeof(buffer));
    memcpy(buffer, &temp, sizeof(temp));
    memcpy(buffer + sizeof(temp), &opcode, sizeof(opcode));
    memcpy(buffer + sizeof(temp) + sizeof(opcode), my_name, strlen(my_name));


    if(client_sock != 0) {
        send(client_sock, buffer, sizeof buffer, 0);
    }

    if(server_sock != 0) {
        send(server_sock, buffer, sizeof buffer, 0);
    }

}

void SendMsg(){
    char user_input_buffer[BUFFER_LEN];

    char write_buffer[BUFFER_LEN];
    int len;
    char opcode;
    uint16_t temp;

    while(1) {
        memset(write_buffer, 0, sizeof(write_buffer));
        memset(user_input_buffer, 0, sizeof(user_input_buffer));
        fgets(user_input_buffer, sizeof(user_input_buffer), stdin);
        user_input_buffer[strlen(user_input_buffer) - 1] = '\0';
        if(strcasecmp(user_input_buffer, "bye") == 0) {
            opcode = BYE_OPTCODE;
        } else {
            opcode = TEXT_OPTCODE;
        }


        len = sizeof(char) + strlen(user_input_buffer);
        temp = htons(len);
        memcpy(write_buffer, &temp, sizeof(temp));
        memcpy(write_buffer + sizeof(temp), &opcode, sizeof(opcode));
        memcpy(write_buffer + sizeof(temp) + sizeof(opcode), user_input_buffer, strlen(user_input_buffer));



        //send to peer
        if(client_sock != 0) {
            send(client_sock, write_buffer,sizeof(write_buffer), 0);
        }

        if(server_sock != 0) {
            send(server_sock, write_buffer,sizeof(write_buffer), 0);
        }

    }
}

void SetUpName(){
    char read_buffer[BUFFER_LEN];
    memset(read_buffer, 0, sizeof read_buffer);
    printf("Enter Your Name: ");
    fgets(read_buffer, sizeof(read_buffer), stdin);
    read_buffer[strlen(read_buffer) - 1] = '\0';
    memcpy(my_name, read_buffer, strlen(read_buffer));
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


