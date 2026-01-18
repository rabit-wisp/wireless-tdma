#include <iostream>
#include <chrono>
#include <csignal>
#include <atomic>

#include "docopt.h"
#include "tdma.h"
#include "beacon.h"
#include "qdisc.h"

using std::chrono::operator""ms;
using std::chrono::operator""us;


static const char USAGE[] =
R"(Cooperative TDMA scheduler

Usage:
  tc-tdma <INTERFACE> <SLOT> [--duration=DURATION_US] [--count=COUNT] [--buffer-size=SIZE] [--verbose]

Options:
  <INTERFACE>              name of interface to attach to (e.g. wlan0)
  <SLOT>                   ordinal index of TDMA slot
  --duration=DURATION_US   slot duration in microseconds (when ommitted, this is inferred from beacon TU length) [default: 200]
  --buffer-size=SIZE       number of packets to buffer during plug period [default: 10240]
  --count=COUNT            if specified, only run for specified number of beacon broadcasts
  --verbose                show beacons

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

    std::string interface = args["<INTERFACE>"].asString();
    size_t slotNumber = args["<SLOT>"].asLong();
    size_t slotDuration = args["--duration"].asLong();
    bool verbose = args["--verbose"].asBool();
    std::optional<size_t> pollCount = args["--count"] ? std::optional<size_t>(args["--count"].asLong()) : std::nullopt;

    try {
        QdiscController plug(interface, verbose); // qdisc plug controller

        static TDMAScheduler scheduler(TDMAScheduler::timestamp::clock::now() + 1000ms,
                                       std::chrono::microseconds(100 * 1024), // assume default epoch duration of 100TUs
                                       slotNumber,
                                       std::chrono::microseconds(slotDuration),
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

