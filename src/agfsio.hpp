
#ifndef agfsio
#define agfsio

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
