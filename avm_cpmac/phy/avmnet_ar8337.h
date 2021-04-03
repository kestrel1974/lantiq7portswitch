#if !defined(_AVM_NET_AR8337_H_)
#define _AVM_NET_AR8337_H_

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0)
#define soc_is_ar934x   is_ar934x
#define soc_is_qca955x  is_qca955x
#define soc_is_qca9531  is_qca9531
#define soc_is_qca956x  is_qca956x
#endif

struct aths17_context {
    avmnet_module_t         *this_module;
    struct task_struct      *kthread;
    wait_queue_head_t        event_wq;
    volatile unsigned long   pending_events;

    unsigned int irq;
    unsigned int reset;

    struct semaphore lock;      // mutex used by children for synchronisation among each other
    struct semaphore mdio_lock; // mutex used for protecting local MDIO access

    struct list_head list_free;
    struct list_head list_process;
    struct list_head list_active;
    spinlock_t lock_free;
    spinlock_t lock_process;
    spinlock_t lock_active;
#define AR8337_PORTS 6
#define AR8337_RMON_COUNTER 42
    u32 rmon_cache[AR8337_PORTS][AR8337_RMON_COUNTER];
};

extern struct avmnet_hw_rmon_counter *create_ar8337_rmon_cnt(avmnet_device_t *avm_dev);

/*------------------------------------------------------------------------------------------*\
 * S17 CSR Registers
\*------------------------------------------------------------------------------------------*/
#define S17_MASK_CTL_REG		        0x0000
#define     S17_MASK_CTRL_RESET             (1 << 31)
#define     S17_CHIPID_V1_0			        0x1201
#define     S17_CHIPID_V1_1			        0x1202


#define S17_P0_PADCTRL_REG0             0x0004
#define     S17_MAC0_MAC_MII_RXCLK_SEL	    (1 << 0)
#define     S17_MAC0_MAC_MII_TXCLK_SEL	    (1 << 1)
#define     S17_MAC0_MAC_MII_EN		        (1 << 2)
#define     S17_MAC0_MAC_GMII_RXCLK_SEL	    (1 << 4)
#define     S17_MAC0_MAC_GMII_TXCLK_SEL	    (1 << 5)
#define     S17_MAC0_MAC_GMII_EN		    (1 << 6)
#define     S17_MAC0_SGMII_EN		        (1 << 7)
#define     S17_MAC0_PHY_MII_RXCLK_SEL	    (1 << 8)
#define     S17_MAC0_PHY_MII_TXCLK_SEL	    (1 << 9)
#define     S17_MAC0_PHY_MII_EN		        (1 << 10)
#define     S17_MAC0_PHY_MII_PIPE_SEL	    (1 << 11)
#define     S17_MAC0_PHY_GMII_RXCLK_SEL	    (1 << 12)
#define     S17_MAC0_PHY_GMII_TXCLK_SEL	    (1 << 13)
#define     S17_MAC0_PHY_GMII_EN		    (1 << 14)
#define     S17_MAC0_RGMII_RXCLK_SHIFT	       20
#define     S17_MAC0_RGMII_RXCLK_MASK	    (3 << S17_MAC0_RGMII_RXCLK_SHIFT)   
#define     S17_MAC0_RGMII_TXCLK_SHIFT	       22
#define     S17_MAC0_RGMII_TXCLK_MASK	    (3 << S17_MAC0_RGMII_TXCLK_SHIFT)
#define     S17_MAC0_SGMII_DELAY		    (1 << 19)
#define     S17_MAC0_RGMII_RXCLK_DELAY	    (1 << 24)
#define     S17_MAC0_RGMII_TXCLK_DELAY	    (1 << 25)
#define     S17_MAC0_RGMII_EN		        (1 << 26)


#define S17_P5_PADCTRL_REG1             0x0008
#define     S17_MAC5_MAC_MII_RXCLK_SEL	    (1 << 0)
#define     S17_MAC5_MAC_MII_TXCLK_SEL	    (1 << 1)
#define     S17_MAC5_MAC_MII_EN		        (1 << 2)
#define     S17_MAC5_PHY_MII_RXCLK_SEL	    (1 << 8)
#define     S17_MAC5_PHY_MII_TXCLK_SEL	    (1 << 9)
#define     S17_MAC5_PHY_MII_EN		        (1 << 10)
#define     S17_MAC5_PHY_MII_PIPE_SEL	    (1 << 11)
#define     S17_MAC5_RGMII_RXCLK_SHIFT	       20
#define     S17_MAC5_RGMII_TXCLK_SHIFT	       22
#define     S17_MAC5_RGMII_RXCLK_DELAY	    (1 << 24)
#define     S17_MAC5_RGMII_TXCLK_DELAY	    (1 << 25)
#define     S17_MAC5_RGMII_EN		        (1 << 26)


#define S17_P6_PADCTRL_REG2             0x000C
#define     S17_MAC6_MAC_MII_RXCLK_SEL	    (1 << 0)
#define     S17_MAC6_MAC_MII_TXCLK_SEL	    (1 << 1)
#define     S17_MAC6_MAC_MII_EN		        (1 << 2)
#define     S17_MAC6_MAC_GMII_RXCLK_SEL	    (1 << 4)
#define     S17_MAC6_MAC_GMII_TXCLK_SEL	    (1 << 5)
#define     S17_MAC6_MAC_GMII_EN		    (1 << 6)
#define     S17_MAC6_SGMII_EN		        (1 << 7)
#define     S17_MAC6_PHY_MII_RXCLK_SEL	    (1 << 8)
#define     S17_MAC6_PHY_MII_TXCLK_SEL	    (1 << 9)
#define     S17_MAC6_PHY_MII_EN		        (1 << 10)
#define     S17_MAC6_PHY_MII_PIPE_SEL	    (1 << 11)
#define     S17_MAC6_PHY_GMII_RXCLK_SEL	    (1 << 12)
#define     S17_MAC6_PHY_GMII_TXCLK_SEL	    (1 << 13)
#define     S17_MAC6_PHY_GMII_EN		    (1 << 14)
#define     S17_PHY4_GMII_EN		        (1 << 16)
#define     S17_PHY4_RGMII_EN		        (1 << 17)
#define     S17_PHY4_MII_EN			        (1 << 18)
#define     S17_MAC6_RGMII_RXCLK_SHIFT	     20
#define     S17_MAC6_RGMII_TXCLK_SHIFT	     22
#define     S17_MAC6_RGMII_RXCLK_DELAY	    (1 << 24)
#define     S17_MAC6_RGMII_TXCLK_DELAY	    (1 << 25)
#define     S17_MAC6_RGMII_EN		        (1 << 26)

#define S17_PWS_REG                     0x0010

#define S17_GLOBAL_INT0_REG             0x0020
#define S17_GLOBAL_INT0_MASK_REG        0x0028
#define     S17_INT0_ARLDONE                (1 << 22)
#define     S17_INT0_VTDONE                 (1 << 20)
#define S17_GLOBAL_INT1_REG             0x0024
#define S17_GLOBAL_INT1_MASK_REG        0x002c

#define S17_MODULE_EN_REG               0x0030
#define     S17_MODULE_L3_EN		        (1 << 2)
#define     S17_MODULE_ACL_EN		        (1 << 1)
#define     S17_MODULE_MIB_EN		        (1 << 0)

#define S17_MIB_REG                     0x0034
#define     S17_MIB_FUNC_ALL		        (3 << 24)
#define     S17_MIB_CPU_KEEP		        (1 << 20)
#define     S17_MIB_BUSY			        (1 << 17)
#define     S17_MIB_AT_HALF_EN		        (1 << 16)
#define     S17_MIB_TIMER_DEFAULT		    0x100

#define S17_INTF_HIADDR_REG             0x0038
#define S17_MDIO_CTRL_REG               0x003c
#define S17_BIST_CTRL_REG               0x0040
#define S17_BIST_REC_REG                0x0044
#define S17_SERVICE_REG                 0x0048
#define     S17_SERVICE_STAG_MODE           (1<<17)
#define S17_LED_CTRL0_REG               0x0050
#define S17_LED_CTRL1_REG               0x0054
#define S17_LED_CTRL2_REG               0x0058
#define S17_LED_CTRL3_REG               0x005c
#define S17_MACADDR0_REG                0x0060
#define S17_MACADDR1_REG                0x0064
#define S17_MAX_FRAME_SIZE_REG          0x0078

#define S17_P0STATUS_REG                0x007c
#define S17_P1STATUS_REG                0x0080
#define S17_P2STATUS_REG                0x0084
#define S17_P3STATUS_REG                0x0088
#define S17_P4STATUS_REG                0x008c
#define S17_P5STATUS_REG                0x0090
#define S17_P6STATUS_REG                0x0094
#define S17_PORT_STATUS_REG(a)          (0x7c + (a * 4))
#define     S17_PS_SPEED_10 			        (0 << 0)
#define     S17_PS_SPEED_100 			        (1 << 0)
#define     S17_PS_SPEED_1000 			        (2 << 0)
#define     S17_PS_TXMAC_EN			            (1 << 2)
#define     S17_PS_RXMAC_EN			            (1 << 3)
#define     S17_PS_TX_FLOW_EN			        (1 << 4)
#define     S17_PS_RX_FLOW_EN			        (1 << 5)
#define     S17_PS_DUPLEX			            (1 << 6)
#define     S17_PS_DUPLEX_HALF			        (0 << 6)
#define     S17_PS_TXH_FLOW_EN		            (1 << 7)
#define     S17_PS_LINK_EN			            (1 << 9)
#define     S17_PS_FLOW_LINK_EN		            (1 << 12)

#define     S17_PS_PORT_STATUS_DEFAULT	(S17_PS_SPEED_1000 | \
                                        S17_PS_TXMAC_EN | S17_PS_RXMAC_EN | \
                                        S17_PS_TX_FLOW_EN | S17_PS_RX_FLOW_EN | S17_PS_TXH_FLOW_EN | \
                                        S17_PS_DUPLEX)

#define     S17_PORT_STATUS_AZ_DEFAULT  (S17_PS_SPEED_1000 | \
                                        S17_PS_TXMAC_EN | S17_PS_RXMAC_EN | \
                                        S17_PS_TX_FLOW_EN | S17_PS_RX_FLOW_EN | \
                                        S17_PS_DUPLEX)

#define     S17_PORT0_STATUS_DEFAULT    (S17_PS_SPEED_1000 | \
                                        S17_PS_TXMAC_EN | S17_PS_RXMAC_EN | \
                                        S17_PS_DUPLEX)

#define     S17_PORT6_STATUS_DEFAULT	(S17_PS_SPEED_1000 | \
                                        S17_PS_TXMAC_EN | S17_PS_RXMAC_EN | \
                                        S17_PS_TX_FLOW_EN |  S17_PS_RX_FLOW_EN | S17_PS_TX_HALF_FLOW_EN | \
                                        S17_PS_DUPLEX)

#define S17_HDRCTRL_REG                 0x0098
#define     S17_HDRLENGTH_SEL		        (1 << 16)
#define     S17_HDR_VALUE			        0xAAAA

#define S17_P0HDRCTRL_REG               0x009c
#define S17_P1HDRCTRL_REG               0x00A0
#define S17_P2HDRCTRL_REG               0x00a4
#define S17_P3HDRCTRL_REG               0x00a8
#define S17_P4HDRCTRL_REG               0x00ac
#define S17_P5HDRCTRL_REG               0x00b0
#define S17_P6HDRCTRL_REG               0x00b4
#define S17_PORT_HDRCTRL_REG(a)              (S17_P0HDRCTRL_REG + (a) * 4)

#define     S17_TXHDR_MODE_NO		        0
#define     S17_TXHDR_MODE_MGM		        1
#define     S17_TXHDR_MODE_ALL		        2
#define     S17_RXHDR_MODE_NO		        (0 << 2)
#define     S17_RXHDR_MODE_MGM		        (1 << 2)
#define     S17_RXHDR_MODE_ALL		        (2 << 2)

#define S17_SGMII_CTRL_REG              0x00e0
#define S17_PWR_SEL_REG                 0x00e4
#define     S17_PWR_SEL_RGMII0_1_8V        (1 << 19)
#define     S17_PWR_SEL_RGMII1_1_8V        (1 << 18)

#define S17_EEE_CTRL_REG		        0x0100
#define     S17_LPI_DISABLE_P1		     (1 << 4)
#define     S17_LPI_DISABLE_P2		     (1 << 6)
#define     S17_LPI_DISABLE_P3		     (1 << 8)
#define     S17_LPI_DISABLE_P4		     (1 << 10)
#define     S17_LPI_DISABLE_P5		     (1 << 12)
#define     S17_LPI_DISABLE_ALL		     0x1550

#define BC_MASK(h, l) (((1ULL << ((h) - (l))) - 1) << (l))
#define BC_ALIGN(v, s, d) (((v) >> (s)) << (d))

/*
 * Think memcpy for bit arrays with dst_shift/src_shift being the bit
 * offsets relative to dst/src.
 *
 * TODO put me somewhere more appropriate (but don't break inlining!)
 */
static inline void avmnet_bitcpy(void *dst,
                                 const void *src,
                                 unsigned int dst_shift,
                                 unsigned int src_shift,
                                 unsigned int size) {
    unsigned long wordsize = sizeof(unsigned long) * 8, skew, whole_words,
                  trailing_size;
    struct boundary {
        unsigned long upper_mask;
        unsigned long upper_size;
        unsigned long lower_mask;
        unsigned long lower_size;
    } d, s;
    unsigned long *_dst = ((unsigned long *)dst) + (dst_shift / wordsize);
    const unsigned long *_src = ((unsigned long *)src) + (src_shift / wordsize);

    s = (struct boundary){.lower_size = (src_shift % wordsize),
                          .lower_mask = (1 << (src_shift % wordsize)) - 1,
                          .upper_size = wordsize - (src_shift % wordsize),
                          .upper_mask = ~((1 << (src_shift % wordsize)) - 1) };
    d = (struct boundary){.lower_size = (dst_shift % wordsize),
                          .lower_mask = (1 << (dst_shift % wordsize)) - 1,
                          .upper_size = wordsize - (dst_shift % wordsize),
                          .upper_mask = ~((1 << (dst_shift % wordsize)) - 1) };

    /* respective to the destination word boundaries */
    whole_words = (size - d.upper_size) / wordsize;
    trailing_size = (size - d.upper_size) % wordsize;

    skew = (wordsize + s.lower_size - d.lower_size) % wordsize;

    if(d.lower_size + size <= wordsize) {
        *_dst &= ~(((1 << size) - 1) << d.lower_size);
        if(s.upper_size < size) {
            *_dst |=
              BC_ALIGN(*_src++ & s.upper_mask, s.lower_size, d.lower_size);
            *_dst |= BC_ALIGN(*_src & BC_MASK(size - s.upper_size, 0), 0,
                              d.lower_size + s.upper_size);
        } else {
            *_dst |=
              BC_ALIGN(*_src & BC_MASK(s.lower_size + size, s.lower_size),
                       s.lower_size, d.lower_size);
        }
    } else {
        /* bring write position to the destination word boundary */
        *_dst &= d.lower_mask;
        if(s.lower_size + d.upper_size < wordsize) {
            *_dst++ |= BC_ALIGN(
              *_src & BC_MASK(s.lower_size + d.upper_size, s.lower_size),
              s.lower_size, d.lower_size);
        } else {
            *_dst |= BC_ALIGN(*_src++ & BC_MASK(wordsize, s.lower_size),
                              s.lower_size, d.lower_size);
            *_dst++ |= BC_ALIGN(*_src & BC_MASK(skew, 0), 0, (wordsize - skew));
        }

        while(whole_words--) {
            *_dst = BC_ALIGN(*_src++ & BC_MASK(wordsize, skew), skew, 0);
            *_dst++ |= BC_ALIGN(*_src & BC_MASK(skew, 0), 0, wordsize - skew);
        }

        if(trailing_size) {
            *_dst &= ~((1 << trailing_size) - 1);
            *_dst |= BC_ALIGN(*_src++ & BC_MASK(wordsize, skew), skew, 0);
            if((wordsize - (skew)) <= trailing_size) {
                unsigned long remainder = trailing_size - (wordsize - (skew));
                *_dst |=
                  BC_ALIGN(*_src & BC_MASK(remainder, 0), 0, wordsize - (skew));
            }
        }
    }
}
#undef BC_MASK
#undef BC_ALIGN

/*
 * ABOUT ALL THAT XARGS STUFF
 *
 * Descriptions of the hardware tables are kept as arguments to an undefined
 * macro 'X'. A table entry consists of a list of calls to macro 'X'
 * specifying a field. They are dubbed 'XARGS' for obvious reasons.
 * These lists can be used in several places to define static code for e.g.
 * structs, register access functions or debug output by defining X within the
 * scope of the desired definition.
 */

/* possible definitions of X intended as a more portable bitfield alternative.
 * The encoded bit array is of an "unsigned long" quantity and therefore
 * compatible to bitops.h */
#define XARGS_TO_MEMBERS(member, type, shift, width) typeof(type) member __attribute__ ((aligned (4)));
#if defined(__LITTLE_ENDIAN)
#define XARGS_ENCODE(m, t, s, w) avmnet_bitcpy(dst, &src->m, (s), 0, (w));
#define XARGS_DECODE(m, t, s, w) avmnet_bitcpy(&dst->m, src, 0, (s), (w));
#elif defined(__BIG_ENDIAN)
#define XARGS_ENCODE(m, t, s, w)                                               \
    {                                                                          \
        unsigned int wordsize = sizeof(unsigned long) * 8;                     \
        unsigned int whole_words = ((w) / wordsize);                           \
        unsigned int trailing_bits = ((w) % wordsize);                         \
        unsigned int trailing_bytes = DIV_ROUND_UP(trailing_bits, 8);          \
        unsigned long *src_ = (void *)(&src->m);                               \
        avmnet_bitcpy(dst, src_, (s), 0, whole_words *wordsize);               \
        avmnet_bitcpy(dst, src_ + whole_words, (s) + (whole_words * wordsize), \
                      wordsize - (trailing_bytes * 8), trailing_bits);         \
    }
/* TODO decode für big endian testen */
#define XARGS_DECODE(m, t, s, w)                                      \
    {                                                                 \
        unsigned int wordsize = sizeof(unsigned long) * 8;            \
        unsigned int whole_words = ((w) / wordsize);                  \
        unsigned int trailing_bits = ((w) % wordsize);                \
        unsigned int trailing_bytes = DIV_ROUND_UP(trailing_bits, 8); \
        unsigned long *dst_ = (void *)(&dst->m);                      \
        avmnet_bitcpy(dst_, src, 0, (s), whole_words *wordsize);      \
        avmnet_bitcpy(dst_ + whole_words, src,                        \
                      wordsize - (trailing_bytes * 8),                \
                      (s) + (whole_words * wordsize), trailing_bits); \
    }
#else
#error "Byte-order not defined"
#endif

#define ACL_COMMON_MASK_XARGS \
X(rule_type,      uint8_t,   128,  3)   \
X(rule_valid,     uint8_t,   134,  2)

#define ACL_MAC_PATTERN_XARGS                            \
X(da,               typeof(uint8_t)[6],  0,    48)  \
X(sa,               typeof(uint8_t)[6],  48,   48)  \
X(vid,              uint16_t,            96,   12)  \
X(vlan_dei,         uint8_t,             108,  1)   \
X(vlan_prio,        uint8_t,             109,  3)   \
X(type,             uint16_t,            112,  16)  \
X(source_port,      uint8_t,             128,  7)   \
X(rule_res_inv_en,  uint8_t,             135,  1)

#define ACL_MAC_MASK_XARGS \
X(da,                typeof(uint8_t)[6],  0,    48)  \
X(sa,                typeof(uint8_t)[6],  48,   48)  \
X(vid_mask,          uint16_t,            96,   12)  \
X(dei_mask,          uint8_t,             108,  1)   \
X(vlan_prio_mask,    uint8_t,             109,  3)   \
X(type_mask,         uint16_t,            112,  16)  \
X(rule_type,         uint8_t,             128,  3)   \
X(vid_mask_type,     uint8_t,             131,  1)   \
X(frame_w_tag,       uint8_t,             132,  1)   \
X(frame_w_tag_mask,  uint8_t,             133,  1)   \
X(rule_valid,        uint8_t,             134,  2)

#define ACL_IPV4_PATTERN_XARGS \
X(dip,              uint32_t,  0,    32)  \
X(sip,              uint32_t,  32,   32)  \
X(ip_protocol,       uint8_t,   64,   8)   \
X(dscp,             uint8_t,   72,   8)   \
X(dport,            uint16_t,  80,   16)  \
X(sport,            uint16_t,  96,   16)  \
X(sport_type,       uint8_t,   116,  1)   \
X(ripv1,            uint8_t,   117,  1)   \
X(dhcpv4,           uint8_t,   118,  1)   \
X(tcp_flags,        uint8_t,   120,  6)   \
X(source_port,      uint8_t,   128,  7)   \
X(rule_res_inv_en,  uint8_t,   135,  1)

#define ACL_IPV4_MASK_XARGS \
X(dip,            uint32_t,  0,    32)  \
X(sip,            uint32_t,  32,   32)  \
X(ip_protocol,     uint8_t,   64,   8)   \
X(dscp,           uint8_t,   72,   8)   \
X(dport,          uint16_t,  80,   16)  \
X(sport,          uint16_t,  96,   16)  \
X(sport_mask_en,  uint8_t,   112,  1)   \
X(dport_mask_en,  uint8_t,   113,  1)   \
X(ripv1,          uint8_t,   117,  1)   \
X(dhcpv4,         uint8_t,   118,  1)   \
X(tcp_flags,      uint8_t,   120,  6)   \
X(rule_type,      uint8_t,   128,  3)   \
X(rule_valid,     uint8_t,   134,  2)

enum acl_action_l3_mode {
    ACL_ACTION_L3_MODE_NONE = 0,
    ACL_ACTION_L3_MODE_SNAT = 1,
    ACL_ACTION_L3_MODE_DNAT = 2,
};

#define ACL_ACTION_XARGS \
X(stag_vid,              uint16_t,                0,   12)  \
X(stag_dei,              uint8_t,                 12,  1)   \
X(stag_prio,             uint8_t,                 13,  3)   \
X(ctag_vid,              uint16_t,                16,  12)  \
X(ctag_dei,              uint8_t,                 28,  1)   \
X(ctag_prio,             uint8_t,                 29,  3)   \
X(dscp,                  uint8_t,                 32,  6)   \
X(dscp_remap_en,         uint8_t,                 38,  1)   \
X(stag_pri_remap_en,     uint8_t,                 39,  1)   \
X(stag_dei_change_en,    uint8_t,                 40,  1)   \
X(ctag_pri_remap_en,     uint8_t,                 41,  1)   \
X(ctag_dei_change_en,    uint8_t,                 42,  1)   \
X(trans_stag_change_en,  uint8_t,                 43,  1)   \
X(trans_ctag_change_en,  uint8_t,                 44,  1)   \
X(lookup_vid_change_en,  uint8_t,                 45,  1)   \
X(force_l3_mode,         enum acl_action_l3_mode, 46,  2)   \
X(arp_index_over_en,     uint8_t,                 48,  1)   \
X(arp_index,             uint8_t,                 49,  7)   \
X(arp_wcmp,              uint8_t,                 56,  1)   \
X(enqueue_pri,           uint8_t,                 57,  3)   \
X(enqueue_pri_over_en,   uint8_t,                 60,  1)   \
X(des_port,              uint8_t,                 61,  7)   \
X(des_port_over_en,      uint8_t,                 68,  1)   \
X(mirror_en,             uint8_t,                 69,  1)   \
X(dp_act,                uint8_t,                 70,  3)   \
X(rate_sel,              uint8_t,                 73,  5)   \
X(rate_en,               uint8_t,                 78,  1)   \
X(eg_trnas_bypass,       uint8_t,                 79,  1)   \
X(match_int_en,          uint8_t,                 80,  1)

#define X XARGS_TO_MEMBERS
/* ACL table */
struct acl_pattern_mac {
    ACL_MAC_PATTERN_XARGS
};
struct acl_mask_mac {
    ACL_MAC_MASK_XARGS
};
struct acl_pattern_ipv4 {
    ACL_IPV4_PATTERN_XARGS
};
struct acl_mask_ipv4 {
    ACL_IPV4_MASK_XARGS
};
struct acl_action {
    ACL_ACTION_XARGS
};
struct acl_mask_common {
    ACL_COMMON_MASK_XARGS
};
#undef X

enum acl_state {
    acl_state_free,
    acl_state_add,
    acl_state_active,
    acl_state_remove,
    acl_state_erase
};

struct acl_rule {
    struct list_head list;
    volatile enum acl_state state;
    uint8_t hw_index;
    enum {
        ACL_RULE_MAC,
        ACL_RULE_IPV4,
    } type;
    union {
        struct acl_pattern_mac mac;
        struct acl_pattern_ipv4 ipv4;
    } pattern;
    union {
        struct acl_mask_mac mac;
        struct acl_mask_ipv4 ipv4;
    } mask;
    struct acl_action action;
    uint64_t byte_count;
};

struct acl_work {
    struct work_struct work;
    struct aths17_context *ctx;
    struct acl_rule *acl_rule;
};

#define AVMNET_AR8337_ACL_RULES         S17_NUMBER_OF_ACL_RULES

#define S17_NUMBER_OF_ACL_RULES         96
#define S17_ACL_FUNC0_REG               0x0400
#define     S17_ACL_FUNC0_REG_ACL_FUNC_INDEX(a)          (((a) & 0x7f) << 0)
#define     S17_ACL_FUNC0_REG_ACL_RULE_SELECT(a)         (((a) & 0x3) << 8)
#define         S17_ACL_FUNC0_REG_ACL_RULE_SELECT_RULE   0x0
#define         S17_ACL_FUNC0_REG_ACL_RULE_SELECT_MASK   0x1
#define         S17_ACL_FUNC0_REG_ACL_RULE_SELECT_RESULT 0x2
#define     S17_ACL_FUNC0_REG_ACL_FUNC(a)                (((a) & 0x1) << 10)
#define         S17_ACL_FUNC0_REG_ACL_FUNC_WRITE         0x0
#define         S17_ACL_FUNC0_REG_ACL_FUNC_READ          0x1
#define     S17_ACL_FUNC0_REG_ACL_BUSY                   (0x1 << 31)
#define S17_ACL_FUNC1_REG               0x0404
#define S17_ACL_FUNC2_REG               0x0408
#define S17_ACL_FUNC3_REG               0x040c
#define S17_ACL_FUNC4_REG               0x0410
#define S17_ACL_FUNC5_REG               0x0414
#define S17_PRIVATE_IP_REG              0x0418
#define S17_ACL_RULE_TYPE_MAC		0x1
#define S17_ACL_RULE_TYPE_IPV4		0x2
#define S17_ACL_RULE_TYPE_IPV6_R1	0x3
#define S17_ACL_RULE_TYPE_IPV6_R2	0x4
#define S17_ACL_RULE_TYPE_IPV6_R3	0x5
#define S17_ACL_RULE_TYPE_WINDOW	0x6
#define S17_ACL_RULE_TYPE_MAC_ENH	0x7
#define S17_ACL_RULE_VALID_S		0x0
#define S17_ACL_RULE_VALID_C		0x1
#define S17_ACL_RULE_VALID_E		0x2
#define S17_ACL_RULE_VALID_SE		0x3

#define S17_P0VLAN_CTRL0_REG            0x0420
#define S17_P0VLAN_CTRL1_REG            0x0424
#define S17_P1VLAN_CTRL0_REG            0x0428
#define S17_P1VLAN_CTRL1_REG            0x042c
#define S17_P2VLAN_CTRL0_REG            0x0430
#define S17_P2VLAN_CTRL1_REG            0x0434
#define S17_P3VLAN_CTRL0_REG            0x0438
#define S17_P3VLAN_CTRL1_REG            0x043c
#define S17_P4VLAN_CTRL0_REG            0x0440
#define S17_P4VLAN_CTRL1_REG            0x0444
#define S17_P5VLAN_CTRL0_REG            0x0448
#define S17_P5VLAN_CTRL1_REG            0x044c
#define S17_P6VLAN_CTRL0_REG            0x0450
#define S17_P6VLAN_CTRL1_REG            0x0454
#define S17_PxVLAN_CTRL0_REG(a)         (S17_P0VLAN_CTRL0_REG + ((a) * 8))

#define S17_PxVLAN_CTRL1_REG(a)         (S17_P0VLAN_CTRL1_REG + ((a) * 8))
#define     S17_PxVLAN_EG_VLAN_MODE(a)      ((a) << 12)
#define     S17_EG_VLAN_MODE_UNMODIFIED     S17_PxVLAN_EG_VLAN_MODE(0)
#define     S17_EG_VLAN_MODE_WITHOUT        S17_PxVLAN_EG_VLAN_MODE(1)
#define     S17_EG_VLAN_MODE_WITH           S17_PxVLAN_EG_VLAN_MODE(2)
#define     S17_EG_VLAN_MODE_UNTOUCHED      S17_PxVLAN_EG_VLAN_MODE(3)
#define     S17_PxVLAN_CORE_PORT            (1<<9)
#define     S17_PxVLAN_FORCE_DEFAULT_VID_EN (1<<8)
#define     S17_PxVLAN_PORT_TLS_MODE        (1<<7)
#define     S17_PxVLAN_PROP_EN              (1<<6)

/* Table Lookup Registers */
#define S17_ATU_DATA0_REG               0x0600
#define S17_ATU_DATA1_REG               0x0604
#define S17_ATU_DATA2_REG               0x0608
#define S17_ATU_FUNC_REG                0x060C
#define     S17_ATU_FUNC_BUSY               (1<<31)
#define     S17_ATU_FUNC_TRUNC_PORT(a)      ((a)<<22)
#define     S17_ATU_FUNC_ATU_INDEX(a)       ((a)<<16)          
#define     S17_ATU_FUNC_VID_EN             (1<<15)
#define     S17_ATU_FUNC_PORT_EN            (1<<14)
#define     S17_ATU_FUNC_MULTI_EN           (1<<13)
#define     S17_ATU_FUNC_FULL_VIO           (1<<12)
#define     S17_ATU_FUNC_PORT_NUM(a)        ((a)<<8)
#define     S17_ATU_FUNC_TYPE               (1<<5)
#define     S17_ATU_FUNC_FLUSH_STATIC       (1<<4)
#define     S17_ATU_FUNC_FLUSH_ALL          1
#define     S17_ATU_FUNC_LOAD               2
#define     S17_ATU_FUNC_PURGE              3
#define     S17_ATU_FUNC_FLUSH_UNLOCKED     4
#define     S17_ATU_FUNC_FLUSH_ONE          5
#define     S17_ATU_FUNC_GET_NEXT           6
#define     S17_ATU_FUNC_SEARCH_MAC         7
#define     S17_ATU_FUNC_CHANGE_TRUNK       8

#define S17_VTU_FUNC0_REG               0x0610
#define     S17_VTU_FUNC0_VALID                 (1 << 20)
#define     S17_VTU_FUNC0_IVL_EN                (1 << 19)
#define     S17_VTU_FUNC0_LEARN_LOOKDIS         (1 << 18)
#define     S17_VTU_FUNC0_EG_VLANMODE_PORT0(a)  ((a) << 4)
#define     S17_VTU_FUNC0_EG_VLANMODE_PORT1(a)  ((a) << 6)
#define     S17_VTU_FUNC0_EG_VLANMODE_PORT2(a)  ((a) << 8)
#define     S17_VTU_FUNC0_EG_VLANMODE_PORT3(a)  ((a) << 10)
#define     S17_VTU_FUNC0_EG_VLANMODE_PORT4(a)  ((a) << 12)
#define     S17_VTU_FUNC0_EG_VLANMODE_PORT5(a)  ((a) << 14)
#define     S17_VTU_FUNC0_EG_VLANMODE_PORT6(a)  ((a) << 16)
#define     VLANMODE_UNMODIFIED 0
#define     VLANMODE_UNTAGGED   1
#define     VLANMODE_TAGGED     2
#define     VLANMODE_NOMEMBER   3

#define S17_VTU_FUNC1_REG               0x0614
#define     S17_VTU_FUNC1_VTBUSY             (1 << 31)
#define     S17_VTU_FUNC1_VID(a)           ((a) << 16)               
#define     S17_VTU_FUNC1_PORTNR(a)        ((a) << 8)               
#define     S17_VTU_FUNC1_VT_FULL_VIO        (1 << 4)
#define     S17_VTU_FUNC1_VT_FUNC_NOP        0
#define     S17_VTU_FUNC1_VT_FUNC_FLUSH      1
#define     S17_VTU_FUNC1_VT_FUNC_LOAD       2
#define     S17_VTU_FUNC1_VT_FUNC_PURGE      3
#define     S17_VTU_FUNC1_VT_FUNC_REMOVE     4
#define     S17_VTU_FUNC1_VT_FUNC_GET_NEXT   5
#define     S17_VTU_FUNC1_VT_FUNC_READ       6

#define S17_ARL_CTRL_REG                0x0618
#define S17_GLOFW_CTRL0_REG             0x0620
#define     S17_CPU_PORT_EN			        (1 << 10)
#define     S17_PPPOE_REDIR_EN		        (1 << 8)
#define     S17_MIRROR_PORT_SHIFT		        4
#define     S17_IGMP_COPY_EN		        (1 << 3)
#define     S17_RIP_COPY_EN			        (1 << 2)
#define     S17_EAPOL_REDIR_EN		        (1 << 0)

#define S17_GLOFW_CTRL1_REG             0x0624
#define     S17_IGMP_JOIN_LEAVE_DP_SHIFT	24
#define     S17_BROAD_DP_SHIFT		        16
#define     S17_MULTI_FLOOD_DP_SHIFT	    8
#define     S17_UNI_FLOOD_DP_SHIFT		    0

#if defined(HYBRID_SWITCH_PORT6_USED)
#define     S17_IGMP_JOIN_LEAVE_DPALL	(0x7f << S17_IGMP_JOIN_LEAVE_DP_SHIFT)
#define     S17_BROAD_DPALL			    (0x7f << S17_BROAD_DP_SHIFT)
#define     S17_MULTI_FLOOD_DPALL		(0x7f << S17_MULTI_FLOOD_DP_SHIFT)
#define     S17_UNI_FLOOD_DPALL		    (0x7f << S17_UNI_FLOOD_DP_SHIFT)
#else
#define     S17_IGMP_JOIN_LEAVE_DPALL	(0x3f << S17_IGMP_JOIN_LEAVE_DP_SHIFT)
#define     S17_BROAD_DPALL			    (0x3f << S17_BROAD_DP_SHIFT)
#define     S17_MULTI_FLOOD_DPALL		(0x3f << S17_MULTI_FLOOD_DP_SHIFT)
#define     S17_UNI_FLOOD_DPALL		    (0x3f << S17_UNI_FLOOD_DP_SHIFT)
#endif

#define S17_GLOLEARN_LIMIT_REG          0x0628
#define S17_TOS_PRIMAP_REG0             0x0630
#define S17_TOS_PRIMAP_REG1             0x0634
#define S17_TOS_PRIMAP_REG2             0x0638
#define S17_TOS_PRIMAP_REG3             0x063c
#define S17_TOS_PRIMAP_REG4             0x0640
#define S17_TOS_PRIMAP_REG5             0x0644
#define S17_TOS_PRIMAP_REG6             0x0648
#define S17_TOS_PRIMAP_REG7             0x064c
#define S17_VLAN_PRIMAP_REG0            0x0650
#define S17_LOOP_CHECK_REG              0x0654

#define S17_P0LOOKUP_CTRL_REG           0x0660
#define S17_P0PRI_CTRL_REG              0x0664
#define S17_P0LEARN_LMT_REG             0x0668
#define S17_P1LOOKUP_CTRL_REG           0x066c
#define S17_P1PRI_CTRL_REG              0x0670
#define S17_P1LEARN_LMT_REG             0x0674
#define S17_P2LOOKUP_CTRL_REG           0x0678
#define S17_P2PRI_CTRL_REG              0x067c
#define S17_P2LEARN_LMT_REG             0x0680
#define S17_P3LOOKUP_CTRL_REG           0x0684
#define S17_P3PRI_CTRL_REG              0x0688
#define S17_P3LEARN_LMT_REG             0x068c
#define S17_P4LOOKUP_CTRL_REG           0x0690
#define S17_P4PRI_CTRL_REG              0x0694
#define S17_P4LEARN_LMT_REG             0x0698
#define S17_P5LOOKUP_CTRL_REG           0x069c
#define S17_P5PRI_CTRL_REG              0x06a0
#define S17_P5LEARN_LMT_REG             0x06a4
#define S17_P6LOOKUP_CTRL_REG           0x06a8
#define S17_P6PRI_CTRL_REG              0x06ac
#define S17_P6LEARN_LMT_REG             0x06b0
#define S17_PxLOOKUP_CTRL_REG(a)            (S17_P0LOOKUP_CTRL_REG + ((a) * 0xc))
#define     S17_LOOKUP_CTRL_LEARN_EN_0          (1 << 20)
#define     S17_LOOKUP_CTRL_PORTSTATE_DISABLE   (0 << 16)
#define     S17_LOOKUP_CTRL_PORTSTATE_BLOCK     (1 << 16)
#define     S17_LOOKUP_CTRL_PORTSTATE_LISTEN    (2 << 16)
#define     S17_LOOKUP_CTRL_PORTSTATE_LEARN     (3 << 16)
#define     S17_LOOKUP_CTRL_PORTSTATE_FORWARD   (4 << 16)
#define     S17_LOOKUP_CTRL_PORT_VLAN_EN        (1 << 10)
#define     S17_LOOKUP_CTRL_VLAN_MODE_DISABLE   (0 <<  8)
#define     S17_LOOKUP_CTRL_VLAN_MODE_FALLBACK  (1 <<  8)
#define     S17_LOOKUP_CTRL_VLAN_MODE_CHECK     (2 <<  8)
#define     S17_LOOKUP_CTRL_VLAN_MODE_SECURE    (3 <<  8)

#define S17_PxPRI_CTRL_REG(a)               (S17_P0PRI_CTRL_REG + ((a) * 0xc))
#define S17_PxLEARN_LMT_REG(a)              (S17_P0LEARN_LMT_REG + ((a) * 0xc))

#define S17_GLO_TRUNK_CTRL0_REG         0x0700
#define S17_GLO_TRUNK_CTRL1_REG         0x0704
#define S17_GLO_TRUNK_CTRL2_REG         0x0708

/* Queue Management Registers */
#define S17_QM_GLOBAL_FLOW_THD_REG      0x0800
#define S17_QM_CTRL_REG 		        0x0808
#define     S17_QM_CTRL_RATE_DROP_EN        (1<<7)
#define     S17_QM_CTRL_FLOW_DROP_EN        (1<<6)

#define S17_PORT0_HOL_CTRL0		        0x0970
#define S17_PORT0_HOL_CTRL1		        0x0974
#define S17_PORT1_HOL_CTRL0		        0x0978
#define S17_PORT1_HOL_CTRL1		        0x097c
#define S17_PORT2_HOL_CTRL0		        0x0980
#define S17_PORT2_HOL_CTRL1		        0x0984
#define S17_PORT3_HOL_CTRL0		        0x0988
#define S17_PORT3_HOL_CTRL1		        0x098c
#define S17_PORT4_HOL_CTRL0		        0x0990
#define S17_PORT4_HOL_CTRL1		        0x0994
#define S17_PORT5_HOL_CTRL0		        0x0998
#define S17_PORT5_HOL_CTRL1		        0x099c
#define S17_PORT6_HOL_CTRL0		        0x09a0
#define S17_PORT6_HOL_CTRL1		        0x09a4
#define     S17_HOL_CTRL0_LAN		        0x2a008888 /* egress priority 8, eg_portq = 0x2a */
#define     S17_HOL_CTRL0_WAN		        0x2a666666 /* egress priority 6, eg_portq = 0x2a */
#define     S17_HOL_CTRL1_DEFAULT		    0xc6	   /* enable HOL control */
#define S17_Px_HOL_CTRL0(a)                 (S17_PORT0_HOL_CTRL0 + ((a) * 8))
#define S17_Px_HOL_CTRL1(a)                 (S17_PORT0_HOL_CTRL1 + ((a) * 8))

/* Port flow control registers */
#define S17_P0_FLCTL_THD_REG            0x09b0
#define S17_P1_FLCTL_THD_REG            0x09b4
#define S17_P2_FLCTL_THD_REG            0x09b8
#define S17_P3_FLCTL_THD_REG            0x09bc
#define S17_P4_FLCTL_THD_REG            0x09c0
#define S17_P5_FLCTL_THD_REG            0x09c4
#define S17_P6_FLCTL_THD_REG            0x09c8
#define S17_Px_FLCTL_THD(a)             (S17_P0_FLCTL_THD_REG + ((a) * 4))

#define AVMNET_AR8337_MAX_ACL_COUNTER   32
#define S17_ACL_POLICY_MODE_REG         0x09F0
#define S17_ACL_COUNTER_MODE_REG        0x09F4
#define S17_ACL_CNT_RESET_REG           0x09F8

#define S17_ACL_COUNTER0_REG            0x1C000
#define S17_ACL_COUNTER1_REG            0x1C004
#define S17_ACL_COUNTER0(a)             (S17_ACL_COUNTER0_REG + ((a) * 8))
#define S17_ACL_COUNTER1(a)             (S17_ACL_COUNTER1_REG + ((a) * 8))

/* Packet Edit registers */
#define S17_PKT_EDIT_CTRL		        0x0c00
#define S17_P0Q_REMAP_REG0		        0x0c40
#define S17_P0Q_REMAP_REG1		        0x0c44
#define S17_P1Q_REMAP_REG0		        0x0c48
#define S17_P2Q_REMAP_REG0		        0x0c4c
#define S17_P3Q_REMAP_REG0		        0x0c50
#define S17_P4Q_REMAP_REG0		        0x0c54
#define S17_P5Q_REMAP_REG0		        0x0c58
#define S17_P5Q_REMAP_REG1		        0x0c5c
#define S17_P6Q_REMAP_REG0		        0x0c60
#define S17_P6Q_REMAP_REG1		        0x0c64
#define S17_ROUTER_VID0			        0x0c70
#define S17_ROUTER_VID1			        0x0c74
#define S17_ROUTER_VID2			        0x0c78
#define S17_ROUTER_VID3			        0x0c7c
#define S17_ROUTER_EG_VLAN_MODE		    0x0c80
#define     S17_ROUTER_EG_UNMOD		        0x0	/* unmodified */
#define     S17_ROUTER_EG_WOVLAN		    0x1	/* without VLAN */
#define     S17_ROUTER_EG_WVLAN		        0x2	/* with VLAN */
#define     S17_ROUTER_EG_UNTOUCH		    0x3	/* untouched */
#define     S17_ROUTER_EG_MODE_DEFAULT	    0x01111111 /* all ports without VLAN */

/* L3 Registers */
#define S17_HROUTER_CTRL_REG            0x0e00
#define S17_HROUTER_PBCTRL0_REG         0x0e04
#define S17_HROUTER_PBCTRL1_REG         0x0e08
#define S17_HROUTER_PBCTRL2_REG         0x0e0c
#define S17_WCMP_HASH_TABLE0_REG        0x0e10
#define S17_WCMP_HASH_TABLE1_REG        0x0e14
#define S17_WCMP_HASH_TABLE2_REG        0x0e18
#define S17_WCMP_HASH_TABLE3_REG        0x0e1c
#define S17_WCMP_NHOP_TABLE0_REG        0x0e20
#define S17_WCMP_NHOP_TABLE1_REG        0x0e24
#define S17_WCMP_NHOP_TABLE2_REG        0x0e28
#define S17_WCMP_NHOP_TABLE3_REG        0x0e2c
#define S17_ARP_ENTRY_CTRL_REG          0x0e30
#define S17_ARP_USECNT_REG              0x0e34
#define S17_HNAT_CTRL_REG               0x0e38
#define S17_NAPT_ENTRY_CTRL0_REG        0x0e3c
#define S17_NAPT_ENTRY_CTRL1_REG        0x0e40
#define S17_NAPT_USECNT_REG             0x0e44
#define S17_ENTRY_EDIT_DATA0_REG        0x0e48
#define S17_ENTRY_EDIT_DATA1_REG        0x0e4c
#define S17_ENTRY_EDIT_DATA2_REG        0x0e50
#define S17_ENTRY_EDIT_DATA3_REG        0x0e54
#define S17_ENTRY_EDIT_CTRL_REG         0x0e58
#define S17_HNAT_PRIVATE_IP_REG         0x0e5c

/* MIB counters */
#define S17_MIB_PORT0			        0x1000
#define S17_MIB_PORT1			        0x1100
#define S17_MIB_PORT2			        0x1200
#define S17_MIB_PORT3			        0x1300
#define S17_MIB_PORT4			        0x1400
#define S17_MIB_PORT5			        0x1500
#define S17_MIB_PORT6			        0x1600
#define S17_MIB_PORT(a)                 (S17_MIB_PORT0 + ((a) * 100))

#define S17_MIB_RXBROAD			            0x0
#define S17_MIB_RXPAUSE			            0x4
#define S17_MIB_RXMULTI			            0x8
#define S17_MIB_RXFCSERR		            0xC
#define S17_MIB_RXALIGNERR		            0x10
#define S17_MIB_RXUNDERSIZE		            0x14
#define S17_MIB_RXFRAG			            0x18
#define S17_MIB_RX64B			            0x1C
#define S17_MIB_RX128B			            0x20
#define S17_MIB_RX256B			            0x24
#define S17_MIB_RX512B			            0x28
#define S17_MIB_RX1024B			            0x2C
#define S17_MIB_RX1518B			            0x30
#define S17_MIB_RXMAXB			            0x34
#define S17_MIB_RXTOOLONG		            0x38
#define S17_MIB_RXBYTE1			            0x3C
#define S17_MIB_RXBYTE2			            0x40
#define S17_MIB_RXOVERFLOW		            0x4C
#define S17_MIB_FILTERED		            0x50
#define S17_MIB_TXBROAD			            0x54
#define S17_MIB_TXPAUSE			            0x58
#define S17_MIB_TXMULTI			            0x5C
#define S17_MIB_TXUNDERRUN		            0x60
#define S17_MIB_TX64B			            0x64
#define S17_MIB_TX128B			            0x68
#define S17_MIB_TX256B			            0x6c
#define S17_MIB_TX512B			            0x70
#define S17_MIB_TX1024B			            0x74
#define S17_MIB_TX1518B			            0x78
#define S17_MIB_TXMAXB			            0x7C
#define S17_MIB_TXOVERSIZE		            0x80
#define S17_MIB_TXBYTE1			            0x84
#define S17_MIB_TXBYTE2			            0x88
#define S17_MIB_TXCOL			            0x8C
#define S17_MIB_TXABORTCOL		            0x90
#define S17_MIB_TXMULTICOL		            0x94
#define S17_MIB_TXSINGLECOL		            0x98
#define S17_MIB_TXEXCDEFER		            0x9C
#define S17_MIB_TXDEFER			            0xA0
#define S17_MIB_TXLATECOL		            0xA4

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_s17_lock(avmnet_module_t *this);
void avmnet_s17_unlock(avmnet_module_t *this);
int avmnet_s17_trylock(avmnet_module_t *this);
int avmnet_ar8337_status_poll(avmnet_module_t *this);
int avmnet_ar8337_setup_interrupt(avmnet_module_t *this, uint32_t on_off);
int avmnet_ar8337_setup(avmnet_module_t *this);
int avmnet_ar8337_init(avmnet_module_t *this);
int avmnet_ar8337_exit(avmnet_module_t *this);

unsigned int avmnet_s17_rd_phy(avmnet_module_t *this, unsigned int phy_addr, unsigned int reg_addr);
int avmnet_s17_wr_phy(avmnet_module_t *this, unsigned int phy_addr, unsigned int reg_addr, unsigned int write_data);

int avmnet_ar8337_set_status(avmnet_module_t *this, avmnet_device_t *id, avmnet_linkstatus_t status);
void avmnet_ar8337_status_changed(avmnet_module_t *this, avmnet_module_t *caller);

int avmnet_ar8337_HW223_poll(avmnet_module_t *this);

int avmnet_ar8334_hbee_setup(avmnet_module_t *this);
int avmnet_ar8334_vlan_setup(avmnet_module_t *this);
#endif
