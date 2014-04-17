#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <cstring>
#include <pwd.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include "agfs-server.hpp"
#include "constants.hpp"
#include "agfsio.hpp"

ClientConnection::ClientConnection(int connfd) {

	//Set timeout
	struct timeval tv;
	tv.tv_sec = SERVER_BLOCK_SEC;
	tv.tv_usec = SERVER_BLOCK_USEC;
	setsockopt(connfd, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)(&tv), sizeof(struct timeval));
	int iMode = 0;
	ioctl(connfd, FIONBIO, &iMode); 

	//Verify server key
	char key[ASCII_KEY_LEN+1];
	memset(key, 0, ASCII_KEY_LEN+1);
	read(connfd, key, ASCII_KEY_LEN);

	std::fstream authkeys;
	authkeys.open(KEY_LIST_PATH, std::fstream::in);

	std::string clientKey{key};
	std::string mountPath, user, serverKey;
	bool validKey = false;

	while(!authkeys.eof()) {
		authkeys >> mountPath >> user >> serverKey;
		std::cout << "CLIENT: " << clientKey << std::endl;
		std::cout << "SERVER: " << serverKey << std::endl;
		std::cout << serverKey.length() << " " << clientKey.length() << std::endl;
		if(serverKey.compare(clientKey) == 0) {
			validKey = true;
			break;
		}
	}

	if(validKey == false) {
		agfs_write_cmd(connfd, cmd::INVALID_KEY);
		close(connfd);
		fd_ = -1;
		return;
	}

	//Change user
	struct passwd* userPwd = getpwnam(user.c_str());
	if(userPwd == NULL) {
		agfs_write_cmd(connfd, cmd::USER_NOT_FOUND);
		close(connfd);
		fd_ = -1;
		return;
	} else {
		setuid(userPwd->pw_uid);
		setgid(userPwd->pw_gid);
	}

	//Check that mount point exists
	if(!boost::filesystem::exists(mountPath)) {
		agfs_write_cmd(connfd, cmd::MOUNT_NOT_FOUND);
		close(connfd);
		fd_ = -1;
		return;
	}

	fd_ = connfd;
	mountPoint_ = mountPath;
}

bool ClientConnection::connected() {
	return fd_ > -1;
}

void ClientConnection::processCommands() {
	//Passed all checks accept connection and keep alive until heartbeat
	//failure or explicit stop
	agfs_write_cmd(fd_, cmd::ACCEPT);
	while(1) {
		cmd_t cmd = cmd::GETATTR;
		int respVal = agfs_read_cmd(fd_, cmd);
		if(respVal <= 0) {
			std::cout << "Missed heartbeat" << std::endl;
		} else {
			switch(cmd) {
			case cmd::STOP:
				std::cerr << "STOP called" << std::endl;
				close(fd_);
				fd_ = -1;
				return;
			case cmd::HEARTBEAT:
				std::cerr << "Received heartbeat" << std::endl;
				processHeartbeat();
				break;
			case cmd::GETATTR:
				std::cerr << "GETATTR called" << std::endl;
				processGetAttr();
				break;
			case cmd::READDIR:
				std::cerr << "READDIR called" << std::endl;
				processReaddir();
				break;
			case cmd::ACCESS:
				std::cerr << "ACCESS called" << std::endl;
				processAccess();
				break;
			case cmd::READ:
				std::cerr << "READ called" << std::endl;
				processRead();
				break;
			default:
				std::cerr << "Unknown command" << std::endl;
			}
		}
		//Do stuff here
	}
}

void ClientConnection::processHeartbeat() {
	agfs_write_cmd(fd_, cmd::HEARTBEAT);
}

/*
 * Incoming stack looks like:
 * 
 *      STRING
 *
 * Outgoing stack looks like:
 *
 *      ERROR [STAT]
 */
void ClientConnection::processGetAttr() {
	std::string path;
	agfs_read_string(fd_, path);
	boost::filesystem::path fusePath{path};
	boost::filesystem::path file{mountPoint_};
	file /= fusePath;

	struct stat retValue;
	memset(&retValue, 0, sizeof(struct stat));
	agerr_t error;
	if ((error = lstat(file.c_str(), &retValue)) < 0) {
		error = -errno;
	}

	agfs_write_error(fd_, error);
	if (error >= 0) {
		agfs_write_stat(fd_, retValue);
	}
}

/*
 * Incoming stack looks like:
 * 
 *      STRING MASK
 *
 * Outgoing stack looks like:
 *
 *      ERROR
 */
void ClientConnection::processAccess() {
	std::string path;
	agfs_read_string(fd_, path);

	//Cosntruct the filepath
	boost::filesystem::path fusePath{path};
	boost::filesystem::path file{mountPoint_};
	file /= fusePath;

	//Grab the access mask.
	agmask_t mask;
	read(fd_, &mask, sizeof(agmask_t));

	//Attempt to access the file.
	agerr_t result = access(file.c_str(), mask);

	//Write our result
	agfs_write_error(fd_, result);
}

/*
 * Incoming stack looks like:
 * 
 *      STRING
 *
 * Outgoing stack looks like:
 *
 *      ERROR [SIZE [STRING STAT]*]
 */
void ClientConnection::processReaddir() {
	std::string path;
	agfs_read_string(fd_, path);

	//Cosntruct the filepath
	boost::filesystem::path fusePath{path};
	boost::filesystem::path file{mountPoint_};
	file /= fusePath;

	agerr_t error = 0;
	if (!boost::filesystem::exists(file)) {
		error = -ENOENT;
	}

	if (!boost::filesystem::is_directory(file)) {
		error = -ENOTDIR;
	}
	agfs_write_error(fd_, error);

	//Short circuit if we have an error.
	if (error >= 0) {
		//Write the number of entries in the directory to the pipe.
		agsize_t count = 0;
		boost::filesystem::directory_iterator end_itr{};
		for (boost::filesystem::directory_iterator dir_itr(file);
			dir_itr != end_itr; dir_itr++) {
			count++;
		}
		count = htobe64(count);
		write(fd_, &count, sizeof(agsize_t));

		//Write the filenames and associated stat's to the pipe.
		struct stat stbuf;
		for (boost::filesystem::directory_iterator dir_itr(file);
			dir_itr != end_itr; dir_itr++) {
			
			//Write the relative name here
			agfs_write_string(fd_, dir_itr->path().filename().c_str());

			//We need to stat the absolute name here
			memset(&stbuf, 0, sizeof(struct stat));
			lstat(dir_itr->path().c_str(), &stbuf);
			agfs_write_stat(fd_, stbuf);
		}
	}
}

/*
 * Incoming stack looks like:
 * 
 *      STRING SIZE OFFSET
 *
 * Outgoing stack looks like:
 *
 *      ERROR [SIZE [DATA]*]
 */
void ClientConnection::processRead()
{
	std::string path;
	agfs_read_string(fd_, path);

	//Cosntruct the filepath
	boost::filesystem::path fusePath{path};
	boost::filesystem::path file{mountPoint_};
	file /= fusePath;

	agsize_t size = 0;
	agfs_read_size(fd_, size);

	agsize_t offset = 0;
	agfs_read_size(fd_, offset);

	//Process the request
	agerr_t error = 0;

	int fd;
	if ((fd = open(file.c_str(), O_RDONLY)) < 0)
	{
		error = -errno;
		agfs_write_error(fd_, error);
		return;
	}

	lseek(fd, offset, SEEK_SET);

	//Read the file data into the buffer until we have either hit an error,
	//or we have satisfied the read request.
	agsize_t total_read = 0;
	std::vector<unsigned char> buf;
	buf.resize(size);
	while (total_read != size && 
			(error = read(fd, &buf[0] + total_read, size - total_read)) > 0) {
		total_read += error;
	}
	//Write the total number of bytes sent to the error value,
	//unless there was a legitimate error.
	error = error >= 0 ? total_read : error;
	agfs_write_error(fd_, error);

	if (error >= 0) {
		agfs_write_size(fd_, total_read);
		write(fd_, &buf[0], total_read);
	}
}

void ClientConnection::processOpen()
{

}

void ClientConnection::processRelease()
{

}

