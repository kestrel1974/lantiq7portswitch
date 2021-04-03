#
# Makefile for the avmnet infrastructure core modules
# Copyright (C) 2011 AVM GmbH <fritzbox_info@avm.de>
#

subdir-ccflags-y += -Wall -Wextra -Wno-missing-field-initializers
subdir-ccflags-y += -I$(src)/include -I$(src)

# GCOV_PROFILE := y

ifeq ($(CONFIG_AVM_CPMAC),y)
obj-y	+= common/ switch/ phy/ management/
subdir-$(CONFIG_AR9)  += lib
subdir-$(CONFIG_VR9)  += lib
subdir-$(CONFIG_AR10) += lib
subdir-$(CONFIG_ATH79) += lib
endif

obj-y	+= eth_oam/
