#include "pitbull.h"
#include "config.h"
#include <hgutil/socket.h>
#include <hgutil/fd.h>
#include <hgutil/time.h>
#include <memory>
#include <gnutls/gnutls.h>
#include <iostream>
#include <signal.h>
#include <mutex>
#include <syslog.h>
#include <limits.h>
#include <sys/time.h>
#include <chrono>

Pitbull p;

void expiration(int sig){
    //assuming we're woken up by an alarm
    (void)sig;
    std::lock_guard<std::recursive_mutex> t_lock(p.timelock);
    std::lock_guard<std::recursive_mutex> lock_all(p.tracked_tasks.lock);
    for(auto t: p.tracked_tasks.data){
        std::lock_guard<std::recursive_mutex> lock_individual(t.second.lock);
        if(t.second.data.expired()){
            std::cerr << "Task: " << t.first << " has expired" << std::endl;
            syslog(LOG_WARNING, "Task: %s has expired", t.first.c_str());
        }
    }
}

int main(int argc, char *argv[]){
    get_config(argc, argv);
    openlog("watchdog", LOG_NDELAY, LOG_LOCAL1);
    setlogmask(LOG_UPTO(LOG_INFO));
    syslog(LOG_INFO, "Watchdog starting...");

    gnutls_certificate_credentials_t x509_cred = tls_init(KEYFILE.c_str(), CERTFILE.c_str(), CAFILE.c_str());

    Connection_Factory ears{};
    int port1 = listen_on(PORT, false);
    ears.add_socket(port1);

    signal(SIGALRM, expiration);

    set_timer(std::chrono::nanoseconds::max());

    for(;;){
        try{
            int next = ears.next_connection();
            std::shared_ptr<Task> t(new Incoming_Connection(new Secure_Socket(next, true, x509_cred)));
            p.queue_task(t);
            p.handle_next();
        }
        catch(Network_Error e){
            std::cerr << "Network_Error: " << e.msg << std::endl;
        }
    }

    return 0;
}
