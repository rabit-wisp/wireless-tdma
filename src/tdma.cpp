#include <thread>
#include <iostream>
#include "tdma.h"

using std::chrono::operator""s;

TDMAScheduler::TDMAScheduler(size_t slotPosition,
                             size_t slotCount,
                             std::chrono::microseconds frameDuration,
                             std::function<void()> pause,
                             std::function<void()> resume,
                             std::optional<size_t> count,
                             bool verbose) : slot_position(slotPosition),
                                             slots_per_frame(slotCount),
                                             frame_duration(frameDuration),
                                             pause_transmissions(pause),
                                             resume_transmissions(resume),
                                             count(count),
                                             verbose(verbose)
{
    frame_start = timestamp::clock::now();
    //next_frame = frame_start.load() + frame_duration;

    slot_duration = frame_duration / slots_per_frame;
}

void TDMAScheduler::terminate() { stop = true; }

void TDMAScheduler::resynchronize(TDMAScheduler::timestamp beacon, size_t TUs)
{
    // beacon corresponds to last received beacon - it is in the past

    // TODO: think about the synchronization here
    //std::lock_guard<std::mutex> guard(mutex);
    std::cout << "resynchronizing tdma scheduler - new beacon start @"
              << beacon.time_since_epoch().count() << " beacon interval: " << TUs << " TUs" << std::endl;

    frame_start = beacon + frame_duration;
}

void TDMAScheduler::run()
{
    // we purposefully set the frame start to 5 seconds in the future because the default state of the TDMA scheduler
    // upon construction is to simply let traffic pass through - and thus behave like a vanilla wifi client.
    std::this_thread::sleep_for(5s);
    std::cout << "Starting TDMA scheduler..." << std::endl;

    while(!stop && (!count || count.value() > 0))
    {
        if(count)
            count.value()--;

        if (verbose) std::cout << "plugging qdisc   @" << TDMAScheduler::timestamp::clock::now().time_since_epoch().count() << std::endl;
        pause_transmissions();

        std::this_thread::sleep_until(frame_start.load() + (slot_duration * slot_position));

        if (verbose) std::cout << "unplugging qdisc @" << TDMAScheduler::timestamp::clock::now().time_since_epoch().count() << std::endl;

        resume_transmissions();

        std::this_thread::sleep_for(slot_duration);
        //std::this_thread::sleep_until(current_frame.load() + (slot_duration * (slot_position + 1)));

        frame_start = frame_start.load() + frame_duration;
    }
}
