#include "watchdog.pb.h"
#include "common_config.h"
#include <string>
#include <memory>
#include <smplsocket.h>
#include <iostream>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

std::shared_ptr<smpl::Remote_Address> server_address;
std::string to_dump;
bool status_request;

void get_config(int ac, char *av[]){
    po::options_description desc("Options");
    desc.add_options()
        ("help", "Produce help message")
        ("server_address", po::value<std::string>(&CONFIG_SERVER_ADDRESS), "Network address to connect to")
        ("to_dump", po::value<std::string>(&to_dump), "Task to dump")
        ("status_request", po::bool_switch(&status_request), "Request server status")
        ;

    po::variables_map vm;
    po::store(po::parse_command_line(ac, av, desc), vm);
    po::notify(vm);

    server_address = std::shared_ptr<smpl::Remote_Address>(new smpl::Remote_Port(CONFIG_SERVER_ADDRESS, CONFIG_INSECURE_PORT));

}

int main(int argc, char *argv[]){
    std::cout << "Starting" << std::endl;

    get_config(argc, argv);

    std::shared_ptr<smpl::Channel> server(server_address->connect());

    watchdog::Message m;
        auto query = m.mutable_query();

    if(status_request){
        query->set_question("Status");
    }
    else if(to_dump != ""){
        query->set_question("Dump");
        query->set_signature(to_dump);
    }
    else{
        std::cout << "No valid command" << std::endl;
        return 1;
    }

    std::string s;
    m.SerializeToString(&s);

    server->send(s);
    std::string r = server->recv();

    watchdog::Message response;
    response.ParseFromString(r);

    std::cout << response.DebugString() << std::endl;

    return 0;
}
