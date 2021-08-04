// projectileTracker.hpp
// Tracks current position of all projectiles
#pragma once
#include <unordered_map>
#include <vector>
#include <deque>
#include <memory>
#include <string>
#include "bw/armaTypes.hpp"

class dataServer;
class projectileTracker{
    private:
        struct projectile {
            std::string m_classname = "";
            unsigned int m_uid = 0;

            float m_positionX = 0.f;
            float m_positionY = 0.f;
            float m_positionZ = 0.f;

            float m_vectorUpX = 0.f;
            float m_vectorUpY = 0.f;
            float m_vectorUpZ = 0.f;

            float m_vectorDirX = 0.f;
            float m_vectorDirY = 0.f;
            float m_vectorDirZ = 0.f;
        };

        std::unordered_map<unsigned int, unsigned int> m_activeProjectiles;
        std::unordered_map<unsigned int, std::deque<projectile>> m_projectiles;

        // look out for projectile created events
        void logEvent(const std::vector<std::unique_ptr<potato::baseARMAVariable>> &variables);
        void updateProjectile(const std::vector<std::unique_ptr<potato::baseARMAVariable>> &variables);

        void drawProjectileInfo(const projectile &projectile) const;

        static constexpr unsigned int m_tickRecycleTime = 5;

        bool m_hasUpdate = false;
        unsigned int m_tick = 0;

    public:
        projectileTracker(dataServer &server);
        void drawInfo() const;

        void update();
};

