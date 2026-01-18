#pragma once
#ifndef _QDISC_H_
#define _QDISC_H_

#include <functional>
#include <atomic>
#include <iostream>
#include <iomanip>
#include <string>
#include <unistd.h>
#include <pcap.h>
#include <net/if.h>
#include <netlink/route/qdisc/plug.h>

#include "ieee80211.h"

struct QdiscController
{
    int slot_number;
    std::chrono::microseconds slot_duration;

    std::string interface;
    struct nl_sock* socket;
    int if_index;
    bool verbose;
    bool tx_enabled;
    struct rtnl_qdisc *qdisc;

    std::function<void()> terminate;
    std::optional<size_t> count;
    std::atomic<bool>& run;
    pcap_t *handle = nullptr;

    QdiscController(const std::string &iface, int slot,
                    std::chrono::microseconds duration, int ifindex,
                    bool verbose, std::function<void()> terminate,
                    std::atomic<bool> &run, std::optional<size_t> count);
    ~QdiscController();

    void tx_resume();
    void tx_pause();
    static void packet_handler(uint8_t* user, const struct pcap_pkthdr* pkthdr, const uint8_t* packet);
};

#endif // _QDISC_H_
