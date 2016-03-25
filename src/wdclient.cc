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
        std::shared_ptr<smpl::Remote_Postbox> server(new smpl::Remote_UDP(CONFIG_SERVER_ADDRESS, CONFIG_INSECURE_PORT));
        beat(server, CONFIG_SIGNATURE);
    }
    else{
        std::shared_ptr<smpl::Channel> server(server_address->connect());
        Heart h(server, CONFIG_SIGNATURE);
        h.beat();
    }

};
