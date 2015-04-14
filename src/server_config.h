#ifndef __SERVER_CONFIG_H__
#define __SERVER_CONFIG_H__

#include <string>
#include <smpl.h>
#include <memory>
#include "common_config.h"

extern void get_config(int ac, char *av[]);
extern std::shared_ptr<smpl::Local_Address> server_address;

#endif
