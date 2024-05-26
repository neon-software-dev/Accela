#include <Accela/Common/Timer.h>

#include <sstream>

namespace Accela::Common
{

Timer::Timer(std::string identifier)
    : m_identifier(std::move(identifier))
{
    m_startTime = std::chrono::high_resolution_clock::now();
}

std::chrono::duration<double, std::milli> Timer::StopTimer()
{
    return std::chrono::high_resolution_clock::now() - m_startTime;
}

std::chrono::duration<double, std::milli> Timer::StopTimer(const Common::ILogger::Ptr& logger)
{
    const auto duration = StopTimer();

    std::stringstream ss;
    ss << "[Timer] " << m_identifier << " - " << duration.count() << "ms";

    logger->Log(Common::LogLevel::Debug, ss.str());

    return duration;
}

std::chrono::duration<double, std::milli> Timer::StopTimer(const IMetrics::Ptr& metrics)
{
    const auto duration = StopTimer();

    metrics->SetDoubleValue(m_identifier, duration.count());

    return duration;
}

}