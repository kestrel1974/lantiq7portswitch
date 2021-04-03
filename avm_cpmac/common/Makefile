#
# Makefile for the avmnet infrastructure core modules
# Copyright (C) 2011 AVM GmbH <fritzbox_info@avm.de>
#

AVMNET_VERSION = $(shell echo "`cat $(obj)/.avmnet-version` - `date -r $(obj)/.avmnet-version`")

obj-y	+= avmnet_config.o avmnet_common.o .avmnet-version.o

obj-$(CONFIG_IP_MULTICAST_FASTFORWARD)	+= avmnet_multicast.o

extra-y		+= supportdata

quiet_cmd_install = INSTALL $< -> $@
      cmd_install = ( test -d $(dir $@) || mkdir -p $(dir $@) ) && cp -p $< $@

quiet_cmd_set_executable = CHMOD+x $@
      cmd_set_executable = chmod +x $@

.INTERMEDIATE: $(obj)/.avmnet-version.new

$(obj)/.avmnet-version.new: FORCE
	@$(srctree)/$(src)/version.sh > $@

$(obj)/.avmnet-version: $(obj)/.avmnet-version.new
	@if ! test -e $@ ; then touch $@ ; fi
	@if ! cmp -s $@ $< || \
	    test `find $(srctree)/$(src)/.. -newer $(obj)/.avmnet-version -and -name "*.[ch]" -and -not -name ".avmnet-version.c" | wc -l` -gt 0 ; then \
		echo "  REFRESH $@" ; \
		mv $< $@ ; \
	else \
		rm -f $< ; \
	fi

$(obj)/.avmnet-version.c: $(obj)/.avmnet-version
	@echo "  GEN     $@"
	@echo '#include <avmnet_common.h>' > $@
	@echo 'const char *avmnet_version = "$(shell echo "`cat $<``date -r $<`")";' >> $@

$(obj)/.avmnet-version.o: $(obj)/.avmnet-version.c

$(obj)/avmnet_config.o: $(obj)/.avmnet-version.o

ifeq ($(INSTALL_MOD_PATH),)
export INSTALL_MOD_PATH = $(realpath $(srctree)/../../filesystem)
$(info avmnet/common: Guessing INSTALL_MOD_PATH: $(INSTALL_MOD_PATH))
endif

$(INSTALL_MOD_PATH)/bin/supportdata.avmnet: $(srctree)/$(src)/supportdata.avmnet
	$(call if_changed,install)
	$(call if_changed,set_executable)

PHONY+=$(obj)/supportdata
$(obj)/supportdata: $(INSTALL_MOD_PATH)/bin/supportdata.avmnet
