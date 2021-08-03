#include "eventProcessor.hpp"
#include "dataServer.hpp"
#include "bw/packetTypes.hpp"
#include "bw/armaTypes.hpp"
#include "spdlog/spdlog.h"
#include <functional>

void eventProcessor::readPacket(const std::vector<std::unique_ptr<potato::baseARMAVariable>> &variables) {
    eventData data;

    double number = 0.0;
    variables[0]->convert(number);
    data.type = static_cast<armaEvents>(static_cast<int>(number));
    variables[1]->convert(data.eventTime);

    auto uniqueArray = potato::copyARMAVariable(variables[2].get());
    potato::armaArray *array = static_cast<potato::armaArray*>(uniqueArray.get());

    data.eventInformation = std::move(array->data);

    spdlog::info("Event: {}", potato::getEventString(data.type));

    m_events.push(std::move(data));
}

eventProcessor::eventProcessor(dataServer &server) {
    server.subscribe(potato::packetTypes::GAME_EVENT, std::bind(&eventProcessor::readPacket, this, std::placeholders::_1));
}

void eventProcessor::outputData(std::ostream &out) {
    potato::armaArray outArray;

    while (!m_events.empty()) {
        eventData &event = m_events.front();

        outArray.data.emplace_back(std::make_unique<potato::armaNumber>());
        outArray.data.back()->set(&event.type, potato::variableType::NUMBER, sizeof(event));
        
        m_events.pop();
    }

    out << outArray.toString();
}
