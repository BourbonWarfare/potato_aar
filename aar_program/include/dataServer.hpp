// dataServer.hpp
// The server which data is recieved from ARMA and processed into relevant meta-data
#pragma once
#include "asio.hpp"
#include "eventSubscriber.hpp"
#include <thread>

class dataServer : public eventSubscriber {
    private:
        // port we are listening on. Since this is looped back this doesnt really matter
        static constexpr int c_port = 13;
        // Maximum packet size. Anything greater than this is split into multiple packets
        static constexpr int c_maxMessageSizeBytes = 4096;

        asio::io_context m_ioContext;
        asio::ip::udp::socket m_socket;
        asio::ip::udp::endpoint m_remoteEndpoint;

        // thread variables
        std::thread m_ioThread;
        bool m_running = true;

        // Thread update loop. Created and called in constructor
        void handleMessages();

    public:
        dataServer();
        ~dataServer();
};
