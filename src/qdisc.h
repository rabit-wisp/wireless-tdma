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
    struct rtnl_qdisc *qdisc;

    QdiscController(const std::string &iface, size_t bufferSize, bool verbose);
    ~QdiscController();

    void tx_resume();
    void tx_pause();
};

#endif // _QDISC_H_
