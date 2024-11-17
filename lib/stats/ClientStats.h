#pragma once

#include <cstdint>
#include <chrono>

class ClientStats
{
public:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = std::chrono::time_point<Clock>;

    void StartTimer()
    {
        startTime = Clock::now();
    }

    void StopTimer()
    {
        endTime = Clock::now();
        m_TotalRequests++;
        m_TotalElapsedTimeUs += std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime).count();
    }

    uint64_t GetTotalRequests() const
    {
        return m_TotalRequests;
    }

    uint64_t GetAverageElapsedTimeUs() const
    {
        if (m_TotalRequests == 0)
        {
            return 0;
        }
        return m_TotalElapsedTimeUs / m_TotalRequests;
    }

// private:
    TimePoint startTime;
    TimePoint endTime;
    uint64_t m_TotalElapsedTimeUs { 0 };
    uint64_t m_TotalRequests { 0 };

};
