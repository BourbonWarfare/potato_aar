#include "spdlog/spdlog.h"
#include "dataSender.hpp"
#include <string>

int main() {
    dataSender sender;

    std::string message = "1234";
    sender.sendData(potato::packetTypes::NONE, message.data(), 3);

    while(true) {}

    return 0;
}
