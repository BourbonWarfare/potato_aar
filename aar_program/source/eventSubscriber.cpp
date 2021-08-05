#include "eventSubscriber.hpp"
#include "spdlog/spdlog.h"

void eventSubscriber::signal(potato::packetTypes packetType, variableType dataFromPacket) const {
    if (m_eventHandlers.find(packetType) == m_eventHandlers.end()) { return; }

    for (auto &subscriberEvent : m_eventHandlers.at(packetType)) {
        subscriberEvent.m_callback(dataFromPacket);
    }
}

int eventSubscriber::subscribe(potato::packetTypes packetType, eventSignature function){
    int uid = m_globalIDs++;
    m_eventHandlers[packetType].push_back(eventCallback{
        uid,
        function
    });
    return uid;
}

void eventSubscriber::unsubscribe(potato::packetTypes packetType, int id) {
    if (m_eventHandlers.find(packetType) == m_eventHandlers.end()) {
        spdlog::error("Could not find packet handler to delete {} {}", packetType, id);
        return;
    }
    std::vector<eventCallback> &callbacks = m_eventHandlers.at(packetType);

    callbacks.erase(std::remove_if(callbacks.begin(), callbacks.end(), [&id] (const eventCallback &callback) {
        return callback.m_uid == id;
    }), callbacks.end());
}
