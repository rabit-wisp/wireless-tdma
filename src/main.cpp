#include <iostream>
#include <chrono>
#include <csignal>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <ranges>

#include "docopt.h"
#include "tdma.h"
#include "beacon.h"
#include "qdisc.h"

using std::chrono::operator""us;

static const char USAGE[] =
R"(Cooperative TDMA scheduler

Usage:
  tc-tdma <INTERFACE> <SLOTS>... (--bssid=BSSID|--AP-mode) [options]
  tc-tdma <INTERFACE> --show-beacons [--bssid=BSSID]

Options:
  <INTERFACE>                name of interface to attach to (e.g. wlan0)
  <SLOTS>                    ordinal 0-based index of TDMA slot
  --AP-mode                  run on access point
  --bssid=BSSID              BSSID to synchronize with (in XX:XX:XX:XX:XX:XX format)
  --show-beacons             don't do TDMA, simply show beacon frame statistics
  --slots-per-frame=SLOTS    number of slots per TDMA frame [default: 10]
  --frame-TUs=TUs            Time Units for each TDMA frame [default: 10]
  --buffer-size=SIZE         number of packets to buffer during plug period [default: 10240]
  --count=COUNT              if specified, only run for specified number of TDMA frames
  --beacon-timeout=TIMEOUT   exit with error if first beacon doesn't arrive within timeout
  --system-jitter=JITTER     expected system jitter in µs [default: 10]
  -v --verbose               show beacons and various things

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

class comma_numpunct : public std::numpunct<char> {
protected:
    virtual char do_thousands_sep() const { return ','; }
    virtual std::string do_grouping() const { return "\03"; } // Group by 3
};

int show_beacon_stats(const std::string& interface, std::optional<std::array<uint8_t, 6>> bssid, const bool ap_mode)
{
    Beacon beacon(interface, bssid, [&](auto ts, auto... args){}, true);

    std::cout << "Starting WiFi beacon detection on interface: " << interface << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    std::cout << "Capturing beacon frames... (Press Ctrl+C to stop)" << std::endl;
    std::cout << std::string(80, '-') << std::endl;

    beacon.listen();

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

void set_thread_priority_high()
{
    pthread_t thread = pthread_self();

    // Set scheduling policy to FIFO (real-time)
    struct sched_param params;
    params.sched_priority = sched_get_priority_max(SCHED_FIFO);

    int result = pthread_setschedparam(thread, SCHED_FIFO, &params);
    if (result != 0) {
        std::cerr << "Failed to set thread priority: " << result << std::endl;
    }
}

std::vector<TDMAScheduler::slot_info> getSlotNumbers(const std::vector<std::string>& slots)
{
    if (slots.empty()) return {};

    std::vector<size_t> indices(slots.size());
    std::transform(slots.begin(),
                   slots.end(),
                   indices.begin(),
                   [](const auto& a) {
                       return std::stoul(a);
                   });

    // Merge consecutive indices into spans
    std::vector<TDMAScheduler::slot_info> results;
    size_t span_start = indices[0];
    size_t span_length = 1;

    for (size_t i = 1; i < indices.size(); i++) {
        if (indices[i] == indices[i-1] + 1) {
            // Consecutive index, extend the span
            ++span_length;
        } else {
            // Gap found, save current span and start new one
            results.emplace_back(span_start, span_length);
            span_start = indices[i];
            span_length = 1;
        }
    }
    // Add the last span
    results.emplace_back(span_start, span_length);

    return results;
}

int main(int argc, const char* argv[])
{
    auto args = docopt::docopt(USAGE, {argv + 1, argv + argc});

    std::locale comma_locale(std::locale(), new comma_numpunct());
    std::cout.imbue(comma_locale);
    const bool ap_mode = args["--AP-mode"].asBool();
    const bool beacon_only = args["--show-beacons"].asBool();
    const std::string interface = args["<INTERFACE>"].asString();
    const auto slots = beacon_only ? std::vector<TDMAScheduler::slot_info>{} : getSlotNumbers(args["<SLOTS>"].asStringList());
    const size_t slotsPerFrame = args["--slots-per-frame"].asLong();
    const size_t frameTUs = args["--frame-TUs"].asLong();
    const size_t jitter = args["--system-jitter"].asLong();
    const auto frameDuration = std::chrono::microseconds(frameTUs * 1024);
    std::optional<std::array<uint8_t, 6>> bssid;

    if (!slots.empty() &&
        !ap_mode &&
        slots[0].index == 0 &&
        (slots.back().index + slots.back().length) == slotsPerFrame)
    {
        std::cout << "Error: found slots wrapping around frame - this is only allowed in AP mode" << std::endl;
        return 1;
    }

    if( std::any_of(slots.begin(),
                    slots.end(),
                    [slotsPerFrame](const auto& a){ return a.index >= slotsPerFrame; }))
    {
        std::cout << "slots must be les than the total number of --slots-per-frame " << std::endl;
        return 1;
    }

    if (args["--bssid"]) {
        const auto bssidString = args["--bssid"].asString();
        if (bssidString.length() != 17) {
            std::cout << "BSSID must be specified in XX:XX:XX:XX:XX:XX format. Got " << bssidString << " (1)." << std::endl;
            return 1;
        }
        auto tokens = bssidString
            | std::views::split(':')
            | std::views::transform([](auto &&rng) {
                return std::string(rng.begin(), rng.end());
            });

        if (std::ranges::distance(tokens) != 6 ) {
            std::cout << "BSSID must be specified in XX:XX:XX:XX:XX:XX format. Got "
                      << bssidString << " (2)." << std::endl;
          return 2;
        }

        if (!std::all_of(tokens.begin(), tokens.end(), [](auto a) { return a.length() == 2; })) {
                std::cout << "BSSID must be specified in XX:XX:XX:XX:XX:XX format. Got "
                      << bssidString << " (3)." << std::endl;
            return 3;
        }

        bssid.emplace();
        std::transform(tokens.begin(),
                       tokens.end(),
                       bssid.value().begin(),
                       [](const std::string &s) { return static_cast<uint8_t>(std::stoi(s, nullptr, 16));});
    }

    using ms = std::chrono::milliseconds;
    std::optional<ms> timeout = (args["--beacon-timeout"] ?
                                 std::optional<ms>(ms(args["--beacon-timeout"].asLong())) : std::nullopt);
    std::optional<size_t> pollCount = args["--count"] ? std::optional<size_t>(args["--count"].asLong()) : std::nullopt;
    bool verbose = args["--verbose"].asBool();

    try
    {
        if (beacon_only)
            return show_beacon_stats(interface, bssid, ap_mode);

        QdiscController plug(interface, args["--buffer-size"].asLong(), verbose); // qdisc plug controller

        std::condition_variable first_beacon;
        std::mutex mutex;
        std::optional<TDMAScheduler::timestamp> firstFrame;

        // 80211 BSS beacon broadcast listener
        Beacon beacon( interface,
                       bssid,
                       [&](auto ts, auto... args) {
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

        static TDMAScheduler scheduler(slots,
                                       slotsPerFrame,
                                       firstFrame.value_or(TDMAScheduler::timestamp::clock::now()),
                                       frameDuration,
                                       std::chrono::microseconds(jitter),
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

        set_thread_priority_high();
        scheduler.run();

    } catch ( std::exception& e ) {

        std::cerr << "exiting on unhandled exception: " << e.what() << std::endl;
    }

    return 0;
}

