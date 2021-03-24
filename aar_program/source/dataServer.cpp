#include "dataServer.hpp"
#include "spdlog/spdlog.h"
#include "bw/packetHeader.hpp"
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

            std::vector<std::uint8_t> message;
            message.resize(header.sizeInBytes);
            for (uint16_t i = 0; i < header.sizeInBytes; i++) {
                message[i] = data[i];
            }
            message.push_back('\0');

            spdlog::info(message.data());
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
