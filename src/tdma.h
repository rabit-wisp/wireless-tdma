#pragma once
#include <random>
#ifndef _TDMA_H_
#define _TDMA_H_

#include <chrono>
#include <functional>
#include <mutex>
#include <atomic>

struct TDMAScheduler {

  /*
   * TDMA Scheduler class: this class maintains the current state of TDMA and
   * schedules
   */

    using timestamp = std::chrono::time_point<std::chrono::steady_clock>;

    std::atomic<timestamp> frame_start;
    std::chrono::microseconds frame_duration;

    std::optional<size_t> count;

    bool verbose;

    std::mutex mutex;
    std::atomic<bool> stop{false};

    // computed values
    std::chrono::microseconds slot_duration;   // slot duration

    std::chrono::microseconds jitter;          // expected system jitter

    // configured values
    size_t slot_position;                      // our slot position
    size_t slots_per_frame;                    // number of slots per frame



    std::function<void()> pause_transmissions;
    std::function<void()> resume_transmissions;

    TDMAScheduler(size_t slotPosition,
                  size_t slotCount,
                  timestamp frame_start,
                  std::chrono::microseconds frame_duration,
                  std::chrono::microseconds system_jitter,
                  std::function<void()> pause_transmissions,
                  std::function<void()> resume_transmissions,
                  std::optional<size_t> count,
                  bool verbose);

    void resynchronize(timestamp frame_start, size_t TUs);
    void terminate();
    void run();
};

#endif // _TDMA_H_
