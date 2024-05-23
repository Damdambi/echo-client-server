#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>

#define MAXCLIENTS 20
#define MAXBUF 1024


void usage(){
    printf("syntax : echo-server <port> [-e[-b]]\n");
    printf("sample : echo-server 1234 -e -b\n");
}

typedef struct{
    int SockFd;
    struct sockaddr_in Addr;
} client_t;

int EchoFlag = 0;
int BroadFlag = 0;

client_t Clients[MAXCLIENTS];

pthread_mutex_t ClientMutex = PTHREAD_MUTEX_INITIALIZER;

void BroadCastClient(char* SndBuf, int CurrentIDX){
    for(int i=0; i< MAXCLIENTS; i++){
        if(Clients[i].SockFd != -1 && i != CurrentIDX){
            send(Clients[i].SockFd, SndBuf, strlen(SndBuf), 0);
        }
    }
}

void* ClientRcvSnd(void* pClient){
    int ClientIDX = *(int*)pClient;
    char RcvBuf[MAXBUF];
    int ret;

    free(pClient);
    while(1){
        ret = recv(Clients[ClientIDX].SockFd, RcvBuf, MAXBUF, 0);
        if(ret>0){
            RcvBuf[ret] = '\0';
            printf("%d Client's message\n", ClientIDX);
            printf("%s\n", RcvBuf);
            if(EchoFlag){
                send(Clients[ClientIDX].SockFd, RcvBuf, strlen(RcvBuf), 0);
            }
            if(BroadFlag){
                pthread_mutex_lock(&ClientMutex);   
                BroadCastClient(RcvBuf, ClientIDX);
                pthread_mutex_unlock(&ClientMutex);    
            }
        }
        else if(ret==0){
            printf("Disconncted %d Clients\n", ClientIDX);
            Clients[ClientIDX].SockFd = -1;
            break;
        }
        else{
            printf("Error code: %d\n", ret);
            printf("Receive fail!!!\n");
            Clients[ClientIDX].SockFd = -1;
            break;
        }
    }
}

int main(int argc, char* argv[]){
    /*check main parameter*/
    if(argc<2 || argc>4){
        usage();
        return -1;
    }
    
    if(argc == 3){
        if(strcmp(argv[2], "-e")==0) EchoFlag = 1;
        else{
            printf("Use option -e\n");
            return -1;
        }
    }

    if(argc == 4){
        if(strcmp(argv[2], "-e")==0){
            if(strcmp(argv[3], "-b")==0){
                EchoFlag = 1;
                BroadFlag = 1;
            }
            else{
                printf("Use option -e -b\n");
                return -1;
            }
        }
        else{
            printf("Use option -e -b\n");
            return -1;
        }
    }
    /*main start!!!*/
    int ServerSock;
    int NewSock;
    int PortNum;
    struct sockaddr_in ServerAddr;
    struct sockaddr_in ClientAddr;
    int AddrLen = sizeof(ClientAddr);
    pthread_t ThreadNum;
    
    for(int i=0; i<MAXCLIENTS; i++){
        Clients[i].SockFd = -1;
    }

    /*ipv4, TCP*/
    ServerSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(ServerSock < 0){
        printf("FAIL to create socket\n");
        exit(1);
    }

    /*Allocated ip address, port number to socket*/
    memset(&ServerAddr, 0, sizeof(ServerAddr));
    PortNum = atoi(argv[1]);
    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_port = htons(PortNum);
    ServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(ServerSock, (struct sockaddr*)&ServerAddr, sizeof(ServerAddr)) < 0){
        printf("FAIL to binding\n");
        close(ServerSock);
        exit(1);
    }

    if(listen(ServerSock, 5) < 0){
        printf( "FAIL to listening\n");
        close(ServerSock);
        exit(1);
    }

    while(1){
        NewSock = accept(ServerSock, (struct sockaddr*)&ClientAddr, (socklen_t*)&AddrLen);
        if(NewSock<0){
            printf("FAIL to accept\n");
            close(ServerSock);
            return -1;
        }

        /*find client in CLIENTS[i]*/
        int ClientIDX = -1;
        pthread_mutex_lock(&ClientMutex);
        for(int i=0; i<MAXCLIENTS; i++){
            if(Clients[i].SockFd==-1){
                Clients[i].SockFd = NewSock;
                Clients[i].Addr = ClientAddr;
                ClientIDX = i;
                break;
            }
        }
        pthread_mutex_unlock(&ClientMutex);

        if(ClientIDX == -1){
            printf("Client is too much!!!\n");
            close(NewSock);
            continue; /*again repeat while*/
        }

        int* pClient = malloc(sizeof(int));
        *pClient = ClientIDX;
        if(pthread_create(&ThreadNum, NULL, ClientRcvSnd, pClient) != 0){
            printf("FAIL create thread!!!\n");
            free(pClient);
        }
        pthread_detach(ThreadNum);
    }

    close(ServerSock);

    return 0;
}