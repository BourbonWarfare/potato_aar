// armaEvents.hpp
// A struct that represents an event that occured in game
#pragma once
#include <string_view>
#include <vector>
#include <memory>
#include "bw/armaTypes.hpp"

enum class armaEvents {
	NONE                = 0,
	OBJECT_CREATED      = 1,
	OBJECT_KILLED       = 2,
    FIRED               = 3,
    MARKER_CREATED      = 4,
    MARKER_DESTROYED    = 5,
    MARKER_UPDATED      = 6,
    OBJECT_GET_IN       = 7,
    OBJECT_GET_OUT      = 8,
    CUSTOM              = 9,
    MISSION_LOAD        = 10,
    MISSION_END         = 11,
};

struct eventData {
	armaEvents type = armaEvents::NONE;
	double eventTime = 0.0;
	std::vector<std::unique_ptr<potato::baseARMAVariable>> eventInformation;

    eventData() = default;
    eventData(const eventData &rhs) {
        *this = rhs;
    }

    eventData(const std::vector<std::unique_ptr<potato::baseARMAVariable>> &variables) {
        fromPacket(variables);
    }

    eventData &operator=(const eventData &rhs) {
        if (&rhs != this) {
            this->type = rhs.type;
            this->eventTime = rhs.eventTime;
            for (auto &variable : rhs.eventInformation) {
                this->eventInformation.push_back(std::move(variable->copy()));
            }
        }

        return *this;
    }

    void fromPacket(const std::vector<std::unique_ptr<potato::baseARMAVariable>> &variables) {
        potato::armaArray &eventMetaInfo = *static_cast<potato::armaArray *>(variables[0].get());

        double eventNumber = 0;
        eventMetaInfo.data[0]->convert(eventNumber);
        eventMetaInfo.data[1]->convert(eventTime);

        type = static_cast<armaEvents>(eventNumber);

        potato::armaArray &eventInfo = *static_cast<potato::armaArray *>(eventMetaInfo.data[2].get());

        for (auto &variable : eventInfo.data) {
            eventInformation.push_back(variable->copy());
        }
    }
};

namespace potato {
	inline std::string_view getEventString(armaEvents event) {
		switch (event) {
            case armaEvents::NONE:
                return "None";
			case armaEvents::OBJECT_CREATED:
				return "Object Created";
			case armaEvents::OBJECT_KILLED:
				return "Object Killed";
            case armaEvents::FIRED:
                return "Fired";
            case armaEvents::MARKER_CREATED:
                return "Marker Created";
            case armaEvents::MARKER_DESTROYED:
                return "Marker Destroyed";
            case armaEvents::MARKER_UPDATED:
                return "Marker Updated";
            case armaEvents::OBJECT_GET_IN:
                return "Object Get In";
            case armaEvents::OBJECT_GET_OUT:
                return "Object Get Out";
			default:
				return "unknown";
		}
		return "this should never occur";
	}
}
