
avmnet_includes := ../../../include/avmnet_module.h ../../../include/avmnet_config.h

obj-y	+= mac_7port.o swi_7port.o ifxmips_ppa_datapath_7port_common.o 

ifneq ($(VERSION), 2)
# kernel 2.6.32 has switch api in kernel source tree
obj-$(CONFIG_IFX_ETHSW_API)		+= ifxmips_switch_api/
endif
