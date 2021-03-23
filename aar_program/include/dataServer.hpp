// dataServer.hpp
// The server which data is recieved from ARMA and processed into relevant meta-data
#pragma once
#include "asio.hpp"
#include <array>

class dataServer {
    private:
        static constexpr int c_port = 13;
        static constexpr int c_maxMessageSizeBytes = 4096;

        asio::io_context m_ioContext;
        asio::ip::udp::socket m_socket;
        asio::ip::udp::endpoint m_remoteEndpoint;

        std::array<char, c_maxMessageSizeBytes> m_receiveBuffer;

        void handleMessage(const asio::error_code &error, std::size_t messageInBytes);
        void startReceive();

    public:
        dataServer();
};
