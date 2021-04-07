// armaEvents.hpp
// Enum of all events that can be sent from ARMA
#pragma once
#include <string_view>

enum class armaEvents {
	NONE = 0,
	OBJECT_CREATED = 1,
	OBJECT_KILLED = 2,
};

namespace potato {
	inline std::string_view getEventString(armaEvents event) {
		switch (event) {
			case armaEvents::OBJECT_CREATED:
				return "Object Created";
				break;
			case armaEvents::OBJECT_KILLED:
				return "Object Created";
				break;
			default:
				return "unknown";
				break;
		}
		return "this should never occur";
	}
}
