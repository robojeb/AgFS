#ifndef CONSTANTS_HPP_INC
#define CONSTANTS_HPP_INC

#include <cstdint>

typedef cmd_t uint16_t;
typedef agsize_t uint64_t;
typedef agerr_t int64_t;

namespace cmd {
	constexpr cmd_t STOP = 0;
	constexpr cmd_t HEARTBEAT = 1;
	constexpr cmd_t GETATTR = 2;
}

#endif