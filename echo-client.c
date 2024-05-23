#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>

#define MAXBUF 1024


void usage(){
    printf("syntax : echo-client <ip> <port>\n");
    printf("sample : echo-client 192.168.10.2 1234\n");
}
pthread_mutex_t ClientMutex = PTHREAD_MUTEX_INITIALIZER;
int ClientSock;
struct sockaddr_in ServerAddr;
int ReturnFlag = 0;

void* RcvMessage(void* arg){
    int RcvRet = 0;
    char RcvBuf[MAXBUF];
    while(1){
        pthread_mutex_lock(&ClientMutex);
        RcvRet = recv(ClientSock, RcvBuf, MAXBUF-1, 0);
        pthread_mutex_unlock(&ClientMutex);
        if(RcvRet > 0){
            printf("Server's message\n");
            RcvBuf[RcvRet] = '\0';
            printf("%s\n", RcvBuf);
            continue;
        }
        else if(RcvRet == 0){
            printf("Disconnected to server\n");
            printf("You press any key, program end!!!\n");
            ReturnFlag = 1;
            break;
        }
        else{
            printf("Server don't response message\n");
            continue;
        }
    }

}
int main(int argc, char* argv[]){

    if(argc!=3){
        usage();
        return -1;
    }
    char SndBuf[MAXBUF] = {0,};
    int ConnectCheck;
    int RcvRet;
    pthread_t ThreadNum;

    ClientSock = socket(PF_INET, SOCK_STREAM, 0);
    if(ClientSock < 0){
        printf("FAIL to create socket\n");
        exit(1);
    }

    /*server information*/
    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_port = htons(atoi(argv[2]));
    if(inet_pton(AF_INET, argv[1], &ServerAddr.sin_addr)!=1){
        printf("Improperty IP address\n");
        exit(1);
    }

    ConnectCheck = connect(ClientSock, (struct sockaddr*)&ServerAddr, sizeof(ServerAddr));
    if(ConnectCheck<0){
        printf("FAIL to connect\n");
        close(ClientSock);
        exit(1);
    }

    if(pthread_create(&ThreadNum, NULL, RcvMessage, NULL)!=0){
        printf("FAIL to create thread!!!\n");
        return -1;
    }
    pthread_detach(ThreadNum);

    while(1){
        scanf("%s", SndBuf);
        if(send(ClientSock, SndBuf, strlen(SndBuf), 0)!=strlen(SndBuf)){
            printf("FAIL to send!!!\n");
            continue;
        }
        if(ReturnFlag){
            break;
        }
    }
    close(ClientSock);
    return 0;
}