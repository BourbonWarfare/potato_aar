// armaEvents.hpp
// A struct that represents an event that occured in game
#pragma once
#include <string_view>
#include <vector>
#include <memory>
#include "bw/armaTypes.hpp"

enum class armaEvents {
	NONE = 0,
	OBJECT_CREATED      = 1,
	OBJECT_KILLED       = 2,
    FIRED               = 3,
    MARKER_CREATED      = 4,
    MARKER_DESTROYED    = 5,
    MARKER_UPDATED      = 6,
};

struct eventData {
	armaEvents type = armaEvents::NONE;
	double eventTime = 0.0;
	std::vector<std::unique_ptr<potato::baseARMAVariable>> eventInformation;
};

namespace potato {
	inline std::string_view getEventString(armaEvents event) {
		switch (event) {
            case armaEvents::NONE:
                return "None";
			case armaEvents::OBJECT_CREATED:
				return "Object Created";
			case armaEvents::OBJECT_KILLED:
				return "Object Created";
            case armaEvents::FIRED:
                return "Fired";
            case armaEvents::MARKER_CREATED:
                return "Marker Created";
            case armaEvents::MARKER_DESTROYED:
                return "Marker Destroyed";
            case armaEvents::MARKER_UPDATED:
                return "Marker Updated";
			default:
				return "unknown";
		}
		return "this should never occur";
	}
}
