#include "objectTracker.hpp"
#include "dataServer.hpp"
#include "imgui.h"
#include "spdlog/spdlog.h"
#include "nlohmann/json.hpp"
#include "zip.h"

void objectTracker::created(eventData &event) {
    objectInfo newObject;
    float x, y, z;

    event.eventInformation[0]->convert(newObject.m_uid);
    event.eventInformation[1]->convert(newObject.m_classname);
    potato::armaArray &positionInformation = *static_cast<potato::armaArray *>(event.eventInformation[2].get());

    positionInformation.data[0]->convert(x);
    positionInformation.data[1]->convert(y);
    positionInformation.data[2]->convert(z);

    event.eventInformation[3]->convert(newObject.m_realName);

    if (newObject.m_classname == "\"C_man_polo_1_F\"") {
        int i = 0;
    }

    bool isPlayer = static_cast<potato::armaBool *>(event.eventInformation[4].get())->data;

    event.eventInformation[5]->convert(newObject.m_type);

    objectInfo::state startState;

    startState.time = event.eventTime;

    startState.positionX = x;
    startState.positionY = y;
    startState.positionZ = z;
    newObject.m_states.push_back(startState);

    objectInfo::occupantState startOccupation;
    startOccupation.time = event.eventTime;
    newObject.m_occupationStates.push_back(startOccupation);

    m_objects.insert({ newObject.m_uid, std::move(newObject) });
    if (isPlayer) {
        m_players.insert(newObject.m_uid);
    }
}

void objectTracker::destroyed(eventData &event) {
    std::string uid;
    event.eventInformation[0]->convert(uid);

    if (m_objects.find(uid) != m_objects.end()) {
        objectInfo::state endState = m_objects.at(uid).m_states.back();
        endState.time = event.eventTime;
        endState.lifeState = objectInfo::lifeState::DEAD;
        m_objects.at(uid).m_states.push_back(endState);
    }
}

void objectTracker::getIn(eventData &event) {
    std::string uidUnit;
    std::string uidVehicle;
    event.eventInformation[0]->convert(uidUnit);
    event.eventInformation[1]->convert(uidVehicle);

    if (m_objects.find(uidUnit) != m_objects.end() && m_objects.find(uidVehicle) != m_objects.end()) {
        objectInfo::occupantState unitOccupation;
        unitOccupation.time = event.eventTime;
        unitOccupation.m_occupantOf = uidVehicle;
        m_objects.at(uidUnit).m_occupationStates.push_back(unitOccupation);

        objectInfo::occupantState vehicleOccupants = m_objects.at(uidVehicle).m_occupationStates.back();
        vehicleOccupants.time = event.eventTime;
        vehicleOccupants.m_occupants.insert(uidUnit);
        m_objects.at(uidVehicle).m_occupationStates.push_back(vehicleOccupants);
    }
}

void objectTracker::getOut(eventData &event) {
    std::string uidUnit;
    std::string uidVehicle;
    event.eventInformation[0]->convert(uidUnit);
    event.eventInformation[1]->convert(uidVehicle);

    if (m_objects.find(uidUnit) != m_objects.end() && m_objects.find(uidVehicle) != m_objects.end()) {
        objectInfo::occupantState unitOccupation;
        unitOccupation.time = event.eventTime;
        unitOccupation.m_occupantOf = "";
        m_objects.at(uidUnit).m_occupationStates.push_back(unitOccupation);

        objectInfo::occupantState vehicleOccupants = m_objects.at(uidVehicle).m_occupationStates.back();
        vehicleOccupants.time = event.eventTime;
        vehicleOccupants.m_occupants.extract(uidUnit);
        m_objects.at(uidVehicle).m_occupationStates.push_back(vehicleOccupants);
    }
}

void objectTracker::logEvent(const std::vector<std::unique_ptr<potato::baseARMAVariable>> &variables) {
    eventData event(variables);
    switch (event.type) {
        case armaEvents::OBJECT_CREATED:
            created(event);
            break;
        case armaEvents::OBJECT_KILLED:
            destroyed(event);
            break;
        case armaEvents::OBJECT_GET_IN:
            getIn(event);
            break;
        case armaEvents::OBJECT_GET_OUT:
            getOut(event);
            break;
        case armaEvents::MISSION_END:
            m_server.unsubscribe(potato::packetTypes::GAME_EVENT, m_eventID);
            m_server.unsubscribe(potato::packetTypes::UPDATE_OBJECT, m_updateID);
            break;
        default:
            break;
    }
}

void objectTracker::updateObject(const std::vector<std::unique_ptr<potato::baseARMAVariable>> &variables) {
    potato::armaArray &objectsToUpdate = *static_cast<potato::armaArray*>(variables[0].get());

    double time;
    objectsToUpdate.data[0]->convert(time);

    potato::armaArray &objectArrayInfo = *static_cast<potato::armaArray*>(objectsToUpdate.data[1].get());

    for (auto &object : objectArrayInfo.data) {
        potato::armaArray &objectData = *static_cast<potato::armaArray*>(object.get());

        std::string uid;
        objectData.data[0]->convert(uid);

        if (m_objects.find(uid) != m_objects.end()) {
            objectInfo::state updateState;
            updateState.time = time;

            potato::armaArray &objectInfo = *static_cast<potato::armaArray*>(objectData.data[1].get());

            potato::armaArray &position = *static_cast<potato::armaArray*>(objectInfo.data[0].get());
            potato::armaArray &viewDir = *static_cast<potato::armaArray*>(objectInfo.data[1].get());

            position.data[0]->convert(updateState.positionX);
            position.data[1]->convert(updateState.positionY);
            position.data[2]->convert(updateState.positionZ);

            viewDir.data[0]->convert(updateState.azimuth);
            viewDir.data[1]->convert(updateState.pitch);

            m_objects.at(uid).m_states.push_back(updateState);
        }
    }
}

void objectTracker::drawObjectState(const objectInfo::state &state) const {
    ImGui::Text(fmt::format("Last Update {:.2f}", state.time).c_str());
    ImGui::Text(fmt::format("Position [{} {} {}]", state.positionX, state.positionY, state.positionZ).c_str());
    ImGui::Text(fmt::format("Azimuth {}", state.azimuth).c_str());
    ImGui::Text(fmt::format("Pitch {}", state.pitch).c_str());

    switch (state.lifeState) {
        case objectInfo::lifeState::ALIVE:
            ImGui::Text("Life State: ALIVE");
            break;
        case objectInfo::lifeState::UNCONCIOUS:
            ImGui::Text("Life State: UNCONCIOUS");
            break;
        case objectInfo::lifeState::DEAD:
            ImGui::Text("Life State: DEAD");
            break;
        default:
            break;
    }
}

objectTracker::objectTracker(dataServer &server) :
    m_server(server)
{
    m_eventID = server.subscribe(potato::packetTypes::GAME_EVENT, std::bind(&objectTracker::logEvent, this, std::placeholders::_1));
    m_updateID = server.subscribe(potato::packetTypes::UPDATE_OBJECT, std::bind(&objectTracker::updateObject, this, std::placeholders::_1));
}

objectTracker::~objectTracker() {
    m_server.unsubscribe(potato::packetTypes::GAME_EVENT, m_eventID);
    m_server.unsubscribe(potato::packetTypes::UPDATE_OBJECT, m_updateID);
}

void objectTracker::drawInfo() const {
    if (ImGui::BeginTabItem("Object Tracking")) {
        ImGui::BeginMenuBar();
        ImGui::Text(fmt::format("Object Count: {}", m_objects.size()).c_str());
        ImGui::EndMenuBar();

        for (auto &worldObject : m_objects) {
            if (ImGui::TreeNode(worldObject.first.c_str())) {
                ImGui::Text(fmt::format("Classname: {}", worldObject.second.m_classname).c_str());
                ImGui::Text(fmt::format("Name: {}", worldObject.second.m_realName).c_str());

                ImGui::Text("Latest State");
                drawObjectState(worldObject.second.m_states.back());

                if (ImGui::TreeNode("States")) {
                    for (auto it = worldObject.second.m_states.rbegin(); it != worldObject.second.m_states.rend(); ++it) {
                        drawObjectState(*it);
                        ImGui::Separator();
                    }
                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Occupation States")) {
                    for (auto it = worldObject.second.m_occupationStates.rbegin(); it != worldObject.second.m_occupationStates.rend(); ++it) {
                        const objectInfo::occupantState &occupantState = *it;

                        ImGui::Text(fmt::format("Last Update {}", occupantState.time).c_str());
                        ImGui::Text("Occupants");
                        for (auto &occupant : occupantState.m_occupants) {
                            ImGui::Text(occupant.c_str());
                        }

                        ImGui::Text(fmt::format("Occupant Of {}", occupantState.m_occupantOf).c_str());

                        ImGui::Separator();
                    }
                    ImGui::TreePop();
                }

                ImGui::TreePop();
            }
        }
        ImGui::EndTabItem();
    }
}

void objectTracker::serialise(struct zip_t *zip) const {
    for (auto &object : m_objects) {
        nlohmann::json workingObject;

        workingObject["uid"] = object.first;
        workingObject["classname"] = object.second.m_classname;
        workingObject["realName"] = object.second.m_realName;
        workingObject["type"] = object.second.m_type;

        bool firstState = true;
        float lastX = 0.f;
        float lastY = 0.f;
        float lastZ = 0.f;

        std::vector<nlohmann::json> updateStates;
        for (auto &state : object.second.m_states) {
            bool relevant = false;

            nlohmann::json stateJSON;
            stateJSON["time"] = state.time;
            if (firstState) {
                relevant = true;
                firstState = false;
            } else {
                float dx = lastX - state.positionX;
                float dy = lastY - state.positionY;
                float dz = lastZ - state.positionZ;

                float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
                relevant = distance > 1.f;
            }

            if (!relevant) { continue; }

            lastX = state.positionX;
            lastY = state.positionY;
            lastZ = state.positionZ;

            stateJSON["position"] = { state.positionX, state.positionY, state.positionZ };
            stateJSON["azimuth"] = state.azimuth;
            stateJSON["pitch"] = state.pitch;
            stateJSON["lifeState"] = state.lifeState;

            updateStates.push_back(stateJSON);
        }
        workingObject["generalStates"] = updateStates;

        std::vector<nlohmann::json> occupationStates;
        for (auto &state : object.second.m_occupationStates) {
            nlohmann::json stateJSON;
            stateJSON["time"] = state.time;
            stateJSON["occupantOf"] = state.m_occupantOf;
            stateJSON["occupants"] = state.m_occupants;
            occupationStates.push_back(stateJSON);
        }
        workingObject["occupationStates"] = occupationStates;

        std::string filename = object.first;
        filename.erase(std::remove(filename.begin(), filename.end(), '"'), filename.end());
        std::replace(filename.begin(), filename.end(), ':', '_');

        spdlog::info("Saving object state to {}.bson", filename);

        zip_entry_open(zip, fmt::format("{}.bson", filename).c_str());

        std::vector<std::uint8_t> bson = nlohmann::json::to_bson(workingObject);
        zip_entry_write(zip, bson.data(), bson.size());

        zip_entry_close(zip);
    }
}

unsigned int objectTracker::playerCount() const {
    return m_players.size();
}

