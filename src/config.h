#ifndef __CONFIG_H__
#define __CONFIG_H__

#include<string>
#include<fstream>
#include<stdlib.h>

#include<boost/program_options.hpp>

namespace po = boost::program_options;

std::string global_config_file = "/etc/wd.conf";

int PORT = 7877;

std::string CERTFILE = "/home/a2brenna/.ssl/cert.pem";
std::string KEYFILE = "/home/a2brenna/.ssl/key.pem";
std::string CAFILE = "/home/a2brenna/.ssl/ca-cert.pem";

void get_config(int ac, char *av[]){

    po::options_description desc("Options");
    desc.add_options()
        ("certfile", po::value<std::string>(&CERTFILE), "specify certificate file")
        ("keyfile", po::value<std::string>(&KEYFILE), "specify key file")
        ("cafile", po::value<std::string>(&CAFILE), "specify ca certificate file")
        ("port", po::value<int>(&PORT), "port number")
        ;

    std::ifstream global(global_config_file, std::ios_base::in);
    std::string user_config_file = getenv("HOME");
    std::ifstream user(user_config_file, std::ios_base::in);

    po::variables_map vm;
    po::store(po::parse_config_file(global, desc), vm);
    po::store(po::parse_config_file(user, desc), vm);
    po::store(po::parse_command_line(ac, av, desc), vm);
    po::notify(vm);

    return;
}

#endif
