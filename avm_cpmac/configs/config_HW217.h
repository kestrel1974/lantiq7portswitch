#if !defined(__AVM_NET_CFG_ATHEROS_HW217)
#define __AVM_NET_CFG_ATHEROS_HW217

#include <avmnet_module.h>
#include <avmnet_config.h>
#include "../switch/atheros/atheros_mac.h"
#include "../phy/avmnet_ar803x.h"
#include "../phy/avmnet_ar8337.h"

#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,4,46)
#include <atheros.h>
#else
#include <avm_atheros.h>
#endif

extern avmnet_module_t hw217_gmac0, hw217_ath_switch, 
                       hw217_module_eth0, 
                       hw217_module_eth1, 
                       hw217_module_eth2, 
                       hw217_module_eth3, 
                       hw217_module_eth4;

avmnet_device_t avmnet_hw217_avm_device_0 ____cacheline_aligned =
{
   .device            = NULL,
   .device_name       = "eth1",
   .external_port_no  = 0,
   .device_ops        = {
                          /*--- .ndo_get_stats        = athr_gmac_get_stats, ---*/
                          .ndo_open             = avmnet_netdev_open,
                          .ndo_stop             = avmnet_netdev_stop,
                          /*--- .ndo_do_ioctl         = athr_gmac_do_ioctl, ---*/
                          .ndo_tx_timeout       = athr_gmac_tx_timeout,
                          .ndo_start_xmit       = athr_gmac_hard_start
                        },
   .mac_module        = &hw217_gmac0,
   .vlanID            = 1, // GMAC1, SW MAC 1
   .sizeof_priv       = sizeof(avmnet_netdev_priv_t),
   .device_setup      = athr_gmac_setup_eth,
   .device_setup_priv = athr_gmac_setup_eth_priv,
   .flags		  	  = AVMNET_DEVICE_IFXPPA_ETH_LAN,
};

avmnet_device_t avmnet_hw217_avm_device_1 ____cacheline_aligned =
{
   .device            = NULL,
   .device_name       = "eth2",
   .external_port_no  = 1,
   .device_ops        = {
                          /*--- .ndo_get_stats        = athr_gmac_get_stats, ---*/
                          .ndo_open             = avmnet_netdev_open,
                          .ndo_stop             = avmnet_netdev_stop,
                          /*--- .ndo_do_ioctl         = athr_gmac_do_ioctl, ---*/
                          .ndo_tx_timeout       = athr_gmac_tx_timeout,
                          .ndo_start_xmit       = athr_gmac_hard_start
                        },
   .mac_module        = &hw217_gmac0,
   .vlanID            = 2, // GMAC1, SW MAC 1
   .sizeof_priv       = sizeof(avmnet_netdev_priv_t),
   .device_setup      = athr_gmac_setup_eth,
   .device_setup_priv = athr_gmac_setup_eth_priv,
   .flags		  	  = AVMNET_DEVICE_IFXPPA_ETH_LAN,
};

avmnet_device_t avmnet_hw217_avm_device_2 ____cacheline_aligned =
{
   .device            = NULL,
   .device_name       = "eth3",
   .external_port_no  = 2,
   .device_ops        = {
                          /*--- .ndo_get_stats        = athr_gmac_get_stats, ---*/
                          .ndo_open             = avmnet_netdev_open,
                          .ndo_stop             = avmnet_netdev_stop,
                          /*--- .ndo_do_ioctl         = athr_gmac_do_ioctl, ---*/
                          .ndo_tx_timeout       = athr_gmac_tx_timeout,
                          .ndo_start_xmit       = athr_gmac_hard_start
                        },
   .mac_module        = &hw217_gmac0,
   .vlanID            = 3, // GMAC1, SW MAC 1
   .sizeof_priv       = sizeof(avmnet_netdev_priv_t),
   .device_setup      = athr_gmac_setup_eth,
   .device_setup_priv = athr_gmac_setup_eth_priv,
   .flags		  	  = AVMNET_DEVICE_IFXPPA_ETH_LAN,
};

avmnet_device_t avmnet_hw217_avm_device_3 ____cacheline_aligned =
{
   .device            = NULL,
   .device_name       = "eth4",
   .external_port_no  = 3,
   .device_ops        = {
                          /*--- .ndo_get_stats        = athr_gmac_get_stats, ---*/
                          .ndo_open             = avmnet_netdev_open,
                          .ndo_stop             = avmnet_netdev_stop,
                          /*--- .ndo_do_ioctl         = athr_gmac_do_ioctl, ---*/
                          .ndo_tx_timeout       = athr_gmac_tx_timeout,
                          .ndo_start_xmit       = athr_gmac_hard_start
                        },
   .mac_module        = &hw217_gmac0,
   .vlanID            = 4, // GMAC1, SW MAC 1
   .sizeof_priv       = sizeof(avmnet_netdev_priv_t),
   .device_setup      = athr_gmac_setup_eth,
   .device_setup_priv = athr_gmac_setup_eth_priv,
   .flags		  	  = AVMNET_DEVICE_IFXPPA_ETH_LAN,
};

avmnet_device_t avmnet_hw217_avm_device_4 ____cacheline_aligned =
{
   .device            = NULL,
   .device_name       = "eth0",
   .external_port_no  = 4,
   .device_ops        = {
                          /*--- .ndo_get_stats        = athr_gmac_get_stats, ---*/
                          .ndo_open             = avmnet_netdev_open,
                          .ndo_stop             = avmnet_netdev_stop,
                          /*--- .ndo_do_ioctl         = athr_gmac_do_ioctl, ---*/
                          .ndo_tx_timeout       = athr_gmac_tx_timeout,
                          .ndo_start_xmit       = athr_gmac_hard_start
                        },
   .mac_module        = &hw217_gmac0,
   .vlanID            = 5, // GMAC1, SW MAC 1
   .sizeof_priv       = sizeof(avmnet_netdev_priv_t),
   .device_setup      = athr_gmac_setup_eth,
   .device_setup_priv = athr_gmac_setup_eth_priv,
   .flags		  	  = AVMNET_DEVICE_IFXPPA_ETH_WAN,
};


avmnet_device_t *avmnet_hw217_avm_devices[] = {
    &avmnet_hw217_avm_device_0,
    &avmnet_hw217_avm_device_1,
    &avmnet_hw217_avm_device_2,
    &avmnet_hw217_avm_device_3,
    &avmnet_hw217_avm_device_4
}; 

avmnet_module_t avmnet_HW217 ____cacheline_aligned =
{
    .name           = "athmac",
    .type           = avmnet_modtype_switch,
    .priv           = NULL,
    .initdata       = { .mac = { .flags = 0 }},

    .init           = athmac_init,
    .setup          = athmac_setup,
    .exit           = athmac_exit,
    
    .lock           = athmac_lock,
    .unlock         = athmac_unlock,
    .trylock        = athmac_trylock,
    
#if 0
    .status_changed = athmac_status_changed,
    .poll           = athmac_poll,
    .set_status     = athmac_set_status,
#endif

    .parent         = NULL,
    .num_children   = 1,
    .children       = { &hw217_gmac0 }
};

avmnet_module_t hw217_gmac0 ____cacheline_aligned =
{
    .name           = "gmac0",
    .type           = avmnet_modtype_switch,
    .priv           = NULL,
    .initdata.mac   = {  .flags = AVMNET_CONFIG_FLAG_BASEADDR
                                | AVMNET_CONFIG_FLAG_IRQ
                                | AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_ASYM
                                | AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_SYM
                                | AVMNET_CONFIG_FLAG_SWITCHPORT,
                         .base_addr = ATH_GE0_BASE, 
                         .irq = ATH_CPU_IRQ_GE0, 
                         .mac_nr = 1    /*--- ACHTUNG der QCA956x hat nur einen GMAC, wir nutzen aber VLAN mit Switch ---*/
                      },

    .init           = athmac_gmac_init,
    .setup          = athmac_gmac_setup,
    .exit           = athmac_gmac_exit,

    .lock           = athmac_gmac_lock,
    .unlock         = athmac_gmac_unlock,
    .trylock        = athmac_trylock,

    .reg_read       = athmac_reg_read,
    .reg_write      = athmac_reg_write,
    .status_changed = athmac_status_changed,
    .poll           = athmac_poll,
    .set_status     = athgmac_set_status,
    .setup_irq      = athmac_setup_irq,
    .setup_special_hw = dragonfly_setup_hw,

    .parent         = &avmnet_HW217,
    .num_children   = 1,
    .children       = { &hw217_ath_switch },
};

avmnet_module_t hw217_ath_switch ____cacheline_aligned =
{
    .name           = "ar8337",
    .type           = avmnet_modtype_switch,
    .priv           = NULL,
    .initdata       = { .swi = { .flags = AVMNET_CONFIG_FLAG_IRQ | AVMNET_CONFIG_FLAG_RESET, 
                                 .irq = 10 + ATH_GPIO_IRQ_BASE,   /*--- ist externer GPIO ---*/  
                                 .reset = 11,
                               },
                      },

    .init           = avmnet_ar8337_init,
    .setup          = avmnet_ar8337_setup,
    .exit           = avmnet_ar8337_exit,

    .reg_read       = avmnet_s17_rd_phy,
    .reg_write      = avmnet_s17_wr_phy,
    .lock           = avmnet_s17_lock,
    .unlock         = avmnet_s17_unlock,
    .trylock        = avmnet_s17_trylock,
    .status_changed = avmnet_ar8337_status_changed,
    .poll           = avmnet_ar8337_status_poll,
    .set_status     = avmnet_ar8337_set_status,
    .setup_irq      = avmnet_ar8337_setup_interrupt,

    .parent         = &hw217_gmac0,
    .num_children   = 5,
    .children       = { &hw217_module_eth0, &hw217_module_eth1, &hw217_module_eth2, &hw217_module_eth3, &hw217_module_eth4 }
};

avmnet_module_t hw217_module_eth0 ____cacheline_aligned =
{
    .name           = "ar803x0",
    .device_id      = &avmnet_hw217_avm_device_0,
    .type           = avmnet_modtype_phy,
    .priv           = NULL,
    .initdata.phy   = { .flags =   AVMNET_CONFIG_FLAG_INTERNAL 
                                 /*--- | AVMNET_CONFIG_FLAG_MDIOPOLLING ---*/
                                 | AVMNET_CONFIG_FLAG_DEFAULT_EN_FC_RX
                                 | AVMNET_CONFIG_FLAG_DEFAULT_EN_FC_TX
                                 | AVMNET_CONFIG_FLAG_PHY_GBIT
                                 | AVMNET_CONFIG_FLAG_MDIOADDR,
                                 .mdio_addr = 0, 
                      },

     AR803X_STDFUNCS,
    .setup_special_hw = avmnet_athrs17_setup_phy,
    
    .ethtool_ops    = AR803X_ETHOPS,

    .parent         = &hw217_ath_switch,
    .num_children   = 0,
    .children       = {}
};

avmnet_module_t hw217_module_eth1 ____cacheline_aligned =
{
    .name           = "ar803x1",
    .device_id      = &avmnet_hw217_avm_device_1,
    .type           = avmnet_modtype_phy,
    .priv           = NULL,
    .initdata.phy   = { .flags =   AVMNET_CONFIG_FLAG_INTERNAL 
                                 /*--- | AVMNET_CONFIG_FLAG_MDIOPOLLING ---*/
                                 | AVMNET_CONFIG_FLAG_DEFAULT_EN_FC_RX
                                 | AVMNET_CONFIG_FLAG_DEFAULT_EN_FC_TX
                                 | AVMNET_CONFIG_FLAG_PHY_GBIT
                                 | AVMNET_CONFIG_FLAG_MDIOADDR,
                                 .mdio_addr = 1, 
                      },

     AR803X_STDFUNCS,
    
    .ethtool_ops    = AR803X_ETHOPS,
    .setup_special_hw = avmnet_athrs17_setup_phy,

    .parent         = &hw217_ath_switch,
    .num_children   = 0,
    .children       = {}
};

avmnet_module_t hw217_module_eth2 ____cacheline_aligned =
{
    .name           = "ar803x2",
    .device_id      = &avmnet_hw217_avm_device_2,
    .type           = avmnet_modtype_phy,
    .priv           = NULL,
    .initdata.phy   = { .flags =   AVMNET_CONFIG_FLAG_INTERNAL 
                                 /*--- | AVMNET_CONFIG_FLAG_MDIOPOLLING ---*/
                                 | AVMNET_CONFIG_FLAG_DEFAULT_EN_FC_RX
                                 | AVMNET_CONFIG_FLAG_DEFAULT_EN_FC_TX
                                 | AVMNET_CONFIG_FLAG_PHY_GBIT
                                 | AVMNET_CONFIG_FLAG_MDIOADDR,
                                 .mdio_addr = 2, 
                      },

     AR803X_STDFUNCS,
    
    .ethtool_ops    = AR803X_ETHOPS,
    .setup_special_hw = avmnet_athrs17_setup_phy,

    .parent         = &hw217_ath_switch,
    .num_children   = 0,
    .children       = {}
};

avmnet_module_t hw217_module_eth3 ____cacheline_aligned =
{
    .name           = "ar803x3",
    .device_id      = &avmnet_hw217_avm_device_3,
    .type           = avmnet_modtype_phy,
    .priv           = NULL,
    .initdata.phy   = { .flags =   AVMNET_CONFIG_FLAG_INTERNAL 
                                 /*--- | AVMNET_CONFIG_FLAG_MDIOPOLLING ---*/
                                 | AVMNET_CONFIG_FLAG_DEFAULT_EN_FC_RX
                                 | AVMNET_CONFIG_FLAG_DEFAULT_EN_FC_TX
                                 | AVMNET_CONFIG_FLAG_PHY_GBIT
                                 | AVMNET_CONFIG_FLAG_MDIOADDR,
                                 .mdio_addr = 3, 
                      },

     AR803X_STDFUNCS,
    
    .ethtool_ops    = AR803X_ETHOPS,
    .setup_special_hw = avmnet_athrs17_setup_phy,

    .parent         = &hw217_ath_switch,
    .num_children   = 0,
    .children       = {}
};

avmnet_module_t hw217_module_eth4 ____cacheline_aligned =
{
    .name           = "ar803x4",
    .device_id      = &avmnet_hw217_avm_device_4,
    .type           = avmnet_modtype_phy,
    .priv           = NULL,
    .initdata.phy   = { .flags =   AVMNET_CONFIG_FLAG_INTERNAL 
                                 /*--- | AVMNET_CONFIG_FLAG_MDIOPOLLING ---*/
                                 | AVMNET_CONFIG_FLAG_DEFAULT_EN_FC_RX
                                 | AVMNET_CONFIG_FLAG_DEFAULT_EN_FC_TX
                                 | AVMNET_CONFIG_FLAG_PHY_GBIT
                                 | AVMNET_CONFIG_FLAG_MDIOADDR,
                                 .mdio_addr = 4, 
                      },

     AR803X_STDFUNCS,
    
    .ethtool_ops    = AR803X_ETHOPS,
    .setup_special_hw = avmnet_athrs17_setup_phy,

    .parent         = &hw217_ath_switch,
    .num_children   = 0,
    .children       = {}
};

#endif
