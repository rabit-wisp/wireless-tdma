#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <pcap.h>
#include <net/if.h>
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <netlink/route/qdisc.h>
#include <netlink/route/qdisc/plug.h>
#include <netlink/route/qdisc/netem.h>
#include <netlink/route/tc.h>
#include <netlink/route/link.h>
#include <netlink/cache.h>
#include <linux/nl80211.h>
#include <chrono>
#include <thread>
#include "docopt.h"

#include "ieee80211.h"

using std::chrono::operator""us;
using std::chrono::operator""s;

//bool verbose = false;
size_t pollCount = 0;
pcap_t *handle = nullptr;

static const char USAGE[] =
R"(Cooperative TDMA test

Usage:
  tdma-test <INTERFACE> <SLOT> [<DURATION>] [<COUNT>] [--verbose]

Options:
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

std::chrono::time_point<std::chrono::high_resolution_clock> last_beacon_time;
std::chrono::microseconds beacon_epoch_offset;

class TDMAController {
public:
    int slot_number;
    std::chrono::microseconds slot_duration;

private:
    std::string interface;
    struct nl_sock* route_sock;
    //struct nl_cache* link_cache;
    int if_index;
    bool tx_enabled;

    struct rtnl_qdisc *qdisc;

    /*
      struct nl_dump_params dp = {
        .dp_type = NL_DUMP_DETAILS,
        .dp_fd = stdout,
    };
    struct nl_cli_tc_module *tm;
    struct rtnl_tc_ops *ops;
    */


public:
    TDMAController(const std::string& iface, int slot, std::chrono::microseconds duration,  int ifindex) : interface(iface),
                                                                                                           slot_number(slot),
                                                                                                           slot_duration(duration),
                                                                                                           if_index(ifindex),
                                                                                                           tx_enabled(true)
    {

        route_sock = nl_socket_alloc();
        if (!route_sock) {
            std::cerr << "Failed to allocate netlink socket" << std::endl;
            return;
        }

        if (nl_connect(route_sock, NETLINK_ROUTE) < 0) {
            std::cerr << "Failed to connect to netlink route" << std::endl;
            nl_socket_free(route_sock);
            route_sock = nullptr;
            return;
        }

        qdisc = rtnl_qdisc_alloc();
        if (!qdisc) {
            std::cerr << "Failed to allocated qdisc" << std::endl;
            return;
        }

        rtnl_tc_set_ifindex(TC_CAST(qdisc), if_index);
        rtnl_tc_set_parent(TC_CAST(qdisc), TC_H_ROOT);

        int kind_err = rtnl_tc_set_kind(TC_CAST(qdisc), "plug");
        if (kind_err < 0) {
            std::cerr << "Failed to allocated plug type qdisc: " << nl_geterror(kind_err) << std::endl;
            rtnl_qdisc_put(qdisc);
            return;
        }

        rtnl_qdisc_plug_set_limit(qdisc, 10240);
        rtnl_qdisc_plug_release_indefinite(qdisc); // start the qdisc in released mode

        int err = rtnl_qdisc_add(route_sock, qdisc, NLM_F_CREATE | NLM_F_REPLACE);
        if (err < 0) {
            std::cerr << "Failed to add qdisc: " << nl_geterror(err) << std::endl;
            rtnl_qdisc_put(qdisc);
            return;
        }

        std::cout << "TDMA controller initialized for " << interface << std::endl;
    }

    ~TDMAController() {

        std::cerr << "Removing qdisc from interface " << interface << std::endl;
        int err = rtnl_qdisc_delete(route_sock, qdisc);
        if (err < 0) {
            std::cerr << "Failed to remove qdisc: " << nl_geterror(err) << std::endl;
        }

        rtnl_qdisc_put(qdisc);

        if (route_sock) {
            nl_socket_free(route_sock);
        }
    }

    // tc qdisc change dev wlan0 root netem delay 0ms limit 10000
    void tx_resume()
    {
        if (!route_sock) return;
        if (tx_enabled) return;

        rtnl_qdisc_plug_release_indefinite(qdisc);
        int err = rtnl_qdisc_update(route_sock, qdisc, qdisc, NLM_F_REPLACE);

        if (err < 0) {
            std::cerr << "Failed to enable TX: " << nl_geterror(err) << " (" << err << ")" << std::endl;
        } else {
            tx_enabled = true;
            //std::cout << "[SLOT " << slot_number << "] TX ENABLED (released)" << std::endl;
        }
    }

// tc qdisc change dev wlan0 root plug block
    void tx_pause()
    {
        if (!route_sock) return;
        if (!tx_enabled) return;

        rtnl_qdisc_plug_buffer(qdisc);
        int err = rtnl_qdisc_update(route_sock, qdisc, qdisc, NLM_F_REPLACE);

        if (err < 0) {
            std::cerr << "Failed to pause TX: " << nl_geterror(err) << " (" << err << ")" << std::endl;
        } else {
            tx_enabled = false;
            //std::cout << "[SLOT " << slot_number << "] TX PAUSED (buffered)" << std::endl;
        }
    }
};

// Packet handler callback
void packet_handler(uint8_t* user, const struct pcap_pkthdr* pkthdr, const uint8_t* packet) {
    // Parse radiotap header
    if (pollCount == 0)
        pcap_breakloop(handle);

    pollCount--;

    TDMAController* controller = reinterpret_cast<TDMAController*>(user);
    const radiotap_header* rtap = reinterpret_cast<const radiotap_header*>(packet);
    int rtap_len = rtap->it_len;

    // Parse 802.11 header (after radiotap)
    const ieee80211_mgmt_header* mgmt = 
        reinterpret_cast<const ieee80211_mgmt_header*>(packet + rtap_len);

    // Check if this is a beacon frame (type=0, subtype=8)
    if (mgmt->fc.type == 0 && mgmt->fc.subtype == 8) {
        // Parse beacon body
        const beacon_fixed_params* beacon = 
            reinterpret_cast<const beacon_fixed_params*>(
                packet + rtap_len + sizeof(ieee80211_mgmt_header));

        // Calculate reception time in microseconds
        uint64_t reception_time_us = (uint64_t)pkthdr->ts.tv_sec * 1000000ULL + 
                                      (uint64_t)pkthdr->ts.tv_usec;

        // Get TSF timestamp (already in microseconds)
        uint64_t tsf_time_us = beacon->timestamp;

        static int64_t previous_diff = reception_time_us - tsf_time_us;
        int64_t this_diff = reception_time_us - tsf_time_us;

        uint16_t beacon_interval_tu = beacon->beacon_interval;
        auto beacon_interval = std::chrono::microseconds(beacon_interval_tu * 1024);

        // Print beacon information (concise format)
        std::cout << "BSSID: ";
        print_mac(mgmt->bssid);
        std::cout << " | RX_Time: " << std::setw(16) << reception_time_us << " µs"
                  << " | TSF: " << std::setw(16) << tsf_time_us << " µs"
                  << " | drift: " << (previous_diff - this_diff) << " µs"
                  << std::endl;
        std::cout.flush(); // Force immediate output

        previous_diff = this_diff;

        // TODO: adapt this to TU length
        for (int i = 0; i < 20 ; i++ )
        {
            controller->tx_pause();

            //std::this_thread::sleep_until(now + (controller->slot_duration * controller->slot_number));
            std::this_thread::sleep_for((controller->slot_duration * controller->slot_number));

            controller->tx_resume();

            std::this_thread::sleep_for(controller->slot_duration);

            controller->tx_pause();

            //std::cout << ".";
        }
        //std::cout << std::endl;
    }
}

int main(int argc, const char* argv[])
{
    auto args = docopt::docopt(USAGE, {argv + 1, argv + argc});
    std::string interface = args["<INTERFACE>"].asString();
    size_t slotNumber = args["<SLOT>"].asLong();
    size_t slotDuration = args["<DURATION>"].asLong();
    pollCount = args["<COUNT>"].asLong();

    unsigned int if_index = if_nametoindex(interface.c_str());
    if (if_index == 0) {
        std::cerr << "Interface " << interface << " not found" << std::endl;
        return 1;
    }


    char errbuf[PCAP_ERRBUF_SIZE];

    std::cout << "Starting WiFi beacon detection on interface: " << interface << std::endl;
    std::cout << "Note: Interface must be in monitor mode!" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    std::cout << "Capturing beacon frames... (Press Ctrl+C to stop)" << std::endl;
    std::cout << std::string(80, '-') << std::endl;

    // Open the interface for monitor mode capture
    // Using pcap_create and pcap_activate for explicit monitor mode control
    handle = pcap_create(interface.c_str(), errbuf);
    if (handle == nullptr) {
        std::cerr << "Error creating pcap handle: " << errbuf << std::endl;
        return 1;
    }

    // Set interface to monitor mode (rfmon)
    if (pcap_set_rfmon(handle, 1) != 0) {
        std::cerr << "Warning: Could not set monitor mode on interface" << std::endl;
        std::cerr << "Make sure interface is already in monitor mode" << std::endl;
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

    std::cout << "Monitor mode confirmed (datalink: DLT_IEEE802_11_RADIO)" << std::endl;

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

    TDMAController controller(interface, slotNumber, std::chrono::microseconds(slotDuration), if_index);

    // Start packet capture loop
    pcap_loop(handle, 0, packet_handler, reinterpret_cast<u_char*>(&controller));

    // Cleanup
    pcap_close(handle);


    return 0;
}

