#ifndef SERVERCONN_HPP_INC
#define SERVERCONN_HPP_INC

class ServerConnection {
public:
	ServerConnection(std::string hostname, std::string port);

	//Returns true if we have a healty connection with the server
	bool connected(); 

	//Returns the hostname
	std::string hostname();

	std::pair<struct stat*, int> getattr(char* path);

private:

	int dnsLookup(char* port);

	//Consecutive heartbeats missed
	size_t beatsMissed_;

	//The hostname
	std::string hostname_;

	//Socket descriptor
	int socket_;
};

#endif