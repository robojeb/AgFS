#ifndef SERVERCONN_HPP_INC
#define SERVERCONN_HPP_INC

#include <sys/stat.h>
#include <string>
#include "constants.hpp"
#include <vector>
#include <mutex>

/// Provides the client with an interface over which to talk with the server.
class ServerConnection {
public:
	ServerConnection(std::string hostname, std::string port, std::string key);
	ServerConnection(ServerConnection const& connection);

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
	std::pair<std::vector<std::pair<std::string, struct stat>>, agerr_t> readdir(const char* path);

	/**
	 * \brief Execute read on a specified path
	 * \param path String containing the path to be looked up
	 * \param size The number of bytes to read from the file.
	 * \param offset The offset to start reading from.
	 * \returns A pair of a vector of the data that the server
	 *          read from the file and an error code.
	 */
	std::pair<std::vector<unsigned char>, agerr_t> readFile(const char* path, agsize_t size, agsize_t offset);

	/**
	 * \brief Execute write on a specified path
	 * \param path String containing the path to be looked up
	 * \param size The number of bytes to write to the file.
	 * \param offset The offset to start writing at.
	 * \returns A pair of the number of bytes written and the error
	 *          code returned by the external server.
	 */
	std::pair<agsize_t, agerr_t> writeFile(const char* path, agsize_t size, agsize_t offset, const char* buf);

	/**
	 * \brief Halt communication with the server
	 * \returns an error indicating the success of the connection halt.
	 */
	agerr_t stop();

	//Perform a heartbeat operation
	agerr_t heartbeat();

	// Returns true if the connection was terminated succesfully
	bool stopped();
	
private:

	int dnsLookup(const char* port);

	//heartbeat missed or 
	bool failedCommand_;

	//
	bool connectionStopped_;

	//The hostname
	std::string hostname_;

	//A mutex to turn this object into a monitor
	std::mutex monitor_;

	//Socket descriptor
	int socket_;

	enum {
		DNS_ERROR = -1,
		SOCKET_FAILURE = -2,
		CONNECTION_FAILURE = -3
	};
};

#endif