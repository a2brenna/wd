#include "pitbull.h"
#include "watchdog.pb.h"
#include <hgutil/time.h>
#include <hgutil/network.h>

void Task_Data::beat(){
    int s = ivals.size();
    while( s > (max_ivals - 1) ){
        ivals.pop_back();
    }

    auto current = milli_time() / 1000.0;

    if (last != -1 ){
        ivals.push_front(current - last);
    }

    last = current;

    //calculate expiration
    //
    ///update itimer for expiration wakeup

    return;
}

void Pitbull::handle(Task *t){
    if(Incoming_Connection *i = dynamic_cast<Incoming_Connection *>(t)){
        std::string request;
        recv_string(i->sockfd, request);

        watchdog::Message m;
        m.ParseFromString(request);

        if(m.has_beat()){

        }
        else if(m.has_query()){

        }
        else if(m.orders_size() > 0){

        }
        else{
            throw Handler_Exception("Bad Request");
        }

    }
    else{
        throw Handler_Exception("Bad task");
    }
}
