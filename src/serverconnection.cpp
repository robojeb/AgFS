#include "serverconnection.hpp"

ServerConnection::ServerConnection(std::string hostname, std::string port):
	beatsMissed_{0},
	hostname_{hostname},
	socket_{-1}
{
	socket_ = dnsLookup(port.cstr());
	switch (socket_){
	case -1:
		std::cerr << "Failed DNS lookup" << std::endl;
		break;
	case -2:
		std::cerr << "Failed to create socket" << std::endl;
		break;
	case -3:
		std::cerr << "Could not connect to server " << hostname << ":" << port << std::endl;
		break;
	default:
		break;
	}
};

bool ServerConnection::connected()
{
	if (socket_ < 0) {
		return false;
	}
}

/******************
 * FUSE functions *
 ******************/

std::pair<struct stat, int> ServerConnection::getattr(char* path)
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
	return new std::pair<struct stat, int>(readValues, errVal);
}

/*********************
 * Private Functions *
 *********************/

int ServerConnection::dnsLookup(char* port)
{
  struct addrinfo hints, *hostaddress = NULL;
  int error, fd;

  memset(&hints, 0, sizeof(hints));
  
  hints.ai_flags = AI_ADDRCONFIG | AI_V4MAPPED;
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_CANONNAME;
  
  if ((error = getaddrinfo(hostname_.cstr(), port, &hints, &hostaddress)) != 0) {
    return -1;
  }

  fd = socket(hostaddress->ai_family, hostaddress->ai_socktype, hostaddress->ai_protocol);

  if (fd == -1) {
  	return -2;
  }

  if (connect(fd, hostaddress->ai_addr, hostaddress->ai_addrlen) == -1) {
  	return -3;
  }

  freeaddrinfo(hostaddress);

  return fd;
}