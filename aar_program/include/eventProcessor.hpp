// eventProcessor.hpp
// Reads events from ARMA and allows them to be output as JSON
#pragma once
#include <vector>
#include <deque>
#include <memory>
#include "armaEvents.hpp"
#include "bw/armaTypes.hpp"
#include "nlohmann/json_fwd.hpp"

class dataServer;
class eventProcessor {
    private:
        std::deque<eventData> m_eventQueue;

        void logEvent(const std::vector<std::unique_ptr<potato::baseARMAVariable>> &variables);

    public:
        eventProcessor(dataServer &server);
        void drawInfo() const;

        nlohmann::json serialise() const;
};
