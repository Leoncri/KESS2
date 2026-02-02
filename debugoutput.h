#pragma once
#include <iostream>

#include "config.h"

#if DEBUG == 1
#define OUTPUT_DEBUG_MESSAGE(s) std::cout << "[DEBUG]: " << s << std::endl;
#else
#define OUTPUT_DEBUG_MESSAGE(s) {}
#endif

#define OUTPUT_INFO(s) std::cout << "[INFO]: " << s << std::endl;

#define OUTPUT_ERROR(s) std::cout << "[ERROR]: " << s << std::endl;