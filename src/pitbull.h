#ifndef __PITBULL_H__

#include <hgutil/server.h>
#include <hgutil/socket.h>
#include <string>
#include <map>
#include <deque>
#include <chrono>

#include "watchdog.pb.h"

const int max_intervals = 10000;

class Incoming_Connection : public Task{
    public:
        Socket *sock;
        Incoming_Connection(Socket *s) { sock = s; };
        ~Incoming_Connection() { delete sock; };
};

class Task_Data {
    private:
        std::deque<std::chrono::high_resolution_clock::duration> intervals;
    public:
        std::chrono::high_resolution_clock::time_point l = std::chrono::high_resolution_clock::time_point::min();
        std::chrono::high_resolution_clock::time_point e;

        int num_beats();
        double beat();
        bool expired();
};

class Pitbull : public Handler{
    private:
        struct timeval next_expiration;
        std::string next;

        void handle_beat(watchdog::Message m);
        void reset_expiration();
    public:
        Lockable< std::map<std::string, Lockable<Task_Data>> > tracked_tasks;
        std::recursive_mutex timelock;

        Pitbull(){ next_expiration.tv_sec = 0; next_expiration.tv_usec = 0; };
        void handle(Task *t);
};

#endif
