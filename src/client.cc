#include "client.h"
#include "watchdog.pb.h"

void Heart::beat(){
    if( (_server != nullptr) && (_id != "") ){
        watchdog::Message m;
        auto foo = m.mutable_beat();
        foo->set_signature(_id);
        if(_cookie.size() == 32){
            foo->set_cookie(_cookie);
        }

        std::string message;
        m.SerializeToString(&message);

        _server->send(message);
    }
}

void beat(std::shared_ptr<smpl::Remote_Postbox> server, const std::string id){
        watchdog::Message m;
        m.mutable_beat()->set_signature(id);

        std::string message;
        m.SerializeToString(&message);

        server->send(message);
}

void beat(std::shared_ptr<smpl::Remote_Postbox> server, const std::string id, const std::string &cookie){
        assert(cookie.size() == 32);

        watchdog::Message m;
        auto foo = m.mutable_beat();
        foo->set_signature(id);
        foo->set_cookie(cookie);

        std::string message;
        m.SerializeToString(&message);

        server->send(message);
}
