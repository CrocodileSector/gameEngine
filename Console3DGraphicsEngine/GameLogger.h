#pragma once

#include "Util.h"

#include <iostream>
#include <sstream>
#include <string>
#include <mutex>

const std::string sDefaultLogFile = "game-engine-log-";
const std::string sError = "ERROR";
const std::string sWarning = "WARNING";
const std::string sInfo = "INFO";
const std::string sDebug = "DEBUG";

enum eLogLevel
{
	None = 0, LogError = 1, LogWarning, LogInfo, LogDebug
};

template <typename OutputPolicy>
class Logger
{
	eLogLevel m_messageLevel;
	static const eLogLevel m_loggerLevel = LogDebug;

	Logger(const Logger&);
	Logger& operator = (const Logger&);

	static OutputPolicy m_output;

protected:
	std::ostringstream m_outputStream;

public:

	Logger()
	{

	}

	const std::string& LevelToString(eLogLevel level)
	{
		switch (level)
		{
		case LogError:
			return sError;
		case LogWarning:
			return sWarning;
		case LogInfo:
			return sInfo;
		case LogDebug:
			return sDebug;
		default:
			break;
		}

		return std::string();
	}

	std::ostringstream& Get(eLogLevel level)
	{
		std::string timestamp = Util::GetTimestamp();
		m_outputStream << "- " << timestamp;
		m_outputStream << " " << LevelToString(level) << ": ";
		m_outputStream << std::string(level > eLogLevel::LogDebug ? 0 : level - eLogLevel::LogDebug + "\t");
		m_messageLevel = level;

		return m_outputStream;
	}
	 
	static const eLogLevel& LoggingLevel() { return m_loggerLevel; }

	Logger::~Logger()
	{
		m_outputStream << std::endl;
		OutputPolicy::Output(m_outputStream.str());
	}
};

class StdErrPolicy
{
	static std::recursive_mutex m_defaultMutex;

public:
	static void Output(const std::string& msg);
};

class FilePolicy 
{
private:
	static FILE*& StreamImpl();
	static std::recursive_mutex m_fileMutex;
	static std::string m_filePath;
public:
	static void SetFile(const std::string& pFile);
	static void Output(const std::string& msg);
	static bool StreamExists() 
	{
		if (StreamImpl())
			return true;
		else 
			return false;
	}

};

typedef Logger<FilePolicy> FileLog;
#define FILE_LOG(level) \
	if (level > FileLog::LoggingLevel() /*|| !FilePolicy::StreamExists()*/) ; \
		else FileLog().Get(level)

typedef Logger<StdErrPolicy> Log;
#define LOG(level) \
	if (level > Log::LoggingLevel()) ; \
		else Log().Get(level) 

