#ifndef SERVERCONN_HPP_INC
#define SERVERCONN_HPP_INC

#include <sys/stat.h>
#include <string>
#include "constants.hpp"

class ServerConnection {
public:
	ServerConnection(std::string hostname, std::string port, std::string key);

	//Returns true if we have a healty connection with the server
	bool connected(); 

	//Returns the hostname
	std::string hostname();

	std::pair<struct stat, agerr_t> getattr(char* path);
	std::pair<struct statvfs, agerr_t> statfs(char* path);

private:

	int dnsLookup(const char* port);

	//Consecutive heartbeats missed
	size_t beatsMissed_;

	//The hostname
	std::string hostname_;

	//Socket descriptor
	int socket_;

	enum {
		DNS_ERROR = -1,
		SOCKET_FAILURE = -2,
		CONNECTION_FAILURE = -3
	};
};

#endif