// objectTracker.hpp
// Handles object position, velocity, etc over time
#pragma once
#include <unordered_map>
#include <vector>
#include <string>
#include <deque>
#include <memory>
#include <string>
#include <set>
#include "bw/armaTypes.hpp"
#include "armaEvents.hpp"
#include <string_view>

class dataServer;
class objectTracker {
    private:
        struct objectInfo {
            enum class lifeState {
                ALIVE,
                UNCONCIOUS,
                DEAD
            };

            struct state {
                double time = 0.0;

                float positionX = 0.f;
                float positionY = 0.f;
                float positionZ = 0.f;

                float azimuth = 0.f;
                float pitch = 0.f;

                lifeState lifeState = lifeState::ALIVE;
            };

            // This happens way less than position updates, so we don't want to store frivilous info
            struct occupantState {
                double time = 0.0;

                std::set<std::string> m_occupants;
                std::string m_occupantOf = "";
            };

            std::string m_classname = "";
            std::string m_uid = "";
            std::string m_realName = "";

            std::deque<state> m_states;
            std::deque<occupantState> m_occupationStates;
        };

        std::unordered_map<std::string, objectInfo> m_objects;

        dataServer &m_server;
        int m_eventID = 0;
        int m_updateID = 0;

        void created(eventData &event);
        void destroyed(eventData &event);
        void getIn(eventData &event);
        void getOut(eventData &event);

        void logEvent(const std::vector<std::unique_ptr<potato::baseARMAVariable>> &variables);
        void updateObject(const std::vector<std::unique_ptr<potato::baseARMAVariable>> &variables);

        void drawObjectState(const objectInfo::state &state) const;

    public:
        objectTracker(dataServer &server);
        ~objectTracker();
        void drawInfo() const;

        void serialise(struct zip_t *zip) const;
};
