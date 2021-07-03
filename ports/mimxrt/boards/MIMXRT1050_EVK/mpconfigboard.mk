MCU_SERIES = MIMXRT1052
MCU_VARIANT = MIMXRT1052DVL6B

JLINK_PATH ?= /media/RT1050-EVK/

MICROPY_FLOAT_IMPL = double

CFLAGS += -DFSL_FEATURE_PHYKSZ8081_USE_RMII50M_MODE=1

SRC_LWIP_C += \
	lib/lwip/src/apps/mdns/mdns.c \
	lib/lwip/src/core/def.c \
	lib/lwip/src/core/dns.c \
	lib/lwip/src/core/inet_chksum.c \
	lib/lwip/src/core/init.c \
	lib/lwip/src/core/ip.c \
	lib/lwip/src/core/mem.c \
	lib/lwip/src/core/memp.c \
	lib/lwip/src/core/netif.c \
	lib/lwip/src/core/pbuf.c \
	lib/lwip/src/core/raw.c \
	lib/lwip/src/core/stats.c \
	lib/lwip/src/core/sys.c \
	lib/lwip/src/core/tcp.c \
	lib/lwip/src/core/tcp_in.c \
	lib/lwip/src/core/tcp_out.c \
	lib/lwip/src/core/timeouts.c \
	lib/lwip/src/core/udp.c \
	lib/lwip/src/core/ipv4/autoip.c \
	lib/lwip/src/core/ipv4/dhcp.c \
	lib/lwip/src/core/ipv4/etharp.c \
	lib/lwip/src/core/ipv4/icmp.c \
	lib/lwip/src/core/ipv4/igmp.c \
	lib/lwip/src/core/ipv4/ip4_addr.c \
	lib/lwip/src/core/ipv4/ip4.c \
	lib/lwip/src/core/ipv4/ip4_frag.c \
	lib/lwip/src/core/ipv6/dhcp6.c \
	lib/lwip/src/core/ipv6/ethip6.c \
	lib/lwip/src/core/ipv6/icmp6.c \
	lib/lwip/src/core/ipv6/inet6.c \
	lib/lwip/src/core/ipv6/ip6_addr.c \
	lib/lwip/src/core/ipv6/ip6.c \
	lib/lwip/src/core/ipv6/ip6_frag.c \
	lib/lwip/src/core/ipv6/mld6.c \
	lib/lwip/src/core/ipv6/nd6.c \
	lib/lwip/src/netif/ethernet.c \

SRC_ETH_C = \
	modusocket.c \
	modnetwork.c \
	network_lan.c \
	eth.c \
	hal/phy/device/phyksz8081/fsl_phyksz8081.c \
	hal/phy/mdio/enet/fsl_enet_mdio.c \
	lib/netutils/netutils.c \
	lib/netutils/trace.c \
	lib/netutils/dhcpserver.c \
	extmod/modlwip.c \
	$(MCU_DIR)/drivers/fsl_enet.c \
	$(MCU_DIR)/drivers/fsl_ocotp.c \
	$(SRC_LWIP_C) \


deploy: $(BUILD)/firmware.bin
	cp $< $(JLINK_PATH)
