#pragma once
#include <iostream>
#include <sstream>
#include <string>
#include <mutex>
#include <chrono>
#include <ctime>

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
	static inline eLogLevel m_loggerLevel = LogDebug;

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
		std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		m_outputStream << "- " << std::ctime(&now);
		m_outputStream << " " << LevelToString(level) << ": ";
		m_outputStream << std::string(level > eLogLevel::LogDebug ? 0 : level - eLogLevel::LogDebug + "\t");
		m_messageLevel = level;

		return m_outputStream;
	}

	static eLogLevel& LoggingLevel() { return m_loggerLevel; }

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

inline void StdErrPolicy::Output(const std::string& msg)
{
	std::scoped_lock<std::recursive_mutex> lock(m_defaultMutex);

	FILE* pStream = stderr;
	if (!pStream)
		return;

	fprintf(pStream, "%s", msg.c_str());
	fflush(pStream);
}

class FilePolicy 
{
private:
	static FILE*& StreamImpl();
	static std::recursive_mutex m_fileMutex;
public:
	static void SetStream(FILE* pFile);
	static void Output(const std::string& msg);
	static bool StreamExists() { return (StreamImpl()) ? true : false; }

};

inline FILE*& FilePolicy::StreamImpl()
{
	static FILE* pStream = stderr;
	return pStream;
}

inline void FilePolicy::SetStream(FILE* pFile)
{
	std::scoped_lock<std::recursive_mutex> lock(m_fileMutex);
	FilePolicy::StreamImpl() = pFile;
}

inline void FilePolicy::Output(const std::string& msg)
{
	std::scoped_lock<std::recursive_mutex> lock(m_fileMutex);

	FILE* pStream = StreamImpl();

	if (!pStream)
		return;

	fprintf(pStream, "%s", msg.c_str());
	fflush(pStream);
}

typedef Logger<FilePolicy> FileLog;
#define FILE_LOG(level) \
	if (level > FileLog::LoggingLevel() || FilePolicy::StreamExists()) ; \
		else FileLog().Get(level)

typedef Logger<StdErrPolicy> Log;
#define LOG(level) \
	if (level > Log::LoggingLevel()) ; \
		else Log().Get(level) 

