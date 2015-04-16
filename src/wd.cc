#include "server_config.h"
#include <hgutil/time.h>
#include <hgutil/log.h>
#include <sys/time.h>

#include <chrono>
#include <syslog.h>
#include <memory>
#include <mutex>
#include <map>
#include <signal.h>

#include <smpl.h>
#include <smplsocket.h>
#include <thread>
#include <functional>
#include <cassert>
#include <fstream>
#include <sstream>
#include <txtable.h>
#include "task.h"
#include "watchdog.pb.h"

#include <iostream>

#include <utility>

std::pair<std::ofstream, std::mutex> _log;
Log CRITICAL(std::shared_ptr<Char_Stream>(new File(&_log, std::string("Critical: "))));
Log ERROR(std::shared_ptr<Char_Stream>(new File(&_log, std::string("Error: "))));
Log INFO(std::shared_ptr<Char_Stream>(new File(&_log, std::string("Info: "))));
Log DEBUG(std::shared_ptr<Char_Stream>(new File(&_log, std::string("Debug: "))));

typedef std::string Task_Signature;

std::mutex tasks_lock;
std::map<Task_Signature, std::shared_ptr<Task_Data>> tasks;

std::mutex expiration_lock;
std::pair<Task_Signature, std::chrono::high_resolution_clock::time_point> next_expiration;

void reset_expiration(){
    std::unique_lock<std::mutex> l(tasks_lock);
    std::pair<Task_Signature, std::shared_ptr<Task_Data>> next;
    if(tasks.empty()){
        return;
    }
    else{
        next = *(tasks.begin());
        for(const auto &t: tasks){
            if( next.second->expected() > t.second->expected() ){
                next = t;
            }
        }
    }
    assert(next.second != nullptr);
    {
        std::unique_lock<std::mutex> l(expiration_lock);
        next_expiration.first = next.first;
        next_expiration.second= next.second->expected();
        if ( next_expiration.second > std::chrono::high_resolution_clock::now()){
            set_timer(next_expiration.second - std::chrono::high_resolution_clock::now());
        }
        else{
            set_timer(std::chrono::high_resolution_clock::duration(0));
        }
    }
}

void expiration(int sig){

    if (sig == SIGALRM){
        //handle potential expiration
        {
            std::unique_lock<std::mutex> l(expiration_lock);
            const auto current_time = std::chrono::high_resolution_clock::now();

            if( current_time > next_expiration.second ){
                INFO << "Task: " << next_expiration.first << " expired" << std::endl;
            }
            else{
                ERROR << "Woke up too soon!!" << std::endl;
            }
        }
        reset_expiration();
    }
    else{
        ERROR << "Unhandled signal " << sig << std::endl;
    }
}

void handle_beat(const watchdog::Message &request){
    std::shared_ptr<Task_Data> task;
    const Task_Signature sig = request.beat().signature();

    {
        std::unique_lock<std::mutex> l(tasks_lock);
        try{
            task = tasks.at(sig);
        }
        catch(std::out_of_range o){
            task = std::shared_ptr<Task_Data>(new Task_Data);
            tasks[sig] = task;
        }
    }

    {
        std::unique_lock<std::mutex> l(task->lock);
        task->beat();
    }

    DEBUG << "Beat: " << sig << std::endl;

    reset_expiration();

}

watchdog::Message handle_query(const watchdog::Message &request){
    auto r = watchdog::Message();
    auto response = r.mutable_response();
    auto query = request.query();

    if(query.question() == "Dump"){
        auto task_signature = query.signature();
        auto task_dump = response->mutable_dump();
        try{
            std::unique_lock<std::mutex> l(tasks_lock);
            for(const auto &d: tasks.at(task_signature)->intervals){
                double i = to_seconds(d);
                task_dump->add_interval(i);
            }
        }
        catch(std::out_of_range e){
            ERROR << "No such task: " << task_signature << std::endl;
        }
    }
    else if( query.question() == "Status"){
        std::unique_lock<std::mutex> l(tasks_lock);
        for (const auto &t: tasks){
            auto task = response->add_task();
            task->set_signature(t.first);
            task->set_last(to_seconds(t.second->last().time_since_epoch()));
            task->set_expected(to_seconds(t.second->expected().time_since_epoch()));
            task->set_mean(to_seconds(t.second->mean()));
            task->set_deviation(to_seconds(t.second->deviation()));
            task->set_time_to_expiration(to_seconds(t.second->to_expiration()));
            task->set_beats(t.second->num_beats());
        }
    }
    else{
        assert(false);
    }

    return r;
}

void handle_orders(const watchdog::Message &request){
    for(int i = 0; i < request.orders_size(); i++){
        const watchdog::Command &o = request.orders(i);
        for( int j = 0; j < o.to_forget_size(); j++){
            const watchdog::Command::Forget &f = o.to_forget(j);
            std::unique_lock<std::mutex> la(tasks_lock);
            tasks.erase(f.signature());
            reset_expiration();
        }
    }
}

void handle(std::shared_ptr<smpl::Channel> client){
    for(;;){

        std::string incoming;
        try{
            incoming = client->recv();
        }
        catch(smpl::Error e){
            break;
        }

        watchdog::Message request;
        request.ParseFromString(incoming);

        if(request.IsInitialized()){

            if(request.has_beat()){
                handle_beat(request);
            }
            else if(request.has_query()){
                watchdog::Message response;
                response = handle_query(request);
                std::string s;
                response.SerializeToString(&s);
                client->send(s);
            }
            else if(request.orders_size() > 0){
                handle_orders(request);
            }
            else{
                ERROR << "Unhandled Request: " << request.DebugString() << std::endl;
            }

        }
        else{
            ERROR << "Uninitialized Message: " << request.DebugString() << std::endl;;
            break;
        }

    }

}

int main(int argc, char *argv[]){
    get_config(argc, argv);


    std::string log_file = getenv("HOME");
    log_file.append("/.wd.log");

    _log.first.open(log_file, std::ofstream::app);
    //openlog("watchdog", LOG_NDELAY, LOG_LOCAL1);
    //setlogmask(LOG_UPTO(LOG_INFO));
    INFO << "Watchdog starting..." << std::endl;

    signal(SIGALRM, expiration);
    set_timer(std::chrono::nanoseconds::max());

    for(;;){
        try{
            std::shared_ptr<smpl::Channel> client(server_address->listen());
            std::function<void()> handler = std::bind(handle, client);
            auto h = std::thread(handler);
            h.detach();
        }
        catch(...){
            std::cerr << "Error accepting connection" << std::endl;
        }
    }

    return 0;
}
