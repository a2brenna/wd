#include "client.h"
#include <hgutil/time.h>
#include <hgutil/socket.h>
#include <hgutil/address.h>
#include <hgutil/fd.h>

#include <iostream>

auto CERTFILE = "/home/a2brenna/.ssl/cert.pem";
auto KEYFILE = "/home/a2brenna/.ssl/key.pem";
auto CAFILE = "/home/a2brenna/.ssl/ca-cert.pem";

int main(){

    Socket *server;

    try{
        gnutls_certificate_credentials_t x509_cred = tls_init(KEYFILE, CERTFILE, CAFILE);
        Unix_Domain_Address server_address("/tmp/pb.sock", false);
        server = new Secure_Socket(connect_to(&server_address), false, x509_cred);
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
