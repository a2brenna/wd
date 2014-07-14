#include "pitbull.h"
#include "watchdog.pb.h"
#include <mutex>
#include <hgutil/time.h>
#include <hgutil/socket.h>
#include <hgutil/fd.h>
#include <hgutil/math.h>
#include <iostream>

double Task_Data::beat(){
    int s = ivals.size();
    while( s > (max_ivals - 1) ){
        ivals.pop_back();
    }

    double current = milli_time() / 1000.0;

    if (last != -1 ){
        ivals.push_front(current - last);
    }

    last = current;
    expiration = last + mean(ivals.begin(), ivals.end()) * 2; //zomg do a better job than this

    return expiration;
}

bool Task_Data::expired(){
    double current = milli_time() / 1000.0;
    if(current > expiration){
        return true;
    }
    return false;
}

int Task_Data::num_beats(){
    return ivals.size();
}

void Pitbull::reset_expiration(){

    std::pair<std::string, double> next_expiration;
    next_expiration.second = std::numeric_limits<double>::max();

    {
        std::lock_guard<std::recursive_mutex> t(tracked_tasks.lock);
        for(auto &t: tracked_tasks.data){
            std::lock_guard<std::recursive_mutex> t_lock(t.second.lock);
            if(t.second.data.expiration < next_expiration.second){
                next_expiration.first = t.first;
                next_expiration.second = t.second.data.expiration;
            }
        }
    }

    auto countdown = next_expiration.second - (milli_time() / 1000);

    next = next_expiration.first;

    std::cerr << "Setting countdown for " << next_expiration.first << " " << set_itimer_countdown(countdown) << std::endl;
}

void Pitbull::handle_beat(watchdog::Message m){
    Lockable<Task_Data> *task;
    std::string signature = m.beat().signature();
    {
        std::lock_guard<std::recursive_mutex> t(tracked_tasks.lock);
        task = &(tracked_tasks.data[signature]);
    }

    std::lock_guard<std::recursive_mutex> t(task->lock);
    double task_expiration = task->data.beat();
    std::cout << "Task: " << signature << " has " << task->data.num_beats() << std::endl;

    std::lock_guard<std::recursive_mutex> time_lock(timelock);
    reset_expiration();
}

void Pitbull::handle(Task *t){
    if(Incoming_Connection *i = dynamic_cast<Incoming_Connection *>(t)){
        for (;;){
            try{
                std::string request;
                recv_string(i->sock, request);

                watchdog::Message m;
                m.ParseFromString(request);

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
