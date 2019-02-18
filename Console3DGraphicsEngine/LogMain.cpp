#include <iostream>

#include "GameLogger.h"

#define LOG_LEVEL eLogLevel::LogDebug

int main()
{
	const int count = 3;

	LOG(LOG_LEVEL) << "A loop with " << count << " iterations";

	for (int i = 0; i != count; ++i)
	{
		Logger<StdErrPolicy>().Get(eLogLevel::LogWarning) << " counter's at " << i;
	}
}