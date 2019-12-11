#pragma once

#include "easylogging++.h"


#define CONFIGURE_LOGGER(filename) \
	el::Loggers::reconfigureAllLoggers(el::ConfigurationType::Filename, "Log\\" filename); \
	el::Loggers::reconfigureAllLoggers(el::Level::Error, el::ConfigurationType::Format, "%datetime %level [%logger]: %func %msg"); \
	el::Loggers::reconfigureAllLoggers(el::ConfigurationType::ToStandardOutput, "false");
