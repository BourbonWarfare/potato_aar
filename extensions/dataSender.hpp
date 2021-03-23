// dataSender.hpp
// Sends data to external server on LAN to process data
#pragma once
#include "extensionDllExportInfo.hpp"
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "connection.hpp"

class dataSender {
    private:
        using dataType = char;

        std::vector<dataType> m_buffers[2];
        unsigned int m_bufferIndex = 0;

        // buffer that is sending data
        std::vector<dataType> *m_currentSendBuffer = &m_buffers[0];

        // buffer that is being populated from SQF
        std::vector<dataType> *m_currentPopulateBuffer = &m_buffers[1];

        bool m_running = true;
        bool m_newData = false;
        std::thread m_sendThread;

        std::mutex m_swapMutex;
        std::condition_variable m_sendBufferLock;

        connection m_connection;

        void swapBuffers();
        void send();

    public:
        POTATO_AAR_SYMBOL dataSender();
        POTATO_AAR_SYMBOL ~dataSender();

        POTATO_AAR_SYMBOL void sendData(void *data, uint64_t size);

};
