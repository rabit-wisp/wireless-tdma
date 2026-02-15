
Note: [guide here](https://openwrt.org/docs/guide-developer/toolchain/use-buildsystem#custom_files)

# build device/kernel specific qdisc plug .ko

use .config.buildinfo from:

- https://downloads.openwrt.org/releases/24.10.1/targets/ipq40xx/mikrotik/
- https://downloads.openwrt.org/releases/24.10.1/targets/ath79/mikrotik/


1. `git clone https://git.openwrt.org/openwrt/openwrt.git`
2. `git checkout v24.10.1`
3. put the `.config.buildinfo` into the `openwrt`
4. `make menuconfig` and navigate to Kernel Modules > Network Support and look for `kmod-sched-plug`
6. if `kmod-sched-plug` is missing, add the following excerpt to `package/kernel/linux/modules/netsupport.mk`

```
define KernelPackage/sched-plug
  SUBMENU:=$(NETWORK_SUPPORT_MENU)
  TITLE:=Plug network traffic until release
  DEPENDS:=+kmod-sched-core
  KCONFIG:=CONFIG_NET_SCH_PLUG
  FILES:=$(LINUX_DIR)/net/sched/sch_plug.ko
  AUTOLOAD:=$(call AutoProbe,sch_plug) 
endef

define KernelPackage/sched-plug/description
 A queueing discipline that allows network traffic to be plugged
 and unplugged.
endef

$(eval $(call KernelPackage,sched-plug))
```

7. re-run `make menuconfig`, select `sch_plug` as module "M"
8. run `make download` (speeds up build)
9. `make -j31`
10. `find -iname sch_plug.ko` to find built plugins.
   
at this point, we have a .ko file in `build_dir/target-arm_cortex-a7+neon-vfpv4_musl_eabi/linux-ipq40xx_mikrotik/linux-6.6.86/net/sched/sch_plug.ko`

We can run `insmod sch_plug.ko` on the target platform.

# command line actions

the following are the command line action equivalents of what the tdma binary does:

1. setup: `tc qdisc add dev wlan0 root plug limit 10240`
1. plug action: `tc qdisc add dev wlan0 root plug`
1. release action: `tc qdisc change dev wlan0 root plug release_indefinite`
1. dealloc `tc qdisc del dev wlan0 root plug`




