#ifndef __PITBULL_H__

#include <hgutil/server.h>
#include <string>
#include <map>
#include <deque>

const int max_ivals = 10000;

class Incoming_Connection : public Task{
    public:
        int sockfd;
        Incoming_Connection(int s) { sockfd = s; };
};

class Task_Data {
    private:
        std::deque<double> ivals;
    public:
        double last = -1;
        double mean = 0;
        double deviation = 0;

        void beat();
};

class Pitbull : public Handler{
    private:
        Lockable< std::map<std::string, Lockable<Task_Data>> > tracked_tasks;
    public:
        void handle(Task *t);
};

#endif
