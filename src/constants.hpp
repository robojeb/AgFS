#ifndef CONSTANTS_HPP_INC
#define CONSTANTS_HPP_INC

#include <cstdint>
#include <string>

typedef uint16_t cmd_t;
typedef uint64_t agsize_t;
typedef int64_t agerr_t;
typedef uint64_t agdev_t;
typedef uint32_t agmode_t;
typedef uint32_t agmask_t;
typedef uint64_t agtime_t;

constexpr int KEY_LEN = 256;
constexpr size_t ASCII_KEY_LEN = 512;

constexpr int CLIENT_BLOCK_SEC = 1;
constexpr int CLIENT_BLOCK_USEC = 0;

constexpr int SERVER_BLOCK_SEC = 10;
constexpr int SERVER_BLOCK_USEC = 0;

const std::string KEY_LIST_PATH = "/var/lib/agfs/authkeys";
const std::string KEY_NAME_PATH = "/var/lib/agfs/";
const std::string KEY_EXTENSION = ".agkey";

namespace cmd {
	constexpr cmd_t STOP = 0;
	constexpr cmd_t HEARTBEAT = 1;
	constexpr cmd_t GETATTR = 2;
	constexpr cmd_t ACCEPT = 3;
	constexpr cmd_t INVALID_KEY = 4;
	constexpr cmd_t MOUNT_NOT_FOUND = 5;
	constexpr cmd_t USER_NOT_FOUND = 6;
	constexpr cmd_t ACCESS = 7;
	constexpr cmd_t READDIR = 8;
}

#endif