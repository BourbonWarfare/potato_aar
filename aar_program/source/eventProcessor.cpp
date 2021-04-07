#include "eventProcessor.hpp"
#include "dataServer.hpp"
#include "bw/packetTypes.hpp"
#include "spdlog/spdlog.h"
#include <functional>

void eventProcessor::readPacket(const std::vector<std::unique_ptr<potato::baseARMAVariable>> &variables) {
    spdlog::info("Event: {}", potato::getEventString(armaEvents::NONE));
}

eventProcessor::eventProcessor(dataServer &server) {
    server.subscribe(potato::packetTypes::GAME_EVENT, std::bind(this, &eventProcessor::readPacket, std::placeholders::_1));
}
