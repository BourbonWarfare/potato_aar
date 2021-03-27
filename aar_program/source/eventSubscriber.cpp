#include "eventSubscriber.hpp"

void eventSubscriber::signal(potato::packetTypes packetType, variableType dataFromPacket) const {
    if (m_eventHandlers.find(packetType) == m_eventHandlers.end()) { return; }

    for (auto &subscriberEvent : m_eventHandlers.at(packetType)) {
        subscriberEvent(dataFromPacket);
    }
}

void eventSubscriber::subscribe(potato::packetTypes packetType, eventSignature function){
    m_eventHandlers[packetType].push_back(function);
}
