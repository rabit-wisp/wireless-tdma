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

    using timestamp = std::chrono::time_point<std::chrono::system_clock>;

    std::atomic<timestamp> current_frame;
    std::atomic<timestamp> next_frame;
    std::atomic<std::chrono::nanoseconds> frame_duration;
    std::atomic<size_t> TUs; // beacon TUs

    std::optional<size_t> count;

    bool verbose;

    std::mutex mutex;
    std::atomic<bool> stop{false};

    std::chrono::microseconds slot_duration; // slot duration
    size_t slot_position;                    // our slot position
    size_t slots_per_frame;                      // number of slots per frame

    std::function<void()> pause_transmissions;
    std::function<void()> resume_transmissions;

    TDMAScheduler(timestamp frame_start,
                  std::chrono::microseconds frame_duration,
                  size_t slotPosition,
                  std::chrono::microseconds slotDuration,
                  std::function<void()> pause_transmissions,
                  std::function<void()> resume_transmissions,
                  std::optional<size_t> count,
                  bool verbose);

    void resynchronize(timestamp beacon,
                       std::chrono::microseconds duration);
    void terminate();
    void run();
};

#endif // _TDMA_H_
