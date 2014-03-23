#include <iostream>
#include "serverconnection.hpp"
#include "constants.hpp"
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fuse.h>
#include <fcntl.h>

ServerConnection::ServerConnection(std::string hostname, std::string port, std::string key):
	beatsMissed_{0},
	hostname_{hostname},
	socket_{-1}
{
	socket_ = dnsLookup(port.c_str());

	switch (socket_){
	case DNS_ERROR:
		std::cerr << "Failed DNS lookup" << std::endl;
		return;
		break;
	case SOCKET_FAILURE:
		std::cerr << "Failed to create socket" << std::endl;
		return;
		break;
	case CONNECTION_FAILURE:
		std::cerr << "Could not connect to server " << hostname << ":" << port << std::endl;
		return;
		break;
	default:
		break;
	}

	//Set timeout
	struct timeval tv;
	tv.tv_sec = CLIENT_BLOCK_SEC;
	tv.tv_usec = CLIENT_BLOCK_USEC;
	setsockopt(socket_, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)(&tv), sizeof(struct timeval));
	int iMode = 0;
	ioctl(socket_, FIONBIO, &iMode);  

	//send key for verification
	write(socket_, key.c_str(), ASCII_KEY_LEN);
	cmd_t servResp;
	int n = read(socket_, &servResp, sizeof(cmd_t));
	std::cout << n << std::endl;

	switch(servResp) {
	case cmd::INVALID_KEY:
		std::cerr << "Server " << hostname << ": Invalid key" << std::endl;
		close(socket_);
		socket_ = -1;
		break;
	case cmd::MOUNT_NOT_FOUND:
		std::cerr << "Server " << hostname << ": Remote mount not found" << std::endl;
		close(socket_);
		socket_ = -1;
		break;
	case cmd::USER_NOT_FOUND:
		std::cerr << "Server " << hostname << ": Remote user not found" << std::endl;
		close(socket_);
		socket_ = -1;
		break;
	case cmd::ACCEPT:
		std::cerr << "Connected to server" << std::endl;
		break;
	default: 
		std::cerr << "Other/No response" << std::endl;
		close(socket_);
		socket_ = -1;
		break;
	}
};

bool ServerConnection::connected()
{
	if (socket_ < 0) {
		return false;
	}
	return true;
}

/******************
 * FUSE functions *
 ******************/

std::pair<struct stat, agerr_t> ServerConnection::getattr(char* path)
{
	//Write command and path
	cmd_t cmd = cmd::GETATTR;
	write(socket_, &cmd, sizeof(cmd_t));
	agsize_t pathLen = strlen(path);
	write(socket_, &pathLen, sizeof(agsize_t));
	write(socket_, path, pathLen);
	struct stat readValues;
	memset(&readValues, 0, sizeof(struct stat));
	agerr_t errVal;
	read(socket_, &errVal, sizeof(agerr_t));
	if(errVal >= 0) {
		read(socket_, &readValues, sizeof(struct stat));
	}
	return std::pair<struct stat, agerr_t>(readValues, errVal);
}

std::pair<struct statvfs, agerr_t> ServerConnection::statfs(char* path)
{
	//Write command and path
	cmd_t cmd = cmd::GETATTR;
	write(socket_, &cmd, sizeof(cmd_t));
	agsize_t pathLen = strlen(path);
	write(socket_, &pathLen, sizeof(agsize_t));
	write(socket_, path, pathLen);
	
	struct statvfs readValues;
	memset(&readValues, 0, sizeof(struct statvfs));
	agerr_t errVal;

	read(socket_, &errVal, sizeof(agerr_t));
	if (errVal >= 0) {
		read(socket_, &readValues, sizeof(struct statvfs));
	}
	return std::pair<struct statvfs, agerr_t>(readValues, errVal);
}

/*********************
 * Private Functions *
 *********************/

int ServerConnection::dnsLookup(const char* port)
{
  struct addrinfo hints, *hostaddress = NULL;
  int error, fd;

  memset(&hints, 0, sizeof(hints));
  
  hints.ai_flags = AI_ADDRCONFIG | AI_V4MAPPED;
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_CANONNAME;
  
  if ((error = getaddrinfo(hostname_.c_str(), port, &hints, &hostaddress)) != 0) {
    return DNS_ERROR;
  }

  fd = socket(hostaddress->ai_family, hostaddress->ai_socktype, hostaddress->ai_protocol);

  if (fd == -1) {
  	return SOCKET_FAILURE;
  }

  if (connect(fd, hostaddress->ai_addr, hostaddress->ai_addrlen) == -1) {
  	return CONNECTION_FAILURE;
  }

  freeaddrinfo(hostaddress);

  return fd;
}