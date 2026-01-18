#include <iostream>
#include <pcap.h>
#include <chrono>
#include <csignal>
#include <atomic>
#include <net/if.h>

#include "docopt.h"


#include "qdisc.h"

std::atomic<bool> run{true};

void signal_handler(int signal) {
    if (signal == SIGHUP || signal == SIGINT || signal == SIGTERM) {
        run = false;
    }
}


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

    unsigned int if_index = if_nametoindex(interface.c_str());
    if (if_index == 0) {
        std::cerr << "Interface " << interface << " not found" << std::endl;
        return 1;
    }

    std::signal(SIGHUP, signal_handler);
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    char errbuf[PCAP_ERRBUF_SIZE];

    std::cout << "Starting WiFi beacon detection on interface: " << interface << std::endl;
    //std::cout << "Note: Interface must be in monitor mode!" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    std::cout << "Capturing beacon frames... (Press Ctrl+C to stop)" << std::endl;
    std::cout << std::string(80, '-') << std::endl;

    pcap_t *handle = pcap_create(interface.c_str(), errbuf);
    if (handle == nullptr) {
        std::cerr << "Error creating pcap handle: " << errbuf << std::endl;
        return 1;
    }

    // Set interface to monitor mode (rfmon)
    if (pcap_set_rfmon(handle, 1) != 0) {
        std::cerr << "Warning: Could not set monitor mode on interface" << std::endl;
    }

    // Set snapshot length
    if (pcap_set_snaplen(handle, 65535) != 0) {
        std::cerr << "Error setting snaplen" << std::endl;
        pcap_close(handle);
        return 1;
    }

    // Set promiscuous mode
    if (pcap_set_promisc(handle, 1) != 0) {
        std::cerr << "Error setting promiscuous mode" << std::endl;
        pcap_close(handle);
        return 1;
    }

    // Set timeout to minimal value
    if (pcap_set_timeout(handle, 1) != 0) {
        std::cerr << "Error setting timeout" << std::endl;
        pcap_close(handle);
        return 1;
    }

    // Set immediate mode to disable buffering for real-time delivery
    if (pcap_set_immediate_mode(handle, 1) != 0) {
        std::cerr << "Warning: Could not enable immediate mode" << std::endl;
    }

    // Set smaller buffer size for lower latency
    if (pcap_set_buffer_size(handle, 2*1024*1024) != 0) {
        std::cerr << "Warning: Could not set buffer size" << std::endl;
    }

    // Activate the handle
    int status = pcap_activate(handle);
    if (status < 0) {
        std::cerr << "Error activating pcap: " << pcap_geterr(handle) << std::endl;
        pcap_close(handle);
        return 1;
    } else if (status > 0) {
        std::cerr << "Warning: " << pcap_statustostr(status) << std::endl;
    }

    // Verify we're in monitor mode
    if (pcap_datalink(handle) != DLT_IEEE802_11_RADIO) {
        std::cerr << "Error: Interface not in monitor mode (expected radiotap headers)" << std::endl;
        std::cerr << "Current datalink type: " << pcap_datalink(handle) << std::endl;
        std::cerr << "Please enable monitor mode manually first" << std::endl;
        pcap_close(handle);
        return 1;
    }
    // Set up filter for management frames (type 0)
    struct bpf_program fp;
    const char* filter_exp = "type mgt subtype beacon";

    if (pcap_compile(handle, &fp, filter_exp, 0, PCAP_NETMASK_UNKNOWN) == -1) {
        std::cerr << "Error compiling filter: " << pcap_geterr(handle) << std::endl;
        pcap_close(handle);
        return 1;
    }

    if (pcap_setfilter(handle, &fp) == -1) {
        std::cerr << "Error setting filter: " << pcap_geterr(handle) << std::endl;
        pcap_freecode(&fp);
        pcap_close(handle);
        return 1;
    }

    pcap_freecode(&fp);

    QdiscController controller(interface,
                              slotNumber,
                              std::chrono::microseconds(slotDuration),
                              if_index,
                              verbose,
                              [handle](){ pcap_breakloop(handle);},
                              run,
                              pollCount);

    pcap_loop(handle, 0, controller.packet_handler, reinterpret_cast<u_char*>(&controller));
    pcap_close(handle);

    return 0;
}

