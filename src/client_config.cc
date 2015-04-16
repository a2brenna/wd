#include "client_config.h"

#include "common_config.h"

#include <string>
#include <boost/program_options.hpp>
#include <smpl.h>
#include <smplsocket.h>
#include <fstream>

namespace po = boost::program_options;

std::string CONFIG_SIGNATURE = "";
std::shared_ptr<smpl::Remote_Address> server_address;

void get_config(int ac, char *av[]){
    po::options_description desc("Options");
    desc.add_options()
        ("help", "Produce help message")
        ("sig", po::value<std::string>(&CONFIG_SIGNATURE), "The signature to beat with")
        ("port", po::value<int>(&CONFIG_INSECURE_PORT), "Network address to attempt to connect to")
        ("server_address", po::value<std::string>(&CONFIG_SERVER_ADDRESS), "Network address to attempt to connect to")
        ;

    std::ifstream global(global_config_file, std::ios_base::in);

    std::string user_config_file = getenv("HOME");
    user_config_file.append("/.wd.conf");

    std::ifstream user(user_config_file, std::ios_base::in);

    po::variables_map vm;
    po::store(po::parse_config_file(global, desc), vm);
    po::store(po::parse_config_file(user, desc), vm);
    po::store(po::parse_command_line(ac, av, desc), vm);
    po::notify(vm);

    if(CONFIG_SIGNATURE == ""){
        std::cout << "Need to supply a signature" << std::endl;
        throw Config_Error();
    }

    server_address = std::shared_ptr<smpl::Remote_Address>(new smpl::Remote_Port(CONFIG_SERVER_ADDRESS, CONFIG_INSECURE_PORT));

}
