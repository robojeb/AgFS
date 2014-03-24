
#ifndef agfsio
#define agfsio

#include "constants.hpp"

int agfs_write_cmd(int fd, cmd_t cmd);
int agfs_read_cmd(int fd, cmd_t& cmd);
int agfs_write_stat(int fd, struct stat& buf);
int agfs_read_stat(int fd, struct stat& buf);

#endif
