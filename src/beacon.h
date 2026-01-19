#pragma once
#ifndef _BEACON_H_

#include <iostream>
#include <chrono>
#include <functional>
#include <thread>
#include <pcap.h>


struct Beacon {

    // Note: the beacon class uses static variables and is meant to be used as a singleton only

    using sync_function = std::function<void(std::chrono::time_point<std::chrono::steady_clock>,size_t)>;
    sync_function& sync;
    std::string interface;
    bool verbose;

    pcap_t *handle;
    std::thread worker;

    std::optional<std::chrono::microseconds> tsf_to_steady_clock_offset; // offset between beacon TSF and std::chrono::steady_clock
    std::vector<std::chrono::microseconds> clock_offset_buffer;

    Beacon(std::string interface, sync_function&& synchronize, bool verbose);
    ~Beacon();

    void listen();
    void show_beacon(const uint8_t bssid[6], uint64_t rx, uint64_t tsf);
    void packet_handler_impl(std::chrono::steady_clock::time_point rx, const struct pcap_pkthdr* pkthdr, const uint8_t* packet);

    void update_clock_offset(std::chrono::microseconds tsf, std::chrono::steady_clock::time_point steady_ts);

    static void begin_worker(Beacon* this_);
    static void packet_handler(uint8_t* user, const struct pcap_pkthdr* pkthdr, const uint8_t* packet);
};


#endif // _BEACON_H_
