#include "dataSender.hpp"
#include "bw/packetHeader.hpp"
#include "spdlog/spdlog.h"
#include <algorithm>

void dataSender::swapBuffers()  {
    m_currentPopulateBuffer = &m_buffers[m_bufferIndex];
    m_bufferIndex = (m_bufferIndex + 1) % 2;
    m_currentSendBuffer = &m_buffers[m_bufferIndex];
}

void dataSender::send() {
    while (m_running) {
        // wait until we have data. When we do, lock the buffer and swap. Then process data
        std::unique_lock<std::mutex> lock(m_swapMutex);
        m_sendBufferLock.wait(lock, [this]{ return m_newData || !m_running; });
        if (!m_running)
            {
                return;
            }
        swapBuffers();
        lock.unlock();

        m_newData = false;

        // prepend header and send across network
        for (const auto &packetMetaData : *m_currentSendBuffer) {
            std::uint16_t unadjustedSize = static_cast<uint16_t>(packetMetaData.buffer.size() * sizeof(dataType));
            if (unadjustedSize + potato::packetHeader::c_headerSizeBytes < potato::packetHeader::c_maxPacketSize) {
                potato::packetHeader header;
                header.sizeInBytes = unadjustedSize;
                header.type = packetMetaData.packetType;
                header.packetNumber = m_currentPacketNumber++;
                header.packetGroup = m_currentPacketGroup++;
                header.packetCount = 1;

                std::vector<uint8_t> packet(potato::packetHeader::c_headerSizeBytes);
                // convert header data in memory to data we can send
                for (uint8_t i = 0; i < potato::packetHeader::c_headerSizeBytes; i++) {
                    packet[i] = reinterpret_cast<char*>(&header)[i];
                }
                packet.insert(packet.end(), packetMetaData.buffer.begin(), packetMetaData.buffer.end());

                m_connection.send(packet.data(), packet.size() * sizeof(dataType));
                spdlog::info("sent non-split packet with {} bytes [{} with header]", unadjustedSize, packet.size());
            } else {
                // need to split packet into multiple groups
                // the header could theoretically push us over the limit at some point, so we need to take them into account
                std::uint16_t allPacketsSize = unadjustedSize;
                int packetCountWithHeader = 0; // actual amount of packets we need
                int packetCountNoHeader = 1;
                while (packetCountWithHeader != packetCountNoHeader) {
                    packetCountNoHeader = 1 + (allPacketsSize / potato::packetHeader::c_maxPacketSize);
                    allPacketsSize = unadjustedSize + packetCountNoHeader * potato::packetHeader::c_headerSizeBytes;
                    packetCountWithHeader = 1 + (allPacketsSize / potato::packetHeader::c_maxPacketSize);
                }

                std::uint16_t remainingPacketSize = allPacketsSize;
                std::intptr_t bufferOffset = 0;

                for (std::uint16_t i = 0; i < allPacketsSize; i += potato::packetHeader::c_maxPacketSize) {
                    potato::packetHeader header;
                    header.sizeInBytes = std::min(potato::packetHeader::c_maxPacketSize, static_cast<std::uint16_t>(remainingPacketSize - i)) - potato::packetHeader::c_headerSizeBytes;
                    header.type = packetMetaData.packetType;
                    header.packetNumber = m_currentPacketNumber++;
                    header.packetGroup = m_currentPacketGroup;
                    header.packetCount = packetCountWithHeader;

                    std::vector<dataType> packet(potato::packetHeader::c_headerSizeBytes);
                    // convert header data in memory to data we can send
                    for (uint8_t i = 0; i < potato::packetHeader::c_headerSizeBytes; i++) {
                        packet[i] = reinterpret_cast<char*>(&header)[i];
                    }
                    packet.insert(packet.end(), packetMetaData.buffer.begin() + bufferOffset, packetMetaData.buffer.begin() + bufferOffset + header.sizeInBytes);

                    bufferOffset += header.sizeInBytes;

                    m_connection.send(packet.data(), packet.size() * sizeof(dataType));
                    spdlog::info("sent split packet with {} bytes [{} with header] {}/{}", header.sizeInBytes, packet.size(), 1 + i / potato::packetHeader::c_maxPacketSize, header.packetCount);
                }

                m_currentPacketGroup++;
            }
        }

        m_currentSendBuffer->clear();       
        std::this_thread::sleep_for(std::chrono::milliseconds(c_sendFrequencyMilliseconds));
    }
}

dataSender::dataSender()  {
    m_running = true;
    m_sendThread = std::thread(&dataSender::send, this);
}

dataSender::~dataSender()  {
    m_running = false;
    m_sendBufferLock.notify_all();
    m_sendThread.join();
}

bool dataSender::running() const {
    return m_running;
}

void dataSender::sendData(potato::packetTypes type, void *data, uint64_t size) {
    std::unique_lock<std::mutex> lock(m_swapMutex);

    m_currentPopulateBuffer->emplace_back();
    m_currentPopulateBuffer->back().packetType = type;
    for (uint64_t i = 0; i < size; i++) {
        m_currentPopulateBuffer->back().buffer.push_back(reinterpret_cast<std::uint8_t*>(data)[i]);
    }

    m_newData = true;
    m_sendBufferLock.notify_all();

    lock.unlock();
}
