# Cooperative Wifi 5 TDMA protocol

This is a lightweight quick and dirty cooperative TDMA protocol implementation using linux qdisc/plug scheduler.

## Premise

If you have an AP with high bandwidth (e.g. Netmetal 5 with 800Mbps) and you control a finite number of clients connecting to this hub, you can implement a cooperative TDMA schema where each client is configured locally to stay quiescent for a fixed period of time.

The working idea is that we utilize the 802.11 beacon frames to synchronize all clients, and each client gets locally configured with the number of time slots available, and which timeslot it occupies. The hub is left unchanged and gets to do whatever it wants.

Example timing of a single client:

```
     beacon frame                                                                   next beacon
-------------------###-----------------------------------------------------------------###------------
                     |          |          |          |          |          |          |
         TDMA epochs       0         2           3         4          ...        N
```

Epochs 0 and N are special in that the beacon frame arrival is "a bit" stochastic. We treat them specially.



Inside each TDMA epoch, the clients volountarily stay quiet until their configured turn comes:

```
    epoch start                                                                           epoch end
---------##----------------------------------------------------------------------------------##-------
          |    |        |        |        |        |        |        |        |        |     |
slots     XXXXX    0         1       2        3        4         5       ...      M     XXXXXX

```

Each client gets assigned a slot number, as well as slot duration.
