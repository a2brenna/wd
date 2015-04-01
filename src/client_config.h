#ifndef __CLIENT_CONFIG_H__
#define __CLIENT_CONFIG_H__

#include "common_config.h"
#include <smpl.h>
#include <memory>
#include <string>

extern std::string CONFIG_SIGNATURE;
extern std::unique_ptr<smpl::Remote_Address> server_address;

void get_config(int ac, char *av[]);

#endif
