#include "task.h"

/*
WARNING: For some reason, the first recorded interval seems to be noticably
smaller than subsequent ones.  This leads to an exagerated deviation...I'm
reasonably sure that this is because of the additional one time cost of
queueing the new Task, spawning a thread, popping the Task, performing the
dynamic cast and then entering the for loop.  Reasonably sure that the client
considers the beat() sent epsilon after the connection is estabilished, but
the previously mentioned operations mean that the server does not record the
beat time until noticably later.
*/

void Task_Data::beat(const std::chrono::high_resolution_clock::time_point &c){
    //If supplied time is in the past...
    if( c < l ){
        throw Bad_Beat();
    }

    if( l != std::chrono::high_resolution_clock::time_point::min()){
        _intervals( (c - l).count() );
    }
    l = c;

    const auto count = ba::count(_intervals);
    if(count > 2){
        const auto mean = ba::mean(_intervals);
        const auto stdev = std::sqrt(ba::variance(_intervals));
        e = l + std::chrono::high_resolution_clock::duration((unsigned long long)(mean + (stdev * 3)) );
    }

}

std::chrono::high_resolution_clock::time_point Task_Data::beat(){
    const auto c = std::chrono::high_resolution_clock::now();
    beat(c);
    return c;
}

bool Task_Data::expired() const{
    if(std::chrono::high_resolution_clock::now() > e){
        return true;
    }
    return false;
}

size_t Task_Data::num_beats() const{
    size_t c = ba::count(_intervals);
    if(c > 0){
        return c + 1;
    }
    else{
        if (l == std::chrono::high_resolution_clock::time_point::min()){
            return 0;
        }
        else{
            return 1;
        }
    }
}

std::chrono::high_resolution_clock::time_point Task_Data::last() const{
    return l;
}

std::chrono::high_resolution_clock::time_point Task_Data::expected() const{
    return e;
}

std::chrono::high_resolution_clock::duration Task_Data::to_expiration() const{
    return e - std::chrono::high_resolution_clock::now();
}

std::chrono::high_resolution_clock::duration Task_Data::mean() const{
    return std::chrono::high_resolution_clock::duration((unsigned long long)ba::mean(_intervals));
}

std::chrono::high_resolution_clock::duration Task_Data::deviation() const{
    return std::chrono::high_resolution_clock::duration((unsigned long long)std::sqrt(ba::variance(_intervals)));
}
