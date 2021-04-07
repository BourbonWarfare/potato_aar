// eventProcessor.hpp
// Process events sent from ARMA
#pragma once
#include <vector>
#include <queue>
#include <memory>
#include <ostream>
#include "armaEvents.hpp"

namespace potato {
	struct baseARMAVariable;
}

class dataServer;
class eventProcessor {
	private:
		std::queue<eventData> m_events;
		void readPacket(const std::vector<std::unique_ptr<potato::baseARMAVariable>> &variables);

	public:
		eventProcessor(dataServer &server);
		void outputData(std::ostream &out);
};
