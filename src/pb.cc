#include "server_config.h"
#include <hgutil/time.h>
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
                syslog(LOG_INFO, "Task: %s expired", next_expiration.first.c_str());
            }
            else{
                syslog(LOG_ERR, "Woke up too soon!!");
            }
        }
        reset_expiration();
    }
    else if (sig == SIGUSR1){
        //dump report to file
        std::unique_lock<std::mutex> l(tasks_lock);
        std::vector<std::string> report_header = { "Signature", "Last", "Beats", "Expected" };
        Table report(report_header);
        for(const auto &t: tasks){
            const std::string sig = t.first;
            const std::shared_ptr<Task_Data> task = t.second;

            std::string last;
            {
                std::stringstream s;
                s << task->last().time_since_epoch().count();
                last = s.str();
            }

            std::string num_beats;
            {
                std::stringstream s;
                s << task->num_beats();
                num_beats = s.str();
            }

            std::string expected;
            {
                std::stringstream s;
                s << task->expected().time_since_epoch().count();
                expected= s.str();
            }

            std::vector<std::string> row = { sig, last, num_beats, expected };

            report.add_row(row);

        }

        std::fstream fs;
        fs.open (CONFIG_REPORT_FILE, std::fstream::out | std::fstream::trunc);

        fs << report;

    }
    else{
        syslog(LOG_ERR, "Unhandled signal %d", sig);
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

    reset_expiration();

}

void handle_query(const watchdog::Message &request){

}

void handle_orders(const watchdog::Message &request){

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
                handle_query(request);
            }
            else if(request.orders_size() > 0){
                handle_orders(request);
            }
            else{
                syslog(LOG_ERR, "Unhandled Request: %s", request.DebugString().c_str());
            }

        }
        else{
            syslog(LOG_ERR, "Uninitialized Message: %s", request.DebugString().c_str());
            break;
        }

    }

}

int main(int argc, char *argv[]){
    get_config(argc, argv);

    openlog("watchdog", LOG_NDELAY, LOG_LOCAL1);
    setlogmask(LOG_UPTO(LOG_INFO));
    syslog(LOG_INFO, "Watchdog starting...");

    signal(SIGALRM, expiration);
    signal(SIGUSR1, expiration);
    set_timer(std::chrono::nanoseconds::max());

    std::unique_ptr<smpl::Local_Address> incoming(new smpl::Local_Port("127.0.0.1", CONFIG_INSECURE_PORT));

    for(;;){
        try{
            std::shared_ptr<smpl::Channel> client(incoming->listen());
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
