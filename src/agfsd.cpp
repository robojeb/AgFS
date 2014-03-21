#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

#include <iostream>

#include "constants.hpp"
#include "agfs-server.hpp"

int openPort(char* port){
  int listenfd, optval=1, error;
  struct addrinfo hints;
  struct addrinfo *hostaddress = NULL;
  
  memset(&hints, 0, sizeof hints);

  hints.ai_flags = AI_ADDRCONFIG | AI_PASSIVE; //get info about the port config
  hints.ai_family = AF_INET; //IPv4
  hints.ai_socktype = SOCK_STREAM; //tcp socket
  
  error = getaddrinfo(NULL, port, &hints, &hostaddress);
  if (error != 0){
    printf("get info failed\n");
    return -2;
  }
  
  if ((listenfd = socket(hostaddress->ai_family, hostaddress->ai_socktype, hostaddress->ai_protocol)) == -1){
    printf("socket failed\n");
    return -1;
  }

  if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
     (const void *) &optval, sizeof(optval)) == -1){
    error = errno;
    printf("opt failed\n");
    if (error == EBADF) printf("Not a socket");
    if (error == EFAULT) printf("fault");
    if (error == EINVAL) printf("einval");
    return -1;
  }
  
  if (bind(listenfd, hostaddress->ai_addr, hostaddress->ai_addrlen) == -1){
    printf("bind failed\n");
    return -1;
  }
 
  if (listen(listenfd, 1) == -1){
    printf("listen failed\n");
    return -1;
  }
  
  return listenfd;
}

using namespace std;

int main(int argc, char** argv){

  if(argc != 2) {
    cout << "Usage: agfs {port}" << endl;
    return 0;
  }

  int lfd, connfd;
  struct sockaddr_in client;
  socklen_t clientlen;

  lfd = openPort(argv[1]);

  if (lfd < 0) {
    printf("Could not open file descriptor for listening\n");
    return 1;
  }

  while (1) {
    clientlen = sizeof client;
    connfd = accept(lfd, (struct sockaddr*)&client, &clientlen);
    if(connfd == -1) continue;

    pid_t pid;
    //Could fork here
    if((pid = fork()) < 0) {
      cerr << "Fork Error" << endl;
    } else if(pid == 0) {
      close(lfd);
      agfsServer(connfd);
      break;
    }
    cerr << "Forked client" << endl;
    close(connfd);
  }
}