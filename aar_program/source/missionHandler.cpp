#include "missionHandler.hpp"
#include "dataServer.hpp"
#include "imgui.h"
#include "spdlog/fmt/fmt.h"

#include <fstream>
#include <filesystem>
#include <ctime>
#include "nlohmann/json.hpp"

void missionHandler::dumpToDisk() {
    std::string missionName = m_missionName;
    missionName.erase(std::find(missionName.begin(), missionName.end(), '\"'), missionName.end());
    if (missionName == "") {
        missionName = "NO NAME";
    }

    std::string directory = fmt::format("{} - {}", m_missionDate, missionName);
    std::filesystem::create_directory(directory);

    std::ofstream out(fmt::format("{}/meta.json", directory));
    nlohmann::json metaInfo;
    metaInfo["name"] = m_missionName;
    metaInfo["map"] = m_worldName;
    metaInfo["endTime"] = m_missionEnd;
    out << metaInfo.dump(4);
    out.close();

    out.open(fmt::format("{}/events.json", directory));
    out << m_eventHandler.serialise().dump(4);
    out.close();

    out.open(fmt::format("{}/objects.json", directory));
    out << m_objectHandler.serialise().dump(4);
    out.close();

    out.open(fmt::format("{}/projectiles.json", directory));
    out << m_projectileHandler.serialise().dump(4);
    out.close();
}

void missionHandler::onStart(eventData &event) {
    event.eventInformation[0]->convert(m_worldName);
    event.eventInformation[1]->convert(m_missionName);

    time_t rawtime;
    struct tm *timeinfo;
    char buffer[80];

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer, sizeof(buffer), "%d%m%y %H%M%S", timeinfo);
    m_missionDate = buffer;
}

void missionHandler::onEnd(eventData &event) {
    m_missionEnd = event.eventTime;
    dumpToDisk();
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

        if (ImGui::Button("Dump To Disk")) {
            dumpToDisk();
        }

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
