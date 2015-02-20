#include "client.h"
#include <hgutil/time.h>
#include <hgutil/socket.h>
#include <hgutil/address.h>

#include <iostream>

auto CERTFILE = "/home/a2brenna/.ssl/cert.pem";
auto KEYFILE = "/home/a2brenna/.ssl/key.pem";
auto CAFILE = "/home/a2brenna/.ssl/ca-cert.pem";

int main(){

    Socket *server;

    try{
        std::shared_ptr<Address> server_address(new INET_Address("127.0.0.1", 7876, false));
        server = new Raw_Socket(server_address);
    }
    catch(Network_Error e){
        std::cerr << "Initial Network Error: " << e.msg << std::endl;
        return 1;
    }

    Heart h(server, "Test");
    for(;;){
        h.beat();
        sleep(1.0);
    }
    return 0;
}
