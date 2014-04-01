#ifndef AGFS_SERVER_HPP_INCLUDE
#define AGFS_SERVER_HPP_INCLUDE

#include <boost/filesystem.hpp>

class ClientConnection {
public:
	ClientConnection(int connFd);

	bool connected();

	void processCommands();

private:
	void processGetAttr();
	void processAccess();
	void processHeartbeat();

	boost::filesystem::path mountPoint_;
	int fd_;
};


#endif
