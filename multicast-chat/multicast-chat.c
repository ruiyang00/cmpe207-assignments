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
#include <netinet/in.h>



/*constents, global varibles*/
#define BUFFER_LEN 1024
char TEXT_OPTCODE='1';
char BYE_OPTCODE='2';
char myName[100];
char *MultiCastIPAddr;
int UDPport;
int udp_sock;
struct sockaddr_in multiAddr;
struct ip_mreq group;



void SendMessage();
void RecvMessage();
void ExitSysWithError();
void ExitWithOptError();
void InitChat();
void InitName();
void EnableLoopBack();
void SetTTL();
void JoinGroup(); 
void ReuseAddr();

int main(int argc, char **argv){

    int opt;

    if(argc != 5) {
        ExitWithOptError();
    }


    static struct option long_opts[] = {
        { "mcip", required_argument, NULL, 'a' },
        {"port", required_argument, NULL, 'p'},
        {0, 0, 0, 0}
    };


    while( (opt = getopt_long(argc, argv, "a:p:", long_opts, NULL)) != -1) {
        switch(opt) {
            case 'a':
                MultiCastIPAddr=optarg;
                break;
            case 'p':
                UDPport=atoi(optarg);
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



    InitChat();

    return 0;

}

void InitChat(){
    struct hostent *he;

    /*clean the structs preventing garbage left from other programs*/
    memset(&group, 0, sizeof(group));
    memset(&multiAddr, 0, sizeof(multiAddr));
    /* set up name for a char user to join group*/
    InitName();

    udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(udp_sock == -1) {
        ExitSysWithError("socket()");
    }

    he = gethostbyname(MultiCastIPAddr);
    if(he == NULL) {
        ExitSysWithError("gethostbyname()");
    }

    multiAddr.sin_family = AF_INET;
    multiAddr.sin_port = htons(UDPport);
    multiAddr.sin_addr.s_addr = inet_addr(MultiCastIPAddr); 


    /*enable to reuse the same addr and port*/
    ReuseAddr();


    if(bind(udp_sock, (struct sockaddr*) &multiAddr, sizeof(multiAddr)) < 0) {
        ExitSysWithError("Creating UDP sock failed with the bind() call");
    }


    /*TBD: enable loop back message in which the sender recevie a message just sent*/
    EnableLoopBack(); 

    /*TBD: set TTL*/
    SetTTL();

    /*TBD: join the multicast group*/

    //group.imr_multiaddr = multiAddr.sin_addr;
    //group.imr_interface.s_addr = htonl(INADDR_ANY);
    //if(setsockopt(udp_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &group, sizeof(group)) < 0) {
    //    ExitSysWithError("JoinGroup()");
    //}
    JoinGroup();


    /* create a thread to send group message */



    pthread_t input_thread;
    if(pthread_create(&input_thread, NULL, (void * (*)(void *))SendMessage, (void *) (intptr_t)udp_sock) < 0){
        ExitSysWithError("pthread_creat()");
    } else {
        pthread_detach(input_thread);
    }

    // recv message
    RecvMessage();
}


void RecvMessage(){
    int n, text_len, name_len, len;
    uint16_t l;
    char opcode, name_len_ch;
    char name[100];
    char text[BUFFER_LEN];
    char read_buffer[BUFFER_LEN];
    struct sockaddr_in from;

    len = sizeof(from);
    while(1) {
        memset(read_buffer,0 ,sizeof(read_buffer));
        memset(text,0 ,sizeof(text));
        memset(name,0 ,sizeof(name));
        if((n = recvfrom(udp_sock, read_buffer, sizeof read_buffer, 0, (struct sockaddr*)&from, &len)) < 0) {
            ExitSysWithError("recvfrom()");
            break;
        }
        read_buffer[n] = '\0';
        
        memcpy(&opcode, read_buffer, sizeof(opcode));
        if(opcode == BYE_OPTCODE) {
            memcpy(name, read_buffer + sizeof(opcode), sizeof(read_buffer));
            printf("%s leave the chat room\n", name);
        } else {
            memcpy(&name_len_ch, read_buffer + sizeof(opcode), sizeof(name_len_ch));
            name_len = name_len_ch - '0';
            memcpy(name, read_buffer + sizeof(opcode) + sizeof(name_len_ch), name_len);
            memcpy(&l, read_buffer + sizeof(opcode) + sizeof(name_len_ch) + name_len, sizeof(l));
            text_len = htons(l);
            memcpy(text, read_buffer + sizeof(opcode) + sizeof(name_len_ch) + name_len + sizeof(l), text_len);
            printf("%s>%s\n",name,text);
        }
    }
}

void JoinGroup() {
    group.imr_multiaddr = multiAddr.sin_addr;
    group.imr_interface.s_addr = htonl(INADDR_ANY);
    if(setsockopt(udp_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &group, sizeof(group)) < 0) {
        ExitSysWithError("JoinGroup()");
    }
}

void LeaveGroup(){
    if(setsockopt(udp_sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, &group, sizeof(group)) < 0) {
        ExitSysWithError("JoinGroup()");
    }
}

void EnableLoopBack(){
    int loop_val = 1;
    if(setsockopt(udp_sock, IPPROTO_IP, IP_MULTICAST_LOOP, &loop_val, sizeof(loop_val)) < 0) {
        ExitSysWithError("EnableLoopBack()");
    } 
}
void SetTTL() {
    int ttl_val = 10;
    if(setsockopt(udp_sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl_val, sizeof(ttl_val))) {
        ExitSysWithError("SetTTL");
    } 
}
void ReuseAddr(){
    int optval = 1;
    if(setsockopt(udp_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))){
        ExitSysWithError("ReuseAddr()");
    }
}


void SendMessage(){
    char user_input_buffer[BUFFER_LEN];
    char write_buffer[BUFFER_LEN];
    int name_len, text_len;
    char opcode, name_len_ch;
    uint16_t text_len_netbyte;

    name_len = strlen(myName) + '0';
    name_len_ch = (char) name_len; 

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

        text_len = strlen(user_input_buffer);
        text_len_netbyte = htons(text_len);

        if(opcode == BYE_OPTCODE) {
            memcpy(write_buffer, &opcode, sizeof(opcode));
            memcpy(write_buffer + sizeof(opcode), myName, strlen(myName));
            if(sendto(udp_sock, write_buffer, sizeof(write_buffer), 0, (struct sockaddr*) &multiAddr, sizeof(multiAddr)) < 0) {
                ExitSysWithError("SendMessage() -> sendto()");
            }
            LeaveGroup();
            exit(0);
        } else {
            /*dump data(opcode:name_len:name_ascii:text_len(network-byte-order):text_ascii) into write buffer */
            memcpy(write_buffer, &opcode, sizeof(opcode));
            memcpy(write_buffer + sizeof(opcode), &name_len_ch, sizeof(name_len_ch));
            memcpy(write_buffer + sizeof(opcode) + sizeof(name_len_ch), myName, strlen(myName));
            memcpy(write_buffer + sizeof(opcode) + sizeof(name_len_ch) + 
                    strlen(myName), &text_len_netbyte, sizeof(text_len_netbyte));
            memcpy(write_buffer + sizeof(opcode) + sizeof(name_len_ch) 
                    + strlen(myName) + sizeof(text_len_netbyte), user_input_buffer, sizeof(user_input_buffer));

            if(sendto(udp_sock, write_buffer, sizeof(write_buffer), 0, (struct sockaddr*) &multiAddr, sizeof(multiAddr)) < 0) {
                ExitSysWithError("SendMessage() -> sendto()");
            }

        }
    }
}


void InitName(){
    char read_buffer[100];
    memset(read_buffer, 0, sizeof read_buffer);
    printf("Enter Your Chat Name: ");
    fgets(read_buffer, sizeof(read_buffer), stdin);
    read_buffer[strlen(read_buffer) - 1] = '\0';
    //read_buffer[strlen(read_buffer)] = '\0';
    memcpy(myName, read_buffer, strlen(read_buffer));
    memset(read_buffer, 0, sizeof(read_buffer));
}



void ExitWithOptError() {
    printf("The program only accept long opt args  --port xxx --mcip xxx.xxx.xxx.xxx in any order\n");
    printf("ex:./multicast-chat --mcip 123.0.1.219 --port 12000\n");
    printf("Note: Make sure using the same port for every char user to join the multicast chat group\n");
    exit(1);
}

void ExitSysWithError(char *call){
    fprintf(stderr, "Syscall %s failed with errno=%d, msg=%s\n", call, errno, strerror(errno));
    exit(-1);
}
