#include "missionHandler.hpp"
#include "dataServer.hpp"
#include "imgui.h"
#include "spdlog/fmt/fmt.h"

#include <fstream>
#include <filesystem>
#include <ctime>
#include "nlohmann/json.hpp"

#include "zip.h"

void missionHandler::dumpToDisk() {
    std::string missionName = m_missionName;
    missionName.erase(std::find(missionName.begin(), missionName.end(), '\"'), missionName.end());
    if (missionName == "") {
        missionName = "NO NAME";
    }

    std::string filename = fmt::format("{}_{}.zip", m_missionDate, missionName);

    struct zip_t *zip = zip_open(filename.c_str(), ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');

    {
        zip_entry_open(zip, "meta.bson");

        nlohmann::json metaInfo;
        metaInfo["name"] = m_missionName;
        metaInfo["map"] = m_worldName;
        metaInfo["endTime"] = m_missionEnd;
        metaInfo["projectileUpdateRate"] = m_projectileUpdateRate;
        metaInfo["objectUpdateRate"] = m_objectUpdateRate;

        std::vector<std::uint8_t> bson = nlohmann::json::to_bson(metaInfo);
        zip_entry_write(zip, bson.data(), bson.size());

        zip_entry_close(zip);
    }

    {
        zip_entry_open(zip, "events.bson");

        std::vector<std::uint8_t> bson = nlohmann::json::to_bson(m_eventHandler.serialise());
        zip_entry_write(zip, bson.data(), bson.size());

        zip_entry_close(zip);
    }

    m_objectHandler.serialise(zip);

    {
        zip_entry_open(zip, "projectiles.bson");

        std::vector<std::uint8_t> bson = nlohmann::json::to_bson(m_projectileHandler.serialise());
        zip_entry_write(zip, bson.data(), bson.size());

        zip_entry_close(zip);
    }

    zip_close(zip);

    m_complete = true;
}

void missionHandler::onStart(eventData &event) {
    event.eventInformation[0]->convert(m_worldName);
    event.eventInformation[1]->convert(m_missionName);
    event.eventInformation[2]->convert(m_objectUpdateRate);
    event.eventInformation[3]->convert(m_projectileUpdateRate);

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
    m_readyToDump = true;
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

missionHandler::~missionHandler() {
    if (m_dumpThread.joinable()) {
        m_dumpThread.join();
    }
}

void missionHandler::drawInfo(float appWidth, float appHeight) {
    m_projectileHandler.update();

    if (ImGui::Begin("Server Overview", false, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_MenuBar)) {
        ImGui::SetWindowSize({ appWidth, appHeight });
        ImGui::SetWindowPos({ 0, 0 });

        if (ImGui::BeginTabBar("##ViewTabs")) {
            if (ImGui::BeginTabItem("Mission Information")) {
                ImGui::Text(fmt::format("Date: {}", m_missionDate).c_str());
                ImGui::Text(fmt::format("Mission: {}", m_missionName).c_str());
                ImGui::Text(fmt::format("Map: {}", m_worldName).c_str());
                ImGui::EndTabItem();
            }

            m_eventHandler.drawInfo();
            m_projectileHandler.drawInfo();
            m_objectHandler.drawInfo();
            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

void missionHandler::dump() {
    while (!m_readyToDump) {}
    m_dumpThread = std::thread(&missionHandler::dumpToDisk, this);
}

bool missionHandler::complete() const {
    return m_complete;
}
