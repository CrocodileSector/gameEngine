#include "GameLogger.h"

std::recursive_mutex StdErrPolicy::m_defaultMutex;
std::recursive_mutex FilePolicy::m_fileMutex;
std::string FilePolicy::m_filePath = sDefaultLogFile + Util::GetTimestamp() + ".txt";

void StdErrPolicy::Output(const std::string& msg)
{
	std::scoped_lock<std::recursive_mutex> lock(m_defaultMutex);

	FILE* pStream = stderr;
	if (!pStream)
		return;

	fprintf(pStream, "%s", msg.c_str());
	fflush(pStream);

	fclose(pStream);
}

FILE*& FilePolicy::StreamImpl()
{
	static FILE* pStream = NULL;

	if (!m_filePath.empty())
		pStream = fopen(m_filePath.c_str(), "a");

	return pStream;
}

void FilePolicy::SetFile(const std::string& pFile)
{
	std::scoped_lock<std::recursive_mutex> lock(m_fileMutex);
	m_filePath = pFile;
}

void FilePolicy::Output(const std::string& msg)
{
	std::scoped_lock<std::recursive_mutex> lock(m_fileMutex);

	FILE* pStream = StreamImpl();

	if (!pStream)
	{
		pStream = fopen(m_filePath.c_str(), "a");

		if (!pStream)
			return;
	}

	fprintf(pStream, "%s", msg.c_str());
	fflush(pStream);

	fclose(pStream);
}