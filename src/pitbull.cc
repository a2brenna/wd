#include "pitbull.h"
#include "watchdog.pb.h"
#include <mutex>
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
        //TODO: can't just recv_string off plain socket... TLS magic needs to happen first
        //recv_string(i->sockfd, request);

        watchdog::Message m;
        m.ParseFromString(request);

        if(m.has_beat()){
            Lockable<Task_Data> *task;
            {
                std::lock_guard<std::recursive_mutex> t(tracked_tasks.lock);
                task = &(tracked_tasks.data[m.beat().signature()]);
            }

            std::lock_guard<std::recursive_mutex> t(task->lock);
            task->data.beat();
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
