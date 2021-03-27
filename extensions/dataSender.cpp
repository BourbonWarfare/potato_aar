#include "dataSender.hpp"
#include "bw/packetHeader.hpp"
#include "spdlog/spdlog.h"

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
        std::uint16_t totalSize = sizeof(totalSize);
        std::vector<uint8_t> data;
        for (const auto &packetMetaData : *m_currentSendBuffer) {
            std::vector<uint8_t> thisData;
            potato::packetHeader header;
            header.sizeInBytes = static_cast<uint16_t>(packetMetaData.buffer.size() * sizeof(dataType));
            header.type = packetMetaData.packetType;
            header.packetNumber = m_currentPacketNumber++;
            header.packetGroup = m_currentPacketGroup++;
            
            thisData.resize(potato::packetHeader::c_headerSizeBytes);
            // convert header data in memory to data we can send
            for (uint8_t i = 0; i < potato::packetHeader::c_headerSizeBytes; i++) {
                thisData[i] = reinterpret_cast<char*>(&header)[i];
            }

            // shitty name, but thisData refers to the packet currently being worked on
            thisData.insert(thisData.end(), packetMetaData.buffer.begin(), packetMetaData.buffer.end());

            // I know we can refactor this to make it simpler, but my sleep deprived brain cant think that much right now
            data.insert(data.end(), thisData.begin(), thisData.end());

            totalSize += header.sizeInBytes + potato::packetHeader::c_headerSizeBytes;
        }

        // prepend total packet size so we can read later
        std::uint8_t totalSizeData[2] = {
            *(reinterpret_cast<std::uint8_t*>(&totalSize) + 0),
            *(reinterpret_cast<std::uint8_t*>(&totalSize) + 1)
        };

        // insert size data at beginning of header so we avoid buffer overruns in server
        data.insert(data.begin(), std::begin(totalSizeData), std::end(totalSizeData));

        m_connection.send(data.data(), data.size() * sizeof(dataType));
        m_currentSendBuffer->clear();

        spdlog::info("sent packet with {} bytes [{}, {}]", totalSize, data.size(), *reinterpret_cast<std::uint16_t*>(&totalSizeData));
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
