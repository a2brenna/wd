#include "server_config.h"
#include <sys/time.h>
#include <slog/slog.h>
#include <slog/file.h>

#include <chrono>
#include <memory>
#include <mutex>
#include <map>
#include <signal.h>

#include <smpl.h>
#include <smplsocket.h>
#include <thread>
#include <cassert>
#include <fstream>
#include <sstream>
#include <txtable.h>
#include "task.h"
#include "watchdog.pb.h"

#include <iostream>

#include <utility>
#include <boost/algorithm/string.hpp>

std::shared_ptr<std::pair<std::ofstream, std::mutex>> _log(new std::pair<std::ofstream, std::mutex>());
slog::Log CRITICAL(std::shared_ptr<slog::Log_Sink>(new slog::File(_log)), slog::kLogErr, "CRITICAL: ");
slog::Log ERROR(std::shared_ptr<slog::Log_Sink>(new slog::File(_log)), slog::kLogErr, "ERROR: ");
slog::Log INFO(std::shared_ptr<slog::Log_Sink>(new slog::File(_log)), slog::kLogErr, "INFO: ");
slog::Log DEBUG(std::shared_ptr<slog::Log_Sink>(new slog::File(_log)), slog::kLogErr, "DEBUG: ");

typedef std::string Task_Signature;

class Fatal_Error {};

std::mutex tasks_lock;
std::map<Task_Signature, std::shared_ptr<Task_Data>> tasks;

std::pair<Task_Signature, std::chrono::high_resolution_clock::time_point> next_expiration;

double to_seconds(const std::chrono::nanoseconds &ns){
    std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(ns);
    return (ms.count() / 1000.0);
}

void set_timer(const std::chrono::nanoseconds &ns){
    struct itimerval t;
    t.it_interval.tv_sec = 0;
    t.it_interval.tv_usec = 0;
    t.it_value.tv_sec = ns.count() / 1000000000;
    t.it_value.tv_usec = (ns.count() - (t.it_value.tv_sec * 1000000000)) / 1000;
    const auto r = setitimer(ITIMER_REAL, &t, nullptr);
    if(r != 0){
        throw Fatal_Error();
    }
    else{
        return;
    }
}

void unsafe_reset_expiration(){
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

void handle_expiration(){
    std::unique_lock<std::mutex> l(tasks_lock);
    const auto current_time = std::chrono::high_resolution_clock::now();

    if( current_time > next_expiration.second ){
        INFO << "Task: " << next_expiration.first << " expired" << std::endl;
    }
    else{
        ERROR << "Woke up too soon!!" << std::endl;
    }
    unsafe_reset_expiration();
}

void handle_signal(int sig){
    if (sig == SIGALRM){
        handle_expiration();
    }
    else{
        ERROR << "Unhandled signal " << sig << std::endl;
    }
}

std::shared_ptr<Task_Data> get_task(const Task_Signature &sig){
    std::unique_lock<std::mutex> l(tasks_lock);
    std::shared_ptr<Task_Data> task;

    try{
        task = tasks.at(sig);
    }
    catch(std::out_of_range o){
        task = std::shared_ptr<Task_Data>(new Task_Data);
        tasks[sig] = task;
    }
    return task;
}

void handle_beat(const watchdog::Message &request){
    const Task_Signature sig = request.beat().signature();
    std::shared_ptr<Task_Data> task = get_task(sig);

    {
        std::unique_lock<std::mutex> l(task->lock);
        const auto t = task->beat();
        INFO << "BEAT " << sig << " time " << t.time_since_epoch().count() << std::endl;
        unsafe_reset_expiration();
    }
}

void handle_orders(const watchdog::Message &request){
    for(int i = 0; i < request.orders_size(); i++){
        const watchdog::Command &o = request.orders(i);
        for( int j = 0; j < o.to_forget_size(); j++){
            const watchdog::Command::Forget &f = o.to_forget(j);
            std::unique_lock<std::mutex> la(tasks_lock);
            tasks.erase(f.signature());
            INFO << "FORGET " << f.signature() << std::endl;
            unsafe_reset_expiration();
        }
    }
}

watchdog::Message handle_query(const watchdog::Message &request){
    auto r = watchdog::Message();
    auto response = r.mutable_response();
    auto query = request.query();

    if(query.question() == "Dump"){
        //Replace this with a thing that queries the log file... we no longer
        //necessarily store all the data
        /*
        auto task_signature = query.signature();
        auto task_dump = response->mutable_dump();
        try{
            std::unique_lock<std::mutex> l(tasks_lock);
            for(const auto &d: tasks.at(task_signature)->_intervals){
                double i = to_seconds(d);
                task_dump->add_interval(i);
            }
        }
        catch(std::out_of_range e){
            ERROR << "No such task: " << task_signature << std::endl;
        }
        */
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

void handle(std::shared_ptr<smpl::Channel> client){
    for(;;){

        std::string incoming;
        try{
            incoming = client->recv();
        }
        catch(...){
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

void load_log(const std::string &logfile){
    size_t line_number = 0;

    std::ifstream f(logfile, std::ifstream::in);

    while(!f.eof()){
        line_number++;
        std::string line;
        std::getline(f, line);

        std::vector<std::string> tokens;
        split(tokens, line, boost::is_any_of(" "));

        for(auto i = tokens.begin(); i != tokens.end(); i++){
            if(*i == "BEAT"){
                i++;
                const std::string sig = *i;
                i++;
                i++;
                const std::string time = *i;
                const auto from_epoch = std::chrono::high_resolution_clock::duration(strtoull(time.c_str(), nullptr, 10));
                std::chrono::high_resolution_clock::time_point c(from_epoch);

                auto t = get_task(sig);
                try{
                    t->beat(c);
                }
                catch(Bad_Beat b){
                    std::cerr << line_number << " " << f << std::endl;
                }
                break;
            }
            else if(*i == "FORGET"){
                i++;
                const std::string sig = *i;
                std::unique_lock<std::mutex> l(tasks_lock);
                tasks.erase(sig);
            }
        }
    }

    {
        std::unique_lock<std::mutex> l(tasks_lock);
        unsafe_reset_expiration();
    }
}

int main(int argc, char *argv[]){
    get_config(argc, argv);

    std::string log_file = getenv("HOME");
    log_file.append("/.wd.log");

    _log->first.open(log_file, std::ofstream::app);
    INFO << "Watchdog starting..." << std::endl;

    signal(SIGALRM, handle_signal);
    set_timer(std::chrono::nanoseconds::max());

    load_log(log_file);

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
