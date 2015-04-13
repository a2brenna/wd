#include "client_config.h"
#include "client.h"
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

    std::shared_ptr<smpl::Channel> server(server_address->connect());
    Heart h(server, CONFIG_SIGNATURE);
    h.beat();

};
