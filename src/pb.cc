#include "pitbull.h"
#include <hgutil/socket.h>
#include <hgutil/fd.h>
#include <memory>
#include <gnutls/gnutls.h>
#include <iostream>

const int PORT = 7877;

auto CERTFILE = "/home/a2brenna/.ssl/cert.pem";
auto KEYFILE = "/home/a2brenna/.ssl/key.pem";
auto CAFILE = "/home/a2brenna/.ssl/ca-cert.pem";

int main(){

    gnutls_certificate_credentials_t x509_cred = tls_init(KEYFILE, CERTFILE, CAFILE);

    Connection_Factory ears{};
    int port1 = listen_on(PORT, false);
    ears.add_socket(port1);

    Pitbull p{};

    for(;;){
        try{
            int next = ears.next_connection();
            std::cerr << "Got incoming" << std::endl;
            std::shared_ptr<Task> t(new Incoming_Connection(new Secure_Socket(next, true, x509_cred)));
            p.queue_task(t);
            p.handle_next();
        }
        catch(Network_Error e){
            std::cerr << "Network_Error: " << e.msg << std::endl;
        }
    }

    return 0;
}
