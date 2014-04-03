
#ifndef agfsio
#define agfsio

#include "constants.hpp"

/**
 * Attempt to 
 */
int agfs_write_cmd(int fd, cmd_t cmd);
int agfs_read_cmd(int fd, cmd_t& cmd);

int agfs_write_error(int fd, agerr_t err);
int agfs_read_error(int fd, agerr_t& err);

int agfs_write_stat(int fd, struct stat& buf);
int agfs_read_stat(int fd, struct stat& buf);

int agfs_write_string(int fd, const char* str);
int agfs_read_string(int fd, std::string& str);

#endif
