#include "serverObserver.hpp"
#include "dataServer.hpp"
#include "armaEvents.hpp"
#include "imgui.h"

void serverObserver::startMission(eventData &startEvent) {
    m_activeMissionsMutex.lock();
    m_activeMissions.emplace_back(std::make_unique<missionHandler>(m_server, startEvent));
    m_activeMissionsMutex.unlock();
}

void serverObserver::logEvent(const std::vector<std::unique_ptr<potato::baseARMAVariable>> &variables) {
    eventData event(variables);
    switch (event.type) {
        case armaEvents::MISSION_LOAD:
            startMission(event);
            break;
        default:
            break;
    }
}

serverObserver::serverObserver(dataServer &server) :
    m_server(server)
{
    m_server.subscribe(potato::packetTypes::GAME_EVENT, std::bind(&serverObserver::logEvent, this, std::placeholders::_1));
}

void serverObserver::update() {
    m_activeMissionsMutex.lock();
    for (auto it = m_activeMissions.begin(); it != m_activeMissions.end(); it) {
        (*it)->update();

        if ((*it)->readyToDump()) {
            (*it)->dump();
        }

        if ((*it)->readyToDelete()) {
            it = m_activeMissions.erase(it);
        } else {
            ++it;
        }
    }
    m_activeMissionsMutex.unlock();
}

void serverObserver::drawInfo(float appWidth, float appHeight) {
    if (ImGui::Begin("Server Overview", false, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_MenuBar)) {
        ImGui::SetWindowSize({ appWidth, appHeight });
        ImGui::SetWindowPos({ 0, 0 });

        ImGui::BeginTabBar("##Missions");

        if (ImGui::BeginTabItem("Server Information")) {
            ImGui::EndTabItem();
        }

        m_activeMissionsMutex.lock();
        for (auto &mission : m_activeMissions) {
            if (ImGui::BeginTabItem(mission->getMissionName().c_str(), false)) {
                mission->drawInfo();
                ImGui::EndTabItem();
            }
        }
        ImGui::EndTabBar();
        m_activeMissionsMutex.unlock();

        ImGui::End();
    }
}
