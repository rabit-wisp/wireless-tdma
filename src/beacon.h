#pragma once
#ifndef _BEACON_H_

#include <iostream>
#include <chrono>
#include <functional>
#include <thread>
#include <pcap.h>


struct Beacon {

    using sync_function = std::function<void(std::chrono::time_point<std::chrono::system_clock>,std::chrono::microseconds)>;
    pcap_t *handle;
    sync_function& sync;

    std::string interface;

    std::thread worker;

    bool verbose;

    Beacon(std::string interface, sync_function&& synchronize, bool verbose);
    ~Beacon();

    void listen();

    static void begin_worker(Beacon* this_);
    static void packet_handler(uint8_t* user, const struct pcap_pkthdr* pkthdr, const uint8_t* packet);
};


#endif // _BEACON_H_
