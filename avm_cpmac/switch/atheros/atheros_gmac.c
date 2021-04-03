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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <asm/byteorder.h>
#include <linux/init.h>
#include <linux/errno.h>

#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/mm.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/ethtool.h>
#if __has_include(<avm/pa/pa.h>)
#include <avm/pa/pa.h>
#else
#include <linux/avm_pa.h>
#endif

#include <asm/irq.h>
#include <asm/io.h>
#include <asm/prom.h>
#include <net/sch_generic.h>
#include <asm/mach_avm.h>

#include <linux/if_vlan.h>

#include "avmnet_debug.h"
#include "avmnet_module.h"
#include "avmnet_config.h"
#include "avmnet_common.h"
#include "../management/avmnet_links.h"

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,4,46)
#include <atheros.h>
#else
#include <avm_atheros.h>
#endif
#include "ag934x.h"
#include "atheros_mac.h"

#if __has_include(<avm/profile/profile.h>)
#include <avm/profile/profile.h>
#else
#include <linux/avm_profile.h>
#endif

static void update_rmon_cache(avmnet_module_t *this);
static int read_rmon_all_open(struct inode *inode, struct file *file);
static const struct file_operations read_rmon_all_fops = {
    .open    = read_rmon_all_open,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = single_release,
};

static struct resource ath_gmac_gpio = {
    .flags = IORESOURCE_IO,
    .start = -1,
    .end   = -1
};

static unsigned athr_tx_desc_num[ATHR_GMAC_NMACS] = { CONFIG_ATHR_GMAC_NUMBER_TX_PKTS, CONFIG_ATHR_GMAC_NUMBER_TX_PKTS_1 };
static unsigned athr_rx_desc_num[ATHR_GMAC_NMACS] = { CONFIG_ATHR_GMAC_NUMBER_RX_PKTS, CONFIG_ATHR_GMAC_NUMBER_RX_PKTS_1 };

#define TX_TIMEOUT  (10 * HZ)
#define TX_MAX_DESC_PER_DS_PKT  2       /*--- darf nicht kleiner als 2 sein, sonst funktioniert athr_gmac_tx_reap nicht ---*/

/*------------------------------------------------------------------------------------------*\
 * AVM_REBOOT_STATUS uses internal sram at 0x500
 * CONFIG_SOC_QCN550X CONFIG_SOC_AR934X internal sram is too small 
 * CONFIG_SOC_QCA953X nmi-workarround is using internal sram
 * ATH_REBOOT_LOGBUFFER is using internal sram
\*------------------------------------------------------------------------------------------*/
#if defined(CONFIG_MACH_QCA955x) || defined(CONFIG_SOC_QCA955X)
#define ATHR_USE_IRAM_DESC
#endif
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static inline avmnet_device_t *find_avmnet_device(unsigned int port)
{
    avmnet_device_t *avm_dev = NULL;
    int i;

    for(i = 0; i < avmnet_hw_config_entry->nr_avm_devices; ++i){
        if(avmnet_hw_config_entry->avm_devices[i]->vlanID == port){
            avm_dev = avmnet_hw_config_entry->avm_devices[i];
            break;
        }
    }

    return avm_dev;
}

#if 0
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
atomic_t athr_rx_disable_count;

static inline void athr_gmac_intr_disable_recv(struct athmac_context *mac) {
    unsigned long flags;
    spin_lock_irqsave(&(mac->spinlock), flags);
    if (atomic_inc_return(&athr_rx_disable_count) != 1) {
        printk(KERN_ERR "{%s} ERROR IRQ disable count 0x%x 0x%x\n", __func__, athr_rx_disable_count, athr_gmac_reg_rd((mac->BaseAddress), ATHR_GMAC_DMA_INTR_MASK));
        atomic_set(&athr_rx_disable_count, 1);
    }
    athr_gmac_reg_rmw_clear(mac->BaseAddress, ATHR_GMAC_DMA_INTR_MASK, ATHR_GMAC_INTR_RX );
    spin_unlock_irqrestore(&mac->spinlock, flags);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static inline void athr_gmac_intr_enable_recv(struct athmac_context *mac) {
    unsigned long flags;
    spin_lock_irqsave(&(mac->spinlock), flags);
    if (atomic_dec_return(&athr_rx_disable_count) != 0) {
        printk(KERN_ERR "{%s} ERROR IRQ enable count 0x%x 0x%x\n", __func__, athr_rx_disable_count, athr_gmac_reg_rd((mac->BaseAddress), ATHR_GMAC_DMA_INTR_MASK));
        atomic_set(&athr_rx_disable_count, 0);
    }
    athr_gmac_reg_rmw_set(mac->BaseAddress, ATHR_GMAC_DMA_INTR_MASK, ATHR_GMAC_INTR_RX );
    spin_unlock_irqrestore(&mac->spinlock, flags);
}
#endif

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
#if 0
void athr_print_descriptors(struct athmac_context *mac) {
    unsigned int i;
    athr_gmac_ring_t *r;
    athr_gmac_desc_t *dc;
    r = &mac->mac_rxring; 
    dc = r->ring_desc;
    printk("[%s] head %d tail %d anzahl %d\n", __FUNCTION__, 
            r->ring_head, 
            r->ring_tail, 
            r->ring_nelem);
    for(i = 0 ; i < r->ring_nelem; i++, dc++ ) {
        printk("[%3d] %#p -> %#p %08x %c %c %d\n", 
                i,
                dc,
                dc->next_desc,
                dc->pkt_start_addr,
                dc->is_empty ? 'E' : '-',
                dc->more ? 'M' : '-',
                dc->pkt_size
              );
    }
}
#endif

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
#if ! defined(ATHR_USE_IRAM_DESC)
static int athr_gmac_ring_alloc(athr_gmac_ring_t *r, int count) {

    r->ring_desc  =  dma_alloc_coherent(NULL, sizeof(athr_gmac_desc_t) * count, 
                                              &r->ring_desc_dma, GFP_DMA);

    if (! r->ring_desc) {
        AVMNET_ERR("[%s] : unable to allocate coherent descs\n", __func__);
        return -ENOMEM;
    }

    memset(r->ring_desc, 0, sizeof(athr_gmac_desc_t) * count);
    return 0;
}
#endif

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static void athr_gmac_ring_free(athr_gmac_ring_t *r) {

    dma_free_coherent(NULL, sizeof(athr_gmac_desc_t)*r->ring_nelem, r->ring_desc, r->ring_desc_dma);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static void athr_gmac_rx_free(struct athmac_context *mac) {
    athr_gmac_ring_free(&mac->mac_rxring);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static void athr_gmac_tx_free(struct athmac_context *mac) {
    
    athr_gmac_ring_free(&mac->mac_txring);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
#if defined(ATHR_USE_IRAM_DESC)
#define IRAM_DESC_RX	1
#define IRAM_DESC_TX	2
static unsigned long ag71xx_ring_bufs[4] = {
	0x1d000008UL,		/*--- AVM_REBOOT_STATUS uses internal sram at 0x500 ---*/
	0x1d001008UL,		/*--- IRAM_DESC_TX ---*/
	0x1d002008UL,		/*--- IRAM_DESC_RX ---*/
	0x1d003008UL		
};
#endif /*--- #if defined(ATHR_USE_IRAM_DESC) ---*/

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static int athr_gmac_tx_alloc(struct athmac_context *mac) {

	athr_gmac_ring_t *r ;
	athr_gmac_desc_t *ds;
	int ringlen;
	int i, next;

	r  = &mac->mac_txring;
	memset(r, 0, sizeof(athr_gmac_ring_t));
	r->ring_nelem = mac->num_tx_desc;

	if (mac->mac_unit == ath_gmac1)
		ringlen = r->ring_nelem * 2;
	else
		ringlen = r->ring_nelem;

#if ! defined(CONFIG_MACH_QCA955x) && ! defined(CONFIG_SOC_QCA955X)
	r->mac_txheader  = dma_alloc_coherent(NULL, sizeof(athr_mac_packet_header_t) * r->ring_nelem,
			&r->txheader_dma, GFP_DMA);
#endif

#if defined(ATHR_USE_IRAM_DESC)
	r->ring_desc  = ioremap_nocache(ag71xx_ring_bufs[IRAM_DESC_TX], sizeof(athr_gmac_desc_t) * ringlen);
	if ( ! r->ring_desc)
		return -ENOMEM;
	memset(r->ring_desc, 0, sizeof(athr_gmac_desc_t) * ringlen);
#else
	if (athr_gmac_ring_alloc(r, ringlen))
		return -ENOMEM;
#endif

	/*----------------------------------------------------------------------------------*\
	 * Es werden immer descriptor paare alloziert, deshalb sind in Wirklichkeit
	 * ring_nelem * 2 Descriptoren vorhanden
	 \*----------------------------------------------------------------------------------*/
	ds = r->ring_desc;
	for(i = 0; i < ringlen ; i++ ) {
		next             =   (i == (ringlen - 1)) ? 0 : (i + 1);
		ds[i].next_desc = ((unsigned int)&ds[next]) & 0x1fffffff;
		athr_gmac_tx_own(&ds[i]);
	}
	return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static inline struct sk_buff * athr_gmac_buffer_alloc(void) {

    return dev_alloc_skb(ATHR_GMAC_RX_BUF_SIZE);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static int athr_gmac_rx_alloc(struct athmac_context  *mac) {

    athr_gmac_ring_t *r;
    athr_gmac_desc_t *ds;
    unsigned int i, next;

    /*--- AVMNET_DEBUG("[%s] num_rx_desc %d\n", __func__, mac->num_rx_desc); ---*/

    r = &mac->mac_rxring; 
    memset(r, 0, sizeof(athr_gmac_ring_t));
    r->ring_nelem = mac->num_rx_desc;

#if defined(ATHR_USE_IRAM_DESC)
    r->ring_desc  = ioremap_nocache(ag71xx_ring_bufs[IRAM_DESC_RX], sizeof(athr_gmac_desc_t) * r->ring_nelem);
	if ( ! r->ring_desc)
		return -ENOMEM;
    memset(r->ring_desc, 0, sizeof(athr_gmac_desc_t) * r->ring_nelem);
#else
    if (athr_gmac_ring_alloc(r, r->ring_nelem))
        return 1;
#endif

    ds = r->ring_desc;
    for(i = 0; i < r->ring_nelem; i++ ) {
        next = (i == (r->ring_nelem - 1)) ? 0 : (i + 1);
        ds[i].next_desc = ((unsigned int)&ds[next]) & 0x1fffffff;
    }

    for (i = 0; i < r->ring_nelem; i++) {
        ds = &r->ring_desc[i];
        ds->pskb = athr_gmac_buffer_alloc();
        if ( ! ds->pskb) 
            goto error;

/*---         dma_cache_wback((unsigned long)skb_shinfo(ds->pskb), sizeof(struct skb_shared_info)); ---*/
        dma_cache_inv((unsigned long)(ds->pskb->data), ATHR_GMAC_RX_BUF_SIZE);

        if (mac->mac_unit == ath_gmac0)
            skb_reserve(ds->pskb, NET_IP_ALIGN + ATHR_GMAC_RX_RESERVE);

        ds->packet.Register = 0;
        ds->pkt_start_addr  = virt_to_phys(ds->pskb->data);
		wmb();
        athr_gmac_rx_give_to_dma(ds);
    }

    return 0;
error:
    AVMNET_ERR("[%s] : unable to allocate rx\n", __func__);
    athr_gmac_rx_free(mac);
    return -ENOMEM;
}


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static void athmac_gmac_enable_hw(struct athmac_context *mac) {

    /*--- enable FLOW-Control ---*/
#if defined(CONFIG_MACH_QCA955x) || defined(CONFIG_SOC_QCA955X)
    athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_CFG1, (ATHR_GMAC_CFG1_RX_EN | ATHR_GMAC_CFG1_RX_SYNC_EN |
                                                        ATHR_GMAC_CFG1_TX_EN | ATHR_GMAC_CFG1_TX_SYNC_EN |
                                                        ATHR_GMAC_CFG1_RX_FCTL));
#else
    athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_CFG1, (ATHR_GMAC_CFG1_RX_EN   | 
                                                        ATHR_GMAC_CFG1_TX_EN   | 
                                                        ATHR_GMAC_CFG1_RX_FCTL |
                                                        ATHR_GMAC_CFG1_TX_FCTL));
#endif

    AVMNET_TRC("[%s] : cfg1 %#x cfg2 %#x\n", __func__, athr_gmac_reg_rd(mac->BaseAddress, ATHR_GMAC_CFG1), athr_gmac_reg_rd(mac->BaseAddress, ATHR_GMAC_CFG2));
}

#if defined(CONFIG_MACH_QCA956x) || defined(CONFIG_SOC_QCA956X) || defined(CONFIG_SOC_QCN550X)
int dragonfly_setup_hw(avmnet_module_t *this) {

    struct athmac_context *mac = (struct athmac_context *)this->priv;
    athr_gmac_ring_t *tx, *rx;

    athr_gmac_reg_wr (mac->BaseAddress, ATHR_GMAC_FIFO_CFG_0, 0x1f00);
    athr_gmac_reg_wr (mac->BaseAddress, ATHR_GMAC_FIFO_CFG_1, 0x10ffff);
    athr_gmac_reg_wr (mac->BaseAddress, ATHR_GMAC_FIFO_CFG_2, 0x015500aa);
    athr_gmac_reg_wr (mac->BaseAddress, ATHR_GMAC_FIFO_CFG_3, 0x1f00140);
    athr_gmac_reg_wr (mac->BaseAddress, ATHR_GMAC_FIFO_CFG_4, ATHR_ALL_FRAMES);

    athr_gmac_reg_wr (mac->BaseAddress, ATHR_GMAC_FIFO_CFG_5, ATHR_DEFAULT_FRAMES);
    athr_gmac_reg_wr (mac->BaseAddress, ATHR_GMAC_DMA_TXFIFO_THRESH ,0x01d80160);
    athr_gmac_reg_rmw_clear (mac->BaseAddress, ATHR_GMAC_DMA_TX_ARB_CFG, ATHR_GMAC_TX_QOS_MODE_WEIGHTED);

    athr_gmac_reg_rmw_clear(mac->BaseAddress, ATHR_GMAC_CFG2, (ATHR_GMAC_CFG2_IF_1000| ATHR_GMAC_CFG2_IF_10_100));

    switch (mac->BaseAddress) {
        case ATH_GE0_BASE:
            if (mac->this_module->initdata.mac.flags & AVMNET_CONFIG_FLAG_SGMII_ENABLE) {
                ath_reg_wr ( ATHR_GMAC_ETH_CFG, ATH_GMAC_ETH_CFG_GE0_SGMII_SET (1));

                ath_reg_wr ( ATH_PLL_ETH_SGMII, 
                        ATH_PLL_ETH_SGMII_GIGE_SET         (1) | 
                        ATH_PLL_ETH_SGMII_RX_DELAY_SET     (1) | 
                        ATH_PLL_ETH_SGMII_TX_DELAY_SET     (1) | 
                        ATH_PLL_ETH_SGMII_CLK_SEL_SET      (1) | 
                        ATH_PLL_ETH_SGMII_PHASE1_COUNT_SET (1) | 
                        ATH_PLL_ETH_SGMII_PHASE0_COUNT_SET (1) );

                athr_gmac_reg_rmw_set ( mac->BaseAddress, ATHR_GMAC_CFG2, (ATHR_GMAC_CFG2_PAD_CRC_EN | ATHR_GMAC_CFG2_LEN_CHECK  | ATHR_GMAC_CFG2_IF_1000));
                athr_gmac_reg_rmw_set ( mac->BaseAddress, ATHR_GMAC_FIFO_CFG_5, ATHR_GMAC_BYTE_PER_CLK_EN);
                break;
            } 

            if( ! (mac->this_module->initdata.mac.flags & AVMNET_CONFIG_FLAG_NO_WANPORT)) {
                AVMNET_DEBUG("{%s} ETH_CFG_SW_PHY_SWAP_SET\n", __func__);
                ath_reg_wr ( ATH_GMAC_ETH_CFG, ATH_GMAC_ETH_CFG_SW_PHY_SWAP_SET (1));
            }

            athr_gmac_reg_rmw_set ( mac->BaseAddress, ATHR_GMAC_CFG2, (ATHR_GMAC_CFG2_PAD_CRC_EN | ATHR_GMAC_CFG2_LEN_CHECK  ));
            athr_gmac_reg_rmw_clear ( mac->BaseAddress, ATHR_GMAC_FIFO_CFG_5, ATHR_GMAC_BYTE_PER_CLK_EN);
            break;

        case ATH_GE1_BASE:
            if(mac->this_module->initdata.mac.flags & AVMNET_CONFIG_FLAG_NO_WANPORT){
                AVMNET_DEBUG("{%s} ETH_CFG_SW_ONLY_MODE_SET\n", __func__);
                ath_reg_rmw_set ( ATH_GMAC_ETH_CFG, ATH_GMAC_ETH_CFG_SW_ONLY_MODE_SET (1));
            }

            athr_gmac_reg_rmw_set ( mac->BaseAddress, ATHR_GMAC_CFG2, (ATHR_GMAC_CFG2_FDX | ATHR_GMAC_CFG2_PAD_CRC_EN | ATHR_GMAC_CFG2_IF_1000)); 
            athr_gmac_reg_rmw_set ( mac->BaseAddress, ATHR_GMAC_FIFO_CFG_5, ATHR_GMAC_BYTE_PER_CLK_EN);
            break;
    }

    tx = &mac->mac_txring;
    rx = &mac->mac_rxring;
#if defined(ATHR_USE_IRAM_DESC)
    athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_DMA_TX_DESC_Q0, ((unsigned int)&tx->ring_desc[0]) & 0x1fffffff);
    athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_DMA_RX_DESC, ((unsigned int)&rx->ring_desc[0]) & 0x1fffffff);
#else
    athr_gmac_reg_wr (mac->BaseAddress, ATHR_GMAC_DMA_TX_DESC_Q0, virt_to_phys(&tx->ring_desc[0]));
    athr_gmac_reg_wr (mac->BaseAddress, ATHR_GMAC_DMA_RX_DESC, virt_to_phys(&rx->ring_desc[0]));
#endif

    AVMNET_TRC("[%s] : ATHR_GMAC_DMA_TX_ARB_CFG 0x%x\n", __func__, athr_gmac_reg_rd(mac->BaseAddress, ATHR_GMAC_DMA_TX_ARB_CFG));

    return 0;
}
#endif

#if defined(CONFIG_MACH_QCA955x) || defined(CONFIG_SOC_QCA955X)
int scorpion_setup_hw(avmnet_module_t *this) {

    struct athmac_context *mac = (struct athmac_context *)this->priv;
    athr_gmac_ring_t *tx, *rx;

    athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_FIFO_CFG_0, 0x1f00);
    athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_FIFO_CFG_1, 0x10ffff);
    athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_FIFO_CFG_2, 0x015500aa);
    athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_FIFO_CFG_3, 0x1f00140);
    athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_FIFO_CFG_4, ATHR_ALL_FRAMES);

    athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_FIFO_CFG_5, ATHR_DEFAULT_FRAMES);
    athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_DMA_TXFIFO_THRESH ,0x01d80160);
    athr_gmac_reg_rmw_clear(mac->BaseAddress, ATHR_GMAC_DMA_TX_ARB_CFG, ATHR_GMAC_TX_QOS_MODE_WEIGHTED);

    athr_gmac_reg_rmw_clear(mac->BaseAddress, ATHR_GMAC_CFG2, (ATHR_GMAC_CFG2_IF_1000| ATHR_GMAC_CFG2_IF_10_100));

    AVMNET_INFO("[%s] mac->mac_unit %d\n", __func__, mac->mac_unit);
    switch (mac->BaseAddress) {
        case ATH_GE0_BASE:
            if (mac->this_module->initdata.mac.flags & AVMNET_CONFIG_FLAG_SGMII_ENABLE) {
                ath_reg_wr(ETH_CFG_ADDRESS, ETH_CFG_GE0_SGMII_SET(1));

                athr_gmac_reg_rmw_set(mac->BaseAddress, ATHR_GMAC_CFG2, (ATHR_GMAC_CFG2_PAD_CRC_EN | ATHR_GMAC_CFG2_LEN_CHECK));
                athr_gmac_reg_rmw_set(mac->BaseAddress, ATHR_GMAC_FIFO_CFG_5, ATHR_GMAC_BYTE_PER_CLK_EN);
            } else {
                /*--- External RMII Mode on GE0,Master and High speed ---*/
                AVMNET_INFO("set External RMII Mode on GE0\n");
#if defined(CONFIG_ATH79_MACH_AVM_HW238)
				ath_reg_wr(ETH_CFG_ADDRESS, ETH_CFG_ETH_RXDV_DELAY_SET(0x3) |
                    						ETH_CFG_ETH_RXD_DELAY_SET(0x3)  |
						                    ETH_CFG_RGMII_GE0_SET(0x1));
#else
                ath_reg_wr(ETH_CFG_ADDRESS, ETH_CFG_RGMII_GE0_SET(1));
#endif

                athr_gmac_reg_rmw_set(mac->BaseAddress, ATHR_GMAC_CFG2, (ATHR_GMAC_CFG2_PAD_CRC_EN | ATHR_GMAC_CFG2_LEN_CHECK | ATHR_GMAC_CFG2_IF_10_100));
                athr_gmac_reg_rmw_clear(mac->BaseAddress, ATHR_GMAC_FIFO_CFG_5, ATHR_GMAC_BYTE_PER_CLK_EN);
            }
            break;
        case ATH_GE1_BASE:
            if (is_qca9558()) {
                AVMNET_INFO("set External RGMII Mode on GE1\n");
                ath_reg_wr(ETH_CFG_ADDRESS, ETH_CFG_RGMII_GE0_SET(1));
            } else {
                ath_reg_wr(ETH_CFG_ADDRESS, ETH_CFG_GE0_SGMII_SET(0));
            }
            athr_gmac_reg_rmw_set(mac->BaseAddress, ATHR_GMAC_CFG2, (ATHR_GMAC_CFG2_FDX | ATHR_GMAC_CFG2_PAD_CRC_EN | ATHR_GMAC_CFG2_IF_1000)); 
            athr_gmac_reg_rmw_set(mac->BaseAddress, ATHR_GMAC_FIFO_CFG_5, ATHR_GMAC_BYTE_PER_CLK_EN);
            break;
    }

    tx = &mac->mac_txring;
    rx = &mac->mac_rxring;

#if defined(ATHR_USE_IRAM_DESC)
    athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_DMA_TX_DESC_Q0, ((unsigned int)&tx->ring_desc[0]) & 0x1fffffff);
    athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_DMA_RX_DESC, ((unsigned int)&rx->ring_desc[0]) & 0x1fffffff);
#else
    athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_DMA_TX_DESC_Q0, virt_to_phys(&tx->ring_desc[0]));
    athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_DMA_RX_DESC, virt_to_phys(&rx->ring_desc[0]));
#endif

    AVMNET_TRC("[%s] : ATHR_GMAC_DMA_TX_ARB_CFG 0x%x\n", __func__, athr_gmac_reg_rd(mac->BaseAddress, ATHR_GMAC_DMA_TX_ARB_CFG));

    return 0;
}
#endif

#if defined(CONFIG_MACH_AR724x) || defined(CONFIG_MACH_AR934x) ||\
    defined(CONFIG_SOC_AR724X)  || defined(CONFIG_SOC_AR934X)
int wasp_virian_setup_hw(avmnet_module_t *this) {

    struct athmac_context *mac = (struct athmac_context *)this->priv;
    athr_gmac_ring_t *tx, *rx;
    unsigned int sw_only;

    athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_FIFO_CFG_0, 0x1f00);
    athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_FIFO_CFG_1, 0x10ffff);
    athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_FIFO_CFG_2, 0x015500aa);
    athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_FIFO_CFG_3, 0x1f00140);
    athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_FIFO_CFG_4, ATHR_ALL_FRAMES);

    athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_FIFO_CFG_5, ATHR_DEFAULT_FRAMES);
    athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_DMA_TXFIFO_THRESH ,0x01d80160);
    athr_gmac_reg_rmw_clear(mac->BaseAddress, ATHR_GMAC_DMA_TX_ARB_CFG, ATHR_GMAC_TX_QOS_MODE_WEIGHTED);

    athr_gmac_reg_rmw_clear(mac->BaseAddress, ATHR_GMAC_CFG2, (ATHR_GMAC_CFG2_IF_1000| ATHR_GMAC_CFG2_IF_10_100));

    AVMNET_INFO("[%s] mac->mac_unit %d\n", __func__, mac->mac_unit);
    switch (mac->BaseAddress) {
        case ATH_GE0_BASE:
            /*--- External RMII Mode on GE0,Master and High speed ---*/
            AVMNET_INFO("set External RMII Mode on GE0\n");

            // make sure we keep WAN PHY attachment to switch, if set
            sw_only = ath_reg_rd(ATHR_GMAC_ETH_CFG) & ATHR_GMAC_ETH_CFG_SW_ONLY_MODE;
            ath_reg_wr(ATHR_GMAC_ETH_CFG, ATHR_GMAC_ETH_CFG_RGMII_EN | sw_only);

            athr_gmac_reg_rmw_set(mac->BaseAddress, ATHR_GMAC_CFG2, (ATHR_GMAC_CFG2_PAD_CRC_EN | ATHR_GMAC_CFG2_LEN_CHECK | ATHR_GMAC_CFG2_IF_10_100));
            athr_gmac_reg_rmw_clear(mac->BaseAddress, ATHR_GMAC_FIFO_CFG_5, ATHR_GMAC_BYTE_PER_CLK_EN);
            break;
        case ATH_GE1_BASE:
            // if configured, connect WAN port PHY to switch MAC 5
            if(mac->this_module->initdata.mac.flags & AVMNET_CONFIG_FLAG_NO_WANPORT){
                ath_reg_rmw_set(ATHR_GMAC_ETH_CFG, ATHR_GMAC_ETH_CFG_SW_ONLY_MODE);
            }

            athr_gmac_reg_rmw_set(mac->BaseAddress, ATHR_GMAC_CFG2, (ATHR_GMAC_CFG2_FDX | ATHR_GMAC_CFG2_PAD_CRC_EN | ATHR_GMAC_CFG2_IF_1000)); 
            athr_gmac_reg_rmw_set(mac->BaseAddress, ATHR_GMAC_FIFO_CFG_5, ATHR_GMAC_BYTE_PER_CLK_EN);
            break;
    }

    tx = &mac->mac_txring;
    rx = &mac->mac_rxring;
#if defined(ATHR_USE_IRAM_DESC)
    athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_DMA_TX_DESC_Q0, ((unsigned int)&tx->ring_desc[0]) & 0x1fffffff);
    athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_DMA_RX_DESC, ((unsigned int)&rx->ring_desc[0]) & 0x1fffffff);
#else
    athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_DMA_TX_DESC_Q0, virt_to_phys(&tx->ring_desc[0]));
    athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_DMA_RX_DESC, virt_to_phys(&rx->ring_desc[0]));
#endif

    AVMNET_TRC("[%s] : ATHR_GMAC_DMA_TX_ARB_CFG 0x%x\n", __func__, athr_gmac_reg_rd(mac->BaseAddress, ATHR_GMAC_DMA_TX_ARB_CFG));

    return 0;
}
#endif

#if defined(CONFIG_MACH_QCA953x) || defined(CONFIG_SOC_QCA953X)
int hbee_setup_hw(avmnet_module_t *this) {

    struct athmac_context *mac = (struct athmac_context *)this->priv;
    athr_gmac_ring_t *tx, *rx;

    athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_FIFO_CFG_0, 0x1f00);
    athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_FIFO_CFG_1, 0x10ffff);
    athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_FIFO_CFG_2, 0x015500aa);
    athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_FIFO_CFG_4, ATHR_ALL_FRAMES);

    athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_FIFO_CFG_5, ATHR_DEFAULT_FRAMES);
    athr_gmac_reg_rmw_clear(mac->BaseAddress, ATHR_GMAC_DMA_TX_ARB_CFG, ATHR_GMAC_TX_QOS_MODE_WEIGHTED);

    athr_gmac_reg_rmw_clear(mac->BaseAddress, ATHR_GMAC_CFG2, (ATHR_GMAC_CFG2_IF_1000| ATHR_GMAC_CFG2_IF_10_100));

    AVMNET_INFO("[%s] mac->mac_unit %d\n", __func__, mac->mac_unit);
    switch (mac->BaseAddress) {
        case ATH_GE0_BASE:
            /*--- beim Honeybee ist dieses Interface max. 100MBit ---*/
            AVMNET_INFO("set Port0 to External on GE0\n");

            if( ! (mac->this_module->initdata.mac.flags & AVMNET_CONFIG_FLAG_NO_WANPORT)) {
                AVMNET_INFO("{%s} ETH_CFG_SW_PHY_SWAP_SET\n", __func__);
                ath_reg_wr(ATH_GMAC_ETH_CFG, ATH_GMAC_ETH_CFG_SW_PHY_SWAP_SET (1));
            } else {
                ath_reg_wr(ATHR_GMAC_ETH_CFG, 0);
            }
            athr_gmac_reg_rmw_set(mac->BaseAddress, ATHR_GMAC_CFG2, (ATHR_GMAC_CFG2_FDX | ATHR_GMAC_CFG2_PAD_CRC_EN | ATHR_GMAC_CFG2_LEN_CHECK | ATHR_GMAC_CFG2_IF_10_100));
            athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_DMA_TXFIFO_THRESH ,0x00880060);
            athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_FIFO_CFG_3, 0x00f00040);
            athr_gmac_reg_rmw_clear(mac->BaseAddress, ATHR_GMAC_FIFO_CFG_5, ATHR_GMAC_BYTE_PER_CLK_EN);
            break;
        case ATH_GE1_BASE:
            /*--- beim Honeybee ist dieses Interface immer 1000MBit ---*/
            // if configured, connect WAN port PHY to switch MAC 5
            if(mac->this_module->initdata.mac.flags & AVMNET_CONFIG_FLAG_NO_WANPORT){
                ath_reg_rmw_set(ATHR_GMAC_ETH_CFG, ATHR_GMAC_ETH_CFG_SW_ONLY_MODE);
            }

            athr_gmac_reg_rmw_set(mac->BaseAddress, ATHR_GMAC_CFG2, (ATHR_GMAC_CFG2_FDX | ATHR_GMAC_CFG2_PAD_CRC_EN | ATHR_GMAC_CFG2_IF_1000)); 
            athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_FIFO_CFG_3, 0x1f00140);
            athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_DMA_TXFIFO_THRESH ,0x01d80160);
            athr_gmac_reg_rmw_set(mac->BaseAddress, ATHR_GMAC_FIFO_CFG_5, ATHR_GMAC_BYTE_PER_CLK_EN);
            break;
    }

    tx = &mac->mac_txring;
    rx = &mac->mac_rxring;
#if defined(ATHR_USE_IRAM_DESC)
    athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_DMA_TX_DESC_Q0, ((unsigned int)&tx->ring_desc[0]) & 0x1fffffff);
    athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_DMA_RX_DESC, ((unsigned int)&rx->ring_desc[0]) & 0x1fffffff);
#else
    athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_DMA_TX_DESC_Q0, virt_to_phys(&tx->ring_desc[0]));
    athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_DMA_RX_DESC, virt_to_phys(&rx->ring_desc[0]));
#endif

    AVMNET_TRC("[%s] : ATHR_GMAC_DMA_TX_ARB_CFG 0x%x\n", __func__, athr_gmac_reg_rd(mac->BaseAddress, ATHR_GMAC_DMA_TX_ARB_CFG));

    return 0;
}
#endif

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int athmac_setup_irq(avmnet_module_t *this __attribute__ ((unused)), 
					 unsigned int on __attribute__ ((unused))) {

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static inline unsigned int athr_gmac_ndesc_unused(athr_gmac_ring_t *ring) {

    int head = ring->ring_head, tail = ring->ring_tail;
    return ((tail > head ? 0 : ring->ring_nelem) + tail - head);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static inline unsigned int athr_gmac_tx_ring_full(athr_gmac_ring_t *r) {

#if (AVMNET_USE_DEBUG_LEVEL_DEBUG != 0)
    if (athr_gmac_ndesc_unused(r) < 10)
        printk(KERN_ERR "{%s} ndesc_unused %d head %d tail %d\n", __func__, athr_gmac_ndesc_unused(r), r->ring_head, r->ring_tail);
#endif
    return (athr_gmac_ndesc_unused(r) < TX_MAX_DESC_PER_DS_PKT);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
#if ! defined(CONFIG_MACH_QCA955x) && ! defined(CONFIG_SOC_QCA955X)
static void athr_gmac_tx_reap(struct athmac_context *mac, unsigned int sent) {    

    athr_gmac_ring_t   *r = &mac->mac_txring;
    athr_gmac_desc_t   *ds, *dsf;
    unsigned int        pkts = 0, bytes = 0;

    sent >>= 1;     /*--- wir senden immer 2 Descriptoren ---*/
    /*--- AVMNET_ERR("sent %d (tail %d)\n", sent, r->ring_tail); ---*/

    while (sent--) {

        if (likely( ! mac->txdma_acked)) {
            athr_gmac_intr_ack_tx(mac->BaseAddress);
        } else {
            mac->txdma_acked--;
        }

        if (likely( ! mac->txdma_acked)) {
            athr_gmac_intr_ack_tx(mac->BaseAddress);
        } else {
            mac->txdma_acked--;
        }

        ds = &r->ring_desc[r->ring_tail * 2];
        athr_gmac_ring_incr(r->ring_nelem, r->ring_tail);
        dsf = ds + 1;
        pkts++;
        bytes += ds->pskb->len;

        if ((ds->pskb->data) == phys_to_virt(dsf->pkt_start_addr - 2 * ETH_ALEN - 2)) 
            dev_kfree_skb_any(ds->pskb);
        else {
            printk(KERN_ERR "{%s} ERROR skb sent %d %p %p\n", __func__, sent, ds->pskb->data, phys_to_virt(dsf->pkt_start_addr - 2 * ETH_ALEN - 2));
        }
    } 
    if ( pkts )
        netdev_completed_queue(mac->device, pkts, bytes);
    return;
}
#endif

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static void athr_gmac_tx_one_reap(struct athmac_context *mac, unsigned int sent) {    

    athr_gmac_ring_t   *r = &mac->mac_txring;
    athr_gmac_desc_t   *ds;
    unsigned int        pkts = 0, bytes = 0;

    while (sent--) {
        athr_gmac_intr_ack_tx(mac->BaseAddress);
        ds = &r->ring_desc[r->ring_tail];
        athr_gmac_ring_incr(r->ring_nelem, r->ring_tail);
        pkts++;
        bytes += ds->pskb->len;
        dev_kfree_skb_any(ds->pskb);
    } 

    if ( pkts )
        netdev_completed_queue(mac->device, pkts, bytes);
    return;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static unsigned int round_robin = 0;
static void athr_gmac_txreap_timer(unsigned long data) {

    avmnet_module_t *this = (avmnet_module_t *)data;
    struct athmac_context *mac = (struct athmac_context *)this->priv;
    unsigned int    sent = (athr_gmac_reg_rd(mac->BaseAddress, ATHR_GMAC_DMA_TX_STATUS) >> 16) & 0xFF;
    int i;

    athr_gmac_ring_t   *r = &mac->mac_txring;

    AVMNET_DEBUG("timer %s sent 0x%x\n", this->name, sent);

    if (sent)
        mac->tx_reap(mac, sent);

    if (athr_gmac_ndesc_unused(r) > mac->qstart_thresh) {

        mac->dma_stopped = 0;

        AVMNET_DEBUG("{%s} wakeup device %s no_devices %d unused %d\n", __func__, this->name, avmnet_hw_config_entry->nr_avm_devices, athr_gmac_ndesc_unused(r));

        round_robin++;
        for(i = 0; i < avmnet_hw_config_entry->nr_avm_devices; i++){
            avmnet_device_t *avmnet_device = avmnet_hw_config_entry->avm_devices[(i + round_robin) % avmnet_hw_config_entry->nr_avm_devices];

            if(avmnet_device->mac_module != this)
                continue;

            if(netif_queue_stopped(avmnet_device->device) && netif_carrier_ok(avmnet_device->device)) {
                AVMNET_DEBUG("{%s} wakeup device %s unused %d\n", __func__, avmnet_device->device->name, athr_gmac_ndesc_unused(r));
                netif_tx_wake_all_queues(avmnet_device->device);
            }
        }
        return;
    } 

    mod_timer(&mac->txreaptimer, jiffies + 1);

    return;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
#if ! defined(CONFIG_MACH_QCA955x) && ! defined(CONFIG_SOC_QCA955X)
static athr_gmac_desc_t *athr_gmac_get_tx_ds(athr_gmac_ring_t *r, struct sk_buff *skb) {

    athr_gmac_desc_t      *ds;
    
    ds = &r->ring_desc[r->ring_head * 2];  /* immer paarweise */
    ds->pkt_start_addr = (unsigned long)&(r->mac_txheader[r->ring_head]);
    ds->pskb = skb;

    athr_gmac_ring_incr(r->ring_nelem, r->ring_head);

    return ds;
}
#endif

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static athr_gmac_desc_t *athr_gmac_get_one_tx_ds(athr_gmac_ring_t *r) {

    unsigned int head;

    head = r->ring_head;
    athr_gmac_ring_incr(r->ring_nelem, r->ring_head);

    return &r->ring_desc[head];
}

/*
 * Tx operation:
 * We do lazy reaping - only when the ring is "thresh" full. If the ring is 
 * full and the hardware is not even done with the first pkt we q'd, we turn
 * on the tx interrupt, stop all q's and wait for h/w to
 * tell us when its done with a "few" pkts, and then turn the Qs on again.
 *
 * Locking:
 * The interrupt only touches the ring when Q's stopped  => Tx is lockless, 
 * except when handling ring full.
 *
 * Desc Flushing: Flushing needs to be handled at various levels, broadly:
 * - The DDr FIFOs for desc reads.
 * - WB's for desc writes.
 */
static noinline void athr_gmac_handle_tx_full(avmnet_module_t *this) {

    struct athmac_context *mac = (struct athmac_context *)this->priv;
    avmnet_device_t *avmnet_device;
    int i;

    AVMNET_DEBUG("[%s]: %s\n", __func__, this->name);

    /*--- Do nothing if the link went down while we are here. ---*/ 
    if (unlikely(mac->dma_stopped)) {
        return;
    }

    mac->dma_stopped = 1;

    for(i = 0; i < avmnet_hw_config_entry->nr_avm_devices; ++i) {
        avmnet_device = avmnet_hw_config_entry->avm_devices[i];

        if(avmnet_device->mac_module != this)
            continue;

        if( ! netif_queue_stopped(avmnet_device->device)){
            AVMNET_DEBUG("{%s} %s stop queue\n", __func__, avmnet_device->device->name);
            netif_tx_lock(avmnet_device->device);
            netif_tx_stop_all_queues(avmnet_device->device);
            netif_tx_unlock(avmnet_device->device);
        } 
        avmnet_device->device->stats.tx_fifo_errors ++;
    }

    mod_timer(&mac->txreaptimer, jiffies + 1);
}


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int athr_gmac_hard_start(struct sk_buff *skb, struct net_device *device) {

    avmnet_netdev_priv_t *net_priv = netdev_priv(device);
    avmnet_device_t    *avm_device = net_priv->avm_dev;
    avmnet_module_t          *this = (avmnet_module_t *)avm_device->mac_module;
    struct athmac_context     *mac = (struct athmac_context *)this->priv;
    unsigned int sent;

#if ! defined(CONFIG_MACH_QCA955x) && ! defined(CONFIG_SOC_QCA955X)
    athr_gmac_desc_t    *ds, *head_ds;
    athr_mac_packet_header_t *header;
#else
    athr_gmac_desc_t    *ds;
#endif

    /* Emulate vlan offloading, a requirement for bridged vlan avm_pa sessions. */
    if (!(skb = vlan_hwaccel_push_inside(skb))) {
        net_err_ratelimited("[%s] failed to insert vlan, dropping skb %p.\n", __func__, skb);
        goto dropit;
    }

    if(skb->len < 16) {
        AVMNET_DEBUG("[%s] skb->len %u < 16. Dropping skb %p.\n", __func__, skb->len, skb);
        goto dropit;
    }

#define AVM_PPE_PAD_TO 64
    /*---  == AVM/CBU 2017-02-14 short packet drop issue == ---*/
    /*---     we need frames >= 64 bytes, otherwise  ---*/
    /*---     we might drop frames in (WLAN -> DSL_WAN_DHCP) scenarios ---*/
    if (unlikely(skb->len < AVM_PPE_PAD_TO)) {
        if (skb_padto(skb, AVM_PPE_PAD_TO)){
            pr_err("[%s] no more memory for padding\n", __func__);
            goto dropit;
        }
        skb->len = AVM_PPE_PAD_TO;
    }

#if (AVMNET_USE_DEBUG_LEVEL_DEBUG != 0)
    {
        athr_gmac_ring_t    *r = &mac->mac_txring[skb->priority];
        printk(KERN_ERR "sent - h %d t %d l %d 0x%x 0x%x 0x%x 0x%x\n", 
                            r->ring_head, 
                            r->ring_tail, 
                            skb->len, 
                            athr_gmac_reg_rd(mac->BaseAddress, ATHR_GMAC_DMA_TX_STATUS), 
                            athr_gmac_reg_rd(mac->BaseAddress, ATHR_GMAC_DMA_TX_CTRL), 
                            athr_gmac_reg_rd(mac->BaseAddress, ATHR_GMAC_DMA_FIFO_DEPTH), 
                            athr_gmac_reg_rd(mac->BaseAddress, ATHR_GMAC_DMA_TXFIFO_THRESH));
    }
#endif

#   if defined(CONFIG_IP_MULTICAST_FASTFORWARD)
    {
        mcfw_portset set;
        mcfw_portset_reset(&set);
        mcfw_portset_port_add(&set, 0);
        (void) mcfw_snoop_send(&net_priv->mcfw_netdrv, set, skb);
    }
#   endif /*--- #if defined(CONFIG_IP_MULTICAST_FASTFORWARD) ---*/

#if ! defined(CONFIG_MACH_QCA955x) && ! defined(CONFIG_SOC_QCA955X)
    switch (mac->mac_unit) {
        case ath_gmac0:
#endif
            ds = athr_gmac_get_one_tx_ds(&mac->mac_txring);
            ds->pskb                 = skb;

            ds->packet.Register      = ATHR_TX_EMPTY;
            ds->packet.Bits.pkt_size = skb->len;
            ds->pkt_start_addr       = virt_to_phys(skb->data);
            dma_cache_wback((unsigned long)(skb->data), skb->len);

            wmb();
            athr_gmac_tx_give_to_dma(ds);       
#if ! defined(CONFIG_MACH_QCA955x) && ! defined(CONFIG_SOC_QCA955X)
            break;
        case ath_gmac1:
            /*------------------------------------------------------------------------------*\
             * DMA Descriptor für den MAC Header und den Atheros Spezial Header (14 Bytes)   1.
             * DMA Descriptor für den Inhalt des Paketes ohne MAC Header (offset 12 Bytes)   2.
            \*------------------------------------------------------------------------------*/
            head_ds = athr_gmac_get_tx_ds(&mac->mac_txring, skb);
            head_ds->packet.Register   = ATHR_TX_EMPTY;
            head_ds->packet.Bits.more  = 1;

            header = (athr_mac_packet_header_t *)head_ds->pkt_start_addr;

            memcpy(header->buff, skb->data, 2 * ETH_ALEN);

            header->s.header.version  = 2;
            header->s.header.priority = skb->priority;
            header->s.header.type     = 0;
            header->s.header.from_cpu = 1;
            header->s.header.portnum  = 1 << (avm_device->vlanID); 
            header->s.data            = *(unsigned short *)(skb->data + 2 * ETH_ALEN);

            head_ds->packet.Bits.pkt_size = 2 * ETH_ALEN + 2 + 2;
            head_ds->pkt_start_addr   = virt_to_phys(header);

            ds                        = head_ds + 1;
            ds->packet.Register       = ATHR_TX_EMPTY;

            ds->packet.Bits.pkt_size  = skb->len - 2 * ETH_ALEN - 2;
            ds->pkt_start_addr        = virt_to_phys((skb->data + 2 * ETH_ALEN + 2));
            dma_cache_wback((unsigned long)(skb->data), skb->len);

            /*--------------------------------------------------------------------------------------*\
             * Bei zweigeteilten Frames zuerst den Zweiten Teil auf DMA Owner setzen und anschließend
             * den ersten. Wenn man das umgekehrt macht, besteht die Gefahr, dass zwischen die beiden
             * 'give_to_dma' aufrufe ein Interrupt kommt und den Vorgang verzögert. Dann könnte der
             * DMA Controller schon mit dem Senden des ersten Teil angefangen haben und findet kein 
             * zweiten Teil. Dadurch wird der Senden des gesamtframe unterbrochen und somit der Frame
             * zerstört.
            \*--------------------------------------------------------------------------------------*/
            wmb();
            athr_gmac_tx_give_to_dma(ds);       
            athr_gmac_tx_give_to_dma(head_ds);
            break;
    }
#endif

    athr_gmac_tx_start(mac->BaseAddress);

    device->trans_start = jiffies;
    /*--- printk(KERN_ERR "{%s} send_queue %d\n", __func__, skb->len); ---*/
    netdev_sent_queue(mac->device, skb->len);
    avm_device->device->stats.tx_packets ++;
    avm_device->device->stats.tx_bytes += skb->len;

    sent = (athr_gmac_reg_rd(mac->BaseAddress, ATHR_GMAC_DMA_TX_STATUS) >> 16) & 0xFF;
    if (sent) {
        mac->tx_reap(mac, sent + mac->txdma_acked);
    }

    if (unlikely(athr_gmac_tx_ring_full(&mac->mac_txring))) {
        athr_gmac_handle_tx_full(this);
    }

    return NETDEV_TX_OK;

dropit:
	if (skb)
		dev_kfree_skb_any(skb);

    return NETDEV_TX_OK;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void athr_gmac_fast_reset(struct athmac_context *mac) {

    avmnet_module_t *this = mac->this_module;
    uint32_t rx_ds, tx_ds;
    unsigned long flags;
    unsigned int isr, irq_mask, resetmask;
    unsigned int w1 = 0, w2 = 0;
    unsigned int unused, sent;

    athr_gmac_ring_t *t = &mac->mac_txring;

    spin_lock_irqsave(&mac->spinlock, flags);

    irq_mask = athr_gmac_get_msr(mac->BaseAddress);
    isr = athr_gmac_get_isr(mac->BaseAddress);

    athr_gmac_tx_stop(mac->BaseAddress);
    athr_gmac_rx_stop(mac->BaseAddress);

    unused = athr_gmac_ndesc_unused(t);
    sent = (athr_gmac_reg_rd(mac->BaseAddress, ATHR_GMAC_DMA_TX_STATUS) >> 16) & 0xFF;
    AVMNET_ERR("<%s> %s irq 0x%x 0x%x unused %d sent %d\n", __func__, this->name, irq_mask, isr, unused, sent);

    mac->tx_reap(mac, sent + mac->txdma_acked);

    /*------------------------------------------------------------------------------------------*\
     * after reset ATHR_GMAC_DMA_TX_STATUS == 0, do not ack too much descriptors on next tx_reap
    \*------------------------------------------------------------------------------------------*/
    mac->txdma_acked = (athr_gmac_reg_rd(mac->BaseAddress, ATHR_GMAC_DMA_TX_STATUS) >> 16) & 0xFF;
    
    AVMNET_ERR("<%s> txdma_acked %d\n", __func__, mac->txdma_acked);

    rx_ds = athr_gmac_reg_rd(mac->BaseAddress, ATHR_GMAC_DMA_RX_DESC);
    tx_ds = athr_gmac_reg_rd(mac->BaseAddress, ATHR_GMAC_DMA_TX_DESC);

    resetmask = athr_gmac_reset_mask(mac->mac_unit);
    
    /*--- put into reset, hold, pull out. ---*/
    ath_reg_rmw_set(ATH_RST_RESET, resetmask);
    udelay(10);
    ath_reg_rmw_clear(ATH_RST_RESET, resetmask);
    udelay(10);

    /*--- athr_init_hwaccels(mac); ---*/
    /*--- athmac_gmac_setup_hw(mac); ---*/
    this->setup_special_hw(this);

    /*--- restore RX/TX- Descriptors ---*/
    athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_DMA_TX_DESC, tx_ds);
    athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_DMA_RX_DESC, rx_ds);

    /*--- set the mac addr ---*/
    addr_to_words(mac->device->dev_addr, w1, w2);
    athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_GE_MAC_ADDR1, w1);
    athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_GE_MAC_ADDR2, w2);
    
    mac->dma_reset = 3;

    /*--- Restore interrupts ---*/
    athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_DMA_INTR_MASK, irq_mask);

    spin_unlock_irqrestore(&mac->spinlock, flags);

    if(this->initdata.mac.flags & AVMNET_CONFIG_FLAG_SWITCHPORT) 
        athmac_gmac_enable_hw(mac);

    athr_gmac_tx_start(mac->BaseAddress);
    athr_gmac_rx_start(mac->BaseAddress);

    mod_timer(&mac->txreaptimer, jiffies + 1);
#if 0
    AVMNET_ERR("<%s> irq 0x%x rx 0x%x\n", __func__, athr_gmac_get_isr(mac->BaseAddress), athr_gmac_reg_rd(mac->BaseAddress, ATHR_GMAC_DMA_RX_STATUS));
    AVMNET_ERR("enable RX-IRQ 0x%x\n", athr_gmac_get_msr(mac->BaseAddress));
#endif
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void athr_gmac_tx_timeout(struct net_device *device) {

    avmnet_device_t *avm_device = ((avmnet_netdev_priv_t *)netdev_priv(device))->avm_dev;
    avmnet_module_t       *this = (avmnet_module_t *)avm_device->mac_module;
    struct athmac_context *mac = (struct athmac_context *)this->priv;

    del_timer(&mac->txreaptimer);
    athr_gmac_fast_reset(mac);
#if defined(CONFIG_ATH79_MACH_AVM_HW238)
    {
        uint16_t bad_net_magic[2] = {0xDEAD, 0x6B1F};
        ath79_set_boot_mdio_regs(2, bad_net_magic);
    }
#endif
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static inline void athr_gmac_set_mac_duplex(struct athmac_context *mac, int fdx) {

    if (fdx) {
#if defined(CONFIG_MACH_QCA955x) || defined(CONFIG_SOC_QCA955X) || defined(CONFIG_MACH_QCA956x) || defined(CONFIG_SOC_QCA956X) || defined(CONFIG_SOC_QCN550X)
        athr_gmac_reg_rmw_set(mac->BaseAddress, ATHR_GMAC_CFG1, ATHR_GMAC_CFG1_TX_FCTL);
#endif
        athr_gmac_reg_rmw_set(mac->BaseAddress, ATHR_GMAC_CFG2, ATHR_GMAC_CFG2_FDX);
    } else {
#if defined(CONFIG_MACH_QCA955x) || defined(CONFIG_SOC_QCA955X) || defined(CONFIG_MACH_QCA956x) || defined(CONFIG_SOC_QCA956X) || defined(CONFIG_SOC_QCN550X)
        athr_gmac_reg_rmw_clear(mac->BaseAddress, ATHR_GMAC_CFG1, ATHR_GMAC_CFG1_TX_FCTL);
#endif
        athr_gmac_reg_rmw_clear(mac->BaseAddress, ATHR_GMAC_CFG2, ATHR_GMAC_CFG2_FDX);
    }
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static inline void athr_gmac_set_mac_speed(struct athmac_context *mac, int speed) {
    
    switch (speed) {
        case AVMNET_LINKSTATUS_SPD_10:
            athr_gmac_reg_rmw_clear(mac->BaseAddress, ATHR_GMAC_IFCTL, ATHR_GMAC_IFCTL_SPEED);
            break;
        case AVMNET_LINKSTATUS_SPD_100:
            athr_gmac_reg_rmw_set(mac->BaseAddress, ATHR_GMAC_IFCTL, ATHR_GMAC_IFCTL_SPEED);
            break;
        case AVMNET_LINKSTATUS_SPD_1000:
        case ATHR_PHY_SPEED_UNKNOWN:
            break;
    }
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static inline void athr_gmac_set_mac_if(struct athmac_context *mac, int speed) {

    athr_gmac_reg_rmw_clear(mac->BaseAddress, ATHR_GMAC_CFG2, (ATHR_GMAC_CFG2_IF_1000| ATHR_GMAC_CFG2_IF_10_100));

    switch (speed) {
        case AVMNET_LINKSTATUS_SPD_10: 
        case AVMNET_LINKSTATUS_SPD_100:
            athr_gmac_reg_rmw_set(mac->BaseAddress, ATHR_GMAC_CFG2, ATHR_GMAC_CFG2_IF_10_100);
            athr_gmac_reg_rmw_clear(mac->BaseAddress,ATHR_GMAC_FIFO_CFG_5, ATHR_GMAC_BYTE_PER_CLK_EN);
            break;
        case AVMNET_LINKSTATUS_SPD_1000:
            athr_gmac_reg_rmw_set(mac->BaseAddress, ATHR_GMAC_CFG2, ATHR_GMAC_CFG2_IF_1000);
            athr_gmac_reg_rmw_set(mac->BaseAddress, ATHR_GMAC_FIFO_CFG_5, ATHR_GMAC_BYTE_PER_CLK_EN);
            break;
        case ATHR_PHY_SPEED_UNKNOWN:
            break;
    }

}

/*------------------------------------------------------------------------------------------*\
 * Several fields need to be programmed based on what the PHY negotiated
 * Ideally we should quiesce everything before touching the pll, but:
 * 1. If its a linkup/linkdown, we dont care about quiescing the traffic.
 * 2. If its a single gigE PHY, this can only happen on lup/ldown.
 * 3. If its a 100Mpbs switch, the link will always remain at 100 (or nothing)
 * 4. If its a gigE switch then the speed should always be set at 1000Mpbs, 
 *    and the switch should provide buffering for slower devices.
 *
 * XXX Only gigE PLL can be changed as a parameter for now. 100/10 is hardcoded.
 * XXX Need defines for them -
 * XXX FIFO settings based on the mode
\*------------------------------------------------------------------------------------------*/
void athr_gmac_set_mac_from_link(avmnet_module_t *this, avmnet_linkstatus_t status) {

    struct athmac_context *mac = (struct athmac_context *)this->priv;

    if (mac->mac_unit == ath_gmac0) {
        AVMNET_DEBUG("[%s] %s : carrier %s %sduplex\n", __func__, this->name, 
                                status.Details.link ? "on":"off", 
                                status.Details.fullduplex ? "full":"half");

        athr_gmac_set_mac_if(mac, status.Details.speed);
        athr_gmac_set_mac_speed(mac, status.Details.speed);
        athr_gmac_set_mac_duplex(mac, status.Details.fullduplex);

        athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_FIFO_CFG_3, 0x1f00140);
        athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_DMA_TXFIFO_THRESH ,0x01d80160);

        switch (status.Details.speed) {
            case AVMNET_LINKSTATUS_SPD_1000: 
                AVMNET_ERR("[%s] AVMNET_LINKSTATUS_SPD_1000\n", __func__);

#if defined(CONFIG_MACH_QCA955x) || defined(CONFIG_SOC_QCA955X) || defined(CONFIG_MACH_QCA956x) || defined(CONFIG_SOC_QCA956X) || defined(CONFIG_SOC_QCN550X)
                if (mac->this_module->initdata.mac.flags & AVMNET_CONFIG_FLAG_SGMII_ENABLE) {
                    ath_reg_wr (
                        ATH_PLL_ETH_SGMII, 
                        ATH_PLL_ETH_SGMII_GIGE_SET     (1) | 
                        ATH_PLL_ETH_SGMII_CLK_SEL_SET  (1) );
                } else {
                    ath_reg_wr (
                        ATH_PLL_ETH_XMII,  
                        ATH_PLL_ETH_XMII_TX_INVERT_SET (1) | 
#if defined(CONFIG_ATH79_MACH_AVM_HW238)
                        ATH_PLL_ETH_XMII_TX_DELAY_SET (1)  | 
                        ATH_PLL_ETH_XMII_RX_DELAY_SET (2)  | 
#endif
                        ATH_PLL_ETH_XMII_GIGE_SET      (1) );
                }
#elif defined(CONFIG_MACH_AR724x) || defined(CONFIG_SOC_AR724X)
                ath_reg_wr (
                    ATH_PLL_ETH_XMII, 
                    ATH_PLL_ETH_XMII_GIGE_SET     (1) | 
                    ATH_PLL_ETH_XMII_TX_DELAY_SET (1) | 
                    ATH_PLL_ETH_XMII_RX_DELAY_SET (1) );
#else
                ath_reg_wr (
                    ATH_PLL_ETH_XMII, 
                    ATH_PLL_ETH_XMII_GIGE_SET (1));
#endif
                break;

            case AVMNET_LINKSTATUS_SPD_100:
                AVMNET_ERR("[%s] AVMNET_LINKSTATUS_SPD_100\n", __func__);

                if (! status.Details.fullduplex) {
                    athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_DMA_TXFIFO_THRESH ,0x00880060);
                    athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_FIFO_CFG_3, 0x00f00040);
                }

#if defined(CONFIG_MACH_QCA955x) || defined(CONFIG_SOC_QCA955X) || defined(CONFIG_MACH_QCA956x) || defined(CONFIG_SOC_QCA956X) || defined(CONFIG_SOC_QCN550X)
                if (mac->this_module->initdata.mac.flags & AVMNET_CONFIG_FLAG_SGMII_ENABLE) {
                    ath_reg_wr (
                        ATH_PLL_ETH_SGMII, 
                        ATH_PLL_ETH_SGMII_PHASE0_COUNT_SET (0x01) |
                        ATH_PLL_ETH_SGMII_PHASE1_COUNT_SET (0x01) );
                } else {
                    ath_reg_wr (
                        ATH_PLL_ETH_XMII,  
                        ATH_PLL_ETH_XMII_TX_INVERT_SET    (1)  |
                        ATH_PLL_ETH_XMII_PHASE0_COUNT_SET (0x01) |
                        ATH_PLL_ETH_XMII_PHASE1_COUNT_SET (0x01) );
                }
#else
                ath_reg_wr (
                    ATH_PLL_ETH_XMII, 
                    ATH_PLL_ETH_XMII_PHASE0_COUNT_SET (0x01) | 
                    ATH_PLL_ETH_XMII_PHASE1_COUNT_SET (0x01) );
#endif
                break;

            case AVMNET_LINKSTATUS_SPD_10:
                AVMNET_ERR("[%s] AVMNET_LINKSTATUS_SPD_10\n", __func__);

                if (! status.Details.fullduplex) {
                    athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_DMA_TXFIFO_THRESH ,0x00880060);
                    athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_FIFO_CFG_3, 0x00f00040);
                }

#if defined(CONFIG_MACH_QCA955x) || defined(CONFIG_SOC_QCA955X) || defined(CONFIG_MACH_QCA956x) || defined(CONFIG_SOC_QCA956X) || defined(CONFIG_SOC_QCN550X)
                if (mac->this_module->initdata.mac.flags & AVMNET_CONFIG_FLAG_SGMII_ENABLE) {
                    ath_reg_wr (
                        ATH_PLL_ETH_SGMII, 
                        ATH_PLL_ETH_SGMII_PHASE0_COUNT_SET (0x13) |
                        ATH_PLL_ETH_SGMII_PHASE1_COUNT_SET (0x13) );
                } else {
                    ath_reg_wr(
                        ATH_PLL_ETH_XMII, 
                        ATH_PLL_ETH_XMII_TX_INVERT_SET      (1) | 
                        ATH_PLL_ETH_XMII_PHASE0_COUNT_SET (0x13) |
                        ATH_PLL_ETH_XMII_PHASE1_COUNT_SET (0x13) );
                }
#else
                ath_reg_wr (
                    ATH_PLL_ETH_XMII, 
                    ATH_PLL_ETH_XMII_PHASE0_COUNT_SET (0x13) | 
                    ATH_PLL_ETH_XMII_PHASE1_COUNT_SET (0x13) );
#endif
                break;

            default:
                AVMNET_ERR("[%s] ATHR_PHY_SPEED_UNKNOWN\n", __func__);
                /*--- assert(0); ---*/
        }
    } 

    AVMNET_DEBUG("[%s]: cfg_1: %#x\n", __func__, athr_gmac_reg_rd(mac->BaseAddress, ATHR_GMAC_FIFO_CFG_1));
    AVMNET_DEBUG("[%s]: cfg_2: %#x\n", __func__, athr_gmac_reg_rd(mac->BaseAddress, ATHR_GMAC_FIFO_CFG_2));
    AVMNET_DEBUG("[%s]: cfg_3: %#x\n", __func__, athr_gmac_reg_rd(mac->BaseAddress, ATHR_GMAC_FIFO_CFG_3));
    AVMNET_DEBUG("[%s]: cfg_4: %#x\n", __func__, athr_gmac_reg_rd(mac->BaseAddress, ATHR_GMAC_FIFO_CFG_4));
    AVMNET_DEBUG("[%s]: cfg_5: %#x\n", __func__, athr_gmac_reg_rd(mac->BaseAddress, ATHR_GMAC_FIFO_CFG_5));
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static void athmac_reg_busy_wait(avmnet_module_t *this) {
    struct athmac_context *mac = (struct athmac_context *)this->priv;
    int rddata;
    unsigned short ii = 0x1000;

    do {
        schedule();
        rddata = athr_gmac_reg_rd(mac->BaseAddressMdio, ATHR_GMAC_MII_MGMT_IND) & 0x7;
    } while (rddata && --ii);

    if (ii == 0) {
        AVMNET_ERR("ERROR:%s:%d transaction failed\n",__func__,__LINE__);
    }
}


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
unsigned int athmac_reg_read(avmnet_module_t *this, unsigned int phyAddr, unsigned int reg) {
    struct athmac_context *mac = (struct athmac_context *)this->priv;
    unsigned int addr = (phyAddr << ATHR_GMAC_ADDR_SHIFT) | reg, val = 0;

    /*--- AVMNET_DEBUG("[%s/%d] unit 0x%x phy_addr 0x%x reg_addr 0x%x\n", __func__, __LINE__, mac->mac_unit, phyAddr, reg); ---*/

    athr_gmac_reg_wr(mac->BaseAddressMdio, ATHR_GMAC_MII_MGMT_ADDRESS, addr);
    athr_gmac_reg_wr(mac->BaseAddressMdio, ATHR_GMAC_MII_MGMT_CMD, ATHR_GMAC_MII_MGMT_CMD_READ);

    athmac_reg_busy_wait(this);

    val = athr_gmac_reg_rd(mac->BaseAddressMdio, ATHR_GMAC_MII_MGMT_STATUS);
    athr_gmac_reg_wr(mac->BaseAddressMdio, ATHR_GMAC_MII_MGMT_CMD, 0x0);

    athmac_reg_busy_wait(this);

    return val;
}


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int athmac_reg_write(avmnet_module_t *this, unsigned int phyAddr, unsigned int reg, unsigned int Value) {
    struct athmac_context *mac = (struct athmac_context *)this->priv;
    unsigned short addr  = (phyAddr << ATHR_GMAC_ADDR_SHIFT) | reg;

    /*--- AVMNET_DEBUG("[%s/%d] unit 0x%x phy_addr 0x%x reg_addr 0x%x value 0x%x\n", __func__, __LINE__, mac->mac_unit, phyAddr, reg, Value); ---*/

    athr_gmac_reg_wr(mac->BaseAddressMdio, ATHR_GMAC_MII_MGMT_ADDRESS, addr);
    athr_gmac_reg_wr(mac->BaseAddressMdio, ATHR_GMAC_MII_MGMT_CTRL, Value);

    athmac_reg_busy_wait(this);

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int athmac_gmac_lock(avmnet_module_t *this) {
    return this->parent->lock(this->parent);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void athmac_gmac_unlock(avmnet_module_t *this) {
    this->parent->unlock(this->parent);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int athmac_gmac_trylock(avmnet_module_t *this) {
    return this->parent->trylock(this->parent);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void athmac_status_changed(avmnet_module_t *this, avmnet_module_t *caller) {

    avmnet_module_t *child = NULL;
    int i;

    /*
     * find out which of our progeny demands our attention
     */
    for(i = 0; i < this->num_children; ++i){
        if(this->children[i] == caller){
            child = this->children[i];
            break;
        }
    }

    if(child == NULL) {
        AVMNET_ERR("[%s] module %s: received status_changed from unknown module.\n", __func__, this->name);
        return;
    }

    /*
     * handle status change
     */

    return;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int athmac_poll(avmnet_module_t *this) {
    int i, result;

    update_rmon_cache(this);

    for(i = 0; i < this->num_children; ++i){
        result = this->children[i]->poll(this->children[i]);
        if(result < 0){
            AVMNET_WARN("Module %s: poll() failed on child %s\n", this->name, this->children[i]->name);
        }
    }
    
    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int athgmac_reinit(avmnet_module_t *this) {
    struct athmac_context *mac = (struct athmac_context *) this->priv;

    athr_gmac_fast_reset(mac);
    return 0;
}

/*------------------------------------------------------------------------------------------*\
tc qdisc add dev eth0 handle 2:0 root tbf latency 10ms mtu 1600 cell_overhead 1 packet_overhead 4 mpu 64 rate 10048kbit burst 32153/64
tc qdisc add dev eth0 handle 2:0 root tbf latency 10ms mtu 1600 cell_overhead 1 packet_overhead 4 mpu 64 rate 104020kbit burst 16000/64
tc qdisc del dev eth0 root
\*------------------------------------------------------------------------------------------*/

#if LINUX_VERSION_CODE < KERNEL_VERSION (3,10,0) 
static void free_argv(char **argv, char **envp __attribute__((unused))) {
	kfree(argv[1]); /* check call_modprobe() */
	kfree(argv);
}

struct subprocess_info *
compat_call_usermodehelper_setup(char *path, char **argv, char **envp, gfp_t gfp_mask,
                                int (*init)(struct subprocess_info *info, struct cred *new),
			                    void (*cleanup)(char **argv, char **envp *), void *data) {
	struct subprocess_info *info = NULL;

    WARN_ON(init != NULL);
    WARN_ON(data != NULL);

	info = call_usermodehelper_setup(path, argv, envp, mask);
	if (!info) {
		goto out;
    }

	call_usermodehelper_setcleanup(info, cleanup);

out:
    return info;
}

#define call_usermodehelper_setup compat_call_usermodehelper_setup
#else
static void free_argv(struct subprocess_info *info) {
	kfree(info->argv[1]); /* check call_modprobe() */
	kfree(info->argv);
}
#endif

int athr_gmac_set_tc(char *device_name, avmnet_linkstatus_t status) {

	static char *envp[] = { "HOME=/", "TERM=linux", "PATH=/sbin:/usr/sbin:/bin:/usr/bin", NULL };
    char *eth_name = kstrdup(device_name, GFP_KERNEL);
	struct subprocess_info *info = NULL;
    char **argv;

    if ( ! eth_name)
        return -ENOMEM;

	argv = kmalloc(sizeof(char *[4]), GFP_KERNEL);
	if ( ! argv)
		goto out;

	argv[0] = "/sbin/tc_set.sh";
	argv[1] = eth_name;
    if (status.Details.speed == AVMNET_LINKSTATUS_SPD_10) {
        argv[2] = "10";
    } else {
        argv[2] = "100";
    }
	argv[3] = NULL;

	info = call_usermodehelper_setup(argv[0], argv, envp, GFP_ATOMIC, NULL, free_argv, NULL);
	if (!info) {
		goto out;
    }

	return call_usermodehelper_exec(info, UMH_WAIT_EXEC | UMH_KILLABLE);

out:
	kfree(eth_name);
	kfree(argv);
	return -ENOMEM;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int athgmac_set_status(avmnet_module_t *this, avmnet_device_t *id, avmnet_linkstatus_t status) {
    struct athmac_context *mac;
    struct net_device *device = id->device;
    unsigned long flags;
    unsigned int setup_link = 0;

    AVMNET_DEBUG("[%s] %s - %s: carrier %s %sduplex\n", __func__, this->name, device->name, 
                status.Details.link ? "on":"off", 
                status.Details.fullduplex ? "full":"half");

    mac = (struct athmac_context *) this->priv;

    if (id->status.Status != status.Status) {   /*--- Interface nur anfassen, wenn sich der Status geändert hat ---*/
        id->status = status;
        avmnet_links_port_update(id);

        if (this->reinit && status.Details.link)    
            this->reinit(this);     /*--- ruft bei Scorpion mit AR8033 bei linkup athr_gmac_fast_reset auf ---*/

        setup_link = 1;
    } 

    if (status.Details.link) {      

        if( ! (this->initdata.mac.flags & AVMNET_CONFIG_FLAG_SWITCHPORT)) {
            if (setup_link || mac->dma_reset) {
                spin_lock_irqsave(&mac->spinlock, flags);
                athr_gmac_set_mac_from_link(this, status);      /*--- set Interface ---*/
                athmac_gmac_enable_hw(mac);
                spin_unlock_irqrestore(&mac->spinlock, flags);
                
                mac->dma_reset = 0;
            }
        } else {
            AVMNET_DEBUG("[%s] %s: Connected to switch, not touching interface config.\n", __func__, this->name);
        }

        if( ! netif_carrier_ok(device)){
            AVMNET_DEBUG("[%s] %s: setting carrier on\n", __func__, device->name);
            netif_carrier_on(device);
#if defined(CONFIG_MACH_QCA956x) || defined(CONFIG_SOC_QCA956X) || defined(CONFIG_SOC_QCN550X)
            if (mac->mac_unit == ath_gmac1) {
                if ( athr_gmac_set_tc(device->name, status)) 
                    AVMNET_ERR("{%s} traffic_shaping failed\n", __func__);
            }
#endif
        }

        if (netif_queue_stopped(device) && ! mac->dma_stopped) {
            AVMNET_DEBUG("[%s] %s: starting tx queues\n", __func__, device->name);
            netif_tx_lock(device);
            netif_tx_wake_all_queues(device);
            netif_tx_unlock(device);
        }
    } else {                        
        /*--- link down ---*/
        if( ! (this->initdata.mac.flags & AVMNET_CONFIG_FLAG_SWITCHPORT)){
            spin_lock_irqsave(&mac->spinlock, flags);
            athr_gmac_intr_disable_tx(mac->BaseAddress);    /*--- disable TX ---*/
            athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_CFG1, 0);      /*--- disable HW ---*/
            spin_unlock_irqrestore(&mac->spinlock, flags);
        }else{
            AVMNET_DEBUG("[%s] %s: Connected to switch, not disabling MAC.\n", __func__, this->name);
        }

        if(netif_carrier_ok(device)) {
            AVMNET_DEBUG("[%s] %s: seting carrier off\n", __func__, device->name);
            netif_carrier_off(device);
        }
        if( ! netif_queue_stopped(device)){
            AVMNET_DEBUG("[%s] %s: stopping tx queues\n", __func__, device->name);
            netif_tx_lock(device);
            netif_tx_stop_all_queues(device);
            netif_tx_unlock(device);
        }
    }

    return 0;
};

/*------------------------------------------------------------------------------------------*\
 * Head is the first slot with a valid buffer. Tail is the last slot replenished. 
 * Tries to refill buffers from tail to head.
\*------------------------------------------------------------------------------------------*/
static int athr_gmac_rx_replenish(struct athmac_context *mac) {

    athr_gmac_ring_t    *r  = &mac->mac_rxring;
    athr_gmac_desc_t    *ds;
    struct sk_buff      *skb;

    AVMNET_DEBUG("[%s] unit %d head %d tail %d\n", __func__, mac->mac_unit, r->ring_head, r->ring_tail);

    while ( r->ring_tail != r->ring_head ) {
        
        skb = athr_gmac_buffer_alloc(); 
        if (unlikely( ! skb)) {
            return 0;
        }

        ds = &r->ring_desc[r->ring_tail];
        athr_gmac_ring_incr(r->ring_nelem, r->ring_tail);

        ds->pskb = skb;

        /* Invalidate cache should be cache aligned */
/*---         dma_cache_wback((unsigned long)skb_shinfo(ds->pskb), sizeof(struct skb_shared_info)); ---*/
        dma_cache_inv((unsigned long)(ds->pskb->data), ATHR_GMAC_RX_BUF_SIZE);

        if (likely(mac->mac_unit == ath_gmac0))
            skb_reserve(ds->pskb, NET_IP_ALIGN + ATHR_GMAC_RX_RESERVE);
		else
			skb_reserve(ds->pskb, ATHR_GMAC_RX_RESERVE);

        ds->packet.Register = 0;
        ds->pkt_start_addr  = virt_to_phys(ds->pskb->data);
		wmb();
        athr_gmac_rx_give_to_dma(ds);
    }

    return 1;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static int athr_gmac_recv_packets(struct athmac_context *mac, int quota) {
    struct net_device *dev = NULL;
#   if defined(CONFIG_IP_MULTICAST_FASTFORWARD)
    avmnet_netdev_priv_t *priv;
#   endif /*--- #if defined(CONFIG_IP_MULTICAST_FASTFORWARD) ---*/
    unsigned short port = GMAC_MAX_ETH_DEVS;
    athr_gmac_ring_t *r = &mac->mac_rxring;
    athr_gmac_desc_t *ds;
    struct sk_buff *skb;
    unsigned int status, more_pkts;
    int start_quota = quota;

process_pkts:
    do {
        // Avoid accessing the descriptor, if no packets are available anyway
        if (unlikely((athr_gmac_reg_rd(mac->BaseAddress, ATHR_GMAC_DMA_RX_STATUS) >> ATHR_GMAC_RX_STATUS_PKTCNT_SHIFT) == 0)) {
            break;
        }

        ds   = &r->ring_desc[r->ring_head];

        prefetch(ds->pskb);
        if (unlikely(athr_gmac_rx_owned_by_dma(ds))) {
            break;
        }

        athr_gmac_ring_incr(r->ring_nelem, r->ring_head);
        athr_gmac_intr_ack_rx(mac->BaseAddress);
        skb = ds->pskb;

        __skb_put(skb, ds->packet.Bits.pkt_size - ETH_FCS_LEN);

        /*------------------------------------------------------------------------------------------*\
         * hier die VLANs auseinanderpflücken und in die Daten ins richtige Device stecken
        \*------------------------------------------------------------------------------------------*/
#if ! defined(CONFIG_MACH_QCA955x) && ! defined(CONFIG_SOC_QCA955X)
        switch(mac->mac_unit) {
            case ath_gmac1:
                {
                    p_athr_special_tx_header_t header;

                    /*
                     * find out which switch port (SW MAC 1-5) this frame was received on
                     * and then remove the special header
                     */
                    header = (p_athr_special_tx_header_t)(skb->data + 12);

#if defined(CONFIG_MACH_AR934x)
                    if (likely(header->version == 2) && (header->type == 0)) {
#else
                    if (likely(header->version == 2)) {
#endif
                        port = header->portnum;
                        memmove(skb->data + 2, skb->data, 12);
                        skb_pull(skb, 2);
                        skb_reset_network_header(skb);
                    } else {
                        AVMNET_ERR("[%s] Received illegal Frame version %d priority %d type %d portnum %d\n", __func__, 
                                header->version, header->priority, header->type, header->portnum);
                        AVMNET_ERR("{%s} packet.Bits.pkt_size 0x%x\n", __func__, ds->packet.Bits.pkt_size);
#if 1
                        {
                            int j; 
                            unsigned char *p = (unsigned char *)skb->data;
                            for (j = 0; j < 64; j++)
                                printk(" %02x", *p++);
                            printk("\n");
                        } 
#endif
                        dev_kfree_skb_any(skb);
                        continue;
                    }
                }
                break;
            case ath_gmac0:
                // Phy on GMAC0 is always port 0
                port = 0;
                break;
        }

        /*
         *  complain loudly and abort if avm_device is not in lookup table.
         */
		if(unlikely((port > (GMAC_MAX_ETH_DEVS - 1)) || (mac->devices_lookup[port] == NULL))) {
            AVMNET_ERR("[%s] Received Frame from illegal port %d gmac %d\n", __func__, port, mac->mac_unit);
            dev_kfree_skb_any(skb);
            continue;
        }
#else
        port = mac->mac_unit;
#endif

        prefetch(mac->devices_lookup[port]->device);
        dev = mac->devices_lookup[port]->device;
        AVMNET_DEBUG("[%s] dev %s\n", __func__, dev->name);
        dev->stats.rx_packets++;
        dev->stats.rx_bytes += skb->len;
        dev->last_rx        = jiffies;
        skb->dev            = dev;
        skb->protocol       = eth_type_trans(skb, dev);
#       if defined(CONFIG_IP_MULTICAST_FASTFORWARD)
        priv = netdev_priv(dev);
        /* No snooping on WAN ports */
        if(!(priv->avm_dev->flags & AVMNET_CONFIG_FLAG_SWITCH_WAN)) {
            int hlen = skb->data - skb_mac_header(skb);
            skb_push(skb, hlen);
            mcfw_snoop_recv(&priv->mcfw_netdrv, 0, skb);
            skb_pull(skb, hlen);
        }
#       endif /*--- #if defined(CONFIG_IP_MULTICAST_FASTFORWARD) ---*/

#ifdef CONFIG_ATHRS_HW_NAT
        athr_nat_process_ingress_pkts(mac->mac_unit, skb, ds);
#endif

        /* Emulate vlan offloading, a requirement for bridged vlan avm_pa sessions. */
        if (vlan_hw_offload_capable(dev->features, skb->protocol)) {
            skb->mac_len = ETH_HLEN; /* must be set for skb_vlan_untag() */
            if (!(skb = skb_vlan_untag(skb))) {
                pr_err("[%s] out of memory during vlan strip\n", __func__);
                continue;
            }
        }

#if defined(CONFIG_AVM_PA)
        if(unlikely(avm_pa_dev_receive(AVM_PA_DEVINFO(dev), skb) != AVM_PA_RX_ACCELERATED)) {
            netif_receive_skb(skb);
        } 
#else
        netif_receive_skb(skb);
#endif /*--- #if defined(CONFIG_AVM_PA) ---*/
    } while (--quota);

	if (unlikely( ! athr_gmac_rx_replenish(mac))) {    /*--- keine Buffer ---*/
		return -1;
    }

    status = athr_gmac_reg_rd(mac->BaseAddress, ATHR_GMAC_DMA_RX_STATUS);
    if (unlikely(status & ATHR_GMAC_RX_STATUS_OVF)) {
        if (likely(dev))
			dev->stats.rx_fifo_errors++;
        athr_gmac_intr_ack_rxovf(mac->BaseAddress);
        athr_gmac_rx_start(mac->BaseAddress);
		if (printk_ratelimit())
			AVMNET_ERR("[%s] RX_OVERRUN\n", __func__);
    }

    /*--- more pkts arrived; if we have quota left, get rolling again ---*/
    more_pkts = (status & ATHR_GMAC_RX_STATUS_PKT_RCVD);
    if (quota && more_pkts)
        goto process_pkts;

    /*--- out of quota ---*/
    AVMNET_DEBUG("[%s] status 0x%x more_pkts 0x%x quota %d RXE %d\n", __func__, status, more_pkts, quota, athr_gmac_reg_rd(mac->BaseAddress, ATHR_GMAC_DMA_RX_CTRL));
    return start_quota - quota;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
#if defined(ATHR_USE_NAPI)
static int athr_gmac_napi_poll(struct napi_struct *napi, int budget) {

    struct athmac_context *mac = (struct athmac_context *)container_of(napi, struct athmac_context, napi);
    unsigned long flags;

    int rcv = athr_gmac_recv_packets(mac, budget);

    /*--- AVMNET_INFO("[%s] device %s ret 0x%x\n", __func__, dev->name, ret); ---*/

    if (unlikely(rcv < 0)) {
        athr_gmac_rx_stop(mac->BaseAddress);    /*--- disable RX ---*/
        napi_complete(&mac->napi);
        mod_timer(&mac->mac_oom_timer, jiffies + 2000);
        return 0;
    } else if (rcv < budget) {
        napi_complete(&mac->napi);
        spin_lock_irqsave(&mac->spinlock, flags);
        athr_gmac_intr_enable_recv(mac->BaseAddress);
        athr_gmac_intr_enable_rxovf(mac->BaseAddress);
        spin_unlock_irqrestore(&mac->spinlock, flags);
    }
	return rcv;
}
#else
static void athr_gmac_poll(unsigned long data) {

    avmnet_module_t *this = (avmnet_module_t *)data;
    struct athmac_context *mac = (struct athmac_context *)this->priv;

    unsigned long flags;
    unsigned int buffers, budget = ATHR_GMAC_NAPI_WEIGHT;
    int rcv;

#if defined(CONFIG_AVM_SIMPLE_PROFILING) 
    avm_simple_profiling_log(avm_profile_data_type_func_begin, (unsigned int)athr_gmac_recv_packets, 
            athr_gmac_reg_rd(mac->BaseAddress, ATHR_GMAC_DMA_RX_STATUS) >> ATHR_GMAC_RX_STATUS_PKTCNT_SHIFT);
#endif
    buffers = athr_gmac_reg_rd(mac->BaseAddress, ATHR_GMAC_DMA_RX_STATUS) >> ATHR_GMAC_RX_STATUS_PKTCNT_SHIFT;
    if (buffers > ATHR_GMAC_NAPI_WEIGHT)
        budget = ATHR_GMAC_NAPI_WEIGHT << 1;

    rcv = athr_gmac_recv_packets(mac, budget);
#if defined(CONFIG_AVM_SIMPLE_PROFILING) 
    avm_simple_profiling_log(avm_profile_data_type_func_end, (unsigned int)athr_gmac_recv_packets, 
            athr_gmac_reg_rd(mac->BaseAddress, ATHR_GMAC_DMA_RX_STATUS) >> ATHR_GMAC_RX_STATUS_PKTCNT_SHIFT);
#endif

    if (unlikely(rcv < 0)) {
        athr_gmac_rx_stop(mac->BaseAddress);    /*--- disable RX ---*/
        mod_timer(&mac->mac_oom_timer, jiffies + 2000);
        return;
    } else if ( rcv < budget) {
        spin_lock_irqsave(&mac->spinlock, flags);
        athr_gmac_intr_enable_recv(mac->BaseAddress);
		/*--- athr_gmac_intr_enable_rxovf(mac->BaseAddress); ---*/
        spin_unlock_irqrestore(&mac->spinlock, flags);
        return;
    }
    tasklet_hi_schedule(&mac->irqtasklet);
}
#endif /*--- #if defined(ATHR_USE_NAPI) ---*/

/*------------------------------------------------------------------------------------------*\
 * Interrupt handling:
 * - Recv NAPI style (refer to Documentation/networking/NAPI)
 *
 *   2 Rx interrupts: RX and Overflow (OVF).
 *   - If we get RX and/or OVF, schedule a poll. Turn off _both_ interurpts. 
 *
 *   - When our poll's called, we
 *     a) Have one or more packets to process and replenish
 *     b) The hardware may have stopped because of an OVF.
 *
 *   - We process and replenish as much as we can. For every rcvd pkt 
 *     indicated up the stack, the head moves. For every such slot that we
 *     replenish with an skb, the tail moves. If head catches up with the tail
 *     we're OOM. When all's done, we consider where we're at:
 *
 *      if no OOM:
 *      - if we're out of quota, let the ints be disabled and poll scheduled.
 *      - If we've processed everything, enable ints and cancel poll.
 *
 *      If OOM:
 *      - Start a timer. Cancel poll. Ints still disabled. 
 *        If the hardware's stopped, no point in restarting yet. 
 *
 *      Note that in general, whether we're OOM or not, we still try to
 *      indicate everything recvd, up.
 *
 * Locking: 
 * The interrupt doesnt touch the ring => Rx is lockless
\*------------------------------------------------------------------------------------------*/
static irqreturn_t athr_gmac_intr(int irq __attribute__((unused)), void *dev) {
    
    avmnet_module_t *this = (avmnet_module_t *)dev;
    struct athmac_context *mac = (struct athmac_context *)this->priv;

    int   isr, handled = 0;

    isr   = athr_gmac_get_isr(mac->BaseAddress);

    if (unlikely(!isr)) {
        spurious_interrupt();
        AVMNET_WARN("[%s]: Spurious interrupt.", this->name);
        return IRQ_HANDLED;
    }

    if (likely(isr & ATHR_GMAC_INTR_RX)) {
        handled = 1;
		athr_gmac_intr_disable_recv(mac->BaseAddress);
#if defined(ATHR_USE_NAPI)
        if (likely(napi_schedule_prep(&mac->napi))) {
            __napi_schedule(&mac->napi);     /*--- napi_schedule ohne Prüfung ---*/
        } 
#else
        tasklet_hi_schedule(&mac->irqtasklet);
#endif
    }
#if 0
    if (likely(isr & ATHR_GMAC_INTR_TX)) {
        handled = 1;
        /*--- AVMNET_DEBUG("[%s] ATHR_GMAC_INTR_TX\n", __func__); ---*/
        athr_gmac_intr_disable_tx(mac->BaseAddress);
        mod_timer(&mac->txreaptimer, jiffies + 1);
    }
#endif
#if 0
    if (unlikely(isr & ATHR_GMAC_INTR_RX_OVF)) {
        handled = 1;
        athr_gmac_intr_disable_rxovf(mac->BaseAddress);
		athr_gmac_intr_disable_recv(mac->BaseAddress);
#if defined(ATHR_USE_NAPI)
        if (likely(napi_schedule_prep(&mac->napi))) {
            __napi_schedule(&mac->napi);     /*--- napi_schedule ohne Prüfung ---*/
        }
#else
        tasklet_hi_schedule(&mac->irqtasklet);
#endif
		if (printk_ratelimit())
			AVMNET_ERR("[%s] RX_OVERRUN\n", __func__);
    }
#endif
    if (unlikely(isr & ATHR_GMAC_INTR_RX_BUS_ERROR)) {
        handled = 1;
        AVMNET_ERR("[%s] ATHR_GMAC_INTR_RX_BUS_ERROR\n", __func__);
        athr_gmac_intr_ack_rxbe(mac->BaseAddress);
    }
    if (unlikely(isr & ATHR_GMAC_INTR_TX_BUS_ERROR)) {
        handled = 1;
        AVMNET_ERR("[%s] ATHR_GMAC_INTR_TX_BUS_ERROR\n", __func__);
        athr_gmac_intr_ack_txbe(mac->BaseAddress);
    }
    if (unlikely( ! handled)) {
        AVMNET_ERR("[%s]: unhandled intr isr %#x\n", __func__, isr);
        BUG_ON(1);
    }

    return IRQ_HANDLED;
}

/*------------------------------------------------------------------------------------------*\
 * Error timers
\*------------------------------------------------------------------------------------------*/
static void athr_gmac_oom_timer(unsigned long data) {

    avmnet_module_t *this = (avmnet_module_t *)data;
    struct athmac_context *mac = (struct athmac_context *)this->priv;

    int count = athr_gmac_rx_replenish(mac);

    AVMNET_ERR("[%s] %s count %d\n", __func__, this->name, count);
    if ( ! count) {  /*--- OOM ---*/
        int val = mod_timer(&mac->mac_oom_timer, jiffies + 2000);
        BUG_ON(val);
    } else {
        athr_gmac_rx_start(mac->BaseAddress);
#if defined(ATHR_USE_NAPI)
        napi_reschedule(&mac->napi);
#else
        tasklet_hi_schedule(&mac->irqtasklet);
#endif
    }
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static void athmac_gmac_setup_mii(struct athmac_context *mac) {
    unsigned int mgmt_cfg_val = 0;

#if ! defined(CONFIG_MACH_AR724x) && ! defined(CONFIG_SOC_AR724X)
    // [GJu] der QCA9563 hat keinen Switch, wird aber identisch behandelt, weil er anhand der 
    //       Revisionsnummer nicht vom QCA9561 zu unterscheiden ist
    if (soc_is_ar934x() || soc_is_qca953x() || soc_is_qca955x() || soc_is_qca956x() || soc_is_qcn550x()) {
        ath_reg_rmw_set (ATH_PLL_SWITCH_CLOCK_CONTROL, (1 << 6));      /*--- MDIO_CLK = 100MHz ---*/
    }
#endif

    if (soc_is_qca955x() || soc_is_qca956x() || soc_is_qcn550x()) {
        mgmt_cfg_val = 0x7;
    } else {
        mgmt_cfg_val = 0x6;
    }

    athr_gmac_reg_wr(mac->BaseAddressMdio, ATHR_GMAC_MII_MGMT_CFG, mgmt_cfg_val | (1 << 31));
    athr_gmac_reg_wr(mac->BaseAddressMdio, ATHR_GMAC_MII_MGMT_CFG, mgmt_cfg_val);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void athr_gmac_setup_eth( struct net_device* device) {

    ether_setup(device);           /*--- setup eth-defaults ---*/

    device->hard_header_len += 48;
    /* needed for vlan offload emulation in transmit path */
    device->needed_headroom = VLAN_HLEN;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_set_macaddr(int device_nr, struct net_device *dev);
void athr_gmac_setup_eth_priv(avmnet_device_t *avm_dev) {

    avmnet_netdev_priv_t *priv;
    struct net_device *device = avm_dev->device;
    
    AVMNET_INFO("[%s] devicename %s module %s\n", __func__, device->name, avm_dev->mac_module->name);

    priv = netdev_priv( device );

    if ( priv ){
        priv->avm_dev = avm_dev;
    } else {
        AVMNET_ERR("[%s] ERROR: %s avm_dev = %p\n", __func__, avm_dev->device_name, avm_dev);
        return;
    }

    if ( ! strncmp("eth0", avm_dev->device_name, sizeof("eth0")-1)) {
        if (avmnet_set_macaddr(0, device)) {
            AVMNET_ERR("[%s] ERROR: %s set mac-Address\n" , __func__, avm_dev->device_name);
            BUG_ON(1);
        }
        {
            /* set the mac addr - Stichwort Pauseframes */
            avmnet_module_t          *this = (avmnet_module_t *)avm_dev->mac_module;
            struct athmac_context     *mac = (struct athmac_context *)this->priv;
            unsigned int w1 = 0, w2 = 0;

            addr_to_words(device->dev_addr, w1, w2);
            athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_GE_MAC_ADDR1, w1);
            athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_GE_MAC_ADDR2, w2);
        }
    }
    if ( ! strncmp("eth1", avm_dev->device_name, sizeof("eth1")-1)) {
        if (avmnet_set_macaddr(1, device)) {
            AVMNET_ERR("[%s] ERROR: %s set mac-Address\n" , __func__, avm_dev->device_name);
            BUG_ON(1);
        }
        {
            /* set the mac addr - Stichwort Pauseframes */
            avmnet_module_t          *this = (avmnet_module_t *)avm_dev->mac_module;
            struct athmac_context     *mac = (struct athmac_context *)this->priv;
            unsigned int w1 = 0, w2 = 0;

            addr_to_words(device->dev_addr, w1, w2);
            athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_GE_MAC_ADDR1, w1);
            athr_gmac_reg_wr(mac->BaseAddress, ATHR_GMAC_GE_MAC_ADDR2, w2);
        }
    }
    if ( ! strncmp("eth2", avm_dev->device_name, sizeof("eth2")-1)) {
        if (avmnet_set_macaddr(2, device)) {
            AVMNET_ERR("[%s] ERROR: %s set mac-Address\n" , __func__, avm_dev->device_name);
            BUG_ON(1);
        }
    }
    if ( ! strncmp("eth3", avm_dev->device_name, sizeof("eth3")-1)) {
        if (avmnet_set_macaddr(3, device)) {
            AVMNET_ERR("[%s] ERROR: %s set mac-Address\n" , __func__, avm_dev->device_name);
            BUG_ON(1);
        }
    }
    if ( ! strncmp("eth4", avm_dev->device_name, sizeof("eth4")-1)) {
        if (avmnet_set_macaddr(4, device)) {
            AVMNET_ERR("[%s] ERROR: %s set mac-Address\n" , __func__, avm_dev->device_name);
            BUG_ON(1);
        }
    }
    if ( ! strncmp("plc", avm_dev->device_name, sizeof("plc")-1)) {
        if (avmnet_set_macaddr(2, device)) {
            AVMNET_ERR("[%s] ERROR: %s set mac-Address\n" , __func__, avm_dev->device_name);
            BUG_ON(1);
        }
    }

    device->watchdog_timeo = TX_TIMEOUT;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int athmac_gmac_setup(avmnet_module_t *this) {

    struct athmac_context *mac = (struct athmac_context *)this->priv;
    avmnet_device_t *avm_dev;
    int i, result = -1;
    
    AVMNET_TRC("[%s] Setup on module %s called.\n", __func__, this->name);
    
    /*--- reset HW ---*/
    athr_gmac_reg_rmw_set(mac->BaseAddress, ATHR_GMAC_CFG1, 
            ATHR_GMAC_CFG1_SOFT_RST | ATHR_GMAC_CFG1_RX_RST | ATHR_GMAC_CFG1_TX_RST);

    /*--- mac->tx_ac_bt = 0; ---*/
    /*--- mac->tx_ac_drops = 0; ---*/

    if (athr_gmac_tx_alloc(mac)) goto tx_failed;
    if (athr_gmac_rx_alloc(mac)) goto rx_failed;

    athmac_gmac_setup_mii(mac);
    
    /*--- athr_init_hwaccels(mac); ---*/
    /*------------------------------------------------------------------------------------------*\
     * muss auf jeden Fall vor dem Setup der PHYs aufgerufen werden!!! Atheos-Phy latchen die Config 
     * beim Reset an den MII Pins, das Interface muss als am Prozessor schon initialisiert sein
    \*------------------------------------------------------------------------------------------*/
    /*--- athmac_gmac_setup_hw(mac); ---*/
    this->setup_special_hw(this);

    if (this->initdata.mac.flags & AVMNET_CONFIG_FLAG_SWITCHPORT)
        athmac_gmac_enable_hw(mac);

    register_netdev( mac->device );

#if defined(ATHR_USE_NAPI)
    netif_napi_add(mac->device , &mac->napi, athr_gmac_napi_poll, ATHR_GMAC_NAPI_WEIGHT);
    netif_carrier_off(mac->device);
    netif_stop_queue(mac->device);
    napi_enable(&mac->napi);
#else
    tasklet_init(&mac->irqtasklet, athr_gmac_poll, (unsigned long)this);
#endif /*--- #if defined(ATHR_USE_NAPI) ---*/

    if (this->initdata.mac.flags & AVMNET_CONFIG_FLAG_IRQ) {
        result = request_irq(mac->irq, athr_gmac_intr, IRQF_TRIGGER_RISING, this->name, this);
        if(result < 0) {
            AVMNET_ERR("[%s] <ERROR: request irq %s - %d failed>\n", __func__, this->name, mac->irq);
            return -EINTR;
        }
    }

    // find avmnet devices associated with this MAC unit and put them in its lookup table
    for(i = 0; i < GMAC_MAX_ETH_DEVS; ++i){
        avm_dev = find_avmnet_device(i);

        if(avm_dev && avm_dev->mac_module == this){
            mac->devices_lookup[i] = avm_dev;
        }
    }

    for(i = 0; i < this->num_children; ++i){
        result = this->children[i]->setup(this->children[i]);
        if(result != 0){
            // handle error
        }
    }

	athr_gmac_intr_enable_recv(mac->BaseAddress);
    athr_gmac_rx_start(mac->BaseAddress);
    return 0;

rx_failed:
    athr_gmac_tx_free(mac);
tx_failed:
    return 1;
}


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int gmac_open(struct net_device *device __attribute__ ((unused))) {
    AVMNET_TRC("[%s] %s\n", __func__, ((struct athmac_context *)netdev_priv(device))->this_module->name);
    return 0;
}


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int gmac_stop(struct net_device *device __attribute__ ((unused))) {
    AVMNET_TRC("[%s] %s\n", __func__, ((struct athmac_context *)netdev_priv(device))->this_module->name);
    return 0;
}


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void gmac_tx_timeout(struct net_device *device) {
    avmnet_module_t *this = ((struct athmac_context *)netdev_priv(device))->this_module;
    AVMNET_WARN("[%s] %s\n", __func__, this->name);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int gmac_hard_start(struct sk_buff *skb, struct net_device *device __attribute__ ((unused))) {
    AVMNET_ERR("[%s] %s\n", __func__, ((struct athmac_context *)netdev_priv(device))->this_module->name);

    dev_kfree_skb_any(skb);
    return 0;
}

struct net_device_ops athmac_device_ops = {
        .ndo_open             = gmac_open,
        .ndo_stop             = gmac_stop,
        .ndo_tx_timeout       = gmac_tx_timeout,
        .ndo_start_xmit       = gmac_hard_start
};

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int athmac_gmac_init(avmnet_module_t *this) {

    struct net_device *device;
    struct athmac_context *mac;
    int i, result;

    AVMNET_INFOTRC("[%s] '%s'\n", __func__, this->name);


#if LINUX_VERSION_CODE < KERNEL_VERSION(3,17,0)
    device = alloc_netdev( sizeof(struct athmac_context), this->name, athr_gmac_setup_eth );
#else
    device = alloc_netdev( sizeof(struct athmac_context), this->name,
                             NET_NAME_PREDICTABLE, athr_gmac_setup_eth );
#endif
    if(!device) {
        AVMNET_ERR("[%s] Could not allocate net_device %s\n", __func__, this->name);
        return -ENOMEM;
    }

    device->netdev_ops = &athmac_device_ops;
    this->priv = netdev_priv( device );

    memset(this->priv, 0, sizeof(struct athmac_context));

    mac = (struct athmac_context *)this->priv;

    mac->device = device;
    mac->this_module = this;
    mac->mac_unit = this->initdata.mac.mac_nr;

    if (this->initdata.mac.flags & AVMNET_CONFIG_FLAG_BASEADDR) {
        mac->BaseAddress = this->initdata.mac.base_addr;
    }

    if (this->initdata.mac.flags & AVMNET_CONFIG_FLAG_BASEADDR_MDIO) {
        mac->BaseAddressMdio = this->initdata.mac.base_mdio;
    } else {
        mac->BaseAddressMdio = mac->BaseAddress;
    }

    if (this->initdata.mac.flags & AVMNET_CONFIG_FLAG_RESET) {
        mac->reset = this->initdata.mac.reset;
        /*--- Resourcen anmelden ---*/
        ath_gmac_gpio.start = mac->reset;
        ath_gmac_gpio.end = mac->reset;
        ath_gmac_gpio.name = this->name;
        if(request_resource(&gpio_resource, &ath_gmac_gpio)) {
            AVMNET_ERR("[%s] ERROR request resource gpio %d\n", __func__, mac->reset);
            kfree(mac);
            return -EIO;
        }
    }

    if (this->initdata.mac.flags & AVMNET_CONFIG_FLAG_IRQ) {
        mac->irq = this->initdata.mac.irq;
    }

    mac->num_tx_desc   = athr_tx_desc_num[mac->mac_unit];
    mac->num_rx_desc   = athr_rx_desc_num[mac->mac_unit];

#if ! defined(CONFIG_MACH_QCA955x) && ! defined(CONFIG_SOC_QCA955X) 
    if (mac->mac_unit == ath_gmac0) {
        mac->qstart_thresh = 5 * TX_MAX_DESC_PER_DS_PKT;
        mac->tx_reap = athr_gmac_tx_one_reap;
    } else {
        mac->qstart_thresh = (5 * TX_MAX_DESC_PER_DS_PKT) << 1;
        mac->tx_reap = athr_gmac_tx_reap;
    }
#else
    mac->qstart_thresh = 5 * TX_MAX_DESC_PER_DS_PKT;
    mac->tx_reap = athr_gmac_tx_one_reap;
#endif
 
    spin_lock_init(&mac->spinlock);

    init_timer(&mac->mac_oom_timer);
    mac->mac_oom_timer.data     = (unsigned long)this;
    mac->mac_oom_timer.function = (void *)athr_gmac_oom_timer;

    init_timer(&mac->txreaptimer);
    mac->txreaptimer.data     = (unsigned long)this;
    mac->txreaptimer.function = (void *)athr_gmac_txreap_timer;

    for(i = 0; i < this->num_children; ++i){
        result = this->children[i]->init(this->children[i]);
        if(result < 0){
            // handle error
        }
    }

    avmnet_cfg_register_module(this);
    avmnet_cfg_add_seq_procentry(this, "rmon_all", &read_rmon_all_fops);

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int athmac_gmac_exit(avmnet_module_t *this) {

    struct athmac_context *mac = (struct athmac_context *)this->priv;
    struct net_device *device = mac->device;
    int i, result;
   
    AVMNET_TRC("[%s] Exit module %s called.\n", __func__, this->name);

    for(i = 0; i < this->num_children; ++i){
        result = this->children[i]->exit(this->children[i]);
        if(result != 0){
            // handle error
        }
    }

    athr_gmac_rx_stop(mac->BaseAddress);
    athr_gmac_tx_stop(mac->BaseAddress);

	athr_gmac_int_disable(mac->BaseAddress);
#if defined(ATHR_USE_NAPI)
    netif_napi_del(&mac->napi);
#else
    tasklet_kill(&mac->irqtasklet);
#endif
    unregister_netdev(device);

    athr_gmac_tx_free(mac);
    athr_gmac_rx_free(mac);

    free_irq(mac->irq, athr_gmac_intr);

    free_netdev(device);   /*--- kein device kein mac ! ---*/

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

void update_rmon_cache(avmnet_module_t *this)
{
	size_t i; 
	struct athmac_context *mac = (struct athmac_context *) this->priv;

	// This may be called for athmac, which is not a real mac
	// and doesn't have a baseaddr from which to read statistics
	if (!(this->initdata.mac.flags & AVMNET_CONFIG_FLAG_BASEADDR)) {
		return;
	}

	for (i=0; i<ATHR_RMON_COUNTERS; i++){
		u32 diff;
		u32 cur_val = athr_gmac_reg_rd(mac->BaseAddress, 0x80 + (i << 2));
		if ( mac->rmon_cache.prev_val[i] > cur_val )
			diff = (1 < ATHR_RMON_BITS) -
				(mac->rmon_cache.prev_val[i]) + cur_val;
		else
			diff = cur_val - (mac->rmon_cache.prev_val[i]);
		mac->rmon_cache.prev_val[i] = cur_val;
		mac->rmon_cache.sum[i] += diff;
	}
}

static inline u64 read_athr_rmon_cache(avmnet_module_t *this, u32 reg)
{
	struct athmac_context *mac = (struct athmac_context *) this->priv;
	return mac->rmon_cache.sum[reg];
}

static int read_rmon_all_show(struct seq_file *seq, void *data __attribute__ ((unused)) ) {

    char *cnt_gmac_names[] = {
              /* 0x080 */    "TR64",
              /* 0x084 */    "TR127",
              /* 0x088 */    "TR255",
              /* 0x08c */    "TR511",
              /* 0x090 */    "TR1K",
              /* 0x094 */    "TRMAX",
              /* 0x098 */    "TRMGV",
              /* 0x09c */    "RxByte",
              /* 0x0a0 */    "RxPacket",
              /* 0x0a4 */    "RxFcsErr",
              /* 0x0a8 */    "RxMulti",
              /* 0x0ac */    "RxBroad",
              /* 0x0B0 */    "RxControl",
              /* 0x0B4 */    "RxPause",
              /* 0x0B8 */    "RxUnkownOPC",
              /* 0x0Bc */    "RxAlignErr",
              /* 0x0C0 */    "RxLenErr",
              /* 0x0C4 */    "RxCodeErr",
              /* 0x0C8 */    "RxSenseErr",
              /* 0x0Cc */    "RxRunt",
              /* 0x0D0 */    "RxTooLong",
              /* 0x0D4 */    "RxFragment",
              /* 0x0D8 */    "RxJabber",
              /* 0x0Dc */    "RxDrop",
              /* 0x0e0 */    "TxByte",
              /* 0x0e4 */    "TxPacket",
              /* 0x0e8 */    "TxMulti",
              /* 0x0ec */    "TxBroad",
              /* 0x0f0 */    "TxPause",
              /* 0x0f4 */    "TxDeverral",
              /* 0x0f8 */    "TxExDeverral",
              /* 0x0fc */    "TxCol",
              /* 0x100 */    "TxMCol",
              /* 0x104 */    "TxLateCol",
              /* 0x108 */    "TxExCol",
              /* 0x10c */    "TxTotalCol",
              /* 0x110 */    "TxPauseHonored",
              /* 0x114 */    "TxDrop",
              /* 0x118 */    "TxJabber",
              /* 0x11c */    "TxFcsErr",
              /* 0x120 */    "TxControl",
              /* 0x124 */    "TxOversize",
              /* 0x128 */    "TxUndersize",
              /* 0x12c */    "TxFragment",
           };

    unsigned int i;
    avmnet_module_t *this;

    this = (avmnet_module_t *) seq->private;
    update_rmon_cache(this);

    for(i = 0; i < sizeof(cnt_gmac_names)/sizeof(*cnt_gmac_names); i++) {
        seq_printf(seq, "  %14s  %10.llu\n", cnt_gmac_names[i], read_athr_rmon_cache(this, i));
    }
    seq_printf(seq, "\n");

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static int read_rmon_all_open(struct inode *inode, struct file *file) {

    return single_open(file, read_rmon_all_show, PDE_DATA(inode));
}

struct avmnet_hw_rmon_counter *create_athr_rmon_cnt(avmnet_device_t *avm_dev)
{
    avmnet_module_t *this;
    struct avmnet_hw_rmon_counter *res;

    if (!avm_dev)
        return NULL;

    this = avm_dev->mac_module;
    if (!this)
        return NULL;

    res = kzalloc(sizeof(struct avmnet_hw_rmon_counter),GFP_KERNEL);
    if (!res)
        return NULL;

    update_rmon_cache(this);

    res->rx_pkts_good = read_athr_rmon_cache(this, 8);
    res->tx_pkts_good = read_athr_rmon_cache(this, 25);

    res->rx_bytes_good = read_athr_rmon_cache(this, 7);  /*TODO: howto handle wrap around?!  */
    res->tx_bytes_good = read_athr_rmon_cache(this, 24); /*TODO: make 64bit! */

    res->rx_pkts_pause = read_athr_rmon_cache(this, 13);
    res->tx_pkts_pause = read_athr_rmon_cache(this, 28);

    res->rx_pkts_dropped = read_athr_rmon_cache(this, 23);
    res->tx_pkts_dropped = read_athr_rmon_cache(this, 37);
    res->rx_bytes_error = 0; /* not available */

    return res;

}

