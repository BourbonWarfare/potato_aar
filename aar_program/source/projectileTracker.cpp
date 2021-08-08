#include "projectileTracker.hpp"
#include "dataServer.hpp"
#include "armaEvents.hpp"
#include "imgui.h"
#include "spdlog/fmt/fmt.h"
#include "nlohmann/json.hpp"
#include "zip.h"
#include <stack>

void projectileTracker::logEvent(const std::vector<std::unique_ptr<potato::baseARMAVariable>> &variables) {
    eventData event(variables);
    if (event.type == armaEvents::FIRED) {
        unsigned int uid = -1;

        projectile newProjectile;
        newProjectile.m_time = event.eventTime;

        event.eventInformation[0]->convert(uid);
        newProjectile.m_uid = static_cast<unsigned int>(uid);

        potato::armaArray &position = *static_cast<potato::armaArray*>(event.eventInformation[1].get());
        potato::armaArray &velocity = *static_cast<potato::armaArray*>(event.eventInformation[2].get());

        position.data[0]->convert(newProjectile.m_positionX);
        position.data[1]->convert(newProjectile.m_positionY);
        position.data[2]->convert(newProjectile.m_positionZ);

        velocity.data[0]->convert(newProjectile.m_velocityX);
        velocity.data[1]->convert(newProjectile.m_velocityY);
        velocity.data[2]->convert(newProjectile.m_velocityZ);

        event.eventInformation[3]->convert(newProjectile.m_classname);
        m_projectiles[uid].push_back(newProjectile);

        m_activeProjectiles[uid] = m_tick;
    } else if (event.type == armaEvents::MISSION_END) {
        m_server.unsubscribe(potato::packetTypes::GAME_EVENT, m_eventID);
        m_server.unsubscribe(potato::packetTypes::UPDATE_PROJECTILE, m_updateProjectileID);
    }
}

void projectileTracker::updateProjectile(const std::vector<std::unique_ptr<potato::baseARMAVariable>> &variables) {
    potato::armaArray &projectilesToUpdate = *static_cast<potato::armaArray *>(variables[0].get());
    for (auto &projectileData : projectilesToUpdate.data) {
        potato::armaArray &projectileArrayInfo = *static_cast<potato::armaArray*>(projectileData.get());

        double time = 0.0;
        double uid = 0.0;

        projectileArrayInfo.data[0]->convert(time);
        projectileArrayInfo.data[1]->convert(uid);

        unsigned int realUID = static_cast<unsigned int>(uid);
        if (m_projectiles.find(realUID) != m_projectiles.end()) {
            projectile updatedProjectile = m_projectiles.at(realUID).back();

            updatedProjectile.m_time = time;

            potato::armaArray &position = *static_cast<potato::armaArray*>(projectileArrayInfo.data[2].get());
            potato::armaArray &velocity = *static_cast<potato::armaArray*>(projectileArrayInfo.data[3].get());

            position.data[0]->convert(updatedProjectile.m_positionX);
            position.data[1]->convert(updatedProjectile.m_positionY);
            position.data[2]->convert(updatedProjectile.m_positionZ);

            velocity.data[0]->convert(updatedProjectile.m_velocityX);
            velocity.data[1]->convert(updatedProjectile.m_velocityY);
            velocity.data[2]->convert(updatedProjectile.m_velocityZ);

            m_projectiles.at(realUID).push_back(updatedProjectile);
            m_activeProjectiles[uid] = m_tick;
        }
    }
    m_hasUpdate = true;
}

void projectileTracker::drawProjectileInfo(const projectile &projectile) const {
    ImGui::Text(fmt::format("Time: {}", projectile.m_time).c_str());
    ImGui::Text(fmt::format("Position: [{} {} {}]", projectile.m_positionX, projectile.m_positionY, projectile.m_positionZ).c_str());
    ImGui::Text(fmt::format("Velocity: [{} {} {}]", projectile.m_velocityX, projectile.m_velocityY, projectile.m_velocityZ).c_str());
}

projectileTracker::projectileTracker(dataServer &server) :
    m_server(server)
{
	m_eventID = server.subscribe(potato::packetTypes::GAME_EVENT, std::bind(&projectileTracker::logEvent, this, std::placeholders::_1));
    m_updateProjectileID = server.subscribe(potato::packetTypes::UPDATE_PROJECTILE, std::bind(&projectileTracker::updateProjectile, this, std::placeholders::_1));
}

projectileTracker::~projectileTracker() {
    m_server.unsubscribe(potato::packetTypes::GAME_EVENT, m_eventID);
    m_server.unsubscribe(potato::packetTypes::UPDATE_PROJECTILE, m_updateProjectileID);
}

void projectileTracker::drawInfo() const {
    if (ImGui::BeginTabItem("Projectile Tracking")) {
        ImGui::BeginMenuBar();
        ImGui::Text(fmt::format("Projectile Count: {}", m_projectiles.size()).c_str());
        ImGui::EndMenuBar();
        if (ImGui::BeginTabBar("##ProjectileTabs")) {
            if (ImGui::BeginTabItem("All Projectiles")) {
                for (auto &projectile : m_projectiles) {
                    if (!projectile.second.empty() && ImGui::TreeNode(reinterpret_cast<void*>(projectile.first), projectile.second.back().m_classname.c_str())) {
                        for (auto &variant : projectile.second) {
                            drawProjectileInfo(variant);
                        }
                        ImGui::TreePop();
                    }
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Active Projectiles")) {
                ImGui::Text("Current Tick: %d", m_tick);
                for (auto &active : m_activeProjectiles) {
                    projectile projectile = m_projectiles.at(active.first).back();
                    if (ImGui::TreeNode(reinterpret_cast<void*>(active.first), projectile.m_classname.c_str())) {
                        ImGui::Text("Last Tick Updated: %d", active.second);
                        drawProjectileInfo(projectile);
                        ImGui::TreePop();
                    }
                }
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::EndTabItem();
    }
}

void projectileTracker::update()
    {
        if (m_hasUpdate) {
            m_hasUpdate = false;
            m_tick++;
        }

        std::stack<unsigned int> toRemove;
        for (auto &projectile : m_activeProjectiles) {
            if (m_tick >= projectile.second + m_tickRecycleTime) {
                toRemove.push(projectile.first);
            }
        }

        while (!toRemove.empty()) {
            m_activeProjectiles.erase(toRemove.top());
            toRemove.pop();
        }
    }

void projectileTracker::serialise(struct zip_t *zip) const {
    {
        nlohmann::json teenyProjectiles;
        teenyProjectiles["count"] = m_projectiles.size();

        nlohmann::json allProjectiles;
        allProjectiles["count"] = m_projectiles.size();
        for (auto &projectile : m_projectiles) {
            double start = projectile.second.front().m_time;
            double end = projectile.second.back().m_time;
            double lifetime = std::max(end - start, updateRate); // if the projectile has 0 recorded lifetime, just go with the smallest known value

            const std::string uid = std::to_string(projectile.first);

            teenyProjectiles[uid]["classname"] = projectile.second.front().m_classname;
            teenyProjectiles[uid]["lifetime"] = lifetime;
            teenyProjectiles[uid]["positionStart"] = {
                projectile.second.front().m_positionX,
                projectile.second.front().m_positionY,
                projectile.second.front().m_positionZ,
            };
            teenyProjectiles[uid]["positionEnd"] = {
                projectile.second.back().m_positionX,
                projectile.second.back().m_positionY,
                projectile.second.back().m_positionZ,
            };

            teenyProjectiles[uid]["velocityStart"] = {
                projectile.second.front().m_velocityX,
                projectile.second.front().m_velocityY,
                projectile.second.front().m_velocityZ,
            };
            teenyProjectiles[uid]["velocityEnd"] = {
                projectile.second.back().m_velocityX,
                projectile.second.back().m_velocityY,
                projectile.second.back().m_velocityZ,
            };

            std::vector<nlohmann::json> updateData;
            for (auto &update : projectile.second) {
                nlohmann::json updateJSON;

                updateJSON["time"] = update.m_time;
                updateJSON["position"] = {
                    update.m_positionX,
                    update.m_positionY,
                    update.m_positionZ
                };

                updateJSON["velocity"] = {
                    update.m_velocityX,
                    update.m_velocityY,
                    update.m_velocityZ
                };

                updateData.push_back(updateJSON);
            }
            allProjectiles[uid]["updates"] = updateData;
            allProjectiles[uid]["classname"] = projectile.second.back().m_classname;
        }

        std::vector<std::uint8_t> bsonTeeny = nlohmann::json::to_bson(teenyProjectiles);
        zip_entry_open(zip, "projectilesTeeny.bson");
        zip_entry_write(zip, bsonTeeny.data(), bsonTeeny.size());
        zip_entry_close(zip);

        std::vector<std::uint8_t> bson = nlohmann::json::to_bson(allProjectiles);
        zip_entry_open(zip, "projectiles.bson");
        zip_entry_write(zip, bson.data(), bson.size());
        zip_entry_close(zip);
    }

}
