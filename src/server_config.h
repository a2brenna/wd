#ifndef __CONFIG_H__
#define __CONFIG_H__

#include<string>

extern std::string global_config_file;
extern std::string CONFIG_REPORT_FILE;
extern int CONFIG_INSECURE_PORT;
extern void get_config(int ac, char *av[]);

#endif
