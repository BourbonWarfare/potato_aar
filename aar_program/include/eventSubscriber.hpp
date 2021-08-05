// eventSubscriber.hpp
// Allows users to subscribe to network events
#pragma once
#include <unordered_map>
#include <functional>
#include <vector>
#include "bw/packetTypes.hpp"

namespace potato {
    struct baseARMAVariable;
}

class eventSubscriber {
    private:
        using variableType = const std::vector<std::unique_ptr<potato::baseARMAVariable>>&;
        using eventSignature = std::function<void(variableType)>;

        std::unordered_map<potato::packetTypes, std::vector<eventSignature>> m_eventHandlers;

    protected:
        // Signals event. Runs through m_eventHandlers and calls each function
        void signal(potato::packetTypes packetType, variableType dataFromPacket) const;

    public:
        // Subscribe a function to be called when we receive a packet of type
        int subscribe(potato::packetTypes packetType, eventSignature function);

        void unsubscribe(potato::packetTypes packetType, int id);
};
