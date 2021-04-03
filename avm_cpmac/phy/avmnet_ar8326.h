#if !defined(_AVM_NET_AR8327_H_)
#define _AVM_NET_AR8327_H_

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0)
#define soc_is_ar934x   is_ar934x
#define soc_is_qca955x  is_qca955x
#define soc_is_qca9531  is_qca9531
#define soc_is_qca956x  is_qca956x
#if defined(CONFIG_MACH_QCA956x) || defined(CONFIG_SOC_QCA956X)
#define MISC_BIT_ENET_LINK RST_MISC_INTERRUPT_STATUS_S27_MAC_INT_LSB
#else
#define MISC_BIT_ENET_LINK RST_MISC_INTERRUPT_MASK_S26_MAC_INT_MASK_LSB
#endif
#endif

typedef struct ar8326_work_struct {
    struct workqueue_struct *workqueue;
    struct delayed_work      work;
} ar8326_work_t;

struct aths27_context {
    avmnet_module_t         *this_module;
    ar8326_work_t           link_work;
    unsigned int irq;
    unsigned int reset;

    struct semaphore lock;      // mutex used by children for synchronisation among each other
    struct semaphore mdio_lock; // mutex used for protecting local MDIO access
#define AR8326_PORTS 6
#define AR8326_RMON_COUNTER 42
    u32 rmon_cache[AR8326_PORTS][AR8326_RMON_COUNTER];
};

extern struct avmnet_hw_rmon_counter *create_ar8326_rmon_cnt(avmnet_device_t *avm_dev);

/*------------------------------------------------------------------------------------------*\
 * S27 CSR Registers
\*------------------------------------------------------------------------------------------*/
#define S27_MASK_CTL_REG		                 0x0000
#define     S27_MASK_CTRL_RESET           (1 << 31)

#define S27_OPMODE_REG0                          0x0004
#define     S27_MAC0_MAC_GMII_EN                     (1 << 6) 

#define S27_OPMODE_REG1                          0x0008
#define     S27_PHY4_RMII_EN                         (1 << 29)
#define     S27_PHY4_MII_EN                          (1 << 28)
#define     S27_MAC5_RGMII_EN                        (1 << 26) 

#define S27_OPMODE_REG2                          0x000C
#define S27_PWRSTRAP_REG                         0x0010
#define S27_GLOBAL_INTR_REG                      0x0014
#define     S27_GINT_EEPROM_INT                     (1<<0)
#define     S27_GINT_EEPROM_ERR_INT                 (1<<1)
#define     S27_GINT_PHY_INT                        (1<<2)
#define     S27_GINT_MDIO_INT                       (1<<3)
#define     S27_GINT_ARL_DONE_INT                   (1<<4)
#define     S27_GINT_ARL_FULL_INT                   (1<<5)
#define     S27_GINT_AT_INI_INT                     (1<<6)
#define     S27_GINT_QM_INI_INT                     (1<<7)
#define     S27_GINT_VT_DONE_INT                    (1<<8)
#define     S27_GINT_VT_MEM_INT                     (1<<9)
#define     S27_GINT_VT_MIS_VIO_INT                 (1<<10)
#define     S27_GINT_BIST_DONE_INT                  (1<<11)
#define     S27_GINT_MIB_DONE_INT                   (1<<12)
#define     S27_GINT_MIB_INI_INT                    (1<<13)
#define     S27_GINT_HW_INI_DONE                    (1<<14)
#define     S27_GINT_LOOP_CHECK_INT                 (1<<18)

#define S27_GLOBAL_INTR_MASK_REG                 0x0018
#define S27_FLD_MASK_REG                         0x002c
/*--- #define     S27_ENABLE_CPU_BROADCAST                (1 << 26) ---*/
/*--- #define     S27_ENABLE_CPU_BCAST_FWD                (1 << 25) ---*/
#define S27_UNI_FLOOD_DP(mask)                  ((mask) << 0)
#define S27_IGMP_JOIN_LEAVE_DP(mask)            ((mask) << 8)
#define S27_MULTI_FLOOD_DP(mask)                ((mask) << 16)
#define S27_ARL_MULTI_LEAKY_EN                  (1 << 23)
#define S27_ARL_UNI_LEAKY_EN                    (1 << 24)
#define S27_BROAD_DP(mask)                      ((mask) << 25)

#define S27_GLOBAL_CTRL_REG                      0x0030
#define S27_FLCTL_REG0				             0x0034
#define S27_FLCTL_REG1				             0x0038
#define S27_QM_CTRL_REG                          0x003C
#define S27_VLAN_TABLE_REG0                      0x0040

#define S27_VLAN_TABLE_REG0_VLAN_OP(op)          ((op) << 0)
#define S27_VLAN_TABLE_REG0_VLAN_OP_NOP             0
#define S27_VLAN_TABLE_REG0_VLAN_OP_FLUSH           1
#define S27_VLAN_TABLE_REG0_VLAN_OP_LOAD            2
#define S27_VLAN_TABLE_REG0_VLAN_OP_PURGE           3
#define S27_VLAN_TABLE_REG0_VLAN_OP_REMOVE          4
#define S27_VLAN_TABLE_REG0_VLAN_OP_GET_NEXT        5
#define S27_VLAN_TABLE_REG0_VLAN_OP_READ            6
#define S27_VLAN_TABLE_REG0_BUSY                 (1 << 3)


#define S27_VLAN_TABLE_REG1                      0x0044
#define S27_ARL_TBL_FUNC_REG0                    0x0050
#define S27_ARL_TBL_FUNC_REG1                    0x0054
#define S27_ARL_TBL_FUNC_REG2                    0x0058
#define S27_ARL_TBL_CTRL_REG                     0x005c
#define S27_IP_PRI_MAP_REG(a)             (0x0060 + ((a) * 4))

#define S27_TAG_PRI_MAP_REG                      0x0070
#define     S27_TAGPRI_DEFAULT                       0xFA50

#define S27_SERVICE_TAG_REG                      0x0074
#define S27_CPU_PORT_REG                         0x0078
#define S27_CPU_PORT_REG_ENABLE                  (1 << 8)
#define S27_CPU_PORT_REG_MIRROR_MASK(mask)       ((mask) << 4)
#define S27_CPU_PORT_REG_MIRROR_MASK_DISABLE     0xf



#define S27_MIB_FUNCTION                         0x0080
#  define S27_MIB_FUNCTION_MIB_EN                (1u << 30)

#define S27_MDIO_CTRL_REG                        0x0098
#define     S27_MDIO_BUSY                            (1 << 31)
#define     S27_MDIO_MASTER                          (1 << 30)
#define     S27_MDIO_CMD_RD                          (1 << 27)
#define     S27_MDIO_CMD_WR                          (0 << 27)
#define     S27_MDIO_SUP_PRE                         (1 << 26)
#define     S27_MDIO_PHY_ADDR                        21
#define     S27_MDIO_REG_ADDR                        16

#define S27_PORT_STATUS_REG(a)          ((0x100 * (a)) + 0x100)
#define     S27_PS_FLOW_LINK_EN                      (1 << 12)
#define     S27_PS_LINK_ASYN_PAUSE_EN                (1 << 11)
#define     S27_PS_LINK_PAUSE_EN                     (1 << 10)
#define     S27_PS_LINK_EN                           (1 << 9)
#define     S27_PS_LINK                              (1 << 8)
#define     S27_PS_TXH_FLOW_EN                       (1 << 7)
#define     S27_PS_DUPLEX                            (1 << 6)
#define     S27_PS_RX_FLOW_EN                        (1 << 5)
#define     S27_PS_TX_FLOW_EN                        (1 << 4)
#define     S27_PS_RXMAC_EN                          (1 << 3)
#define     S27_PS_TXMAC_EN                          (1 << 2)
#define     S27_PS_SPEED_1000                        (1 << 1)
#define     S27_PS_SPEED_100                         (1 << 0)
#define     S27_PS_SPEED_10                                0
#define     S27_PORT_STATUS_DEFAULT                  (S27_PS_FLOW_LINK_EN | S27_PS_LINK_EN | S27_PS_TXH_FLOW_EN)

#define S27_PORT_CONTROL_REG(a)         ((0x100 * (a)) + 0x104)
#define     S27_EAPOL_EN                             (1 << 23)
#define     S27_ARP_LEAKY_EN                         (1 << 22)
#define     S27_IGMP_LEAVE_EN                        (1 << 21)
#define     S27_IGMP_JOIN_EN                         (1 << 20)
#define     S27_DHCP_EN                              (1 << 19)
#define     S27_IPG_DEC_EN                           (1 << 18)
#define     S27_ING_MIRROR_EN                        (1 << 17)
#define     S27_EG_MIRROR_EN                         (1 << 16)
#define     S27_LEARN_EN                             (1 << 14)
#define     S27_MAC_LOOP_BACK                        (1 << 12)
#define     S27_HEADER_EN              	             (1 << 11)
#define     S27_IGMP_MLD_EN                          (1 << 10)
#define     S27_EG_VLAN_MODE(a)                      ((a)<<8)
#define         S27_VLAN_UNMODIFIED         0
#define         S27_VLAN_WITHOUT            1
#define         S27_VLAN_WITH               2
#define     S27_LEARN_ONE_LOCK             	         (1 << 7)
#define     S27_PORT_LOCK_EN                         (1 << 6)
#define     S27_PORT_DROP_EN                         (1 << 5)
#define     S27_PORT_MODE_FWD                        0x4

#define S27_PORT_BASE_VLAN_REG(a)       ((0x100 * (a)) + 0x108)
#define     S27_VLAN_FORCE_PORT_VLAN_EN     (1 << 28)
#define     S27_VLAN_DEFAULT_CVID(a)        ((a) << 16)
#define     S27_VLAN_PORT_CLOSE_EN          (1 << 15)
#define     S27_VLAN_PORT_VLAN_PROP_EN      (1 << 14)
#define     S27_VLAN_PORT_TLS_MODE          (1 << 13)
#define     S27_VLAN_FORCE_DEF_VID_EN       (1 << 12)
#define     S27_VLAN_DEFAULT_SVID(a)        ((a) << 0)


#define S27_PORT_BASE_VLAN2_REG(a)      ((0x100 * (a)) + 0x10C)
#define     S27_VLAN2_MODE_DISABLE          (0 << 30)
#define     S27_VLAN2_MODE_FALLBACK         (1 << 30)
#define     S27_VLAN2_MODE_CHECK            (2 << 30)
#define     S27_VLAN2_MODE_SECURE           (3 << 30) 
#define     S27_VLAN2_CORE_PORT             (1 << 29)
#define     S27_VLAN2_ING_VLAN_MODE_ALL     (0 << 27)
#define     S27_VLAN2_ING_VLAN_MODE_WITH    (1 << 27)
#define     S27_VLAN2_ING_VLAN_MODE_WITHOUT (2 << 27)
#define     S27_VLAN2_VID_MEM(vlan_mask)    ((vlan_mask) << 16)
#define     S27_VLAN2_UNI_LEAKY_EN          (1 << 14)
#define     S27_VLAN2_MULTI_LEAKY_EN        (1 << 13)
#define S27_PORT_RATE_LIMIT_REG(a)      ((0x100 * (a)) + 0x110)
#define S27_PORT_PRI_CTRL_REG(a)        ((0x100 * (a)) + 0x114)
#define S27_PORT_STORM_CTRL_REG(a)      ((0x100 * (a)) + 0x118)
#define S27_PORT_QUEUE_CTRL_REG(a)      ((0x100 * (a)) + 0x11C)
#define S27_PORT_RATE_LIMIT_1_REG(a)    ((0x100 * (a)) + 0x120)
#define S27_PORT_RATE_LIMIT_2_REG(a)    ((0x100 * (a)) + 0x124)
#define S27_PORT_RATE_LIMIT_3_REG(a)    ((0x100 * (a)) + 0x128)
#define S27_PORT_ROBIN_REG(a)           ((0x100 * (a)) + 0x12C)

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_s27_lock(avmnet_module_t *this);
void avmnet_s27_unlock(avmnet_module_t *this);
int avmnet_s27_trylock(avmnet_module_t *this);
int avmnet_ar8326_status_poll(avmnet_module_t *this);
int avmnet_ar8326_setup_interrupt(avmnet_module_t *this, uint32_t on_off);
int avmnet_ar8326_setup(avmnet_module_t *this);
int avmnet_ar8326_init(avmnet_module_t *this);
int avmnet_ar8326_exit(avmnet_module_t *this);

int avmnet_ar8326_set_status(avmnet_module_t *this, avmnet_device_t *id, avmnet_linkstatus_t status);
unsigned int avmnet_s27_rd_phy(avmnet_module_t *this, unsigned int phy_addr, unsigned int reg_addr);
int avmnet_s27_wr_phy(avmnet_module_t *this, unsigned int phy_addr, unsigned int reg_addr, unsigned int write_data);
void avmnet_ar8326_status_changed(avmnet_module_t *this, avmnet_module_t *caller);

int avmnet_ar8326_poll_intern(avmnet_module_t *this);

int avmnet_ar8326_setup_wan_HW222(avmnet_module_t *this);
int avmnet_ar8326_setup_wan_HW219(avmnet_module_t *this);

int avmnet_ar8326_init_wan(avmnet_module_t *this);
int avmnet_ar8326_exit_wan(avmnet_module_t *this);
int avmnet_ar8326_poll_intern_wan(avmnet_module_t *this);
int avmnet_ar8326_poll_hbee_wan(avmnet_module_t *this);

#endif
