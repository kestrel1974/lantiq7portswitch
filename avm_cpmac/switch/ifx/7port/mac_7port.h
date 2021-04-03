#if !defined(__AVM_NET_MAC_VR9_)
#define __AVM_NET_MAC_VR9_

#include <avmnet_module.h>
#include <linux/list.h>

extern struct avmnet_hw_rmon_counter *create_7port_rmon_cnt(avmnet_device_t *avm_dev);

int avmnet_mac_vr9_init(avmnet_module_t *this);
int avmnet_mac_vr9_setup(avmnet_module_t *this);
int avmnet_mac_vr9_exit(avmnet_module_t *this);

unsigned int avmnet_mac_vr9_reg_read(avmnet_module_t *this, unsigned int addr,
                                        unsigned int reg);
int avmnet_mac_vr9_reg_write(avmnet_module_t *this, unsigned int addr, unsigned int reg,
                                unsigned int val);
int avmnet_mac_vr9_lock(avmnet_module_t *this);
void avmnet_mac_vr9_unlock(avmnet_module_t *this);
int avmnet_mac_vr9_trylock(avmnet_module_t *this);
void avmnet_mac_vr9_status_changed(avmnet_module_t *this, avmnet_module_t *child);
int avmnet_mac_vr9_poll(avmnet_module_t *this);
int avmnet_mac_vr9_setup_irq(avmnet_module_t *this, unsigned int on);
int avmnet_mac_vr9_set_status(avmnet_module_t *this, avmnet_device_t *device_id,
                                avmnet_linkstatus_t status);
int avmnet_mac_vr9_powerdown(avmnet_module_t *this);
int avmnet_mac_vr9_powerup(avmnet_module_t *this);
int avmnet_mac_vr9_suspend(avmnet_module_t *this, avmnet_module_t *caller);
int avmnet_mac_vr9_resume(avmnet_module_t *this, avmnet_module_t *caller);
int avmnet_mac_vr9_reinit(avmnet_module_t *this);
void avmnet_mac_vr9_disable_all(void);


struct avmnet_mac_vr9_context
{
    struct semaphore mutex;
    unsigned int mac_nr;
    unsigned int mode;
    avmnet_linkstatus_t last_known_state;
    int suspend_cnt;
};

#define MAC_VR9_STDFUNCS \
    .init               = avmnet_mac_vr9_init,            \
    .setup              = avmnet_mac_vr9_setup,           \
    .exit               = avmnet_mac_vr9_exit,            \
    .reg_read           = avmnet_mac_vr9_reg_read,        \
    .reg_write          = avmnet_mac_vr9_reg_write,       \
    .lock               = avmnet_mac_vr9_lock,            \
    .unlock             = avmnet_mac_vr9_unlock,          \
    .trylock            = avmnet_mac_vr9_trylock,         \
    .status_changed     = avmnet_mac_vr9_status_changed,  \
    .set_status         = avmnet_mac_vr9_set_status,      \
    .poll               = avmnet_mac_vr9_poll,            \
    .setup_irq          = avmnet_mac_vr9_setup_irq,       \
    .powerup            = avmnet_mac_vr9_powerup,         \
    .powerdown          = avmnet_mac_vr9_powerdown,       \
    .suspend            = avmnet_mac_vr9_suspend,         \
    .resume             = avmnet_mac_vr9_resume,          \
    .reinit             = avmnet_mac_vr9_reinit

#endif
