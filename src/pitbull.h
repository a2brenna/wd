#ifndef __PITBULL_H__

#include <hgutil/server.h>
#include <hgutil/socket.h>
#include <string>
#include <map>
#include <deque>

#include "watchdog.pb.h"

const int max_ivals = 10000;

class Incoming_Connection : public Task{
    public:
        Socket *sock;
        Incoming_Connection(Socket *s) { sock = s; };
        ~Incoming_Connection() { delete sock; };
};

class Task_Data {
    private:
        std::deque<double> ivals;
    public:
        double last = -1;
        double deviation = 0;
        double expiration = 0;

        int num_beats();
        double beat();
        bool expired();
};

class Pitbull : public Handler{
    private:
        struct timeval next_expiration;

        void handle_beat(watchdog::Message m);
    public:
        Lockable< std::map<std::string, Lockable<Task_Data>> > tracked_tasks;
        std::recursive_mutex timelock;

        Pitbull(){ next_expiration.tv_sec = 0; next_expiration.tv_usec = 0; };
        void handle(Task *t);
};

#endif
