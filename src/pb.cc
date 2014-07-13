#include "pitbull.h"
#include <hgutil/network.h>
#include <memory>

const int PORT = 7877;
const int HEARTRATE = 60;
const int MAX_ATTEMPTS = 5;

int main(){

    Connection_Factory ears{};
    ears.add_socket( listen_on(PORT, false) );

    Pitbull p{};

    for(;;){
        std::shared_ptr<Task> t(new Incoming_Connection(ears.next_connection()));
        p.queue_task(t);
        p.handle_next();
    }

    return 0;
}
