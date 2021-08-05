#include "eventSubscriber.hpp"
#include "spdlog/spdlog.h"

void eventSubscriber::signal(potato::packetTypes packetType, variableType dataFromPacket) {
    if (m_eventHandlers.find(packetType) == m_eventHandlers.end()) { return; }

    std::vector<eventCallback> callbacks = m_eventHandlers.at(packetType);
    for (auto &callback : callbacks) {
        if (callback.m_active) {
            callback.m_callback(dataFromPacket);
        }
    }
}

int eventSubscriber::subscribe(potato::packetTypes packetType, eventSignature function) {
    int uid = m_globalIDs++;
    m_eventHandlers[packetType].push_back(eventCallback{
        uid,
        true,
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

    for (auto &callback : callbacks) {
        if (callback.m_uid == id) {
            callback.m_callback = [] (const auto &args) {};
            callback.m_active = false;
            break;
        }
    }
}
