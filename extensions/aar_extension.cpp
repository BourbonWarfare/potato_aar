// Brandon (TCVM)
// gets data from SQF and dumps it across network to external program
// in msvc this should be built in "release" otherwise you get buffer overflow errors
#include <cstdint>
#include <memory>
#include <exception>
#include <string>
#include "connection.hpp"
#include "dataSender.hpp"

using asio::ip::udp;


namespace globals {
    constexpr auto versionString = "v1.0.0";
    std::unique_ptr<dataSender> dataProcessor{};
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
    strncpy_s(output, outputSize, globals::versionString, outputSize);
}

void __stdcall RVExtension(char *output, int outputSize, const char *function) {
    switch (hash(function)) {
        case hash("version"):
            RVExtensionVersion(output, outputSize);
            break;
        case hash("startup"):
            try {
                globals::dataProcessor = std::make_unique<dataSender>();
                strncpy_s(output, outputSize, "data sender started", _TRUNCATE);
            } catch (const std::exception &e) {
                strncpy_s(output, outputSize, e.what(), _TRUNCATE);
            };
            break;
        case hash("shutdown"):
            globals::dataProcessor.reset();
            strncpy_s(output, outputSize, "data sender shutdown", _TRUNCATE);
            break;
        default:
            strncpy_s(output, outputSize, ("Invalid Function: function [" + std::string(function) + "] hash [" + std::to_string(hash(function)) + "]").c_str(), _TRUNCATE);
            break;
    }
}

void __stdcall RVExtensionArgs(char *output, int outputSize, const char *function, const char **args, int argsCnt) {
    switch (hash(function)) {
        case hash("processData"):
            {
                if (!globals::dataProcessor) {
                    strncpy_s(output, outputSize, "Cannot process data when we haven't started the processor", _TRUNCATE);
                    return;
                }

                if (!globals::dataProcessor->running()) {
                    strncpy_s(output, outputSize, "Data processor is not running. Aborting", _TRUNCATE);
                    return;
                }

                if (argsCnt < 2) {
                    strncpy_s(output, outputSize, "Too few arguments", _TRUNCATE);
                    return;
                }

                const char *argument0 = args[0];
                const char *argument1 = args[1];

                for (int i = 2; i < argsCnt; i++) {
                    std::string argument = args[i];
                    globals::dataProcessor->sendData(potato::packetTypes::NONE, argument.data(), argument.size());
                }

                strncpy_s(output, outputSize, "Data Sent", _TRUNCATE);
            }
            break;
        default:
            strncpy_s(output, outputSize, ("Invalid Function: function [" + std::string(function) + "] hash [" + std::to_string(hash(function)) + "]").c_str(), _TRUNCATE);
            break;
        }
}
