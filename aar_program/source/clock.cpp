#include "clock.hpp"
#include <ctime>

fe::clock::clock()
    {
        restart();
        m_stopped = false;
    }

void fe::clock::restart()
    {
        m_startTime = getTimeSinceEpoch();
    }

void fe::clock::stop(bool value)
    {
        fe::time currentTime = getTimeSinceEpoch();
        m_stopped = value;

        // if the clock is restarted we have to subtract the time the clock has been non-active from start_time
        if (!m_stopped)
            {
                m_offsetTime += m_stopTime - m_startTime;
            }

        m_stopTime = currentTime;
    }

fe::time fe::clock::getTime() const
    {
        if (!m_stopped)
            {
                return getTimeSinceEpoch() - m_startTime - m_offsetTime;
            }

        return m_stopTime - m_startTime;
    }

fe::time fe::clock::getTimeSinceEpoch()
    {
        std::timespec ts;
        int ret = std::timespec_get(&ts, TIME_UTC);
        if (ret == 0)
            {
                return fe::seconds(0);
            }
        return fe::seconds(ts.tv_sec) + fe::microseconds(ts.tv_nsec / 1000);
    }

