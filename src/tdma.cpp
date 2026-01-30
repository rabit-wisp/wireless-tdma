#include <thread>
#include <iostream>
#include "tdma.h"

using std::chrono::operator""s;

TDMAScheduler::TDMAScheduler(size_t slotPosition,
                             size_t slotCount,
                             timestamp frameStart,
                             std::chrono::microseconds frameDuration,
                             std::chrono::microseconds systemJitter,
                             std::function<void()> pause,
                             std::function<void()> resume,
                             std::optional<size_t> count,
                             bool verbose) : slot_position(slotPosition),
                                             slots_per_frame(slotCount),
                                             frame_start(frameStart),
                                             frame_duration(frameDuration),
                                             slot_duration(frameDuration / slotCount),
                                             jitter(systemJitter),
                                             pause_transmissions(pause),
                                             resume_transmissions(resume),
                                             count(count),
                                             verbose(verbose)
{
    std::array<std::chrono::nanoseconds, 21> timings;
    constexpr size_t middle = timings.size() / 2;
    constexpr size_t iterations = 100;
    for (auto& t : timings)
    {
        const auto a = timestamp::clock::now();
        cpu_spinner(iterations);
        const auto b = timestamp::clock::now();
        t = std::chrono::duration_cast<std::chrono::nanoseconds>(b - a);
    }

    std::nth_element(timings.begin(), timings.begin() + middle, timings.end());
    nop_duration = timings[middle] / iterations;

    if (verbose)
        std::cout << "tdma: cpu spin cost " << std::fixed << std::setprecision(3) << double(nop_duration.count()) / 1000.0  << " µs" << std::endl;
}

void TDMAScheduler::cpu_spinner(int count) noexcept {
    for (int i = 0; i < count ; i++ )
        asm volatile(
                     "nop; nop; nop; nop; nop; nop; nop; nop; nop; nop;"
                     "nop; nop; nop; nop; nop; nop; nop; nop; nop; nop;"
                     "nop; nop; nop; nop; nop; nop; nop; nop; nop; nop;"
                     ::: "memory"
                     );
}

void TDMAScheduler::terminate() { stop = true; }

void TDMAScheduler::resynchronize(TDMAScheduler::timestamp beacon, size_t TUs)
{
    // beacon corresponds to last received beacon - it is in the past

    // TODO: think about the synchronization here
    //std::lock_guard<std::mutex> guard(mutex);
    if (verbose)
        std::cout << "resynchronizing tdma scheduler - new beacon start "
                  << beacon.time_since_epoch().count() << "ns beacon interval: " << TUs << " TUs" << std::endl;

    frame_start = beacon + frame_duration;
}

void TDMAScheduler::run()
{
    using namespace std::chrono;
    const auto downtime = slot_duration * slot_position;

    pause_transmissions();

    while(!stop && (!count || count.value() > 0))
    {
        !!count && --*count;

        const time_point this_frame_start = frame_start.load() + downtime;
        std::this_thread::sleep_until(this_frame_start - jitter);


        const auto slotStart = timestamp::clock::now();

        resume_transmissions();

        std::this_thread::sleep_for(slot_duration - jitter);

        const auto slotEnd = timestamp::clock::now();

        pause_transmissions();

        if (verbose)
        {
            const auto a = duration_cast<microseconds>(slotStart - frame_start.load()).count();
            const auto b = duration_cast<microseconds>(slotEnd - frame_start.load()).count();
            std::cout << "qdisc PLUG: @"
                      << (double)frame_start.load().time_since_epoch().count() / 1e9
                      << " s   | " << std::setw(8) << std::setfill(' ') << a
                      << "  --> " << std::setw(8) << std::setfill(' ') << b << " µs"
                      << "   | total duration: " << std::setw(8) << std::setfill(' ') << b - a  << "µs" << std::endl;
        }

        frame_start = frame_start.load() + frame_duration;
    }
}
