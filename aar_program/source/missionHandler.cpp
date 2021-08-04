#include "missionHandler.hpp"
#include "dataServer.hpp"
#include "imgui.h"
#include "spdlog/fmt/fmt.h"

void missionHandler::onStart(eventData &event) {
    event.eventInformation[0]->convert(m_worldName);
    event.eventInformation[1]->convert(m_missionName);
}

void missionHandler::onEnd(eventData &event) {

}

void missionHandler::logEvent(const std::vector<std::unique_ptr<potato::baseARMAVariable>> &variables) {
    eventData event(variables);

    switch (event.type) {
        case armaEvents::MISSION_LOAD:
            onStart(event);
            break;
        case armaEvents::MISSION_END:
            onEnd(event);
            break;
        default:
            break;
    }
}

missionHandler::missionHandler(dataServer &server) :
    m_eventHandler(server),
    m_objectHandler(server),
    m_projectileHandler(server)
{
    server.subscribe(potato::packetTypes::GAME_EVENT, std::bind(&missionHandler::logEvent, this, std::placeholders::_1));
}

void missionHandler::drawInfo(float appWidth, float appHeight) {
    m_projectileHandler.update();

    if (ImGui::Begin("Server Overview", false, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_MenuBar)) {
        ImGui::SetWindowSize({ appWidth, appHeight });
        ImGui::SetWindowPos({ 0, 0 });

        ImGui::BeginMenuBar();

        ImGui::Text(fmt::format("Mission: {}", m_missionName).c_str());
        ImGui::Text(fmt::format("Map: {}", m_worldName).c_str());

        ImGui::EndMenuBar();

        if (ImGui::BeginTabBar("##ViewTabs")) {
            m_eventHandler.drawInfo();
            m_projectileHandler.drawInfo();
            m_objectHandler.drawInfo();
            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}
