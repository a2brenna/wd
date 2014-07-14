#ifndef __PITBULL_H__

#include <deque>
#include <chrono>

const int max_intervals = 10000;

class Task_Data {
    private:
        std::deque<std::chrono::high_resolution_clock::duration> intervals;
    public:
        std::chrono::high_resolution_clock::time_point l = std::chrono::high_resolution_clock::time_point::min();
        std::chrono::high_resolution_clock::time_point e;

        int num_beats();
        double beat();
        bool expired();
};

#endif
