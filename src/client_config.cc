#include "client_config.h"

#include <string>
#include <boost/program_options.hpp>
#include <smpl.h>
#include <smplsocket.h>
#include <iostream>

namespace po = boost::program_options;

std::string CONFIG_SIGNATURE = "";
std::unique_ptr<smpl::Remote_Address> server_address(new Remote_Port("happiestface.convextech.ca", CONFIG_INSECURE_PORT));

void get_config(int ac, char *av[]){
    po::options_description desc("Options");
    desc.add_options()
        ("help", "Produce help message")
        ("sig", po::value<std::string>(&CONFIG_SIGNATURE), "The signature to beat with")
        ;

    po::variables_map vm;
    po::store(po::parse_command_line(ac, av, desc), vm);
    po::notify(vm);

    if(CONFIG_SIGNATURE == ""){
        std::cout << "Need to supply a signature" << std::endl;
        throw Config_Error();
    }

}
