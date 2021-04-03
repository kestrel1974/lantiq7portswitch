#
# Makefile for the avmnet infrastructure core modules
# Copyright (C) 2011 AVM GmbH <fritzbox_info@avm.de>
#

avmnet_includes := ../../../include/avmnet_module.h ../../../include/avmnet_config.h

obj-$(CONFIG_IFX_PPA) += ifx_ppe_drv_wrapper.o ifx_ppa_mini_stack.o

ifeq ($(CONFIG_AVM_PA),y)
obj-m += ifx_ppa_mini_sessions.o
obj-m += ifx_ppa_filter_sessions.o
endif


ifeq ($(CONFIG_IFX_PPA_QOS),y)
obj-m += ifx_ppa_mini_qos.o
endif

obj-$(CONFIG_AVM_PA) += avm_pa_ifx_multiplexer.o

obj-y	+=  swi_ifx_common.o

ifdef CONFIG_LTQ_ETH_OAM
obj-y += ltq_eth_oam_netif.o
endif

