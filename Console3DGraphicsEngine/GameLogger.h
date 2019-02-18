#pragma once

#include <sstream>
#include <mutex>

//Logging macro
#define LOG(level) \
	if (level > Logger<StdErrPolicy>::LoggingLevel()) ; else Logger<StdErrPolicy>().Get(level) 

enum eLogLevel
{
	None = 0, LogError = 1, LogWarning, LogInfo, LogDebug
};

template <typename OutputPolicy>
class Logger
{
	eLogLevel m_messageLevel;
	static eLogLevel m_loggerLevel;

	Logger(const Logger&);
	Logger& operator = (const Logger&);

	std::string LevelToString(eLogLevel level);

protected:
	std::ostringstream m_outputStream;

public:
	Logger();

	virtual ~Logger();

	std::ostringstream& Get(eLogLevel level = LogInfo);

	static eLogLevel& LoggingLevel() { return m_loggerLevel; }
};


class StdErrPolicy
{
public:
	static void Output(const std::string& msg)
	{
		FILE* pStream = stderr;

		fprintf(pStream, "%s", msg.c_str());
		fflush(pStream);
	}
};

class FilePolicy
{
private:
	static FILE*& StreamImpl();
	static std::mutex m_mutex;

public:
	static void SetStream(FILE* pFile);
	static void Output(const std::string& msg);
};

inline FILE*& FilePolicy::StreamImpl()
{
	static FILE* pStream = stderr;
	return pStream;
}

inline void FilePolicy::SetStream(FILE* pFile)
{
	std::unique_lock<std::mutex> lock(m_mutex);
	FilePolicy::StreamImpl() = pFile;
}

inline void FilePolicy::Output(const std::string& msg)
{
	std::unique_lock<std::mutex> lock(m_mutex);

	FILE* pStream = StreamImpl();

	if (!pStream)
		return;

	fprintf(pStream, "%s", msg.c_str());
	fflush(pStream);
}

