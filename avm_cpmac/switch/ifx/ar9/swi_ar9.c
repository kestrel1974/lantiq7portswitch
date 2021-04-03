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

/*------------------------------------------------------------------------------------------*\
 * Driver for the AR9's internal switch. Large parts cannibalized from
 * Lantiq's ifxmips_3port_eth_sw.c
 \*------------------------------------------------------------------------------------------*/
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/workqueue.h>
#include <ifx_pmu.h>
#include <ifx_rcu.h>
#include <ifx_dma_core.h>
#include <ifx_gpio.h>
#include <avm/net/ifx_ppa_api.h>

#ifdef CONFIG_AVM_PA
#if __has_include(<avm/pa/pa.h>)
#include <avm/pa/pa.h>
#else
#include <linux/avm_pa.h>
#endif
#if __has_include(<avm/pa/hw.h>)
#include <avm/pa/hw.h>
#else
#include <linux/avm_pa_hw.h>
#endif
#if __has_include(<avm/pa/ifx_multiplexer.h>)
#include <avm/pa/ifx_multiplexer.h>
#else
#include <linux/avm_pa_ifx_multiplexer.h>
#endif
#endif

#include <avmnet_module.h>
#include <avmnet_config.h>
#include <avmnet_debug.h>
#include <avmnet_common.h>
#include "swi_ar9.h"
#include "swi_ar9_reg.h"
#include "../common/swi_ifx_common.h"
#include "../common/ifx_ppe_drv_wrapper.h"
#include "mac_ar9.h"
#include "../../../management/avmnet_links.h"

#if !defined(CONFIG_NAPI_ENABLED) || CONFIG_NAPI_ENABLED == 'n'
#error driver requires NAPI
#endif

#define IFX_TX_TIMEOUT                  (10 * HZ)
#define DMA_TX_BURST_LEN                DMA_BURSTL_4DW
#define DMA_RX_BURST_LEN                DMA_BURSTL_4DW
#define DMA_RX_ALIGNMENT                128
#define TANTOS_NAPI_WEIGHT              64
#define RMON_COUNT_SIZE                 0x28
#define RMON_PORT_SIZE                  0x3
#define MAC_TIMEOUT                     300

#define GPIO_MDIO                       42
#define GPIO_MDC                        43
// #define USE_HW_SETUP 					1

char *rmon_names[] = {
        "Receive Unicast Packet Count Count      :",
        "Receive Broadcast Packet Count          :",
        "Receive Multicast Packet Count          :",
        "Receive CRC Error Packet Count          :",
        "Receive Undersize Good Packet Count     :",
        "Receive Oversize Good Packet Count      :",
        "Receive Undersize Error Packet Count    :",
        "Receive Good Pause Packet Count         :",
        "Receive Oversize Error Packet Count     :",
        "Receive Align Error Packet Count        :",
        "Filtered Packet Count                   :",
        "Receive Size 64 Packet Count            :",
        "Receive Size 65-127 Packet Count        :",
        "Receive Size 128-255 Packet Count       :",
        "Receive Size 256-511 Packet Count       :",
        "Receive Size 512-1023 Packet Size Count :",
        "Receive Size 1024-Max Packet Size Count :",
        "Transmit Unicast Packet Count           :",
        "Transmit Broadcast Packet Count         :",
        "Transmit Multicast Packet Count         :",
        "Transmit Single Collision Count         :",
        "Transmit Multiple Collision Count       :",
        "Transmit Late Collision Count           :",
        "Transmit Excessive Collision Count      :",
        "Transmit Collision Count                :",
        "Transmit Pause Packet Count             :",
        "Transmit Size 64 Packet Count           :",
        "Transmit Size 65-127 Packet Count       :",
        "Transmit Size 128-255 Packet Count      :",
        "Transmit Size 256-511 Packet Count      :",
        "Transmit Size 512-1023 Packet Count     :",
        "Transmit Size 1024-Max Packet Count     :",
        "Transmit Drop Packet Count              :",
        "Receive Dropped Packet Count            :",
        "Receive Good Byte Count (high)          :",
        "Receive Good Byte Count (low)           :",
        "Receive Bad Byte Count (high)           :",
        "Receive Bad Byte Count (low)            :",
        "Transmit Good Byte Count (high)         :",
        "Transmit Good Byte Count (low)          :", };

struct dma_device_info *g_dma_device_ppe = NULL;
EXPORT_SYMBOL(g_dma_device_ppe);

volatile int g_stop_datapath = 0;
EXPORT_SYMBOL( g_stop_datapath );

avmnet_device_t *avm_devices[MAX_ETH_INF]  = { [0 ... (MAX_ETH_INF - 1)] = NULL};

#if defined(NETDEV_PORT_STATS_READ_RMON)
struct net_device_stats port_stats[MAX_ETH_INF];
#endif

struct net_device *g_eth_net_dev[MAX_ETH_INF] = { [0 ... (MAX_ETH_INF - 1)] = NULL};
EXPORT_SYMBOL(g_eth_net_dev);

int num_ar9_eth_devs = 0;
EXPORT_SYMBOL(num_ar9_eth_devs);

DEFINE_PER_CPU(struct softnet_data, g_ar9_eth_queue);
EXPORT_PER_CPU_SYMBOL(g_ar9_eth_queue);

DECLARE_MUTEX(swi_ar9_mutex);
DECLARE_MUTEX(swi_pa_mutex);

static int write_mdio_sched(unsigned int phyAddr, unsigned int regAddr, unsigned int data);
static unsigned short read_mdio_sched(unsigned int phyAddr, unsigned int regAddr);

static int eth_poll(struct napi_struct *napi, int work_to_do);
static int dma_setup_init(void);
static void dma_setup_uninit(void);
static void enable_dma_channel(void);
static void disable_dma_channel(void);
static void ifx_eth_rx(struct net_device *dev, int len, struct sk_buff* skb);
static int sw_dma_buffer_free(unsigned char* dataptr, void* opt);
static unsigned char* sw_dma_buffer_alloc(int len, int* byte_offset, void** opt);
static int sw_dma_intr_handler(struct dma_device_info* dma_dev, int status, int dma_ch_no);
static void switch_hw_receive(struct dma_device_info* dma_dev, int dma_chan_no);
static int process_backlog(struct napi_struct *napi, int quota);
static int clear_mac_table(void);
static int clear_mac_table_port(unsigned int port);
static int del_mac_entry(struct mac_entry entry);
static int add_mac_entry(struct mac_entry entry);
static uint64_t read_port_counter(unsigned int port, unsigned int offset) __attribute__((unused));

struct net_device  dummy_dev;

// functions for proc-files
static int read_flow_control_show(struct seq_file *seq, void *data);
static int read_flow_control_open(struct inode *inode, struct file *file);
static ssize_t write_flow_control(struct file *file, const char *buf, size_t count, loff_t *ppos);
static int read_rmon_show(struct seq_file *seq, void *data);
static int read_rmon_open(struct inode *inode, struct file *file);
static int mac_table_show(struct seq_file *seq, void *data);
static int mac_table_open(struct inode *inode, struct file *file);

static const struct file_operations mac_table_fops =
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 32)
   .owner   = THIS_MODULE,
#endif
   .open    = mac_table_open,
   .read    = seq_read,
   .llseek  = seq_lseek,
   .release = single_release,
};

static const struct file_operations read_flow_control_fops =
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 32)
   .owner   = THIS_MODULE,
#endif
   .open    = read_flow_control_open,
   .read    = seq_read,
   .write   = write_flow_control,
   .llseek  = seq_lseek,
   .release = single_release,
};

static const struct file_operations read_rmon_fops =
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 32)
   .owner   = THIS_MODULE,
#endif
   .open    = read_rmon_open,
   .read    = seq_read,
   .llseek  = seq_lseek,
   .release = single_release,
};

//#define DUMP_PACKET
#if defined(DUMP_PACKET)
/*
 * \brief    dump skb data
 * \param[in] len length of the data buffer
 * \param[in] pData Pointer to data to dump
 *
 * \return void No Value
 */
static inline void dump_buffer(const char *prefix, char *pData, u32 len){
    unsigned int i;
    if(len > 256)
    len = 256;

    printk("[%s] ", prefix);
    for(i = 0; i < len; i++){
        printk("%2.2x ",(u8)(pData[i]));
        if (i % 16 == 15)
        printk("\n");
    }
    printk("\n");
}
#endif

#ifdef CONFIG_AVM_PA

#define PA_MAC_TABLE_SIZE               512

static int avmnet_ar9_pce_add_session(struct avm_pa_session *avm_session);
static int avmnet_ar9_pce_remove_session(struct avm_pa_session *avm_session);
void avmnet_ar9_remove_session_entry(struct avmnet_ar9_pa_session *session);
static int pa_sessions_open(struct inode *inode, struct file *file);
static int pa_sessions_show(struct seq_file *seq, void *data __attribute__ ((unused)) );

static struct avm_hardware_pa_instance avmnet_pce_pa = {
	.features = HW_PA_CAPABILITY_BRIDGING | HW_PA_CAPABILITY_LAN2LAN,
    .add_session = avmnet_ar9_pce_add_session,
    .remove_session = avmnet_ar9_pce_remove_session,
    .name = "avmnet_ar9_pce",
};

// list of established PA sessions
struct list_head pce_sessions;

// table of entries we made into the switch's MAC table
struct mac_entry pa_mac_table[PA_MAC_TABLE_SIZE];

static const struct file_operations pa_sessions_fops =
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 32)
   .owner   = THIS_MODULE,
#endif
   .open    = pa_sessions_open,
   .read    = seq_read,
   .llseek  = seq_lseek,
   .release = single_release,
};

#endif // CONFIG_AVM_PA

/*
 * Functions for manipulating the switch's MAC table. None of these perform any
 * locking, so they should only be called with the main mutex held.
 */

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

/*
 * Delete an entry from the switch's MAC table. Entries are identified by
 * MAC address and FID only.
 */
static int del_mac_entry(struct mac_entry entry)
{
    unsigned int addr_ctl0, addr_ctl1, addr_ctl2;
    unsigned int addr_st2;
    int result;

    // prepare values to be written into the registers
    // erase entry with given MAC address and FID
    addr_ctl0 = entry.addr[5]
              | entry.addr[4] << 8
              | entry.addr[3] << 16
              | entry.addr[2] << 24;

    addr_ctl1 = entry.addr[1]
              | entry.addr[0] << 8
              | ADRTB_REG1_FID(entry.fid);

    addr_ctl2 = ADRTB_CMD_ERASE;

    // wait for engine to be ready
    while((addr_st2 = IFX_REG_R32(IFX_AR9_ETH_ADRTB_ST2)) & ADRTB_REG2_BUSY)
        ;

    // execute request
    IFX_REG_W32(addr_ctl0, IFX_AR9_ETH_ADRTB_CTL0);
    IFX_REG_W32(addr_ctl1, IFX_AR9_ETH_ADRTB_CTL1);
    IFX_REG_W32(addr_ctl2, IFX_AR9_ETH_ADRTB_CTL2);

    while((addr_st2 = IFX_REG_R32(IFX_AR9_ETH_ADRTB_ST2)) & ADRTB_REG2_BUSY)
        ;

    AVMNET_DEBUG("[%s] Removing MAC entry %02x:%02x:%02x:%02x:%02x:%02x FID: %x Result: %x\n", __func__, entry.addr[0], entry.addr[1], entry.addr[2], entry.addr[3], entry.addr[4], entry.addr[5], entry.fid, addr_st2);

    result = (addr_st2 & ADRTB_REG2_RSLT_MASK) >> 28;

    return result;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
/*
 * Clear MAC table from entries pointing to a given port
 */
static int clear_mac_table_port(unsigned int port)
{
    struct mac_entry entry;
    unsigned int addr_ctl0, addr_ctl1;
    unsigned int addr_st0, addr_st1, addr_st2;
    int result __attribute__((unused));

    AVMNET_INFO("[%s] clearing MAC entries for port %d\n", __func__, port);

    // prepare search. Find entries for port
    addr_ctl0 = 0;
    addr_ctl1 = ADRTB_REG1_PMAP_PORT(port);

    do{
        // wait until table search engine is ready
        while(IFX_REG_R32(IFX_AR9_ETH_ADRTB_ST2) & ADRTB_REG2_BUSY)
            ;

        // re-initialize entry pointer
        IFX_REG_W32(ADRTB_CMD_INIT_START, IFX_AR9_ETH_ADRTB_CTL2);

        // wait for completion
        while(IFX_REG_R32(IFX_AR9_ETH_ADRTB_ST2) & ADRTB_REG2_BUSY)
            ;

        // write search criteria and start search
        IFX_REG_W32(addr_ctl0, IFX_AR9_ETH_ADRTB_CTL0);
        IFX_REG_W32(addr_ctl1, IFX_AR9_ETH_ADRTB_CTL1);
        IFX_REG_W32(ADRTB_CMD_SRCH_MAC_PORT, IFX_AR9_ETH_ADRTB_CTL2);

        while((addr_st2 = IFX_REG_R32(IFX_AR9_ETH_ADRTB_ST2)) & ADRTB_REG2_BUSY)
            ;

        // entry found
        if((addr_st2 & ADRTB_REG2_RSLT_MASK) == ADRTB_REG2_RSLT_OK){

            // copy data of found entry into a struct mac_entry
            addr_st0 = IFX_REG_R32(IFX_AR9_ETH_ADRTB_ST0);
            addr_st1 = IFX_REG_R32(IFX_AR9_ETH_ADRTB_ST1);

            entry.addr[5] = addr_st0 & 0xff;
            addr_st0 >>= 8;
            entry.addr[4] = addr_st0 & 0xff;
            addr_st0 >>= 8;
            entry.addr[3] = addr_st0 & 0xff;
            addr_st0 >>= 8;
            entry.addr[2] = addr_st0 & 0xff;

            entry.addr[1] = addr_st1 & 0xff;
            entry.addr[0] = (addr_st1 >> 8) & 0xff;

            entry.fid = (addr_st1 & ADRTB_REG1_FID_MASK) >> 16;

            AVMNET_DEBUG("[%s] Found MAC entry %02x:%02x:%02x:%02x:%02x:%02x FID: %x\n", __func__, entry.addr[0], entry.addr[1], entry.addr[2], entry.addr[3], entry.addr[4], entry.addr[5], entry.fid);

            // delete entry
            result = del_mac_entry(entry);
            AVMNET_DEBUG("[%s] Result of del_mac_entry: %x\n", __func__, result);
        }
    }while((addr_st2 & ADRTB_REG2_RSLT_MASK) == ADRTB_REG2_RSLT_OK);

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
/*
 * Clear the whole MAC table
 */
static int clear_mac_table(void)
{
    int i;

    for(i = 0; i <= MAX_ETH_INF; ++i){
        clear_mac_table_port(i);
    }

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static void dump_eg_pkt_header( cpu_ingress_pkt_header_t * header __attribute__((unused))) {
#if defined(CONFIG_AVMNET_DEBUG) 
	if ( !(g_dbg_datapath & DBG_ENABLE_MASK_DUMP_EG_HEADER))
        return;

	printk(KERN_ERR "[%s] called by %pF\n", __FUNCTION__, __builtin_return_address(0));
	printk(KERN_ERR "[%s] spid=%d\n", __FUNCTION__, header->SPID );
	printk(KERN_ERR "[%s] dpid=%d\n", __FUNCTION__, header->DPID );
	printk(KERN_ERR "[%s] qid=%d\n", __FUNCTION__, header->QID );
	printk(KERN_ERR "[%s] direct=%d\n", __FUNCTION__, header->DIRECT );
	printk(KERN_ERR "[%s]    - res32_27=%d\n", __FUNCTION__, header->res31_27 );
	printk(KERN_ERR "[%s]    - res32_27=%d\n", __FUNCTION__, header->res23_18 );
	printk(KERN_ERR "[%s]    - res32_27=%d\n", __FUNCTION__, header->res15_10 );
	printk(KERN_ERR "[%s]    - res32_27=%d\n", __FUNCTION__, header->res7_1 );
#endif
};

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
/*
 * Add a MAC entry to the switch's MAC table
 */
static int add_mac_entry(struct mac_entry entry)
{
    int result;
    unsigned int addr_ctl0, addr_ctl1, addr_ctl2;
    unsigned int addr_st2;
    unsigned long flags;

    local_irq_save(flags);

    // wait for table engine to become ready
    while(IFX_REG_R32(IFX_AR9_ETH_ADRTB_ST2) & ADRTB_REG2_BUSY)
        ;

    // re-initialize entry search pointer
    IFX_REG_W32(ADRTB_CMD_INIT_START, IFX_AR9_ETH_ADRTB_CTL2);

    // wait for completion
    while(IFX_REG_R32(IFX_AR9_ETH_ADRTB_ST2) & ADRTB_REG2_BUSY)
        ;

    // first check if a matching entry exists
    addr_ctl0 = entry.addr[5]
                | entry.addr[4] << 8
                | entry.addr[3] << 16
                | entry.addr[2] << 24;

    addr_ctl1 = entry.addr[1]
                | entry.addr[0] << 8
                | ADRTB_REG1_FID(entry.fid);

    addr_ctl2 = ADRTB_CMD_SRCH_MAC_FID;

    IFX_REG_W32(addr_ctl0, IFX_AR9_ETH_ADRTB_CTL0);
    IFX_REG_W32(addr_ctl1, IFX_AR9_ETH_ADRTB_CTL1);
    IFX_REG_W32(addr_ctl2, IFX_AR9_ETH_ADRTB_CTL2);

    while((addr_st2 = IFX_REG_R32(IFX_AR9_ETH_ADRTB_ST2)) & ADRTB_REG2_BUSY)
        ;

    if((addr_st2 & ADRTB_REG2_RSLT_MASK) == ADRTB_REG2_RSLT_OK){
        // matching entry found, we will have to replace it
        addr_ctl2 = ADRTB_CMD_REPLACE;
    }else{
        // no entry found, create new one
        addr_ctl2 = ADRTB_CMD_CREATE;
    }

    // TODO: handle table entry timing out between check and replace operation

    addr_ctl1 |= ADRTB_REG1_PMAP(entry.portmap);

    if(entry.timeout == 0){
        // no timeout -> static entry
        addr_ctl2 |= ADRTB_REG2_INFOT;
    }else{
        // somehow the switch seems to erase entries 7 or 8 seconds too early
        entry.timeout += 8;

        if(entry.timeout > MAC_TIMEOUT){
            entry.timeout = MAC_TIMEOUT;
        }

        addr_ctl2 |= (MAC_TIMEOUT - entry.timeout) & 0x7ff;
    }

    AVMNET_DEBUG("[%s] addr_ctl0: 0x%08x, addr_ctl1: 0x%08x, addr_ctl2: 0x%08x\n", __func__, addr_ctl0, addr_ctl1, addr_ctl2);

    while((addr_st2 = IFX_REG_R32(IFX_AR9_ETH_ADRTB_ST2)) & ADRTB_REG2_BUSY)
        ;

    IFX_REG_W32(addr_ctl0, IFX_AR9_ETH_ADRTB_CTL0);
    IFX_REG_W32(addr_ctl1, IFX_AR9_ETH_ADRTB_CTL1);
    IFX_REG_W32(addr_ctl2, IFX_AR9_ETH_ADRTB_CTL2);

    while((addr_st2 = IFX_REG_R32(IFX_AR9_ETH_ADRTB_ST2)) & ADRTB_REG2_BUSY)
        ;

    AVMNET_DEBUG("[%s] Adding MAC entry %02x:%02x:%02x:%02x:%02x:%02x: FID: %x Portmap: %x Result: %x\n", __func__, entry.addr[0], entry.addr[1], entry.addr[2], entry.addr[3], entry.addr[4], entry.addr[5], entry.fid, entry.portmap, addr_st2);

    result = (addr_st2 & ADRTB_REG2_RSLT_MASK) >> 28;

    local_irq_restore(flags);
    return result;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

int avmnet_swi_ar9_init(avmnet_module_t *this)
{
    struct avmnet_swi_ar9_context *my_ctx;
    int i, result;

    PPE_DPLUS_PMU_SETUP(IFX_PMU_DISABLE);
    PPE_TOP_PMU_SETUP(IFX_PMU_DISABLE);
    SWITCH_PMU_SETUP(IFX_PMU_DISABLE);

    AVMNET_INFO("{%s} Init on module %s called.\n", __func__, this->name);

    init_dummy_netdev(&dummy_dev);


    my_ctx = kzalloc(sizeof(struct avmnet_swi_ar9_context), GFP_KERNEL);
    if(my_ctx == NULL){
        AVMNET_ERR("{%s} init of avmnet-module %s failed.\n", __func__, this->name);
        return -ENOMEM;
    }

    this->priv = my_ctx;

    init_MUTEX(&(my_ctx->mutex));

    for_each_possible_cpu(i) {
        struct softnet_data *queue;

        queue = &per_cpu(g_ar9_eth_queue, i);

        skb_queue_head_init(&queue->input_pkt_queue);
        queue->completion_queue = NULL;
        INIT_LIST_HEAD(&queue->poll_list);

        netif_napi_add(&dummy_dev, &queue->backlog, process_backlog, 64);
        napi_enable(&queue->backlog);
    }

    for(i = 0; i < this->num_children; ++i){
        result = this->children[i]->init(this->children[i]);
        if(result < 0){
            // handle error
        }
    }

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

void avmnet_swi_ar9_disable_learning(avmnet_module_t *this __attribute__ ((unused))) {
	unsigned int reg_p0;
	unsigned int reg_p1;

    AVMNET_INFO("[%s] called\n", __func__);

    if(this->lock(this)){
        AVMNET_ERR("[%s] interrupted while trying to acquire lock!\n", __func__);
        return;
    }
    reg_p0 = IFX_REG_R32(IFX_AR9_ETH_CTL(0));
    reg_p1 = IFX_REG_R32(IFX_AR9_ETH_CTL(1));

    reg_p0 |= PX_CTL_LD;
    reg_p1 |= PX_CTL_LD;

    IFX_REG_W32(reg_p0, IFX_AR9_ETH_CTL(0));
    IFX_REG_W32(reg_p1, IFX_AR9_ETH_CTL(1));

    this->unlock(this);
}
EXPORT_SYMBOL(avmnet_swi_ar9_disable_learning);


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_swi_ar9_hw_setup(avmnet_module_t *this __attribute__ ((unused))) {

	unsigned int reg;

    // enable and configure switch
    reg = IFX_REG_R32(IFX_AR9_ETH_GCTL0);
    reg &= ~(GCTL0_MPL_MASK | GCTL0_ATS_MASK);

    reg |= (GCTL0_SE            // enable switch
            | GCTL0_LPE         // enable virtual ports on CPU
            | GCTL0_ATS_300S    // 300 seconds address table entry aging
            | GCTL0_MPL_1522);  // max. frame length

    IFX_REG_W32(reg, IFX_AR9_ETH_GCTL0);

    // TODO: steht so im alten Treiber...
    // RX/TX DLL Bypass ausschalten?
    IFX_REG_W32(0, IFX_RCU_PPE_CONF);

    // Lantiq fix?
    IFX_REG_W32(0x13B, IFX_AR9_ETH_PMAC_RX_IPG);

    /*Queue Mapping */
    IFX_REG_W32( 0xFA41, IFX_AR9_ETH_1P_PRT);
    IFX_REG_W32( 0x11100000, IFX_AR9_ETH_DFSRV_MAP0);
    IFX_REG_W32( 0x22211111, IFX_AR9_ETH_DFSRV_MAP1);
    IFX_REG_W32( 0x30222222, IFX_AR9_ETH_DFSRV_MAP2);
    IFX_REG_W32( 0x00030003, IFX_AR9_ETH_DFSRV_MAP3);

    /** Scheduling Scheme
     **  Strict Priority scheduling is used for all ports and Rate limit is disabled */
    IFX_REG_W32( 0x03E803E8, IFX_AR9_ETH_P0_ECS_Q32_REG);
    IFX_REG_W32( 0x03E803E8, IFX_AR9_ETH_P0_ECS_Q10_REG);
    IFX_REG_W32( 0x03E803E8, IFX_AR9_ETH_P1_ECS_Q32_REG);
    IFX_REG_W32( 0x03E803E8, IFX_AR9_ETH_P1_ECS_Q10_REG);
    IFX_REG_W32( 0x03E803E8, IFX_AR9_ETH_P2_ECS_Q32_REG);
    IFX_REG_W32( 0x03E803E8, IFX_AR9_ETH_P2_ECS_Q10_REG);

    IFX_REG_W32( 0, IFX_AR9_ETH_P0_ECW_Q32_REG);
    IFX_REG_W32( 0, IFX_AR9_ETH_P0_ECW_Q10_REG);
    IFX_REG_W32( 0, IFX_AR9_ETH_P1_ECW_Q32_REG);
    IFX_REG_W32( 0, IFX_AR9_ETH_P1_ECW_Q10_REG);
    IFX_REG_W32( 0, IFX_AR9_ETH_P2_ECW_Q32_REG);
    IFX_REG_W32( 0, IFX_AR9_ETH_P2_ECW_Q10_REG);

    // disable MAC on CPU port, disable learning and VLAN processing
    reg = IFX_REG_R32(IFX_AR9_ETH_CTL(2));
    AVMNET_DEBUG("[%s] IFX_AR9_ETH_CTL old: %#08x\n", __func__, reg);
    reg &= ~(PX_CTL_FLP);
    reg |= (PX_CTL_FLD | PX_CTL_LD | PX_CTL_BYPASS);

    AVMNET_DEBUG("[%s] IFX_AR9_ETH_CTL new: %#08x\n", __func__, reg);
    IFX_REG_W32(reg, IFX_AR9_ETH_CTL(2));

    // configure PMAC special headers for CPU port ingress / egress
    reg = IFX_REG_R32(IFX_AR9_ETH_PMAC_HD_CTL);
    AVMNET_DEBUG("[%s] IFX_AR9_ETH_PMAC_HD_CTL old: %#08x\n", __func__, reg);
    reg |= (PMAC_HD_CTL_RXSH | PMAC_HD_CTL_AS | PMAC_HD_CTL_AC);

    AVMNET_DEBUG("[%s] IFX_AR9_ETH_PMAC_HD_CTL new: %#08x\n", __func__, reg);

    IFX_REG_W32(reg, IFX_AR9_ETH_PMAC_HD_CTL);

    // make CPU port default destination for unicast and multicast,
    // enable broadcasts on ports 0, 1, 2
    reg = IFX_REG_R32(IFX_AR9_ETH_DF_PORTMAP_REG);
    reg &= ~(DF_PORTMAP_UP_MASK | DF_PORTMAP_MP_MASK | DF_PORTMAP_BP_MASK);

    reg |= (DF_PORTMAP_UP(2)
            | DF_PORTMAP_MP(2)
            | DF_PORTMAP_BP(0)
            | DF_PORTMAP_BP(1)
            | DF_PORTMAP_BP(2));

    AVMNET_DEBUG("[%s] IFX_AR9_ETH_DF_PORTMAP_REG: %#08x\n", __func__, reg);
    IFX_REG_W32(reg, IFX_AR9_ETH_DF_PORTMAP_REG);

    // set MDC to 1MHz
    reg = IFX_REG_R32(IFX_AR9_ETH_RGMII_CTL);
    reg &= ~(RGMII_CTL_MCS_MASK);
    reg |= 12 << 24; // 25MHz / ((12 + 1) * 2)

    IFX_REG_W32(reg, IFX_AR9_ETH_RGMII_CTL);

    return 0;
}

EXPORT_SYMBOL(avmnet_swi_ar9_hw_setup);
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

void avmnet_swi_ar9_enable_cpu_mac_port(void){
    unsigned int reg;
    // enable MAC on CPU port
    reg = IFX_REG_R32(IFX_AR9_ETH_CTL(2));
    reg &= ~(PX_CTL_FLD);
    reg |= PX_CTL_FLP;

    AVMNET_DEBUG("[%s] IFX_AR9_ETH_CTL new: %#08x\n", __func__, reg);
    IFX_REG_W32(reg, IFX_AR9_ETH_CTL(2));
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_swi_ar9_enable_dma (void){

    if(g_dma_device_ppe != NULL){
        AVMNET_ERR("[%s] AR9 switch already initialized, aborting.\n", __func__);
        return -EFAULT;
    }

    /*--- ctx = (struct avmnet_swi_ar9_context *) this->priv; ---*/

    /*
     * do cunning setup-stuff
     */

    // set up DMA device
    if(dma_setup_init() != 0){
        AVMNET_ERR("[%s] Failed to set up DMA device\n", __func__);
        return -EFAULT;
    }

    // enable DMA receive channels
    enable_dma_channel();

    PPE_DPLUS_PMU_SETUP(IFX_PMU_ENABLE);
    PPE_TOP_PMU_SETUP(IFX_PMU_ENABLE);
    SWITCH_PMU_SETUP(IFX_PMU_ENABLE);

    /*
     *             !! DO NOT RESET THE SWITCH MODULE !!
     *
     * Resetting the switch module will result in a stuck DMA TX queue
     *
     */

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

int avmnet_swi_ar9_setup(avmnet_module_t *this)
{
    int i, result;
    /*--- struct avmnet_swi_ar9_context *ctx; ---*/
    avmnet_device_t *avm_dev;

    AVMNET_INFO("[%s] Setup on module %s called.\n", __func__, this->name);
    
    result = avmnet_swi_ar9_enable_dma();
    if(result){
        AVMNET_ERR("[%s] avmnet_swi_ar9_enable_dma() failed!\n", __func__);
        return result;
    }

    // reserve pins for MDIO
    if(ifx_gpio_register(IFX_GPIO_MODULE_EXTPHY_MDIO)){
        AVMNET_ERR("[%s]: Could not register GPIO EXTPHY_MDIO\n", __func__);
        return -EFAULT;
    }

    avmnet_swi_ar9_hw_setup( this );

    // create lookup table for avm devices by port
    for(i = 0; i < avmnet_hw_config_entry->nr_avm_devices; i++){
        avm_dev = avmnet_hw_config_entry->avm_devices[i];

        if(avm_dev && avm_dev->mac_module && avm_dev->mac_module->initdata.mac.mac_nr < MAX_ETH_INF){
            avm_devices[avm_dev->mac_module->initdata.mac.mac_nr] = avm_dev;
        }else{
            AVMNET_ERR("[%s] Config error: avmnet device %s: no mac module or mac_nr >= MAX_ETH_INF.\n", __func__, avm_dev?avm_dev->device_name:"no device");
            BUG();
        }
    }

    for(i = 0; i < this->num_children; ++i){
        result = this->children[i]->setup(this->children[i]);
        if(result != 0){
            AVMNET_ERR("[%s] Setup failed on child %s\n", __func__, this->children[i]->name);
        }
    }

    clear_mac_table();

    avmnet_swi_ar9_enable_cpu_mac_port();

    // clear RMON counters
    // wait for register to become available
    while(IFX_REG_R32(IFX_AR9_ETH_RMON_CTL) & RMON_CTL_BUSY)
        ;

    IFX_REG_W32((RMON_CTL_CAC_RST | RMON_CTL_BUSY), IFX_AR9_ETH_RMON_CTL);


    avmnet_cfg_register_module(this);
    avmnet_cfg_add_seq_procentry(this, "flow_control", &read_flow_control_fops);
    avmnet_cfg_add_seq_procentry(this, "rmon_counters", &read_rmon_fops);
    avmnet_cfg_add_seq_procentry(this, "mac_table", &mac_table_fops);


#ifdef CONFIG_AVM_PA
    avm_pa_multiplexer_init();
    INIT_LIST_HEAD(&pce_sessions);

    if ( avmnet_hw_config_entry->nr_avm_devices > 1 ) {
        AVMNET_INFO("[%s] Box has %d Ethernet-Devices: Register pa_pce_session-Manager at AVM_PA\n", __func__, avmnet_hw_config_entry->nr_avm_devices );
        avm_pa_multiplexer_register_instance( &avmnet_pce_pa );
        avmnet_cfg_add_seq_procentry( this, "pa_pce_sessions", &pa_sessions_fops );
    } else {
        AVMNET_INFO("[%s] Box has %d Ethernet-Devices: Don't register pa_pce_session-Manager at AVM_PA\n", __func__, avmnet_hw_config_entry->nr_avm_devices);
    }
#endif

#if 0
    AVMNET_DEBUG("[%s] doing initial poll.\n", __func__);
    for(i = 0; i < this->num_children; ++i){
        result = this->children[i]->poll(this->children[i]);
        if(result < 0){
            // handle error
        }
    }
#endif

    ifx_proc_file_create();

    return 0;
}


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_swi_ar9_exit(avmnet_module_t *this)
{
    struct avmnet_swi_ar9_context *ctx;
    int i, result;

    AVMNET_INFO("{%s} Exit on module %s called.\n", __func__, this->name);

    for(i = 0; i < this->num_children; ++i){
        result = this->children[i]->exit(this->children[i]);
        if(result != 0){
            // handle error
        }
    }

    /*
     * clean up our mess
     */

    // stop DMA channels and unregister from DMA core
    disable_dma_channel();
    dma_setup_uninit();

    ctx = (struct avmnet_swi_ar9_context *) this->priv;

    this->priv = NULL;
    kfree(ctx);

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_swi_ar9_setup_irq(avmnet_module_t *this_modul __attribute__ ((unused)),
                                unsigned int on __attribute__ ((unused)))
{
    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
unsigned int avmnet_swi_ar9_reg_read(avmnet_module_t *this __attribute__ ((unused)),
                                        unsigned int addr, unsigned int reg)
{
    unsigned int data = 0xFFFF;

    if(in_interrupt()){
        AVMNET_ERR("[%s] in_interrupt()! Abort.\n", __func__);
        dump_stack();
    } 

    data = read_mdio_sched(addr, reg);
    return data;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_swi_ar9_reg_write(avmnet_module_t *this __attribute__ ((unused)),
                                unsigned int addr, unsigned int reg, unsigned int val)
{

    if(in_interrupt()){
        AVMNET_ERR("[%s] in_interrupt()! Abort.\n", __func__);
        dump_stack();
        return -EFAULT;
    }

    write_mdio_sched(addr, reg, val);
    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_swi_ar9_lock(avmnet_module_t *this __attribute__ ((unused)) )
{
    if(in_interrupt()){
        AVMNET_ERR("[%s] in_interrupt()! Abort.\n", __func__);
        dump_stack();
        return -EFAULT;
    }

    return down_interruptible(&(swi_ar9_mutex));
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void avmnet_swi_ar9_unlock(avmnet_module_t *this __attribute__ ((unused)) )
{
    up(&(swi_ar9_mutex));
}

int avmnet_swi_ar9_trylock(avmnet_module_t *this __attribute__ ((unused)) )
{
    return down_trylock(&(swi_ar9_mutex));
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void avmnet_swi_ar9_status_changed(avmnet_module_t *this, avmnet_module_t *caller)
{
    int i;
    avmnet_module_t *child = NULL;

    /*
     * find out which of our progeny demands our attention
     */
    for(i = 0; i < this->num_children; ++i){
        if(this->children[i] == caller){
            child = this->children[i];
            break;
        }
    }

    if(child == NULL){
        AVMNET_ERR("{%s}: module %s: received status_changed from unknown module.\n",
                        __func__, this->name);
        return;
    }

    /*
     * handle status change
     */

    return;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_swi_ar9_poll(avmnet_module_t *this)
{
    int i, result;
    /*--- struct avmnet_swi_ar9_context *ctx = (struct avmnet_swi_ar9_context *) this->priv; ---*/

#ifdef CONFIG_AVM_PA
    // clear any session that has been marked for deletion
    if(down_trylock(&swi_pa_mutex) == 0){
    	struct avmnet_ar9_pa_session *session, *next;

    	list_for_each_entry_safe(session, next, &pce_sessions, list){
			if(session->removed){
				AVMNET_INFO("[%s] Found removed PA session %d, cleaning up.\n", __func__, session->session_id);
				avmnet_ar9_remove_session_entry(session);
			}
    	}
        up(&swi_pa_mutex);
    }
#endif // CONFIG_AVM_PA

    for(i = 0; i < this->num_children; ++i){
        if(this->children[i]->poll){
            result = this->children[i]->poll(this->children[i]);
            if(result != 0){
                AVMNET_ERR("Module %s: poll() failed on child %s\n",
                                this->name, this->children[i]->name);
            }
        }
    }

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_swi_ar9_set_status(avmnet_module_t *this __attribute__ ((unused)),
        avmnet_device_t *device_id, avmnet_linkstatus_t status)
{
    avmnet_linkstatus_t old;
    struct avmnet_swi_ar9_context *ctx;

    ctx = (struct avmnet_swi_ar9_context *) this->priv;

    if(device_id != NULL){
        old = device_id->status;

        if(old.Status != status.Status){
            AVMNET_INFO("[%s] status change for device %s\n", __func__, device_id->device_name);
            AVMNET_INFO("[%s] old status: powerup: %x link %x flowctrl: %x duplex: %x speed: %x\n", __func__, old.Details.powerup, old.Details.link, old.Details.flowcontrol, old.Details.fullduplex, old.Details.speed);
            AVMNET_INFO("[%s] new status: powerup: %x link %x flowctrl: %x duplex: %x speed: %x\n", __func__, status.Details.powerup, status.Details.link, status.Details.flowcontrol, status.Details.fullduplex, status.Details.speed);

            device_id->status = status;
            avmnet_links_port_update(device_id);
        }

        if(device_id->device != NULL){
            if(status.Details.link){
                if(!netif_carrier_ok(device_id->device)){
                    netif_carrier_on(device_id->device);
                }

                /*
                 * only restart tx queues if the global stop flag is not set and the module
                 * has not been suspended.
                 * If the mutex is locked, just skip this part and wait for the next link
                 * status report.
                 */

                if(!down_trylock(&(ctx->mutex))){
                    if(   netif_queue_stopped(device_id->device)
                       && g_stop_datapath == 0
                       && ctx->suspend_cnt == 0)
                    {
                        AVMNET_DEBUG("{%s} %s starting queues\n", __func__, device_id->device->name);
                        netif_tx_wake_all_queues(device_id->device);
                    }

                    up(&(ctx->mutex));
                }
            }else{
                /*
                 * no link. Stop tx queues, update carrier flag and clear MAC table for this
                 * port.
                 */
                if(netif_carrier_ok(device_id->device)){
                    netif_carrier_off(device_id->device);
                    clear_mac_table_port(device_id->mac_module->initdata.mac.mac_nr);
                }

                if(!netif_queue_stopped(device_id->device)){
                    AVMNET_DEBUG("{%s} %s stopping queues\n", __func__, device_id->device->name);
                    netif_tx_stop_all_queues(device_id->device);
                }
            }
        }
    }else{
        AVMNET_ERR("[%s] received status update for invalid device id: %s\n", __func__, device_id->device_name);
        return -EINVAL;
    }

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_swi_ar9_powerdown(avmnet_module_t *this __attribute__ ((unused)))
{
    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_swi_ar9_powerup(avmnet_module_t *this __attribute__ ((unused)))
{
    return 0;
}

static uint64_t read_port_counter(unsigned int port, unsigned int offset)
{
    uint64_t value;

    offset &= RMON_CTL_OFFSET_MASK;
    port = (port << 6) & RMON_CTL_PORTC_MASK;

    // wait for register to become available
    while(IFX_REG_R32(IFX_AR9_ETH_RMON_CTL) & RMON_CTL_BUSY)
        ;

    IFX_REG_W32((RMON_CTL_BUSY | port | offset), IFX_AR9_ETH_RMON_CTL);

    // wait for command to finish
    while(IFX_REG_R32(IFX_AR9_ETH_RMON_CTL) & RMON_CTL_BUSY)
        ;

    value = IFX_REG_R32(IFX_AR9_ETH_RMON_ST);

    // 64 bit register?
    if(offset >= 0x22){
        ++offset;
        value <<= 32;

        IFX_REG_W32((RMON_CTL_BUSY | port | offset), IFX_AR9_ETH_RMON_CTL);

        // wait for command to finish
        while(IFX_REG_R32(IFX_AR9_ETH_RMON_CTL) & RMON_CTL_BUSY)
            ;

        value |= IFX_REG_R32(IFX_AR9_ETH_RMON_ST);

    }

    return value;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

#if defined(NETDEV_PORT_STATS_READ_RMON)
static void update_device_stats(unsigned int port)
{

#if 0
    if(in_interrupt()){
        AVMNET_ERR("[%s] in_interrupt()! Abort.\n", __func__);
        dump_stack();
        return;
    }
#endif


    port_stats[port].rx_packets = read_port_counter(port, 0x00);
    port_stats[port].tx_packets = read_port_counter(port, 0x11);
    port_stats[port].rx_bytes = read_port_counter(port, 0x22);
    port_stats[port].tx_bytes = read_port_counter(port, 0x26);

    port_stats[port].rx_crc_errors = read_port_counter(port, 0x03);
    port_stats[port].rx_errors =  port_stats[port].rx_crc_errors;
    port_stats[port].rx_errors += read_port_counter(port, 0x06);
    port_stats[port].rx_errors += read_port_counter(port, 0x08);
    port_stats[port].rx_errors += read_port_counter(port, 0x09);

    port_stats[port].rx_dropped = read_port_counter(port, 0x21);
    port_stats[port].tx_dropped = read_port_counter(port, 0x20);

    port_stats[port].multicast = read_port_counter(port, 0x02);
    port_stats[port].collisions = read_port_counter(port, 0x18);

}
#endif

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

#if defined(NETDEV_PORT_STATS_READ_RMON)
struct net_device_stats *avmnet_swi_ar9_get_net_device_stats(struct net_device *dev)
{
    avmnet_device_t **avm_devp = netdev_priv(dev);
    unsigned int mac_nr = 0;

    if(down_trylock(&swi_ar9_mutex)){
        AVMNET_INFO("[%s] semaphore locked, deferring stats update\n", __func__);
    } else {
        if((*avm_devp)->mac_module != NULL){
            mac_nr = (*avm_devp)->mac_module->initdata.mac.mac_nr;

            // read stats from switch counters
            update_device_stats(mac_nr);

            // add errors counted here
            port_stats[mac_nr].rx_dropped += (*avm_devp)->device_stats.rx_dropped;
            port_stats[mac_nr].rx_errors += (*avm_devp)->device_stats.rx_errors;
            port_stats[mac_nr].tx_dropped += (*avm_devp)->device_stats.tx_dropped;
            port_stats[mac_nr].tx_errors += (*avm_devp)->device_stats.tx_errors;
        } else {
            AVMNET_ERR("[%s] No MAC-module for device %s\n", __func__, dev->name);
        }

        up(&swi_ar9_mutex);
    }

    // FIXME: will return stats for port 0 if no MAC module is found
    return &(port_stats[mac_nr]);
}
#else

struct net_device_stats *avmnet_swi_ar9_get_net_device_stats(struct net_device *dev){
    avmnet_device_t **avm_devp = netdev_priv(dev);
    return &(*avm_devp)->device_stats;
}
#endif

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static int write_mdio_sched(unsigned int phyAddr, unsigned int regAddr, unsigned int data)
{
    unsigned int reg;

    if(unlikely(in_interrupt())){
        dump_stack();
        return -EFAULT;
    }

    do{
        schedule();
        reg = IFX_REG_R32(IFX_AR9_ETH_MDIO_CTL);
    }while(reg & MDIO_CTL_MBUSY);

    reg = (MDIO_CTL_WD(data)
            | MDIO_CTL_OP_WRITE
            | MDIO_CTL_MBUSY
            | MDIO_CTL_PHYAD_SET(phyAddr)
            | MDIO_CTL_REGAD_SET(regAddr));

    IFX_REG_W32(reg, IFX_AR9_ETH_MDIO_CTL);

    do{
        schedule();
        reg = IFX_REG_R32(IFX_AR9_ETH_MDIO_CTL);
    }while(reg & MDIO_CTL_MBUSY);

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static unsigned short read_mdio_sched(unsigned int phyAddr, unsigned int regAddr)
{
    unsigned int reg;
    unsigned short data;

    if(unlikely(in_interrupt())){
        dump_stack();
        return 0xffff; // simulate read error
    }

    do{
        schedule();
        reg = IFX_REG_R32(IFX_AR9_ETH_MDIO_CTL);
    }while(reg & MDIO_CTL_MBUSY);

    reg = MDIO_CTL_OP_READ
            | MDIO_CTL_MBUSY
            | MDIO_CTL_PHYAD_SET(phyAddr)
            | MDIO_CTL_REGAD_SET(regAddr);

    IFX_REG_W32(reg, IFX_AR9_ETH_MDIO_CTL);

    do{
        schedule();
        reg = IFX_REG_R32(IFX_AR9_ETH_MDIO_CTL);
    }while(reg & MDIO_CTL_MBUSY);

    data = (IFX_REG_R32(IFX_AR9_ETH_MDIO_DATA) & MDIO_CTL_RD_MASK);

    return data;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_swi_ar9_ioctl(struct net_device *dev __attribute__ ((unused)),
        struct ifreq *ifr __attribute__ ((unused)), int cmd __attribute__ ((unused)))
{
    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_swi_ar9_set_mac_address(struct net_device *dev __attribute__ ((unused)),
        void *p __attribute__ ((unused)))
{
    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_swi_ar9_eth_init(struct net_device *dev)
{
    avmnet_netdev_priv_t *priv = NULL;

    AVMNET_INFO("[%s] Called for device %s\n", __func__, dev->name);

    priv = (avmnet_netdev_priv_t *) netdev_priv(dev);

    if(priv == NULL){
        AVMNET_ERR("[%s] No private data in netdev %s\n", __func__, dev->name);
        return -EFAULT;
    }

    if(priv->avm_dev == NULL){
        AVMNET_ERR("[%s] No avmnet_device in netdev %s\n", __func__, dev->name);
        return -EFAULT;
    }

    ether_setup(dev);

    if(avmnet_set_macaddr(priv->avm_dev->external_port_no, dev) < 0){
        AVMNET_ERR("[%s] Setting MAC address for device %s failed\n", __func__, dev->name);
        return -EFAULT;
    }

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void avmnet_swi_ar9_setup_eth(struct net_device *dev __attribute__ ((unused)) )
{
	if (num_ar9_eth_devs < MAX_ETH_INF) {

		g_eth_net_dev[num_ar9_eth_devs++] = dev;

	} else {

		AVMNET_ERR("ERROR:setup_dev_nr=%d  number supported devices by driver =%d\n", num_ar9_eth_devs, MAX_ETH_INF);
	}

    AVMNET_INFO("[%s] Called for device %p (no=%d)\n", __func__, dev, num_ar9_eth_devs);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void avmnet_swi_ar9_setup_eth_priv(avmnet_device_t *avm_dev)
{
    avmnet_netdev_priv_t *priv = NULL;
    struct net_device *dev = avm_dev->device;

    AVMNET_INFO("[%s] Called for avm_device %s\n", __func__, avm_dev->device_name);

    priv = netdev_priv(dev);

    if(priv){
		signed char i;
		priv->lantiq_device_index = -1;
        priv->avm_dev = avm_dev;
		for (i = 0; i < num_ar9_eth_devs; i++)
			if (g_eth_net_dev[i] == dev)
				priv->lantiq_device_index = i;
    }else{
        AVMNET_ERR("[%s] ERROR: priv = %p  avm_dev =%p\n", __FUNCTION__, priv, avm_dev);
    }

    // FIXME: is this still necessary?
    netif_napi_add(dev, &(avm_dev->napi), eth_poll, 64);

    return;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
/*
 * This function is called in dma intr handler (DMA RCV_INT interrupt).
 * This function gets the data from the DMA device.
 * if the packet is valid then it will send to upper layer based on  criteria.
 * The switch CPU port PMAC status header is enabled, then remove the header and
 * look from which port the packet comes and send to relative network device.
 * if PMAC status header is not enabled, then send the packets eth0 interafce
 */
static void switch_hw_receive(struct dma_device_info* dma_dev, int dma_chan_no)
{
    unsigned long flags;
    unsigned char* buf = NULL;
    struct sk_buff *skb = NULL;
    struct softnet_data *queue;
    int len = 0;

    len = dma_device_read(dma_dev, &buf, (void**) &skb, dma_chan_no);

    if(unlikely(skb == NULL)){
        AVMNET_ERR("[%s]: Can't restore the Packet !!!\n", __func__);
        return;
    }

    if(unlikely(g_stop_datapath)){
        AVMNET_TRC("[%s]: datapath to be stopped, dropping skb.\n", __func__);
        dev_kfree_skb_any(skb);
        return;
    }

    if(unlikely((len >= 0x600) || (len < 64))){
        AVMNET_ERR("[%s]: Packet is too large/small (%d)!!!\n", __func__, len);
        dev_kfree_skb_any(skb);
        return;
    }

    /* remove CRC */
    len -= 4;
    if(unlikely(len > (skb->end - skb->tail))){
        AVMNET_ERR("[%s]: len:%d end:%p tail:%p Err!!!\n", __func__, (len + 4), skb->end, skb->tail);
        return;
    }

    skb_put(skb, len);

#ifdef DUMP_PACKET
        dump_buffer(__FUNCTION__, skb->data, skb->len);
#endif

    local_irq_save(flags);
    queue = &__get_cpu_var(g_ar9_eth_queue);

    if(likely(queue->input_pkt_queue.qlen <= 1000)){
        if(unlikely(queue->input_pkt_queue.qlen == 0)){
            napi_schedule(&queue->backlog);
        }

        __skb_queue_tail(&queue->input_pkt_queue, skb);
    } else {
        dev_kfree_skb_any(skb);
    }

    local_irq_restore(flags);
}

static int process_backlog(struct napi_struct *napi, int quota)
{
    cpu_egress_pkt_header_t eg_pkt_hdr;
    int sourcePortId;
    struct sk_buff *skb = NULL;
    struct net_device *dev;
    avmnet_device_t *avm_dev;
    int work = 0;
    struct softnet_data *queue = &__get_cpu_var(g_ar9_eth_queue);
    unsigned long start_time = jiffies;
    unsigned long flags;

    napi->weight = 64;
    do {
        local_irq_save(flags);
        skb = __skb_dequeue(&queue->input_pkt_queue);
        if (!skb) {
            __napi_complete(napi);
            local_irq_restore(flags);
            break;
        }
        local_irq_restore(flags);

        eg_pkt_hdr = *((cpu_egress_pkt_header_t *) skb->data);
        sourcePortId = (eg_pkt_hdr.srcPortID) & 0x7;

        if(likely(sourcePortId < MAX_ETH_INF)){
            avm_dev = avm_devices[sourcePortId];
            dev = avm_dev->device;

            /*Remove PMAC to DMA header */
            skb_pull(skb, 8);

            /* Strip potential padding */
//            __skb_trim(skb, len);
            skb->ip_summed = CHECKSUM_NONE;

            skb->dev = dev;
#if defined(CONFIG_IP_MULTICAST_FASTFORWARD)
            {
                avmnet_netdev_priv_t *priv = netdev_priv(dev);

                mcfw_portset set;
                mcfw_portset_reset(&set);
                mcfw_portset_port_add(&set, 0);
                (void) mcfw_snoop_recv(&priv->mcfw_netdrv, set, skb);
            }
#endif /*--- #if defined(CONFIG_IP_MULTICAST_FASTFORWARD) ---*/
            skb->protocol = eth_type_trans(skb, dev);

            ifx_eth_rx(dev, skb->len, skb);
        }else{
            AVMNET_ERR("[%s] Received packet with invalid source port ID: %d\n", __func__, sourcePortId);
            dev_kfree_skb_any(skb);
        }
    } while (++work < quota && jiffies == start_time);

    return work;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
/* Trasmit timeout */
void avmnet_swi_ar9_tx_timeout(struct net_device *dev)
{
    struct dma_device_info* dma_dev = g_dma_device_ppe;
    avmnet_netdev_priv_t *eth_priv;
    unsigned int i;

    AVMNET_ERR("[%s] Transmit timed out on device %s, disabling the dma channel irq\n", __func__, dev->name);

    eth_priv = (avmnet_netdev_priv_t *) netdev_priv(dev);
    if(eth_priv == NULL){
        AVMNET_ERR("[%s] net_device %s contains no priv.\n", __func__, dev->name);
        return;
    }

    eth_priv->avm_dev->device_stats.tx_errors++;

    for(i = 0; i < dma_dev->num_tx_chan; i++){
        dma_dev->tx_chan[i]->disable_irq(dma_dev->tx_chan[i]);
    }

    netif_wake_queue(dev);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
/**
 * Transmit packet over DMA, which comes from the Tx Queue
 * Note: Tx packet based on the interface queue.
 *       if packet comes from eth0, then sendout the packet over Tx DMA channel 0
 *       if packet comes from eth1, then sendout the packet over Tx DMA channel 1
 * refer the function "ifx_select_tx_chan" selection of dma channels
 * if switch CPU port PMAC status header is enabled, then set the status header
 * based on criteria and push the status header infront of header.
 * if head room is not availabe for status header(4-bytes), reallocate the head room
 * and push status header  infront of the header
 */
int avmnet_swi_ar9_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
    int len;
    char *data;
    cpu_ingress_pkt_header_t ig_pkt_hdr;
    struct dma_device_info* dma_dev = g_dma_device_ppe;
    avmnet_netdev_priv_t *eth_priv = NULL;
    int mac_nr, dma_ch_no;

    if(skb == NULL){
        AVMNET_ERR("[%s]: Empty skb given!\n", __func__);
        return -EFAULT;
    }

    if( g_stop_datapath )
	    goto ETH_XMIT_DROP;

    len = skb->len;
    if (unlikely(skb->len < ETH_ZLEN )){
	    if (skb_pad(skb, ETH_ZLEN - skb->len )){
		    skb = NULL;
		    goto ETH_PAD_DROP;
	    }
	    len = ETH_ZLEN;
    }

    // get avm_device and find out the device's mac_nr
    eth_priv = (avmnet_netdev_priv_t *) netdev_priv(dev);
    if(eth_priv == NULL){
        AVMNET_ERR("[%s] net_device %s contains no priv.\n", __func__, dev->name);
        return -EFAULT;
    }

    mac_nr = eth_priv->avm_dev->mac_module->initdata.mac.mac_nr;
    if(mac_nr > MAX_ETH_INF){
        AVMNET_ERR("[%s] Configuration error in module %s: mac_nr %#x > MAX_ETH_INF\n",
                       __func__, eth_priv->avm_dev->mac_module->name, mac_nr);
        BUG();
    }


#ifdef DUMP_PACKET
    AVMNET_ERR("[%s] skb->len: %d\n", __func__, skb->len);
    dump_buffer("start_xmit-Pre",skb->data, skb->len);
#endif

#   if defined(CONFIG_IP_MULTICAST_FASTFORWARD)
    {
        avmnet_netdev_priv_t *priv = netdev_priv(dev);

        mcfw_portset set;
        mcfw_portset_reset(&set);
        mcfw_portset_port_add(&set, 0);
        (void) mcfw_snoop_send(&priv->mcfw_netdrv, set, skb);
    }
#   endif /*--- #if defined(CONFIG_IP_MULTICAST_FASTFORWARD) ---*/

    memset((void *) &ig_pkt_hdr, 0, sizeof (ig_pkt_hdr));

    ig_pkt_hdr.SPID = 2;        /* source port CPU port */
    ig_pkt_hdr.DPID = mac_nr;   /* destination port */
    ig_pkt_hdr.DIRECT = 1;      /* send direct to port, disable switching */

    if(skb_headroom(skb) >= 4){
        skb_push(skb, 4);
        memcpy(skb->data, (void*)&ig_pkt_hdr, 4);
        len += 4;
    }else{
        struct sk_buff *tmp = skb;
        skb = skb_realloc_headroom(tmp, 4);

        if(tmp){
            dev_kfree_skb_any(tmp);
        }

        if(skb == NULL){
            AVMNET_ERR("[%s]: skb_realloc_headroom failed!!!\n", __func__);
            return -ENOMEM;
        }

        skb_push(skb, 4);
        memcpy(skb->data, &ig_pkt_hdr, 4);
        len += 4;

    }
    dump_eg_pkt_header( &ig_pkt_hdr );

#ifdef DUMP_PACKET
    AVMNET_ERR("[%s] skb->len: %d\n", __func__, skb->len);
    dump_buffer("start_xmit-Post",skb->data, skb->len);
#endif

    data = skb->data;
    dev->trans_start = jiffies;
    dma_ch_no = 0;

    // FIXME: get a lock before updating stats
    if(dma_device_write(dma_dev, data, len, skb, dma_ch_no) != len){
        AVMNET_TRC("[%s] dma_device_write failed.\n", __func__);
        goto ETH_XMIT_DROP;
    }else{
        eth_priv->avm_dev->device_stats.tx_packets++;
        eth_priv->avm_dev->device_stats.tx_bytes += len;
    }

    return NETDEV_TX_OK;

ETH_XMIT_DROP:
	dev_kfree_skb_any(skb);
ETH_PAD_DROP:
	if (eth_priv){
		eth_priv->avm_dev->device_stats.tx_dropped++;
		eth_priv->avm_dev->device_stats.tx_errors++;
	}
    return NETDEV_TX_OK;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
/**
 * DMA Pseudo Interrupt handler.
 * This function handle the DMA pseudo interrupts to handle the data packets Rx/Tx over DMA channels
 * It will handle the following interrupts
 *   RCV_INT: DMA receive the packet interrupt,So get from the PPE peripheral
 *   TX_BUF_FULL_INT: TX channel descriptors are not availabe, so, stop the transmission
 *                    and enable the Tx channel interrupt.
 *   TRANSMIT_CPT_INT: Tx channel descriptors are availabe and resume the transmission and
 *                     disable the Tx channel interrupt.
 */
int sw_dma_intr_handler(struct dma_device_info* dma_dev, int status, int dma_chan_no)
{
    avmnet_device_t *avm_dev;
    unsigned long sys_flag;
    unsigned int i;

    switch(status){
    case RCV_INT:
        switch_hw_receive(dma_dev, dma_chan_no);
        break;
    case TX_BUF_FULL_INT:
        local_irq_save(sys_flag);
        for(i = 0; i < MAX_ETH_INF; ++i){
            avm_dev = avm_devices[i];
            if(avm_dev && avm_dev->device){
                netif_stop_queue(avm_dev->device);
            }
        }

        for(i = 0; i < dma_dev->num_tx_chan; ++i){
            if((dma_dev->tx_chan[i])->control == IFX_DMA_CH_ON){
                dma_dev->tx_chan[i]->enable_irq(dma_dev->tx_chan[i]);
            }
        }
        local_irq_restore(sys_flag);
        break;
    case TRANSMIT_CPT_INT:
        local_irq_save(sys_flag);
        for(i = 0; i < dma_dev->num_tx_chan; ++i){
            dma_dev->tx_chan[i]->disable_irq(dma_dev->tx_chan[i]);
        }

        for(i = 0; i < MAX_ETH_INF; ++i){
            avm_dev = avm_devices[i];
            if(avm_dev && avm_dev->device){
                netif_wake_queue(avm_dev->device);
            }
        }
        local_irq_restore(sys_flag);
        break;
    }

    return IFX_SUCCESS;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
/*
 * Allocates the buffer for ethernet packet.
 * This function is invoke when DMA callback function to be called
 * to allocate a new buffer for Rx packets.
 */
#define RX_BYTE_OFFSET 2
unsigned char* sw_dma_buffer_alloc(int len __attribute__ ((unused)), int* byte_offset, void** opt)
{
    unsigned char *buffer = NULL;
    struct sk_buff *skb = NULL;
    unsigned int align_off = 0;

    *byte_offset = RX_BYTE_OFFSET; /* for reserving 2 bytes in skb buffer, so, set offset 2 bytes infront of data pointer */
    skb = dev_alloc_skb(ETH_PKT_BUF_SIZE + DMA_RX_ALIGNMENT);

    if(unlikely(skb == NULL)){
        AVMNET_ERR("[%s]: Buffer allocation failed.\n", __func__);
        return NULL;
    } else {
        align_off = DMA_RX_ALIGNMENT - ((u32)skb->data & (DMA_RX_ALIGNMENT - 1));
        skb_reserve(skb, align_off + RX_BYTE_OFFSET);

        buffer = (unsigned char *) skb->data - RX_BYTE_OFFSET;
        *(int*) opt = (int) skb;
    }

    return buffer;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
/* Free skb buffer
 * This function frees a buffer previously allocated by the DMA buffer
 * alloc callback function.
 */
int sw_dma_buffer_free(unsigned char* dataptr, void* opt)
{
    struct sk_buff *skb = NULL;

    if(opt == NULL){
        if(dataptr){
            kfree(dataptr);
        }
    }else{
        skb = (struct sk_buff*) opt;
        if(skb){
            dev_kfree_skb_any(skb);
        }
    }

    return IFX_SUCCESS;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
/*
 * Send the packet to network rx queue, used by  switch_hw_receive function
 */
static void ifx_eth_rx(struct net_device *dev, int len, struct sk_buff* skb)
{
    int rx_res;
    avmnet_netdev_priv_t *eth_priv;

    len = skb->len;

    if(netif_running(dev)){
#if defined(CONFIG_AVM_PA)
        if(likely(avm_pa_dev_receive(AVM_PA_DEVINFO(dev), skb) == AVM_PA_RX_ACCELERATED)) {
            dev->last_rx = jiffies;
            return;
        }
#endif /* CONFIG_AVM_PA */
        rx_res = netif_receive_skb(skb);
    }else{
        dev_kfree_skb_any(skb);
        rx_res = NET_RX_DROP;
    }

    eth_priv = (avmnet_netdev_priv_t *) netdev_priv(dev);
    if(rx_res == NET_RX_DROP){
        eth_priv->avm_dev->device_stats.rx_dropped++;
    }else{
        eth_priv->avm_dev->device_stats.rx_packets++;
        eth_priv->avm_dev->device_stats.rx_bytes += len;
    }
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
/*
 * Turn On RX DMA channels
 */
static void enable_dma_channel(void)
{
    struct dma_device_info* dma_dev = g_dma_device_ppe;
    unsigned int i;

    for(i = 0; i < dma_dev->max_rx_chan_num; i++){
        if((dma_dev->rx_chan[i])->control == IFX_DMA_CH_ON){
            dma_dev->rx_chan[i]->open(dma_dev->rx_chan[i]);
        }
    }
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
/*
 * Turn Off RX DMA channels
 */
static void disable_dma_channel()
{
    struct dma_device_info* dma_dev = g_dma_device_ppe;
    unsigned int i;

    for(i = 0; i < dma_dev->max_rx_chan_num; i++){
        dma_dev->rx_chan[i]->close(dma_dev->rx_chan[i]);
    }
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
/** This function scheduled from upper layer when the NAPI is enabled*/
static int eth_poll(struct napi_struct *napi __attribute__((unused)), int budget __attribute__((unused)))
{
    // kein NAPI-Poll mehr aus dem DMA-IRQ-Handler
    BUG();

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void eth_activate_poll(struct dma_device_info* dma_dev __attribute__ ((unused)))
{
    // kein NAPI-Poll mehr aus dem DMA-IRQ-Handler
    BUG();
}
EXPORT_SYMBOL(eth_activate_poll);

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void eth_inactivate_poll(struct dma_device_info* dma_dev __attribute__ ((unused)))
{
    // kein NAPI-Poll mehr aus dem DMA-IRQ-Handler
    BUG();
}
EXPORT_SYMBOL(eth_inactivate_poll);

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
/** Register with DMA device core driver */
static int dma_setup_init(void)
{
    unsigned int i; 
    int ret;

    // even with multiple instances, we would all use the same DMA device
    // make sure it is only configured once
    if(g_dma_device_ppe != NULL){
        return 0;
    }

    g_dma_device_ppe = dma_device_reserve("PPE");
    if(!g_dma_device_ppe){
        AVMNET_ERR("[%s]: Reserved with DMA core driver failed!!!\n", __func__);
        return -ENODEV;
    }

    g_dma_device_ppe->buffer_alloc = &sw_dma_buffer_alloc;
    g_dma_device_ppe->buffer_free = &sw_dma_buffer_free;
    g_dma_device_ppe->intr_handler = &sw_dma_intr_handler;
    g_dma_device_ppe->num_rx_chan = 4;
	/*
     * TKL: using two TX channels will lead to constant starting/stopping of both interfaces
     * in case of tx congestion
     */
    g_dma_device_ppe->num_tx_chan = 1; // 2;
    g_dma_device_ppe->tx_burst_len = DMA_TX_BURST_LEN;
    g_dma_device_ppe->rx_burst_len = DMA_RX_BURST_LEN;
    g_dma_device_ppe->tx_endianness_mode = IFX_DMA_ENDIAN_TYPE3;
    g_dma_device_ppe->rx_endianness_mode = IFX_DMA_ENDIAN_TYPE3;
    /*g_dma_device_ppe->port_packet_drop_enable   = IFX_DISABLE;*/

    for(i = 0; i < g_dma_device_ppe->num_rx_chan; i++){
        g_dma_device_ppe->rx_chan[i]->packet_size = ETH_PKT_BUF_SIZE;
        g_dma_device_ppe->rx_chan[i]->control = IFX_DMA_CH_ON;
    }

    for(i = 0; i < g_dma_device_ppe->num_tx_chan; i++){
        /* TODO: Modify with application requirement
         * eth0 -->DMA Tx channel 0, eth1 --> DMA Tx channel 1
         */
        if((i == 0) || (i == 1)){
            g_dma_device_ppe->tx_chan[i]->control = IFX_DMA_CH_ON;
        }else{
            g_dma_device_ppe->tx_chan[i]->control = IFX_DMA_CH_OFF;
        }
    }

    g_dma_device_ppe->activate_poll = eth_activate_poll;
    g_dma_device_ppe->inactivate_poll = eth_inactivate_poll;

    ret = dma_device_register(g_dma_device_ppe);
    if(ret != IFX_SUCCESS){
        AVMNET_ERR("[%s]: Register with DMA core driver Failed!!!\n", __func__);
    }

    return ret;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static void dma_setup_uninit(void)
{
    struct dma_device_info* dma_dev = g_dma_device_ppe;

    if(dma_dev){
        dma_device_unregister(dma_dev);
        dma_device_release(dma_dev);
    }
}

#ifdef CONFIG_AVM_PA
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static avmnet_device_t* get_avm_dev_from_pid(avm_pid_handle pid)
{
    avmnet_device_t *avmdev;
    int i;

    i = 0;
    avmdev = NULL;
    while(i < MAX_ETH_INF){
        if(avm_devices[i] &&
                ( pid == avm_devices[i]->device->avm_pa.devinfo.pid_handle)){
            avmdev = avm_devices[i];
            break;
        }
        ++i;
    }

    return avmdev;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

static int add_pa_mac_entry(struct mac_entry entry)
{
    int i, found_idx, free_idx, used_idx;
    unsigned int updated;

    found_idx = -1;
    free_idx = -1;
    used_idx = -1;

    // find either matching entry or first free slot in our MAC table
    // TODO: maybe use some hashing scheme rather than linear search
    for(i = 0; i < PA_MAC_TABLE_SIZE; ++i){
        /*
         * check for stale entries.
         * This can happen if adding an entry in add_session() fails after
         * one entry has already been added. The session is reported as
         * failed to the PA, so it won't call remove_session() for it. We
         * can safely set its refCount to zero, as the entry in the switch's
         *  MAC table will have expired already
         */
        if((pa_mac_table[i].refCount > 0)
            && (pa_mac_table[i].endtime > 0)
            && time_is_before_jiffies(pa_mac_table[i].endtime + (10 * HZ)))
        {
            AVMNET_INFO("[%s] Stale entry found for MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
                         __func__, entry.addr[0], entry.addr[1], entry.addr[2],
                                   entry.addr[3], entry.addr[4], entry.addr[5]);

            pa_mac_table[i].refCount = 0;
        }

        if(pa_mac_table[i].refCount > 0
           && memcmp(&(entry.addr[0]), &(pa_mac_table[i].addr), ETH_ALEN) == 0)
        {
            // valid entry for MAC found
            found_idx = i;
            break;
        }

        if(free_idx < 0 && pa_mac_table[i].refCount == 0){
            // first free slot in table found, remember it for later
            free_idx = i;
        }
    }

    if(found_idx >= 0){
        // there is already a valid entry for this address. Update it if needed
        // and increase reference counter

        updated = 0;
        used_idx = found_idx;
        ++pa_mac_table[found_idx].refCount;

        if(pa_mac_table[found_idx].endtime > 0
           && time_after(entry.endtime, pa_mac_table[found_idx].endtime))
        {
            pa_mac_table[found_idx].endtime = entry.endtime;
            updated = 1;
        }

        // does the new portmap contain ports not already in the old portmap?
        if((entry.portmap ^ pa_mac_table[found_idx].portmap) & entry.portmap){
            pa_mac_table[found_idx].portmap |= entry.portmap;
            updated = 1;
        }

    }else if(free_idx >= 0){
        pa_mac_table[free_idx] = entry;
        pa_mac_table[free_idx].refCount = 1;
        used_idx = free_idx;
        updated = 1;
    }else{
        AVMNET_INFO("[%s] No previous entry for MAC %02x:%02x:%02x:%02x:%02x:%02x found and no slot free\n",
                     __func__, entry.addr[0], entry.addr[1], entry.addr[2],
                               entry.addr[3], entry.addr[4], entry.addr[5]);

        return -EFAULT;
    }

    if(updated){
        if(pa_mac_table[used_idx].endtime > 0){
            // calculate timeout. Borderline case of integer underflow will be
            // handled by max. timeout in add_mac_entry()
            // TODO: handle jiffies wrap-around
            pa_mac_table[used_idx].timeout = (pa_mac_table[used_idx].endtime - jiffies) / HZ;
        }else{
            // timeout of zero creates static entry
            pa_mac_table[used_idx].timeout = 0;
        }

        add_mac_entry(pa_mac_table[used_idx]);
    }

    return used_idx;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

static int avmnet_ar9_pce_add_session(struct avm_pa_session *avm_session)
{
    unsigned int i, negress;
    int mac_nr;
    int status, tbl_idx;
    avmnet_device_t *indev, *outdev;
    struct avmnet_ar9_pa_session *new_session;
    struct mac_entry entry;

    if(avm_session->bsession == NULL){
        AVMNET_DEBUG("[%s] Refusing non-bridging PA session.\n", __func__);
        return AVM_PA_TX_ERROR_SESSION;
    }

    indev = get_avm_dev_from_pid(avm_session->ingress_pid_handle);
    if(indev == NULL){
        AVMNET_INFO("[%s] Unknown ingress device for pid handle %d.\n", __func__, avm_session->ingress_pid_handle);
        return AVM_PA_TX_ERROR_SESSION;
    }

    // see if someone else is working with the PA session list and
    // bail out if it is locked
    if(down_trylock(&swi_pa_mutex)){
        AVMNET_ERR("[%s] PA session list is locked, aborting.", __func__);
        return AVM_PA_TX_ERROR_SESSION;
    }

    AVMNET_DEBUG("[%s] Ingress device: %s\n", __func__, indev->device->name);
    new_session = kzalloc(sizeof(struct avmnet_ar9_pa_session), GFP_KERNEL);
    if(new_session == NULL){
        AVMNET_ERR("[%s] Could not allocate memory for new session.\n", __func__);
        up(&swi_pa_mutex);
        return AVM_PA_TX_ERROR_SESSION;
    }

    status = AVM_PA_TX_SESSION_ADDED;

    new_session->ingress.mac_nr = indev->mac_module->initdata.mac.mac_nr;
    new_session->session_id = avm_session->session_handle;
    new_session->pa_session = avm_session;

    AVMNET_DEBUG("[%s] Adding new ingress device %s for session %d, pid handle %d on mac_nr %d\n",
                 __func__, indev->device->name, avm_session->session_handle,
                 avm_session->ingress_pid_handle, new_session->ingress.mac_nr);

    memcpy(&(new_session->ingress.header),
           &(avm_session->bsession->ethh),
           sizeof(struct ethhdr));

    negress = 0;
    for(i = 0; i < avm_session->negress; ++i){
        outdev = get_avm_dev_from_pid(avm_session->egress[i].pid_handle);
        if(outdev){
            AVMNET_DEBUG("[%s] Egress device: %s\n", __func__, outdev->device->name);

            mac_nr = outdev->mac_module->initdata.mac.mac_nr;

            AVMNET_DEBUG("[%s] Egress MAC: %d\n", __func__, mac_nr);
            AVMNET_DEBUG("[%s] Adding new egress device %s for pid handle %d on port %d\n",
                         __func__, outdev->device->name, avm_session->ingress_pid_handle, mac_nr);

            new_session->egress[negress].mac_nr = mac_nr;
            memcpy(&(new_session->egress[negress].header),
                   &(avm_session->bsession->ethh),
                   sizeof(struct ethhdr));

            ++negress;
        }else{
            AVMNET_INFO("[%s] Ignoring unknown egress device for pid handle %d\n",
                        __func__, avm_session->ingress_pid_handle);
        }
    }
    new_session->negress = negress;

    if(negress == 0){
        // this may happen if we are asked to accelerate a session across an
        // interface we don't own
        AVMNET_INFO("[%s] No egress ports for session found, aborting\n", __func__);
        kfree(new_session);
        up(&swi_pa_mutex);
        return AVM_PA_TX_ERROR_SESSION;
    }

#if 0 // only accelerate in egress direction
    memcpy(entry.addr, new_session->ingress.header.h_source, sizeof(entry.addr));
    entry.portmap = (1 << new_session->ingress.port) & 0xff;
    entry.fid = 0;
    entry.endtime = jiffies + avm_session->timeout;
    entry.endtime += HZ; // give the PA an extra second to call remove_session()

    tbl_idx = add_pa_mac_entry(entry);
    AVMNET_DEBUG("[%s] Adding ingress %d at mac_idx: %#x\n", __func__, i, tbl_idx);
    if(tbl_idx >= 0 && tbl_idx < PA_MAC_TABLE_SIZE){
        new_session->ingress.mac_tbl_idx = tbl_idx;
    } else {
        AVMNET_INFO("[%s] invalid mac_idx!\n", __func__);
        status = AVM_PA_TX_ERROR_SESSION;
    }
#endif

    // add a MAC table entry for each egress MAC.
    i = 0;
    while(status != AVM_PA_TX_ERROR_SESSION && i < new_session->negress){
        memcpy(entry.addr, new_session->egress[i].header.h_dest, sizeof(entry.addr));
        entry.portmap = (1 << new_session->egress[i].mac_nr) & 0xff;
        entry.fid = 0;
        entry.endtime = jiffies + avm_session->timeout;
        entry.endtime += HZ; // give the PA an extra second to call remove_session()

        tbl_idx = add_pa_mac_entry(entry);
        AVMNET_DEBUG("[%s] Adding egress %d at mac_idx: %#x\n", __func__, i, tbl_idx);
        if(tbl_idx >= 0 && tbl_idx < PA_MAC_TABLE_SIZE){
            new_session->egress[i].mac_tbl_idx = tbl_idx;
        } else {
            AVMNET_INFO("[%s] invalid mac_idx!\n", __func__);
            status = AVM_PA_TX_ERROR_SESSION;
        }

        ++i;
    }

    if(status == AVM_PA_TX_SESSION_ADDED){
        INIT_LIST_HEAD(&(new_session->list));
        list_add_tail(&(new_session->list), &pce_sessions);
        avm_session->hw_session = new_session;
    }else{
        /*
         * TODO: there is no easy way to handle a failed session entry attempt.
         * If adding one of the egress entries fails, we can't just remove all
         * the entries we have added so far. There might be other sessions using
         * one of these MAC addresses, and removing those would break them.
         * The clean way would be to scan all other sessions, remove only those
         * entries unique to the failed session request, and update the timeout
         * values on the others.
         * Since I'm a lazy bum, we'll just let automatic aging take care of
         * this entries in the switch's MAC table and check for stale entries
         * in our MAC table whenever a new entry is added.
         */
        avm_session->hw_session = NULL;
        kfree(new_session);
    }

    up(&swi_pa_mutex);

    return status;

}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

static int avmnet_ar9_remove_pa_port(struct ar9_pa_port *port, unsigned int is_ingress)
{
    unsigned char *addr, *entry;

    /*
     * Check whether table index is valid and the entry is really the
     * MAC we are looking for
     *
     * Yes, I'm paranoid...
     */
    if(is_ingress){
        addr = port->header.h_source;
    }else{
        addr = port->header.h_dest;
    }

    if(port->mac_tbl_idx < 0 || port->mac_tbl_idx >= PA_MAC_TABLE_SIZE){
        AVMNET_ERR("[%s] Invalid table index (%d) for MAC %02x:%02x:%02x:%02x:%02x:%02x.\n",
                    __func__, port->mac_tbl_idx, addr[0], addr[1], addr[2],
                                                 addr[3], addr[4], addr[5]);

        return -EFAULT;
    }

    entry = pa_mac_table[port->mac_tbl_idx].addr;
    if(memcmp(addr, entry, ETH_ALEN) != 0){
        AVMNET_ERR("[%s] Wrong index for MAC %02x:%02x:%02x:%02x:%02x:%02x.\n",
                    __func__, addr[0], addr[1], addr[2],
                              addr[3], addr[4], addr[5]);
        AVMNET_ERR("[%s] MAC at table index %d: %02x:%02x:%02x:%02x:%02x:%02x.\n",
                    __func__, port->mac_tbl_idx, entry[0], entry[1], entry[2],
                                                 entry[3], entry[4], entry[5]);

        return -EFAULT;
    }

    // decrease reference count for entry and remove the address from switch
    // when it reaches 0
    if(pa_mac_table[port->mac_tbl_idx].refCount > 0){
        --pa_mac_table[port->mac_tbl_idx].refCount;

        if(pa_mac_table[port->mac_tbl_idx].refCount == 0){
            del_mac_entry(pa_mac_table[port->mac_tbl_idx]);
        }
    }else{
        // this should not happen
        AVMNET_ERR("[%s] RefCount for MAC %02x:%02x:%02x:%02x:%02x:%02x is 0!\n",
                    __func__, addr[0], addr[1], addr[2],
                              addr[3], addr[4], addr[5]);
    }

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

static int avmnet_ar9_pce_remove_session(struct avm_pa_session *avm_session)
{
    struct avmnet_ar9_pa_session *session;

    AVMNET_DEBUG("[%s] delete session %d", __func__, avm_session->session_handle);

    if(avm_session->hw_session == NULL){
        AVMNET_ERR("[%s] invalid hw_session pointer in avm_session %d\n", __func__,  avm_session->session_handle);
        return AVM_PA_TX_ERROR_SESSION; //FIXME
    }

    session = (struct avmnet_ar9_pa_session *) avm_session->hw_session;
    avm_session->hw_session = NULL;

    if(down_trylock(&swi_pa_mutex)){
        // List of active PA sessions is locked, just mark this one as to
        // be removed and let our heart beat func clean it up
        session->removed = 1;
        return AVM_PA_TX_OK;
    } else {
        avmnet_ar9_remove_session_entry(session);
        up(&swi_pa_mutex);
    }

    return AVM_PA_TX_OK;
}

/*------------------------------------------------------------------------------------------*\
 * free an PA session in the list
 * Must be called with the swi_pa_mutex held!
\*------------------------------------------------------------------------------------------*/
void avmnet_ar9_remove_session_entry(struct avmnet_ar9_pa_session *session)
{
    unsigned int i;

    list_del(&(session->list));

    // remove MAC address entries from switch and free session data
    AVMNET_DEBUG("[%s] Freeing Session %d\n", __func__, session->session_id);

    #if 0 // only accelerate in egress direction
    avmnet_ar9_remove_pa_port(&(session->ingress), 1);
    #endif

    for(i = 0; i < session->negress; ++i){
        avmnet_ar9_remove_pa_port(&(session->egress[i]), 0);
    }

    kfree(session);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static int pa_sessions_show(struct seq_file *seq, void *data __attribute__ ((unused)) )
{
    struct avmnet_ar9_pa_session *session;
    struct ar9_pa_port *port;
    avmnet_device_t *avmdev;
    unsigned int i;

    if(in_interrupt()){
        AVMNET_ERR("[%s] in_interrupt()! Abort.\n", __func__);
        dump_stack();
        return -EFAULT;
    }

    if(down_interruptible(&swi_pa_mutex)){
        AVMNET_WARN("[%s] Interrupted while waiting for mutex, aborting\n", __func__);

        return -EFAULT;
    }

    seq_printf(seq, "Active PA sessions:\n");

    list_for_each_entry(session, &pce_sessions, list){

        port = &(session->ingress);
        avmdev = get_avm_dev_from_pid(session->pa_session->ingress_pid_handle);

        seq_printf(seq, "\n");
        seq_printf(seq, "Session handle: %d\n", session->session_id);

        seq_printf(seq, "  Ingress MAC: %02x:%02x:%02x:%02x:%02x:%02x HW-Port: %d Dev: %s\n",
                    port->header.h_source[0], port->header.h_source[1], port->header.h_source[2],
                    port->header.h_source[3], port->header.h_source[4], port->header.h_source[5],
                    port->mac_nr, avmdev->device->name);

        for(i = 0; i < session->negress; ++i){
            port = &(session->egress[i]);
            avmdev = get_avm_dev_from_pid(session->pa_session->egress[i].pid_handle);

            seq_printf(seq, "  Egress  MAC: %02x:%02x:%02x:%02x:%02x:%02x HW-Port: %d Dev: %s\n",
                        port->header.h_dest[0], port->header.h_dest[1], port->header.h_dest[2],
                        port->header.h_dest[3], port->header.h_dest[4], port->header.h_dest[5],
                        port->mac_nr, avmdev->device->name);
        }

    }

    up(&swi_pa_mutex);

    return 0;
}

static int pa_sessions_open(struct inode *inode, struct file *file)
{
    avmnet_module_t *this;

    this = (avmnet_module_t *) PDE(inode)->data;

    return single_open(file, pa_sessions_show, this);
}


#endif

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
union pause_on_wm {
    struct {
        unsigned int res:14;
        unsigned int  wm:18;
    } bits;
    unsigned int reg;
};

union pause_off_wm {
    struct {
        unsigned int res:14;
        unsigned int  wm:18;
    } bits;
    unsigned int reg;
};

union buffer_threshold_reg {
    struct {
        unsigned int puo2:2;
        unsigned int puo1:2;
        unsigned int puo0:2;
        unsigned int res3:2;
        unsigned int pfo2:2;
        unsigned int pfo1:2;
        unsigned int pfo0:2;
        unsigned int res2:4;
        unsigned int tla:1;
        unsigned int tha:1;
        unsigned int tlo:2;
        unsigned int tho:2;
        unsigned int pua:3;
        unsigned int res1:1;
        unsigned int pfa:4;
        unsigned int res0:1;
    } bits;
    unsigned int reg;
};

union ingress_control_reg {
    struct {
        unsigned int res:19;
        unsigned int itt:2;
        unsigned int itr:11;
    } bits;
    unsigned int reg;
};

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

static int read_flow_control_show(struct seq_file *seq, void *data __attribute__ ((unused)) )
{
    union pause_off_wm p_off_wm;
    union pause_on_wm p_on_wm;
    union buffer_threshold_reg bf_th_reg;
    union ingress_control_reg p0_inctl, p1_inctl, p2_inctl;

    p_on_wm.reg  = *IFX_AR9_ETH_PAUSE_ON_WM;
    p_off_wm.reg = *IFX_AR9_ETH_PAUSE_OFF_WM;
    bf_th_reg.reg = *IFX_AR9_ETH_BF_TH_REG;
    p0_inctl.reg = *IFX_AR9_ETH_P0_INCTL_REG;
    p1_inctl.reg = *IFX_AR9_ETH_P1_INCTL_REG;
    p2_inctl.reg = *IFX_AR9_ETH_P2_INCTL_REG;

    seq_printf(seq, "===================               ========\n");
    seq_printf(seq, "Pause on Watermark                %#x\n", p_on_wm.bits.wm);
    seq_printf(seq, "Pause off Watermark               %#x\n", p_off_wm.bits.wm);
    seq_printf(seq, "===================               ========\n");
    seq_printf(seq, "\n");
    seq_printf(seq, "puo2:                             %#x\n", bf_th_reg.bits.puo2);
    seq_printf(seq, "puo1:                             %#x\n", bf_th_reg.bits.puo1);
    seq_printf(seq, "puo0:                             %#x\n", bf_th_reg.bits.puo0);
    seq_printf(seq, "pfo2:                             %#x\n", bf_th_reg.bits.pfo2);
    seq_printf(seq, "pfo1:                             %#x\n", bf_th_reg.bits.pfo1);
    seq_printf(seq, "pfo0:                             %#x\n", bf_th_reg.bits.pfo0);
    seq_printf(seq, "tla :                             %#x\n", bf_th_reg.bits.tla );
    seq_printf(seq, "tha :                             %#x\n", bf_th_reg.bits.tha );
    seq_printf(seq, "tlo :                             %#x\n", bf_th_reg.bits.tlo );
    seq_printf(seq, "tho :                             %#x\n", bf_th_reg.bits.tho );
    seq_printf(seq, "pua :                             %#x\n", bf_th_reg.bits.pua );
    seq_printf(seq, "pfa :                             %#x\n", bf_th_reg.bits.pfa );
    seq_printf(seq, "\n");
    seq_printf(seq, "Ingress Control TB         PORT0       PORT1       PORT2     \n");
    seq_printf(seq, "===================        ==========  ==========  ==========  \n");
    seq_printf(seq, "Token Bucket T :           %#010x  %#010x  %#010x\n", p0_inctl.bits.itt, p1_inctl.bits.itt, p2_inctl.bits.itt );
    seq_printf(seq, "Token Bucket R :           %#010x  %#010x  %#010x\n", p0_inctl.bits.itr, p1_inctl.bits.itr, p2_inctl.bits.itr );
    seq_printf(seq, "\n");
    seq_printf(seq, "Egress Control TB          PORT0       PORT1       PORT2     \n");
    seq_printf(seq, "===================        ==========  ==========  ==========  \n");
    seq_printf(seq, "ECS Q32:                   %#010x  %#010x  %#010x\n", *IFX_AR9_ETH_P0_ECS_Q32_REG, *IFX_AR9_ETH_P1_ECS_Q32_REG, *IFX_AR9_ETH_P2_ECS_Q32_REG);
    seq_printf(seq, "ECS Q10:                   %#010x  %#010x  %#010x\n", *IFX_AR9_ETH_P0_ECS_Q10_REG, *IFX_AR9_ETH_P1_ECS_Q10_REG, *IFX_AR9_ETH_P2_ECS_Q10_REG);
    seq_printf(seq, "ECW Q32:                   %#010x  %#010x  %#010x\n", *IFX_AR9_ETH_P0_ECW_Q32_REG, *IFX_AR9_ETH_P1_ECW_Q32_REG, *IFX_AR9_ETH_P2_ECW_Q32_REG);
    seq_printf(seq, "ECW Q10:                   %#010x  %#010x  %#010x\n", *IFX_AR9_ETH_P0_ECW_Q10_REG, *IFX_AR9_ETH_P1_ECW_Q10_REG, *IFX_AR9_ETH_P2_ECW_Q10_REG);
    seq_printf(seq, "\n");
    seq_printf(seq, "Send Pause Frame           PORT0       PORT1       PORT2     \n");
    seq_printf(seq, "===================        ==========  ==========  ==========  \n");
    seq_printf(seq, "                           %d           %d           ----------\n", RGMII_CTL_FCE_GET(0, (*IFX_AR9_ETH_RGMII_CTL)), RGMII_CTL_FCE_GET(1, (*IFX_AR9_ETH_RGMII_CTL)));

    return 0;

}

static int read_flow_control_open(struct inode *inode, struct file *file)
{
    avmnet_module_t *this;

    this = (avmnet_module_t *) PDE(inode)->data;

    return single_open(file, read_flow_control_show, this);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
#define FC_BUF_SIZE 1024
unsigned char fc_buf[FC_BUF_SIZE];

static ssize_t write_flow_control(struct file *file, const char *buf, size_t count, loff_t *ppos) {
  int end_pos;
  int parse_pos = 0;

  if (*ppos != 0)
    return -EFAULT;

  if (count >= sizeof(fc_buf))
    return -EFAULT;

  if (copy_from_user(fc_buf, buf, count))
    return -EFAULT;

  // remove whitespace at tail
  end_pos = count-1;
  while ((end_pos >= 0) && (fc_buf[end_pos] <= ' '))
    end_pos--;
  end_pos++;
  fc_buf[ end_pos ] = '\0';

  // remove whitespace at head
  while ((parse_pos < end_pos ) && (fc_buf[parse_pos] <= ' '))
      parse_pos++;

  if ( strncmp(&fc_buf[parse_pos], "pause_off_wm", 12 ) == 0 ){
      unsigned int new_val = 0;
      union pause_off_wm p_off_wm;

      parse_pos += sizeof("pause_off_wm");
      sscanf( &fc_buf[parse_pos], "%x", &new_val );
      printk(KERN_ERR "setup pause_off_wm: new_val=%#x\n", new_val);

      p_off_wm.bits.wm = new_val;
      *IFX_AR9_ETH_PAUSE_OFF_WM = p_off_wm.reg;
  }

  if ( strncmp(&fc_buf[parse_pos], "pause_on_wm", 11 ) == 0 ){
      unsigned int new_val = 0;
      union pause_on_wm p_on_wm;

      parse_pos += sizeof("pause_on_wm");
      sscanf( &fc_buf[parse_pos], "%x", &new_val );
      printk(KERN_ERR "setup pause_on_wm: new_val=%#x\n", new_val);

      p_on_wm.bits.wm = new_val;
      *IFX_AR9_ETH_PAUSE_ON_WM = p_on_wm.reg;

  }
  if ( strncmp(&fc_buf[parse_pos], "ingress_control", 15 ) == 0 ){
      unsigned int port = 0;
      unsigned int new_val_itt = 0;
      unsigned int new_val_itr = 0;
      union ingress_control_reg inctl;

      parse_pos += sizeof("ingress_control");
      sscanf( &fc_buf[parse_pos], "%d %x %x", &port, &new_val_itt, &new_val_itr );
      printk(KERN_ERR "setup ingress_control for port %d: T=%#x, R=%#x\n", port, new_val_itt, new_val_itr);
      inctl.bits.itt = new_val_itt;
      inctl.bits.itr = new_val_itr;

      switch(port){
        case 0:
            *IFX_AR9_ETH_P0_INCTL_REG = inctl.reg;
            break;
        case 1:
            *IFX_AR9_ETH_P1_INCTL_REG = inctl.reg;
            break;
        case 2:
            *IFX_AR9_ETH_P2_INCTL_REG = inctl.reg;
            break;
      }

  }
  if ( strncmp(&fc_buf[parse_pos], "flow_control", 12 ) == 0 ){
      unsigned int port = 0;
      unsigned int fc;

      parse_pos += sizeof("flow_control");
      sscanf( &fc_buf[parse_pos], "%d %d ", &port, &fc);
      if ((port < 2) && (fc <2)){
        unsigned int fc_reg;
        printk(KERN_ERR "setup flow_control for port %d: %d\n", port, fc);
        fc_reg = *IFX_AR9_ETH_RGMII_CTL;

        fc_reg &= ~RGMII_CTL_FCE_SET(port, 1);
        fc_reg |= RGMII_CTL_FCE_SET(port, fc);

        *IFX_AR9_ETH_RGMII_CTL = fc_reg;
      }
  }


  *ppos = count;
  return count;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static int read_rmon_show(struct seq_file *seq, void *data __attribute__ ((unused)) )
{
    int i;
    unsigned int index, portindex;
    unsigned int counters[RMON_PORT_SIZE][RMON_COUNT_SIZE];
    avmnet_module_t *this __maybe_unused__;

    if(in_interrupt()){
        AVMNET_ERR("[%s] in_interrupt()! Abort.\n", __func__);
        dump_stack();
        return -EFAULT;
    }

    this = (avmnet_module_t *) seq->private;

    if(down_interruptible(&swi_ar9_mutex)){

        AVMNET_ERR("[%s] interrupted while waiting for semaphore\n", __func__);

        return -EFAULT;
    }

    // wait for register to become available
    while(IFX_REG_R32(IFX_AR9_ETH_RMON_CTL) & RMON_CTL_BUSY)
        ;

    portindex = 0;
    do{
        // configure for incremental read
        IFX_REG_W32((RMON_CTL_CAC_PORT_CNT | RMON_CTL_BUSY | (portindex << 6)),
                    IFX_AR9_ETH_RMON_CTL);

        // trigger read sequence
        IFX_REG_R32(IFX_AR9_ETH_RMON_ST);

        index = 0;
        do{
            // wait for next value to become available
            while(IFX_REG_R32(IFX_AR9_ETH_RMON_CTL) & RMON_CTL_BUSY)
                ;

            // read counter value
            counters[portindex][index] = IFX_REG_R32(IFX_AR9_ETH_RMON_ST);

            ++index;
        }while(index < RMON_COUNT_SIZE);

        ++portindex;
    }while(portindex < RMON_PORT_SIZE);
    up(&swi_ar9_mutex);

    seq_printf(seq, "RMON counter for Port:                     Port 0   Port 1   Port 2\n");
    seq_printf(seq, "======================                    ======== ======== ========\n");
    for(i = 0; i < RMON_COUNT_SIZE; ++i){
        seq_printf(seq, "%s %08x %08x %08x\n", rmon_names[i], counters[0][i], counters[1][i], counters[2][i]);
    }

    return 0;
}

static int read_rmon_open(struct inode *inode, struct file *file)
{
    avmnet_module_t *this;

    this = (avmnet_module_t *) PDE(inode)->data;

    return single_open(file, read_rmon_show, this);
}



static int mac_table_show(struct seq_file *seq, void *data __attribute__ ((unused)) )
{
    struct mac_entry entry;
    unsigned int addr_ctl0, addr_ctl1;
    unsigned int addr_st0, addr_st1, addr_st2;
    int i;
    avmnet_module_t *this __maybe_unused__;

    if(in_interrupt()){
        AVMNET_ERR("[%s] in_interrupt()! Abort.\n", __func__);
        dump_stack();
        return -EFAULT;
    }

    this = (avmnet_module_t *) seq->private;

    if(down_interruptible(&swi_ar9_mutex)){

        AVMNET_ERR("[%s] interrupted while waiting for semaphore\n", __func__);

        return -EFAULT;
    }

    seq_printf(seq, "MAC table entries:\n");

    for(i = 0; i < 3; ++i){
        addr_ctl0 = 0;
        addr_ctl1 = ADRTB_REG1_PMAP_PORT(i);

        while(IFX_REG_R32(IFX_AR9_ETH_ADRTB_ST2) & ADRTB_REG2_BUSY)
            ;

        IFX_REG_W32(ADRTB_CMD_INIT_START, IFX_AR9_ETH_ADRTB_CTL2);

        do{
            while(IFX_REG_R32(IFX_AR9_ETH_ADRTB_ST2) & ADRTB_REG2_BUSY)
                ;

            IFX_REG_W32(addr_ctl0, IFX_AR9_ETH_ADRTB_CTL0);
            IFX_REG_W32(addr_ctl1, IFX_AR9_ETH_ADRTB_CTL1);
            IFX_REG_W32(ADRTB_CMD_SRCH_MAC_PORT, IFX_AR9_ETH_ADRTB_CTL2);

            while((addr_st2 = IFX_REG_R32(IFX_AR9_ETH_ADRTB_ST2)) & ADRTB_REG2_BUSY)
                ;

            if((addr_st2 & ADRTB_REG2_RSLT_MASK) == ADRTB_REG2_RSLT_OK){
                addr_st0 = IFX_REG_R32(IFX_AR9_ETH_ADRTB_ST0);
                addr_st1 = IFX_REG_R32(IFX_AR9_ETH_ADRTB_ST1);

                // copy data of found entry into a struct mac_entry
                addr_st0 = IFX_REG_R32(IFX_AR9_ETH_ADRTB_ST0);
                addr_st1 = IFX_REG_R32(IFX_AR9_ETH_ADRTB_ST1);

                entry.addr[5] = addr_st0 & 0xff;
                addr_st0 >>= 8;
                entry.addr[4] = addr_st0 & 0xff;
                addr_st0 >>= 8;
                entry.addr[3] = addr_st0 & 0xff;
                addr_st0 >>= 8;
                entry.addr[2] = addr_st0 & 0xff;

                entry.addr[1] = addr_st1 & 0xff;
                entry.addr[0] = (addr_st1 >> 8) & 0xff;

                entry.fid = (addr_st1 & ADRTB_REG1_FID_MASK) >> 16;
                entry.portmap = (addr_st1 & ADRTB_REG1_PMAP_MASK) >> 20;

                if(addr_st2 & ADRTB_REG2_INFOT){
                    entry.timeout = 0;
                }else{
                    entry.timeout = addr_st2 & ADRTB_REG2_ITAT_MASK;
                }

                seq_printf(seq, "Port %d: %02x:%02x:%02x:%02x:%02x:%02x FID: 0x%02x, PMAP: 0x%02x, Age: %d\n",
                           i, entry.addr[0], entry.addr[1], entry.addr[2],
                              entry.addr[3], entry.addr[4], entry.addr[5],
                              entry.fid, entry.portmap, entry.timeout);
            }
        }while((addr_st2 & ADRTB_REG2_RSLT_MASK) == ADRTB_REG2_RSLT_OK);
    }

    up(&swi_ar9_mutex);

    return 0;
}

static int mac_table_open(struct inode *inode, struct file *file)
{
    avmnet_module_t *this;

    this = (avmnet_module_t *) PDE(inode)->data;

    return single_open(file, mac_table_show, this);
}

#if defined(DEBUG_PP32_PROC) && DEBUG_PP32_PROC
int proc_read_pp32(char *page, 
                   char **start __attribute__ ((unused)),
                   off_t off __attribute__ ((unused)), 
                   int count __attribute__ ((unused)), 
                   int *eof, 
                   void *data __attribute__ ((unused))) {
    static const char *stron = " on";
    static const char *stroff = "off";

    int len = 0;
    int cur_context;
    int f_stopped = 0;
    char str[256];
    char strlength;
    int i, j;

    if ( PP32_CPU_USER_STOPPED || PP32_CPU_USER_BREAKIN_RCV || PP32_CPU_USER_BREAKPOINT_MET )
    {
        strlength = 0;
        if ( PP32_CPU_USER_STOPPED )
            strlength += sprintf(str + strlength, "stopped");
        if ( PP32_CPU_USER_BREAKPOINT_MET )
            strlength += sprintf(str + strlength, strlength ? " | breakpoint" : "breakpoint");
        if ( PP32_CPU_USER_BREAKIN_RCV )
            strlength += sprintf(str + strlength, strlength ? " | breakin" : "breakin");
        f_stopped = 1;
    }
    else if ( PP32_CPU_CUR_PC == PP32_CPU_CUR_PC )
    {
        sprintf(str, "stopped");
        f_stopped = 1;
    }
    else
        sprintf(str, "running");
    cur_context = PP32_BRK_CUR_CONTEXT;
    len += sprintf(page + len, "Context: %d, PC: 0x%04x, %s\n", cur_context, PP32_CPU_CUR_PC, str);

    if ( PP32_CPU_USER_BREAKPOINT_MET )
    {
        strlength = 0;
        if ( PP32_BRK_PC_MET(0) )
            strlength += sprintf(str + strlength, "pc0");
        if ( PP32_BRK_PC_MET(1) )
            strlength += sprintf(str + strlength, strlength ? " | pc1" : "pc1");
        if ( PP32_BRK_DATA_ADDR_MET(0) )
            strlength += sprintf(str + strlength, strlength ? " | daddr0" : "daddr0");
        if ( PP32_BRK_DATA_ADDR_MET(1) )
            strlength += sprintf(str + strlength, strlength ? " | daddr1" : "daddr1");
        if ( PP32_BRK_DATA_VALUE_RD_MET(0) )
        {
            strlength += sprintf(str + strlength, strlength ? " | rdval0" : "rdval0");
            if ( PP32_BRK_DATA_VALUE_RD_LO_EQ(0) )
            {
                if ( PP32_BRK_DATA_VALUE_RD_GT_EQ(0) )
                    strlength += sprintf(str + strlength, " ==");
                else
                    strlength += sprintf(str + strlength, " <=");
            }
            else if ( PP32_BRK_DATA_VALUE_RD_GT_EQ(0) )
                strlength += sprintf(str + strlength, " >=");
        }
        if ( PP32_BRK_DATA_VALUE_RD_MET(1) )
        {
            strlength += sprintf(str + strlength, strlength ? " | rdval1" : "rdval1");
            if ( PP32_BRK_DATA_VALUE_RD_LO_EQ(1) )
            {
                if ( PP32_BRK_DATA_VALUE_RD_GT_EQ(1) )
                    strlength += sprintf(str + strlength, " ==");
                else
                    strlength += sprintf(str + strlength, " <=");
            }
            else if ( PP32_BRK_DATA_VALUE_RD_GT_EQ(1) )
                strlength += sprintf(str + strlength, " >=");
        }
        if ( PP32_BRK_DATA_VALUE_WR_MET(0) )
        {
            strlength += sprintf(str + strlength, strlength ? " | wtval0" : "wtval0");
            if ( PP32_BRK_DATA_VALUE_WR_LO_EQ(0) )
            {
                if ( PP32_BRK_DATA_VALUE_WR_GT_EQ(0) )
                    strlength += sprintf(str + strlength, " ==");
                else
                    strlength += sprintf(str + strlength, " <=");
            }
            else if ( PP32_BRK_DATA_VALUE_WR_GT_EQ(0) )
                strlength += sprintf(str + strlength, " >=");
        }
        if ( PP32_BRK_DATA_VALUE_WR_MET(1) )
        {
            strlength += sprintf(str + strlength, strlength ? " | wtval1" : "wtval1");
            if ( PP32_BRK_DATA_VALUE_WR_LO_EQ(1) )
            {
                if ( PP32_BRK_DATA_VALUE_WR_GT_EQ(1) )
                    strlength += sprintf(str + strlength, " ==");
                else
                    strlength += sprintf(str + strlength, " <=");
            }
            else if ( PP32_BRK_DATA_VALUE_WR_GT_EQ(1) )
                strlength += sprintf(str + strlength, " >=");
        }
        len += sprintf(page + len, "break reason: %s\n", str);
    }

    if ( f_stopped )
    {
        len += sprintf(page + len, "General Purpose Register (Context %d):\n", cur_context);
        for ( i = 0; i < 4; i++ )
        {
            for ( j = 0; j < 4; j++ )
                len += sprintf(page + len, "   %2d: %08x", i + j * 4, *PP32_GP_CONTEXTi_REGn(cur_context, i + j * 4));
            len += sprintf(page + len, "\n");
        }
    }

    len += sprintf(page + len, "break out on: break in - %s, stop - %s\n",
                                        PP32_CTRL_OPT_BREAKOUT_ON_BREAKIN ? stron : stroff,
                                        PP32_CTRL_OPT_BREAKOUT_ON_STOP ? stron : stroff);
    len += sprintf(page + len, "     stop on: break in - %s, break point - %s\n",
                                        PP32_CTRL_OPT_STOP_ON_BREAKIN ? stron : stroff,
                                        PP32_CTRL_OPT_STOP_ON_BREAKPOINT ? stron : stroff);
    len += sprintf(page + len, "breakpoint:\n");
    len += sprintf(page + len, "     pc0: 0x%08x, %s\n", *PP32_BRK_PC(0), PP32_BRK_GRPi_PCn(0, 0) ? "group 0" : "off");
    len += sprintf(page + len, "     pc1: 0x%08x, %s\n", *PP32_BRK_PC(1), PP32_BRK_GRPi_PCn(1, 1) ? "group 1" : "off");
    len += sprintf(page + len, "  daddr0: 0x%08x, %s\n", *PP32_BRK_DATA_ADDR(0), PP32_BRK_GRPi_DATA_ADDRn(0, 0) ? "group 0" : "off");
    len += sprintf(page + len, "  daddr1: 0x%08x, %s\n", *PP32_BRK_DATA_ADDR(1), PP32_BRK_GRPi_DATA_ADDRn(1, 1) ? "group 1" : "off");
    len += sprintf(page + len, "  rdval0: 0x%08x\n", *PP32_BRK_DATA_VALUE_RD(0));
    len += sprintf(page + len, "  rdval1: 0x%08x\n", *PP32_BRK_DATA_VALUE_RD(1));
    len += sprintf(page + len, "  wrval0: 0x%08x\n", *PP32_BRK_DATA_VALUE_WR(0));
    len += sprintf(page + len, "  wrval1: 0x%08x\n", *PP32_BRK_DATA_VALUE_WR(1));

    *eof = 1;

    ASSERT(len <= 4096, "proc read buffer overflow");
    return len;
}





static char str[2048]; /* FIXME Allocate memory */
int proc_write_pp32(struct file *file __attribute__((unused)), const char *buf,
                    unsigned long count, void *data __attribute__((unused)))
{
    char *p;
    u32 addr;

    int len, rlen;

    len = count < sizeof(str) ? count : sizeof(str) - 1;
    rlen = len - copy_from_user(str, buf, len);
    while ( rlen && str[rlen - 1] <= ' ' )
        rlen--;
    str[rlen] = 0;
    for ( p = str; *p && *p <= ' '; p++, rlen-- );
    if ( !*p )
    {
        return 0;
    }

    if ( ifx_stricmp(p, "start") == 0 )
        *PP32_CTRL_CMD = PP32_CTRL_CMD_RESTART;
    else if ( ifx_stricmp(p, "stop") == 0 )
        *PP32_CTRL_CMD = PP32_CTRL_CMD_STOP;
    else if ( ifx_stricmp(p, "step") == 0 )
        *PP32_CTRL_CMD = PP32_CTRL_CMD_STEP;
    else if ( ifx_strincmp(p, "pc0 ", 4) == 0 )
    {
        p += 4;
        rlen -= 4;
        if ( ifx_stricmp(p, "off") == 0 )
        {
            *PP32_BRK_TRIG = PP32_BRK_GRPi_PCn_OFF(0, 0);
            *PP32_BRK_PC_MASK(0) = PP32_BRK_CONTEXT_MASK_EN;
            *PP32_BRK_PC(0) = 0;
        }
        else
        {
            addr = ifx_get_number(&p, &rlen, 1);
            *PP32_BRK_PC(0) = addr;
            *PP32_BRK_PC_MASK(0) = PP32_BRK_CONTEXT_MASK_EN | PP32_BRK_CONTEXT_MASK(0) | PP32_BRK_CONTEXT_MASK(1) | PP32_BRK_CONTEXT_MASK(2) | PP32_BRK_CONTEXT_MASK(3);
            *PP32_BRK_TRIG = PP32_BRK_GRPi_PCn_ON(0, 0);
        }
    }
    else if ( ifx_strincmp(p, "pc1 ", 4) == 0 )
    {
        p += 4;
        rlen -= 4;
        if ( ifx_stricmp(p, "off") == 0 )
        {
            *PP32_BRK_TRIG = PP32_BRK_GRPi_PCn_OFF(1, 1);
            *PP32_BRK_PC_MASK(1) = PP32_BRK_CONTEXT_MASK_EN;
            *PP32_BRK_PC(1) = 0;
        }
        else
        {
            addr = ifx_get_number(&p, &rlen, 1);
            *PP32_BRK_PC(1) = addr;
            *PP32_BRK_PC_MASK(1) = PP32_BRK_CONTEXT_MASK_EN | PP32_BRK_CONTEXT_MASK(0) | PP32_BRK_CONTEXT_MASK(1) | PP32_BRK_CONTEXT_MASK(2) | PP32_BRK_CONTEXT_MASK(3);
            *PP32_BRK_TRIG = PP32_BRK_GRPi_PCn_ON(1, 1);
        }
    }
    else if ( ifx_strincmp(p, "daddr0 ", 7) == 0 )
    {
        p += 7;
        rlen -= 7;
        if ( ifx_stricmp(p, "off") == 0 )
        {
            *PP32_BRK_TRIG = PP32_BRK_GRPi_DATA_ADDRn_OFF(0, 0);
            *PP32_BRK_DATA_ADDR_MASK(0) = PP32_BRK_CONTEXT_MASK_EN;
            *PP32_BRK_DATA_ADDR(0) = 0;
        }
        else
        {
            addr = ifx_get_number(&p, &rlen, 1);
            *PP32_BRK_DATA_ADDR(0) = addr;
            *PP32_BRK_DATA_ADDR_MASK(0) = PP32_BRK_CONTEXT_MASK_EN | PP32_BRK_CONTEXT_MASK(0) | PP32_BRK_CONTEXT_MASK(1) | PP32_BRK_CONTEXT_MASK(2) | PP32_BRK_CONTEXT_MASK(3);
            *PP32_BRK_TRIG = PP32_BRK_GRPi_DATA_ADDRn_ON(0, 0);
        }
    }
    else if ( ifx_strincmp(p, "daddr1 ", 7) == 0 )
    {
        p += 7;
        rlen -= 7;
        if ( ifx_stricmp(p, "off") == 0 )
        {
            *PP32_BRK_TRIG = PP32_BRK_GRPi_DATA_ADDRn_OFF(1, 1);
            *PP32_BRK_DATA_ADDR_MASK(1) = PP32_BRK_CONTEXT_MASK_EN;
            *PP32_BRK_DATA_ADDR(1) = 0;
        }
        else
        {
            addr = ifx_get_number(&p, &rlen, 1);
            *PP32_BRK_DATA_ADDR(1) = addr;
            *PP32_BRK_DATA_ADDR_MASK(1) = PP32_BRK_CONTEXT_MASK_EN | PP32_BRK_CONTEXT_MASK(0) | PP32_BRK_CONTEXT_MASK(1) | PP32_BRK_CONTEXT_MASK(2) | PP32_BRK_CONTEXT_MASK(3);
            *PP32_BRK_TRIG = PP32_BRK_GRPi_DATA_ADDRn_ON(1, 1);
        }
    }
    else
    {

        printk("echo \"<command>\" > /proc/eth/pp32\n");
        printk("  command:\n");
        printk("    start  - run pp32\n");
        printk("    stop   - stop pp32\n");
        printk("    step   - run pp32 with one step only\n");
        printk("    pc0    - pc0 <addr>/off, set break point PC0\n");
        printk("    pc1    - pc1 <addr>/off, set break point PC1\n");
        printk("    daddr0 - daddr0 <addr>/off, set break point data address 0\n");
        printk("    daddr0 - daddr1 <addr>/off, set break point data address 1\n");
        printk("    help   - print this screen\n");
    }

    if ( *PP32_BRK_TRIG )
        *PP32_CTRL_OPT = PP32_CTRL_OPT_STOP_ON_BREAKPOINT_ON;
    else
        *PP32_CTRL_OPT = PP32_CTRL_OPT_STOP_ON_BREAKPOINT_OFF;

    return count;
}

int proc_write_mem(struct file *file, const char *buf, unsigned long count, void *data) {
    char *p1, *p2;
    int len;
    int colon;
    unsigned long *p;
    unsigned long dword;
    char local_buf[1024];
    int i, n, l;

    len = sizeof(local_buf) < count ? sizeof(local_buf) - 1 : count;
    len = len - copy_from_user(local_buf, buf, len);
    local_buf[len] = 0;

    p1 = local_buf;
    p2 = NULL;
    colon = 1;
    while ( ifx_get_token(&p1, &p2, &len, &colon) )
    {
        if ( ifx_stricmp(p1, "w") == 0 || ifx_stricmp(p1, "write") == 0 || ifx_stricmp(p1, "r") == 0 || ifx_stricmp(p1, "read") == 0 )
            break;

        p1 = p2;
        colon = 1;
    }

    if ( *p1 == 'w' )
    {
		PPA_FPI_ADDR conv;
        ifx_ignore_space(&p2, &len);
        conv.addr_orig = (unsigned long *)ifx_get_number(&p2, &len, 1);
		ifx_ppa_drv_dp_sb_addr_to_fpi_addr_convert(&conv, 0);
        p = (unsigned long *)conv.addr_fpi;

        if ( (u32)p >= KSEG0 )
            while ( 1 )
            {
                ifx_ignore_space(&p2, &len);
                if ( !len || !((*p2 >= '0' && *p2 <= '9') || (*p2 >= 'a' && *p2 <= 'f') || (*p2 >= 'A' && *p2 <= 'F')) )
                    break;

                *p++ = (u32)ifx_get_number(&p2, &len, 1);
            }
    }
    else if ( *p1 == 'r' )
    {
        PPA_FPI_ADDR conv;
        ifx_ignore_space(&p2, &len);
        conv.addr_orig = (unsigned long *)ifx_get_number(&p2, &len, 1);
        p = (unsigned long *)conv.addr_fpi;

        if ( (u32)p >= KSEG0 )
        {
            ifx_ignore_space(&p2, &len);
            n = (int)ifx_get_number(&p2, &len, 0);
            if ( n )
            {
                char str[32] = {0};
                char *pch = str;
                int k;
                char c;

                n += (l = ((int)p >> 2) & 0x03);
                p = (unsigned long *)((u32)p & ~0x0F);
                for ( i = 0; i < n; i++ )
                {
                    if ( (i & 0x03) == 0 )
                    {
                        printk("%08X:", (u32)p);
                        pch = str;
                    }
                    if ( i < l )
                    {
                        printk("         ");
                        sprintf(pch, "    ");
                    }
                    else
                    {
                        dword = *p;
                        printk(" %08X", (u32)dword);
                        for ( k = 0; k < 4; k++ )
                        {
                            c = ((char*)&dword)[k];
                            pch[k] = c < ' ' ? '.' : c;
                        }
                    }
                    p++;
                    pch += 4;
                    if ( (i & 0x03) == 0x03 )
                    {
                        pch[0] = 0;
                        printk(" ; %s\n", str);
                    }
                }
                if ( (n & 0x03) != 0x00 )
                {
                    for ( k = 4 - (n & 0x03); k > 0; k-- )
                        printk("         ");
                    pch[0] = 0;
                    printk(" ; %s\n", str);
                }
            }
        }
    }

    return count;
}

#endif


/*------------------------------------------------------------------------------------------*\
 * wird beim neuaufsetzen des DMAs benutzt
\*------------------------------------------------------------------------------------------*/
void avmnet_swi_ar9_force_macs_down( avmnet_module_t *this ) {
  	AVMNET_ERR("[%s] force mac link down on module %s called. Please use suspend instead.\n", __func__, this->name);
  	dump_stack();

  	this->suspend(this, this);
}


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void avmnet_swi_ar9_reinit_macs(avmnet_module_t *this) {
    AVMNET_ERR("[%s] Re-init on module %s called. Please use resume instead.\n", __func__, this->name);
    dump_stack();

    this->resume(this, this);
}
EXPORT_SYMBOL(avmnet_swi_ar9_reinit_macs);


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int dma_shutdown_gracefully_ar9(void) {
    unsigned long org_jiffies;
    unsigned int p2_ctrl;

    if(!g_dma_device_ppe) {
    	dbg( "[%s] g_dma_device_ppe not setup -> nothing to do\n", __FUNCTION__);
    	return IFX_FAILURE;
    }

	dbg( "[%s] starting shutdown\n", __FUNCTION__);
    org_jiffies = jiffies;

	// (0) disable xmit
	g_stop_datapath = 1;

	// (1) block all incoming switch ports
	// TKL: done by datapath module calling suspend()
	// avmnet_swi_ar9_force_macs_down( avmnet_hw_config_entry->config );

    // (2) disable switch external MAC ports - keep CPU Port running
	// disable_fdma_sdma(0, NUM_OF_PORTS_INCLUDE_CPU_PORT - 1);

	// (3) receive packets, until DMA is idle
    while ( ((IFX_REG_R32(DM_RXPGCNT) & 0x0FFF) != 0 || (IFX_REG_R32(DM_RXPKTCNT) & 0x0FFF) != 0 || (IFX_REG_R32(DS_RXPGCNT) & 0x0FFF) != 0) && jiffies - org_jiffies < HZ / 10 + 1 )
        schedule();
    if ( jiffies - org_jiffies > HZ / 10 )
        err("Can not clear DM_RXPGCNT/DM_RXPKTCNT/DS_RXPGCNT");

    // (4) force-link down in CPU-Port: MII2
  	p2_ctrl = IFX_REG_R32( IFX_AR9_ETH_CTL(2));
    p2_ctrl &= ~( 1 < 31 );
    p2_ctrl |= (1 < 30);
    p2_ctrl &= ~PX_CTL_FLP;
    p2_ctrl |= PX_CTL_FLD;
    IFX_REG_W32( p2_ctrl, IFX_AR9_ETH_CTL(2));

	// (4) disable DMA in Switch (MAC + CPU Ports)
	// dbg( "[%s] shutting down FDMA & SDMA\n", __FUNCTION__);
	// disable_fdma_sdma(0, NUM_OF_PORTS_INCLUDE_CPU_PORT);

	// (5) disable dma_channels
	dbg(  "[%s] disable dma channels\n", __FUNCTION__);
	disable_dma_channel();

	// (6) recover PMAC / FDMA defaults
    // *PMAC_HD_CTL_REG = 0x0000074C;
    // *FDMA_PCTRL_REG(6) &= ~0x02;

	// (8) unregister g_dma_device_ppe
	dbg(  "[%s] unregister g_dma_device_ppe\n", __FUNCTION__);
	dma_device_unregister(g_dma_device_ppe);

	// (9) dma_device_release g_dma_device_ppe
	dbg(  "[%s] unregister g_dma_device_ppe\n", __FUNCTION__);
	dma_device_release(g_dma_device_ppe);


	dbg(  "[%s] shutdown finished\n", __FUNCTION__);
  	return IFX_SUCCESS;
}
EXPORT_SYMBOL(dma_shutdown_gracefully_ar9);

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_swi_ar9_suspend(avmnet_module_t *this, avmnet_module_t *caller)
{
    int i;
    struct avmnet_swi_ar9_context *ctx;

    ctx = (struct avmnet_swi_ar9_context *) this->priv;

    if(this->lock(this)){
        AVMNET_ERR("[%s] Interrupted while waiting for lock.\n", __func__);
        return -EINTR;
    }

    AVMNET_INFO("[%s] Called on module %s by %s.\n", __func__, this->name, caller != NULL ? caller->name:"unknown");

    if(ctx->suspend_cnt == 0){
        for(i = 0; i < num_ar9_eth_devs; ++i){
            if(!netif_queue_stopped(avm_devices[i]->device)){
                AVMNET_DEBUG("[%s] stopping queues for device %s\n", __func__, avm_devices[i]->device->name);
                netif_tx_stop_all_queues(avm_devices[i]->device);
            }
        }

        for(i = 0; i < this->num_children; ++i){
            if(this->children[i]->suspend && this->children[i] != caller){
                this->children[i]->suspend(this->children[i], this);
            }
        }
    }

    ctx->suspend_cnt++;

    this->unlock(this);
    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_swi_ar9_resume(avmnet_module_t *this, avmnet_module_t *caller)
{
    int i;
    struct avmnet_swi_ar9_context *ctx;

    ctx = (struct avmnet_swi_ar9_context *) this->priv;

    if(this->lock(this)){
        AVMNET_ERR("[%s] Interrupted while waiting for lock.\n", __func__);
        return -EINTR;
    }

    AVMNET_INFO("[%s] Called on module %s by %s.\n", __func__, this->name, caller != NULL ? caller->name:"unknown");

    if(ctx->suspend_cnt == 0){
        AVMNET_ERR("[%s] unbalanced call on %s by %s, suspend_cnt = 0.\n", __func__, this->name, caller != NULL ? caller->name:"unknown");
        up(&(ctx->mutex));
        return -EFAULT;
    }

    ctx->suspend_cnt--;

    if(ctx->suspend_cnt == 0){
        for(i = 0; i < this->num_children; ++i){
            if(this->children[i]->resume && this->children[i] != caller){
                this->children[i]->resume(this->children[i], this);
            }
        }

        for(i = 0; i < num_ar9_eth_devs; ++i){
            if(   netif_queue_stopped(avm_devices[i]->device)
               && netif_carrier_ok(avm_devices[i]->device)
               && g_stop_datapath == 0)
            {
                AVMNET_DEBUG("[%s] starting queues for device %s\n", __func__, avm_devices[i]->device->name);
                netif_tx_wake_all_queues(avm_devices[i]->device);
            }
        }
    }

    this->unlock(this);
    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

int avmnet_swi_ar9_get_itf_mask( const char *devname ) {

	int mac_mask = 0;
	int i;

	if (!devname)
	    return 0;

	AVMNET_ERR("[%s] %s\n", __func__, devname);

    for(i = 0; i < avmnet_hw_config_entry->nr_avm_devices; i++){
    	if (avmnet_hw_config_entry->avm_devices[i]->device_name &&
    	        ( strncmp( avmnet_hw_config_entry->avm_devices[i]->device_name, devname, NETDEV_NAME_LEN ) == 0)) {
    	    if ( avmnet_hw_config_entry->avm_devices[i]->mac_module )
    		    mac_mask |= ( 1 << avmnet_hw_config_entry->avm_devices[i]->mac_module->initdata.mac.mac_nr );
    	}
    }
	AVMNET_ERR("[%s] %#x.\n", __func__, mac_mask);
    return mac_mask;

}
EXPORT_SYMBOL(avmnet_swi_ar9_get_itf_mask);



/*------------------------------------------------------------------------------------------*\
 * this function is called, when Datapath (A5) modules are unloaded
\*------------------------------------------------------------------------------------------*/
void reinit_ar9_common_eth(avmnet_module_t *this) {
	int i;

    printk(KERN_ERR "Re-init AVM Net Common Datapath Driver for AR9 ...... ");

	/* common xmit-Funktionen registrieren */
	
    for (i = 0; i < num_ar9_eth_devs; i++) {
		struct net_device *dev;
		struct net_device_ops * ops;
		dev = g_eth_net_dev[i];
		ops = dev->netdev_ops;
		ops->ndo_start_xmit = &avmnet_swi_ar9_start_xmit;
		ops->ndo_tx_timeout = &avmnet_swi_ar9_tx_timeout;
		ops->ndo_do_ioctl = &avmnet_swi_ar9_ioctl;
	}

    avmnet_swi_ar9_enable_dma();
    
    // avmnet_swi_ar9_hw_setup( this );

    avmnet_swi_ar9_enable_cpu_mac_port();

    // restore NAPI poll function
    for_each_possible_cpu(i){
        struct softnet_data *queue;

        queue = &per_cpu(g_ar9_eth_queue, i);

        queue->backlog.poll = process_backlog;
        napi_enable(&queue->backlog);
    }

	/* (re-)aktivieren der MACs */
	// TKL: done in ppe_[ae]5_exit by calling resume()
	// avmnet_swi_vr9_reinit_macs(avmnet_hw_config_entry->config);

	/* disable learning, flush mac table -> not for ar9?!*/
    // avmnet_swi_ar9_disable_learning(avmnet_hw_config_entry->config);

	/* disable drop xmit packets*/
	// TKL: done in ppe_[ae]5_exit now
	//g_stop_datapath = 0;

	enable_dma_channel();

	printk(KERN_ERR "[%s] Succeeded!\n", __func__ );
}
EXPORT_SYMBOL(reinit_ar9_common_eth);

