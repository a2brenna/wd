#include "task.h"
#include <hgutil/math.h>
#include <cmath>
#include <iostream>

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
void Task_Data::beat(){
    int s = intervals.size();
    while( s > (max_intervals - 1) ){
        intervals.pop_back();
    }

    const auto c = std::chrono::high_resolution_clock::now();

    if(l != std::chrono::high_resolution_clock::time_point::min()){
        auto t = c - l;
        intervals.push_front(t);
    }

    l = c;

    if(intervals.size() > 2){
        /*
        auto m = ::mean(intervals, std::chrono::high_resolution_clock::duration(0));
        e = l + std::chrono::nanoseconds(m + d * 3); //If actually normal, this gives us 99.7% chance of NOT getting a false positive
        std::cerr << "Mean: " << m << " Deviation: " << d << std::endl;
        */
    }
}

bool Task_Data::expired() const{
    if(std::chrono::high_resolution_clock::now() > e){
        return true;
    }
    return false;
}

size_t Task_Data::num_beats() const{
    return intervals.size();
}

std::chrono::high_resolution_clock::time_point Task_Data::last() const{
    return l;
}

std::chrono::high_resolution_clock::time_point Task_Data::expected() const{
    return e;
}
