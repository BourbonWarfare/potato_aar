#include "objectTracker.hpp"
#include "dataServer.hpp"
#include "imgui.h"
#include "spdlog/fmt/fmt.h"

void objectTracker::created(eventData &event) {
    std::string uid;
    std::string classname;
    std::string name;
    float x, y, z;

    event.eventInformation[0]->convert(uid);
    event.eventInformation[1]->convert(classname);
    potato::armaArray &positionInformation = *static_cast<potato::armaArray *>(event.eventInformation[2].get());

    positionInformation.data[0]->convert(x);
    positionInformation.data[1]->convert(y);
    positionInformation.data[2]->convert(z);

    event.eventInformation[3]->convert(name);

    m_objects.emplace(uid, object(classname, uid, name));
    m_objects.at(uid).positionX = x;
    m_objects.at(uid).positionY = y;
    m_objects.at(uid).positionZ = z;
}

void objectTracker::destroyed(eventData &event) {
    std::string uid;
    event.eventInformation[0]->convert(uid);

    if (m_objects.find(uid) != m_objects.end()) {
        m_objects.at(uid).setLifeState(object::lifeState::DEAD);
    }
}

void objectTracker::getIn(eventData &event) {
    std::string uidUnit;
    std::string uidVehicle;
    event.eventInformation[0]->convert(uidUnit);
    event.eventInformation[1]->convert(uidVehicle);

    if (m_objects.find(uidUnit) != m_objects.end() && m_objects.find(uidVehicle) != m_objects.end()) {
        m_objects.at(uidUnit).enter(uidVehicle);
        m_objects.at(uidVehicle).moveIn(uidUnit);
    }
}

void objectTracker::getOut(eventData &event) {
    std::string uidUnit;
    std::string uidVehicle;
    event.eventInformation[0]->convert(uidUnit);
    event.eventInformation[1]->convert(uidVehicle);

    if (m_objects.find(uidUnit) != m_objects.end() && m_objects.find(uidVehicle) != m_objects.end()) {
        m_objects.at(uidUnit).exit();
        m_objects.at(uidVehicle).moveOut(uidUnit);
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
        default:
            break;
    }
}

void objectTracker::updateObject(const std::vector<std::unique_ptr<potato::baseARMAVariable>> &variables) {
    potato::armaArray &objectsToUpdate = *static_cast<potato::armaArray*>(variables[0].get());
    for (auto &objectData : objectsToUpdate.data) {
        potato::armaArray &objectArrayInfo = *static_cast<potato::armaArray*>(objectData.get());

        std::string uid;
        objectArrayInfo.data[0]->convert(uid);

        if (m_objects.find(uid) != m_objects.end()) {
            object &workingObject = m_objects.at(uid);

            potato::armaArray &objectInfo = *static_cast<potato::armaArray*>(objectArrayInfo.data[1].get());

            potato::armaArray &position = *static_cast<potato::armaArray*>(objectInfo.data[0].get());
            potato::armaArray &viewDir = *static_cast<potato::armaArray*>(objectInfo.data[1].get());

            position.data[0]->convert(workingObject.positionX);
            position.data[1]->convert(workingObject.positionY);
            position.data[2]->convert(workingObject.positionZ);

            viewDir.data[0]->convert(workingObject.azimuth);
            viewDir.data[1]->convert(workingObject.pitch);
        }
    }
}

objectTracker::objectTracker(dataServer &server) {
    server.subscribe(potato::packetTypes::GAME_EVENT, std::bind(&objectTracker::logEvent, this, std::placeholders::_1));
    server.subscribe(potato::packetTypes::UPDATE_OBJECT, std::bind(&objectTracker::updateObject, this, std::placeholders::_1));
}

void objectTracker::drawInfo() const {
    if (ImGui::BeginTabItem("Object Tracking")) {
        for (auto &worldObject : m_objects) {
            if (ImGui::TreeNode(worldObject.first.c_str())) {
                ImGui::Text(fmt::format("Classname: {}", worldObject.second.getClassname()).c_str());
                if (worldObject.second.getName() != "") {
                    ImGui::Text(fmt::format("Name: {}", worldObject.second.getName()).c_str());
                }
                ImGui::Text("Position: [%f %f %f]", worldObject.second.positionX, worldObject.second.positionY, worldObject.second.positionZ);
                ImGui::Text("Pitch: %f", worldObject.second.pitch);
                ImGui::Text("Azimuth: %f", worldObject.second.azimuth);

                if (worldObject.second.insideObject()) {
                    ImGui::Text(fmt::format("Inside: {}", worldObject.second.getVehicleIn()).c_str());
                } else {
                    ImGui::Text("Outside");
                }

                if (!worldObject.second.getCargo().empty()) {
                    if (ImGui::TreeNode("Occupants")) {
                        for (auto &occupantID : worldObject.second.getCargo()) {
                            const object &occupant = m_objects.at(occupantID);
                            ImGui::Text(fmt::format("{} | {}", occupantID, occupant.getName()).c_str());
                        }
                        ImGui::TreePop();
                    }
                }

                ImGui::TreePop();
            }
        }
        ImGui::EndTabItem();
    }
}
