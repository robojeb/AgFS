#ifndef CONSTANTS_HPP_INC
#define CONSTANTS_HPP_INC

#include <cstdint>

typedef uint16_t cmd_t;
typedef uint64_t agsize_t;
typedef int64_t agerr_t;

constexpr size_t ASCII_KEY_LEN = 512;

constexpr int CLIENT_BLOCK_SEC = 1;
constexpr int CLIENT_BLOCK_USEC = 0;

namespace cmd {
	constexpr cmd_t STOP = 0;
	constexpr cmd_t HEARTBEAT = 1;
	constexpr cmd_t GETATTR = 2;
	constexpr cmd_t ACCEPT = 3;
	constexpr cmd_t INVALID_KEY = 4;
	constexpr cmd_t MOUNT_NOT_FOUND = 5;
	constexpr cmd_t USER_NOT_FOUND = 6;
}

#endif