/*------------------------------------------------------------------------------------------*\
 *   Copyright (C) 2011 AVM GmbH <fritzbox_info@avm.de>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation version 2 of the License.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 \*------------------------------------------------------------------------------------------*/

#if !defined(__AVM_NET_MODULE_)
#define __AVM_NET_MODULE_

#include <linux/version.h>
#include <linux/list.h>
#include <linux/workqueue.h>
#include <linux/netdevice.h>
#include <linux/ethtool.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0)
#include <linux/netdev_features.h>
#endif

#if defined(CONFIG_IP_MULTICAST_FASTFORWARD)
#include <linux/mcfastforward.h>
#endif /*--- #if defined(CONFIG_IP_MULTICAST_FASTFORWARD) ---*/

struct _avmnet_module;
typedef struct _avmnet_device avmnet_device_t;
typedef struct _avmnet_module avmnet_module_t;

/*
 * Flags indicating the presence of config variables or device features (i.e. bugs)
 */
#define AVMNET_CONFIG_FLAG_RESET                    (1 <<  0) // [var:smp:reset]     device with a real reset line or internal phy mdio address register
#define AVMNET_CONFIG_FLAG_BASEADDR                 (1 <<  1) // [var:m:base_addr]   base address of MAC registers
#define AVMNET_CONFIG_FLAG_MDIOADDR                 (1 <<  2) // [var:p:mdio_addr]   MDIO address of PHY
#define AVMNET_CONFIG_FLAG_IRQ                      (1 <<  3) // [var:smp:irq]       IRQ line of device
#define AVMNET_CONFIG_FLAG_MDIOPOLLING              (1 <<  4) // [flg:sm?]           enable automatic polling of PHY status by MAC/switch
#define AVMNET_CONFIG_FLAG_INTERNAL                 (1 <<  5) // [flg:p]             PHY is integrated into switch/SoC
#define AVMNET_CONFIG_FLAG_RX_DELAY                 (1 <<  6) // [var:mp:rx_delay]   RX delay on MII
#define AVMNET_CONFIG_FLAG_TX_DELAY                 (1 <<  7) // [var:mp:tx_delay]   TX delay on MII
#define AVMNET_CONFIG_FLAG_PHY_GBIT                 (1 <<  8) // [flg:p]             PHY is GBit version, used to distinguish between ar803[05] and PHY11G/22F
#define AVMNET_CONFIG_FLAG_POLARITY                 (1 <<  9) // [var:p:polarity]    apply polarity correction on link lines
#define AVMNET_CONFIG_FLAG_RST_ON_LINKFAIL          (1 << 10) // [flg:p]             PHY is broken and needs to be reset after link fail
#define AVMNET_CONFIG_FLAG_LINKFAIL_TIME            (1 << 11) // [var:p:lnkf_time]   wait at least x jiffies before reporting link down
#define AVMNET_CONFIG_FLAG_IRQ_ON                   (1 << 12) // [flg:?mp]           enable IRQ, do not wait for upper module to enable it
#define AVMNET_CONFIG_FLAG_SWITCHPORT               (1 << 13) // [flg:m]             MAC is connected to a switch
#define AVMNET_CONFIG_FLAG_PHY_MODE_CONF            (1 << 14) // [var:p:mode_conf]   limit PHY modes to this ethtool flags
#define AVMNET_CONFIG_FLAG_PHY_PWDWN_ON_MAC_SUSPEND (1 << 15) // [var:p:mode_conf]   Put PHY to powerdown, while MAC suspended, might help to reinit DMA on IFX-Platforms
#define AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_SYM       (1 << 16) // [flg:m]             MAC supports SYM Pause
#define AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_ASYM      (1 << 17) // [flg:m]             MAC supports ASYM Pause
#define AVMNET_CONFIG_FLAG_DEFAULT_EN_FC_RX         (1 << 18) // [flg:p]             default flow control setup: enable rx pause
#define AVMNET_CONFIG_FLAG_DEFAULT_EN_FC_TX         (1 << 19) // [flg:p]             default flow control setup: enable tx pause
#define AVMNET_CONFIG_FLAG_BASEADDR_MDIO            (1 << 20) // [flg:m]             base address of MAC MDIO registers (differs on WASP/SCORPION)
#define AVMNET_CONFIG_FLAG_NO_WANPORT               (1 << 21) // [flg:m]             on Atheros, connect WAN port PHY to switch
#define AVMNET_CONFIG_FLAG_OPEN_FIRMWARE            (1 << 22) // [flg:p]??           Read baseaddr, irq, mdio, etc. from device tree
#define AVMNET_CONFIG_FLAG_SWITCH_WAN               (1 << 23)
#define AVMNET_CONFIG_FLAG_SWITCH_LAN               (1 << 24)
#define AVMNET_CONFIG_FLAG_SGMII_ENABLE             (1 << 25) // [flg:m]             on Atheros, enabled SGMII-Link
#define AVMNET_CONFIG_FLAG_USE_VLAN                 (1 << 26) // [var:s]             VLAN-Config Value
#define AVMNET_CONFIG_FLAG_USE_GPON                 (1 << 27) // [var:s]             VLAN-Config Value

#define NETDEV_NAME_LEN                 32

#define AVMNET_DEVICE_FLAG_INITIALIZED               (1 << 0)
#define AVMNET_DEVICE_FLAG_WAIT_FOR_MODULE_FUNCTIONS (1 << 1)
#define AVMNET_DEVICE_FLAG_DMA_TX_QUEUE_FULL         (1 << 2)
#define AVMNET_DEVICE_FLAG_NO_MCFW                   (1 << 3)
#define AVMNET_DEVICE_FLAG_PHYS_OFFLOAD_LINK         (1 << 4)
/*
 * TAKE CARE: BIT (1 << 10) ... (1 << 16) reserved for AVM_PA DEVICE FLAGS!
 */

typedef union _avmnet_linkstatus_
{
    struct _avmnet_linkstatus
    {
        unsigned int link        : 1;
        unsigned int speed       : 2; /*--- 0-10MBit 1-100MBit 2-1GBit 3-undefined ---*/
        unsigned int fullduplex  : 1;
        unsigned int powerup     : 1;
        unsigned int flowcontrol : 2; /*--- rx = 1 tx = 2 ---*/
        unsigned int mdix        : 2; /*--- invalid = 0 mdi = 1 mdix = 2 ---*/
    } Details;
    unsigned int Status;
} avmnet_linkstatus_t, *p_avmnet_linkstatus_t;

#define AVMNET_LINKSTATUS_FC_RX     (0x1)
#define AVMNET_LINKSTATUS_FC_TX     (0x2)
#define AVMNET_LINKSTATUS_FC_RXTX   (0x3)

#define AVMNET_LINKSTATUS_SPD_10    (0x0)
#define AVMNET_LINKSTATUS_SPD_100   (0x1)
#define AVMNET_LINKSTATUS_SPD_1000  (0x2)

#define AVMNET_LINKSTATUS_MDI_INV   (0x0)
#define AVMNET_LINKSTATUS_MDI_MDI   (0x1)
#define AVMNET_LINKSTATUS_MDI_MDIX  (0x2)

#define AVMNET_POLARITY_MDI         (1 << 0)
#define AVMNET_POLARITY_MDIX        (1 << 1)
#define AVMNET_POLARITY_10MBIT      (1 << 2)
#define AVMNET_POLARITY_100MBIT     (1 << 3)
#define AVMNET_POLARITY_1000MBIT    (1 << 4)
#define AVMNET_POLARITY_SHIFT_WIDTH (8)
#define AVMNET_POLARITY_A_SHIFT     (0)
#define AVMNET_POLARITY_B_SHIFT     (8)
#define AVMNET_POLARITY_C_SHIFT     (16)
#define AVMNET_POLARITY_D_SHIFT     (24)

#define MAC_MODE_AUTO       0
#define MAC_MODE_MII        1
#define MAC_MODE_RMII       2
#define MAC_MODE_GMII       3
#define MAC_MODE_RGMII_100  4
#define MAC_MODE_RGMII_1000 5
#define MAC_MODE_SGMII	    6

#define AVMNET_DEVICE_INVALID_EXTERNAL_PORT 255

struct avmnet_initdata_swi
{
    unsigned int flags;
    int reset;
    int irq;
    int cpu_mac_port;
    unsigned int rx_delay;
    unsigned int tx_delay;
};

struct avmnet_initdata_mac
{
    unsigned int flags;
    unsigned int base_addr;
    unsigned int base_mdio;
    unsigned int mac_mode;
    unsigned int mac_nr;
    unsigned int rx_delay;
    unsigned int tx_delay;
    int reset;
    int irq;
    const char *of_compatible;
};

struct avmnet_initdata_phy
{
    unsigned int flags;
    unsigned int mdio_addr;
    unsigned int load_fw;
    unsigned int rx_delay;
    unsigned int tx_delay;
    unsigned int polarity;
    unsigned int lnkf_time;
    unsigned int mode_conf;
    int reset;
    int irq;
};

union avmnet_module_initdata
{
    struct avmnet_initdata_swi swi;
    struct avmnet_initdata_mac mac;
    struct avmnet_initdata_phy phy;
};

enum rst_type {
    rst_on,
    rst_off,
    rst_pulse
};

typedef enum _avmnet_modtype
{
    avmnet_modtype_undef = 0, //
    avmnet_modtype_phy,       //
    avmnet_modtype_mac,       //
    avmnet_modtype_switch,    //
    avmnet_modtype_pa,        // packet accelerator
} avmnet_modtype_t;

enum avmnet_pause_setup {
	avmnet_pause_none = 0,
	avmnet_pause_rx,
	avmnet_pause_tx,
	avmnet_pause_rx_tx,
};

typedef struct _avmnet_ethtool_ops
{
    int (*get_settings)(avmnet_module_t *, struct ethtool_cmd *);
    int (*set_settings)(avmnet_module_t *, struct ethtool_cmd *);
    void (*get_drvinfo)(avmnet_module_t *, struct ethtool_drvinfo *);
    int (*get_regs_len)(avmnet_module_t *);
    void (*get_regs)(avmnet_module_t *, struct ethtool_regs *, void *);
    void (*get_wol)(avmnet_module_t *, struct ethtool_wolinfo *);
    int (*set_wol)(avmnet_module_t *, struct ethtool_wolinfo *);
    u32 (*get_msglevel)(avmnet_module_t *);
    void (*set_msglevel)(avmnet_module_t *, u32);
    int (*nway_reset)(avmnet_module_t *);
    u32 (*get_link)(avmnet_module_t *);
    int (*get_eeprom_len)(avmnet_module_t *);
    int (*get_eeprom)(avmnet_module_t *, struct ethtool_eeprom *, u8 *);
    int (*set_eeprom)(avmnet_module_t *, struct ethtool_eeprom *, u8 *);
    int (*get_coalesce)(avmnet_module_t *, struct ethtool_coalesce *);
    int (*set_coalesce)(avmnet_module_t *, struct ethtool_coalesce *);
    void (*get_ringparam)(avmnet_module_t *, struct ethtool_ringparam *);
    int (*set_ringparam)(avmnet_module_t *, struct ethtool_ringparam *);
    void (*get_pauseparam)(avmnet_module_t *, struct ethtool_pauseparam*);
    int (*set_pauseparam)(avmnet_module_t *, struct ethtool_pauseparam*);
    void (*self_test)(avmnet_module_t *, struct ethtool_test *, u64 *);
    void (*get_strings)(avmnet_module_t *, u32 stringset, u8 *);
    void (*get_ethtool_stats)(avmnet_module_t *, struct ethtool_stats *, u64 *);
    int (*begin)(avmnet_module_t *);
    void (*complete)(avmnet_module_t *);
    u32 (*get_flags)(avmnet_module_t *);
    int (*set_flags)(avmnet_module_t *, u32);
    u32 (*get_priv_flags)(avmnet_module_t *);
    int (*set_priv_flags)(avmnet_module_t *, u32);
    int (*get_sset_count)(avmnet_module_t *, int);

    /* the following hooks are obsolete */
    int (*self_test_count)(avmnet_module_t *);/* use get_sset_count */
    int (*get_stats_count)(avmnet_module_t *);/* use get_sset_count */
    int (*get_rxnfc)(avmnet_module_t *, struct ethtool_rxnfc *, void *);
    int (*set_rxnfc)(avmnet_module_t *, struct ethtool_rxnfc *);
    int (*flash_device)(avmnet_module_t *, struct ethtool_flash *);
} avmnet_ethtool_ops_t;

typedef struct _avmnet_ethtool_table
{
    avmnet_module_t *get_settings;
    avmnet_module_t *set_settings;
    avmnet_module_t *get_drvinfo;
    avmnet_module_t *get_regs_len;
    avmnet_module_t *get_regs;
    avmnet_module_t *get_wol;
    avmnet_module_t *set_wol;
    avmnet_module_t *get_msglevel;
    avmnet_module_t *set_msglevel;
    avmnet_module_t *nway_reset;
    avmnet_module_t *get_link;
    avmnet_module_t *get_eeprom_len;
    avmnet_module_t *get_eeprom;
    avmnet_module_t *set_eeprom;
    avmnet_module_t *get_coalesce;
    avmnet_module_t *set_coalesce;
    avmnet_module_t *get_ringparam;
    avmnet_module_t *set_ringparam;
    avmnet_module_t *get_pauseparam;
    avmnet_module_t *set_pauseparam;
    avmnet_module_t *get_rx_csum;
    avmnet_module_t *set_rx_csum;
    avmnet_module_t *get_tx_csum;
    avmnet_module_t *set_tx_csum;
    avmnet_module_t *get_sg;
    avmnet_module_t *set_sg;
    avmnet_module_t *get_tso;
    avmnet_module_t *set_tso;
    avmnet_module_t *self_test;
    avmnet_module_t *get_strings;
    avmnet_module_t *phys_id;
    avmnet_module_t *get_ethtool_stats;
    avmnet_module_t *begin;
    avmnet_module_t *complete;
    avmnet_module_t *get_ufo;
    avmnet_module_t *set_ufo;
    avmnet_module_t *get_flags;
    avmnet_module_t *set_flags;
    avmnet_module_t *get_priv_flags;
    avmnet_module_t *set_priv_flags;
    avmnet_module_t *get_sset_count;

    /* the following hooks are obsolete */
    avmnet_module_t *self_test_count;
    avmnet_module_t *get_stats_count;
    avmnet_module_t *get_rxnfc;
    avmnet_module_t *set_rxnfc;
    avmnet_module_t *flash_device;
} avmnet_ethtool_table_t;

enum avmnet_etht_stat_type {
    AVMNET_RMON_STATS,
    LINUX_NETDEV_STATS,
    LINUX_NETDEV_STATS_64,
    AVMNET_NETDEV_STATS
};

struct avmnet_etht_stat_entry {
    char stat_string[ETH_GSTRING_LEN];
    enum avmnet_etht_stat_type type;
    int sizeof_stat;
    int stat_offset;
};

struct avmnet_hw_rmon_counter {
    u64 rx_pkts_good;
    u64 tx_pkts_good;
    u64 rx_bytes_good;
    u64 tx_bytes_good;
    u64 rx_pkts_pause;
    u64 tx_pkts_pause;
    u64 rx_pkts_dropped;
    u64 tx_pkts_dropped;
    u64 rx_bytes_error;
};

struct avmnet_etht_stat {
    size_t hw_cnt;
    size_t sw_cnt;
    struct avmnet_etht_stat_entry *hw_entries;
    struct avmnet_etht_stat_entry *sw_entries;
    struct avmnet_hw_rmon_counter *(*gather_hw_stats)(avmnet_device_t *);
};

typedef struct _avmnet_device
{
    avmnet_linkstatus_t status;
    struct net_device *device;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0)
    netdev_features_t net_dev_features;
#else
    unsigned int net_dev_features;
#endif
    struct napi_struct napi;
    avmnet_module_t *mac_module;
    struct net_device_ops device_ops;
    struct net_device_stats device_stats;
    const char device_name[NETDEV_NAME_LEN];
    unsigned char external_port_no; /* 255 indicates, that this is no external port */
    const unsigned char offload_mac[ETH_ALEN];
    unsigned int vlanID;
    unsigned int vlanCFG;
    int sizeof_priv;
    avmnet_ethtool_table_t ethtool_module_table;
    volatile unsigned int flags;
    
    struct avmnet_etht_stat etht_stat;

    void (*device_setup)(struct net_device *);    /* wird waehrend netdev_alloc ausgefuehrt */
    void (*device_setup_priv)(avmnet_device_t *); /* wird nach netdev_alloc ausgefuehrt, Treiber kann
                                                     hier avmnet_device_t in seinen priv-data eintragen */
    void (*device_setup_late)(avmnet_device_t *);
        /* wird nach netdev_alloc() und register_netdev() ausgeführt,
         * net_device und net_device->dev sind ordentlich initialisiert */
} avmnet_device_t;


#define DEFAULT_HW_STAT_CNT 9
#define DEFAULT_NETDEV_STAT_CNT 8
#define DEFAULT_NETDEV_STAT_64_CNT 8
#define DEFAULT_AVMNET_STAT_CNT 8

#define DEFAULT_HW_STATS .hw_cnt = ARRAY_SIZE(default_hw_stats),\
                         .hw_entries = default_hw_stats

#define DEFAULT_NETDEV_STATS_64 .sw_cnt = ARRAY_SIZE(default_netdev_stats_64),\
                                .sw_entries = default_netdev_stats_64

#define DEFAULT_NETDEV_STATS .sw_cnt = ARRAY_SIZE(default_netdev_stats),\
                             .sw_entries = default_netdev_stats

#define DEFAULT_AVMNET_STATS .sw_cnt = ARRAY_SIZE(default_avmnet_stats),\
                             .sw_entries = default_avmnet_stats

extern struct avmnet_etht_stat_entry default_hw_stats[DEFAULT_HW_STAT_CNT];
extern struct avmnet_etht_stat_entry default_netdev_stats[DEFAULT_NETDEV_STAT_CNT];
extern struct avmnet_etht_stat_entry default_netdev_stats_64[DEFAULT_NETDEV_STAT_64_CNT];
extern struct avmnet_etht_stat_entry default_avmnet_stats[DEFAULT_AVMNET_STAT_CNT];

#define AVMNET_RMON_STAT(str, m) { \
    .stat_string = str, \
    .type = AVMNET_RMON_STATS, \
    .sizeof_stat = sizeof(((struct avmnet_hw_rmon_counter*)0)->m), \
    .stat_offset = offsetof(struct avmnet_hw_rmon_counter, m) }

#define LINUX_NETDEV_STAT(str, m) { \
    .stat_string = str, \
    .type = LINUX_NETDEV_STATS, \
    .sizeof_stat = sizeof(((struct net_device *)0)->stats.m), \
    .stat_offset = offsetof(struct net_device, stats.m) }

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,34)
#define LINUX_NETDEV_STAT_64(str, m) { \
    .stat_string = str, \
    .type = LINUX_NETDEV_STATS_64, \
    .sizeof_stat = sizeof(((struct rtnl_link_stats64 *)0)->m), \
    .stat_offset = offsetof(struct rtnl_link_stats64, m) }
#endif

#define AVMNET_NETDEV_STAT(str, m) { \
    .stat_string = str, \
    .type = AVMNET_NETDEV_STATS, \
    .sizeof_stat = sizeof(((avmnet_device_t *)0)->device_stats.m), \
    .stat_offset = offsetof(avmnet_device_t, device_stats.m) }

typedef struct avmnet_netdev_priv {
    avmnet_device_t       *avm_dev;
    signed char            lantiq_device_index;
    unsigned long          lantiq_ppe_hold_timeout;
#   if defined(CONFIG_IP_MULTICAST_FASTFORWARD)
    unsigned char          mcfw_init_done;
    unsigned char          mcfw_used;
    struct mcfw_netdriver  mcfw_netdrv;
    int                    mcfw_sourceid;
    struct tasklet_struct  mcfw_tasklet;
    struct sk_buff_head    mcfw_queue;
#   endif /*--- #if defined(CONFIG_IP_MULTICAST_FASTFORWARD) ---*/
} avmnet_netdev_priv_t;

typedef struct _avmnet_module
{
    char *name;
    avmnet_device_t *device_id;
    avmnet_modtype_t type;
    union avmnet_module_initdata initdata;
    void *priv;

    int (*init)(avmnet_module_t *);
    int (*setup)(avmnet_module_t *);
    int (*exit)(avmnet_module_t *);

    unsigned int (*reg_read)(avmnet_module_t *, unsigned int addr, unsigned int reg);
    int (*reg_write)(avmnet_module_t *, unsigned int addr, unsigned int reg, unsigned int val);
    uint32_t (*reg_read32)(avmnet_module_t *, unsigned int addr, unsigned int reg);
    int (*reg_write32)(avmnet_module_t *, unsigned int addr, unsigned int reg, uint32_t val);

    int (*lock)(avmnet_module_t *);
    void (*unlock)(avmnet_module_t *);
    int (*trylock)(avmnet_module_t *);

    void (*status_changed)(avmnet_module_t *, avmnet_module_t *);
    int (*set_status)(avmnet_module_t *, avmnet_device_t *id, avmnet_linkstatus_t status);
    int (*poll)(avmnet_module_t *);
    int (*setup_irq)(avmnet_module_t *, unsigned int on);
    int (*setup_special_hw)(avmnet_module_t *);

    int (*powerdown)(avmnet_module_t *);
    int (*powerup)(avmnet_module_t *);
    int (*suspend)(avmnet_module_t *, avmnet_module_t *);
    int (*resume)(avmnet_module_t *, avmnet_module_t *);
    int (*reinit)(avmnet_module_t *);

    int (*set_gpio)(avmnet_module_t *, unsigned int gpio, unsigned int on);

    avmnet_ethtool_ops_t ethtool_ops;

    avmnet_module_t *parent;
    int num_children;
    avmnet_module_t *children[];
} avmnet_module_t;


/*------------------------------------------------------------------------------------------*\
 * timer
\*------------------------------------------------------------------------------------------*/
typedef enum
{
    none,
    poll
} avmnet_timer_enum_t;

typedef struct
{
    struct list_head list;
    avmnet_module_t *module;
    avmnet_timer_enum_t type;
    struct workqueue_struct *workqueue;
    struct delayed_work work;
} avmnet_timer_t;

#endif // !defined(_AVM_NET_MODULE_)
