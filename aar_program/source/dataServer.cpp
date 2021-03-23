#include "dataServer.hpp"
#include "spdlog/spdlog.h"

using asio::ip::udp;

void dataServer::handleMessage(const asio::error_code &error, std::size_t messageInBytes)
    {
        if (error)
            {
                spdlog::error(error.message());
                return;
            }

        spdlog::info(m_receiveBuffer.data());
    }

void dataServer::startReceive() {
    m_socket.async_receive_from(
        asio::buffer(m_receiveBuffer), m_remoteEndpoint,
        std::bind(&dataServer::handleMessage, this, std::placeholders::_1, std::placeholders::_2)
    );
}

dataServer::dataServer() :
    m_socket(m_ioContext, udp::endpoint(udp::v4(), c_port))
{
    startReceive();
    m_ioContext.run();
}
