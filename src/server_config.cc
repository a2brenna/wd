#include "server_config.h"

#include<string>
#include<fstream>
#include<stdlib.h>
#include<smplsocket.h>

#include<boost/program_options.hpp>

namespace po = boost::program_options;

std::shared_ptr<smpl::Local_Address> server_address;

void get_config(int ac, char *av[]){

    po::options_description desc("Options");
    desc.add_options()
        ("insecure_port", po::value<int>(&CONFIG_INSECURE_PORT), "port number")
        ("server_address", po::value<std::string>(&CONFIG_SERVER_ADDRESS), "Network address to listen for connections on")
        ;

    std::ifstream global(global_config_file, std::ios_base::in);
    std::string user_config_file = getenv("HOME");
    std::ifstream user(user_config_file, std::ios_base::in);

    po::variables_map vm;
    po::store(po::parse_config_file(global, desc), vm);
    po::store(po::parse_config_file(user, desc), vm);
    po::store(po::parse_command_line(ac, av, desc), vm);
    po::notify(vm);

    server_address = std::shared_ptr<smpl::Local_Address>(new smpl::Local_Port(CONFIG_SERVER_ADDRESS, CONFIG_INSECURE_PORT));

    return;
}
