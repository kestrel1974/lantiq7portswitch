#if !defined(__AVM_NET_PHY_SCRPN_TARGET_)
#define __AVM_NET_PHY_SCRPN_TARGET_

#include <avmnet_module.h>
#include <avm/net/net.h>

int avmnet_phy_scrpn_tgt_init(avmnet_module_t *this);
int avmnet_phy_scrpn_tgt_setup(avmnet_module_t *this);
int avmnet_phy_scrpn_tgt_exit(avmnet_module_t *this);


unsigned int avmnet_phy_scrpn_tgt_reg_read(avmnet_module_t *this, unsigned int addr,
                                        unsigned int reg);

int avmnet_phy_scrpn_tgt_reg_write(avmnet_module_t *this, unsigned int addr, unsigned int reg,
                                unsigned int val);

int avmnet_phy_scrpn_tgt_lock(avmnet_module_t *this);
void avmnet_phy_scrpn_tgt_unlock(avmnet_module_t *this);
int avmnet_phy_scrpn_tgt_trylock(avmnet_module_t *this);
void avmnet_phy_scrpn_tgt_status_changed(avmnet_module_t *this, avmnet_module_t *child);
int avmnet_phy_scrpn_tgt_poll(avmnet_module_t *this);
int avmnet_phy_scrpn_tgt_setup_irq(avmnet_module_t *this, unsigned int on);
int avmnet_phy_scrpn_tgt_set_status(avmnet_module_t *this, avmnet_device_t *device_id,
                                avmnet_linkstatus_t status);

int avmnet_phy_scrpn_tgt_powerup(avmnet_module_t *this);
int avmnet_phy_scrpn_tgt_powerdown(avmnet_module_t *this);
#endif /* __AVM_NET_PHY_SCRPN_TARGET_ */
