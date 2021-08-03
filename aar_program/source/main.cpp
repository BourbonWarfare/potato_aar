#include "bw/armaTypes.hpp"
#include "dataServer.hpp"
#include "spdlog/spdlog.h"

#include "armaEvents.hpp"

void logPacket(const std::vector<std::unique_ptr<potato::baseARMAVariable>> &variables) {
    spdlog::info("debug message");
    spdlog::info("start");
    for (auto &variable : variables) {
        spdlog::info("\t{}: {}", potato::getTypeString(variable->type), variable->toString());
    }
    spdlog::info("end");
}

void logEvent(const std::vector<std::unique_ptr<potato::baseARMAVariable>> &variables) {
    spdlog::info("game event");

    eventData event;

    potato::armaArray &eventMetaInfo = *static_cast<potato::armaArray*>(variables[0].get());

    double eventNumber = 0;
    eventMetaInfo.data[0]->convert(eventNumber);
    eventMetaInfo.data[1]->convert(event.eventTime);

    event.type = static_cast<armaEvents>(eventNumber);

    potato::armaArray &eventInfo = *static_cast<potato::armaArray*>(eventMetaInfo.data[2].get());

    for (auto &variable : eventInfo.data)
        {
            event.eventInformation.push_back(variable->copy());
        }

    spdlog::info("Event: {} Time: {}", potato::getEventString(event.type), event.eventTime);
    for (auto &variable : event.eventInformation) {
        spdlog::info("\t{}: {}", potato::getTypeString(variable->type), variable->toString());
    }
    spdlog::info("End Event");
}

int main() {
    dataServer server;
    server.subscribe(potato::packetTypes::DEBUG_MESSAGE, logPacket);
    server.subscribe(potato::packetTypes::GAME_EVENT, logEvent);

    while (true) {
        
    }

    return 0;
}
