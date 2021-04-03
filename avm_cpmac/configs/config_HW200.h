#if !defined(__AVM_NET_CFG_ATHEROS_HW200_H_)
#define __AVM_NET_CFG_ATHEROS_HW200_H_

#include <avmnet_module.h>
#include <avmnet_config.h>
#include "../switch/atheros/atheros_mac.h"
#include "../phy/avmnet_ar803x.h"
#include "../phy/avmnet_ar8326.h"

#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,4,46)
#include <atheros.h>
#else
#include <avm_atheros.h>
#endif

extern avmnet_module_t hw200_gmac0, hw200_module_eth0;

avmnet_device_t avmnet_hw200_avm_device_0 ____cacheline_aligned =
{
   .device            = NULL,
   .device_name       = "eth0",
   .external_port_no  = 0,
   .device_ops        = {
                          /*--- .ndo_get_stats        = athr_gmac_get_stats, ---*/
                          .ndo_open             = avmnet_netdev_open,
                          .ndo_stop             = avmnet_netdev_stop,
                          /*--- .ndo_do_ioctl         = athr_gmac_do_ioctl, ---*/
                          .ndo_tx_timeout       = athr_gmac_tx_timeout,
                          .ndo_start_xmit       = athr_gmac_hard_start
                        },
   .mac_module        = &hw200_gmac0,
   .vlanID            = 0, // GMAC0
   .sizeof_priv       = sizeof(avmnet_netdev_priv_t),
   .device_setup      = athr_gmac_setup_eth,
   .device_setup_priv = athr_gmac_setup_eth_priv,
};

avmnet_device_t *avmnet_hw200_avm_devices[] = {
    &avmnet_hw200_avm_device_0,
}; 

avmnet_module_t avmnet_hw200 ____cacheline_aligned =
{
    .name           = "athmac",
    .type           = avmnet_modtype_mac,
    .priv           = NULL,
    .initdata       = { .mac = { .flags = 0 }},

    .init           = athmac_init,
    .setup          = athmac_setup,
    .exit           = athmac_exit,

    .lock           = athmac_lock,
    .unlock         = athmac_unlock,
    .trylock        = athmac_trylock,
    
    .poll           = athmac_poll,
    
    .parent         = NULL,
    .num_children   = 1,
    .children       = { &hw200_gmac0 }
};

avmnet_module_t hw200_gmac0 ____cacheline_aligned =
{
    .name           = "gmac0",
    .type           = avmnet_modtype_mac,
    .priv           = NULL,
    .initdata.mac   = { .flags = AVMNET_CONFIG_FLAG_BASEADDR | 
                                 AVMNET_CONFIG_FLAG_IRQ | 
                                 AVMNET_CONFIG_FLAG_SGMII_ENABLE |
                                 AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_ASYM |
                                 AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_SYM |
                                 AVMNET_CONFIG_FLAG_BASEADDR_MDIO,
                        .base_addr = ATH_GE0_BASE, 
                        .base_mdio = ATH_GE1_BASE,
                        .irq = ATH_CPU_IRQ_GE0,
                        .mac_nr = 0 
                      },

    .init           = athmac_gmac_init,
    .setup          = athmac_gmac_setup,
    .exit           = athmac_gmac_exit,

    .lock           = athmac_gmac_lock,
    .unlock         = athmac_gmac_unlock,
    .trylock        = athmac_gmac_trylock,

    .poll           = athmac_poll,
    .reg_read       = athmac_reg_read,
    .reg_write      = athmac_reg_write,
    .status_changed = athmac_status_changed,
    .set_status     = athgmac_set_status,
    .setup_irq      = athmac_setup_irq,
    .setup_special_hw = scorpion_setup_hw,

    .reinit         = athgmac_reinit,   /*--- wird beim AR8033 für fast_reset bei Linkup genutzt ---*/

    .parent         = &avmnet_hw200,
    .num_children   = 1,
    .children       = { &hw200_module_eth0 },
};

avmnet_module_t hw200_module_eth0 ____cacheline_aligned =
{
    .name           = "ar8033",
    .device_id      = &avmnet_hw200_avm_device_0,
    .type           = avmnet_modtype_phy,
    .priv           = NULL,
    .initdata.phy   = { .flags =   AVMNET_CONFIG_FLAG_RESET 
                                 | AVMNET_CONFIG_FLAG_PHY_GBIT
                                 | AVMNET_CONFIG_FLAG_IRQ
                                 | AVMNET_CONFIG_FLAG_MDIOADDR
                                 | AVMNET_CONFIG_FLAG_DEFAULT_EN_FC_RX
                                 | AVMNET_CONFIG_FLAG_DEFAULT_EN_FC_TX,
                                 .mdio_addr = 0, 
                                 .irq = ATH_MISC_IRQ_ENET_LINK,
                                 .reset = 11
                      },

    AR8033_STDFUNCS,

    .set_gpio           = avmnet_ar8033_gpio_set,

    .ethtool_ops    = AR8033_ETHOPS,

    .parent         = &hw200_gmac0,
    .num_children   = 0,
    .children       = {}
};


#endif /*--- #if !defined(__AVM_NET_CFG_ATHEROS_HW200_H_) ---*/
