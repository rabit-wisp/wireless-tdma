#pragma once
#ifndef _IEEE80211_H_
#define _IEEE80211_H_
#include <arpa/inet.h>

// Radiotap header structure (simplified)
struct radiotap_header {
    uint8_t it_version;
    uint8_t it_pad;
    uint16_t it_len;
    uint32_t it_present;
} __attribute__((packed));

// 802.11 frame control
struct frame_control {
    uint8_t protocol_version : 2;
    uint8_t type : 2;
    uint8_t subtype : 4;
    uint8_t flags;
} __attribute__((packed));

// 802.11 management frame header
struct ieee80211_mgmt_header {
    struct frame_control fc;
    uint16_t duration;
    uint8_t da[6];  // destination address
    uint8_t sa[6];  // source address (BSSID for beacons)
    uint8_t bssid[6];
    uint16_t seq_ctrl;
} __attribute__((packed));

// Beacon frame fixed parameters
struct beacon_fixed_params {
    uint64_t timestamp;
    uint16_t beacon_interval;
    uint16_t capability_info;
} __attribute__((packed));

/*
// Function to print MAC address
void print_mac(const uint8_t* mac) {
    for (int i = 0; i < 6; i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<int>(mac[i]);
        if (i < 5) std::cout << ":";
    }
    std::cout << std::dec;
}



// Parse SSID from tagged parameters
std::string parse_ssid(const uint8_t* data, int len) {
    int pos = 0;
    while (pos < len) {
        uint8_t tag_number = data[pos];
        uint8_t tag_length = data[pos + 1];

        if (tag_number == 0) { // SSID tag
            if (tag_length > 0 && pos + 2 + tag_length <= len) {
                return std::string(reinterpret_cast<const char*>(&data[pos + 2]), tag_length);
            }
            return "<Hidden>";
        }

        pos += 2 + tag_length;
        if (pos >= len) break;
    }
    return "<Unknown>";
}
*/

#endif // _IEEE80211_H_
