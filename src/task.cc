#include "task.h"
#include <cmath>

typedef std::chrono::high_resolution_clock::duration nanos;

nanos t_mean(std::deque<nanos>::iterator start, std::deque<nanos>::iterator end){
    nanos accumulator;
    int count = 0;
    for(std::deque<nanos>::iterator i = start; i != end; i++){
        accumulator = accumulator + (*i);
        count++;
    }

    auto result = accumulator / count;
    return result;
}

double t_deviation (std::deque<nanos>::iterator start, std::deque<nanos>::iterator end, nanos m){
    int count = 0;
    std::deque<nanos> variances;
    for(std::deque<nanos>::iterator i = start; i != end; i++){
        variances.push_back( (*i - m) * (*i - m).count() );
        count++;
    }

    auto result = sqrt((t_mean(variances.begin(), variances.end())).count());
    return result;
}

void Task_Data::beat(){
    int s = intervals.size();
    while( s > (max_intervals - 1) ){
        intervals.pop_back();
    }

    std::chrono::high_resolution_clock::time_point c = std::chrono::high_resolution_clock::now();

    if(l != std::chrono::high_resolution_clock::time_point::min()){
        intervals.push_front(c - l);
    }

    l = c;

    if(intervals.size() > 2){
        e = l + (intervals.front() * 2);
    }
}

bool Task_Data::expired(){
    if(std::chrono::high_resolution_clock::now() > e){
        return true;
    }
    return false;
}

int Task_Data::num_beats(){
    return intervals.size();
}

double Task_Data::last(){
    auto last = l.time_since_epoch();
    return to_seconds(last);
}

std::chrono::high_resolution_clock::time_point Task_Data::_expected(){
    return l + (intervals.back() * 2);
}


double Task_Data::expected(){
    return to_seconds(e.time_since_epoch());
}

double Task_Data::mean(){
    return to_seconds(t_mean(intervals.begin(), intervals.end()));
}

double Task_Data::deviation(){
    return t_deviation(intervals.begin(), intervals.end(), t_mean(intervals.begin(), intervals.end()));
}

double Task_Data::time_to_expiration(){
    auto n = to_seconds(std::chrono::high_resolution_clock::now().time_since_epoch());
    return expected() - n;
}
