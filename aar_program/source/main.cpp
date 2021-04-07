#include "bw/armaTypes.hpp"
#include "dataServer.hpp"
#include "spdlog/spdlog.h"

void logPacket(const std::vector<std::unique_ptr<potato::baseARMAVariable>> &variables) {
    spdlog::info("debug message");
    spdlog::info("start");
    for (auto &variable : variables) {
        spdlog::info("\t{}: {}", potato::getTypeString(variable->type), variable->toString());
    }
    spdlog::info("end");
}

int main() {
    dataServer server;
    server.subscribe(potato::packetTypes::DEBUG_MESSAGE, logPacket);

    while (true) {
    }

    return 0;
}
