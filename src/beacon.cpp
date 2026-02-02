#include "beacon.h"
#include <iomanip>
#include <exception>
#include <algorithm>
#include <pcap.h>
#include <limits>
#include "ieee80211.h"

using std::chrono::operator""us;

Beacon::Beacon(std::string interface,
               std::optional<std::array<uint8_t, 6>> bssid,
               Beacon::sync_function&& func,
               bool verbose) : interface(interface),
                               bssid(bssid),
                               sync(func),
                               verbose(verbose)
{
    clock_offset_buffer.resize(50, std::chrono::microseconds(std::numeric_limits<long long>::max())); // look back 50 beacon broadcasts

    char errbuf[PCAP_ERRBUF_SIZE];

    handle = pcap_create(interface.c_str(), errbuf);
    if (handle == nullptr) {
        std::cerr << "Error creating pcap handle: " << errbuf << std::endl;
        throw std::exception();
    }

    // Set interface to monitor mode (rfmon)
    if (pcap_set_rfmon(handle, 1) != 0) {
        std::cerr << "Warning: Could not set monitor mode on interface" << std::endl;
    }

    // Set snapshot length
    if (pcap_set_snaplen(handle, 65535) != 0) {
        std::cerr << "Error setting snaplen" << std::endl;
        pcap_close(handle);
        throw std::exception();
    }

    // Set promiscuous mode
    if (pcap_set_promisc(handle, 1) != 0) {
        std::cerr << "Error setting promiscuous mode" << std::endl;
        pcap_close(handle);
        throw std::exception();
    }

    // Set timeout to minimal value
    if (pcap_set_timeout(handle, 1) != 0) {
        std::cerr << "Error setting timeout" << std::endl;
        pcap_close(handle);
        throw std::exception();
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
        throw std::exception();
    } else if (status > 0) {
        std::cerr << "Warning: " << pcap_statustostr(status) << std::endl;
    }

    // Verify we're in monitor mode
    if (pcap_datalink(handle) != DLT_IEEE802_11_RADIO) {
        std::cerr << "Error: Interface not in monitor mode (expected radiotap headers)" << std::endl;
        std::cerr << "Current datalink type: " << pcap_datalink(handle) << std::endl;
        std::cerr << "Please enable monitor mode manually first" << std::endl;
        pcap_close(handle);
        throw std::exception();
    }
    // Set up filter for management frames (type 0)
    struct bpf_program fp;
    const char* filter_exp = "type mgt subtype beacon";

    if (pcap_compile(handle, &fp, filter_exp, 0, PCAP_NETMASK_UNKNOWN) == -1) {
        std::cerr << "Error compiling filter: " << pcap_geterr(handle) << std::endl;
        pcap_close(handle);
        throw std::exception();
    }

    if (pcap_setfilter(handle, &fp) == -1) {
        std::cerr << "Error setting filter: " << pcap_geterr(handle) << std::endl;
        pcap_freecode(&fp);
        pcap_close(handle);
        throw std::exception();
    }

    pcap_freecode(&fp);

}

Beacon::~Beacon()
{
    pcap_breakloop(handle);
    worker.join();
    pcap_close(handle);
}

void Beacon::listen()
{
    worker = std::thread(Beacon::begin_worker, this);
}

void Beacon::begin_worker(Beacon* this_)
{
    pcap_loop(this_->handle, 0, Beacon::packet_handler, reinterpret_cast<u_char*>(this_));
}

//void Beacon::show_beacon(const struct pcap_pkthdr* pkthdr, const uint8_t* packet)
void Beacon::show_beacon(const uint8_t beacon[6], uint64_t rx, uint64_t tsf)
{
    static int64_t previous_diff = rx - tsf;
    int64_t this_diff = rx - tsf;

    std::cout << "beacon:       " << std::hex << std::setw(2) << std::setfill('0')
              << static_cast<int>(beacon[0]) << ":"
              << static_cast<int>(beacon[1]) << ":"
              << static_cast<int>(beacon[2]) << ":"
              << static_cast<int>(beacon[3]) << ":"
              << static_cast<int>(beacon[4]) << ":"
              << static_cast<int>(beacon[5])
              << std::dec
        //<< " | local clock: " << std::setfill(' ') << std::setw(16) << std::chrono::steady_clock::now().time_since_epoch().count() / 1000 << " µs"
              << " | local clock: " << std::setw(16)  << std::setfill(' ') << rx << " µs"
              << " | remote tsf: " << std::setw(16) << tsf << " µs"
              << " | offset: " << std::setw(16) << tsf_to_steady_clock_offset.value_or(0us).count() << " µs"
              << " | drift: " << std::setw(4)<<  (previous_diff - this_diff) << " µs"
              << std::endl;
    std::cout.flush();

    previous_diff = this_diff;
}

void Beacon::update_clock_offset(std::chrono::microseconds tsf_epoch,
                                 std::chrono::steady_clock::time_point steady_timestamp)
{
    /*
     * @tsf_epoch is maintained by the AP hardware (remote) - presumptively, it is networking hardware driven
     * @steady_timestamp corresponds to our local steady_clock representation and while it may be steady
     * it is subject to jitter (reception and cpu).
     *
     * this function maintains a running window of offsets between the remote beacon TSF and the local steady clock.
     * We cannot simply take the minimum offset ever seen because the clocks can drift away from each other in either
     * direction indefinitely.
     */

    auto diff = std::chrono::duration_cast<std::chrono::microseconds>(steady_timestamp.time_since_epoch()) - tsf_epoch;

    if (!tsf_to_steady_clock_offset) // first time initialization
    {
        tsf_to_steady_clock_offset = diff;
        std::fill(clock_offset_buffer.begin(),
                  clock_offset_buffer.end(),
                  diff);
    }

    // left shift clock_offset_buffer
    std::copy(clock_offset_buffer.begin() + 1, clock_offset_buffer.end(), clock_offset_buffer.begin());
    clock_offset_buffer.back() = diff;

    // get the running minimum
    tsf_to_steady_clock_offset = *std::min_element(clock_offset_buffer.begin(),clock_offset_buffer.end());
}

void Beacon::packet_handler_impl(std::chrono::steady_clock::time_point rx, const struct pcap_pkthdr* pkthdr, const uint8_t* packet)
{
    const radiotap_header* rtap = reinterpret_cast<const radiotap_header*>(packet);
    int rtap_len = rtap->it_len;

    // Parse 802.11 header (after radiotap)
    const ieee80211_mgmt_header* mgmt = reinterpret_cast<const ieee80211_mgmt_header*>(packet + rtap_len);

    // filter by BSSID if it is set
    if (!!bssid && bssid.value() != std::to_array(mgmt->bssid))
        return;

    // Check if this is a beacon frame (type=0, subtype=8)
    if (mgmt->fc.type == 0 && mgmt->fc.subtype == 8)
    {
        const beacon_fixed_params* beacon = reinterpret_cast<const beacon_fixed_params*>(packet + rtap_len + sizeof(ieee80211_mgmt_header));

        // uint64_t reception_time_us = (uint64_t)pkthdr->ts.tv_sec * 1000000ULL + (uint64_t)pkthdr->ts.tv_usec;
        // the hardware reception time from the packet header is less useful than one might think as it is
        // a) not readily available on embedded hardware platforms, and b) irrelevant to our needs since we are trying to
        // compute the offset between local wall clock time and remote advertised TSF time.

        update_clock_offset(std::chrono::microseconds(beacon->timestamp), rx);

        if (verbose)
            show_beacon(mgmt->bssid, rx.time_since_epoch().count() / 1000, beacon->timestamp);

        // don't synchronize if we don't yet know the offset between remote and local
        // (i.e. remote is simply a meaningless number)
        if (tsf_to_steady_clock_offset)
            sync(std::chrono::steady_clock::time_point{} + (std::chrono::microseconds(beacon->timestamp) + tsf_to_steady_clock_offset.value()),
                 beacon->beacon_interval);
    }
};

void Beacon::packet_handler(uint8_t* user, const struct pcap_pkthdr* pkthdr, const uint8_t* packet)
{
    const auto now = std::chrono::steady_clock::now();
    reinterpret_cast<Beacon*>(user)->packet_handler_impl(now, pkthdr, packet);
};
