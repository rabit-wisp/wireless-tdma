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

    std::atomic<timestamp> current_epoch;
    std::atomic<timestamp> next_epoch;
    std::atomic<std::chrono::nanoseconds> epoch_duration;
    std::atomic<size_t> TUs; // beacon TUs

    std::optional<size_t> count;

    bool verbose;

    std::mutex mutex;
    std::atomic<bool> stop{false};

    std::chrono::microseconds slot_duration; // slot duration
    size_t slot_position;                    // our slot position
    size_t epoch_slots;                      // number of slots per epoch

    std::function<void()> pause_transmissions;
    std::function<void()> resume_transmissions;

    TDMAScheduler(timestamp beacon,
                  std::chrono::microseconds epoch_duration,
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
