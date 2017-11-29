#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <cstring>
#include <iostream>
#define SERVER_PORT "8888"
#define MAXDATASIZE 1500

using namespace std;

int connectToServer(char* serverIP , char* serverPort);
int main(int argc , char *argv[]) {
    //usage: ./client <SERVER_IP> <SERVER_PORT>
    char *serverIP = (char*)malloc(20 * sizeof(char));
    char *serverPort = (char*)malloc(10 * sizeof(char));
    if(argc != 3) {
	return 1;
        strcpy(serverIP , "127.0.0.1");
        strcpy(serverPort , SERVER_PORT);
    }
    else {
        serverIP = argv[1];
        serverPort = argv[2];
    }

    int sockfd = connectToServer(serverIP , serverPort);
    if(sockfd < 0) {
        exit(1);
    }
    int fdmax = sockfd;

    fd_set read_fds;
    fd_set master;
    FD_ZERO(&read_fds);
    FD_ZERO(&master);

    FD_SET(sockfd , &master);
    FD_SET(STDIN_FILENO , &master);

    char *inputBuffer = (char *)malloc(MAXDATASIZE * sizeof(char));
    memset(inputBuffer , 0 , MAXDATASIZE);

    for(;;) {
        read_fds = master;
        select(fdmax +1 , &read_fds , NULL , NULL , NULL);

        if(FD_ISSET(STDIN_FILENO , &read_fds)) {
            memset(inputBuffer , 0 , MAXDATASIZE);
            int bytes = 0;
            char ch;
            while(read(STDIN_FILENO , &ch , 1) > 0) {
                inputBuffer[bytes] = ch;
                bytes++;
                if(ch == '\n') {
                    break;
                }
            }
            inputBuffer[bytes] = '\0';

            if(strcmp(inputBuffer , "exit\n") == 0) {
                break;
            }
            else {
                if(send(sockfd , inputBuffer , bytes , 0) < 0) {
                    perror("send");
                }
            }
        }
        if(FD_ISSET(sockfd , &read_fds)) {
            memset(inputBuffer , 0 , MAXDATASIZE);
            int bytes = 0;
            if((bytes = recv(sockfd , inputBuffer , MAXDATASIZE , 0)) < 0){
                perror("recv");
            }
            inputBuffer[bytes] = '\0';
            printf("%s" , inputBuffer);
        }
    }

    close(sockfd);
    return 0;
}

int connectToServer(char* serverIP , char* serverPort) {
    int sockfd;
    struct addrinfo hints = { 0 };
    struct addrinfo* res = 0;
    int yes = 1;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int addrInfoStatus;
    if((addrInfoStatus = getaddrinfo(serverIP , serverPort , &hints , &res)) != 0) {
        if(res)
            freeaddrinfo(res);
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(addrInfoStatus));
        return -1;
    }

    if ((sockfd = socket(res -> ai_family , res -> ai_socktype , 0)) < 0) {
        perror("socket() failed");
        if(res)
            freeaddrinfo(res);
        return -1;
    }

    if(setsockopt(sockfd , SOL_SOCKET , SO_REUSEADDR , (char*)&yes , sizeof(int)) == -1) {
        perror("setsockopt() failed");
        if(res)
            freeaddrinfo(res);
        return -1;
    }

    if (connect(sockfd , res -> ai_addr , res -> ai_addrlen) == -1) {
        perror("connect() failed");
        if(res)
            freeaddrinfo(res);
        return -1;
    }

    if(res)
        freeaddrinfo(res);
    return sockfd;
}
