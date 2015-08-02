#ifndef __PITBULL_H__

#include <deque>
#include <chrono>
#include <mutex>

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/variance.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/count.hpp>
namespace ba = boost::accumulators;

const int MAX_INTERVALS = 10000;

class Bad_Beat {};

class Task_Data {

    private:
        std::chrono::high_resolution_clock::time_point l = std::chrono::high_resolution_clock::time_point::min();
        std::chrono::high_resolution_clock::time_point e = std::chrono::high_resolution_clock::time_point::max();

    public:
        std::mutex lock;
        ba::accumulator_set<unsigned long long, ba::stats<ba::tag::count, ba::tag::mean, ba::tag::variance>> _intervals;

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
