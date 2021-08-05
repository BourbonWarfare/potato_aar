#include "eventSubscriber.hpp"

void eventSubscriber::signal(potato::packetTypes packetType, variableType dataFromPacket) const {
    if (m_eventHandlers.find(packetType) == m_eventHandlers.end()) { return; }

    for (auto &subscriberEvent : m_eventHandlers.at(packetType)) {
        subscriberEvent(dataFromPacket);
    }
}

int eventSubscriber::subscribe(potato::packetTypes packetType, eventSignature function){
    int index = m_eventHandlers.size();
    m_eventHandlers[packetType].push_back(function);
    return index;
}

void eventSubscriber::unsubscribe(potato::packetTypes packetType, int id) {
    if (m_eventHandlers.find(packetType) == m_eventHandlers.end()) { return; }
    m_eventHandlers.at(packetType).erase(m_eventHandlers.at(packetType).begin() + id);
}
