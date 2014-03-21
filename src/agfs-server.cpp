#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#include "constants.hpp"

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


int main(int argc, char** argv){
  int lfd, connfd;
  struct sockaddr_in client;
  size_t clientlen;
  char buf[256];
  int n;

  char ipbuf[20];

  lfd = openPort(argv[1]);

  if (lfd < 0) {
    printf("no file descriptor\n");
    return 1;
  }

  while (1) {
    clientlen = sizeof client;
    connfd = accept(lfd, (struct sockaddr*)&client, &clientlen);
    if(connfd == -1) continue;

    //Could fork here
    while (1) {
      cmd_t cmd;
      read(connfd, &cmd, sizeof(cmd_t));
      switch(cmd){
      case cmd::GETATTR:
        agsize_t pathLen;
        read(connfd, &pathLen, sizeof(agsize_t));
        char* path = malloc(sizeof(char) * (pathLen + 1));
        read(connfd, path, pathLen);
        path[pathLen] = 0; //NULL terminate the string
        struct stat statBuf;
        agerr_t res = lstat(path, &statBuf);
        if (res < 0) {
          res = -errno;
          write(connfd, &res, sizeof(agerr_t));
        } else {
          write(connfd, &res, sizeof(agerr_t));
          write(connfd, &statBuf, sizeof(struct stat));
        }
        break;
      default:
        std::cerr << "Command not supported: " << cmd << std::endl;
      }
    }
    
    close(connfd);
  }
}