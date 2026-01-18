#include "beacon.h"
#include <exception>
#include <pcap.h>
#include "ieee80211.h"

Beacon::Beacon(std::string interface, Beacon::sync_function&& func, bool verbose) : interface(interface),
                                                                                     sync(func),
                                                                                     verbose(verbose)
{
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
    std::cout << "Starting WiFi beacon detection on interface: " << this_->interface << std::endl;
    //std::cout << "Note: Interface must be in monitor mode!" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    std::cout << "Capturing beacon frames... (Press Ctrl+C to stop)" << std::endl;
    std::cout << std::string(80, '-') << std::endl;

    pcap_loop(this_->handle, 0, Beacon::packet_handler, reinterpret_cast<u_char*>(this_));
}

void Beacon::packet_handler(uint8_t* user, const struct pcap_pkthdr* pkthdr, const uint8_t* packet)
{
    Beacon* this_ = reinterpret_cast<Beacon*>(user);

    const radiotap_header* rtap = reinterpret_cast<const radiotap_header*>(packet);
    int rtap_len = rtap->it_len;

    // Parse 802.11 header (after radiotap)
    const ieee80211_mgmt_header* mgmt = reinterpret_cast<const ieee80211_mgmt_header*>(packet + rtap_len);
    // Check if this is a beacon frame (type=0, subtype=8)
    if (mgmt->fc.type == 0 && mgmt->fc.subtype == 8)
    {
        const beacon_fixed_params* beacon = reinterpret_cast<const beacon_fixed_params*>(packet + rtap_len + sizeof(ieee80211_mgmt_header));
        uint64_t reception_time_us = (uint64_t)pkthdr->ts.tv_sec * 1000000ULL + (uint64_t)pkthdr->ts.tv_usec;
        auto seconds = std::chrono::seconds(pkthdr->ts.tv_sec);
        auto useconds = std::chrono::microseconds(pkthdr->ts.tv_usec);
        uint64_t tsf_time_us = beacon->timestamp;

        uint16_t beacon_interval_tu = beacon->beacon_interval;
        auto beacon_interval = std::chrono::microseconds(beacon_interval_tu * 1024);
        
        if (this_->verbose)
        {
            static int64_t previous_diff = reception_time_us - tsf_time_us;
            int64_t this_diff = reception_time_us - tsf_time_us;

            std::cout << "BSSID: " << std::hex << std::setw(2) << std::setfill('0')
                      << static_cast<int>(mgmt->bssid[0]) << ":"
                      << static_cast<int>(mgmt->bssid[1]) << ":"
                      << static_cast<int>(mgmt->bssid[2]) << ":"
                      << static_cast<int>(mgmt->bssid[3]) << ":"
                      << static_cast<int>(mgmt->bssid[4]) << ":"
                      << static_cast<int>(mgmt->bssid[5])
                      << std::dec
                      << " | time: " << std::setw(16) << std::chrono::steady_clock::now().time_since_epoch().count() / 1000 << " µs"
                      << " | rx: " << std::setw(16) << reception_time_us << " µs"
                      << " | tsf: " << std::setw(16) << tsf_time_us << " µs"
                      << " | drift: " << (previous_diff - this_diff) << " µs"
                      << std::endl;
            std::cout.flush();

            previous_diff = this_diff;
        }

        this_->sync(std::chrono::system_clock::time_point(seconds + useconds), beacon_interval);
    }
};
