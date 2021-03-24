#include "connection.hpp"
#include "spdlog/spdlog.h"
#include <string>

connection::connection() : 
    m_socket(m_ioContext) 
{
    asio::ip::udp::resolver resolver(m_ioContext);
    m_endpoint = *resolver.resolve(asio::ip::udp::v4(), "localhost", "daytime");

    m_socket.open(asio::ip::udp::v4());
}

void connection::send(void *data, uint64_t sizeInBytes) {
    try {
        m_socket.send_to(asio::buffer(data, sizeInBytes), m_endpoint);
    } catch (std::exception &e) {
        spdlog::error(e.what());
    }
}
