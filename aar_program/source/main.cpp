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

    variables[0]->convert(event.type);
    variables[1]->convert(event.eventTime);

    spdlog::info("Event: {} Time: {}", potato::getEventString(event.type), event.eventTime);
}

int main() {
    dataServer server;
    server.subscribe(potato::packetTypes::DEBUG_MESSAGE, logPacket);
    server.subscribe(potato::packetTypes::GAME_EVENT, logEvent);

    while (true) {
        
    }

    return 0;
}
