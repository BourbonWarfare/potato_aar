// packetTypes.hpp
// A shared enum that allows any program to identify what a packet contains
#pragma once
#include <cstdint>

namespace potato {
	enum class packetTypes : uint16_t {
		DEBUG_MESSAGE = 1,
		GAME_EVENT = 2,
		COUNT,
		NONE = 0,
	};
}
