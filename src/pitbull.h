#ifndef __PITBULL_H__

#include <hgutil/server.h>
#include <hgutil/socket.h>
#include <string>
#include <map>
#include <deque>
#include <chrono>

#include "task.h"
#include "watchdog.pb.h"

class Incoming_Connection : public Task{
    public:
        Socket *sock;
        Incoming_Connection(Socket *s) { sock = s; };
        ~Incoming_Connection() { delete sock; };
};

class Pitbull : public Handler{
    private:
        struct timeval next_expiration;
        std::string next;

        void handle_beat(watchdog::Message m);
        void handle_query(watchdog::Message m, Incoming_Connection *i);
        void handle_orders(watchdog::Message m);
        void reset_expiration();
        void forget(std::string to_forget);
    public:
        Lockable< std::map<std::string, Lockable<Task_Data>> > tracked_tasks;
        std::recursive_mutex timelock;

        Pitbull(){ next_expiration.tv_sec = 0; next_expiration.tv_usec = 0; };
        void handle(Task *t);
};

#endif
