#ifndef __CLIENT_CONFIG_H__
#define __CLIENT_CONFIG_H__

#include "common_config.h"
#include <smpl.h>
#include <memory>
#include <string>

extern std::string CONFIG_SIGNATURE;
extern std::shared_ptr<smpl::Remote_Address> server_address;
extern bool CONFIG_TCP;

void get_config(int ac, char *av[]);

#endif
