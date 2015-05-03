#include "client.h"
#include "watchdog.pb.h"

void Heart::beat(){
    if( (_server != nullptr) && (_id != "") ){
        watchdog::Message m;
        m.mutable_beat()->set_signature(_id);

        std::string message;
        m.SerializeToString(&message);

        _server->send(message);
    }
}
