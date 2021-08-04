// objectTracker.hpp
// Handles object position, velocity, etc over time
#pragma once
#include <unordered_map>
#include <vector>
#include <string>
#include <deque>
#include <memory>
#include <string>
#include "bw/armaTypes.hpp"
#include "armaEvents.hpp"
#include "object.hpp"

class dataServer;
class objectTracker {
    private:
        std::unordered_map<std::string, object> m_objects;

        void created(eventData &event);
        void destroyed(eventData &event);
        void getIn(eventData &event);
        void getOut(eventData &event);

        void logEvent(const std::vector<std::unique_ptr<potato::baseARMAVariable>> &variables);
        void updateObject(const std::vector<std::unique_ptr<potato::baseARMAVariable>> &variables);

    public:
        objectTracker(dataServer &server);
        void drawInfo() const;
};
