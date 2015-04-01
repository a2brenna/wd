#include "client.h"
#include <smpl.h>
#include <smplsocket.h>
#include <thread>
#include <chrono>

int main(){

    std::unique_ptr<smpl::Remote_Address> server_address(new Remote_Port("127.0.0.1", 7876));
    std::shared_ptr<smpl::Channel> server(server_address->connect());

    Heart h(server, "Test");
    for(;;){
        h.beat();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return 0;
}
