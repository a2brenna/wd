#ifndef __PITBULL_H__

#include <deque>
#include <chrono>
#include <hgutil/time.h>
#include <mutex>

const int max_intervals = 10000;

class Task_Data {
    private:
        std::deque<std::chrono::high_resolution_clock::duration> intervals;
        std::deque<std::chrono::high_resolution_clock::time_point> beats;
        std::chrono::high_resolution_clock::time_point l = std::chrono::high_resolution_clock::time_point::min();
        std::chrono::high_resolution_clock::time_point e = std::chrono::high_resolution_clock::time_point::max();
    public:
        std::mutex lock;

        void beat();
        std::chrono::high_resolution_clock::time_point last() const;
        std::chrono::high_resolution_clock::time_point expected() const;
        std::chrono::high_resolution_clock::time_point to_expiration() const;
        size_t num_beats() const;
        bool expired() const;
};

#endif
