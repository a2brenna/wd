#ifndef __PITBULL_H__

#include <deque>
#include <chrono>
#include <hgutil/time.h>
#include <mutex>

const int MAX_INTERVALS = 10000;

class Bad_Beat {};

class Task_Data {
    private:
        //std::deque<std::chrono::high_resolution_clock::duration> intervals;
        //std::deque<std::chrono::high_resolution_clock::time_point> beats;
        std::chrono::high_resolution_clock::time_point l = std::chrono::high_resolution_clock::time_point::min();
        std::chrono::high_resolution_clock::time_point e = std::chrono::high_resolution_clock::time_point::max();
    public:
        std::mutex lock;
        std::deque<std::chrono::high_resolution_clock::duration> intervals;

        std::chrono::high_resolution_clock::time_point beat();
        void beat(const std::chrono::high_resolution_clock::time_point &c);
        std::chrono::high_resolution_clock::time_point last() const;
        std::chrono::high_resolution_clock::time_point expected() const;
        std::chrono::high_resolution_clock::duration to_expiration() const;
        std::chrono::high_resolution_clock::duration mean() const;
        std::chrono::high_resolution_clock::duration deviation() const;
        size_t num_beats() const;
        bool expired() const;
};

#endif
