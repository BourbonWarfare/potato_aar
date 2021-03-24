#include "dataSender.hpp"
#include "bw/packetHeader.hpp"

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
        potato::packetHeader header;
        header.sizeInBytes = m_currentSendBuffer->size() * sizeof(dataType);
        header.type = potato::packetTypes::DEBUG_MESSAGE;

        std::vector<uint8_t> data;
        data.resize(potato::packetHeader::c_headerSizeBytes);

        // convert header data in memory to data we can send
        for (uint8_t i = 0; i < potato::packetHeader::c_headerSizeBytes; i++) {
            data[i] = reinterpret_cast<char*>(&header)[i];
        }

        data.insert(data.end(), m_currentSendBuffer->begin(), m_currentSendBuffer->end());

        m_connection.send(data.data(), data.size() * sizeof(dataType));
        m_currentSendBuffer->clear();
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

    for (uint64_t i = 0; i < size; i++) {
        m_currentPopulateBuffer->push_back(reinterpret_cast<char*>(data)[i]);
    }

    m_newData = true;
    m_sendBufferLock.notify_all();

    lock.unlock();
}
