#include <thread>
#include <iostream>
#include <iomanip>
#include <ranges>
#include "tdma.h"

using std::chrono::operator""s;

TDMAScheduler::TDMAScheduler(std::vector<slot_info> slots,
                             size_t slotCount,
                             timestamp frameStart,
                             std::chrono::microseconds frameDuration,
                             std::chrono::microseconds systemJitter,
                             std::function<void()> pause,
                             std::function<void()> resume,
                             std::optional<size_t> count,
                             bool verbose) : slots(slots),
                                             slots_per_frame(slotCount),
                                             frame_start(frameStart),
                                             frame_duration(frameDuration),
                                             slot_duration(frameDuration / slotCount),
                                             jitter(systemJitter),
                                             pause_transmissions(pause),
                                             resume_transmissions(resume),
                                             frame_counter(count),
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
        std::cout << "qdisc: SYNC @" << std::fixed << std::setprecision(6) << (double)(beacon.time_since_epoch().count()) / 1e9
                  << " s   |   beacon interval: " << TUs << " TUs" << std::endl;

    frame_start = beacon + frame_duration;
}

void TDMAScheduler::run()
{
    using namespace std::chrono;
    const bool wrapAround = slots[0].index == 0 && ((slots.back().index + slots.back().length) == slots_per_frame);
    const bool first_slot_live = slots[0].index == 0; // if we occupy the first slot 
    const auto first_slot_downtime = slot_duration * slots[0].index; // this is the beginning of the first slot in a frame

    pause_transmissions(); // default state is to be plugged

    while(!stop && (!frame_counter || *frame_counter > 0))
    {
        ////////////////////////////////////////////////////////////////////////////////////////////
        // FRAME BEGIN
        // At the beginning of the frame, we do an absolute time alignment for two reasons:
        // 1) during the frame, we may have accumulated drift from repeated relative timed sleeps
        // 2) we may have received a new beacon that makes us have to resynchronize our frame

        !!frame_counter && --*frame_counter;

        const time_point first_slot = frame_start.load() + first_slot_downtime;
        std::this_thread::sleep_until(first_slot - jitter);

        nanoseconds remaining = duration_cast<nanoseconds>(first_slot - timestamp::clock::now());
        cpu_spinner(remaining / nop_duration);

        // SLOTS
        for (auto it = slots.begin(); it != slots.end(); it++)
        {
            const auto& current_slot = *it;
            auto next_it = std::next(it);
            const bool lastSlot = next_it == slots.end();
            const bool wrappedSlot = wrapAround && lastSlot; 

            const auto slotStart = timestamp::clock::now();
            resume_transmissions();

            const auto sleep_duration = (slot_duration * current_slot.length);
            std::this_thread::sleep_for(sleep_duration - jitter);

            remaining = duration_cast<nanoseconds>(sleep_duration - (timestamp::clock::now() - slotStart));

            cpu_spinner(remaining / nop_duration);

            const auto slotEnd = timestamp::clock::now();

            // minor optimization: don't pause transmissions if this slot is the last of the frame and the frame starts
            // with our slot as well
            if ( !wrappedSlot ) [[likely]]
                pause_transmissions();

            if (verbose)
            {
                const auto a = duration_cast<microseconds>(slotStart - frame_start.load()).count();
                const auto b = duration_cast<microseconds>(slotEnd - frame_start.load()).count();
                std::cout << "qdisc: PLUG @" << std::setprecision(6)
                          << (double)frame_start.load().time_since_epoch().count() / 1e9
                          << " s   | " << std::setw(8) << std::setfill(' ') << a
                          << "  --> " << std::setw(8) << std::setfill(' ') << b << " µs"
                          << "   | real duration: " << std::setw(8) << std::setfill(' ') << b - a  << "µs"
                          << "   | slot #" << current_slot.index << " len " << current_slot.length
                          << (wrappedSlot? " (wrap around slot)" : "")
                          << std::endl;
            }

            if (next_it != slots.end())
            {
                const auto& next_slot = *next_it;
                const auto sleep_duration = (slot_duration * (next_slot.index - (current_slot.index + current_slot.length)));
                std::this_thread::sleep_for(sleep_duration - jitter);

                remaining = duration_cast<nanoseconds>(sleep_duration - (timestamp::clock::now() - slotStart));
                cpu_spinner(remaining / nop_duration);
            }
        }

        frame_start = frame_start.load() + frame_duration;
    }
}
