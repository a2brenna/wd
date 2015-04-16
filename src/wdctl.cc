#include "watchdog.pb.h"
#include "common_config.h"
#include <txtable.h>
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
        ("dump", po::value<std::string>(&to_dump), "Task to dump")
        ("status_request", po::bool_switch(&status_request), "Request server status")
        ;

    po::variables_map vm;
    po::store(po::parse_command_line(ac, av, desc), vm);
    po::notify(vm);

    server_address = std::shared_ptr<smpl::Remote_Address>(new smpl::Remote_Port(CONFIG_SERVER_ADDRESS, CONFIG_INSECURE_PORT));

}

int main(int argc, char *argv[]){
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

    if(status_request){
        //print table
        const std::vector<std::string> headings = { "Signature", "Last Seen", "Expected", "Mean", "Deviation", "Time To Expiration", "Beats" };
        Table table(headings);
        for(const auto t: response.response().task()){
            const std::string s = t.signature();
            const std::string l = std::to_string(t.last());
            const std::string e = std::to_string(t.expected());
            const std::string m = std::to_string(t.mean());
            const std::string d = std::to_string(t.deviation());
            const std::string ttl = std::to_string(t.time_to_expiration());
            const std::string b = std::to_string(t.beats());

            table.add_row({s,l,e,m,d,ttl,b});

        }
        std::cout << table << std::endl;
    }
    else if(to_dump != ""){
        std::cout << response.DebugString() << std::endl;
    }
    else{
        std::cout << "No valid command" << std::endl;
        return 1;
    }

    return 0;
}
