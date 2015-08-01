#include "task.h"
#include <hgutil/math.h>
#include <cmath>
#include <iostream>

std::chrono::high_resolution_clock::duration _deviation(const std::chrono::high_resolution_clock::duration &m, const std::deque<std::chrono::high_resolution_clock::duration> &data){
    if( data.size() == 0 ){
        return std::chrono::high_resolution_clock::duration(0);
    }
    else{
        //Cast durations to a lower resolution... mostly out of fear that the
        //math will get wonky because nanosecond values are *too big* and that
        //doubles are *too sparse* in the relevent ranges.
        //
        //On the other hand... if the math is done right we should always end
        //up with the double most closely approximating the *true* value...
        //TODO: Examine floating point math vs. nanosecond timestamp magnitude
        std::vector<std::chrono::milliseconds> low_resolution;
        for(const auto &d: data){
            low_resolution.push_back( std::chrono::duration_cast<std::chrono::milliseconds>(d));
        }

        const std::chrono::milliseconds mean = std::chrono::duration_cast<std::chrono::milliseconds>(m);

        //we switch to doubles here because overflow's a bitch
        std::vector<double> diff;
        for(const auto &l: low_resolution){
            const std::chrono::milliseconds deviation = l - mean;
            diff.push_back( (double)deviation.count() * (double)deviation.count() );
        }

        double total_variance = 0;
        for(const auto &x: diff){
            total_variance = total_variance + x;
        }

        double mean_variance = total_variance / data.size();
        double standard_deviation = std::sqrt(mean_variance);

        return std::chrono::milliseconds((unsigned long long)std::round(standard_deviation));
    }
}

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

    while( intervals.size() > MAX_INTERVALS ){
        intervals.pop_back();
    }

    if(l != std::chrono::high_resolution_clock::time_point::min()){
        auto t = c - l;
        intervals.push_front(t);
    }

    l = c;

    if(intervals.size() > 2){
        auto m = ::mean(intervals, std::chrono::high_resolution_clock::duration(0));
        auto d = _deviation(m, intervals);
        e = l + std::chrono::nanoseconds(m + d * 3);
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
    if(intervals.size() > 0){
        return intervals.size() + 1;
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
    if(intervals.size() > 0){
        return ::mean(intervals, std::chrono::high_resolution_clock::duration(0));
    }
    else{
        return std::chrono::high_resolution_clock::duration(0);
    }
}

std::chrono::high_resolution_clock::duration Task_Data::deviation() const{
    return _deviation(mean(), intervals);
}
