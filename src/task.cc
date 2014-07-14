#include "task.h"

double Task_Data::beat(){
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
        return 0;
    }

    return -1;

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
