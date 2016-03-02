#include "client_config.h"
#include "client.h"
#include "watchdog.pb.h"
#include <smpl.h>
#include <smplsocket.h>
#include <memory>

int main(int argc, char *argv[]){

    try{
        get_config(argc, argv);
    }
    catch(Config_Error e){
        return 1;
    }

    if(CONFIG_UDP){
        smpl::Remote_UDP server(CONFIG_SERVER_ADDRESS, CONFIG_INSECURE_PORT);

        watchdog::Message m;
        m.mutable_beat()->set_signature(CONFIG_SIGNATURE);

        std::string message;
        m.SerializeToString(&message);

        server.send(message);
    }
    else{
        std::shared_ptr<smpl::Channel> server(server_address->connect());
        Heart h(server, CONFIG_SIGNATURE);
        h.beat();
    }

};
