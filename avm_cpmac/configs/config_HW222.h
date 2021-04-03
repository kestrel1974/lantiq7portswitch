#if !defined(__AVM_NET_CFG_ATHEROS_HW222)
#define __AVM_NET_CFG_ATHEROS_HW222

#include <avmnet_module.h>
#include <avmnet_config.h>
#include "../switch/atheros/atheros_mac.h"
#include "../phy/avmnet_ar803x.h"
#include "../phy/avmnet_ar8326.h"
#include "../phy/avmnet_ar8337.h"
#include "../phy/phy_plc.h"
#include "../phy/avmnet_mdio_bitbang.h"

#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,4,46)
#include <atheros.h>
#else
#include <avm_atheros.h>
#endif

extern avmnet_module_t hw222_gmac0, 
                       hw222_ath_switch, 
                       hw222_ath_switch_wan, 
                       hw222_module_eth0, 
                       hw222_module_plc;

avmnet_device_t avmnet_hw222_avm_device_0 ____cacheline_aligned =
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
   .mac_module        = &hw222_gmac0,
   .vlanID            = 3, // Port3 am ar8334
   .sizeof_priv       = sizeof(avmnet_netdev_priv_t),
   .device_setup      = athr_gmac_setup_eth,
   .device_setup_priv = athr_gmac_setup_eth_priv,
   .flags		  	  = AVMNET_DEVICE_IFXPPA_ETH_LAN,
   .etht_stat         = {
                            DEFAULT_HW_STATS,
                            DEFAULT_NETDEV_STATS,
                            .gather_hw_stats = create_ar8337_rmon_cnt,
                        },
};

avmnet_device_t avmnet_hw222_avm_device_plc ____cacheline_aligned =
{
   .device            = NULL,
   .device_name       = "plc",
   .external_port_no  = 255,
   .device_ops        = {
                          /*--- .ndo_get_stats        = athr_gmac_get_stats, ---*/
                          .ndo_open             = avmnet_netdev_open,
                          .ndo_stop             = avmnet_netdev_stop,
                          /*--- .ndo_do_ioctl         = athr_gmac_do_ioctl, ---*/
                          .ndo_tx_timeout       = athr_gmac_tx_timeout,
                          .ndo_start_xmit       = athr_gmac_hard_start
                        },
   .mac_module        = &hw222_gmac0,
   .vlanID            = 0, // Port0 am ar8334
   .sizeof_priv       = sizeof(avmnet_netdev_priv_t),
   .device_setup      = athr_gmac_setup_eth,
   .device_setup_priv = athr_gmac_setup_eth_priv,
   .flags		  	  = AVMNET_DEVICE_IFXPPA_ETH_LAN,
   .etht_stat         = {
                            DEFAULT_HW_STATS,
                            DEFAULT_NETDEV_STATS,
                            .gather_hw_stats = create_ar8337_rmon_cnt,
                        },
};


avmnet_device_t *avmnet_hw222_avm_devices[] = {
    &avmnet_hw222_avm_device_plc,
    &avmnet_hw222_avm_device_0
}; 

avmnet_module_t avmnet_HW222 ____cacheline_aligned =
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
    
    .status_changed = athmac_status_changed,
    .poll           = athmac_poll,
    .set_status     = athmac_set_status,

    .parent         = NULL,
    .num_children   = 1,
    .children       = { &hw222_gmac0 }
};

avmnet_module_t hw222_gmac0 ____cacheline_aligned =
{
    .name           = "gmac0",
    .type           = avmnet_modtype_mac,
    .priv           = NULL,
    .initdata.mac   = {  .flags = AVMNET_CONFIG_FLAG_BASEADDR
                                | AVMNET_CONFIG_FLAG_BASEADDR_MDIO
                                | AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_ASYM
                                | AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_SYM
                                | AVMNET_CONFIG_FLAG_IRQ
                                | AVMNET_CONFIG_FLAG_SWITCHPORT,
                         .base_addr = ATH_GE0_BASE, 
                         .base_mdio = ATH_GE1_BASE,
                         .irq = ATH_CPU_IRQ_GE0, 
                         .mac_nr = 1
                      },

    .init           = athmac_gmac_init,
    .setup          = athmac_gmac_setup,
    .exit           = athmac_gmac_exit,

    .lock           = athmac_gmac_lock,
    .unlock         = athmac_gmac_unlock,
    .trylock        = athmac_gmac_trylock,

    .reg_read       = athmac_reg_read,
    .reg_write      = athmac_reg_write,
    .status_changed = athmac_status_changed,
    .poll           = athmac_poll,
    .set_status     = athgmac_set_status,
    .setup_irq      = athmac_setup_irq,
    .setup_special_hw = hbee_setup_hw,

    .parent         = &avmnet_HW222,
    .num_children   = 1,
    .children       = { &hw222_ath_switch_wan },
};

avmnet_module_t hw222_ath_switch_wan ____cacheline_aligned =
{
    .name           = "ar8326",
    .type           = avmnet_modtype_switch,
    .priv           = NULL,
    .initdata.swi   = { .flags =   AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_ASYM
                                 | AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_SYM
                      },

    .init           = avmnet_ar8326_init_wan,
    .setup          = avmnet_ar8326_setup_wan_HW222,
    .exit           = avmnet_ar8326_exit_wan,

    .reg_read       = avmnet_bb_mdio_read,
    .reg_write      = avmnet_bb_mdio_write,
    .lock           = avmnet_s27_lock,
    .unlock         = avmnet_s27_unlock,
    .trylock        = avmnet_s27_trylock,
    .status_changed = avmnet_ar8326_status_changed,
    .poll           = avmnet_ar8326_poll_hbee_wan,
    .set_status     = avmnet_ar8326_set_status,
    .setup_irq      = avmnet_ar8326_setup_interrupt,
    .setup_special_hw = avmnet_bb_mdio_init,

    .parent         = &hw222_gmac0,
    .num_children   = 1,
    .children       = { &hw222_ath_switch },
};

avmnet_module_t hw222_ath_switch ____cacheline_aligned =
{
    .name           = "ar8334",
    .type           = avmnet_modtype_switch,
    .priv           = NULL,
    .initdata.swi   = { .flags = AVMNET_CONFIG_FLAG_TX_DELAY | 
                                 AVMNET_CONFIG_FLAG_RX_DELAY |
                                 AVMNET_CONFIG_FLAG_RESET |
                                 AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_SYM |
                                 AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_ASYM,
                        .reset = 4,
                        .rx_delay = 2, 
                        .tx_delay = 0  },

    .init           = avmnet_ar8337_init,
    .setup          = avmnet_ar8334_hbee_setup,
    .exit           = avmnet_ar8337_exit,

    .lock           = avmnet_s17_lock,
    .unlock         = avmnet_s17_unlock,
    .trylock        = avmnet_s17_trylock,

    .reg_read       = avmnet_s17_rd_phy,
    .reg_write      = avmnet_s17_wr_phy,
    .status_changed = avmnet_ar8337_status_changed,
    .poll           = avmnet_ar8337_status_poll,
    .set_status     = avmnet_ar8337_set_status,
    .setup_irq      = avmnet_ar8337_setup_interrupt,

    .parent         = &hw222_ath_switch_wan,
    .num_children   = 2,
    .children       = { &hw222_module_eth0, &hw222_module_plc }
};

avmnet_module_t hw222_module_eth0 ____cacheline_aligned =
{
    .name           = "ar803x0",
    .device_id      = &avmnet_hw222_avm_device_0,
    .type           = avmnet_modtype_phy,
    .priv           = NULL,
    .initdata.phy   = { .flags = (   AVMNET_CONFIG_FLAG_MDIOADDR
                                   | AVMNET_CONFIG_FLAG_PHY_GBIT 
                                   | AVMNET_CONFIG_FLAG_DEFAULT_EN_FC_RX
                                   | AVMNET_CONFIG_FLAG_DEFAULT_EN_FC_TX
                                 ),
                        .mdio_addr = 2 
                      },

     AR803X_STDFUNCS,
    .setup_special_hw = avmnet_athrs17_setup_phy,

    .ethtool_ops    = AR803X_ETHOPS,

    .parent         = &hw222_ath_switch,
    .num_children   = 0,
    .children       = {}
};

avmnet_module_t hw222_module_plc ____cacheline_aligned =
{
    .name           = "plc",
    .device_id      = &avmnet_hw222_avm_device_plc,
    .type           = avmnet_modtype_phy,
    .priv           = NULL,
    .initdata.phy   = {   .flags = (   AVMNET_CONFIG_FLAG_RESET
                                     | AVMNET_CONFIG_FLAG_PHY_GBIT
                                     | AVMNET_CONFIG_FLAG_MDIOADDR
                                   ),
                          .mdio_addr = 8,
                          .reset = 13
                      },
    .init               = avmnet_phy_plc_init,
    .setup              = avmnet_phy_plc_setup,
    .exit               = avmnet_phy_plc_exit,

    .reg_read           = avmnet_phy_plc_reg_read,
    .reg_write          = avmnet_phy_plc_reg_write,
    .lock               = avmnet_phy_plc_lock,
    .unlock             = avmnet_phy_plc_unlock,
    .trylock            = avmnet_phy_plc_trylock,
    .status_changed     = avmnet_phy_plc_status_changed,
    .set_status         = avmnet_phy_plc_set_status,
    .poll               = avmnet_phy_plc_poll,
    .setup_irq          = avmnet_phy_plc_setup_irq,
    .powerup            = avmnet_phy_plc_powerup,
    .powerdown          = avmnet_phy_plc_powerdown,

    .ethtool_ops    = {},

    .parent         = &hw222_ath_switch,
    .num_children   = 0,
    .children       = {}
};

#endif
