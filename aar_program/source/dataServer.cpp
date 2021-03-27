#include "dataServer.hpp"
#include "spdlog/spdlog.h"
#include "bw/packetHeader.hpp"
#include "bw/armaTypes.hpp"
#include <array>
#include <vector>

using asio::ip::udp;

void dataServer::handleMessages() {
    std::vector<std::unique_ptr<potato::baseARMAVariable>> variablesInPacket;
    while (m_running) {
        try {
            std::array<std::uint8_t, c_maxMessageSizeBytes> receiveBuffer;
            udp::endpoint remoteEndpoint;
            m_socket.receive_from(asio::buffer(receiveBuffer), remoteEndpoint);

            if (!m_running) {
                return;
            }

            std::uint16_t totalPacketSize = *reinterpret_cast<std::uint16_t*>(receiveBuffer.data());

            for (int offset = sizeof(totalPacketSize); offset < totalPacketSize; offset) {
                potato::packetHeader header = *reinterpret_cast<potato::packetHeader*>(receiveBuffer.data() + offset);
                offset += potato::packetHeader::c_headerSizeBytes;

                std::uint8_t *data = receiveBuffer.data() + offset;

                for (int i = 0; i < header.sizeInBytes; i) {
                    potato::variableType type = *reinterpret_cast<potato::variableType*>(data + i);
                    i += sizeof(potato::variableType);

                    std::intptr_t sizeOfType = *reinterpret_cast<std::intptr_t*>(data + i);
                    i += sizeof(sizeOfType);

                    // parse until we find end of string
                    std::vector<std::uint8_t> dataBuffer = {};

                    for (int j = 0; j < sizeOfType; j++) {
                        dataBuffer.push_back(data[i + j]);
                    }
                    i += sizeOfType;

                    if (!dataBuffer.empty()) {
                        variablesInPacket.push_back(std::move(getARMAVariableFromType(type)));
                        // security risk, but this is LAN so it doesn't matter
                        // if this program isn't LAN anymore and this still exists, uh oh you have a security hole.
                        // The `set` function blindly sets the pointer to whatever type `type` is, so if a false packet comes through it could overrun the buffer and have access to program memory
                        variablesInPacket.back()->set(dataBuffer.data(), type);
                    }
                }

                offset += header.sizeInBytes;
                signal(header.type, variablesInPacket);
                spdlog::info("Packet with {} variables of type {} recieved and signaled", variablesInPacket.size(), header.type);
                variablesInPacket.clear();
            }
            spdlog::info("processed packet with {} bytes", totalPacketSize);
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
    m_ioThread = std::thread(&dataServer::handleMessages, this);
}

dataServer::~dataServer()
    {
        m_running = false;
        m_socket.shutdown(asio::socket_base::shutdown_type::shutdown_both);
        m_ioThread.join();
    }
