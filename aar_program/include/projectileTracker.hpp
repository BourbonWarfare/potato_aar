// projectileTracker.hpp
// Tracks current position of all projectiles
#pragma once
#include <unordered_map>
#include <vector>
#include <deque>
#include <memory>
#include <string>
#include "bw/armaTypes.hpp"

#include "nlohmann/json_fwd.hpp"

class dataServer;
class projectileTracker{
    private:
        struct projectile {
            std::string m_classname = "";
            unsigned int m_uid = 0;

            double m_time = 0.0;

            float m_positionX = 0.f;
            float m_positionY = 0.f;
            float m_positionZ = 0.f;

            float m_velocityX = 0.f;
            float m_velocityY = 0.f;
            float m_velocityZ = 0.f;
        };

        std::unordered_map<unsigned int, unsigned int> m_activeProjectiles;
        std::unordered_map<unsigned int, std::deque<projectile>> m_projectiles;

        double m_updateRate = 0.0;

        dataServer &m_server;
        int m_eventID = 0;
        int m_updateProjectileID = 0;

        // look out for projectile created events
        void logEvent(const std::vector<std::unique_ptr<potato::baseARMAVariable>> &variables);
        void updateProjectile(const std::vector<std::unique_ptr<potato::baseARMAVariable>> &variables);

        void drawProjectileInfo(const projectile &projectile) const;

        static constexpr unsigned int m_tickRecycleTime = 5;

        bool m_hasUpdate = false;
        unsigned int m_tick = 0;

    public:
        double updateRate = 0.0;

        projectileTracker(dataServer &server);
        ~projectileTracker();

        void drawInfo() const;

        void update();

        void serialise(struct zip_t *zip) const;
};

