#ifndef __CONFIG_H__
#define __CONFIG_H__

#include<string>

extern std::string global_config_file;
extern int SECURE_PORT;
extern int INSECURE_PORT;
extern std::string CERTFILE;
extern std::string KEYFILE;
extern std::string CAFILE;
extern void get_config(int ac, char *av[]);

#endif
