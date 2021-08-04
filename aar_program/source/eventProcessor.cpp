#include "eventProcessor.hpp"
#include "dataServer.hpp"
#include "spdlog/spdlog.h"
#include "imgui.h"

void eventProcessor::logEvent(const std::vector<std::unique_ptr<potato::baseARMAVariable>> &variables) {
    m_eventQueue.emplace_back(variables);
}

eventProcessor::eventProcessor(dataServer &server) {
    server.subscribe(potato::packetTypes::GAME_EVENT, std::bind(&eventProcessor::logEvent, this, std::placeholders::_1));
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
        ImGui::EndTabItem();
    }
}
