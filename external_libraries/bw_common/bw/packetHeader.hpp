// packetHeader.hpp
// The header which preceeds all packets. Has version info
#pragma once
#include <cstdint>
#include "packetTypes.hpp"

namespace potato {
	struct packetHeader {
		const uint16_t c_version = 1; // range [1, 2^16-1]
		uint16_t sizeInBytes = 0; // size of message in bytes
		uint16_t packetNumber = 0;
		potato::packetTypes type = potato::packetTypes::NONE;
		uint8_t packetGroup = 0;

		// the header size is constant once we deploy. This should be able to contain any information that lets us extract packet data through any version
		static constexpr uint64_t c_headerSizeBytes = sizeof(packetHeader::c_version) + sizeof(packetHeader::sizeInBytes) + sizeof(packetHeader::packetNumber) + sizeof(packetHeader::type) + sizeof(packetHeader::packetGroup);
	};
}
