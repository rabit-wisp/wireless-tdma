# Performance Results

## beacon frame drift

```bash

root@radio-backhaul-002:~# ./tc-tdma wlan0  --show-beacons --bssid=f4:1e:57:e8:61:90
Starting WiFi beacon detection on interface: wlan0
================================================================================
Capturing beacon frames... (Press Ctrl+C to stop)
--------------------------------------------------------------------------------
beacon:       f4:1e:57:e8:61:90 | local clock:  541,798,719,616 µs | remote tsf:  694,859,980,859 µs | offset: -153,061,261,243 µs | drift:    0 µs
beacon:       f4:1e:57:e8:61:90 | local clock:  541,798,822,143 µs | remote tsf:  694,860,083,304 µs | offset: -153,061,261,243 µs | drift:  -82 µs
beacon:       f4:1e:57:e8:61:90 | local clock:  541,798,924,469 µs | remote tsf:  694,860,185,659 µs | offset: -153,061,261,243 µs | drift:   29 µs
beacon:       f4:1e:57:e8:61:90 | local clock:  541,799,026,827 µs | remote tsf:  694,860,288,059 µs | offset: -153,061,261,243 µs | drift:   42 µs
beacon:       f4:1e:57:e8:61:90 | local clock:  541,799,129,482 µs | remote tsf:  694,860,390,633 µs | offset: -153,061,261,243 µs | drift:  -81 µs
beacon:       f4:1e:57:e8:61:90 | local clock:  541,799,233,291 µs | remote tsf:  694,860,494,507 µs | offset: -153,061,261,243 µs | drift:   65 µs
beacon:       f4:1e:57:e8:61:90 | local clock:  541,799,334,181 µs | remote tsf:  694,860,595,267 µs | offset: -153,061,261,243 µs | drift: -130 µs
beacon:       f4:1e:57:e8:61:90 | local clock:  541,799,436,342 µs | remote tsf:  694,860,697,659 µs | offset: -153,061,261,317 µs | drift:  231 µs
beacon:       f4:1e:57:e8:61:90 | local clock:  541,799,539,807 µs | remote tsf:  694,860,801,061 µs | offset: -153,061,261,317 µs | drift:  -63 µs
beacon:       f4:1e:57:e8:61:90 | local clock:  541,799,641,264 µs | remote tsf:  694,860,902,459 µs | offset: -153,061,261,317 µs | drift:  -59 µs
beacon:       f4:1e:57:e8:61:90 | local clock:  541,799,743,801 µs | remote tsf:  694,861,004,951 µs | offset: -153,061,261,317 µs | drift:  -45 µs
beacon:       f4:1e:57:e8:61:90 | local clock:  541,799,846,234 µs | remote tsf:  694,861,107,463 µs | offset: -153,061,261,317 µs | drift:   79 µs
beacon:       f4:1e:57:e8:61:90 | local clock:  541,799,948,818 µs | remote tsf:  694,861,210,068 µs | offset: -153,061,261,317 µs | drift:   21 µs
beacon:       f4:1e:57:e8:61:90 | local clock:  541,800,050,958 µs | remote tsf:  694,861,312,059 µs | offset: -153,061,261,317 µs | drift: -149 µs
beacon:       f4:1e:57:e8:61:90 | local clock:  541,800,153,660 µs | remote tsf:  694,861,414,799 µs | offset: -153,061,261,317 µs | drift:   38 µs
beacon:       f4:1e:57:e8:61:90 | local clock:  541,800,255,816 µs | remote tsf:  694,861,517,051 µs | offset: -153,061,261,317 µs | drift:   96 µs
beacon:       f4:1e:57:e8:61:90 | local clock:  541,800,460,382 µs | remote tsf:  694,861,721,659 µs | offset: -153,061,261,317 µs | drift:   42 µs
beacon:       f4:1e:57:e8:61:90 | local clock:  541,800,562,740 µs | remote tsf:  694,861,824,100 µs | offset: -153,061,261,360 µs | drift:   83 µs
beacon:       f4:1e:57:e8:61:90 | local clock:  541,800,665,564 µs | remote tsf:  694,861,926,837 µs | offset: -153,061,261,360 µs | drift:  -87 µs
beacon:       f4:1e:57:e8:61:90 | local clock:  541,800,768,026 µs | remote tsf:  694,862,029,382 µs | offset: -153,061,261,360 µs | drift:   83 µs
beacon:       f4:1e:57:e8:61:90 | local clock:  541,800,871,295 µs | remote tsf:  694,862,132,640 µs | offset: -153,061,261,360 µs | drift:  -11 µs
beacon:       f4:1e:57:e8:61:90 | local clock:  541,801,074,703 µs | remote tsf:  694,862,336,059 µs | offset: -153,061,261,360 µs | drift:   11 µs
beacon:       f4:1e:57:e8:61:90 | local clock:  541,801,177,120 µs | remote tsf:  694,862,438,469 µs | offset: -153,061,261,360 µs | drift:   -7 µs
beacon:       f4:1e:57:e8:61:90 | local clock:  541,801,279,583 µs | remote tsf:  694,862,540,891 µs | offset: -153,061,261,360 µs | drift:  -41 µs
beacon:       f4:1e:57:e8:61:90 | local clock:  541,801,381,892 µs | remote tsf:  694,862,643,259 µs | offset: -153,061,261,367 µs | drift:   59 µs
beacon:       f4:1e:57:e8:61:90 | local clock:  541,801,484,337 µs | remote tsf:  694,862,745,659 µs | offset: -153,061,261,367 µs | drift:  -45 µs
beacon:       f4:1e:57:e8:61:90 | local clock:  541,801,586,697 µs | remote tsf:  694,862,848,061 µs | offset: -153,061,261,367 µs | drift:   42 µs
beacon:       f4:1e:57:e8:61:90 | local clock:  541,801,689,155 µs | remote tsf:  694,862,950,475 µs | offset: -153,061,261,367 µs | drift:  -44 µs
beacon:       f4:1e:57:e8:61:90 | local clock:  541,801,791,504 µs | remote tsf:  694,863,052,859 µs | offset: -153,061,261,367 µs | drift:   35 µs
beacon:       f4:1e:57:e8:61:90 | local clock:  541,801,893,956 µs | remote tsf:  694,863,155,259 µs | offset: -153,061,261,367 µs | drift:  -52 µs
beacon:       f4:1e:57:e8:61:90 | local clock:  541,801,996,332 µs | remote tsf:  694,863,257,659 µs | offset: -153,061,261,367 µs | drift:   24 µs
beacon:       f4:1e:57:e8:61:90 | local clock:  541,802,098,757 µs | remote tsf:  694,863,360,059 µs | offset: -153,061,261,367 µs | drift:  -25 µs
beacon:       f4:1e:57:e8:61:90 | local clock:  541,802,201,234 µs | remote tsf:  694,863,462,466 µs | offset: -153,061,261,367 µs | drift:  -70 µs
beacon:       f4:1e:57:e8:61:90 | local clock:  541,802,303,632 µs | remote tsf:  694,863,564,859 µs | offset: -153,061,261,367 µs | drift:   -5 µs
beacon:       f4:1e:57:e8:61:90 | local clock:  541,802,406,073 µs | remote tsf:  694,863,667,313 µs | offset: -153,061,261,367 µs | drift:   13 µs
beacon:       f4:1e:57:e8:61:90 | local clock:  541,802,508,353 µs | remote tsf:  694,863,769,659 µs | offset: -153,061,261,367 µs | drift:   66 µs
beacon:       f4:1e:57:e8:61:90 | local clock:  541,802,610,821 µs | remote tsf:  694,863,872,059 µs | offset: -153,061,261,367 µs | drift:  -68 µs
beacon:       f4:1e:57:e8:61:90 | local clock:  541,802,713,264 µs | remote tsf:  694,863,974,470 µs | offset: -153,061,261,367 µs | drift:  -32 µs
beacon:       f4:1e:57:e8:61:90 | local clock:  541,802,815,624 µs | remote tsf:  694,864,076,859 µs | offset: -153,061,261,367 µs | drift:   29 µs
```

The clock drift between the AP (remote) and local client can move in both directions.

![Beacon Timing](./research/timing.png)

## tdma scheduler timing

The TDMA functionality can be found in [tdma.cpp](./src/tdma.cpp):

```C++

    // simplified code loop
    while(true)
    {
        std::this_thread::sleep_until(frame_start + (slot_duration * slot_position));

        resume_transmissions();

        std::this_thread::sleep_for(slot_duration);

        pause_transmissions();

        frame_start += frame_duration;
    }
```

There are multiple possible sources of jitter:


1. the standard doesn't specify any guarantees for [`sleep_until`](https://en.cppreference.com/w/cpp/thread/sleep_until.html)
2. [`sleep_for`](https://en.cppreference.com/w/cpp/thread/sleep_for.html) will sleep *at least* `slot_duration`.
3. the `(pause|resume)_transmissions` internally call `nl_send_sync` which is a socket communication with the kernel. They will await an ack from the kernel.


```bash
oot@radio-backhaul-002:~# ./tc-tdma wlan0 5 --bssid=f4:1e:57:e8:61:90 -v --frame-TUs=20 --slots-per-frame=7
TDMA controller initialized for wlan0
Waiting for first BSS beacon to arrive...
beacon:       f4:1e:57:e8:61:90 | local clock:  556,674,703,939 µs | remote tsf:  709,735,833,659 µs | offset: -153,061,129,720 µs | drift:    0 µs
tdma: cpu spin cost 0.047 µs
qdisc PLUG: @556,674.704 s   |   14,658  -->   18,110 µs   | total duration:    3,452µs
qdisc PLUG: @556,674.724 s   |   14,662  -->   18,106 µs   | total duration:    3,444µs
qdisc PLUG: @556,674.745 s   |   14,662  -->   18,199 µs   | total duration:    3,537µs
qdisc PLUG: @556,674.765 s   |   14,678  -->   18,172 µs   | total duration:    3,494µs
qdisc PLUG: @556,674.786 s   |   14,654  -->   18,148 µs   | total duration:    3,494µs
beacon:       f4:1e:57:e8:61:90 | local clock:  556,674,806,395 µs | remote tsf:  709,735,936,059 µs | offset: -153,061,129,720 µs | drift:  -56 µs
qdisc SYNC: @556,674.806339 s   |   beacon interval: 100 TUs
qdisc PLUG: @556,674.826819 s   |   -5,818  -->   -2,342 µs   | total duration:    3,476µs
qdisc PLUG: @556,674.847299 s   |   14,662  -->   18,124 µs   | total duration:    3,462µs
qdisc PLUG: @556,674.867779 s   |   14,650  -->   18,094 µs   | total duration:    3,444µs
qdisc PLUG: @556,674.888259 s   |   14,654  -->   18,099 µs   | total duration:    3,445µs
beacon:       f4:1e:57:e8:61:90 | local clock:  556,674,908,811 µs | remote tsf:  709,736,038,459 µs | offset: -153,061,129,720 µs | drift:  -16 µs
qdisc SYNC: @556,674.908739 s   |   beacon interval: 100 TUs
qdisc PLUG: @556,674.929219 s   |   -5,821  -->   -2,359 µs   | total duration:    3,462µs
qdisc PLUG: @556,674.949699 s   |   14,655  -->   18,113 µs   | total duration:    3,458µs
qdisc PLUG: @556,674.970179 s   |   14,656  -->   18,095 µs   | total duration:    3,439µs
qdisc PLUG: @556,674.990659 s   |   14,652  -->   18,128 µs   | total duration:    3,476µs
beacon:       f4:1e:57:e8:61:90 | local clock:  556,675,011,250 µs | remote tsf:  709,736,140,869 µs | offset: -153,061,129,720 µs | drift:  -29 µs
qdisc SYNC: @556,675.011149 s   |   beacon interval: 100 TUs
qdisc PLUG: @556,675.031629 s   |   -5,836  -->   -2,383 µs   | total duration:    3,453µs
qdisc PLUG: @556,675.052109 s   |   14,659  -->   18,139 µs   | total duration:    3,480µs
qdisc PLUG: @556,675.072589 s   |   14,661  -->   18,138 µs   | total duration:    3,477µs
qdisc PLUG: @556,675.093069 s   |   14,652  -->   18,090 µs   | total duration:    3,438µs
```

