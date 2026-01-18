#include <iostream>
#include <chrono>
#include <csignal>
#include <atomic>

#include "docopt.h"
#include "tdma.h"
#include "beacon.h"
#include "qdisc.h"

using std::chrono::operator""us;

static const char USAGE[] =
R"(Cooperative TDMA scheduler

Usage:
  tc-tdma <INTERFACE> <SLOT> [options]

Options:
  <INTERFACE>                name of interface to attach to (e.g. wlan0)
  <SLOT>                     ordinal 0-based index of TDMA slot
  --slots-per-frame=SLOTS    number of slots per TDMA frame [default: 10]
  --frame-TUs=TUs            Time Units for each TDMA frame [default: 10]
  --buffer-size=SIZE         number of packets to buffer during plug period [default: 10240]
  --count=COUNT              if specified, only run for specified number of TDMA frames
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

int main(int argc, const char* argv[])
{
    auto args = docopt::docopt(USAGE, {argv + 1, argv + argc});

    for ( auto [a,b] : args)
        std::cout << a << " : " << b << std::endl;

    std::string interface = args["<INTERFACE>"].asString();
    size_t slotNumber = args["<SLOT>"].asLong();
    size_t slotsPerFrame = args["--slots-per-frame"].asLong();
    size_t frameTUs = args["--frame-TUs"].asLong();
    auto frameDuration = std::chrono::microseconds(frameTUs * 1024);

    std::optional<size_t> pollCount = args["--count"] ? std::optional<size_t>(args["--count"].asLong()) : std::nullopt;

    bool verbose = args["--verbose"].asBool();

    try
    {
        QdiscController plug(interface, args["--buffer-size"].asLong(), verbose); // qdisc plug controller

        static TDMAScheduler scheduler(slotNumber,
                                       slotsPerFrame,
                                       frameDuration,
                                       [&](){ plug.tx_pause(); },
                                       [&](){ plug.tx_resume(); },
                                       pollCount,
                                       verbose);

        // 80211 BSS beacon broadcast listener
        Beacon beacon(interface,
                      [&](auto... args){ scheduler.resynchronize(args...); },
                      verbose);

        // add a signal handler so that we correctly destroy all resources since they have OS level RAII
        auto signal_handler = [](int signal) {
            if (signal == SIGHUP || signal == SIGINT || signal == SIGTERM) {
                scheduler.terminate(); // cause the scheduler loop to terminate, causing execution to leave the current context
            }
        };

        std::signal(SIGHUP, signal_handler);
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);

        beacon.listen(); // start listener thread to monitor beacon broadcasts
        scheduler.run(); // run the TDMA scheduler

    } catch ( std::exception& e ) {

        std::cerr << "exiting on unhandled exception: " << e.what() << std::endl;
    }

    return 0;
}

