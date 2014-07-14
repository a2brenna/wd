#include "pitbull.h"
#include "watchdog.pb.h"
#include <mutex>
#include <hgutil/time.h>
#include <hgutil/socket.h>
#include <hgutil/fd.h>
#include <hgutil/math.h>
#include <iostream>
#include <chrono>
#include <syslog.h>

void Pitbull::handle(Task *t){
    if(Incoming_Connection *i = dynamic_cast<Incoming_Connection *>(t)){
        for (;;){
            try{
                std::string request;
                recv_string(i->sock, request);

                watchdog::Message m;
                m.ParseFromString(request);

                if(m.IsInitialized()){
                    if(m.has_beat()){
                        handle_beat(m);
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
                    syslog(LOG_ERR, "Uninitialized Message: %s", m.DebugString().c_str());
                    break;
                }
            }
            catch(Network_Error e){
                std::cerr << "Network_Error: " << e.msg << std::endl;
                break;
            }
        }
    }
    else{
        throw Handler_Exception("Bad task");
    }
}

void Pitbull::reset_expiration(){

    std::pair<std::string, std::chrono::high_resolution_clock::time_point> next_expiration;
    next_expiration.second = std::chrono::high_resolution_clock::time_point::max();

    {
        std::lock_guard<std::recursive_mutex> t(tracked_tasks.lock);
        for(auto &t: tracked_tasks.data){
            std::lock_guard<std::recursive_mutex> t_lock(t.second.lock);
            if(t.second.data.e < next_expiration.second){
                next_expiration.first = t.first;
                next_expiration.second = t.second.data.e;
            }
        }
    }

    std::chrono::high_resolution_clock::duration countdown = next_expiration.second - std::chrono::high_resolution_clock::now();
    next = next_expiration.first;

    std::chrono::nanoseconds ns(countdown);

    set_timer(ns);
}

void Pitbull::handle_beat(watchdog::Message m){
    Lockable<Task_Data> *task;
    std::string signature = m.beat().signature();
    {
        std::lock_guard<std::recursive_mutex> t(tracked_tasks.lock);
        task = &(tracked_tasks.data[signature]);
    }

    std::lock_guard<std::recursive_mutex> t(task->lock);
    task->data.beat();

    std::lock_guard<std::recursive_mutex> time_lock(timelock);
    reset_expiration();
}
