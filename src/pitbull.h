#ifndef __PITBULL_H__

#include <hgutil/server.h>
#include <hgutil/socket.h>
#include <string>
#include <map>
#include <deque>

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
        double mean = 0;
        double deviation = 0;

        int num_beats();
        void beat();
};

class Pitbull : public Handler{
    private:
        Lockable< std::map<std::string, Lockable<Task_Data>> > tracked_tasks;
    public:
        void handle(Task *t);
};

#endif
