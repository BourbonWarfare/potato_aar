#include "connection.hpp"

connection::connection() : 
    m_socket(m_ioContext) 
{
    asio::ip::udp::resolver resolver(m_ioContext);
    m_endpoint = *resolver.resolve(asio::ip::udp::v4(), "localhost", "daytime");

    m_socket.open(asio::ip::udp::v4());
}

void connection::send(void *data, uint64_t sizeInBytes) {
    m_socket.send_to(asio::buffer(data, sizeInBytes), m_endpoint);
}
