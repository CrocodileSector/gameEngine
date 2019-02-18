#include "GameLogger.h"

#include <iostream>
#include <string>
#include <chrono>
#include <ctime>

eLogLevel Logger<StdErrPolicy>::m_loggerLevel = eLogLevel::LogError;

template <typename OutputPolicy>
Logger::Logger()
{

}
template <typename OutputPolicy>
std::string Logger::LevelToString(eLogLevel level)
{
	std::string sLevel;
	switch (level)
	{
	case LogError:
		sLevel = std::string("ERROR");
		break;
	case LogWarning:
		sLevel = std::string("WARNING");
		break;
	case LogInfo:
		sLevel = std::string("INFO");
		break;
	case LogDebug:
		sLevel = std::string("DEBUG");
		break;
	default:
		break;
	}

	return sLevel;
}

template <typename OutputPolicy>
std::ostringstream& Logger::Get(eLogLevel level)
{
	std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	m_outputStream << "- " << std::ctime(&now);
	m_outputStream << " " << LevelToString(level) << ": ";
	m_outputStream << std::string(level > eLogLevel::LogDebug ? 0 : level - eLogLevel::LogDebug +  "\t");
	m_messageLevel = level;

	return m_outputStream;
}

template <typename OutputPolicy>
Logger::~Logger()
{
	m_outputStream << std::endl;
	OutputPolicy::Output(m_outputStream.str());

	fprintf(stderr, m_outputStream.str().c_str());
	fflush(stderr);
}

