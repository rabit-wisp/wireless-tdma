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

// 802.11 management frame header
struct ieee80211_mgmt_header {
    uint16_t frame_control;
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

#endif // _IEEE80211_H_
