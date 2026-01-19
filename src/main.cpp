#include <iostream>
#include <chrono>
#include <csignal>
#include <atomic>
#include <condition_variable>
#include <mutex>

#include "docopt.h"
#include "tdma.h"
#include "beacon.h"
#include "qdisc.h"

using std::chrono::operator""us;

static const char USAGE[] =
R"(Cooperative TDMA scheduler

Usage:
  tc-tdma <INTERFACE> <SLOT> [options]
  tc-tdma <INTERFACE> --show-beacons

Options:
  <INTERFACE>                name of interface to attach to (e.g. wlan0)
  <SLOT>                     ordinal 0-based index of TDMA slot
  --show-beacons             don't do TDMA, simply show beacon frame statistics
  --slots-per-frame=SLOTS    number of slots per TDMA frame [default: 10]
  --frame-TUs=TUs            Time Units for each TDMA frame [default: 10]
  --buffer-size=SIZE         number of packets to buffer during plug period [default: 10240]
  --count=COUNT              if specified, only run for specified number of TDMA frames
  --beacon-timeout=TIMEOUT   exit with error if first beacon doesn't arrive within timeout
  --verbose                  show beacons and various things

This program effectively does these 4 actions:

)";

/*
 * SETUP: Create the netem qdisc externally before running this program:
 *
 * Command to add netem qdisc to interface:
 *   tc qdisc add dev wlan0 root netem delay 0ms limit 10000
 *   tc qdisc add dev wlan0 root netem delay 0ms limit 10000
 *   tc qdisc change dev wlan0 root netem delay 0ms limit 10000
 *   tc qdisc list dev wlan0
 *   tc qdisc del dev wlan0 root
 */

int show_beacon_stats(const std::string& interface)
{
    Beacon beacon(interface, [&](auto ts, auto... args){}, true, true);

    static std::condition_variable terminate;
    std::mutex mutex;

    auto signal_handler = [](int signal) {
        if (signal == SIGHUP || signal == SIGINT || signal == SIGTERM) {
            terminate.notify_all();
        }
    };

    std::signal(SIGHUP, signal_handler);
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    std::unique_lock<std::mutex> lock(mutex);
    terminate.wait(lock);
    return 0;
}

int main(int argc, const char* argv[])
{
    auto args = docopt::docopt(USAGE, {argv + 1, argv + argc});


    bool beacon_only = args["--show-beacons"].asBool();
    std::string interface = args["<INTERFACE>"].asString();
    size_t slotNumber = beacon_only ? 0 : args["<SLOT>"].asLong();
    size_t slotsPerFrame = args["--slots-per-frame"].asLong();
    size_t frameTUs = args["--frame-TUs"].asLong();
    auto frameDuration = std::chrono::microseconds(frameTUs * 1024);

    using ms = std::chrono::milliseconds;
    std::optional<ms> timeout = (args["--beacon-timeout"] ?
                                 std::optional<ms>(ms(args["--beacon-timeout"].asLong())) : std::nullopt);
    std::optional<size_t> pollCount = args["--count"] ? std::optional<size_t>(args["--count"].asLong()) : std::nullopt;
    bool verbose = args["--verbose"].asBool();

    try
    {
        if (beacon_only)
            return show_beacon_stats(interface);

        QdiscController plug(interface, args["--buffer-size"].asLong(), verbose); // qdisc plug controller

        std::condition_variable first_beacon;
        std::mutex mutex;
        std::optional<TDMAScheduler::timestamp> firstFrame;

        // 80211 BSS beacon broadcast listener
        Beacon beacon(interface,
                      [&](auto ts, auto... args){
                          std::unique_lock<std::mutex> lock(mutex);
                          firstFrame = ts;
                          first_beacon.notify_one();
                      },
                      true); // be verbose for first beacon

        beacon.listen();

        std::cout << "Waiting for first BSS beacon to arrive..." << std::endl;
        {
            std::unique_lock<std::mutex> lock(mutex);
            if (timeout)
                first_beacon.wait_for(lock, timeout.value(), [&]{ return !!firstFrame; });
            else
                first_beacon.wait(lock, [&]{ return !!firstFrame; });

            //beacon.update_clock_offset(std::chrono::microseconds tsf_epoch,
            //std::chrono::steady_clock::time_point steady_timestamp)

            beacon.verbose = verbose; // after first beacon has been received, assume global verbosity level
        }

        static TDMAScheduler scheduler(slotNumber,
                                       slotsPerFrame,
                                       firstFrame.value_or(TDMAScheduler::timestamp::clock::now()),
                                       frameDuration,
                                       [&](){ plug.tx_pause(); },
                                       [&](){ plug.tx_resume(); },
                                       pollCount,
                                       verbose);

        beacon.sync = [&](auto... args){ scheduler.resynchronize(args...); };

        // add a signal handler so that we correctly destroy all resources since they have OS level RAII
        auto signal_handler = [](int signal) {
            if (signal == SIGHUP || signal == SIGINT || signal == SIGTERM) {
                scheduler.terminate(); // cause the scheduler loop to terminate, causing execution to leave the current context
            }
        };

        std::signal(SIGHUP, signal_handler);
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);

        scheduler.run();

    } catch ( std::exception& e ) {

        std::cerr << "exiting on unhandled exception: " << e.what() << std::endl;
    }

    return 0;
}

