// connection.hpp
// Handles a connection for sending data on
#pragma once
#include "extensionDllExportInfo.hpp"
#include <asio.hpp>

class connection {
    private:
        asio::io_context m_ioContext;
        asio::ip::udp::socket m_socket;
        asio::ip::udp::endpoint m_endpoint;

    public:
        POTATO_AAR_SYMBOL connection();
        POTATO_AAR_SYMBOL void send(void *data, uint64_t sizeInBytes);

};
