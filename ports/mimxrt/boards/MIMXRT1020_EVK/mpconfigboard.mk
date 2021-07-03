MCU_SERIES = MIMXRT1021
MCU_VARIANT = MIMXRT1021DAG5A
MCU_DIR ?= lib/nxp_driver/sdk/devices/$(MCU_SERIES)

MICROPY_FLOAT_IMPL = double

JLINK_PATH ?= /media/RT1020-EVK/
JLINK_COMMANDER_SCRIPT = $(BUILD)/script.jlink

ifdef JLINK_IP
JLINK_CONNECTION_SETTINGS = -IP $(JLINK_IP)
else
JLINK_CONNECTION_SETTINGS =
endif

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

deploy_jlink: $(BUILD)/firmware.hex
	$(ECHO) "ExitOnError 1" > $(JLINK_COMMANDER_SCRIPT)
	$(ECHO) "speed auto" >> $(JLINK_COMMANDER_SCRIPT)
	$(ECHO) "r" >> $(JLINK_COMMANDER_SCRIPT)
	$(ECHO) "st" >> $(JLINK_COMMANDER_SCRIPT)
	$(ECHO) "loadfile \"$(realpath $(BUILD)/firmware.hex)\"" >> $(JLINK_COMMANDER_SCRIPT)
	$(ECHO) "qc" >> $(JLINK_COMMANDER_SCRIPT)
	$(JLINK_PATH)JLinkExe -device $(MCU_VARIANT) -if SWD $(JLINK_CONNECTION_SETTINGS) -CommanderScript $(JLINK_COMMANDER_SCRIPT)

deploy: $(BUILD)/firmware.bin
	cp $< $(JLINK_PATH)
