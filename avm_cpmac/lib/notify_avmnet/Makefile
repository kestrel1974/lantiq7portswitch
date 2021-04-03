ifeq ($(INSTALL_MOD_PATH),)
export INSTALL_MOD_PATH = $(realpath $(srctree)/../../filesystem)
$(info avmnet/lib/notify: Guessing INSTALL_MOD_PATH: $(INSTALL_MOD_PATH))
endif

US_INCLUDE := \
		-I$(srctree)/arch/$(SRCARCH)/include/uapi \
		-Iarch/$(SRCARCH)/include/generated/uapi \
		-I$(srctree)/include/uapi \
		-Iinclude/generated/uapi \
		$(USERINCLUDE_AVM_SUBPROJECTS)

extra-y			+= notify_avmnet INSTALL_EXTRA_TARGETS
clean-files		+= libtest libnotify_avmnet.so.0.1 libnotify_avmnet.so.0 libnotify_avmnet.so notify_avmnet.o

quiet_cmd_target_cc_o_c = EXTRACC $@
      cmd_target_cc_o_c = $(CC) $(US_INCLUDE) $(CFLAGS) -c -o $@ $<

quiet_cmd_target_ld = EXTRALD $@
      cmd_target_ld = $(CC) $(LDFLAGS) -o $@ $<

quiet_cmd_dep = DEP     $@
      cmd_dep = $(CC) $(US_INCLUDE) $(CFLAGS) -M -MF $@ -MG -MP -MT $@ -MT $(dir $@)$*.o $<

quiet_cmd_lns = SYMLINK $(notdir $<) -> $@
      cmd_lns = ( test -d $(dir $@) || mkdir -p $(dir $@) ) && ln -sf $(notdir $<) $@

quiet_cmd_install = INSTALL $< -> $@
      cmd_install = ( test -d $(dir $@) || mkdir -p $(dir $@) ) && cp -p $< $@

quiet_cmd_strip = STRIP   $@
      cmd_strip = $(STRIP) --strip-all $@

ifeq ($(CONFIG_AR9),y)
PROCESSOR = CONFIG_AR9
endif 
ifeq ($(CONFIG_AR10),y)
PROCESSOR = CONFIG_AR10
endif 
ifeq ($(CONFIG_VR9),y)
PROCESSOR = CONFIG_VR9
endif 

$(obj)/notify_avmnet: LDFLAGS = -L$(obj) -lnotify_avmnet -ldl
$(obj)/notify_avmnet: $(obj)/notify_avmnet_cmd.o $(obj)/libnotify_avmnet.so
	$(call if_changed,target_ld)

$(obj)/notify_avmnet.o $(obj)/notify_avmnet_cmd.o: CFLAGS = -fPIC -DPIC -D$(PROCESSOR)
$(obj)/notify_avmnet.o $(obj)/notify_avmnet_cmd.o: $(obj)/%.o: $(src)/%.c
	$(call if_changed,target_cc_o_c)

ifneq ($(MAKECMDGOALS),clean)
-include $(obj)/notify_avmnet.d $(obj)/notify_avmnet_cmd.d
endif

$(obj)/notify_avmnet.d $(obj)/notify_avmnet_cmd.d: $(src)/%.c
	$(call if_changed,dep)

$(obj)/libnotify_avmnet.so.0.1: LDFLAGS = -shared -Wl,-soname,libnotify_avmnet.so.0.1
$(obj)/libnotify_avmnet.so.0.1: $(obj)/notify_avmnet.o
	$(call if_changed,target_ld)

$(obj)/libnotify_avmnet.so: $(obj)/libnotify_avmnet.so.0
	$(call if_changed,lns)

$(obj)/libnotify_avmnet.so.0: $(obj)/libnotify_avmnet.so.0.1
	$(call if_changed,lns)

extra_install_libs := $(INSTALL_MOD_PATH)/lib/libnotify_avmnet.so
extra_install_libs += $(INSTALL_MOD_PATH)/lib/libnotify_avmnet.so.0
extra_install_libs += $(INSTALL_MOD_PATH)/lib/libnotify_avmnet.so.0.1
extra_install_headers := $(INSTALL_MOD_PATH)/usr/include/notify_avmnet.h
extra_install_sbin_binaries := $(INSTALL_MOD_PATH)/sbin/notify_avmnet

# Install and strip libraries (and symlinks)
$(extra_install_libs): $(INSTALL_MOD_PATH)/lib/%: $(obj)/%
	$(call if_changed,install)
	$(if $(shell test -L $< || echo not-a-symlink),$(call if_changed,strip))
	$(if $(shell test -L $< || echo not-a-symlink),@echo "  INFO    $@: strip: $$(stat --printf='%s' $<) -> $$(stat --printf='%s' $@)")

# Install header files
$(extra_install_headers): $(INSTALL_MOD_PATH)/usr/include/%: $(src)/%
	$(call if_changed,install)

# Install and strip sbin binaries
$(extra_install_sbin_binaries): $(INSTALL_MOD_PATH)/sbin/%: $(obj)/%
	$(call if_changed,install)
	$(call if_changed,strip)

extra_install_targets := $(extra_install_libs)
extra_install_targets += $(extra_install_headers)
extra_install_targets += $(extra_install_sbin_binaries)
$(obj)/INSTALL_EXTRA_TARGETS: $(extra_install_targets)
	@true
