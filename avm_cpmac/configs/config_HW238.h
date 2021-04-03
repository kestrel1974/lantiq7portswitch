#if !defined(__AVM_NET_CFG_ATHEROS_HW238_H_)
#define __AVM_NET_CFG_ATHEROS_HW238_H_

#include <avmnet_module.h>
#include <avmnet_config.h>
#include "../phy/phy_scrpn_target.h"
#include "../switch/atheros/atheros_mac.h"

#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,4,46)
#include <atheros.h>
#else
#include <avm_atheros.h>
#endif

extern avmnet_module_t hw238_gmac0, hw238_module_phy;

avmnet_device_t avmnet_hw238_avm_device_0 ____cacheline_aligned =
{
   .device            = NULL,
   .device_name       = "eth0",
   .external_port_no  = 255,
   /* Emulate vlan offloading, a requirement for bridged vlan avm_pa sessions. */
   .net_dev_features  = NETIF_F_HW_VLAN_CTAG_RX | NETIF_F_HW_VLAN_CTAG_TX,
   .device_ops        = {
                          /*--- .ndo_get_stats        = athr_gmac_get_stats, ---*/
                          .ndo_open             = avmnet_netdev_open,
                          .ndo_stop             = avmnet_netdev_stop,
                          /*--- .ndo_do_ioctl         = athr_gmac_do_ioctl, ---*/
                          .ndo_tx_timeout       = athr_gmac_tx_timeout,
                          .ndo_start_xmit       = athr_gmac_hard_start
                        },
   .mac_module        = &hw238_gmac0,
   .vlanID            = 0, // GMAC0
   .sizeof_priv       = sizeof(avmnet_netdev_priv_t),
   .device_setup      = athr_gmac_setup_eth,
   .device_setup_priv = athr_gmac_setup_eth_priv,
   .flags             = AVMNET_DEVICE_FLAG_PHYS_OFFLOAD_LINK | AVMNET_DEVICE_FLAG_NO_MCFW
};

avmnet_device_t *avmnet_hw238_avm_devices[] = {
    &avmnet_hw238_avm_device_0,
}; 

avmnet_module_t avmnet_hw238 ____cacheline_aligned =
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
    .children       = { &hw238_gmac0 }
};

avmnet_module_t hw238_gmac0 ____cacheline_aligned =
{
    .name           = "gmac0",
    .type           = avmnet_modtype_mac,
    .priv           = NULL,
    .initdata.mac   = { .flags = AVMNET_CONFIG_FLAG_BASEADDR | 
                                 AVMNET_CONFIG_FLAG_IRQ | 
                                 AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_ASYM |
                                 AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_SYM,
                        .base_addr = ATH_GE0_BASE, 
                        .irq = ATH_CPU_IRQ_GE0,
                        .mac_nr = 0 
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
    .setup_special_hw = scorpion_setup_hw,

    .reinit         = athgmac_reinit,   /*--- wird beim AR8033 für fast_reset bei Linkup genutzt ---*/

    .parent         = &avmnet_hw238,
    .num_children   = 1,
    .children       = { &hw238_module_phy },
};

avmnet_module_t hw238_module_phy ____cacheline_aligned =
{
    .name           = "dummy Phy",
    .device_id      = &avmnet_hw238_avm_device_0,
    .type           = avmnet_modtype_phy,
    .priv           = NULL,
    .initdata.phy   = {
                          .flags = 0,
                      },
    .init               = avmnet_phy_scrpn_tgt_init,
    .setup              = avmnet_phy_scrpn_tgt_setup,
    .exit               = avmnet_phy_scrpn_tgt_exit,

    .reg_read           = avmnet_phy_scrpn_tgt_reg_read,
    .reg_write          = avmnet_phy_scrpn_tgt_reg_write,
    .lock               = avmnet_phy_scrpn_tgt_lock,
    .unlock             = avmnet_phy_scrpn_tgt_unlock,
    .trylock            = avmnet_phy_scrpn_tgt_trylock,
    .status_changed     = avmnet_phy_scrpn_tgt_status_changed,
    .set_status         = avmnet_phy_scrpn_tgt_set_status,
    .poll               = avmnet_phy_scrpn_tgt_poll,
    .setup_irq          = avmnet_phy_scrpn_tgt_setup_irq,
    .powerup            = avmnet_phy_scrpn_tgt_powerup,
    .powerdown          = avmnet_phy_scrpn_tgt_powerdown,

    .ethtool_ops    = {},

    .parent         = &hw238_gmac0,
    .num_children   = 0,
    .children       = {}
};


#endif /*--- #if !defined(__AVM_NET_CFG_ATHEROS_HW238_H_) ---*/
