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
std::string to_forget;
bool status_request;

void get_config(int ac, char *av[]){
    po::options_description desc("Options");
    desc.add_options()
        ("help", "Produce help message")
        ("server_address", po::value<std::string>(&CONFIG_SERVER_ADDRESS), "Network address to connect to")
        ("dump", po::value<std::string>(&to_dump), "Task to dump")
        ("status_request", po::bool_switch(&status_request), "Request server status")
        ("forget", po::value<std::string>(&to_forget), "Task to forget")
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

    bool awaiting_response = false;

    if(status_request){
        auto query = m.mutable_query();
        query->set_question("Status");
        awaiting_response = true;
    }
    else if(to_dump != ""){
        auto query = m.mutable_query();
        query->set_question("Dump");
        query->set_signature(to_dump);
        awaiting_response = true;
    }
    else if(to_forget != ""){
        auto command = m.add_orders();
        auto f = command->add_to_forget();
        f->set_signature(to_forget);
        awaiting_response = false;
    }
    else{
        std::cout << "No valid command" << std::endl;
        return 1;
    }

    std::string s;
    m.SerializeToString(&s);

    server->send(s);
    if( awaiting_response ){
        std::string r = server->recv();

        watchdog::Message response;
        response.ParseFromString(r);

        if(status_request){
            //print table
            const std::vector<std::string> headings = { "Signature", "Last Seen", "Expected", "Mean", "Deviation", "Time To Expiration", "Beats" };
            Table table(headings);
            for(const auto t: response.response().task()){
                const std::string s = t.signature();
                const auto current_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count() / 1000.0;
                const std::string l = std::to_string(t.last()) + " (" + std::to_string( current_time - t.last()) + ")";
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
    }

    return 0;
}
