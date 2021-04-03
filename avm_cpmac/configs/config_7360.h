#if !defined(__AVM_NET_CFG_HW181_)
#define __AVM_NET_CFG_HW181_

#ifdef CONFIG_AVM_PA
#if __has_include(<avm/pa/pa.h>)
#include <avm/pa/pa.h>
#else
#include <linux/avm_pa.h>
#endif
#if __has_include(<avm/pa/hw.h>)
#include <avm/pa/hw.h>
#else
#include <linux/avm_pa_hw.h>
#endif
#endif

#include <avmnet_module.h>
#include <avmnet_config.h>
#include "../phy/phy_11G.h"
#include "../switch/ifx/common/swi_ifx_common.h"
#include "../switch/ifx/7port/swi_7port.h"
#include "../switch/ifx/7port/mac_7port.h"
#include "../phy/avmnet_ar803x.h"

extern avmnet_module_t hw181_avmnet_mac_7port_0;
extern avmnet_module_t hw181_avmnet_mac_7port_1;
extern avmnet_module_t hw181_avmnet_mac_7port_2;
extern avmnet_module_t hw181_avmnet_mac_7port_4;

avmnet_device_t hw181_avmnet_avm_device_0 =
{
   .device            = NULL,
   .device_name       = "eth0",
   .external_port_no  = 0,
#if defined(CONFIG_AVM_SCATTER_GATHER)
   .net_dev_features  =  NETIF_F_SG | NETIF_F_IP_CSUM,
#endif
   .device_ops        = {
                         .ndo_init             = ifx_ppa_eth_init,
                         .ndo_get_stats        = avmnet_swi_7port_get_net_device_stats,
                         .ndo_open             = avmnet_netdev_open,
                         .ndo_stop             = avmnet_netdev_stop,
                         .ndo_do_ioctl         = ifx_ppa_eth_ioctl,
                         .ndo_tx_timeout       = ifx_ppa_eth_tx_timeout,
                         .ndo_start_xmit       = ifx_ppa_eth_hard_start_xmit
                        },
   .sizeof_priv       = sizeof(avmnet_netdev_priv_t),
   .device_setup      = ifx_ppa_setup_eth,
   .device_setup_priv = ifx_ppa_setup_priv,
    .flags		  	  = AVMNET_DEVICE_IFXPPA_ETH_LAN,
   .mac_module        = &hw181_avmnet_mac_7port_4,
   .etht_stat         = {
                            DEFAULT_HW_STATS,
                            DEFAULT_AVMNET_STATS,
                            .gather_hw_stats = create_7port_rmon_cnt,
                        },
};

avmnet_device_t hw181_avmnet_avm_device_1 =
{
   .device            = NULL,
   .device_name       = "eth1",
   .external_port_no  = 1,
#if defined(CONFIG_AVM_SCATTER_GATHER)
   .net_dev_features  =  NETIF_F_SG | NETIF_F_IP_CSUM,
#endif
   .device_ops        = {
                         .ndo_init             = ifx_ppa_eth_init,
                         .ndo_get_stats        = avmnet_swi_7port_get_net_device_stats,
                         .ndo_open             = avmnet_netdev_open,
                         .ndo_stop             = avmnet_netdev_stop,
                         .ndo_do_ioctl         = ifx_ppa_eth_ioctl,
                         .ndo_tx_timeout       = ifx_ppa_eth_tx_timeout,
                         .ndo_start_xmit       = ifx_ppa_eth_hard_start_xmit
                        },
   .sizeof_priv       = sizeof(avmnet_netdev_priv_t),
   .device_setup      = ifx_ppa_setup_eth,
   .device_setup_priv = ifx_ppa_setup_priv,
   .flags		  	  = AVMNET_DEVICE_IFXPPA_ETH_LAN,
   .mac_module        = &hw181_avmnet_mac_7port_2,
   .etht_stat         = {
                            DEFAULT_HW_STATS,
                            DEFAULT_AVMNET_STATS,
                            .gather_hw_stats = create_7port_rmon_cnt,
                        },
};

avmnet_device_t hw181_avmnet_avm_device_2 =
{
   .device            = NULL,
   .device_name       = "eth2",
   .external_port_no  = 2,
#if defined(CONFIG_AVM_SCATTER_GATHER)
   .net_dev_features  =  NETIF_F_SG | NETIF_F_IP_CSUM,
#endif
   .device_ops        = {
                         .ndo_init             = ifx_ppa_eth_init,
                         .ndo_get_stats        = avmnet_swi_7port_get_net_device_stats,
                         .ndo_open             = avmnet_netdev_open,
                         .ndo_stop             = avmnet_netdev_stop,
                         .ndo_do_ioctl         = ifx_ppa_eth_ioctl,
                         .ndo_tx_timeout       = ifx_ppa_eth_tx_timeout,
                         .ndo_start_xmit       = ifx_ppa_eth_hard_start_xmit
                        },
   .sizeof_priv       = sizeof(avmnet_netdev_priv_t),
   .device_setup      = ifx_ppa_setup_eth,
   .device_setup_priv = ifx_ppa_setup_priv,
   .flags		  	  = AVMNET_DEVICE_IFXPPA_ETH_LAN,
   .mac_module        = &hw181_avmnet_mac_7port_0,
   .etht_stat         = {
                            DEFAULT_HW_STATS,
                            DEFAULT_AVMNET_STATS,
                            .gather_hw_stats = create_7port_rmon_cnt,
                        },
};

avmnet_device_t hw181_avmnet_avm_device_3 =
{
   .device            = NULL,
   .device_name       = "eth3",
   .external_port_no  = 3,
#if defined(CONFIG_AVM_SCATTER_GATHER)
   .net_dev_features  =  NETIF_F_SG | NETIF_F_IP_CSUM,
#endif
   .device_ops        = {
                         .ndo_init             = ifx_ppa_eth_init,
                         .ndo_get_stats        = avmnet_swi_7port_get_net_device_stats,
                         .ndo_open             = avmnet_netdev_open,
                         .ndo_stop             = avmnet_netdev_stop,
                         .ndo_do_ioctl         = ifx_ppa_eth_ioctl,
                         .ndo_tx_timeout       = ifx_ppa_eth_tx_timeout,
                         .ndo_start_xmit       = ifx_ppa_eth_hard_start_xmit
                        },
   .sizeof_priv       = sizeof(avmnet_netdev_priv_t),
   .device_setup      = ifx_ppa_setup_eth,
   .device_setup_priv = ifx_ppa_setup_priv,
   .flags		  	  = AVMNET_DEVICE_IFXPPA_ETH_LAN,
   .mac_module        = &hw181_avmnet_mac_7port_1,
   .etht_stat         = {
                            DEFAULT_HW_STATS,
                            DEFAULT_AVMNET_STATS,
                            .gather_hw_stats = create_7port_rmon_cnt,
                        },
};

avmnet_device_t hw181_avmnet_avm_device_4 =
{
   .device            = NULL,
   .device_name       = "ptm_vr9",
   .external_port_no  = 255,
   .sizeof_priv       = sizeof(avmnet_netdev_priv_t),
   .device_setup_priv = ifx_ppa_setup_priv,
   .flags			  = AVMNET_DEVICE_IFXPPA_PTM_WAN | AVMNET_DEVICE_FLAG_WAIT_FOR_MODULE_FUNCTIONS,
   .device_ops        = {
                          .ndo_get_stats = avmnet_generic_get_stats,
                        },
   .mac_module        = NULL,
};

avmnet_device_t *hw181_avmnet_avm_devices[] = {
    &hw181_avmnet_avm_device_0,
    &hw181_avmnet_avm_device_1,
    &hw181_avmnet_avm_device_2,
    &hw181_avmnet_avm_device_3,
    &hw181_avmnet_avm_device_4,
}; 

avmnet_module_t hw181_avmnet =
{
    .name               = "swi_vr9",
    .type               = avmnet_modtype_switch,
    .priv               = NULL,
    .initdata.swi       = {
                            .flags = SWI_DISABLE_LEARNING,
                           },
    .init               = avmnet_swi_7port_init,
    .setup              = avmnet_swi_7port_setup,
    .exit               = avmnet_swi_7port_exit,
    
    .reg_read           = avmnet_swi_7port_reg_read,
    .reg_write          = avmnet_swi_7port_reg_write,
    .lock               = avmnet_swi_7port_lock,
    .unlock             = avmnet_swi_7port_unlock,
    .trylock            = avmnet_swi_7port_trylock,
    .status_changed     = avmnet_swi_7port_status_changed,
    .set_status         = avmnet_swi_7port_set_status,
    .poll               = avmnet_swi_7port_poll,
    .setup_irq          = avmnet_swi_7port_setup_irq,
    .powerup            = avmnet_swi_7port_powerup,
    .powerdown          = avmnet_swi_7port_powerdown,
    .suspend            = avmnet_swi_7port_suspend,
    .resume             = avmnet_swi_7port_resume,
    
    .parent             = NULL,
    .num_children       = 4,
    .children           = { &hw181_avmnet_mac_7port_0,
                            &hw181_avmnet_mac_7port_1,
                            &hw181_avmnet_mac_7port_2,
                            &hw181_avmnet_mac_7port_4
                          }
};


extern avmnet_module_t hw181_avmnet_phy_ar803x_0;
extern avmnet_module_t hw181_avmnet_phy_ar803x_1;
extern avmnet_module_t hw181_avmnet_phy_11G_0;
extern avmnet_module_t hw181_avmnet_phy_11G_1;

avmnet_module_t hw181_avmnet_mac_7port_0 =
{
    .name               = "mac_7port_0",
    .type               = avmnet_modtype_mac,
    .priv               = NULL,
    .initdata.mac       = { 
                            //.flags = AVMNET_CONFIG_FLAG_MDIOPOLLING,
                            .mac_nr = 0,
                            .mac_mode = MAC_MODE_RMII,
                            .flags = AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_ASYM |
                                     AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_SYM,
                          },

     MAC_VR9_STDFUNCS,

    .parent             = &hw181_avmnet,
    .num_children       = 1,
    .children           = { &hw181_avmnet_phy_ar803x_0 }
};

avmnet_module_t hw181_avmnet_mac_7port_1 =
{
    .name               = "mac_7port_1",
    .type               = avmnet_modtype_mac,
    .priv               = NULL,
    .initdata.mac       = { 
                            //.flags = AVMNET_CONFIG_FLAG_MDIOPOLLING,
                            .mac_nr = 1,
                            .mac_mode = MAC_MODE_RMII,
                            .flags = AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_ASYM |
                                     AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_SYM,
                          },

     MAC_VR9_STDFUNCS,
    
    .parent             = &hw181_avmnet,
    .num_children       = 1,
    .children           = { &hw181_avmnet_phy_ar803x_1 }
};

avmnet_module_t hw181_avmnet_mac_7port_2 =
{
    .name               = "mac_7port_2",
    .type               = avmnet_modtype_mac,
    .priv               = NULL,
    .initdata.mac       = { 
                            // .flags = AVMNET_CONFIG_FLAG_MDIOPOLLING,
                            .mac_nr = 2,
                            .mac_mode = MAC_MODE_GMII,
                            .flags = AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_ASYM |
                                     AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_SYM,
                          },

     MAC_VR9_STDFUNCS,
    
    .parent             = &hw181_avmnet,
    .num_children       = 1,
    .children           = { &hw181_avmnet_phy_11G_1 }
};

avmnet_module_t hw181_avmnet_mac_7port_4 =
{
    .name               = "mac_7port_4",
    .type               = avmnet_modtype_mac,
    .priv               = NULL,
    .initdata.mac       = { 
                            //.flags = AVMNET_CONFIG_FLAG_MDIOPOLLING,
                            .mac_nr = 4,
                            .mac_mode =  MAC_MODE_GMII,
                            .flags = AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_ASYM |
                                     AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_SYM,
                          },

     MAC_VR9_STDFUNCS,
    
    .parent             = &hw181_avmnet,
    .num_children       = 1,
    .children           = { &hw181_avmnet_phy_11G_0 }
};

avmnet_module_t hw181_avmnet_phy_ar803x_0 =
{
    .name               = "phy_ar803x_0",
    .device_id          = &hw181_avmnet_avm_device_2,
    .type               = avmnet_modtype_phy,
    .priv               = NULL,
    .initdata.phy       = {
                            .flags = ( AVMNET_CONFIG_FLAG_MDIOADDR
                                     | AVMNET_CONFIG_FLAG_RESET
                                     | AVMNET_CONFIG_FLAG_RST_ON_LINKFAIL
                                     | AVMNET_CONFIG_FLAG_LINKFAIL_TIME
                                     | AVMNET_CONFIG_FLAG_IRQ
                                     | AVMNET_CONFIG_FLAG_IRQ_ON
                                     | AVMNET_CONFIG_FLAG_DEFAULT_EN_FC_RX
                                     ),
                            .mdio_addr = 0x00,
                            .reset = 37,
                            .lnkf_time = HZ,
                            .irq = IFX_EIU_IR3,
                          },

     AR803X_STDFUNCS,

    .ethtool_ops        = AR803X_ETHOPS,

    .parent             = &hw181_avmnet_mac_7port_0,
    .num_children       = 0,
    .children           = {}
};

avmnet_module_t hw181_avmnet_phy_ar803x_1 =
{
    .name               = "phy_ar803x_1",
    .device_id          = &hw181_avmnet_avm_device_3,
    .type               = avmnet_modtype_phy,
    .priv               = NULL,
    .initdata.phy       = {
                            .flags = ( AVMNET_CONFIG_FLAG_MDIOADDR
                                     | AVMNET_CONFIG_FLAG_RESET
                                     | AVMNET_CONFIG_FLAG_RST_ON_LINKFAIL
                                     | AVMNET_CONFIG_FLAG_LINKFAIL_TIME
                                     | AVMNET_CONFIG_FLAG_IRQ
                                     | AVMNET_CONFIG_FLAG_IRQ_ON
                                     | AVMNET_CONFIG_FLAG_DEFAULT_EN_FC_RX
                                     ),
                            .mdio_addr = 0x01,
                            .reset = 44,
                            .lnkf_time = HZ,
                            .irq = IFX_EIU_IR3,
                          },
     AR803X_STDFUNCS,

    .ethtool_ops        = AR803X_ETHOPS,
    
    .parent             = &hw181_avmnet_mac_7port_1,
    .num_children       = 0,
    .children           = {}
};

avmnet_module_t hw181_avmnet_phy_11G_0 =
{
    .name               = "phy_11G_0",
    .device_id          = &hw181_avmnet_avm_device_0,
    .type               = avmnet_modtype_phy,
    .priv               = NULL,
    .initdata.phy       = {
                            .flags = (AVMNET_CONFIG_FLAG_PHY_GBIT
                                     |AVMNET_CONFIG_FLAG_MDIOADDR
                                     |AVMNET_CONFIG_FLAG_INTERNAL
                                     |AVMNET_CONFIG_FLAG_POLARITY
                                     |AVMNET_CONFIG_FLAG_DEFAULT_EN_FC_RX
                                     ),
                            .mdio_addr = 0x13,
                            .polarity =
                                    (AVMNET_POLARITY_MDIX
                                     | AVMNET_POLARITY_10MBIT
//                                     | AVMNET_POLARITY_100MBIT
//                                     | AVMNET_POLARITY_1000MBIT
                                     )
                                    << AVMNET_POLARITY_B_SHIFT,
                          },

     PHY_11G_STDFUNCS,

    .ethtool_ops        = PHY_11G_ETHOPS,

    .parent             = &hw181_avmnet_mac_7port_4,
    .num_children       = 0,
    .children           = {}
};

avmnet_module_t hw181_avmnet_phy_11G_1 =
{
    .name               = "phy_11G_1",
    .device_id          = &hw181_avmnet_avm_device_1,
    .type               = avmnet_modtype_phy,
    .priv               = NULL,
    .initdata.phy       = {
                            .flags = (AVMNET_CONFIG_FLAG_PHY_GBIT
                                     |AVMNET_CONFIG_FLAG_MDIOADDR
                                     |AVMNET_CONFIG_FLAG_INTERNAL
                                     |AVMNET_CONFIG_FLAG_POLARITY
                                     |AVMNET_CONFIG_FLAG_DEFAULT_EN_FC_RX
                                     ),
                            .mdio_addr = 0x11,
                            .polarity =
                                    (AVMNET_POLARITY_MDIX
                                     | AVMNET_POLARITY_10MBIT
//                                     | AVMNET_POLARITY_100MBIT
//                                     | AVMNET_POLARITY_1000MBIT
                                     )
                                    << AVMNET_POLARITY_B_SHIFT,
                          },

     PHY_11G_STDFUNCS,

    .ethtool_ops        = PHY_11G_ETHOPS,
    
    .parent             = &hw181_avmnet_mac_7port_2,
    .num_children       = 0,
    .children           = {}
};


#endif // __AVM_NET_CFG_HW181_
