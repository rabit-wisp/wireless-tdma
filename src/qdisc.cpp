#include <mutex>
#include <thread>

#include "qdisc.h"
#include "ieee80211.h"

QdiscController::QdiscController(const std::string& iface,
                               int slot,
                               std::chrono::microseconds duration,
                               int ifindex,
                               bool verbose_,
                               std::function<void()> terminate_,
                               std::atomic<bool>& run_,
                               std::optional<size_t> count_) : interface(iface),
                                                               slot_number(slot),
                                                               slot_duration(duration),
                                                               verbose(verbose_),
                                                               run(run_),
                                                               count(count_),
                                                               terminate(terminate_),
                                                               if_index(ifindex),
                                                               tx_enabled(true)
{
    socket = nl_socket_alloc();
    if (!socket) {
        std::cerr << "Failed to allocate netlink socket" << std::endl;
        return;
    }

    if (nl_connect(socket, NETLINK_ROUTE) < 0) {
        std::cerr << "Failed to connect to netlink route" << std::endl;
        nl_socket_free(socket);
        socket = nullptr;
        return;
    }

    qdisc = rtnl_qdisc_alloc();
    if (!qdisc) {
        std::cerr << "Failed to allocated qdisc" << std::endl;
        return;
    }

    int kind_err = rtnl_tc_set_kind(TC_CAST(qdisc), "plug");
    if (kind_err < 0) {
        std::cerr << "Failed to allocated plug type qdisc: " << nl_geterror(kind_err) << std::endl;
        rtnl_qdisc_put(qdisc);
        return;
    }


    rtnl_tc_set_ifindex(TC_CAST(qdisc), if_index);
    rtnl_tc_set_parent(TC_CAST(qdisc), TC_H_ROOT);
    rtnl_qdisc_plug_set_limit(qdisc, 10240);
    rtnl_qdisc_plug_release_indefinite(qdisc); // start the qdisc in released mode


    int err = rtnl_qdisc_add(socket, qdisc, NLM_F_CREATE | NLM_F_REPLACE);
    if (err < 0) {
        std::cerr << "Failed to add qdisc: " << nl_geterror(err) << std::endl;
        rtnl_qdisc_put(qdisc);
        return;
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
    std::lock_guard<std::mutex> guard(mutex);
    if (!socket) return;
    if (tx_enabled) return;

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
    std::lock_guard<std::mutex> guard(mutex);
    if (!socket) return;
    if (!tx_enabled) return;

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

    // Packet handler callback
void QdiscController::packet_handler(uint8_t* user, const struct pcap_pkthdr* pkthdr, const uint8_t* packet)
{
    QdiscController* controller = reinterpret_cast<QdiscController*>(user);

    if ((controller->count && *controller->count == 0) || !controller->run)
        controller->terminate();

    (*controller->count)-=1;


    const radiotap_header* rtap = reinterpret_cast<const radiotap_header*>(packet);
    int rtap_len = rtap->it_len;

    // Parse 802.11 header (after radiotap)
    const ieee80211_mgmt_header* mgmt = reinterpret_cast<const ieee80211_mgmt_header*>(packet + rtap_len);
    // Check if this is a beacon frame (type=0, subtype=8)
    if (mgmt->fc.type == 0 && mgmt->fc.subtype == 8)
    {
        const beacon_fixed_params* beacon = reinterpret_cast<const beacon_fixed_params*>(packet + rtap_len + sizeof(ieee80211_mgmt_header));
        uint64_t reception_time_us = (uint64_t)pkthdr->ts.tv_sec * 1000000ULL + (uint64_t)pkthdr->ts.tv_usec;
        uint64_t tsf_time_us = beacon->timestamp;

        if (controller->verbose)
        {
            static int64_t previous_diff = reception_time_us - tsf_time_us;
            int64_t this_diff = reception_time_us - tsf_time_us;

            uint16_t beacon_interval_tu = beacon->beacon_interval;
            auto beacon_interval = std::chrono::microseconds(beacon_interval_tu * 1024);

            std::cout << "BSSID: " << std::hex << std::setw(2) << std::setfill('0')
                      << static_cast<int>(mgmt->bssid[0]) << ":"
                      << static_cast<int>(mgmt->bssid[1]) << ":"
                      << static_cast<int>(mgmt->bssid[2]) << ":"
                      << static_cast<int>(mgmt->bssid[3]) << ":"
                      << static_cast<int>(mgmt->bssid[4]) << ":"
                      << static_cast<int>(mgmt->bssid[5])
                      << std::dec
                      << " | rx: " << std::setw(16) << reception_time_us << " µs"
                      << " | tsf: " << std::setw(16) << tsf_time_us << " µs"
                      << " | drift: " << (previous_diff - this_diff) << " µs"
                      << std::endl;
            std::cout.flush();

            previous_diff = this_diff;
        }

        // TODO: adapt this to TU length
        for (int i = 0; i < 20 ; i++ )
        {
            controller->tx_pause();
            //std::this_thread::sleep_until(now + (controller->slot_duration * controller->slot_number));
            std::this_thread::sleep_for((controller->slot_duration * controller->slot_number));

            controller->tx_resume();

            std::this_thread::sleep_for(controller->slot_duration);

            controller->tx_pause();
        }
    }
};
