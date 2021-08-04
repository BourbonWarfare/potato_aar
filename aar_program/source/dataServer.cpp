#include "dataServer.hpp"
#include "spdlog/spdlog.h"
#include "bw/packetHeader.hpp"
#include "bw/armaTypes.hpp"
#include <array>
#include <vector>

using asio::ip::udp;

std::vector<std::unique_ptr<potato::baseARMAVariable>> dataServer::construct(packetInfo &packetInformation) {
    std::sort(packetInformation.m_splitData.begin(), packetInformation.m_splitData.end(), [] (const packetInfo::splitInformation &a, const packetInfo::splitInformation &b) {
        return a.m_packetNumber < b.m_packetNumber;
    });
    std::vector<std::uint8_t> allVariables;
    for (auto &packet : packetInformation.m_splitData) {
        allVariables.insert(allVariables.end(), packet.m_data.begin(), packet.m_data.end());
    }

    std::vector<std::unique_ptr<potato::baseARMAVariable>> variablesInPacket;
    for (std::uintptr_t offset = 0; offset < allVariables.size(); offset) {
        potato::variableType type = *reinterpret_cast<potato::variableType*>(allVariables.data() + offset);
        offset += sizeof(potato::variableType);

        std::intptr_t variableDataSize = *reinterpret_cast<std::intptr_t*>(allVariables.data() + offset);
        offset += sizeof(std::intptr_t);

        if (offset + variableDataSize > allVariables.size()) {
            throw std::runtime_error(fmt::format("Variable data exceeds incoming size: {} + {} = {} >= {}", offset, variableDataSize, offset + variableDataSize, variablesInPacket.size()));
        }

        std::vector<std::uint8_t> variableData(allVariables.data() + offset, allVariables.data() + offset + variableDataSize);
        offset += variableData.size();

        if (!variableData.empty()) {
            variablesInPacket.emplace_back(std::move(potato::getARMAVariableFromType(type)));
            variablesInPacket.back()->set(variableData.data(), type, variableData.size());
        }
    }

    return variablesInPacket;
}

void dataServer::handleMessages() {
    std::vector<std::unique_ptr<potato::baseARMAVariable>> variablesInPacket;
    while (m_running) {
        try {
            std::array<std::uint8_t, potato::packetHeader::c_maxPacketSize> receiveBuffer;
            udp::endpoint remoteEndpoint;
            m_socket.receive_from(asio::buffer(receiveBuffer), remoteEndpoint);

            if (!m_running) {
                return;
            }

            // Convert binary data into object data
            std::uint16_t offset = 0;
            potato::packetHeader header = *reinterpret_cast<potato::packetHeader*>(receiveBuffer.data() + offset);
            offset += potato::packetHeader::c_headerSizeBytes;

            unsigned int maxSize = receiveBuffer.size() - offset;
            std::uint8_t *data = receiveBuffer.data() + offset;

            packetInfo &workingPacket = m_inboundPackets[header.packetGroup];
            workingPacket.m_splitData.emplace_back(packetInfo::splitInformation{
                std::vector<std::uint8_t>(data, data + header.sizeInBytes),
                header.packetNumber
            });

            // not worried about packet loss because we are on LAN
            if (workingPacket.m_splitData.size() >= header.packetCount) {
                signal(header.type, construct(workingPacket));
                spdlog::info("Packet with {} variables of type {} recieved and signaled", variablesInPacket.size(), header.type);

                m_inboundPackets.erase(header.packetGroup);
            }

            spdlog::info("processed packet with {} bytes", header.sizeInBytes);
        }
        catch (std::exception &e) {
            spdlog::error(fmt::format("Packet Reception: {}", e.what()));
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

