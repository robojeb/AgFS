#ifndef CONSTANTS_HPP_INC
#define CONSTANTS_HPP_INC

#include <cstdint>

typedef uint16_t cmd_t;
typedef uint64_t agsize_t;
typedef int64_t agerr_t;

namespace cmd {
	constexpr cmd_t STOP = 0;
	constexpr cmd_t HEARTBEAT = 1;
	constexpr cmd_t GETATTR = 2;
}

#endif