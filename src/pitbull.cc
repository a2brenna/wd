#include "pitbull.h"
#include "watchdog.pb.h"
#include <mutex>
#include <hgutil/time.h>
#include <hgutil/socket.h>
#include <hgutil/fd.h>
#include <hgutil/math.h>
#include <iostream>
#include <sys/time.h>

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

void Pitbull::handle_beat(watchdog::Message m){
    std::cerr << "Parsed incoming beat" << std::endl;
    Lockable<Task_Data> *task;
    {
        std::lock_guard<std::recursive_mutex> t(tracked_tasks.lock);
        task = &(tracked_tasks.data[m.beat().signature()]);
    }

    std::lock_guard<std::recursive_mutex> t(task->lock);
    double exp = task->data.beat();
    std::cout << "Task: " << m.beat().signature() << " has " << task->data.num_beats() << std::endl;

    std::lock_guard<std::recursive_mutex> time_lock(timelock);
    struct itimerval current_expiration;
    getitimer(ITIMER_REAL, &current_expiration);

    struct timeval tasks_expiration = seconds_to_timeval(exp - milli_time());

    if(current_expiration.it_value < tasks_expiration){
        struct itimerval next;
        next.it_interval = tasks_expiration;
        setitimer(ITIMER_REAL, &next, NULL);
    }
}

void Pitbull::handle(Task *t){
    std::cerr << "Got incoming client" << std::endl;
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
