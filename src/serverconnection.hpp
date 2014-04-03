#ifndef SERVERCONN_HPP_INC
#define SERVERCONN_HPP_INC

#include <sys/stat.h>
#include <string>
#include "constants.hpp"
#include <vector>

/// Provides the client with an interface over which to talk with the server.
class ServerConnection {
public:
	ServerConnection(std::string hostname, std::string port, std::string key);

	/// Returns true if we have a healty connection with the server
	bool connected(); 

	/// Returns the hostname
	std::string hostname();

	/**
	 * \brief Execute getattr on a specified path
	 * \param path String containing the path to be looked up
	 * \returns A pair containing a stat struct and any error generated.
	 */
	std::pair<struct stat, agerr_t> getattr(const char* path);

	/**
	 * \brief Execute access on a specified path
	 * \param path String containing the path to be looked up
	 * \returns The error code generated
	 */
	agerr_t access(const char* path, int mask);

	/**
	 * \brief Execute readdir on a specified path
	 * \param path String containing the path to be looked up
	 * \returns a pair of a vector containing the children files/directories, 
	  *         and any error generated.
	 */
	std::pair<std::vector<std::string>, agerr_t> readdir(const char* path);

	/// Stop the connection
	agerr_t stop();
	
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