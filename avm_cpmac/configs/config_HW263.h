#if !defined(__AVM_NET_CFG_ATHEROS_hw263_H_)
#define __AVM_NET_CFG_ATHEROS_hw263_H_

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

extern avmnet_module_t hw263_gmac1, hw263_module_eth0, hw263_ath_switch;

avmnet_device_t avmnet_hw263_avm_device_0 ____cacheline_aligned =
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
   .mac_module        = &hw263_gmac1,
   .vlanID            = 1, // GMAC1
   .sizeof_priv       = sizeof(avmnet_netdev_priv_t),
   .device_setup      = athr_gmac_setup_eth,
   .device_setup_priv = athr_gmac_setup_eth_priv,
};

avmnet_device_t *avmnet_hw263_avm_devices[] = {
    &avmnet_hw263_avm_device_0,
};

avmnet_module_t avmnet_hw263 ____cacheline_aligned =
{
    .name           = "athmac",
    .type           = avmnet_modtype_mac,
    .priv           = NULL,
    .initdata       = { .mac = { .flags = 0 }},

    .init           = athmac_init,
    .setup          = athmac_setup_hw219,
    .exit           = athmac_exit,

    .lock           = athmac_lock,
    .unlock         = athmac_unlock,
    .trylock        = athmac_trylock,

    .poll           = athmac_poll,

    .parent         = NULL,
    .num_children   = 1,
    .children       = { &hw263_gmac1 }
};

avmnet_module_t hw263_gmac1 ____cacheline_aligned =
{
    .name           = "gmac1",
    .type           = avmnet_modtype_mac,
    .priv           = NULL,
    .initdata.mac   = { .flags = AVMNET_CONFIG_FLAG_BASEADDR |
                                 AVMNET_CONFIG_FLAG_IRQ |
                                 AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_ASYM |
                                 AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_SYM |
                                 AVMNET_CONFIG_FLAG_SWITCHPORT |
                                 AVMNET_CONFIG_FLAG_NO_WANPORT |
                                 AVMNET_CONFIG_FLAG_BASEADDR_MDIO,
                        .base_addr = ATH_GE1_BASE,
                        .base_mdio = ATH_GE1_BASE,
                        .irq = ATH_CPU_IRQ_GE1,
                        .mac_nr = 1
                      },

    .init           = athmac_gmac_init,
    .setup          = athmac_gmac_setup,
    .exit           = athmac_gmac_exit,
    .poll           = athmac_poll,

    .lock           = athmac_gmac_lock,
    .unlock         = athmac_gmac_unlock,
    .trylock        = athmac_gmac_trylock,

    .reg_read       = athmac_reg_read,
    .reg_write      = athmac_reg_write,
    .status_changed = athmac_status_changed,
    .set_status     = athgmac_set_status,
    .setup_irq      = athmac_setup_irq,
    .setup_special_hw = dragonfly_setup_hw,

    .parent         = &avmnet_hw263,
    .num_children   = 1,
    .children       = { &hw263_ath_switch },
};

avmnet_module_t hw263_ath_switch ____cacheline_aligned =
{
    .name           = "ar8326",
    .type           = avmnet_modtype_switch,
    .priv           = NULL,

    .init           = avmnet_ar8326_init,
    .setup          = avmnet_ar8326_setup,
    .exit           = avmnet_ar8326_exit,

    .reg_read       = avmnet_s27_rd_phy,
    .reg_write      = avmnet_s27_wr_phy,
    .lock           = avmnet_s27_lock,
    .unlock         = avmnet_s27_unlock,
    .trylock        = avmnet_s27_trylock,
    .status_changed = avmnet_ar8326_status_changed,
    .poll           = avmnet_ar8326_poll_intern,
    .set_status     = avmnet_ar8326_set_status,
    .setup_irq      = avmnet_ar8326_setup_interrupt,

    .parent         = &hw263_gmac1,
    .num_children   = 1,
    .children       = { &hw263_module_eth0 }
};

avmnet_module_t hw263_module_eth0 ____cacheline_aligned =
{
    .name           = "ar80x0",
    .device_id      = &avmnet_hw263_avm_device_0,
    .type           = avmnet_modtype_phy,
    .priv           = NULL,
    .initdata.phy   = { .flags =   AVMNET_CONFIG_FLAG_INTERNAL 
                                 | AVMNET_CONFIG_FLAG_MDIOADDR,
                                 .mdio_addr = 0, 
                      },

    AR803X_STDFUNCS,

    .ethtool_ops    = AR803X_ETHOPS,

    .parent         = &hw263_ath_switch,
    .num_children   = 0,
    .children       = {}
};


#endif /*--- #if !defined(__AVM_NET_CFG_ATHEROS_HW200_H_) ---*/
