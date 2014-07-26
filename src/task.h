#ifndef __PITBULL_H__

#include <deque>
#include <chrono>
#include <hgutil/time.h>

const int max_intervals = 10000;

class Task_Data {
    private:
        std::deque<long> _intervals;
        std::chrono::high_resolution_clock::time_point _expected();
    public:
        std::deque<std::chrono::high_resolution_clock::duration> intervals;
        std::chrono::high_resolution_clock::time_point l = std::chrono::high_resolution_clock::time_point::min();
        std::chrono::high_resolution_clock::time_point e = std::chrono::high_resolution_clock::time_point::max();

        void beat();

        double last();
        double expected();
        double mean();
        double deviation();
        double time_to_expiration();
        int num_beats();

        bool expired();
};

#endif
