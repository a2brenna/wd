#ifndef __COMMON_CONFIG_H__
#define __COMMON_CONFIG_H__

#include <string>
#include "common_config.h"

class Config_Error {};

extern std::string global_config_file;
extern std::string CONFIG_SERVER_ADDRESS;
extern int CONFIG_INSECURE_PORT;

#endif
