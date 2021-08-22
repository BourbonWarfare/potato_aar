#include "missionHandler.hpp"
#include "dataServer.hpp"
#include "imgui.h"
#include "spdlog/spdlog.h"

#include <fstream>
#include <filesystem>
#include <ctime>
#include "nlohmann/json.hpp"
#include "zip.h"
#include "sqlite3.h"

void missionHandler::dumpToDisk() const {
    spdlog::info("dumping {} to disk", m_missionName);

    std::string missionName = m_missionName;
    missionName.erase(std::remove(missionName.begin(), missionName.end(), '\"'), missionName.end());
    if (missionName == "") {
        missionName = "NO NAME";
    }

    std::string databasePath = c_config["database_path"];
    if (databasePath != "" && databasePath[databasePath.size() - 1] != '/') {
        databasePath += '/';
    }
    databasePath += "missions.db";

    std::string replayPath = c_config["replay_path"];
    if (replayPath != "" && replayPath[replayPath.size() - 1] != '/') {
        replayPath += '/';
    }

    std::string filename = fmt::format("{}_{}.zip", m_missionDate, missionName);
    std::string filepath = replayPath + filename;

    spdlog::info("mission zip path: {}", filepath);

    struct zip_t *zip = zip_open(filepath.c_str(), ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');

    {
        zip_entry_open(zip, "meta.json");

        nlohmann::json metaInfo;
        metaInfo["name"] = m_missionName;
        metaInfo["map"] = m_worldName;
        metaInfo["mapSize"] = m_worldSize;
        metaInfo["endTime"] = m_missionEnd;
        metaInfo["projectileUpdateRate"] = m_projectileUpdateRate;
        metaInfo["objectUpdateRate"] = m_objectUpdateRate;
        metaInfo["version"] = 1;

        std::string metaInfoString = metaInfo.dump(4);
        zip_entry_write(zip, metaInfoString.c_str(), metaInfoString.size());

        zip_entry_close(zip);
    }

    {
        zip_entry_open(zip, "events.bson");

        std::vector<std::uint8_t> bson = nlohmann::json::to_bson(m_eventHandler.serialise());
        zip_entry_write(zip, bson.data(), bson.size());

        zip_entry_close(zip);
    }

    m_objectHandler.serialise(zip);
    m_projectileHandler.serialise(zip);

    zip_close(zip);

    spdlog::info("db path: {}", databasePath);
    sqlite3 *db = nullptr;
    int returnCode = sqlite3_open(databasePath.c_str(), &db);
    if (returnCode != SQLITE_OK) {
        spdlog::error("Cannot open database: {}", sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }

    unsigned int playerCount = m_objectHandler.playerCount();

    const auto sql = fmt::format("INSERT INTO missions VALUES ('{}', '{}', '{}', '{}', '{}')", m_missionName, m_worldName, playerCount, m_missionDateEpoch, filename);

    char *zErrMessage = nullptr;
    spdlog::info("Inserting into database with SQL: {}", sql);
    returnCode = sqlite3_exec(db, sql.c_str(), [] (void *, int, char **, char **) -> int { return 0; }, nullptr, &zErrMessage);
    if (returnCode != SQLITE_OK) {
        spdlog::error("SQL: {} [{}]", sqlite3_errmsg(db), zErrMessage);
        sqlite3_free(zErrMessage);
        sqlite3_close(db);
        return;
    }

    sqlite3_close(db);
}

void missionHandler::onStart(eventData &event) {
    potato::armaArray &metaInfo = *static_cast<potato::armaArray*>(event.eventInformation[0].get());

    metaInfo.data[0]->convert(m_worldName);
    metaInfo.data[1]->convert(m_worldSize);
    metaInfo.data[2]->convert(m_missionName);
    metaInfo.data[3]->convert(m_objectUpdateRate);
    metaInfo.data[4]->convert(m_projectileUpdateRate);

    m_worldName.erase(std::remove(m_worldName.begin(), m_worldName.end(), '"'), m_worldName.end());
    m_missionName.erase(std::remove(m_missionName.begin(), m_missionName.end(), '"'), m_missionName.end());

    m_projectileHandler.updateRate = m_projectileUpdateRate;

    time_t rawtime;
    struct tm *timeinfo;
    char buffer[256];

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer, sizeof(buffer), "%d-%m-%y_%H-%M-%S", timeinfo);
    m_missionDateEpoch = std::mktime(timeinfo);
    m_missionDate = buffer;

    m_eventHandlerHandle = m_server.subscribe(potato::packetTypes::GAME_EVENT, std::bind(&missionHandler::logEvent, this, std::placeholders::_1));
}

void missionHandler::onEnd(eventData &event) {
    spdlog::info("End of mission - dumping to disk");

    m_missionEnd = event.eventTime;
    m_readyToDump = true;

    m_server.unsubscribe(potato::packetTypes::GAME_EVENT, m_eventHandlerHandle);
}

void missionHandler::logEvent(const std::vector<std::unique_ptr<potato::baseARMAVariable>> &variables) {
    eventData event(variables);

    switch (event.type) {
        case armaEvents::MISSION_END:
            onEnd(event);
            break;
        default:
            break;
    }
}

missionHandler::missionHandler(dataServer &server, eventData &startEvent, const nlohmann::json &config) :
    c_config(config),
    m_server(server),
    m_eventHandler(server),
    m_objectHandler(server),
    m_projectileHandler(server)
{
    onStart(startEvent);
}

missionHandler::~missionHandler() {
    if (m_dumpThread.joinable()) {
        m_dumpThread.join();
    }
}

void missionHandler::drawInfo() const {
    if (ImGui::BeginTabBar("##ViewTabs")) {
        if (ImGui::BeginTabItem("Mission Information")) {
            ImGui::Text(fmt::format("Date: {}", m_missionDate).c_str());
            ImGui::Text(fmt::format("Mission: {}", m_missionName).c_str());
            ImGui::Text(fmt::format("Map: {}", m_worldName).c_str());
            ImGui::Text(fmt::format("Map Size: {}", m_worldSize).c_str());
            ImGui::EndTabItem();
        }

        m_eventHandler.drawInfo();
        m_projectileHandler.drawInfo();
        m_objectHandler.drawInfo();
        ImGui::EndTabBar();
    }
}

void missionHandler::update() {
    m_projectileHandler.update();
}

void missionHandler::dump() {
    if (!m_dumping) {
        m_dumping = true;
        m_dumpThread = std::thread([this] () {
            dumpToDisk();
            m_readyToDelete = true;
        });
    }
}

bool missionHandler::isDumping() const {
    return m_dumping;
}

bool missionHandler::readyToDump() const {
    return m_readyToDump;
}

bool missionHandler::readyToDelete() const {
    return m_readyToDelete;
}

std::string missionHandler::getMissionName() const {
    return m_missionName;
}
