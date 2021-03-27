// Brandon (TCVM)
// gets data from SQF and dumps it across network to external program
// in msvc this should be built in "release" otherwise you get buffer overflow errors
#include <cstdint>
#include <memory>
#include <exception>
#include <string>
#include <vector>
#include <charconv>
#include "connection.hpp"
#include "dataSender.hpp"
#include "bw/armaTypes.hpp"

using asio::ip::udp;


namespace globals {
    constexpr auto versionString = "v1.0.0";
    std::unique_ptr<dataSender> dataProcessor{};
}

namespace potato {
    bool isNumber(std::string_view string) {
        return !string.empty() && std::find_if(string.begin(), string.end(), [](unsigned char c){ return std::isdigit(c); }) != string.end();
    }

    void formatMessage(char *output, int outputSize, std::string_view message) {
        strncpy_s(output, outputSize, message.data(), _TRUNCATE);
    }

    void copyDataToBuffer(void *data, int dataSize, std::vector<std::uint8_t> &buffer) {
        for (int i = 0; i < dataSize; i++) {
            buffer.push_back(*(reinterpret_cast<std::uint8_t*>(data) + i));
        }
    }
}

// djb2 hash
constexpr std::uint64_t hash(const char *input) {
    std::uint64_t hash = 5381;
    std::uint64_t index = 0;
    while (input[index] != '\0') {
        hash *= (33 ^ (std::uint64_t)(input[index++])) + 1;
    }
    return hash;
}

extern "C" {
    __declspec(dllexport) void __stdcall RVExtension(char* output, int outputSize, const char* function);
    __declspec (dllexport) void __stdcall RVExtensionArgs(char *output, int outputSize, const char *function, const char **args, int argsCnt);
    __declspec(dllexport) void __stdcall RVExtensionVersion(char* output, int outputSize);
    __declspec (dllexport) void __stdcall RVExtensionRegisterCallback(int(*callbackProc)(char const* name, char const* function, char const* data));
}

void __stdcall RVExtensionRegisterCallback(int(*callbackProc)(char const *name, char const *function, char const *data)) {
    //callbackPtr = callbackProc;
}

void __stdcall RVExtensionVersion(char *output, int outputSize) {
    potato::formatMessage(output, outputSize, globals::versionString);
}

void __stdcall RVExtension(char *output, int outputSize, const char *function) {
    switch (hash(function)) {
        case hash("version"):
            RVExtensionVersion(output, outputSize);
            break;
        case hash("startup"):
            try {
                globals::dataProcessor = std::make_unique<dataSender>();
                potato::formatMessage(output, outputSize, "data sender started");
            } catch (const std::exception &e) {
                potato::formatMessage(output, outputSize, e.what());
            };
            break;
        case hash("shutdown"):
            globals::dataProcessor.reset();
            potato::formatMessage(output, outputSize, "data sender shutdown");
            break;
        default:
            potato::formatMessage(output, outputSize, ("Invalid Function: function [" + std::string(function) + "] hash [" + std::to_string(hash(function)) + "]").c_str());
            break;
    }
}

void __stdcall RVExtensionArgs(char *output, int outputSize, const char *function, const char **args, int argsCnt) {
    switch (hash(function)) {
        case hash("processData"):
            {
                if (!globals::dataProcessor) {
                    potato::formatMessage(output, outputSize, "Cannot process data when we haven't started the processor");
                    return;
                }

                if (!globals::dataProcessor->running()) {
                    potato::formatMessage(output, outputSize, "Data processor is not running. Aborting");
                    return;
                }

                // I reserved two arguments. First one denotes packet type, second one is unused as of 21/03/27
                if (argsCnt < 2) {
                    potato::formatMessage(output, outputSize, "Too few arguments");
                    return;
                }

                std::string arg0 = args[0];
                std::string arg1 = args[1];

                if (!potato::isNumber(arg0)) {
                    potato::formatMessage(output, outputSize, "Argument 0 has to be number");
                    return;
                }

                unsigned long typeInt;
                std::from_chars(arg0.data(), arg0.data() + arg0.size(), typeInt);
                potato::packetTypes packetType = static_cast<potato::packetTypes>(typeInt);
                
                std::vector<std::uint8_t> dataToSend;
                for (int i = 2; i < argsCnt; i++) {
                    potato::variableType argumentType = potato::getTypeFromString(args[i]);
                    std::unique_ptr<potato::baseARMAVariable> armaVariable = potato::getARMAVariableFromType(argumentType);
                    armaVariable->fromString(args[i]);

                    std::intptr_t sizeOfVariable = armaVariable->getDataPointerSize();

                    potato::copyDataToBuffer(&argumentType, sizeof(potato::variableType), dataToSend);
                    potato::copyDataToBuffer(&sizeOfVariable, sizeof(sizeOfVariable), dataToSend);
                    potato::copyDataToBuffer(armaVariable->getDataPointer(), armaVariable->getDataPointerSize(), dataToSend);
                }

                globals::dataProcessor->sendData(packetType, dataToSend.data(), dataToSend.size());
                potato::formatMessage(output, outputSize, "Data Sent with size " + std::to_string(dataToSend.size()) + " arg[2] = " + args[2]);
            }
            break;
        default:
            potato::formatMessage(output, outputSize, ("Invalid Function: function [" + std::string(function) + "] hash [" + std::to_string(hash(function)) + "]").c_str());
            break;
        }
}
