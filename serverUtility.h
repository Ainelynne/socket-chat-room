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
#include <string>
#include <sstream>
#include <vector>
#include <regex>
#define SERVER_PORT "8888"
#define MAXDATASIZE 1500
#define BACKLOG 15

using namespace std;

class user {
    public:
        char* clientIP;
        int clientPort;
        int sockfd;
        char* name;

        user() {
            clientIP = (char*)malloc(20 * sizeof(char));
            clientPort = 0;
            sockfd = 0;
            name = (char*)malloc(20 * sizeof(char));
            strcpy(name , "anonymous");
        }
        user(char *newClientIP , int newClientPort , int newSockfd) {
            clientIP = (char*)malloc(20 * sizeof(char));
            strcpy(clientIP , newClientIP);
            clientPort = newClientPort;
            sockfd = newSockfd;
            name = (char*)malloc(20 * sizeof(char));
            strcpy(name , "anonymous");
        }
        ~user(){};
        void chgName(const char *newName) {
            strcpy(name , newName);
        }
        void printUser() {
            cout<< name << " from " << clientIP << ":" << clientPort;
        }
};
class server {
    public:
        int listenfd;
        int fdmax;
        vector<user *> userList;

        server() {
            listenfd = establishListener();
            fdmax = listenfd;
        }
        ~server() {
            if(listenfd > 0 ) {
                close(listenfd);
            }
        }
        int establishListener() {
            int sockfd;
            struct addrinfo hints = { 0 };
            struct addrinfo* res = 0;
            int yes = 1;

            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_flags = AI_PASSIVE;

            int addrInfoStatus;
            if((addrInfoStatus = getaddrinfo(NULL , SERVER_PORT , &hints , &res)) != 0) {
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

            if(bind(sockfd , res -> ai_addr , res -> ai_addrlen) == -1){
                close(sockfd);
                perror("bind() failed");
                if(res)
                    freeaddrinfo(res);
                return -1;
            }

            if(res)
                freeaddrinfo(res);

            if(listen(sockfd, BACKLOG) == -1){
                perror("listen() failed");
                return -1;
            }

            return sockfd;
        }
        int acceptNewConnection() {
            struct sockaddr_storage remoteaddr;
            socklen_t addrlen = sizeof remoteaddr;
            int newfd = accept(listenfd , (struct sockaddr *)&remoteaddr , &addrlen);
            return newfd;
        }
        user *getUserByfd(int fd) {
            for(unsigned int i = 0 ; i < userList.size() ; i++) {
                if(userList.at(i) -> sockfd == fd) {
                    return userList.at(i);
                }
            }

            return NULL;
        }
        user *getUserByName(const char *name) {
            for(unsigned int i = 0 ; i < userList.size() ; i++) {
                if(strcmp(userList.at(i) -> name , name) == 0) {
                    return userList.at(i);
                }
            }

            return NULL;
        }
        string itos(int i) {
            stringstream s;
            s << i;
            return s.str();
        }
        void newUserJoined(int newfd) {
            if(newfd > fdmax) {
                fdmax = newfd;
            }
            struct sockaddr_storage addr;
            char newClientIP[INET_ADDRSTRLEN];
            int newClientPort;
            socklen_t len = sizeof addr;
            getpeername(newfd , (struct sockaddr*)&addr , &len);
            struct sockaddr_in *tempAddr = (struct sockaddr_in *)&addr;
            inet_ntop(AF_INET , &tempAddr -> sin_addr , newClientIP , sizeof newClientIP);
            newClientPort = ntohs(tempAddr -> sin_port);
            user *temp = new user(newClientIP , newClientPort , newfd);
            userList.push_back(temp);
        }
        void sayHello(user *target) {
            for(unsigned int i = 0 ; i < userList.size() ; i++) {
                if(userList.at(i) != target) {
                    string msg("[Server] Someone is coming!\n");
                    send(userList.at(i) -> sockfd  , msg.c_str() , msg.length() , 0);
                }
                else {

                    string msg("[Server] Hello, anonymous! From: ");
                    msg += target -> clientIP;
                    msg += "/";
                    msg += itos(target -> clientPort);
                    msg += "\n";
                    send(userList.at(i) -> sockfd , msg.c_str() , msg.length() , 0);
                }
            }
        }
        void sayOffline(user *target) {
            for(unsigned int i = 0 ; i < userList.size() ; i++) {
                if(userList.at(i) != target) {
                    string msg("[Server] ");
                    msg += target -> name;
                    msg += " is offline.\n";
                    send(userList.at(i) -> sockfd  , msg.c_str() , msg.length() , 0);
                }
            }
        }
        void removeUser(user *target) {
            for(unsigned int i = 0 ; i < userList.size() ; i++) {
                if(userList.at(i) == target) {
                    userList.erase(userList.begin() + i);
                    free(target);
                }
            }
        }
        void parseCommand(user *target , char *inputBuffer) {
            string input(inputBuffer);
            if (!input.empty() && input[input.length()-1] == '\n') {
                input.erase(input.length() - 1);
                if(input[input.length() - 1] == '\r') {
                    input.erase(input.length() - 1);
                }
            }

            if(input == "who") {
                CMDwho(target);
            }
            else if(strncmp(input.c_str() , "name " , 5) == 0) {
                string subInput = input.substr(5);
                CMDname(target , subInput.c_str());
            }
            else if(strncmp(input.c_str() , "yell " , 5) == 0) {
                string subInput = input.substr(5);
                CMDyell(target , subInput.c_str());
            }
            else if(strncmp(input.c_str() , "tell " , 5) == 0) {
                string subInput = input.substr(5);
                CMDtell(target , subInput.c_str());
            }
            else {
                string msg("[Server] ERROR: Error command.\n");
                send(target -> sockfd , msg.c_str() , msg.length() , 0);
            }
        }
        void CMDwho(user *target) {
            string msg;
            for(unsigned int i = 0 ; i < userList.size() ; i++) {
                msg += "[Server] ";
                msg += userList.at(i) -> name;
                msg += " from ";
                msg += userList.at(i) -> clientIP;
                msg += ":";
                msg += itos(userList.at(i) -> clientPort);
                if(userList.at(i) != target) {
                    msg += "\n";
                }
                else {
                    msg += " ->me\n";
                }
            }
            send(target -> sockfd , msg.c_str() , msg.length() , 0);
        }
        void CMDname(user *target , const char *subCMD) {
            regex reg("[a-zA-Z]{2,12}");
            string str(subCMD);
            if(!regex_match(str , reg)) {
                string msg("[Server] ERROR: Username can only consists of 2~12 English letters.\n");
                send(target -> sockfd , msg.c_str() , msg.length() , 0);
                return;
            }
            else {
                if(str == "anonymous") {
                    string msg("[Server] ERROR: Username cannot be anonymous.\n");
                    send(target -> sockfd , msg.c_str() , msg.length() , 0);
                    return;
                }
                else {
                    int redundantFlag = 0;
                    for(unsigned int i = 0 ; i < userList.size() ; i++) {
                        if(str == userList.at(i) -> name && target != userList.at(i)) {
                            redundantFlag = 1;
                            break;
                        }
                    }

                    if(redundantFlag) {
                        string msg("[Server] ERROR: <NEW USERNAME> has been used by others.\n");
                        send(target -> sockfd , msg.c_str() , msg.length() , 0);
                        return;
                    }
                    else {
                        sayNewName(target , target -> name , str.c_str());
                        target -> chgName(str.c_str());
                    }
                }
            }
        }
        void sayNewName(user *target , char *oldName , const char *newName) {
            for(unsigned int i = 0 ; i < userList.size() ; i++) {
                if(userList.at(i) != target) {
                    string msg("[Server] ");
                    msg += oldName;
                    msg += " is now known as ";
                    msg += newName;
                    msg += ".\n";
                    send(userList.at(i) -> sockfd , msg.c_str() , msg.length() , 0);
                }
                else {
                    string msg("[Server] You're now known as ");
                    msg += newName;
                    msg += ".\n";
                    send(userList.at(i) -> sockfd , msg.c_str() , msg.length() , 0);
                }
            }
        }
        void CMDyell(user *target , const char *subCMD) {
            if(strlen(subCMD) == 0) {
                string msg("[Server] ERROR: Error command.\n");
                send(target -> sockfd , msg.c_str() , msg.length() , 0);
                return;
            }
            else {
                string msg("[Server] ");
                msg += target -> name;
                msg += " yell ";
                msg += subCMD;
                msg += "\n";
                for(unsigned int i = 0 ; i < userList.size() ; i++) {
                    send(userList.at(i) -> sockfd , msg.c_str() , msg.length() , 0);
                }
            }
        }
        void CMDtell(user *target , const char *subCMD) {
            if(strlen(subCMD) == 0) {
                string msg("[Server] ERROR: Error command.\n");
                send(target -> sockfd , msg.c_str() , msg.length() , 0);
                return;
            }
            else {
                string str(subCMD);
                size_t pos = str.find(" ");
                if(pos == string::npos) {
                    string msg("[Server] ERROR: Error command.\n");
                    send(target -> sockfd , msg.c_str() , msg.length() , 0);
                    return;
                }
                else if(strcmp(target -> name , "anonymous") == 0) {
                    string msg("[Server] ERROR: You are anonymous.\n");
                    send(target -> sockfd , msg.c_str() , msg.length() , 0);
                    return;
                }
                else {
                    string recvName = str.substr(0 , pos);
                    user *recvUser = getUserByName(recvName.c_str());
                    if(recvName == "anonymous") {
                        string msg("[Server] ERROR: The client to which you sent is anonymous.\n");
                        send(target -> sockfd , msg.c_str() , msg.length() , 0);
                        return;
                    }
                    else if(!recvUser) {
                        string msg("[Server] ERROR: The receiver doesn't exist.\n");
                        send(target -> sockfd , msg.c_str() , msg.length() , 0);
                        return;
                    }
                    else {
                        string tellMsg = str.substr(pos+1);

                        string msgToRecver("[Server] ");
                        msgToRecver += target -> name;
                        msgToRecver += " tell you ";
                        msgToRecver += tellMsg;
                        msgToRecver += "\n";
                        send(recvUser -> sockfd , msgToRecver.c_str() , msgToRecver.length() , 0);

                        string msgToTarget("[Server] SUCCESS: Your message has been sent.\n");
                        send(target -> sockfd , msgToTarget.c_str() , msgToTarget.length() , 0);
                    }
                }
            }
        }
};
