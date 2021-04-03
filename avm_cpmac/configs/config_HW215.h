#if !defined(__AVM_NET_CFG_ATHEROS_HW215)
#define __AVM_NET_CFG_ATHEROS_HW215

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

extern avmnet_module_t hw215_gmac1, hw215_ath_switch, hw215_module_eth0;

avmnet_device_t avmnet_hw215_avm_device_0 ____cacheline_aligned =
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
   .mac_module        = &hw215_gmac1,
   .vlanID            = 1, // GMAC1, SW MAC 1
   .sizeof_priv       = sizeof(avmnet_netdev_priv_t),
   .device_setup      = athr_gmac_setup_eth,
   .device_setup_priv = athr_gmac_setup_eth_priv,
   .etht_stat         = {
                            DEFAULT_HW_STATS,
                            DEFAULT_NETDEV_STATS,
                            .gather_hw_stats = create_ar8326_rmon_cnt,
                        },
};

avmnet_device_t *avmnet_hw215_avm_devices[] = {
    &avmnet_hw215_avm_device_0
}; 

avmnet_module_t avmnet_HW215 ____cacheline_aligned =
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
    
    .status_changed = athmac_status_changed,
    .poll           = athmac_poll,
    .set_status     = athmac_set_status,

    .parent         = NULL,
    .num_children   = 1,
    .children       = { &hw215_gmac1 }
};

avmnet_module_t hw215_gmac1 ____cacheline_aligned =
{
    .name           = "gmac1",
    .type           = avmnet_modtype_switch,
    .priv           = NULL,
    .initdata.mac   = {  .flags = AVMNET_CONFIG_FLAG_BASEADDR
                                | AVMNET_CONFIG_FLAG_IRQ
                                | AVMNET_CONFIG_FLAG_SWITCHPORT,
                         .base_addr = ATH_GE1_BASE, 
                         .irq = ATH_CPU_IRQ_GE1, 
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

    .parent         = &avmnet_HW215,
    .num_children   = 1,
    .children       = { &hw215_ath_switch },
};

avmnet_module_t hw215_ath_switch ____cacheline_aligned =
{
    .name           = "ar8326",
    .type           = avmnet_modtype_switch,
    .priv           = NULL,
    /*--- .initdata       = { .swi = { .flags = AVMNET_CONFIG_FLAG_IRQ, .irq = ATH_MISC_IRQ_ENET_LINK }}, ---*/

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

    .parent         = &hw215_gmac1,
    .num_children   = 1,
    .children       = { &hw215_module_eth0 }
};

avmnet_module_t hw215_module_eth0 ____cacheline_aligned =
{
    .name           = "ar803x0",
    .device_id      = &avmnet_hw215_avm_device_0,
    .type           = avmnet_modtype_phy,
    .priv           = NULL,
    .initdata.phy   = { .flags = AVMNET_CONFIG_FLAG_MDIOADDR | AVMNET_CONFIG_FLAG_INTERNAL, .mdio_addr = 0 },

     AR803X_STDFUNCS,
    
    .ethtool_ops    = AR803X_ETHOPS,

    .parent         = &hw215_ath_switch,
    .num_children   = 0,
    .children       = {}
};

#endif
