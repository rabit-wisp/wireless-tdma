#pragma once
#ifndef _QDISC_H_
#define _QDISC_H_

#include <atomic>
#include <functional>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <net/if.h>
#include <netlink/route/qdisc/plug.h>
#include <string>
#include <unistd.h>


struct QdiscController
{
    std::string interface;
    struct nl_sock* socket;
    int if_index;
    bool verbose;
    bool tx_enabled;
    std::mutex mutex;
    struct rtnl_qdisc *qdisc;

    QdiscController(const std::string &iface, bool verbose);
    ~QdiscController();

    void tx_resume();
    void tx_pause();
    //static void packet_handler(uint8_t* user, const struct pcap_pkthdr* pkthdr, const uint8_t* packet);
};

#endif // _QDISC_H_
