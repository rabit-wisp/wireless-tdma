#include <thread>
#include <iostream>
#include "tdma.h"


TDMAScheduler::TDMAScheduler(TDMAScheduler::timestamp frame_start,
                             std::chrono::microseconds frame_duration,
                             size_t slotPosition,
                             std::chrono::microseconds slotDuration,
                             std::function<void()> pause,
                             std::function<void()> resume,
                             std::optional<size_t> count,
                             bool verbose) : current_frame(frame_start),
                                             frame_duration(frame_duration),
                                             slot_position(slotPosition),
                                             slot_duration(slotDuration),
                                             pause_transmissions(pause),
                                             resume_transmissions(resume),
                                             count(count),
                                             verbose(verbose)
{
    next_frame = current_frame.load() + frame_duration;
}

void TDMAScheduler::terminate() { stop = true; }

void TDMAScheduler::resynchronize(TDMAScheduler::timestamp beacon, std::chrono::microseconds duration)
{
    // TODO: think about the synchronization here
    //std::lock_guard<std::mutex> guard(mutex);
    std::cout << "resynchronizing tdma scheduler - new beacon start @"
              << beacon.time_since_epoch().count() << " duration: " << duration << std::endl;
    next_frame = beacon + duration;
    current_frame = beacon;
    frame_duration = duration;
}

void TDMAScheduler::run()
{
    while(!stop && (!count || count.value() > 0))
    {
        if(count)
            count.value()--;

        if (verbose) std::cout << "plugging qdisc   @" << TDMAScheduler::timestamp::clock::now().time_since_epoch().count() << std::endl;
        pause_transmissions();

        std::this_thread::sleep_until(current_frame.load() + (slot_duration * slot_position));

        if (verbose) std::cout << "unplugging qdisc @" << TDMAScheduler::timestamp::clock::now().time_since_epoch().count() << std::endl;

        resume_transmissions();

        std::this_thread::sleep_for(slot_duration);
        //std::this_thread::sleep_until(current_frame.load() + (slot_duration * (slot_position + 1)));

        //frame = frame.load() + frame_duration.load();
    }
}
