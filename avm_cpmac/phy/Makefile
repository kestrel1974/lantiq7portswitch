#
# Makefile for the avmnet infrastructure core modules
# Copyright (C) 2011 AVM GmbH <fritzbox_info@avm.de>
#

AVMNET_HDRS = ../include/avmnet_config.h ../include/avmnet_module.h ../include/mdio_reg.h

obj-$(CONFIG_LANTIQ_11G_PHY)	+= phy_11G.o
obj-$(CONFIG_WLAN_OFFLOAD_PHY)  += phy_wasp.o
obj-$(CONFIG_ATHRS27_PHY)       += avmnet_ar8326.o
obj-$(CONFIG_ATHRF2_PHY)	    += avmnet_ar803x.o 
obj-$(CONFIG_AR8033_PHY)	    += avmnet_ar8033.o 
obj-$(CONFIG_ATH_PLC_PHY)	    += phy_plc.o 
obj-$(CONFIG_SCRPN_TARGET_PHY)	    += phy_scrpn_target.o 
obj-$(CONFIG_ATHRS17_PHY)       += avmnet_ar8337.o 

obj-$(CONFIG_MACH_QCA953x)      += avmnet_mdio_bitbang.o
obj-$(CONFIG_SOC_QCA953X)       += avmnet_mdio_bitbang.o

obj-$(CONFIG_MACH_QCA956x)      += avmnet_ar8326_wan.o
obj-$(CONFIG_MACH_QCA953x)      += avmnet_ar8326_wan.o
obj-$(CONFIG_SOC_QCA956X)       += avmnet_ar8326_wan.o
obj-$(CONFIG_SOC_QCA953X)       += avmnet_ar8326_wan.o
