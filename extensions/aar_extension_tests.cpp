#include "spdlog/spdlog.h"
#include "dataSender.hpp"
#include "bw/armaTypes.hpp"
#include <string>
#include <vector>
#include <memory>

void copyDataToBuffer(void *data, int dataSize, std::vector<std::uint8_t> &buffer) {
    for (int i = 0; i < dataSize; i++) {
        buffer.push_back(*(reinterpret_cast<std::uint8_t*>(data) + i));
    }
}

int main() {
    dataSender sender;

    std::vector<std::unique_ptr<potato::baseARMAVariable>> args;
    args.push_back(std::make_unique<potato::armaVariable<potato::variableType::NUMBER>>());
    double data = 123.0;
    args.back()->set(&data, potato::variableType::NUMBER);

    args.push_back(std::make_unique<potato::armaVariable<potato::variableType::NUMBER>>());
    data = 456.0;
    args.back()->set(&data, potato::variableType::NUMBER);

    std::vector<std::uint8_t> dataToSend;
    for (int i = 0; i < args.size(); i++) {
        potato::variableType argumentType = potato::getTypeFromString(args[i]->toString());
        // allow us to re-parse the data since we don't know the size

        double num;
        args[i]->convert(num);

        std::intptr_t sizeOfVariable = args[i]->getDataPointerSize();

        copyDataToBuffer(&argumentType, sizeof(potato::variableType), dataToSend);
        copyDataToBuffer(&sizeOfVariable, sizeof(sizeOfVariable), dataToSend);
        copyDataToBuffer(&num, sizeof(num), dataToSend);
    }

    //sender.sendData(potato::packetTypes::DEBUG_MESSAGE, dataToSend.data(), dataToSend.size());
    //sender.sendData(potato::packetTypes::DEBUG_MESSAGE, dataToSend.data(), dataToSend.size());
    //sender.sendData(potato::packetTypes::DEBUG_MESSAGE, dataToSend.data(), dataToSend.size());

    std::unique_ptr<potato::baseARMAVariable> armaArray = std::make_unique<potato::armaArray>();
    armaArray->fromString("[\"first\",1,2,3,4,5,6,7,8,9,0,true,false,[1,2,3,\"test\"]]");

    std::unique_ptr<potato::baseARMAVariable> armaArray2 = std::make_unique<potato::armaArray>();
    armaArray2->fromString(armaArray->toString());

    spdlog::info("array: {}", armaArray->toString());
    spdlog::info("array: {}", armaArray2->toString());

    while(true) {}

    return 0;
}
