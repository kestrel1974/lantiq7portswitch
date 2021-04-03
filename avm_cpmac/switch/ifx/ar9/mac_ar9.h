#if !defined(__AVMNET_MAC_AR9_)
#define __AVMNET_MAC_AR9_

#include <avmnet_module.h>
#include <avmnet_debug.h>

int avmnet_mac_ar9_init(avmnet_module_t *this);
int avmnet_mac_ar9_setup(avmnet_module_t *this);
int avmnet_mac_ar9_exit(avmnet_module_t *this);

unsigned int avmnet_mac_ar9_reg_read(avmnet_module_t *this, unsigned int addr, unsigned int reg);
int avmnet_mac_ar9_reg_write(avmnet_module_t *this, unsigned int addr, unsigned int reg, unsigned int val);
int avmnet_mac_ar9_lock(avmnet_module_t *this);
void avmnet_mac_ar9_unlock(avmnet_module_t *this);
int avmnet_mac_ar9_trylock(avmnet_module_t *this);
void avmnet_mac_ar9_status_changed(avmnet_module_t *this, avmnet_module_t *child);
int avmnet_mac_ar9_poll(avmnet_module_t *this);
int avmnet_mac_ar9_setup_irq(avmnet_module_t *this, unsigned int on);
int avmnet_mac_ar9_set_status(avmnet_module_t *this, avmnet_device_t *device_id, avmnet_linkstatus_t status);
int avmnet_mac_ar9_powerdown(avmnet_module_t *this);
int avmnet_mac_ar9_powerup(avmnet_module_t *this);

int avmnet_mac_ar9_suspend(avmnet_module_t *this, avmnet_module_t *caller);
int avmnet_mac_ar9_resume(avmnet_module_t *this, avmnet_module_t *caller);
int avmnet_mac_ar9_reinit(avmnet_module_t *this);

#define MAC_MODE_AUTO       0
#define MAC_MODE_MII        1
#define MAC_MODE_RMII       2
#define MAC_MODE_GMII       3
#define MAC_MODE_RGMII_100  4
#define MAC_MODE_RGMII_1000 5

struct avmnet_mac_ar9_context
{
    struct semaphore mutex;
    avmnet_module_t *this;
    unsigned int mac_nr;
    unsigned int mode;
    avmnet_linkstatus_t last_known_state;
    int suspend_cnt;
};

#define MAC_AR9_STDFUNCS                                     \
        .init               = avmnet_mac_ar9_init,           \
        .setup              = avmnet_mac_ar9_setup,          \
        .exit               = avmnet_mac_ar9_exit,           \
        .reg_read           = avmnet_mac_ar9_reg_read,       \
        .reg_write          = avmnet_mac_ar9_reg_write,      \
        .lock               = avmnet_mac_ar9_lock,           \
        .unlock             = avmnet_mac_ar9_unlock,         \
        .trylock            = avmnet_mac_ar9_trylock,        \
        .status_changed     = avmnet_mac_ar9_status_changed, \
        .set_status         = avmnet_mac_ar9_set_status,     \
        .poll               = avmnet_mac_ar9_poll,           \
        .setup_irq          = avmnet_mac_ar9_setup_irq,      \
        .powerup            = avmnet_mac_ar9_powerup,        \
        .powerdown          = avmnet_mac_ar9_powerdown,      \
        .suspend            = avmnet_mac_ar9_suspend,        \
        .resume             = avmnet_mac_ar9_resume,         \
        .reinit             = avmnet_mac_ar9_reinit

#endif
