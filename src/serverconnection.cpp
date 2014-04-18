#include <iostream>
#include "serverconnection.hpp"
#include "constants.hpp"
#include "agfsio.hpp"
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fuse.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <endian.h>

ServerConnection::ServerConnection(std::string hostname, std::string port, std::string key):
	beatsMissed_{false},
	hostname_{hostname},
	socket_{-1}
{
	//Perform DNS lookup and handle DNS errors.
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
	agfs_read_cmd(socket_, servResp);

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
		std::cerr << "Other/No response: " << servResp << std::endl;
		close(socket_);
		socket_ = -1;
		break;
	}
};

ServerConnection::ServerConnection(ServerConnection const& connection) :
	beatsMissed_{connection.beatsMissed_},
	hostname_{connection.hostname_},
	socket_{connection.socket_}
{
	//Copy constructor!
}

bool ServerConnection::connected()
{
	if (socket_ < 0) {
		return false;
	}
	return true;
}

agerr_t ServerConnection::stop()
{
	std::lock_guard<std::mutex> l{monitor_};
	agerr_t error = 0;
	if (connected()) {
		agfs_write_cmd(socket_, cmd::STOP);
		error = close(socket_);
		socket_ = -1;
	}
	return error;
}

/******************
 * FUSE functions *
 ******************/

/*
 * Function stack objects with [] around them indicate tthat they are optional
 * or not always present, based on preceding objects in the stack.
 */

/*
 * Outgoing stack looks like:
 * 
 *      STRING
 *
 * Incoming stack looks like:
 *
 *      ERROR [STAT]
 */
std::pair<struct stat, agerr_t> ServerConnection::getattr(const char* path)
{
	std::lock_guard<std::mutex> l{monitor_};
	//Let server know we want file metadata
	cmd_t cmd = cmd::GETATTR;
	agfs_write_cmd(socket_, cmd);
	
	//Outgoing stack calls
	agfs_write_string(socket_, path);

	agerr_t error = 0;
	agfs_read_error(socket_, error);

	struct stat readValues;
	memset(&readValues, 0, sizeof(struct stat));
	if(error >= 0) {
		agfs_read_stat(socket_, readValues);
	}

	return std::pair<struct stat, agerr_t>(readValues, error);
}

/*
 * Outgoing stack looks like:
 * 
 *      STRING MASK
 *
 * Incoming stack looks like:
 *
 *      ERROR
 */
agerr_t ServerConnection::access(const char* path, int mask)
{
	std::lock_guard<std::mutex> l{monitor_};
	//Let server know we want file access
	cmd_t cmd = cmd::ACCESS;
	agfs_write_cmd(socket_, cmd);

	//Outgoing stack calls
	agfs_write_string(socket_, path);

	agfs_write_mask(socket_, (agmask_t)mask);
	
	//Incoming stack calls
	agerr_t retValue;
	agfs_read_error(socket_, retValue);

	return retValue;
}

/*
 * Outgoing stack looks like:
 * 
 *      STRING
 *
 * Incoming stack looks like:
 *
 *      ERROR [COUNT [STRING STAT]*]
 */
std::pair<std::vector<std::pair<std::string, struct stat>>, agerr_t> ServerConnection::readdir(const char* path)
{
	std::lock_guard<std::mutex> l{monitor_};
	//Let server know we want to read a directory. 
	cmd_t cmd = cmd::READDIR;
	agfs_write_cmd(socket_, cmd);

	//Outgoing stack calls
	agfs_write_string(socket_, path);
	
	//Incoming stack calls
	agerr_t error = 0;
	agfs_read_error(socket_, error);

	//Since the server doesn't send data on errors, we need to
	//short circuit.
	std::vector<std::pair<std::string, struct stat>> files;
	if (error >= 0) {
		agsize_t count = 0;
		agfs_read_size(socket_, count);

		while (count-- > 0) {
			std::string file;
			agfs_read_string(socket_, file);

			struct stat stbuf;
			agfs_read_stat(socket_, stbuf);

			files.push_back(std::pair<std::string, struct stat>{file, stbuf});
		}
	}

	return std::pair<std::vector<std::pair<std::string, struct stat>>, agerr_t>{files, error};
}

/*
 * Outgoing stack looks like:
 * 
 *      STRING SIZE OFFSET
 *
 * Incoming stack looks like:
 *
 *      ERROR [SIZE [DATA]*]
 */
std::pair<std::vector<unsigned char>, agerr_t> ServerConnection::readFile(const char* path, agsize_t size, agsize_t offset)
{
	//Send command to read data from file.
	agfs_write_cmd(socket_, cmd::READ);

	//Send parameters
	agfs_write_string(socket_, path);
	agfs_write_size(socket_, size);
	agfs_write_size(socket_, offset);

	agerr_t error = 0;
	agfs_read_error(socket_, error);

	std::vector<unsigned char> data;
	if (error >= 0) {
		agsize_t amount_read = 0;
		agfs_read_size(socket_, amount_read);
		std::cerr << "size: " << amount_read << std::endl;

		data.resize(amount_read);
		error = std::min((long long)read(socket_, &data[0], amount_read), (long long)error);
	}

	return std::pair<std::vector<unsigned char>, agerr_t>{data, error};
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