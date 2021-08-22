#include "serverObserver.hpp"
#include "dataServer.hpp"
#include "armaEvents.hpp"
#include "imgui.h"
#include "spdlog/spdlog.h"
#include "nlohmann/json.hpp"
#include <fstream>

void serverObserver::startMission(eventData &startEvent) {
    m_activeMissionsMutex.lock();
    m_activeMissions.emplace_back(std::make_unique<missionHandler>(m_server, startEvent, c_config));
    m_activeMissionsMutex.unlock();

    potato::armaArray &metaInfo = *static_cast<potato::armaArray*>(startEvent.eventInformation[0].get());

    double heartbeatRate = 0.0;
    metaInfo.data[5]->convert(heartbeatRate);
    m_heartbeatRate = fe::seconds(heartbeatRate);

    m_heartbeatClock.restart();
}

void serverObserver::logEvent(const std::vector<std::unique_ptr<potato::baseARMAVariable>> &variables) {
    eventData event(variables);
    switch (event.type) {
        case armaEvents::MISSION_LOAD:
            startMission(event);
            spdlog::info("Mission Load");
            m_inMission = true;
            break;
        case armaEvents::MISSION_END:
            spdlog::info("Mission End");
            m_inMission = false;
            break;
        default:
            break;
    }
}

void serverObserver::heartbeat(const std::vector<std::unique_ptr<potato::baseARMAVariable>> &variables) {
    m_heartbeatClock.restart();
    spdlog::info("heartbeat");
}

serverObserver::serverObserver(dataServer &server) :
    m_server(server)
{
    m_server.subscribe(potato::packetTypes::GAME_EVENT, std::bind(&serverObserver::logEvent, this, std::placeholders::_1));
    m_server.subscribe(potato::packetTypes::HEARTBEAT, std::bind(&serverObserver::heartbeat, this, std::placeholders::_1));

    nlohmann::json config;
    std::ifstream in("aar_config.json");
    if (in) {
        in >> config;
    } else {
        std::ofstream out("aar_config.json");
        config["replay_path"] = "";
        config["database_path"] = "";
        config["heartbeat_timeout_count"] = 5;
        out << config;
        out.close();
    }
    in.close();

    const_cast<nlohmann::json&>(c_config) = config;
    if (config["heartbeat_timeout_count"].is_number_integer()) {
        const_cast<unsigned int&>(c_heartbeatTimeouts) = config["heartbeat_timeout_count"].get<unsigned int>();
    }
}

void serverObserver::update() {
    bool nonResponsive = m_inMission && m_heartbeatClock.getTime() > m_heartbeatRate * c_heartbeatTimeouts;
    if (nonResponsive) {
        m_inMission = false;
        if (!m_activeMissions.empty()) {
            spdlog::error("ARMA is non-responsive! Dumping missions...");
        }
    }

    m_activeMissionsMutex.lock();
    for (auto it = m_activeMissions.begin(); it != m_activeMissions.end(); it) {
        (*it)->update();

        if (((*it)->readyToDump() && !(*it)->isDumping()) || nonResponsive) {
            spdlog::info("Ready to dump: {} Is Dumping: {} Non Responsive: {}", (*it)->readyToDump(), (*it)->isDumping(), nonResponsive);
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
            ImGui::Text(fmt::format("Time Since Last Heartbeat: {:.2f}s", m_heartbeatClock.getTime().asSeconds()).c_str());
            if (m_heartbeatRate != 0 && !m_activeMissions.empty()) {
                ImGui::SameLine();
                ImGui::Text(fmt::format("Timeout {}/{}", std::floor(m_heartbeatClock.getTime().asSeconds() / m_heartbeatRate.asSeconds()), c_heartbeatTimeouts).c_str());
            }

            ImGui::Text("AAR Config");
            ImGui::Text(c_config.dump(2).c_str());

            ImGui::EndTabItem();
        }

        m_activeMissionsMutex.lock();
        for (auto &mission : m_activeMissions) {
            std::string tabTitle = mission->getMissionName();
            tabTitle.erase(std::remove(tabTitle.begin(), tabTitle.end(), '"'), tabTitle.end());

            if (mission->readyToDelete()) {
                tabTitle = fmt::format("[COMPLETE] {}", tabTitle);
            } else if (mission->isDumping()) {
                tabTitle = fmt::format("[DUMPING] {}", tabTitle);
            }

            if (ImGui::BeginTabItem(tabTitle.c_str(), false)) {
                mission->drawInfo();
                ImGui::EndTabItem();
            }
        }
        ImGui::EndTabBar();
        m_activeMissionsMutex.unlock();

        ImGui::End();
    }
}
