#ifndef __PITBULL_H__

#include <hgutil/server.h>

class Incoming_Connection : public Task{
    public:
        int sockfd;
        Incoming_Connection(int s) { sockfd = s; };
};

class Pitbull : public Handler{
    public:
        void handle(Task *t);
};

#endif
