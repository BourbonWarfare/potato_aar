#include "dataSender.hpp"

void dataSender::swapBuffers()  {
    m_currentPopulateBuffer = &m_buffers[m_bufferIndex];
    m_bufferIndex = (m_bufferIndex + 1) % 2;
    m_currentSendBuffer = &m_buffers[m_bufferIndex];
}

void dataSender::send()  {
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

        // process
        m_connection.send(m_currentSendBuffer->data(), m_currentSendBuffer->size() * sizeof(dataType));
    }
}

dataSender::dataSender()  {
    m_sendThread = std::thread(&dataSender::send, this);
}

dataSender::~dataSender()  {
    m_running = false;
    m_sendThread.join();
}

void dataSender::sendData(void *data, uint64_t size) {
    std::unique_lock<std::mutex> lock(m_swapMutex);

    for (uint64_t i = 0; i < size; i++) {
        m_currentPopulateBuffer->push_back(reinterpret_cast<char*>(data)[i]);
    }

    m_newData = true;

    lock.unlock();
}
