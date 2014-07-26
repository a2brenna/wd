#include "task.h"
#include <hgutil/math.h>
#include <cmath>

void Task_Data::beat(){
    int s = intervals.size();
    while( s > (max_intervals - 1) ){
        intervals.pop_back();
    }

    int s2 = _intervals.size();
    while( s2 > (max_intervals - 1) ){
        _intervals.pop_back();
    }

    std::chrono::high_resolution_clock::time_point c = std::chrono::high_resolution_clock::now();

    if(l != std::chrono::high_resolution_clock::time_point::min()){
        auto t = c - l;
        intervals.push_front(t);
        _intervals.push_front(t.count());
    }

    l = c;

    if(intervals.size() > 2){
        auto m = ::mean(_intervals, (long)0);
        for(auto x: _intervals){
            std::cerr << "Elem: " << x << std::endl;
        }
        std::cerr << "mean " << m << std::endl;
        auto d = stdev(m, _intervals, (long)0);
        std::cerr << "deviation " << stdev(m, _intervals, (long)0);
        e = l + std::chrono::nanoseconds(m + d * 3);
        std::cerr << "Mean seconds " << mean() << std::endl;
        std::cerr << "Deviation seconds " << deviation() << std::endl;
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

double Task_Data::expected(){
    return to_seconds(e.time_since_epoch());
}

double Task_Data::mean(){
    long mean_nanos = ::mean(_intervals, (long)0);
    std::chrono::nanoseconds ns(mean_nanos);
    std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(ns);
    return ms.count() / 1000.0;
}

double Task_Data::deviation(){
    long deviation_nanos = ::stdev(::mean(_intervals, (long)0), _intervals, (long)0);
    std::chrono::nanoseconds ns(deviation_nanos);
    std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(ns);
    return ms.count() / 1000.0;
}

double Task_Data::time_to_expiration(){
    auto n = to_seconds(std::chrono::high_resolution_clock::now().time_since_epoch());
    return expected() - n;
}
