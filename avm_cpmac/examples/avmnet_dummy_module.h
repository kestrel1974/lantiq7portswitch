#if !defined(__AVM_NET_DUMMY_)
#define __AVM_NET_DUMMY_

#include <avmnet_module.h>

int dummy_init(avmnet_module_t *this);
int dummy_setup(avmnet_module_t *this);
int dummy_exit(avmnet_module_t *this);

int dummy_reg_read(avmnet_module_t *this, unsigned int addr, unsigned int reg);
int dummy_reg_write(avmnet_module_t *this, unsigned int addr, unsigned int reg, unsigned int val);
int dummy_lock(avmnet_module_t *this);
void dummy_unlock(avmnet_module_t *this);
void dummy_status_changed(avmnet_module_t *this, avmnet_module_t *child);
int dummy_poll(avmnet_module_t *this);
int dummy_setup_irq(avmnet_module_t *this, unsigned int on);
int dummy_set_status(avmnet_module_t *this, avmnet_device_t *device_id, avmnet_linkstatus_t status);
int dummy_powerdown(avmnet_module_t *this);
int dummy_powerup(avmnet_module_t *this);

struct dummy_context
{
    avmnet_module_t *this;
    int foo;
    unsigned int bar;
    char *baz;
};

#endif
