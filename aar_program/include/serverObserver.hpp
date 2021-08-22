// serverObserver.hpp
// Handles server state. Observes server and allows the viewing of active missions. Creates new missions when able
#pragma once
#include <vector>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include "bw/armaTypes.hpp"
#include "missionHandler.hpp"
#include "clock.hpp"
#include "time.hpp"

class dataServer;
class serverObserver {
    private:
        const nlohmann::json c_config;

        const unsigned int c_heartbeatTimeouts = 5;
        fe::time m_heartbeatRate;
        fe::clock m_heartbeatClock;
        bool m_inMission = false;

        dataServer &m_server;

        std::mutex m_activeMissionsMutex;
        std::vector<std::unique_ptr<missionHandler>> m_activeMissions;

        void startMission(eventData &startEvent);

        void logEvent(const std::vector<std::unique_ptr<potato::baseARMAVariable>> &variables);
        void heartbeat(const std::vector<std::unique_ptr<potato::baseARMAVariable>> &variables);

    public:
        serverObserver(dataServer &server);
        void update();
        void drawInfo(float appWidth, float appHeight);

};
