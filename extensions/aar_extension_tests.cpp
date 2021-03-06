// shitty test file
// should split these into unit-tests, but thats a lot of work for me at the moment
#include "spdlog/spdlog.h"
#include "dataSender.hpp"
#include "bw/armaTypes.hpp"
#include "bw/packetHeader.hpp"
#include <string>
#include <vector>
#include <memory>

void copyDataToBuffer(void *data, int dataSize, std::vector<std::uint8_t> &buffer) {
    for (int i = 0; i < dataSize; i++) {
        buffer.push_back(*(reinterpret_cast<std::uint8_t*>(data) + i));
    }
}

void testSending(dataSender &sender) {
    std::vector<std::unique_ptr<potato::baseARMAVariable>> args;
    args.push_back(std::make_unique<potato::armaVariable<potato::variableType::NUMBER>>());
    double data = 123.0;
    args.back()->set(&data, potato::variableType::NUMBER, sizeof(data));

    args.push_back(std::make_unique<potato::armaVariable<potato::variableType::NUMBER>>());
    data = 456.0;
    args.back()->set(&data, potato::variableType::NUMBER, sizeof(data));

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

    sender.sendData(potato::packetTypes::DEBUG_MESSAGE, dataToSend.data(), dataToSend.size());
    sender.sendData(potato::packetTypes::DEBUG_MESSAGE, dataToSend.data(), dataToSend.size());
    sender.sendData(potato::packetTypes::DEBUG_MESSAGE, dataToSend.data(), dataToSend.size());
}

void testParsing() {
    std::unique_ptr<potato::baseARMAVariable> simpleFrom = std::make_unique<potato::armaArray>();
    simpleFrom->fromString("[1, true, false, \"foo\", [1, [2.5, 3.3], 4, 5], [], false]");

    spdlog::info("array: {}\n", simpleFrom->toString());

    std::unique_ptr<potato::baseARMAVariable> complexNested = std::make_unique<potato::armaArray>();
    complexNested->fromString("[[\"baz\"]]");

    spdlog::info("array: {}\n", complexNested->toString());

    std::unique_ptr<potato::baseARMAVariable> armaArray = std::make_unique<potato::armaArray>();
    armaArray->fromString("[\"a\", 1, true, false, [1, \"b\", false], 2, [\"foo\"], \"bar\", true, [1, 2, [3, 4, [[5, 6], [7, 8]]]],3]");
    void *dataPtr = armaArray->getDataPointer();

    std::unique_ptr<potato::baseARMAVariable> armaArray2 = std::make_unique<potato::armaArray>();
    armaArray2->fromString(armaArray->toString());

    std::unique_ptr<potato::baseARMAVariable> armaArray3 = std::make_unique<potato::armaArray>();
    armaArray3->set(dataPtr, potato::variableType::ARRAY, armaArray->getDataPointerSize());

    spdlog::info("array: {}", armaArray->toString());
    spdlog::info("array: {}", armaArray2->toString());
    spdlog::info("array: {}", armaArray3->toString());
}

void testSplitting(dataSender &sender) {
    constexpr int limit = 1 + (2 * potato::packetHeader::c_maxPacketSize / sizeof(double));
    std::vector<std::uint8_t> dataToSend;
    for (unsigned int i = 0; i < limit; i++) {
        potato::variableType argumentType = potato::variableType::NUMBER;
        std::intptr_t sizeOfVariable = sizeof(double);

        double num = i;

        copyDataToBuffer(&argumentType, sizeof(potato::variableType), dataToSend);
        copyDataToBuffer(&sizeOfVariable, sizeof(sizeOfVariable), dataToSend);
        copyDataToBuffer(&num, sizeof(num), dataToSend);
    }
    sender.sendData(potato::packetTypes::DEBUG_MESSAGE, dataToSend.data(), dataToSend.size());
    spdlog::info("Sending {} variables", limit);
}

int main() {
    dataSender sender;

    testSending(sender);
    testParsing();
    testSplitting(sender);

    

    while(true) {}

    return 0;
}
