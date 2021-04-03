#if !defined(__AVM_NET_CFG_hw220x_)
#define __AVM_NET_CFG_hw220x_

#include <avmnet_module.h>
#include <avmnet_config.h>
#include "../switch/vlan_master/vlan_master.h"
#include "../phy/avmnet_ar803x.h"
#include "../phy/avmnet_ar8337.h"

avmnet_device_t hw220x_avmnet_avm_device_0 =
{
   .device            = NULL,
   .device_name       = "eth0",
   .external_port_no  = 0,
#if defined(CONFIG_AVM_SCATTER_GATHER)
   .net_dev_features  =  NETIF_F_SG | NETIF_F_IP_CSUM,
#endif
   .device_ops        = {
                        },
   .sizeof_priv       = sizeof(avmnet_netdev_priv_t),
   .device_setup      = ether_setup,
   .device_setup_priv = NULL,
   .flags             = AVMNET_CONFIG_FLAG_USE_VLAN
                        | AVMNET_DEVICE_IFXPPA_ETH_LAN,
   .vlanID            = 1,
   .vlanCFG           = S17_VTU_FUNC0_VALID | S17_VTU_FUNC0_IVL_EN |
                        S17_VTU_FUNC0_EG_VLANMODE_PORT0(VLANMODE_TAGGED) |
                        S17_VTU_FUNC0_EG_VLANMODE_PORT1(VLANMODE_UNTAGGED) |
                        S17_VTU_FUNC0_EG_VLANMODE_PORT2(VLANMODE_NOMEMBER) |
                        S17_VTU_FUNC0_EG_VLANMODE_PORT3(VLANMODE_NOMEMBER) |
                        S17_VTU_FUNC0_EG_VLANMODE_PORT4(VLANMODE_NOMEMBER) |
                        S17_VTU_FUNC0_EG_VLANMODE_PORT5(VLANMODE_NOMEMBER) |
                        S17_VTU_FUNC0_EG_VLANMODE_PORT6(VLANMODE_NOMEMBER),
   .etht_stat         = {
                            DEFAULT_HW_STATS,
                            DEFAULT_NETDEV_STATS_64,
                            .gather_hw_stats = create_ar8337_rmon_cnt,
                        },
};

avmnet_device_t hw220x_avmnet_avm_device_1 =
{
   .device            = NULL,
   .device_name       = "eth1",
   .external_port_no  = 1,
#if defined(CONFIG_AVM_SCATTER_GATHER)
   .net_dev_features  =  NETIF_F_SG | NETIF_F_IP_CSUM,
#endif
   .device_ops        = {
                        },
   .sizeof_priv       = sizeof(avmnet_netdev_priv_t),
   .device_setup      = ether_setup,
   .device_setup_priv = NULL,
   .flags             = AVMNET_CONFIG_FLAG_USE_VLAN
                        | AVMNET_DEVICE_IFXPPA_ETH_LAN,
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
                            DEFAULT_NETDEV_STATS_64,
                            .gather_hw_stats = create_ar8337_rmon_cnt,
                        },
};

avmnet_device_t hw220x_avmnet_avm_device_2 =
{
   .device            = NULL,
   .device_name       = "eth3",
   .external_port_no  = 3,
#if defined(CONFIG_AVM_SCATTER_GATHER)
   .net_dev_features  =  NETIF_F_SG | NETIF_F_IP_CSUM,
#endif
   .device_ops        = {
                        },
   .sizeof_priv       = sizeof(avmnet_netdev_priv_t),
   .device_setup      = ether_setup,
   .device_setup_priv = NULL,
   .flags             = AVMNET_CONFIG_FLAG_USE_VLAN
                        | AVMNET_DEVICE_IFXPPA_ETH_LAN,
   .vlanID            = 3,
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
                            DEFAULT_NETDEV_STATS_64,
                            .gather_hw_stats = create_ar8337_rmon_cnt,
                        },
};

avmnet_device_t hw220x_avmnet_avm_device_3 =
{
   .device            = NULL,
   .device_name       = "eth2",
   .external_port_no  = 2,
#if defined(CONFIG_AVM_SCATTER_GATHER)
   .net_dev_features  =  NETIF_F_SG | NETIF_F_IP_CSUM,
#endif
   .device_ops        = {
                        },
   .sizeof_priv       = sizeof(avmnet_netdev_priv_t),
   .device_setup      = ether_setup,
   .device_setup_priv = NULL,
   .flags             = AVMNET_CONFIG_FLAG_USE_VLAN
                        | AVMNET_DEVICE_IFXPPA_ETH_LAN,
   .vlanID            = 4,
   .vlanCFG           = S17_VTU_FUNC0_VALID | S17_VTU_FUNC0_IVL_EN |
                        S17_VTU_FUNC0_EG_VLANMODE_PORT0(VLANMODE_TAGGED) |
                        S17_VTU_FUNC0_EG_VLANMODE_PORT1(VLANMODE_NOMEMBER) |
                        S17_VTU_FUNC0_EG_VLANMODE_PORT2(VLANMODE_NOMEMBER) |
                        S17_VTU_FUNC0_EG_VLANMODE_PORT3(VLANMODE_NOMEMBER) |
                        S17_VTU_FUNC0_EG_VLANMODE_PORT4(VLANMODE_UNTAGGED) |
                        S17_VTU_FUNC0_EG_VLANMODE_PORT5(VLANMODE_NOMEMBER) |
                        S17_VTU_FUNC0_EG_VLANMODE_PORT6(VLANMODE_NOMEMBER),
   .etht_stat         = {
                            DEFAULT_HW_STATS,
                            DEFAULT_NETDEV_STATS_64,
                            .gather_hw_stats = create_ar8337_rmon_cnt,
                        },
};

avmnet_device_t *hw220x_avmnet_avm_devices[] = {
    &hw220x_avmnet_avm_device_0,
    &hw220x_avmnet_avm_device_1,
    &hw220x_avmnet_avm_device_3,
    &hw220x_avmnet_avm_device_2,
};

extern avmnet_module_t hw220x_avmnet_phy_ar803x_0;
extern avmnet_module_t hw220x_avmnet_phy_ar803x_1;
extern avmnet_module_t hw220x_avmnet_phy_ar803x_2;
extern avmnet_module_t hw220x_avmnet_phy_ar803x_3;
extern avmnet_module_t hw220x_ath_switch;

avmnet_module_t hw220x_avmnet =
{
    .name               = "vlan_master",
    .type               = avmnet_modtype_switch,
    .priv               = NULL,
    .initdata.swi       = {
                          },
    .init               = avmnet_vlan_master_init,
    .setup              = avmnet_vlan_master_setup,
    .exit               = avmnet_vlan_master_exit,

    .reg_read           = avmnet_vlan_master_reg_read,
    .reg_write          = avmnet_vlan_master_reg_write,
    .lock               = avmnet_vlan_master_lock,
    .unlock             = avmnet_vlan_master_unlock,
    .trylock            = avmnet_vlan_master_trylock,
    .status_changed     = avmnet_vlan_master_status_changed,
    .set_status         = avmnet_vlan_master_set_status,
    .poll               = avmnet_vlan_master_poll,
    .setup_irq          = avmnet_vlan_master_setup_irq,
    .powerup            = avmnet_vlan_master_powerup,
    .powerdown          = avmnet_vlan_master_powerdown,
    .suspend            = avmnet_vlan_master_suspend,
    .resume             = avmnet_vlan_master_resume,

    .parent             = NULL,
    .num_children       = 1,
    .children           = {
                            &hw220x_ath_switch
                          }
};

avmnet_module_t hw220x_ath_switch  =
{
    .name           = "ar8327",
    .type           = avmnet_modtype_switch,
    .priv           = NULL,
    .initdata.swi   = {
                        .flags = AVMNET_CONFIG_FLAG_TX_DELAY |
                                 AVMNET_CONFIG_FLAG_RX_DELAY ,
                        .rx_delay = 0,
                        .tx_delay = 3,
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

    .parent         = &hw220x_avmnet,
    .num_children   = 4,
    .children       = {
                        &hw220x_avmnet_phy_ar803x_0,
                        &hw220x_avmnet_phy_ar803x_1,
                        &hw220x_avmnet_phy_ar803x_2,
                        &hw220x_avmnet_phy_ar803x_3
                      }
};

avmnet_module_t hw220x_avmnet_phy_ar803x_0 =
{
    .name               = "phy_ar803x_0",
    .device_id          = &hw220x_avmnet_avm_device_0,
    .type               = avmnet_modtype_phy,
    .priv               = NULL,
    .initdata.phy       = {
                            .flags = AVMNET_CONFIG_FLAG_MDIOADDR |
                                     AVMNET_CONFIG_FLAG_PHY_GBIT |
                                     AVMNET_CONFIG_FLAG_INTERNAL |
                                     AVMNET_CONFIG_FLAG_DEFAULT_EN_FC_RX,
                            .mdio_addr = 0x00,
                          },

     AR803X_STDFUNCS,
    .setup_special_hw   = avmnet_athrs17_setup_phy,

    .ethtool_ops        = AR803X_ETHOPS,

    .parent             = &hw220x_ath_switch,
    .num_children       = 0,
    .children           = {}
};

avmnet_module_t hw220x_avmnet_phy_ar803x_1 =
{
    .name               = "phy_ar803x_1",
    .device_id          = &hw220x_avmnet_avm_device_1,
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

    .parent             = &hw220x_ath_switch,
    .num_children       = 0,
    .children           = {}
};

avmnet_module_t hw220x_avmnet_phy_ar803x_2 =
{
    .name               = "phy_ar803x_2",
    .device_id          = &hw220x_avmnet_avm_device_2,
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

    .parent             = &hw220x_ath_switch,
    .num_children       = 0,
    .children           = {}
};

avmnet_module_t hw220x_avmnet_phy_ar803x_3 =
{
    .name               = "phy_ar803x_3",
    .device_id          = &hw220x_avmnet_avm_device_3,
    .type               = avmnet_modtype_phy,
    .priv               = NULL,
    .initdata.phy       = {
                            .flags = AVMNET_CONFIG_FLAG_MDIOADDR |
                                     AVMNET_CONFIG_FLAG_PHY_GBIT |
                                     AVMNET_CONFIG_FLAG_INTERNAL |
                                     AVMNET_CONFIG_FLAG_DEFAULT_EN_FC_RX,
                            .mdio_addr = 0x03,
                          },

     AR803X_STDFUNCS,
    .setup_special_hw   = avmnet_athrs17_setup_phy,

    .ethtool_ops        = AR803X_ETHOPS,

    .parent             = &hw220x_ath_switch,
    .num_children       = 0,
    .children           = {}
};

#endif // __AVM_NET_CFG_hw220x_
