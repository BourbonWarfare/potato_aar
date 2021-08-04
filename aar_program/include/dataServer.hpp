// dataServer.hpp
// The server which data is recieved from ARMA and processed into relevant meta-data
#pragma once
#include "asio.hpp"
#include "eventSubscriber.hpp"
#include <thread>
#include <unordered_map>
#include <vector>
#include <memory>
#include "bw/armaTypes.hpp"

class dataServer : public eventSubscriber {
    private:
        // port we are listening on. Since this is looped back this doesnt really matter
        static constexpr int c_port = 13;

        asio::io_context m_ioContext;
        asio::ip::udp::socket m_socket;
        asio::ip::udp::endpoint m_remoteEndpoint;

        // thread variables
        std::thread m_ioThread;
        bool m_running = true;

        struct packetInfo {
            struct splitInformation {
                std::vector<std::uint8_t> m_data;
                std::uint16_t m_packetNumber = 0;
            };
            std::vector<splitInformation> m_splitData;
        };

        std::unordered_map<unsigned int, packetInfo> m_inboundPackets;

        std::vector<std::unique_ptr<potato::baseARMAVariable>> construct(packetInfo &packetInformation);

        // Thread update loop. Created and called in constructor
        void handleMessages();

    public:
        dataServer();
        ~dataServer();
};
