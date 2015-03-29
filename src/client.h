#ifndef __CLIENT_H__
#define __CLIENT_H__

#include <memory>
#include <string>
#include <smpl.h>

class Heart {
    private:

        std::shared_ptr<smpl::Channel> _server;
        std::string _id;

    public:

        Heart(std::shared_ptr<smpl::Channel> server, const std::string id) { _server = server; _id = id; };
        void beat();

};

#endif
