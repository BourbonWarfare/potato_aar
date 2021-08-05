// missionHandler.hpp
// Handles current mission state. For each mission we read data, and once the mission ends we dump it to disk
#pragma once
#include <unordered_map>
#include <vector>
#include <deque>
#include <memory>
#include <string>
#include <thread>
#include "bw/armaTypes.hpp"

#include "eventProcessor.hpp"
#include "projectileTracker.hpp"
#include "objectTracker.hpp"

class dataServer;
class missionHandler {
    private:
        dataServer &m_server;
        int m_eventHandlerHandle = -1;

        std::string m_missionDate = "";
        double m_missionEnd = 0.0;

        double m_projectileUpdateRate = 0.0;
        double m_objectUpdateRate = 0.0;

        std::string m_missionName = "";
        std::string m_worldName = "";

        eventProcessor m_eventHandler;
        projectileTracker m_projectileHandler;
        objectTracker m_objectHandler;

        std::thread m_dumpThread;
        bool m_readyToDump = false;
        bool m_readyToDelete = false;

        void dumpToDisk();

        void onStart(eventData &event);
        void onEnd(eventData &event);

        void logEvent(const std::vector<std::unique_ptr<potato::baseARMAVariable>> &variables);

    public:
        missionHandler(dataServer &server, eventData &startEvent);
        ~missionHandler();

        void drawInfo() const;
        void update();

        void dump();
        bool readyToDump() const;
        bool readyToDelete() const;

        std::string getMissionName() const;

};
