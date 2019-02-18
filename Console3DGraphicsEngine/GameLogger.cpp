#include "GameLogger.h"

std::recursive_mutex StdErrPolicy::m_defaultMutex;
std::recursive_mutex FilePolicy::m_fileMutex;