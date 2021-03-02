#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#define UDP 2
#define TCP 1
#define ON 1
#define OFF 0

void validateArgv(int argc,char **argv, int *portnum, int *trans_flag);
void exitWithError();
void exitSysWithError(char *call);

int main(int argc, char **argv) {

	int port_flag;
	int trans_flag;

	//TBD: check input argvi
	validateArgv(argc, argv, &port_flag, &trans_flag);
	printf("port_number=%d\n",port_flag);
	printf("trans_flag=%d\n",trans_flag);


	


	return 0;
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
	
	//unameserver -tcp|-udp portnumber
	if(argc > 3 || argc < 2) {
		exitWithError("input arguments either too less/many");
	}

	if(argc == 3) {
		*portnum = ON;	
	}
	
	if(strcmp(argv[1],"-tcp") == 0) {
		*trans_flag = TCP;
	} else if(strcmp(argv[1],"-udp") == 0) {
		*trans_flag = UDP;
	} else {
		exitWithError("unrecognized transport. We only accpte [-tcp|-udp]");
	}

	
}
