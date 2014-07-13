#include "pitbull.h"
#include "watchdog.pb.h"
#include <mutex>
#include <hgutil/time.h>
#include <hgutil/socket.h>
#include <hgutil/fd.h>
#include <iostream>

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
    std::cerr << "Got incoming client" << std::endl;
    if(Incoming_Connection *i = dynamic_cast<Incoming_Connection *>(t)){
        for (;;){
            try{
                std::string request;
                recv_string(i->sock, request);

                watchdog::Message m;
                m.ParseFromString(request);

                if(m.has_beat()){
                    std::cerr << "Parsed incoming beat" << std::endl;
                    Lockable<Task_Data> *task;
                    {
                        std::lock_guard<std::recursive_mutex> t(tracked_tasks.lock);
                        task = &(tracked_tasks.data[m.beat().signature()]);
                    }

                    std::lock_guard<std::recursive_mutex> t(task->lock);
                    task->data.beat();
                    std::cout << "Task: " << m.beat().signature() << " has " << task->data.num_beats() << std::endl;
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
