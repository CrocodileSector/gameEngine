#include <iostream>

#include "GameLogger.h"

#define LOG_LEVEL eLogLevel::LogDebug

int main()
 {
	int count = 3;

	/*LOG(LOG_LEVEL) << "A loop with " << count << " iterations";

	for (int i = 0; i != count; ++i)
	{
		LOG(LOG_LEVEL) << " counter's at " << i;
	}*/

	FILE* logFile = fopen("test.log", "a");
	FilePolicy::SetStream(logFile);

	FILE_LOG(LOG_LEVEL) << "A loop with " << count << " iterations";

	for (int i = 0; i != count; ++i)
	{
		FILE_LOG(LOG_LEVEL) << " counter's at " << i;
	}
}