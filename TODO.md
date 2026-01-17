

# install qdisc plug

use .config from https://downloads.openwrt.org/releases/24.10.1/targets/ipq40xx/mikrotik/ 


1. clone openwrt 
1. run `make kernel_menuconfig`
1. Navigate to: Networking support -> Networking options -> QoS and/or fair queueing. Find Plug (PLUG). Press M to set it as a module.
1. compile: `make target/linux/compile V=s`

at this point, we have a .ko file in `build_dir/target-arm_cortex-a7+neon-vfpv4_musl_eabi/linux-ipq40xx_mikrotik/linux-6.6.86/net/sched/sch_plug.ko`

which we can directly `insmod` on a target.

# command line actions

the following are the command line action equivalents of what the tdma binary does:

1. setup: `tc qdisc add dev wlan0 root plug limit 10240`
1. plug action: `tc qdisc add dev wlan0 root plug`
1. release action: `tc qdisc change dev wlan0 root plug release_indefinite`
1. dealloc `tc qdisc del dev wlan0 root plug`




