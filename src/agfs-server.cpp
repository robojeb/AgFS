#include <iostream>
#include <fstream>
#include <unistd.h>
#include "agfs-server.hpp"
#include "constants.hpp"

void agfsServer(int connfd) {
	//Verify server key
	char key[ASCII_KEY_LEN];
	read(connfd, key, ASCII_KEY_LEN);
	//cmd_t resp = cmd::INVALID_KEY;
	//write(connfd, &resp, sizeof(cmd_t));
}
