// packetTypes.hpp
// A shared enum that allows any program to identify what a packet contains
// Keep in sync with addons/main/packetTypes.hpp
#pragma once
#include <cstdint>

namespace potato {
	enum class packetTypes : uint16_t {
		DEBUG_MESSAGE = 1,
		GAME_EVENT = 2,
		UPDATE_PROJECTILE = 3,
		UPDATE_OBJECT = 4,
		HEARTBEAT = 5,
		COUNT,
		NONE = 0,
	};
}
