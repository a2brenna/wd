#include "pitbull.h"
#include "watchdog.pb.h"
#include <mutex>
#include <hgutil/time.h>
#include <hgutil/socket.h>
#include <hgutil/fd.h>
#include <hgutil/math.h>
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
                        handle_query(m, i);
                    }
                    else if(m.orders_size() > 0){
                        handle_orders(m);
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

void Pitbull::handle_query(watchdog::Message m, Incoming_Connection *i){
    auto r = watchdog::Message();
    auto response = r.mutable_response();
    auto query = m.query();
    if( query.question() == "Dump" ){
        auto t_sig = query.signature();
        try{
            auto dump_data = response->mutable_dump();
            Lockable<Task_Data> *task_data;
            {
                std::lock_guard<std::recursive_mutex> tasks_lock(tracked_tasks.lock);
                task_data = &(tracked_tasks.data.at(t_sig));
            }
            std::lock_guard<std::recursive_mutex> task_lock(task_data->lock);
            for(auto &d: task_data->data.intervals){
                double seconds = to_seconds(d);
                dump_data->add_interval(seconds);
            }
        }
        catch(std::out_of_range e){
            syslog(LOG_ERR, "No such task: %s", t_sig.c_str());
        }
    }
    else if( m.query().question() == "Status" ){
        std::lock_guard<std::recursive_mutex> tasks_lock(tracked_tasks.lock);
        for (auto &t: tracked_tasks.data){
            auto task = response->add_task();
            std::lock_guard<std::recursive_mutex> task_lock(t.second.lock);
            task->set_signature(t.first);
            task->set_last(t.second.data.last());
            task->set_expected(t.second.data.expected());
            task->set_mean(t.second.data.mean());
            task->set_deviation(t.second.data.deviation());
            task->set_time_to_expiration(t.second.data.time_to_expiration());
            task->set_beats(t.second.data.num_beats());
        }
    }

    std::string response_string;
    r.SerializeToString(&response_string);
    send_string(i->sock, response_string);
}

void Pitbull::handle_orders(watchdog::Message m){
    for( int i = 0; i < m.orders_size(); i++){
        const watchdog::Command &o = m.orders(i);
        for( int j = 0; j < o.to_forget_size(); j++){
            const watchdog::Command::Forget &f = o.to_forget(j);
            forget(f.signature());
        }
        for( int j = 0; j < o.to_fail_size(); j++){
            const watchdog::Command::Fail &f = o.to_fail(j);
            fail(f.signature());
        }
    }
}

void Pitbull::forget(std::string to_forget){
    std::lock_guard<std::recursive_mutex> l(tracked_tasks.lock);

    tracked_tasks.data.erase(to_forget);

    //if we're forgetting the thing that was next to expire we need to reset the expiration counter or we'll get a spurious wakeup (rendered harmless by f803b5946e87bdd5aa4498f2e6d8427ba7792e9a)
    if(next == to_forget){
        next.clear();
        reset_expiration();
    }
}

void Pitbull::fail(std::string to_fail){
    std::lock_guard<std::recursive_mutex> l(tracked_tasks.lock);

    Lockable<Task_Data> *t;
    try{
        t = &(tracked_tasks.data.at(to_fail));
    }
    catch (std::out_of_range o){
        syslog(LOG_ERR, "Tried to fail nonexistent Task: %s", to_fail.c_str());
    }

    {
        std::lock_guard<std::recursive_mutex> l(t->lock);
        t->data.mark_as_failed();
    }

    //TODO: Not sure if this is necessary here... might be a good idea to do it whenever we mutate state
    //if we're forgetting the thing that was next to expire we need to reset the expiration counter or we'll get a spurious wakeup (rendered harmless by f803b5946e87bdd5aa4498f2e6d8427ba7792e9a)
    if(next == to_fail ){
        next.clear();
        reset_expiration();
    }
}
