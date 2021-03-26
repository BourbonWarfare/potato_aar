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
        void signal(potato::packetTypes packetType, variableType dataFromPacket) const;

    public:
        void subscribe(potato::packetTypes packetType, eventSignature function);
};
