// serverObserver.hpp
// Handles server state. Observes server and allows the viewing of active missions. Creates new missions when able
#pragma once
#include <vector>
#include <memory>
#include <mutex>
#include "bw/armaTypes.hpp"
#include "missionHandler.hpp"

class dataServer;
class serverObserver {
    private:
        dataServer &m_server;

        std::mutex m_activeMissionsMutex;
        std::vector<std::unique_ptr<missionHandler>> m_activeMissions;

        void startMission(eventData &startEvent);

        void logEvent(const std::vector<std::unique_ptr<potato::baseARMAVariable>> &variables);

    public:
        serverObserver(dataServer &server);
        void update();
        void drawInfo(float appWidth, float appHeight);

};
