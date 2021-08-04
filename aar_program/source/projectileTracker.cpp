#include "projectileTracker.hpp"
#include "dataServer.hpp"
#include "armaEvents.hpp"
#include "imgui.h"
#include "spdlog/fmt/fmt.h"
#include <stack>

void projectileTracker::logEvent(const std::vector<std::unique_ptr<potato::baseARMAVariable>> &variables) {
    eventData event(variables);
    if (event.type == armaEvents::FIRED) {
        unsigned int uid = -1;

        projectile newProjectile;
        event.eventInformation[0]->convert(uid);
        newProjectile.m_uid = static_cast<unsigned int>(uid);

        potato::armaArray &pos = *static_cast<potato::armaArray*>(event.eventInformation[1].get());
        potato::armaArray &up = *static_cast<potato::armaArray*>(event.eventInformation[2].get());
        potato::armaArray &dir = *static_cast<potato::armaArray*>(event.eventInformation[3].get());

        pos.data[0]->convert(newProjectile.m_positionX);
        pos.data[1]->convert(newProjectile.m_positionY);
        pos.data[2]->convert(newProjectile.m_positionZ);

        dir.data[0]->convert(newProjectile.m_vectorDirX);
        dir.data[1]->convert(newProjectile.m_vectorDirY);
        dir.data[2]->convert(newProjectile.m_vectorDirZ);

        up.data[0]->convert(newProjectile.m_vectorUpX);
        up.data[1]->convert(newProjectile.m_vectorUpY);
        up.data[2]->convert(newProjectile.m_vectorUpZ);

        event.eventInformation[4]->convert(newProjectile.m_classname);
        m_projectiles[uid].push_back(newProjectile);

        m_activeProjectiles[uid] = m_tick;
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

            potato::armaArray &pos = *static_cast<potato::armaArray*>(projectileArrayInfo.data[2].get());
            potato::armaArray &up = *static_cast<potato::armaArray*>(projectileArrayInfo.data[3].get());
            potato::armaArray &dir = *static_cast<potato::armaArray*>(projectileArrayInfo.data[4].get());

            pos.data[0]->convert(updatedProjectile.m_positionX);
            pos.data[1]->convert(updatedProjectile.m_positionY);
            pos.data[2]->convert(updatedProjectile.m_positionZ);

            dir.data[0]->convert(updatedProjectile.m_vectorDirX);
            dir.data[1]->convert(updatedProjectile.m_vectorDirY);
            dir.data[2]->convert(updatedProjectile.m_vectorDirZ);

            up.data[0]->convert(updatedProjectile.m_vectorUpX);
            up.data[1]->convert(updatedProjectile.m_vectorUpY);
            up.data[2]->convert(updatedProjectile.m_vectorUpZ);

            m_projectiles.at(realUID).push_back(updatedProjectile);
            m_activeProjectiles[uid] = m_tick;
        }
    }
    m_hasUpdate = true;
}

void projectileTracker::drawProjectileInfo(const projectile &projectile) const {
    ImGui::Text(fmt::format("UID: {}", projectile.m_uid).c_str());
    ImGui::Text(fmt::format("Position: [{} {} {}]", projectile.m_positionX, projectile.m_positionY, projectile.m_positionZ).c_str());
    ImGui::Text(fmt::format("Vector Dir: [{} {} {}]", projectile.m_vectorDirX, projectile.m_vectorDirY, projectile.m_vectorDirZ).c_str());
    ImGui::Text(fmt::format("Vector Up: [{} {} {}]", projectile.m_vectorUpX, projectile.m_vectorUpY, projectile.m_vectorUpZ).c_str());
}

projectileTracker::projectileTracker(dataServer &server) {
    server.subscribe(potato::packetTypes::GAME_EVENT, std::bind(&projectileTracker::logEvent, this, std::placeholders::_1));
    server.subscribe(potato::packetTypes::UPDATE_PROJECTILE, std::bind(&projectileTracker::updateProjectile, this, std::placeholders::_1));
}

void projectileTracker::drawInfo() const {
    if (ImGui::BeginTabItem("Projectile Tracking")) {
        if (ImGui::BeginTabBar("##ProjectileTabs")) {
            if (ImGui::BeginTabItem("All Projectiles")) {
                for (auto &projectile : m_projectiles) {
                    if (ImGui::TreeNode(reinterpret_cast<void*>(projectile.first), projectile.second.back().m_classname.c_str())) {
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
