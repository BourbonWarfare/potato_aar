// dataSender.hpp
// Sends data to external server on LAN to process data
#pragma once
#include "extensionDllExportInfo.hpp"
#include <cstdint>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "connection.hpp"
#include "bw/packetTypes.hpp"

class dataSender {
    private:
        using dataType = std::uint8_t;

        static constexpr int c_sendFrequencyMilliseconds = 100;

        struct sendMetaData {
            std::vector<dataType> buffer;
            potato::packetTypes packetType = potato::packetTypes::NONE;
        };

        std::vector<sendMetaData> m_buffers[2];
        unsigned int m_bufferIndex = 0;

        // buffer that is sending data
        std::vector<sendMetaData> *m_currentSendBuffer = &m_buffers[0];

        // buffer that is being populated from SQF
        std::vector<sendMetaData> *m_currentPopulateBuffer = &m_buffers[1];

        bool m_running = true;
        bool m_newData = false;
        uint16_t m_currentPacketNumber = 0;
        uint8_t m_currentPacketGroup = 0; // packets can be split up, but in the same group. This tracks that
        std::thread m_sendThread;

        std::mutex m_swapMutex;
        std::condition_variable m_sendBufferLock;

        connection m_connection;

        void swapBuffers();
        void send();

    public:
        POTATO_AAR_SYMBOL dataSender();
        POTATO_AAR_SYMBOL ~dataSender();

        POTATO_AAR_SYMBOL void sendData(potato::packetTypes type, void *data, uint64_t size);
        POTATO_AAR_SYMBOL bool running() const;

};
