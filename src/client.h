#ifndef __CLIENT_H__
#define __CLIENT_H__

#include <memory>
#include <string>
#include <smpl.h>
#include <cassert>

class Heart {
    private:

        std::shared_ptr<smpl::Channel> _server;
        std::string _id;
        std::string _cookie;

    public:

        Heart(std::shared_ptr<smpl::Channel> server, const std::string id) { _server = server; _id = id; };
        Heart(std::shared_ptr<smpl::Channel> server, const std::string id, const std::string &cookie) {
            _server = server;
            _id = id;
            assert(cookie.size() == 32);
            _cookie = cookie;
        };
        Heart() {};
        void beat();

};

void beat(std::shared_ptr<smpl::Remote_Postbox> server, const std::string id);
void beat(std::shared_ptr<smpl::Remote_Postbox> server, const std::string id, const std::string &cookie);

#endif
