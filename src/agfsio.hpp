
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

/**
 * \brief Write a command on the connection.
 * \details Handles endianness and writes a command to a provided connection.
 * \param fd The file descriptor of the connection to write on
 * \param cmd The command to write
 */
int agfs_write_cmd(int fd, cmd_t cmd);

/**
* \brief Read a command on the connection.
* \details Handles endianness and read a command from a provided connection.
* \param fd The file descriptor of the connection to read on
* \param cmd The command buffer to read into.
*/
int agfs_read_cmd(int fd, cmd_t& cmd);

/**
 * \brief Write a mask on the connection.
 * \details Handles endianness and writes a mask to a provided connection.
 * \param fd The file descriptor of the connection to write on
 * \param mask The mask to write
 */
int agfs_write_mask(int fd, agmask_t mask);

/**
* \brief Read a mask on the connection.
* \details Handles endianness and read a mask from a provided connection.
* \param fd The file descriptor of the connection to read on
* \param mask The mask buffer to read into.
*/
int agfs_read_mask(int fd, agmask_t& mask);

/**
 * \brief Write a size to the connection.
 * \details Handles endianness and writes a size to a provided connection.
 * \param fd The file descriptor of the connection to write on
 * \param size The size to write
 */
int agfs_write_size(int fd, agsize_t size);

/**
* \brief Read a size from the connection.
* \details Handles endianness and read a size from a provided connection.
* \param fd The file descriptor of the connection to read on
* \param size The size buffer to read into.
*/
int agfs_read_size(int fd, agsize_t& size);

/**
* \brief Write an error on the connection.
* \details Handles endianness and writes an error to a provided connection.
* \param fd The file descriptor of the connection to write on
* \param cmd The error to write
*/
int agfs_write_error(int fd, agerr_t err);

/**
* \brief Read an error on the connection.
* \details Handles endianness and reads an error from a provided connection.
* \param fd The file descriptor of the connection to read on
* \param cmd The error buffer to read into
*/
int agfs_read_error(int fd, agerr_t& err);

/**
* \brief Write a string on the connection.
* \details Handles endianness and writes a string to a provided connection.
* \param fd The file descriptor of the connection to write on
* \param cmd The string to write
*/
int agfs_write_stat(int fd, struct stat& buf);

/**
* \brief Read a string on the connection.
* \details Handles endianness and reads a string to a provided connection.
* \param fd The file descriptor of the connection to read on
* \param cmd The string buffer to read into
*/
int agfs_read_stat(int fd, struct stat& buf);

/**
* \brief Write a stat struct on the connection.
* \details Handles endianness and writes a stat struct to a provided connection.
* \param fd The file descriptor of the connection to write on
* \param cmd The stat struct to write
*/
int agfs_write_string(int fd, const char* str);

/**
* \brief Read a stat struct on the connection.
* \details Handles endianness and reads a stat struct to a provided connection.
* \param fd The file descriptor of the connection to read on
* \param cmd The stat struct buffer to read into
*/
int agfs_read_string(int fd, std::string& str);

#endif
