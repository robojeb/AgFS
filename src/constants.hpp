/**
 * @file constants.hpp
 */


#ifndef CONSTANTS_HPP_INC
#define CONSTANTS_HPP_INC

#include <cstdint>
#include <string>

/**
 * Commands issued by AgFS
 */
typedef uint16_t cmd_t;

typedef uint64_t agsize_t;

/**
 * Error codes returned by AgFS
 */
typedef int64_t agerr_t;

typedef uint64_t agdev_t;
typedef uint32_t agmode_t;
typedef uint32_t agmask_t;
typedef uint64_t agtime_t;

/// Provides the key length in bytes (hex encoded)
constexpr int KEY_LEN = 256;

/// Provides the key length in bytes (ASCII encoded)
constexpr size_t ASCII_KEY_LEN = 512;

constexpr int CLIENT_BLOCK_SEC = 1;
constexpr int CLIENT_BLOCK_USEC = 0;

constexpr int SERVER_BLOCK_SEC = 10;
constexpr int SERVER_BLOCK_USEC = 0;

// Default locations
const std::string KEY_LIST_PATH = "/var/lib/agfs/authkeys";
const std::string KEY_NAME_PATH = "/var/lib/agfs/";
const std::string KEY_EXTENSION = ".agkey";

/**
 * \brief Provides the AgFS commands and their definitions
 */
namespace cmd {
	/// Command to close a connection
	constexpr cmd_t STOP = 0;

	/// Routine command to check that a connection hasn't failed
	constexpr cmd_t HEARTBEAT = 1;

	/// Command to execute a GETATTR
	constexpr cmd_t GETATTR = 2;

	/// Command to accept a connection
	constexpr cmd_t ACCEPT = 3;

	/// TODO Shouldn't these 3 be errors?
	/// Command(?) to reply that a key is invalid
	constexpr cmd_t INVALID_KEY = 4;

	/// Command(?) to reply that the requested mount was not found
	constexpr cmd_t MOUNT_NOT_FOUND = 5;

	/// Command(?) to reply that the user a client tried to auth as was not found
	constexpr cmd_t USER_NOT_FOUND = 6;

	/// Command to execute an ACCESS
	constexpr cmd_t ACCESS = 7;

	/// Command to execute a READDIR
	constexpr cmd_t READDIR = 8;

	/// Command to execute a READ
	constexpr cmd_t READ = 9;

    /// Command for no response
    constexpr cmd_t NONE = 10;
}

#endif
