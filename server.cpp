#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <iostream>
#include <string>
#include "serverUtility.h"
#define SERVER_PORT "8888"
#define MAXDATASIZE 1500
#define BACKLOG 15

using namespace std;

int main(int argc , char *argv[]) {
    server *myServer = new server();

    fd_set read_fds;
    fd_set master;
    FD_ZERO(&read_fds);
    FD_ZERO(&master);

    FD_SET(myServer -> listenfd , &master);

    char *inputBuffer = (char *)malloc(MAXDATASIZE * sizeof(char));
    memset(inputBuffer , 0 , MAXDATASIZE);

    cout << "Ready for connection" <<endl;

    for(;;) {
        read_fds = master;
        select(myServer -> fdmax +1 , &read_fds , NULL , NULL , NULL);
        for(int i = 0 ; i <= myServer -> fdmax ; i++) {
            if(FD_ISSET(i , &read_fds)) {
                if(i == myServer -> listenfd) {
                    //new client incoming
                    int newfd = myServer -> acceptNewConnection();
                    if(newfd == -1) {
                        perror("accept() failed");
                    }
                    else {
                        //new connection established
                        FD_SET(newfd , &master);
                        myServer -> newUserJoined(newfd);

                        user *temp = myServer -> getUserByfd(newfd);
                        cout << "[New User] ";
                        temp -> printUser();
                        cout << endl;

                        myServer -> sayHello(temp);
                    }
                }
                else {
                    //msg from old client
                    int bytes = 0;
                    memset(inputBuffer , 0 , MAXDATASIZE);
                    user *temp = myServer -> getUserByfd(i);

                    if((bytes = recv(i , inputBuffer , MAXDATASIZE , 0)) <= 0) {
                        if(bytes == 0) {
                            //close connection
                            //server log
                            cout << "[Disconnected] ";
                            temp -> printUser();
                            cout << endl;
                        }
                        else {
                            perror("recv");
                        }
                        //
                        close(i);
                        FD_CLR(i , &master);
                        myServer -> sayOffline(temp);
                        myServer -> removeUser(temp);
                    }
                    else {
                        //command from old client
                        inputBuffer[bytes] = '\0';
                        //server log
                        cout << "[Userinput] ";
                        temp -> printUser();
                        cout << ": ";
                        printf("%s" , inputBuffer);
                        myServer -> parseCommand(temp , inputBuffer);
                    }
                }
            }
        }
    }

    return 0; //never reached
}
