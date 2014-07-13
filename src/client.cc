#include "client.h"
#include "watchdog.pb.h"

void Heart::beat(){
    watchdog::Message m;
    m.mutable_beat()->set_signature(id);

    std::string message;
    m.SerializeToString(&message);

    send_string(server, message);
}
