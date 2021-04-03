#if !defined(__AVMNET_CFG_HW172_)
#define __AVMNET_CFG_HW172_

#include <avmnet_module.h>
#include <avmnet_config.h>
#include "../phy/phy_11G.h"
#include "../switch/ifx/ar9/swi_ar9.h"
#include "../switch/ifx/ar9/mac_ar9.h"

extern avmnet_module_t hw172_avmnet_mac_ar9_0;
extern avmnet_module_t hw172_avmnet_mac_ar9_1;

avmnet_device_t hw172_avmnet_avm_device_0 =
{
   .device            = NULL,
   .device_name       = "eth0",
   .external_port_no  = 0,
#if defined(CONFIG_AVM_SCATTER_GATHER)
   .net_dev_features  =  NETIF_F_SG | NETIF_F_IP_CSUM,
#endif
   .device_ops        = {
                             .ndo_init             = avmnet_swi_ar9_eth_init,
                             .ndo_get_stats        = avmnet_swi_ar9_get_net_device_stats,
                             .ndo_open             = avmnet_netdev_open,
                             .ndo_stop             = avmnet_netdev_stop,
                             .ndo_set_mac_address  = avmnet_swi_ar9_set_mac_address,
                             .ndo_do_ioctl         = avmnet_swi_ar9_ioctl,
                             .ndo_tx_timeout       = avmnet_swi_ar9_tx_timeout,
                             .ndo_start_xmit       = avmnet_swi_ar9_start_xmit
                        },
   .sizeof_priv       = sizeof(avmnet_netdev_priv_t),
   .device_setup      = avmnet_swi_ar9_setup_eth,
   .device_setup_priv = avmnet_swi_ar9_setup_eth_priv,
   .flags		  	  = AVMNET_DEVICE_IFXPPA_ETH_LAN,
   .mac_module        = &hw172_avmnet_mac_ar9_0,
};

avmnet_device_t hw172_avmnet_avm_device_1 =
{
   .device            = NULL,
   .device_name       = "eth1",
   .external_port_no  = 1,
#if defined(CONFIG_AVM_SCATTER_GATHER)
   .net_dev_features  =  NETIF_F_SG | NETIF_F_IP_CSUM,
#endif
   .device_ops        = {
                            .ndo_init             = avmnet_swi_ar9_eth_init,
                            .ndo_get_stats        = avmnet_swi_ar9_get_net_device_stats,
                            .ndo_open             = avmnet_netdev_open,
                            .ndo_stop             = avmnet_netdev_stop,
                            .ndo_set_mac_address  = avmnet_swi_ar9_set_mac_address,
                            .ndo_do_ioctl         = avmnet_swi_ar9_ioctl,
                            .ndo_tx_timeout       = avmnet_swi_ar9_tx_timeout,
                            .ndo_start_xmit       = avmnet_swi_ar9_start_xmit
                        },
   .sizeof_priv       = sizeof(avmnet_netdev_priv_t),
   .device_setup      = avmnet_swi_ar9_setup_eth,
   .device_setup_priv = avmnet_swi_ar9_setup_eth_priv,
   .flags		  	  = AVMNET_DEVICE_IFXPPA_ETH_LAN,
   .mac_module        = &hw172_avmnet_mac_ar9_1,
};


avmnet_device_t *hw172_avmnet_avm_devices[] = {
    &hw172_avmnet_avm_device_0,
    &hw172_avmnet_avm_device_1
}; 

avmnet_module_t hw172_avmnet =
{
    .name               = "swi_ar9",
    .type               = avmnet_modtype_switch,
    .priv               = NULL,
    .initdata.swi       = { 
                          },
    .init               = avmnet_swi_ar9_init,
    .setup              = avmnet_swi_ar9_setup,
    .exit               = avmnet_swi_ar9_exit,
    
    .reg_read           = avmnet_swi_ar9_reg_read,
    .reg_write          = avmnet_swi_ar9_reg_write,
    .lock               = avmnet_swi_ar9_lock,
    .unlock             = avmnet_swi_ar9_unlock,
    .trylock            = avmnet_swi_ar9_trylock,
    .status_changed     = avmnet_swi_ar9_status_changed,
    .set_status         = avmnet_swi_ar9_set_status,
    .poll               = avmnet_swi_ar9_poll,
    .setup_irq          = avmnet_swi_ar9_setup_irq,
    .powerup            = avmnet_swi_ar9_powerup,
    .powerdown          = avmnet_swi_ar9_powerdown,
    .suspend            = avmnet_swi_ar9_suspend,
    .resume             = avmnet_swi_ar9_resume,

    .parent             = NULL,
    .num_children       = 2,
    .children           = { &hw172_avmnet_mac_ar9_0,
                            &hw172_avmnet_mac_ar9_1,
                          }
};


extern avmnet_module_t hw172_avmnet_phy_11G_0;
extern avmnet_module_t hw172_avmnet_phy_11G_1;

avmnet_module_t hw172_avmnet_mac_ar9_0 =
{
    .name               = "mac_ar9_0",
    .type               = avmnet_modtype_mac,
    .priv               = NULL,
    .initdata.mac       = { 
                            .mac_nr = 0,
                            .mac_mode = MAC_MODE_RGMII_1000,
                            .flags = AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_SYM,
                          },

     MAC_AR9_STDFUNCS,

    .parent             = &hw172_avmnet,
    .num_children       = 1,
    .children           = { &hw172_avmnet_phy_11G_0 }
};

avmnet_module_t hw172_avmnet_mac_ar9_1 =
{
    .name               = "mac_ar9_1",
    .type               = avmnet_modtype_mac,
    .priv               = NULL,
    .initdata.mac       = { 
                            .mac_nr = 1,
                            .mac_mode = MAC_MODE_RGMII_1000,
                            .flags = AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_SYM,
                          },

     MAC_AR9_STDFUNCS,

    .parent             = &hw172_avmnet,
    .num_children       = 1,
    .children           = { &hw172_avmnet_phy_11G_1 }
};


avmnet_module_t hw172_avmnet_phy_11G_0 =
{
    .name               = "phy_11G_0",
    .device_id          = &hw172_avmnet_avm_device_0,
    .type               = avmnet_modtype_phy,
    .priv               = NULL,
    .initdata.phy       = {
                            .flags = ( AVMNET_CONFIG_FLAG_PHY_GBIT
                                     | AVMNET_CONFIG_FLAG_MDIOADDR
                                     | AVMNET_CONFIG_FLAG_POLARITY
                                     | AVMNET_CONFIG_FLAG_RESET
                                     | AVMNET_CONFIG_FLAG_DEFAULT_EN_FC_RX
                                     | AVMNET_CONFIG_FLAG_DEFAULT_EN_FC_TX
                                     ),
                            .reset = 34,
                            .mdio_addr = 0x00,
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

    .parent             = &hw172_avmnet_mac_ar9_0,
    .num_children       = 0,
    .children           = {}
};

avmnet_module_t hw172_avmnet_phy_11G_1 =
{
    .name               = "phy_11G_1",
    .device_id          = &hw172_avmnet_avm_device_1,
    .type               = avmnet_modtype_phy,
    .priv               = NULL,
    .initdata.phy       = {
                            .flags = ( AVMNET_CONFIG_FLAG_PHY_GBIT
                                     | AVMNET_CONFIG_FLAG_MDIOADDR
                                     | AVMNET_CONFIG_FLAG_POLARITY
                                     | AVMNET_CONFIG_FLAG_RESET
                                     | AVMNET_CONFIG_FLAG_DEFAULT_EN_FC_RX
                                     | AVMNET_CONFIG_FLAG_DEFAULT_EN_FC_TX
                                     ),
                            .reset = 36,
                            .mdio_addr = 0x01,
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

    .parent             = &hw172_avmnet_mac_ar9_1,
    .num_children       = 0,
    .children           = {}
};

#endif
