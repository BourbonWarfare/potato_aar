// packetHeader.hpp
// The header which preceeds all packets. Has version info
#pragma once
#include <cstdint>
#include "packetTypes.hpp"

namespace potato {
	struct packetHeader {
		const std::uint16_t c_version = 1; // range [1, 2^16-1]
		std::uint16_t sizeInBytes = 0; // size of message in bytes
		std::uint16_t packetNumber = 0; // Increments for each packet. Packets are sent in sequential order, but because of UDP may not be recieved in that order
		std::uint16_t packetCount = 0; // How many packets are in this group. Range [1, 2^16-1]
		potato::packetTypes type = potato::packetTypes::NONE;
		std::uint8_t packetGroup = 0; // This is used to denote when a packet is split. Any packets belonging to the same group should be processed together

		// the header size is constant once we deploy. This should be able to contain any information that lets us extract packet data through any version
		static constexpr uint64_t c_headerSizeBytes = sizeof(packetHeader::c_version) + sizeof(packetHeader::sizeInBytes) + sizeof(packetHeader::packetNumber) + sizeof(packetHeader::type) + sizeof(packetHeader::packetGroup) + sizeof(packetHeader::packetCount);
		static constexpr uint16_t c_maxPacketSize = 4096;
	};
}
