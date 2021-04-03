/*------------------------------------------------------------------------------------------*\
 *   Copyright (C) 2011,2012 AVM GmbH <fritzbox_info@avm.de>
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

#if !defined(__AVM_NET_ATHEROS_MAC_)
#define __AVM_NET_ATHEROS_MAC_

#include <linux/interrupt.h>
#include <linux/if_ether.h>
#include <linux/workqueue.h>
#include <linux/version.h>
#include "avmnet_module.h"
#include "avmnet_config.h"

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0)
#define soc_is_ar934x   is_ar934x
#define soc_is_qca9531  is_qca9531
#define soc_is_qca953x  is_qca953x
#define soc_is_qca955x  is_qca955x
#define soc_is_qca956x  is_qca956x
#endif

#define GMAC_MAX_ETH_DEVS 6

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

#define ENET_NUM_AC      4               /* 4 AC categories */
/* QOS stream classes */
#define ENET_AC_BE       0               /* best effort */
#define ENET_AC_BK       1               /* background */
#define ENET_AC_VI       2               /* video */
#define ENET_AC_VO       3               /* voice */

#define  ATHR_PHY_SPEED_10T         0
#define  ATHR_PHY_SPEED_100T        1
#define  ATHR_PHY_SPEED_1000T       2
#define  ATHR_PHY_SPEED_UNKNOWN     3

/*------------------------------------------------------------------------------------------*\
 * IP needs 16 bit alignment. But RX DMA needs 4 bit alignment. We sacrifice IP
 * Plus Reserve extra head room for wmac
 *
 * (ETH_FRAME_LEN + VLAN_ETH_HLEN + ETH_FCS_LEN) 
 * (1514 + 18 + 4)
 *
 * (ATHR_GMAC_RX_RESERVE + ATHR_GMAC_RX_PACKET_SIZE + NET_SKB_PAD + NET_IP_ALIGN + ATHR_GMAC_CACHE_ALIGN)
 * (128 + 1536 + 2 + ATHR_GMAC_CACHE_ALIGN) % (1<<CONFIG_MIPS_L1_CACHE_SHIFT) = 0
\*------------------------------------------------------------------------------------------*/
#define ATHR_GMAC_CACHE_ALIGN			30
#define ATHR_GMAC_RX_RESERVE           (128)         /*--- Plus Reserve extra head room for wmac ---*/
#define ATHR_GMAC_RX_PACKET_SIZE	(ETH_FRAME_LEN + VLAN_ETH_HLEN + ETH_FCS_LEN)
#define ATHR_GMAC_RX_BUF_SIZE		(ATHR_GMAC_RX_RESERVE + ATHR_GMAC_RX_PACKET_SIZE + NET_IP_ALIGN + ATHR_GMAC_CACHE_ALIGN)

#define ATHR_GMAC_TX_FIFO_LEN          2048
#define ATHR_GMAC_TX_MIN_DS_LEN        128
#define ATHR_GMAC_TX_MAX_DS_LEN        ATHR_GMAC_TX_FIFO_LEN
#define ATHR_GMAC_TX_MTU_LEN           1536

#define ATHR_GMAC_NAPI_WEIGHT          64

#define ATHR_FIFO_BYTE_NIBBLE   (1<<19)
#define ATHR_FIFO_DROP_SHORT    (1<<18)
#define ATHR_FIFO_UNI           (1<<17)
#define ATHR_FIFO_TRUNC         (1<<16)
#define ATHR_FIFO_LONG          (1<<15)
#define ATHR_FIFO_VLAN          (1<<14)
#define ATHR_FIFO_UNSUP         (1<<13)
#define ATHR_FIFO_PAUSE         (1<<12)
#define ATHR_FIFO_CONTROL       (1<<11)
#define ATHR_FIFO_DRIBLLE       (1<<10)
#define ATHR_FIFO_BROADCAST     (1<<9)
#define ATHR_FIFO_MULTICAST     (1<<8)
#define ATHR_FIFO_OK            (1<<7)
#define ATHR_FIFO_OUT_RANGE     (1<<6)
#define ATHR_FIFO_LEN_MISMATCH  (1<<5)
#define ATHR_FIFO_CRC_ERROR     (1<<4)
#define ATHR_FIFO_CODE_ERROR    (1<<3)
#define ATHR_FIFO_FALSE_CARRIER (1<<2)
#define ATHR_FIFO_RX_DV         (1<<1)
#define ATHR_FIFO_DROP          (1<<0)

#define ATHR_ALL_FRAMES         0x3FFFF

#define ATHR_DEFAULT_FRAMES      \
    (   \
        ATHR_FIFO_RX_DV | ATHR_FIFO_LEN_MISMATCH | ATHR_FIFO_OUT_RANGE | \
        ATHR_FIFO_OK | ATHR_FIFO_MULTICAST | ATHR_FIFO_BROADCAST | \
        ATHR_FIFO_CONTROL | ATHR_FIFO_UNSUP | ATHR_FIFO_VLAN | ATHR_FIFO_UNI | \
        ATHR_FIFO_DROP_SHORT \
    )

#define athr_mac_dma_cache_sync(b, c)	                        \
do {						                                    \
        dma_cache_sync(NULL, (void *)b, c, DMA_TO_DEVICE);	    \
} while (0)

#define athr_mac_cache_inv(d, s)                                \
do {                                                            \
        dma_cache_sync(NULL, (void *)d, s, DMA_FROM_DEVICE);    \
} while (0)

#define athr_gmac_reset_mask(_no) (_no) ? (ATH_RESET_GE1_MAC) : (ATH_RESET_GE0_MAC)  

/*------------------------------------------------------------------------------------------*\
 * tx/rx stop start
\*------------------------------------------------------------------------------------------*/
#define athr_gmac_tx_stopped(_mac_base)                                         \
    (!(athr_gmac_reg_rd((_mac_base), ATHR_GMAC_DMA_TX_CTRL) & ATHR_GMAC_TXE))

#define athr_gmac_rx_start(_mac_base)                                           \
    athr_gmac_reg_wr((_mac_base), ATHR_GMAC_DMA_RX_CTRL, ATHR_GMAC_RXE)

#define athr_gmac_rx_stop(_mac_base)                                            \
    athr_gmac_reg_wr((_mac_base), ATHR_GMAC_DMA_RX_CTRL, 0)

#define athr_gmac_tx_start_qos(_mac_base,ac)                               \
switch(ac) {                               \
    case ENET_AC_VO:                               \
        athr_gmac_reg_wr((_mac_base), ATHR_GMAC_DMA_TX_CTRL_Q0, ATHR_GMAC_TXE);  \
        break;                             \
    case ENET_AC_VI:                           \
        athr_gmac_reg_wr((_mac_base), ATHR_GMAC_DMA_TX_CTRL_Q1, ATHR_GMAC_TXE);  \
        break;                             \
    case ENET_AC_BK:                           \
        athr_gmac_reg_wr((_mac_base), ATHR_GMAC_DMA_TX_CTRL_Q2, ATHR_GMAC_TXE);  \
        break;                             \
    case ENET_AC_BE:                           \
        athr_gmac_reg_wr((_mac_base), ATHR_GMAC_DMA_TX_CTRL_Q3, ATHR_GMAC_TXE);  \
        break;                             \
}                                       

#define athr_gmac_tx_start(_mac_base)                                       \
            athr_gmac_reg_wr((_mac_base), ATHR_GMAC_DMA_TX_CTRL_Q0, ATHR_GMAC_TXE) 

#define athr_gmac_tx_stop(_mac_base)                        \
    athr_gmac_reg_wr((_mac_base), ATHR_GMAC_DMA_TX_CTRL, 0)

#define athr_gmac_ring_incr(max_idx, _idx) \
        if(unlikely(++(_idx) == max_idx)) (_idx) = 0;

/*------------------------------------------------------------------------------------------*\
 * ownership of descriptors between DMA and cpu
\*------------------------------------------------------------------------------------------*/
#define athr_gmac_rx_owned_by_dma(_ds)     ((_ds)->packet.Bits.is_empty == 1)
#define athr_gmac_rx_give_to_dma(_ds)      ((_ds)->packet.Bits.is_empty = 1)
#define athr_gmac_tx_owned_by_dma(_ds)     ((_ds)->packet.Bits.is_empty == 0)
#define athr_gmac_tx_give_to_dma(_ds)      ((_ds)->packet.Bits.is_empty = 0)
#define athr_gmac_tx_own(_ds)              ((_ds)->packet.Bits.is_empty = 1)

/*------------------------------------------------------------------------------------------*\
 * Interrupts 
\*------------------------------------------------------------------------------------------*/
#define ATHR_GMAC_INTR_MASK    (   ATHR_GMAC_INTR_RX            \
                                 | ATHR_GMAC_INTR_RX_BUS_ERROR  \
                                 | ATHR_GMAC_INTR_TX_BUS_ERROR  \
                                 /*--- | ATHR_GMAC_INTR_RX_OVF \ ---*/ \
                                 /*--- | ATHR_GMAC_INTR_TX_URN ---*/ \
                                 /*--- | ATHR_GMAC_INTR_TX ---*/ \
                                )

#define athr_gmac_get_isr(_mac_base)     (athr_gmac_reg_rd(_mac_base, ATHR_GMAC_DMA_INTR) & (0xFFFF))
#define athr_gmac_get_msr(_mac_base)      athr_gmac_reg_rd(_mac_base, ATHR_GMAC_DMA_INTR_MASK)
#define athr_gmac_int_enable(_mac_base)   athr_gmac_reg_wr(_mac_base, ATHR_GMAC_DMA_INTR_MASK, ATHR_GMAC_INTR_MASK)
#define athr_gmac_int_disable(_mac_base)  athr_gmac_reg_wr(_mac_base, ATHR_GMAC_DMA_INTR_MASK, 0)

/*
 * ACKS:
 * - We just write our bit - its write 1 to clear.
 * - These are not rmw's so we dont need locking around these.
 * - Txurn and rxovf are not fastpath and need consistency, so we use the flush
 *   version of reg write.
 * - ack_rx is done every packet, and is largely only for statistical purposes;
 *   so we use the no flush version and later cause an explicite flush.
 */
#define athr_gmac_intr_ack_txurn(_mac_base)  athr_gmac_reg_wr((_mac_base), ATHR_GMAC_DMA_TX_STATUS, ATHR_GMAC_TX_STATUS_URN);
#define athr_gmac_intr_ack_rx(_mac_base)     athr_gmac_reg_wr((_mac_base), ATHR_GMAC_DMA_RX_STATUS, ATHR_GMAC_RX_STATUS_PKT_RCVD);
#define athr_gmac_intr_ack_rxovf(_mac_base)  athr_gmac_reg_wr((_mac_base), ATHR_GMAC_DMA_RX_STATUS, ATHR_GMAC_RX_STATUS_OVF);
#define athr_gmac_intr_ack_tx(_mac_base)     athr_gmac_reg_wr((_mac_base), ATHR_GMAC_DMA_TX_STATUS, ATHR_GMAC_TX_STATUS_PKT_SENT);
#define athr_gmac_intr_ack_txbe(_mac_base)   athr_gmac_reg_wr((_mac_base), ATHR_GMAC_DMA_TX_STATUS, ATHR_GMAC_TX_STATUS_BUS_ERROR);
#define athr_gmac_intr_ack_rxbe(_mac_base)   athr_gmac_reg_wr((_mac_base), ATHR_GMAC_DMA_RX_STATUS, ATHR_GMAC_RX_STATUS_BUS_ERROR);

/*
 * Enable Disable. These are Read-Modify-Writes. Sometimes called from ISR
 * sometimes not. So the caller explicitely handles locking.
 */
#define athr_gmac_intr_disable_txurn(_mac_base)                                 \
    athr_gmac_reg_rmw_clear((_mac_base), ATHR_GMAC_DMA_INTR_MASK, ATHR_GMAC_INTR_TX_URN);

#define athr_gmac_intr_enable_txurn(_mac_base)                                  \
    athr_gmac_reg_rmw_set((_mac_base), ATHR_GMAC_DMA_INTR_MASK, ATHR_GMAC_INTR_TX_URN);

#define athr_gmac_intr_enable_tx(_mac_base)                                     \
    athr_gmac_reg_rmw_set((_mac_base), ATHR_GMAC_DMA_INTR_MASK, ATHR_GMAC_INTR_TX);

#define athr_gmac_intr_disable_tx(_mac_base)                                    \
    athr_gmac_reg_rmw_clear((_mac_base), ATHR_GMAC_DMA_INTR_MASK, ATHR_GMAC_INTR_TX);

#define athr_gmac_intr_disable_recv(_mac_base)                                  \
    athr_gmac_reg_rmw_clear(_mac_base, ATHR_GMAC_DMA_INTR_MASK, ATHR_GMAC_INTR_RX );


#define athr_gmac_intr_enable_rxovf(_mac_base)                                  \
    athr_gmac_reg_rmw_set((_mac_base), ATHR_GMAC_DMA_INTR_MASK, ATHR_GMAC_INTR_RX_OVF);

#define athr_gmac_intr_disable_rxovf(_mac_base)                                 \
    athr_gmac_reg_rmw_clear(_mac_base, ATHR_GMAC_DMA_INTR_MASK, ATHR_GMAC_INTR_RX_OVF);

#define athr_gmac_intr_enable_recv(_mac_base)                                   \
    athr_gmac_reg_rmw_set(_mac_base, ATHR_GMAC_DMA_INTR_MASK, ATHR_GMAC_INTR_RX);

#define athr_gmac_intr_enable_poll(_mac_base)                                   \
    athr_gmac_reg_rmw_set(_mac_base, ATHR_GMAC_DMA_INTR_MASK, ATHR_GMAC_INTR_RX | ATHR_GMAC_INTR_TX | ATHR_GMAC_INTR_RX_OVF);

#define athr_gmac_intr_disable_rxtx(_mac_base)                                   \
    athr_gmac_reg_rmw_clear(_mac_base, ATHR_GMAC_DMA_INTR_MASK, ATHR_GMAC_INTR_RX | ATHR_GMAC_INTR_TX);

#if defined(CONFIG_CPU_BIG_ENDIAN)
#define addr_to_words(addr, w1, w2)  {                                 \
    w1 = (addr[5] << 24) | (addr[4] << 16) | (addr[3] << 8) | addr[2]; \
    w2 = (addr[1] << 24) | (addr[0] << 16) | 0;                        \
}
#else
#define addr_to_words(addr, w1, w2)  {                                 \
    w1 = (addr[0] << 24) | (addr[1] << 16) | (addr[2] << 8) | addr[3]; \
    w2 = (addr[4] << 24) | (addr[5] << 16) | 0;                        \
}
#endif

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
#if defined(CONFIG_MACH_AR934x) || defined(CONFIG_MACH_QCA956x) || \
    defined(CONFIG_SOC_AR934X)  || defined(CONFIG_SOC_QCA956X)
    /*--- diesen Header gibt es nur beim internen Switch - ein Header bei RX und TX ---*/
typedef struct __attribute__ ((packed)) _athr_special_header {
    unsigned short version  : 2;
    unsigned short priority : 2;
    unsigned short type     : 4;
    unsigned short from_cpu : 1;
    unsigned short portnum  : 7;
} athr_special_header_t ;
typedef athr_special_header_t *p_athr_special_header_t;

typedef union __attribute__ ((packed)) _athr_mac_packet_header {
    struct __attribute__ ((packed)) __athr_mac_packet_header {
        unsigned char dest_mac[ETH_ALEN];
        unsigned char source_mac[ETH_ALEN];
        athr_special_header_t header;
        unsigned short data;
    } s;
    unsigned char buff[2 * ETH_ALEN + sizeof(athr_special_header_t) + sizeof(unsigned short)];
} athr_mac_packet_header_t ;

typedef struct __attribute__ ((packed)) _athr_special_tx_header {
    unsigned short version  : 2;
    unsigned short priority : 2;
    unsigned short type     : 4;
    unsigned short from_cpu : 1;
    unsigned short portnum  : 7;
} athr_special_tx_header_t ;
typedef athr_special_tx_header_t *p_athr_special_tx_header_t;
#else
/*--- externe Switches - jeweils ein Header bei RX und einer bei TX ---*/
    /*--- RX aus sich des Switches ---*/
typedef struct __attribute__ ((packed)) _athr_special_header {
#if defined(__BIG_ENDIAN_BITFIELD)
    unsigned short version  : 2;
    unsigned short priority : 3;
    unsigned short type     : 3;
    unsigned short from_cpu : 1;
    unsigned short portnum  : 7;
#elif defined(__LITTLE_ENDIAN_BITFIELD)
    unsigned short portnum  : 7;
    unsigned short from_cpu : 1;
    unsigned short type     : 3;
    unsigned short priority : 3;
    unsigned short version  : 2;
#endif
} athr_special_header_t ;
typedef athr_special_header_t *p_athr_special_header_t;

typedef union __attribute__ ((packed)) _athr_mac_packet_header {
    struct __attribute__ ((packed)) __athr_mac_packet_header {
        unsigned char dest_mac[ETH_ALEN];
        unsigned char source_mac[ETH_ALEN];
        athr_special_header_t header;
        unsigned short data;
    } s;
    unsigned char buff[2 * ETH_ALEN + sizeof(athr_special_header_t) + sizeof(unsigned short)];
} athr_mac_packet_header_t ;

typedef struct __attribute__ ((packed)) _athr_special_tx_header {
#if defined(__BIG_ENDIAN_BITFIELD)
    unsigned short version  : 2;
    unsigned short priority : 3;
    unsigned short type : 5;
    unsigned short reserved     : 2;
    unsigned short frame_with_tag : 1;
    unsigned short portnum  : 3;
#elif defined(__LITTLE_ENDIAN_BITFIELD)
    unsigned short portnum  : 3;
    unsigned short frame_with_tag : 1;
    unsigned short reserved     : 2;
    unsigned short type : 5;
    unsigned short priority : 3;
    unsigned short version  : 2;
#endif
} athr_special_tx_header_t;
typedef athr_special_tx_header_t *p_athr_special_tx_header_t;
#endif

/*------------------------------------------------------------------------------------------*\
 * h/w descriptor
\*------------------------------------------------------------------------------------------*/
typedef union _athr_mac_packet_ {
    volatile uint32_t Register;
    struct _packet_ {
        volatile uint32_t    is_empty       :  1;
        volatile uint32_t    res1           :  6;
        volatile uint32_t    more           :  1;
        volatile uint32_t    res2           :  3;
        volatile uint32_t    ftpp_override  :  5;
        volatile uint32_t    res3           :  2;
        volatile uint32_t    pkt_size       : 14;
    } Bits;
} athr_mac_packet;

#define ATHR_TX_EMPTY   (1<<31)

typedef struct {
    volatile uint32_t        pkt_start_addr;
    athr_mac_packet packet;
    volatile uint32_t        next_desc;
    struct sk_buff  *pskb;       /*--- ptr to skb ---*/
} __attribute__((aligned(4))) athr_gmac_desc_t;

/*------------------------------------------------------------------------------------------*\
 * Tx and Rx descriptor rings;
\*------------------------------------------------------------------------------------------*/
typedef struct {
    athr_gmac_desc_t   *ring_desc;          /* hardware descriptors */
    athr_mac_packet_header_t *mac_txheader;
    dma_addr_t         ring_desc_dma;       /* dma addresses of desc*/
    dma_addr_t         txheader_dma;        /* dma addresses of desc*/
    unsigned int       ring_head;           /* producer index       */
    unsigned int       ring_tail;           /* consumer index       */
    unsigned int       ring_nelem;          /* nelements            */
    unsigned int              pad;                 /*--- align to cachesize ---*/
}athr_gmac_ring_t;


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
enum ath_mac_unit {
    ath_gmac0,
    ath_gmac1
};

#define ATHR_RMON_COUNTERS 44
#define ATHR_RMON_BITS 31
struct athr_rmon_cache {
	u64 sum[ATHR_RMON_COUNTERS];
	u32 prev_val[ATHR_RMON_COUNTERS];
};

struct athmac_context {
    athr_gmac_ring_t        mac_rxring;
    athr_gmac_ring_t        mac_txring;

    void (*tx_reap)(struct athmac_context *mac, unsigned int sent);
    unsigned int            BaseAddress;
    unsigned int            BaseAddressMdio;

    unsigned int            num_tx_desc;
    unsigned int            num_rx_desc;
    struct net_device       *device;
#if defined(ATHR_USE_NAPI)
    struct napi_struct      napi;
#else
    struct tasklet_struct   irqtasklet;
#endif

    unsigned int            reset;
    unsigned int            irq;

    enum ath_mac_unit       mac_unit;
    unsigned char           txdma_acked;
    unsigned char           dma_stopped;
    unsigned char           dma_reset;

    unsigned int            qstart_thresh;

    struct timer_list       txreaptimer;
    struct timer_list       mac_oom_timer;

    spinlock_t              spinlock;
    struct semaphore        lock;
    avmnet_device_t         *devices_lookup[GMAC_MAX_ETH_DEVS];

    avmnet_module_t         *this_module;
    struct athr_rmon_cache rmon_cache;
};

int athmac_init(avmnet_module_t *this_module);
int athmac_setup(avmnet_module_t *this_module);
int athmac_setup_hw219(avmnet_module_t *this_module);
int athmac_exit(avmnet_module_t *this_module);

int athmac_gmac_init(avmnet_module_t *this_module);
int athmac_gmac_setup(avmnet_module_t *this_module);
int athmac_gmac_exit(avmnet_module_t *this_module);

int athgmac_reinit(avmnet_module_t *this_module);

unsigned int athmac_reg_read(avmnet_module_t *this_module, unsigned int addr, unsigned int reg);
int athmac_reg_write(avmnet_module_t *this_module, unsigned int addr, unsigned int reg, unsigned int val);
int athmac_gmac_lock(avmnet_module_t *this_module);
void athmac_gmac_unlock(avmnet_module_t *this_module);
int athmac_gmac_trylock(avmnet_module_t *this_module);
int athmac_lock(avmnet_module_t *this_module);
void athmac_unlock(avmnet_module_t *this_module);
int athmac_trylock(avmnet_module_t *this_module);
void athmac_status_changed(avmnet_module_t *this_module, avmnet_module_t *child);
int athmac_poll(avmnet_module_t *this_module);
int athmac_set_status(avmnet_module_t *, avmnet_device_t *id, avmnet_linkstatus_t status);
int athgmac_set_status(avmnet_module_t *, avmnet_device_t *id, avmnet_linkstatus_t status);
int athmac_setup_irq(avmnet_module_t *this_modul, unsigned int onoff);

void athr_gmac_fast_reset(struct athmac_context *mac);

#if 0
int athr_gmac_open(struct net_device *dev);
int athr_gmac_stop(struct net_device *dev);
int athr_gmac_open_plc(struct net_device *dev);
int athr_gmac_stop_plc(struct net_device *dev);
#endif
int athr_gmac_hard_start(struct sk_buff *skb, struct net_device *dev);
void athr_gmac_tx_timeout(struct net_device *dev);
void athr_gmac_setup_eth( struct net_device* dev);
void athr_gmac_setup_eth_priv(avmnet_device_t *avm_device);
struct avmnet_hw_rmon_counter *create_athr_rmon_cnt(avmnet_device_t *avm_dev);

#if defined(CONFIG_MACH_AR724x) || defined(CONFIG_MACH_AR934x) ||\
    defined(CONFIG_SOC_AR724X)  || defined(CONFIG_SOC_AR934X)
int wasp_virian_setup_hw(avmnet_module_t *this);
#endif
#if defined(CONFIG_MACH_QCA955x) || defined(CONFIG_SOC_QCA955X)
int scorpion_setup_hw(avmnet_module_t *this);
#endif
#if defined(CONFIG_MACH_QCA956x) || defined(CONFIG_SOC_QCA956X) || defined(CONFIG_SOC_QCN550X)
int dragonfly_setup_hw(avmnet_module_t *this);
#endif
#if defined(CONFIG_MACH_QCA953x) || defined(CONFIG_SOC_QCA953X)
int hbee_setup_hw(avmnet_module_t *this);
#endif
#endif
