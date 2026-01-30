#include <mutex>
#include <thread>
#include <net/if.h>

#include "qdisc.h"

QdiscController::QdiscController(const std::string& iface,
                                 size_t bufferSize,
                                 bool verbose_) : interface(iface),
                                                  verbose(verbose_),
                                                  tx_enabled(true)
{
    if_index = if_nametoindex(interface.c_str());
    if (if_index == 0) {
        std::cerr << "Interface " << interface << " not found" << std::endl;
        throw std::exception();
    }

    socket = nl_socket_alloc();
    if (!socket) {
        std::cerr << "Failed to allocate netlink socket" << std::endl;
        throw std::exception();
    }

    if (nl_connect(socket, NETLINK_ROUTE) < 0) {
        std::cerr << "Failed to connect to netlink route" << std::endl;
        nl_socket_free(socket);
        socket = nullptr;
        throw std::exception();
    }

    qdisc = rtnl_qdisc_alloc();
    if (!qdisc) {
        std::cerr << "Failed to allocated qdisc" << std::endl;
        throw std::exception();
    }

    int kind_err = rtnl_tc_set_kind(TC_CAST(qdisc), "plug");
    if (kind_err < 0) {
        std::cerr << "Failed to allocated plug type qdisc: " << nl_geterror(kind_err) << std::endl;
        rtnl_qdisc_put(qdisc);
        throw std::exception();
    }


    rtnl_tc_set_ifindex(TC_CAST(qdisc), if_index);
    rtnl_tc_set_parent(TC_CAST(qdisc), TC_H_ROOT);
    rtnl_qdisc_plug_set_limit(qdisc, bufferSize);
    rtnl_qdisc_plug_release_indefinite(qdisc); // start the qdisc in released mode


    int err = rtnl_qdisc_add(socket, qdisc, NLM_F_CREATE | NLM_F_REPLACE);
    if (err < 0) {
        std::cerr << "Failed to add qdisc: " << nl_geterror(err) << std::endl;
        rtnl_qdisc_put(qdisc);
        throw std::exception();
    }

    std::cout << "TDMA controller initialized for " << interface << std::endl;
}

QdiscController::~QdiscController()
{
    std::cerr << "Removing qdisc from interface " << interface << std::endl;
    int err = rtnl_qdisc_delete(socket, qdisc);
    if (err < 0) {
        std::cerr << "Failed to remove qdisc: " << nl_geterror(err) << std::endl;
    }

    rtnl_qdisc_put(qdisc);

    if (socket) {
        nl_socket_free(socket);
    }
}

void QdiscController::tx_resume()
{

    rtnl_qdisc_plug_release_indefinite(qdisc);
    int err = rtnl_qdisc_update(socket, qdisc, qdisc, NLM_F_REPLACE);

    if (err < 0) {
        std::cerr << "Failed to enable TX: " << nl_geterror(err) << " (" << err
                  << ")" << std::endl;
    } else {
        tx_enabled = true;
        //std::cout << "[SLOT " << slot_number << "] TX ENABLED (released)" << std::endl;
    }
}

// tc qdisc change dev wlan0 root plug block
void QdiscController::tx_pause()
{

    rtnl_qdisc_plug_buffer(qdisc);
    int err = rtnl_qdisc_update(socket, qdisc, qdisc, NLM_F_REPLACE);

    if (err < 0) {
        std::cerr << "Failed to pause TX: " << nl_geterror(err) << " (" << err
                  << ")" << std::endl;
    } else {
        tx_enabled = false;
        //std::cout << "[SLOT " << slot_number << "] TX PAUSED (buffered)" << std::endl;
    }
}
