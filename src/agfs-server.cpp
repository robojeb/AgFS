#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <cstring>
#include <pwd.h>
#include <sys/stat.h>
#include <arpa/inet.h>
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
		cmd_t cmd;
		int respVal = agfs_read_cmd(fd_, cmd);
		if(respVal == -1) {
			std::cout << "Missed heartbeat" << std::endl;
		} else {
			switch(cmd) {
			case cmd::STOP:
				close(fd_);
				fd_ = -1;
				return;
			case cmd::HEARTBEAT:
				//respond to a heartbeat
				agfs_write_cmd(fd_, cmd::HEARTBEAT);
				break;
			case cmd::GETATTR:
				processGetAttr();
				break;
			default:
				std::cerr << "Unknown command" << std::endl;
			}
		}
		//Do stuff here
	}
}


/*
 * Incoming stack looks like:
 * 
 *      PATHLEN PATH
 *
 * Outgoing stack looks like:
 *
 *      ERROR STAT
 */
void ClientConnection::processGetAttr() {
	agsize_t pathLen;
	read(fd_, &pathLen, sizeof(agsize_t));
	
	char* path = (char*) malloc(sizeof(char) * pathLen + 1);
	memset(path, 0, pathLen + 1);

	read(fd_, path, pathLen);
	boost::filesystem::path fusePath{path};
	boost::filesystem::path file{mountPoint_};
	file /= fusePath;

	struct stat retValue;
	memset(&retValue, 0, sizeof(struct stat));
	agerr_t error;
	if ((error = lstat(file.c_str(), &retValue)) < 0) {
		error = errno;
	}

	fprintf(stderr, "Got stat call!\n");
	fprintf(stderr, "Error %lu\n", error);
	write(fd_, &error, sizeof(agerr_t));
	write(fd_, &retValue, sizeof(struct stat));
}


