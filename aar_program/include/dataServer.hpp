// dataServer.hpp
// The server which data is recieved from ARMA and processed into relevant meta-data
#pragma once
#include "asio.hpp"

class dataServer {
    private:
        static constexpr int c_port = 13;
        static constexpr int c_maxMessageSizeBytes = 4096;

        asio::io_context m_ioContext;
        asio::ip::udp::socket m_socket;
        asio::ip::udp::endpoint m_remoteEndpoint;

        bool m_running = true;

        void handleMessages();

    public:
        dataServer();
};
