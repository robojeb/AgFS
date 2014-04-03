#ifndef AGFS_SERVER_HPP_INCLUDE
#define AGFS_SERVER_HPP_INCLUDE

#include <boost/filesystem.hpp>


/**
* \brief Provides an interface to a connected client.
*/

class ClientConnection {
public:
	/**
	 * \brief Create an instance of the connection class.
	 * \param connFd The file descriptor for the connection to be used.
	 */
	ClientConnection(int connFd);

  /**
	 * Check whether the connection is still active.
	 */
	bool connected();

  /**
	 * 
	 */
	void processCommands();

private:
	void processGetAttr();
	void processAccess();
	void processHeartbeat();
	void processReaddir();

	boost::filesystem::path mountPoint_;
	int fd_;
};


#endif
