#include "dataServer.hpp"
#include "spdlog/spdlog.h"
#include "bw/packetHeader.hpp"
#include "bw/armaTypes.hpp"
#include <array>
#include <vector>

using asio::ip::udp;

void dataServer::handleMessages() {
    while (m_running) {
        try {
            std::array<std::uint8_t, c_maxMessageSizeBytes> receiveBuffer;
            udp::endpoint remoteEndpoint;
            m_socket.receive_from(asio::buffer(receiveBuffer), remoteEndpoint);

            potato::packetHeader header = *reinterpret_cast<potato::packetHeader*>(receiveBuffer.data());
            std::uint8_t *data = receiveBuffer.data() + potato::packetHeader::c_headerSizeBytes;

            // we know data is in format <type data \0>
            for (int i = 0; i < header.sizeInBytes; i++) {
                potato::variableType type = *reinterpret_cast<potato::variableType*>(data + i);
                i += sizeof(potato::variableType);

                // parse until we find end of string
                std::string dataString = "";
                for (int j = 0; j < header.sizeInBytes - i; j++) {
                    char currentChar = data[i + j];
                    if (currentChar == '|') {
                        i += j;
                        break;
                    }
                    dataString += currentChar;
                }

                spdlog::info("{} {}", potato::getTypeString(type), dataString);
            }
        }
        catch (std::exception &e) {
            spdlog::error(e.what());
        }
    }
}

dataServer::dataServer() :
    m_socket(m_ioContext, udp::endpoint(udp::v4(), c_port))
{
    spdlog::info("starting server");
    handleMessages();
}
