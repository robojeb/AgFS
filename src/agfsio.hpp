
#ifndef agfsio
#define agfsio

/*
 * This file is meant to serve as a wrapper for networking between devices with
 * different endianness. The IO methods present in this file and accompanying
 * cpp file will all handle endianness appropriately. In addition, after having
 * written data to a socket the corresponding read method can be called on the
 * other end and the exact same data will come out (assuming there are no IO 
 * errors with the pipe).
 */

#include "constants.hpp"

int agfs_write_cmd(int fd, cmd_t cmd);
int agfs_read_cmd(int fd, cmd_t& cmd);

int agfs_write_mask(int fd, agmask_t mask);
int agfs_read_mask(int fd, agmask_t& mask);

int agfs_write_error(int fd, agerr_t err);
int agfs_read_error(int fd, agerr_t& err);

int agfs_write_stat(int fd, struct stat& buf);
int agfs_read_stat(int fd, struct stat& buf);

int agfs_write_string(int fd, const char* str);
int agfs_read_string(int fd, std::string& str);

#endif
