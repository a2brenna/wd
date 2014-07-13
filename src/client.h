#ifndef __CLIENT_H__
#define __CLIENT_H__

#include <hgutil/socket.h>

class Heart {
    private:
        Socket *server;
        std::string id;
    public:
        Heart(Socket *s, std::string i) { server = s; id = i; };
        ~Heart() { delete server; };
        void beat();
};

#endif
