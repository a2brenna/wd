#ifndef __PITBULL_H__

#include <deque>
#include <chrono>

const int max_intervals = 10000;

class Task_Data {
    public:
        std::deque<std::chrono::high_resolution_clock::duration> intervals;
        std::chrono::high_resolution_clock::time_point l = std::chrono::high_resolution_clock::time_point::min();
        std::chrono::high_resolution_clock::time_point e;

        int num_beats();
        double beat();
        bool expired();
};

#endif
