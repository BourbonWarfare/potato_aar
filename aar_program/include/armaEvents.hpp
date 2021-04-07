// armaEvents.hpp
// A struct that represents an event that occured in game
#pragma once
#include <string_view>
#include <vector>
#include <memory>
#include "bw/armaTypes.hpp"

enum class armaEvents {
	NONE = 0,
	OBJECT_CREATED = 1,
	OBJECT_KILLED = 2,
};

struct eventData {
	armaEvents type = armaEvents::NONE;
	double eventTime = 0.0;
	std::vector<std::unique_ptr<potato::baseARMAVariable>> eventInformation;
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
