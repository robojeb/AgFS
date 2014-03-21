#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <cstring>
#include <pwd.h>
#include "agfs-server.hpp"
#include "constants.hpp"
#include <boost/filesystem.hpp>

void agfsServer(int connfd) {

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
		cmd_t resp = cmd::INVALID_KEY;
		write(connfd, &resp, sizeof(cmd_t));
		close(connfd);
		return;
	}

	//Change user
	struct passwd* userPwd = getpwnam(user.c_str());
	if(userPwd == NULL) {
		cmd_t resp = cmd::USER_NOT_FOUND;
		write(connfd, &resp, sizeof(cmd_t));
		close(connfd);
		return;
	} else {
		setuid(userPwd->pw_uid);
		setgid(userPwd->pw_gid);
	}

	//Check that mount point exists
	if(!boost::filesystem::exists(mountPath)) {
		cmd_t resp = cmd::MOUNT_NOT_FOUND;
		write(connfd, &resp, sizeof(cmd_t));
		close(connfd);
		return;
	}


	//Passed all checks accept connection and keep alive until heartbeat
	//failure or explicit stop
	cmd_t resp = cmd::ACCEPT;
	write(connfd, &resp, sizeof(cmd_t));
	while(1) {
		cmd_t cmd;
		int respVal = read(connfd, &cmd, sizeof(cmd_t));
		if(respVal == -1) {
			std::cout << "Missed heartbeat" << std::endl;
		} else {
			switch(cmd) {
			case cmd::STOP:
				close(connfd);
				return;
			case cmd::HEARTBEAT:
				//respond to a heartbeat
				cmd = cmd::HEARTBEAT;
				write(connfd, &cmd, sizeof(cmd_t));
				break;
			case cmd::GETATTR:
				break;
			default:
				std::cerr << "Unknown command" << std::endl;
			}
		}
		//Do stuff here
	}
}
