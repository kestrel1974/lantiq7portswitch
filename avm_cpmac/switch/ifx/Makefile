#
# Makefile for the avmnet infrastructure core modules
# Copyright (C) 2011 AVM GmbH <fritzbox_info@avm.de>
#

obj-$(CONFIG_AR9)	+= ar9/ 

obj-$(CONFIG_VR9)	+= vr9/ 
obj-$(CONFIG_VR9)	+= 7port/ 

obj-$(CONFIG_AR10)	+= ar10/ 
obj-$(CONFIG_AR10)	+= 7port/ 

obj-$(CONFIG_VR9)	+= 7port/ 
obj-y				+= common/ 

ifneq ($(VERSION), 2)
# kernel 2.6.32 has vectoring module in kernel source tree
obj-$(CONFIG_PTM_VECTORING)		+= vectoring/
endif

