// eventProcessor.hpp
// Reads events from ARMA and allows them to be output as JSON
#pragma once
#include <vector>
#include <queue>
#include <memory>
#include "armaEvents.hpp"
#include "bw/armaTypes.hpp"

class dataServer;
class eventProcessor
    {
        private:
            //std::queue<eventData> m_eventQueue;

        public:
            eventProcessor(dataServer &server);
            void logEvent(const std::vector<std::unique_ptr<potato::baseARMAVariable>> &variables);
    };
