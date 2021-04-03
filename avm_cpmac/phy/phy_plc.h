#if !defined(__AVM_NET_PHY_PLC_)
#define __AVM_NET_PHY_PLC_

#include <avmnet_module.h>
#include <avm/net/net.h>


int avmnet_phy_plc_init(avmnet_module_t *this);
int avmnet_phy_plc_setup(avmnet_module_t *this);
int avmnet_phy_plc_exit(avmnet_module_t *this);


unsigned int avmnet_phy_plc_reg_read(avmnet_module_t *this, unsigned int addr,
                                        unsigned int reg);

int avmnet_phy_plc_reg_write(avmnet_module_t *this, unsigned int addr, unsigned int reg,
                                unsigned int val);

int avmnet_phy_plc_lock(avmnet_module_t *this);
void avmnet_phy_plc_unlock(avmnet_module_t *this);
int avmnet_phy_plc_trylock(avmnet_module_t *this);
void avmnet_phy_plc_status_changed(avmnet_module_t *this, avmnet_module_t *child);
int avmnet_phy_plc_poll(avmnet_module_t *this);
int avmnet_phy_plc_setup_irq(avmnet_module_t *this, unsigned int on);
int avmnet_phy_plc_set_status(avmnet_module_t *this, avmnet_device_t *device_id,
                                avmnet_linkstatus_t status);

int avmnet_phy_plc_powerup(avmnet_module_t *this);
int avmnet_phy_plc_powerdown(avmnet_module_t *this);


struct avmnet_phy_plc_context
{
    struct semaphore mutex;
    unsigned int reset;
    unsigned int powerdown;
#if defined(CONFIG_MACH_ATHEROS) || defined(CONFIG_ATH79)
    struct resource gpio;
#endif // CONFIG_MACH_ATHEROS != n
};

#endif
