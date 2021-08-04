#include "eventProcessor.hpp"
#include "dataServer.hpp"
#include "spdlog/spdlog.h"
#include "imgui.h"

eventProcessor::eventProcessor(dataServer &server) {
    server.subscribe(potato::packetTypes::GAME_EVENT, std::bind(&eventProcessor::logEvent, this, std::placeholders::_1));
}

void eventProcessor::logEvent(const std::vector<std::unique_ptr<potato::baseARMAVariable>> &variables) {
    eventData event;

    potato::armaArray &eventMetaInfo = *static_cast<potato::armaArray*>(variables[0].get());

    double eventNumber = 0;
    eventMetaInfo.data[0]->convert(eventNumber);
    eventMetaInfo.data[1]->convert(event.eventTime);

    event.type = static_cast<armaEvents>(eventNumber);

    potato::armaArray &eventInfo = *static_cast<potato::armaArray*>(eventMetaInfo.data[2].get());

    for (auto &variable : eventInfo.data) {
        event.eventInformation.push_back(variable->copy());
    }

    m_eventQueue.emplace_back(std::move(event));
}

void eventProcessor::drawInfo() const {
    if (ImGui::BeginTabItem("Game Events")) {
        std::intptr_t index = 0;
        for (auto &event : m_eventQueue) {
            if (ImGui::TreeNode(reinterpret_cast<void*>(index), potato::getEventString(event.type).data())) {
                ImGui::Text(fmt::format("Event Time: {:.2f}", event.eventTime).c_str());

                ImGui::Text("Event Data:");
                for (auto &variable : event.eventInformation) {
                    ImGui::Spacing();
                    ImGui::SameLine();
                    ImGui::Text(variable->toString().c_str());
                }

                ImGui::TreePop();
            }
            index++;
        }
    }
    ImGui::EndTabItem();
}
