#if !defined(__AVM_NET_CFG_HW243_)
#define __AVM_NET_CFG_HW243_

#include <avmnet_module.h>
#include <avmnet_config.h>
#include "../phy/phy_11G.h"
#include "../switch/ifx/common/swi_ifx_common.h"
#include "../switch/ifx/7port/swi_7port.h"
#include "../switch/ifx/7port/mac_7port.h"
#include "../phy/avmnet_ar803x.h"
#include "../phy/avmnet_ar8337.h"
#include "../phy/phy_wasp.h"

extern avmnet_module_t hw243_avmnet_mac_7port_0;
extern avmnet_module_t hw243_avmnet_mac_7port_1;
extern avmnet_module_t hw243_avmnet_mac_7port_2;
extern avmnet_module_t hw243_avmnet_mac_7port_4;
extern avmnet_module_t hw243_avmnet_mac_7port_5;

avmnet_device_t hw243_avmnet_avm_device_0 =
{
   .device            = NULL,
   .device_name       = "wan",
   .external_port_no  = 4,
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
   .flags		  	  = AVMNET_DEVICE_IFXPPA_ETH_WAN | AVMNET_CONFIG_FLAG_SWITCH_WAN | AVMNET_CONFIG_FLAG_USE_GPON,
   .mac_module        = &hw243_avmnet_mac_7port_0,
   .etht_stat         = {
                            DEFAULT_HW_STATS,
                            DEFAULT_AVMNET_STATS,
                            .gather_hw_stats = create_7port_rmon_cnt,
                        },
};

avmnet_device_t hw243_avmnet_avm_device_1 =
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
   .mac_module        = &hw243_avmnet_mac_7port_4,
   .etht_stat         = {
                            DEFAULT_HW_STATS,
                            DEFAULT_AVMNET_STATS,
                            .gather_hw_stats = create_7port_rmon_cnt,
                        },
};

avmnet_device_t hw243_avmnet_avm_device_2 =
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
   .mac_module        = &hw243_avmnet_mac_7port_2,
   .etht_stat         = {
                            DEFAULT_HW_STATS,
                            DEFAULT_AVMNET_STATS,
                            .gather_hw_stats = create_7port_rmon_cnt,
                        },
};

/*------------------------------------------------------------------------------------------*\
 * eth2 und eth3 sind keinem MAC zugeordnet - hier wird nur der Status angezeigt
\*------------------------------------------------------------------------------------------*/
avmnet_device_t hw243_avmnet_avm_device_3 =
{
   .device            = NULL,
   .device_name       = "eth2.2",
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
                         .ndo_tx_timeout       = ifx_ppa_eth_tx_timeout_HW223,
                         .ndo_start_xmit       = ifx_ppa_eth_hard_start_xmit_HW223
                        },
   .sizeof_priv       = sizeof(avmnet_netdev_priv_t),
   .device_setup      = ether_setup,
   .device_setup_priv = ifx_ppa_setup_priv,
   .flags		  	  = AVMNET_DEVICE_IFXPPA_ETH_LAN,
   /*--- .mac_module        = &hw223_avmnet_mac_7port_1, ---*/
   .vlanID            = 2, 
   .vlanCFG           = S17_VTU_FUNC0_VALID | S17_VTU_FUNC0_IVL_EN | 
                        S17_VTU_FUNC0_EG_VLANMODE_PORT0(VLANMODE_TAGGED) |
                        S17_VTU_FUNC0_EG_VLANMODE_PORT1(VLANMODE_NOMEMBER) |
                        S17_VTU_FUNC0_EG_VLANMODE_PORT2(VLANMODE_UNTAGGED) |
                        S17_VTU_FUNC0_EG_VLANMODE_PORT3(VLANMODE_NOMEMBER) |
                        S17_VTU_FUNC0_EG_VLANMODE_PORT4(VLANMODE_NOMEMBER) |
                        S17_VTU_FUNC0_EG_VLANMODE_PORT5(VLANMODE_NOMEMBER) |
                        S17_VTU_FUNC0_EG_VLANMODE_PORT6(VLANMODE_NOMEMBER),
   .etht_stat         = {
                            DEFAULT_HW_STATS,
                            .gather_hw_stats = create_ar8337_rmon_cnt,
                        },
};

avmnet_device_t hw243_avmnet_avm_device_4 =
{
   .device            = NULL,
   .device_name       = "eth3.3",
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
                         .ndo_tx_timeout       = ifx_ppa_eth_tx_timeout_HW223,
                         .ndo_start_xmit       = ifx_ppa_eth_hard_start_xmit_HW223
                        },
   .sizeof_priv       = sizeof(avmnet_netdev_priv_t),
   .device_setup      = ether_setup,
   .device_setup_priv = ifx_ppa_setup_priv,
   .flags		  	  = AVMNET_DEVICE_IFXPPA_ETH_LAN,
   /*--- .mac_module        = &hw243_avmnet_mac_7port_1, ---*/
   .vlanID            = 3,      /*--- vlanID == sourcePortID ---*/
   .vlanCFG           = S17_VTU_FUNC0_VALID | S17_VTU_FUNC0_IVL_EN | 
                        S17_VTU_FUNC0_EG_VLANMODE_PORT0(VLANMODE_TAGGED) |
                        S17_VTU_FUNC0_EG_VLANMODE_PORT1(VLANMODE_NOMEMBER) |
                        S17_VTU_FUNC0_EG_VLANMODE_PORT2(VLANMODE_NOMEMBER) |
                        S17_VTU_FUNC0_EG_VLANMODE_PORT3(VLANMODE_UNTAGGED) |
                        S17_VTU_FUNC0_EG_VLANMODE_PORT4(VLANMODE_NOMEMBER) |
                        S17_VTU_FUNC0_EG_VLANMODE_PORT5(VLANMODE_NOMEMBER) |
                        S17_VTU_FUNC0_EG_VLANMODE_PORT6(VLANMODE_NOMEMBER),
   .etht_stat         = {
                            DEFAULT_HW_STATS,
                            .gather_hw_stats = create_ar8337_rmon_cnt,
                        },
};

avmnet_device_t hw243_avmnet_avm_device_5 =
{
   .device            = NULL,
   .device_name       = "wasp",
   .external_port_no  = 255,
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
   .flags		  	  = AVMNET_DEVICE_IFXPPA_ETH_LAN | AVMNET_DEVICE_FLAG_PHYS_OFFLOAD_LINK | AVMNET_DEVICE_FLAG_NO_MCFW,
   .offload_mac       = {0xba,0xdb,0xad,0xc0,0xff,0xee},
   .mac_module        = &hw243_avmnet_mac_7port_5,
   .etht_stat         = {
                            DEFAULT_HW_STATS,
                            DEFAULT_AVMNET_STATS,
                            .gather_hw_stats = create_7port_rmon_cnt,
                        },
};

avmnet_device_t hw243_avmnet_avm_device_6 =
{
   .device            = NULL,
   .device_name       = "vlan_master0",
   .external_port_no  = 255,
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
   .device_setup_late = hw223_late_init,
   .flags		  	  = AVMNET_DEVICE_IFXPPA_ETH_LAN,
   .mac_module        = &hw243_avmnet_mac_7port_1,
   .etht_stat         = {
                            DEFAULT_HW_STATS,
                            DEFAULT_AVMNET_STATS,
                            .gather_hw_stats = create_7port_rmon_cnt,
                        },
};

avmnet_device_t *hw243_avmnet_avm_devices[] = {
    &hw243_avmnet_avm_device_0,
    &hw243_avmnet_avm_device_1,
    &hw243_avmnet_avm_device_2,
    &hw243_avmnet_avm_device_3,
    &hw243_avmnet_avm_device_4,
    &hw243_avmnet_avm_device_5,
    &hw243_avmnet_avm_device_6,
}; 

avmnet_module_t hw243_avmnet =
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
    .num_children       = 5,
    .children           = {
                            &hw243_avmnet_mac_7port_5,
                            &hw243_avmnet_mac_7port_0,
                            &hw243_avmnet_mac_7port_1,
                            &hw243_avmnet_mac_7port_2,
                            &hw243_avmnet_mac_7port_4
                          }
};

extern avmnet_module_t hw243_avmnet_phy_fiber;
extern avmnet_module_t hw243_avmnet_phy_ar803x_0;
extern avmnet_module_t hw243_avmnet_phy_ar803x_1;
extern avmnet_module_t hw243_ath_switch;
extern avmnet_module_t hw243_avmnet_phy_11G_0;
extern avmnet_module_t hw243_avmnet_phy_11G_1;
extern avmnet_module_t hw243_avmnet_phy_wasp;
extern avmnet_module_t hw243_avmnet_dummy;

avmnet_module_t hw243_avmnet_mac_7port_0 =
{
    .name               = "mac_7port_0",
    .type               = avmnet_modtype_mac,
    .priv               = NULL,
    .initdata.mac       = { 
                            .mac_nr = 0,
                            .mac_mode = MAC_MODE_RGMII_1000,
                            .flags = AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_ASYM |
                                     AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_SYM,
                          },

     MAC_VR9_STDFUNCS,

    .parent             = &hw243_avmnet,
    .num_children       = 1,
    .children           = { &hw243_avmnet_phy_fiber }
};

avmnet_module_t hw243_avmnet_mac_7port_1 =
{
    .name               = "mac_7port_1",
    .type               = avmnet_modtype_mac,
    .priv               = NULL,
    .initdata.mac       = { 
                            .flags = AVMNET_CONFIG_FLAG_SWITCHPORT |
                                     AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_ASYM |
                                     AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_SYM,
                            .mac_nr = 1,
                            .mac_mode = MAC_MODE_RGMII_1000,
    },

    MAC_VR9_STDFUNCS,

    .parent             = &hw243_avmnet,
    .num_children       = 1,
    .children           = { &hw243_ath_switch }
};

avmnet_module_t hw243_avmnet_mac_7port_2 =
{
    .name               = "mac_7port_2",
    .type               = avmnet_modtype_mac,
    .priv               = NULL,
    .initdata.mac       = { 
                            .mac_nr = 2,
                            .mac_mode = MAC_MODE_GMII,
                            .flags = AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_ASYM |
                                     AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_SYM,
                        },

    MAC_VR9_STDFUNCS,

    .parent             = &hw243_avmnet,
    .num_children       = 1,
    .children           = { &hw243_avmnet_phy_11G_1 }
};

avmnet_module_t hw243_avmnet_mac_7port_4 =
{
    .name               = "mac_7port_4",
    .type               = avmnet_modtype_mac,
    .priv               = NULL,
    .initdata.mac       = { 
                            .mac_nr = 4,
                            .mac_mode =  MAC_MODE_GMII,
                            .flags = AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_ASYM |
                                     AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_SYM,
                        },

    MAC_VR9_STDFUNCS,

    .parent             = &hw243_avmnet,
    .num_children       = 1,
    .children           = { &hw243_avmnet_phy_11G_0 }
};

avmnet_module_t hw243_avmnet_mac_7port_5 =
{
    .name               = "mac_7port_5",
    .type               = avmnet_modtype_mac,
    .priv               = NULL,
    .initdata.mac       = {
                            .flags = AVMNET_CONFIG_FLAG_TX_DELAY |
                                     AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_ASYM |
                                     AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_SYM,
                            .mac_nr = 5,
                            .mac_mode =  MAC_MODE_RGMII_1000,
                            .rx_delay = 0,
                            .tx_delay = 1,
                        },

    MAC_VR9_STDFUNCS,

    .parent             = &hw243_avmnet,
    .num_children       = 1,
    .children           = { &hw243_avmnet_phy_wasp }
};

avmnet_module_t hw243_ath_switch  =
{
    .name           = "ar8334",
    .device_id      = &hw243_avmnet_avm_device_6,   /*--- das Interface zum hw243_avmnet_mac_7port_1 wird hier initialisiert ---*/
    .type           = avmnet_modtype_switch,
    .priv           = NULL,
    .initdata.swi   = { 
                        .flags = AVMNET_CONFIG_FLAG_TX_DELAY | 
                                 AVMNET_CONFIG_FLAG_RX_DELAY |
                                 AVMNET_CONFIG_FLAG_RESET,
                        .rx_delay = 0, 
                        .tx_delay = 3,
                        .reset = 44,
                      },

    .init           = avmnet_ar8337_init,
    .setup          = avmnet_ar8334_vlan_setup,
    .exit           = avmnet_ar8337_exit,

    .lock           = avmnet_s17_lock,
    .unlock         = avmnet_s17_unlock,
    .trylock        = avmnet_s17_trylock,

    .reg_read       = avmnet_s17_rd_phy,
    .reg_write      = avmnet_s17_wr_phy,
    .status_changed = avmnet_ar8337_status_changed,
    .poll           = avmnet_ar8337_HW223_poll,
    .set_status     = avmnet_ar8337_set_status,
    .setup_irq      = avmnet_ar8337_setup_interrupt,

    .parent         = &hw243_avmnet_mac_7port_1,
    .num_children   = 2,
    .children       = { &hw243_avmnet_phy_ar803x_0, &hw243_avmnet_phy_ar803x_1 }
};

avmnet_module_t hw243_avmnet_phy_ar803x_0 =
{
    .name               = "phy_ar803x_0",
    .device_id          = &hw243_avmnet_avm_device_3,
    .type               = avmnet_modtype_phy,
    .priv               = NULL,
    .initdata.phy       = {
                            .flags = AVMNET_CONFIG_FLAG_MDIOADDR |
                                     AVMNET_CONFIG_FLAG_PHY_GBIT | 
                                     AVMNET_CONFIG_FLAG_INTERNAL |
                                     AVMNET_CONFIG_FLAG_DEFAULT_EN_FC_RX,
                            .mdio_addr = 0x01,
                          },

     AR803X_STDFUNCS,
    .setup_special_hw   = avmnet_athrs17_setup_phy,

    .ethtool_ops        = AR803X_ETHOPS,

    .parent             = &hw243_ath_switch,
    .num_children       = 0,
    .children           = {}
};

avmnet_module_t hw243_avmnet_phy_ar803x_1 =
{
    .name               = "phy_ar803x_1",
    .device_id          = &hw243_avmnet_avm_device_4,
    .type               = avmnet_modtype_phy,
    .priv               = NULL,
    .initdata.phy       = {
                            .flags = AVMNET_CONFIG_FLAG_MDIOADDR |
                                     AVMNET_CONFIG_FLAG_PHY_GBIT |
                                     AVMNET_CONFIG_FLAG_INTERNAL |
                                     AVMNET_CONFIG_FLAG_DEFAULT_EN_FC_RX,
                            .mdio_addr = 0x02,
                          },

     AR803X_STDFUNCS,
    .setup_special_hw   = avmnet_athrs17_setup_phy,

    .ethtool_ops        = AR803X_ETHOPS,

    .parent             = &hw243_ath_switch,
    .num_children       = 0,
    .children           = {}
};

avmnet_module_t hw243_avmnet_phy_11G_0 =
{
    .name               = "phy_11G_0",
    .device_id          = &hw243_avmnet_avm_device_1,
    .type               = avmnet_modtype_phy,
    .priv               = NULL,
    .initdata.phy       = {
                            .flags = AVMNET_CONFIG_FLAG_MDIOADDR |
                                     AVMNET_CONFIG_FLAG_PHY_GBIT |
                                     AVMNET_CONFIG_FLAG_INTERNAL |
                                     AVMNET_CONFIG_FLAG_DEFAULT_EN_FC_RX,
                            .mdio_addr = 0x9,
                          },

     PHY_11G_STDFUNCS,
    
    .ethtool_ops        = PHY_11G_ETHOPS,

    .parent             = &hw243_avmnet_mac_7port_4,
    .num_children       = 0,
    .children           = {}
};

avmnet_module_t hw243_avmnet_phy_11G_1 =
{
    .name               = "phy_11G_1",
    .device_id          = &hw243_avmnet_avm_device_2,
    .type               = avmnet_modtype_phy,
    .priv               = NULL,
    .initdata.phy       = {
                            .flags = AVMNET_CONFIG_FLAG_MDIOADDR |
                                     AVMNET_CONFIG_FLAG_PHY_GBIT |
                                     AVMNET_CONFIG_FLAG_INTERNAL |
                                     AVMNET_CONFIG_FLAG_DEFAULT_EN_FC_RX,
                            .mdio_addr = 0x5,
                          },

     PHY_11G_STDFUNCS,
    
    .ethtool_ops        = PHY_11G_ETHOPS,

    .parent             = &hw243_avmnet_mac_7port_2,
    .num_children       = 0,
    .children           = {}
};

avmnet_module_t hw243_avmnet_phy_fiber =
{
    .name               = "phy_fiber",
    .device_id          = &hw243_avmnet_avm_device_0,
    .type               = avmnet_modtype_phy,
    .priv               = NULL,
    .initdata.phy       = {
                            .flags = AVMNET_CONFIG_FLAG_MDIOADDR |
                                     AVMNET_CONFIG_FLAG_RESET |
                                     AVMNET_CONFIG_FLAG_PHY_MODE_CONF |
                                     AVMNET_CONFIG_FLAG_PHY_GBIT |
                                     AVMNET_CONFIG_FLAG_DEFAULT_EN_FC_RX,
                            .mode_conf = ( ADVERTISED_1000baseT_Half
                                         | ADVERTISED_1000baseT_Full
                                         | ADVERTISED_100baseT_Half
                                         | ADVERTISED_100baseT_Full
                                         | ADVERTISED_Autoneg
                                         | ADVERTISED_FIBRE
                                         ),
                            .mdio_addr = 0x06,
                            .reset = 32,
                          },

    .init               = avmnet_ar803x_init,            
    .setup              = avmnet_ar803x_setup,           
    .exit               = avmnet_ar803x_exit,            

    .reg_read           = avmnet_ar803x_reg_read,        
    .reg_write          = avmnet_ar803x_reg_write,       
    .lock               = avmnet_ar803x_lock,            
    .unlock             = avmnet_ar803x_unlock,          
    .trylock            = avmnet_ar803x_trylock,         
    .status_changed     = avmnet_ar803x_status_changed,  

    .poll               = avmnet_hw223_fiber_poll,

    .setup_irq          = avmnet_ar803x_setup_interrupt, 
    .powerup            = avmnet_ar803x_powerup,         
    .powerdown          = avmnet_ar803x_powerdown,       
    .suspend            = avmnet_ar803x_suspend,         
    .resume             = avmnet_ar803x_resume,          
    .reinit             = avmnet_ar803x_reinit,

    .ethtool_ops        = AR803X_ETHOPS,

    .parent             = &hw243_avmnet_mac_7port_0,
    .num_children       = 0,
    .children           = {}
};

avmnet_module_t hw243_avmnet_phy_wasp =
{
    .name               = "phy_wasp",
    .device_id          = &hw243_avmnet_avm_device_5,
    .type               = avmnet_modtype_phy,
    .priv               = NULL,
    .initdata.phy       = {
                            .mdio_addr = 0x07,
                            .reset = 34,
                          },
    .init               = avmnet_phy_wasp_init,
    .setup              = avmnet_phy_wasp_setup,
    .exit               = avmnet_phy_wasp_exit,

    .reg_read           = avmnet_phy_wasp_reg_read,
    .reg_write          = avmnet_phy_wasp_reg_write,
    .lock               = avmnet_phy_wasp_lock,
    .unlock             = avmnet_phy_wasp_unlock,
    .status_changed     = avmnet_phy_wasp_status_changed,
    .set_status         = avmnet_phy_wasp_set_status,
    .poll               = avmnet_phy_wasp_poll,
    .setup_irq          = avmnet_phy_wasp_setup_irq,
    .powerup            = avmnet_phy_wasp_powerup,
    .powerdown          = avmnet_phy_wasp_powerdown,

    .parent             = &hw243_avmnet_mac_7port_5,
    .num_children       = 0,
    .children           = {}
};

#endif // __AVM_NET_CFG_HW243_
