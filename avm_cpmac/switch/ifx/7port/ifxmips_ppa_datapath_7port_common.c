/*
 *   Common Head File
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/miscdevice.h>
#include <linux/atmdev.h>
#include <linux/init.h>
#include <linux/etherdevice.h>  /*  eth_type_trans  */
#include <linux/ethtool.h>      /*  ethtool_cmd     */
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#include <linux/if_vlan.h>
#include <asm/uaccess.h>
#include <asm/addrspace.h>
#include <asm/unistd.h>
#include <asm/irq.h>
#include <asm/delay.h>
#include <asm/io.h>
#include <asm/prom.h>
#include <asm/string.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/mii.h>
#include <avm/net/net.h>

/*
 *  Chip Specific Head File
 */
#include <ifx_types.h>
#include <ifx_regs.h>
#include <common_routines.h>
#include <ifx_pmu.h>
#include <ifx_rcu.h>
#include <ifx_gpio.h>
#include <ifx_led.h>
#include <ifx_dma_core.h>
#include <ifx_clk.h>
#include <switch_api/ifx_ethsw_kernel_api.h>
#include <switch_api/ifx_ethsw_api.h>
#include <switch_api/ifx_ethsw_flow_core.h>

#include <avm/net/ifx_ppa_api.h>
#include <avm/net/ifx_ppa_stack_al.h>
#include <avm/net/ifx_ppa_api_directpath.h>
#include <avm/net/ifx_ppa_ppe_hal.h>

#include <avmnet_config.h>

#include "../common/swi_ifx_common.h"
#include "../common/ifx_ppe_drv_wrapper.h"
#include "../common/ifx_ppa_datapath.h"
#include "swi_7port.h"
#include "swi_7port.h"
#include "ifxmips_ppa_datapath_7port_common.h"



/*
 *
 *  Module Parameter
 */


#define INIT_HW_PPECONF 		(1 << 0)
#define INIT_HW_PMU 			(1 << 1)
#define INIT_HW_INTERNAL_TANTOS (1 << 2)
#define INIT_HW_DPLUS 			(1 << 3)
#define INIT_HW_DSL_HW 			(1 << 4)
#define INIT_HW_PDMA	 		(1 << 5)
#define INIT_HW_MAILBOX			(1 << 6)
#define INIT_HW_SHARE_BUFFER	(1 << 7)

int ppe_hw_init = INIT_HW_PPECONF | INIT_HW_PMU | INIT_HW_INTERNAL_TANTOS |
              INIT_HW_DPLUS | INIT_HW_DSL_HW | INIT_HW_PDMA |
              INIT_HW_MAILBOX | INIT_HW_SHARE_BUFFER;



#if 0
module_param(ppe_hw_init, int, S_IRUSR | S_IWUSR); // kann dem Kernel in der Commandline uebergebenwerden:
                                                   // z.B. ifxmips_ppa_datapath_vr9_common.ppe_hw_init=1
#endif

/*------------------------------------------------------------------------------------------*\
 * Globale Variablen
\*------------------------------------------------------------------------------------------*/
static int g_pmac_ewan = 0;

unsigned int num_registered_eth_dev = 0;
EXPORT_SYMBOL(num_registered_eth_dev);


struct dma_device_info *g_dma_device_ppe = NULL;
EXPORT_SYMBOL(g_dma_device_ppe);


volatile int g_stop_datapath = 0;
EXPORT_SYMBOL( g_stop_datapath );

struct net_device  dummy_dev;
DEFINE_PER_CPU(struct softnet_data, g_7port_eth_queue);
EXPORT_PER_CPU_SYMBOL(g_7port_eth_queue);

#ifdef CONFIG_IFX_PPE_E5_OFFCHIP_BONDING
  unsigned int g_slave_ppe_base_addr;
  EXPORT_SYMBOL(g_slave_ppe_base_addr);
  unsigned int g_ppe_base_addr = IFX_PPE_ORG(0);
  EXPORT_SYMBOL(g_ppe_base_addr);
#endif



/*------------------------------------------------------------------------------------------*\
 * local function declarations
\*------------------------------------------------------------------------------------------*/

static void switch_hw_receive(struct dma_device_info* dma_dev, int dma_chan_no) ;
static int process_backlog_common(struct napi_struct *napi, int quota);
static int eth_poll(struct napi_struct *napi __attribute__ ((unused)), int work_to_do);
unsigned char* sw_dma_buffer_alloc(int len, int* byte_offset,void** opt);
int sw_dma_buffer_free(unsigned char* dataptr,void* opt);
int dma_intr_handler(struct dma_device_info* dma_dev,int status, int dma_chan_no);
static void enable_dma_channel(void) ;
static void disable_dma_channel(void) ;
static inline int eth_xmit(struct sk_buff *skb, unsigned int eth_ifid, int ch, int spid __attribute__ ((unused)), int prio __attribute__ ((unused)));
static inline int ethtool_ioctl(struct net_device *dev __attribute__ ((unused)), struct ifreq *ifr);
static inline struct sk_buff *alloc_skb_tx(int len);
#if defined(CONFIG_AR10)
static inline void init_ema(void);
#endif

/*
 *  Hardware init functions
 */
//static inline void reset_ppe(void);
static inline void init_pmu(void);
static inline void setup_ppe_conf(void);
static inline void init_internal_tantos(void);
static inline void init_dplus_noppa(void);
#if defined(CONFIG_VR9)
static inline void init_pdma(void);
#endif
static inline void init_mailbox(void);
static inline void clear_share_buffer(void);
static inline void init_dsl_hw(void);
static inline void init_hw(void);



int ifx_ppa_eth_init(struct net_device *dev __attribute__((unused))) {
	dbg( "[%s] %d\n", __FUNCTION__, __LINE__);


#if 0
	printk(KERN_ERR "[%s] hard_header_len before: %d\n", __func__, dev->hard_header_len);
	dev->hard_header_len += 2;
	printk(KERN_ERR "[%s] hard_header_len changed: %d\n", __func__, dev->hard_header_len);
#endif

	dbg("[%s] %d hard_header_len = %d\n", __FUNCTION__, __LINE__, dev->hard_header_len);

	dbg("Activating DMA rx channels\n");

	enable_dma_channel();
//	turn_on_dma_rx(1); das pendant im e5 treiber

	dbg("[%s] %d done\n", __FUNCTION__, __LINE__);
	return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void hw223_late_init(avmnet_device_t *avm_dev __maybe_unused) {

#if defined(AVM_REGISTER_VLAN_DEVICE)
    rtnl_lock();
    avm_register_vlan_device(avm_dev->device, 2, "eth2", NULL);
    rtnl_unlock();

    rtnl_lock();
    avm_register_vlan_device(avm_dev->device, 3, "eth3", NULL);
    rtnl_unlock();
#endif
    return;
}

/*--------------------------------------------------*\
 * wird von alloc_netdev ausgefuert, dev->name ist zu
 * diesem Zeitpunkt leider noch nicht gesetzt
\*--------------------------------------------------*/
void ifx_ppa_setup_eth(struct net_device* dev) {

	AVMNET_INFO("[%s] dev=%p (%s)\n", __func__, dev, dev->name);
	ether_setup(dev); /*  assign some members */
	if (num_registered_eth_dev < MAX_ETH_INF) {
	    //TODO evtl besser ueber eth namen und dafuer in ifx_ppa_setup_priv?!
		g_eth_net_dev[num_registered_eth_dev++] = dev;
	} else {
		err("[%s] ERROR setup_dev_nr=%d  number supported devices by driver =%d\n",
				__func__, num_registered_eth_dev, MAX_ETH_INF);
	}
	return;
}

/*--------------------------------------------------*\
* wird von alloc_netdev ausgefuert, dev->name ist zu
 * diesem Zeitpunkt leider noch nicht gesetzt
 \*--------------------------------------------------*/
int avmnet_set_macaddr(int device_nr, struct net_device *dev);
void ifx_ppa_setup_priv(avmnet_device_t *avm_dev) {

	avmnet_netdev_priv_t *priv = NULL;
	struct net_device *dev = avm_dev->device;

	priv = netdev_priv(dev);
	dbg("[%s] '%s' priv = %p  avm_dev =%p \n", __FUNCTION__, dev->name, priv, avm_dev);

	if (priv) {
		priv->avm_dev = avm_dev;
		priv->lantiq_device_index = -1;
    } else {
        AVMNET_ERR("[%s] ERROR: %s avm_dev = %p\n", __func__, avm_dev->device_name, avm_dev);
        panic("no priv");
    }

    if (avm_dev->vlanID) {
        AVMNET_ERR("[%s] VlanID no realdev %s\n" , __func__, avm_dev->device_name);
        return;
    }

    if ( ! strncmp("eth0", avm_dev->device_name, sizeof("eth0")-1)) {
        if (avmnet_set_macaddr(0, dev)) {
            AVMNET_ERR("[%s] ERROR: %s set mac-Address\n" , __func__, avm_dev->device_name);
            goto bug_set_mac;
        }
    }
    if ( ! strncmp("eth1", avm_dev->device_name, sizeof("eth1")-1)) {
        if (avmnet_set_macaddr(1, dev)) {
            AVMNET_ERR("[%s] ERROR: %s set mac-Address\n" , __func__, avm_dev->device_name);
            goto bug_set_mac;
        }
    }
    if ( ! strncmp("eth2", avm_dev->device_name, sizeof("eth2")-1)) {
        if (avmnet_set_macaddr(2, dev)) {
            AVMNET_ERR("[%s] ERROR: %s set mac-Address\n" , __func__, avm_dev->device_name);
            goto bug_set_mac;
        }
    }
    if ( ! strncmp("eth3", avm_dev->device_name, sizeof("eth3")-1)) {
        if (avmnet_set_macaddr(3, dev)) {
            AVMNET_ERR("[%s] ERROR: %s set mac-Address\n" , __func__, avm_dev->device_name);
            goto bug_set_mac;
        }
    }
    if ( ! strncmp("wan", avm_dev->device_name, sizeof("wan")-1)) {
        if (avmnet_set_macaddr(4, dev)) {
            AVMNET_ERR("[%s] ERROR: %s set mac-Address\n" , __func__, avm_dev->device_name);
            goto bug_set_mac;
        }
    }
    if ( ! strncmp("wasp", avm_dev->device_name, sizeof("wasp")-1)) {
        if (avmnet_set_macaddr(-1, dev)) {
            AVMNET_ERR("[%s] ERROR: %s set mac-Address\n" , __func__, avm_dev->device_name);
            goto bug_set_mac;
        }
    }
#if defined(CONFIG_AVMNET_VLAN_MASTER)
    if ( ! strncmp("vlan_master0", avm_dev->device_name, sizeof("vlan_master0")-1)) {
        if (avmnet_set_macaddr(2, dev)) {   /*--- bekommt MAC von eth2 ---*/
            AVMNET_ERR("[%s] ERROR: %s set mac-Address\n" , __func__, avm_dev->device_name);
            goto bug_set_mac;
        }
    }
#endif
 
    {
        unsigned char i;
        for (i = 0; i < num_registered_eth_dev; i++) {
            if (g_eth_net_dev[i] == dev)
                priv->lantiq_device_index = i;
        }
    }

    netif_napi_add(dev, &priv->avm_dev->napi, eth_poll, 64);
	return;

bug_set_mac:
    panic("set mac-Address");
}


void dump_dma_device( struct dma_device_info *dma_device, char *additional_notes) {

	printk(KERN_ERR "[%pF] dump_dma_device %s '%s' \n", __builtin_return_address(0), additional_notes, dma_device->device_name);
	printk(KERN_ERR "    port_reserved              = %d\n", dma_device->port_reserved             );
	printk(KERN_ERR "    port_num                   = %d\n", dma_device->port_num                  );
	printk(KERN_ERR "    tx_burst_len               = %d\n", dma_device->tx_burst_len              );
	printk(KERN_ERR "    rx_burst_len               = %d\n", dma_device->rx_burst_len              );
	printk(KERN_ERR "    port_tx_weight             = %d\n", dma_device->port_tx_weight            );
	printk(KERN_ERR "    port_packet_drop_enable    = %d\n", dma_device->port_packet_drop_enable   );
	printk(KERN_ERR "    port_packet_drop_counter   = %d\n", dma_device->port_packet_drop_counter  );
	printk(KERN_ERR "    mem_port_control           = %d\n", dma_device->mem_port_control          );
	printk(KERN_ERR "    tx_endianness_mode         = %d\n", dma_device->tx_endianness_mode        );
	printk(KERN_ERR "    rx_endianness_mode         = %d\n", dma_device->rx_endianness_mode        );
	printk(KERN_ERR "    num_tx_chan                = %d\n", dma_device->num_tx_chan               );
	printk(KERN_ERR "    num_rx_chan                = %d\n", dma_device->num_rx_chan               );
	printk(KERN_ERR "    max_rx_chan_num            = %d\n", dma_device->max_rx_chan_num           );
	printk(KERN_ERR "    max_tx_chan_num            = %d\n", dma_device->max_tx_chan_num           );
	printk(KERN_ERR "    buffer_alloc               = %pF\n", dma_device->buffer_alloc              );
	printk(KERN_ERR "    buffer_free                = %pF\n", dma_device->buffer_free               );
	printk(KERN_ERR "    intr_handler               = %pF\n", dma_device->intr_handler              );
	printk(KERN_ERR "    activate_poll              = %pF\n", dma_device->activate_poll             );
	printk(KERN_ERR "    inactivate_poll            = %pF\n", dma_device->inactivate_poll           );
}
EXPORT_SYMBOL(dump_dma_device);

/*--------------------------------------------------*\
 * aus 7 Port Treiber
\*--------------------------------------------------*/
static int dma_setup_init(void) {
    unsigned int i, ret;
    g_dma_device_ppe=dma_device_reserve("PPE");

    if(!g_dma_device_ppe) {
        dbg_dma( "%s[%d]: Reserved with DMA core driver failed!!!\n", __func__,__LINE__);
        return -ENODEV;
    }

	if ( g_dbg_datapath & DBG_ENABLE_MASK_DUMP_INIT )
        dump_dma_device(g_dma_device_ppe, "just reserved");

    g_dma_device_ppe->buffer_alloc              = &sw_dma_buffer_alloc;
    g_dma_device_ppe->buffer_free               = &sw_dma_buffer_free;
    g_dma_device_ppe->intr_handler              = &dma_intr_handler;
    g_dma_device_ppe->num_rx_chan               = 4;
    g_dma_device_ppe->num_tx_chan               = 2;
    g_dma_device_ppe->tx_burst_len              = DMA_TX_BURST_LEN;
    g_dma_device_ppe->rx_burst_len              = DMA_RX_BURST_LEN;
    g_dma_device_ppe->tx_endianness_mode        = IFX_DMA_ENDIAN_TYPE3;
    g_dma_device_ppe->rx_endianness_mode        = IFX_DMA_ENDIAN_TYPE3;
    g_dma_device_ppe->port_packet_drop_enable   = IFX_DISABLE;

    for (i = 0; i < g_dma_device_ppe->num_rx_chan; i++) {

        // Support Re-Init
        g_dma_device_ppe->rx_chan[i]->no_cpu_data       = 0;
        g_dma_device_ppe->rx_chan[i]->peri_to_peri      = 0;
        g_dma_device_ppe->rx_chan[i]->req_irq_to_free   = -1;
        g_dma_device_ppe->rx_chan[i]->loopback_enable   = 0;
        g_dma_device_ppe->rx_chan[i]->loopback_channel_number   = 0;
#if defined (CONFIG_AVM_SCATTER_GATHER)
        g_dma_device_ppe->tx_chan[i]->scatter_gather_channel = 1;
#endif
        //
        g_dma_device_ppe->rx_chan[i]->packet_size       =  ETH_PKT_BUF_SIZE;
        g_dma_device_ppe->rx_chan[i]->control           = IFX_DMA_CH_ON;
    }
    for (i = 0; i < g_dma_device_ppe->num_tx_chan; i++) {
        // Support Re-Init
        g_dma_device_ppe->tx_chan[i]->no_cpu_data       = 0;
        g_dma_device_ppe->tx_chan[i]->peri_to_peri      = 0;
        g_dma_device_ppe->tx_chan[i]->req_irq_to_free   = -1;
        g_dma_device_ppe->tx_chan[i]->loopback_enable   = 0;
        g_dma_device_ppe->tx_chan[i]->loopback_channel_number   = 0;

        //
        if ( (i == 0) || (i == 1)) /* eth0 --> DMA Tx0 channel, eth1--> DMA Tx1 channel*/
            g_dma_device_ppe->tx_chan[i]->control       = IFX_DMA_CH_ON;
        else
            g_dma_device_ppe->tx_chan[i]->control       = IFX_DMA_CH_OFF;
    }

    g_dma_device_ppe->activate_poll     = dma_activate_poll;
    g_dma_device_ppe->inactivate_poll   = dma_inactivate_poll;

    ret = dma_device_register (g_dma_device_ppe);
    if ( ret != IFX_SUCCESS){
        dbg_dma("%s[%d]: Register with DMA core driver Failed!!!\n", __func__,__LINE__);
    }else{
    	dbg_dma("DMA setup successful\n");
    }

	if ( g_dbg_datapath & DBG_ENABLE_MASK_DUMP_INIT )
        dump_dma_device(g_dma_device_ppe, "registered");


    return ret;
}



void avmnet_swi_7port_drop_everything(void){
        ifx_ethsw_ll_DirectAccessWrite(NULL,
                                           VR9_PCE_PMAP_2_DMCPMAP_OFFSET,
                                           VR9_PCE_PMAP_2_DMCPMAP_SHIFT,
                                           VR9_PCE_PMAP_2_DMCPMAP_SIZE,
                                           0);

        ifx_ethsw_ll_DirectAccessWrite(NULL,
                                           VR9_PCE_PMAP_3_UUCMAP_OFFSET,
                                           VR9_PCE_PMAP_3_UUCMAP_SHIFT,
                                           VR9_PCE_PMAP_3_UUCMAP_SIZE,
                                           0);

        udelay(1000);
}
EXPORT_SYMBOL(avmnet_swi_7port_drop_everything);



/*--------------------------------------------------*\
 * Shutdown DMA gracefully, before A5 E5 can be initialized
\*--------------------------------------------------*/
int stop_7port_dma_gracefully(void) {

    unsigned long org_jiffies;
    int wait_cycles = 0;

    if(!g_dma_device_ppe) {
    	dbg( "[%s] g_dma_device_ppe not setup -> nothing to do\n", __FUNCTION__);
    	return IFX_FAILURE;
    }

	dbg( "[%s] starting shutdown\n", __FUNCTION__);

	// (0) disable xmit
	g_stop_datapath = 1;
	wmb();

#if defined(SG_CHECK_IF_TX_CHANNELS_EMPTY)
	while( tx_channels_busy(g_dma_device_ppe))
	    schedule();
	udelay(1000);
#endif

    // (2) disable switch external MAC ports - keep CPU Port running
	disable_fdma_sdma(0, NUM_OF_PORTS_INCLUDE_CPU_PORT - 1);

	// (3) receive packets, until DMA is idle
	{
	    // debug -> mac-linkstate really down?!
	    int i = 0;
	    for (i = 0; i < 5; i++)
	       dbg_ppe_init("mac[%d] lst=%s (%#x)", i, (SW_READ_REG32(PHY_ADDR( i )) & PHY_ADDR_LINKST_DOWN)?"down":"up", (SW_READ_REG32(PHY_ADDR( i )) & PHY_ADDR_LINKST_MASK ));
	}
    org_jiffies = jiffies;

#if defined(CONFIG_VR9)
    while ( ((IFX_REG_R32(DM_RXPGCNT) & 0x0FFF) != 0 || (IFX_REG_R32(DM_RXPKTCNT) & 0x0FFF) != 0 || (IFX_REG_R32(DS_RXPGCNT) & 0x0FFF) != 0) && jiffies - org_jiffies < HZ  ){
        wait_cycles++;
        mb();
        schedule();
    }
#elif defined(CONFIG_AR10)
    while ( ((IFX_REG_R32(DM_RXPGCNT) & 0x00FF) != 0 || (IFX_REG_R32(DM_RXPKTCNT) & 0x00FF) != 0 || (IFX_REG_R32(DS_RXPGCNT) & 0x00FF) != 0) && ( jiffies - org_jiffies <  HZ ) ){
        wait_cycles++;
        mb();
        schedule();
    }
#else
#error DMA Idle Check just for AR10 and VR9
#endif
    if (wait_cycles)
	    err(  "had to wait for %d cycles (%ld jiffies) to free dma_queue", wait_cycles, jiffies - org_jiffies);
    if ( jiffies - org_jiffies >=  HZ ){
        err("Can not clear DM_RXPGCNT %#x/ DM_RXPKTCNT %#x /DS_RXPGCNT %#x \n", IFX_REG_R32(DM_RXPGCNT),IFX_REG_R32(DM_RXPKTCNT),IFX_REG_R32(DS_RXPGCNT) );
        BUG();
    }

#if !defined(SG_CPU_PORT_OFF)
    // komisch: SDMA von port 6 laueft weiter?!
	// (4) disable DMA in Switch (MAC + CPU Ports)
	dbg( "[%s] shutting down FDMA & SDMA\n", __FUNCTION__);
	disable_fdma_sdma(0, NUM_OF_PORTS_INCLUDE_CPU_PORT);
#endif

	// (5) disable dma_channels (close channels)
	dbg(  "[%s] disable dma channels\n", __FUNCTION__);
	disable_dma_channel();

#if defined(SG_RESET_SWITCH)
	//test: reset switch
	printk("[do_switch_reset] %#x \n", *ETHSW_SWRES_REG);
	*ETHSW_SWRES_REG = GLOB_CTRL_HWRES;
	wmb();
	printk("[do_switch_reset] %#x \n", *ETHSW_SWRES_REG);
	udelay(1000);
	printk("[do_switch_reset] %#x \n", *ETHSW_SWRES_REG);
	*ETHSW_SWRES_REG = 0;
	wmb();
	udelay(1000);
	printk("[do_switch_reset] %#x \n", *ETHSW_SWRES_REG);
#endif

	// (6) recover PMAC / FDMA defaults
    *PMAC_HD_CTL_REG = 0x0000074C;
    *FDMA_PCTRL_REG(6) &= ~0x02;

    // (8) unregister and free is done in datapath driver ( we might need to stop pp32 and free skbs before )
    dbg(  "[%s] stop dma finished\n", __FUNCTION__);

  	return IFX_SUCCESS;
}
EXPORT_SYMBOL(stop_7port_dma_gracefully);


void disable_fdma_sdma(int min_port, int max_port) {
	int j;
  	dbg("[%s] \n", __FUNCTION__);
	for (j = min_port; j < max_port ; j++) {
		ifx_ethsw_ll_DirectAccessWrite(NULL, (VR9_SDMA_PCTRL_PEN_OFFSET + (j * 0x6)), VR9_SDMA_PCTRL_PEN_SHIFT, VR9_SDMA_PCTRL_PEN_SIZE, 0);
		ifx_ethsw_ll_DirectAccessWrite(NULL, (VR9_FDMA_PCTRL_EN_OFFSET + (j * 0x6)), VR9_FDMA_PCTRL_EN_SHIFT, VR9_FDMA_PCTRL_EN_SIZE, 0);
	}
}

void enable_fdma_sdma(void) {
	int j;
  	dbg( "[%s] \n", __FUNCTION__);
	for (j =0; j < NUM_OF_PORTS_INCLUDE_CPU_PORT ; j++) {
		ifx_ethsw_ll_DirectAccessWrite(NULL, (VR9_FDMA_PCTRL_EN_OFFSET + (j * 0x6)), VR9_FDMA_PCTRL_EN_SHIFT, VR9_FDMA_PCTRL_EN_SIZE, 1);
		ifx_ethsw_ll_DirectAccessWrite(NULL, (VR9_SDMA_PCTRL_PEN_OFFSET + (j * 0x6)), VR9_SDMA_PCTRL_PEN_SHIFT, VR9_SDMA_PCTRL_PEN_SIZE, 1);
	}
	disable_cpuport_flow_control();
}
EXPORT_SYMBOL(enable_fdma_sdma);

void disable_cpuport_flow_control(void){

   // disable Flow Control for CPU-Port
   ifx_ethsw_ll_DirectAccessWrite(NULL, (VR9_SDMA_PCTRL_FCEN_OFFSET + (6 * 0x6)),VR9_SDMA_PCTRL_FCEN_SHIFT, VR9_SDMA_PCTRL_FCEN_SIZE, 0);

}
EXPORT_SYMBOL(disable_cpuport_flow_control);


void show_fdma_sdma(void) {
	int j;
	int val = 0;
	for (j =0; j < NUM_OF_PORTS_INCLUDE_CPU_PORT ; j++) {
		val = 0;
		ifx_ethsw_ll_DirectAccessRead(NULL, (VR9_FDMA_PCTRL_EN_OFFSET + (j * 0x6)), VR9_FDMA_PCTRL_EN_SHIFT, VR9_FDMA_PCTRL_EN_SIZE, &val);
		dbg_ppe_init(  "[%s] port %d: fdma-state=%d\n", __FUNCTION__, j, val);
		val = 0;
		ifx_ethsw_ll_DirectAccessRead(NULL, (VR9_SDMA_PCTRL_PEN_OFFSET + (j * 0x6)), VR9_SDMA_PCTRL_PEN_SHIFT, VR9_SDMA_PCTRL_PEN_SIZE, &val);
		dbg_ppe_init( "[%s] port %d: sdma-state=%d\n", __FUNCTION__, j, val);
	}
}
EXPORT_SYMBOL(show_fdma_sdma);

/*--------------------------------------------------*\
 * Enable / Disable DMA Channels
\*--------------------------------------------------*/
static void enable_dma_channel(void) {
    struct dma_device_info* dma_dev=g_dma_device_ppe;
    unsigned int i;

    for(i=0; i<dma_dev->max_rx_chan_num; i++) {
        if ( (dma_dev->rx_chan[i])->control==IFX_DMA_CH_ON )
            dma_dev->rx_chan[i]->open(dma_dev->rx_chan[i]);
    }
}

/** Turn Off RX DMA channels */
static void disable_dma_channel() {
    struct dma_device_info* dma_dev=g_dma_device_ppe;
    unsigned int i;

    // if channels are running in loopback mode:
    // rx has to be switched of before tx

	dbg("[%s] turn off all rx channels\n", __func__);
    for (i=0; i<dma_dev->max_rx_chan_num; i++)
        dma_dev->rx_chan[i]->close(dma_dev->rx_chan[i]);

	dbg("[%s] turn off all tx channels\n", __func__);
	for (i = 0; i < dma_dev->max_tx_chan_num; i++)
		dma_dev->tx_chan[i]->close(dma_dev->tx_chan[i]);
}



/*--------------------------------------------------*\
 * DMA Polling
\*--------------------------------------------------*/
void dma_activate_poll(struct dma_device_info *dma_dev __attribute__ ((unused)))
{
    // kein NAPI-Poll mehr aus dem DMA-IRQ-Handler
    BUG();
}
EXPORT_SYMBOL(dma_activate_poll);

void dma_inactivate_poll(struct dma_device_info *dma_dev __attribute__ ((unused)))
{
    // kein NAPI-Poll mehr aus dem DMA-IRQ-Handler
    BUG();
}
EXPORT_SYMBOL(dma_inactivate_poll);

/**
* DMA Pseudo Interrupt handler.
* This function handle the DMA pseudo interrupts to handle the data packets Rx/Tx over DMA channels
* It will handle the following interrupts
*   RCV_INT: DMA receive the packet interrupt,So get from the PPE peripheral
*   TX_BUF_FULL_INT: TX channel descriptors are not availabe, so, stop the transmission
        and enable the Tx channel interrupt.
*   TRANSMIT_CPT_INT: Tx channel descriptors are availabe and resume the transmission and
        disable the Tx channel interrupt.
*/
int dma_intr_handler(struct dma_device_info* dma_dev,int status, int dma_chan_no)
{
    struct net_device* dev;
    unsigned int i;

    switch(status) {
        case RCV_INT:
            switch_hw_receive(dma_dev, dma_chan_no);
            break;
        case TX_BUF_FULL_INT:
            for(i=0; i < num_registered_eth_dev ; i++){
               avmnet_netdev_priv_t *priv;
               avmnet_device_t *avm_dev;
               dev = g_eth_net_dev[i];
               priv = netdev_priv(dev);
            	avm_dev = priv->avm_dev;
            	AVMNET_DBG_TX_QUEUE_RATE("TX_BUF_FULL_INT stop device %s" ,dev->name);
            	avm_dev->flags |= AVMNET_DEVICE_FLAG_DMA_TX_QUEUE_FULL;
               netif_stop_queue(dev);
            }
            for(i=0;i<dma_dev->max_tx_chan_num;i++) {
               if((dma_dev->tx_chan[i])->control==IFX_DMA_CH_ON)
                    dma_dev->tx_chan[i]->enable_irq(dma_dev->tx_chan[i]);
            }
            break;
        case TRANSMIT_CPT_INT:
            for(i=0;i<dma_dev->max_tx_chan_num;i++) {
               dma_dev->tx_chan[i]->disable_irq(dma_dev->tx_chan[i]);
            }
            for(i=0; i < num_registered_eth_dev ; i++){
               avmnet_netdev_priv_t *priv;
               avmnet_device_t *avm_dev;
               dev = g_eth_net_dev[i];
               priv = netdev_priv(dev);
            	avm_dev = priv->avm_dev;
            	avm_dev->flags &= ~AVMNET_DEVICE_FLAG_DMA_TX_QUEUE_FULL;
            	AVMNET_DBG_TX_QUEUE_RATE("TRANSMIT_CPT_INT wake device %s", dev->name);
               netif_wake_queue(dev);
            }
            break;
    }
    return IFX_SUCCESS;
}

#if 0
#define NR_WPS 2
struct watch_skb {
	struct sk_buff *skb;
	int wp_nr;
};

struct watch_skb watch_skbs[NR_WPS];
int wp_wrong_aligned = 0;
int wp_wrong_size = 0;

int enable_watch_skb( struct sk_buff *skb ){
	int i;
	if (!skb )
		return 0;
	for (i = 0; i < NR_WPS; i++){
		if ( ! watch_skbs[i].skb ){
			watch_skbs[i].wp_nr = i + 2; // 2-3

			if (((unsigned int)skb->head) & 0x7ff){
				//alignemnet check
				wp_wrong_aligned++ ;
				return 0;
			}
			if ((skb->end - skb->head) < 0x680){
				// length check
				wp_wrong_size++ ;
				return 0;
			}

			watch_skbs[i].skb = skb;
			set_watchpoint(watch_skbs[i].wp_nr, (unsigned int)skb->head, 0x7f, 3); // laenge = 0x680 -> max mask = 0x3ff -> double_word_mask = 0x7f
			return 1;
		}
	}
	return 0;
}

int del_watch_skb( struct sk_buff *skb ){
	int i;
	if (!skb )
		return 0;
	for ( i = 0; i < NR_WPS; i++ ){
		if ( watch_skbs[i].skb == skb ){
			del_watchpoint( watch_skbs[i].wp_nr );
			watch_skbs[i].skb = NULL;
			return 1;
		}
	}
	return 0;
}
#endif

/**
* Allocates the buffer for ethernet packet.
* This function is invoke when DMA callback function to be called
* to allocate a new buffer for Rx packets.
*/
#define RX_BYTE_OFFSET 2
unsigned char* sw_dma_buffer_alloc(int len __attribute__ ((unused)),
                                   int* byte_offset, void** opt)
{
    unsigned char *buffer=NULL;
    struct sk_buff *skb=NULL;
    unsigned int align_off = 0;

    /* for reserving 2 bytes in skb buffer, so, set offset 2 bytes infront of data pointer */
    *byte_offset = RX_BYTE_OFFSET;

    skb = dev_alloc_skb(ETH_PKT_BUF_SIZE + DMA_ALIGNMENT);
    if (likely(skb)){
        align_off = DMA_ALIGNMENT - ((u32)skb->data & (DMA_ALIGNMENT - 1));
        skb_reserve(skb, align_off + RX_BYTE_OFFSET);

        buffer = (unsigned char*)(skb->data) - RX_BYTE_OFFSET;
        *(int*)opt=(int)skb;
    } else {
        dbg_dma( "%s[%d]: Buffer allocation failed!!!\n", __func__,__LINE__);
    }

    return buffer;
}

/*
 * Free skb buffer
 * This function frees a buffer previously allocated by the DMA buffer
 * alloc callback function.
 */
int sw_dma_buffer_free(unsigned char* dataptr,void* opt) {
    struct sk_buff *skb=NULL;

    if(unlikely(opt==NULL)){
        if(dataptr){
        	dbg_dma("free dataptr");
        	/*
        	 * AVM: why does Lantiq a kfree dataptr, here,
        	 * we don't have anything to free, do we?
             * kfree(dataptr);
        	 */
        }
    }else {
        skb=(struct sk_buff*)opt;
        if(likely(skb)){
            dbg_dma("free dataptr");
            dev_kfree_skb_any(skb);
        }
    }

    return IFX_SUCCESS;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static void switch_hw_receive(struct dma_device_info* dma_dev, int dma_chan_no)
{
	unsigned char* buf = NULL;
	int len = 0;
	struct sk_buff *skb = NULL;
    struct softnet_data *queue;
    unsigned long flags;

	len = dma_device_read(dma_dev, &buf, (void**)&skb, dma_chan_no);

    if(unlikely(skb == NULL)){
        err( "%s[%d]: Can't restore the Packet !!!\n", __func__,__LINE__);
        return;
    }

	dbg_dma( "[%s] dma_device_read len:%d, skb->len:%d\n",__func__,len, skb->len);

    if(unlikely(g_stop_datapath)){
        dbg("[%s]: datapath to be stopped, dropping skb.\n", __func__);
        dev_kfree_skb_any(skb);
        return;
    }

	if(unlikely((len >= 0x600) || (len < 64))){
		err( "%s[%d]: Packet is too large/small (%d)!!!\n", __func__,__LINE__,len);
        dev_kfree_skb_any(skb);
        return;
	}

	/* do not remove CRC */
	// len -= 4;
	if(unlikely(len > (skb->end - skb->tail))){
		err( "%s[%d]: len:%d end:%p tail:%p Err!!!\n", __func__,__LINE__,(len+4), skb->end, skb->tail);
        dev_kfree_skb_any(skb);
        return;
	}

	skb_put(skb, len);

	if(buf){
	    // dbg("rx len:%d\n",len);
		dump_skb(skb, DUMP_SKB_LEN, "switch_hw_receive raw", 0, 0, 0, 0);
	}

	local_irq_save(flags);
    queue = &__get_cpu_var(g_7port_eth_queue);

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

static inline void ifx_eth_rx(avmnet_device_t *avm_dev, struct sk_buff* skb)
{
    unsigned int keep_len = 0;

    if (likely(skb->len > 0)){
        keep_len = skb->len;
    }

    // TKL: FIXME workaround gegen Haenger beim Laden der Wasp-FW, check auf carrier_ok
    if(likely(avm_dev->device && netif_running( avm_dev->device ) && netif_carrier_ok(avm_dev->device))){
        skb->dev = avm_dev->device;
        skb->protocol = eth_type_trans(skb, avm_dev->device);


#if defined(CONFIG_IP_MULTICAST_FASTFORWARD)
        {
            avmnet_netdev_priv_t *priv = netdev_priv(avm_dev->device);

            if(likely(priv->mcfw_used)){
                mcfw_snoop_recv(&priv->mcfw_netdrv, 0, skb);
            }
        }
#endif /*--- #if defined(CONFIG_IP_MULTICAST_FASTFORWARD) ---*/

        avm_dev->device_stats.rx_packets++;
        avm_dev->device_stats.rx_bytes += keep_len;

#if defined(CONFIG_AVM_PA)
        // avm_pa_set_hstart(skb, hstart);
        if(likely(avm_pa_dev_receive(AVM_PA_DEVINFO(avm_dev->device), skb) == AVM_PA_RX_ACCELERATED)){
            avm_dev->device->last_rx = jiffies;
            return;
        }
#endif /* CONFIG_AVM_PA */

        dump_skb(skb, DUMP_SKB_LEN, "switch_hw_receive to netif_receive_skb", 0, 0, 0, 0);

        netif_receive_skb(skb); //Rueckgabewert wird von hw treibern stets ignoriert vgl. e1000

    } else {
        avm_dev->device_stats.rx_dropped++;
    }
}

static int process_backlog_common(struct napi_struct *napi, int quota)
{
    struct sk_buff *skb = NULL;
    avmnet_device_t *avm_dev = NULL;
    struct pmac_header *pheader;
    int sourcePortId;
    struct softnet_data *queue = &__get_cpu_var(g_7port_eth_queue);
    unsigned long start_time = jiffies;
    unsigned int exp_len;

    int work = 0;
    unsigned long flags;

    napi->weight = 64;

    do{
        local_irq_save(flags);
        skb = __skb_dequeue(&queue->input_pkt_queue);

        if (unlikely(!skb)){
            __napi_complete(napi);
            local_irq_enable();
            break;
        }
        local_irq_restore(flags);

        pheader = (struct pmac_header*)(skb->data);
        dump_pmac_header(pheader, __FUNCTION__);

        /*
         * calculate expected SKB len.
         * packet length from PMAC header + 4 bytes for frame crc
         */
        exp_len = (pheader->PKT_LEN_HI << 8) + pheader->PKT_LEN_LO + 4u;
        if(exp_len != skb->len){
            printk(KERN_ERR "[%s] Malformed PMAC header:\n", __func__);
            dump_skb(skb, DUMP_SKB_LEN, "process_backlog_common raw data", 0, -1, 0, 1);
            dev_kfree_skb_any(skb);
            return 0;
        }

        sourcePortId = (pheader->SPPID) & 0x7;

        // remove PMAC header
        skb_pull(skb, sizeof(struct pmac_header));

        avm_dev = mac_nr_to_avm_dev(sourcePortId);      /*--- kein avm_dev gefunden - nach einem VLAN-TAG suchen ---*/
        if (avm_dev){
            ifx_eth_rx(avm_dev, skb);
        } else {
            dbg_dma("drop ");
            dev_kfree_skb_any(skb);
        }
    }while (++work < quota && jiffies == start_time);

    return work;
}

static int eth_poll(struct napi_struct *napi __attribute__ ((unused)), int work_to_do __attribute__ ((unused)))
{
    // kein NAPI-Poll mehr aus dem DMA-IRQ-Handler
	BUG();

	return 0;
}

/*------------------------------------------------------------------------------------------*\
 * Trasmit timeout
\*------------------------------------------------------------------------------------------*/
void ifx_ppa_eth_tx_timeout(struct net_device *dev) {
    struct dma_device_info* dma_dev = g_dma_device_ppe;
    unsigned int i;


    for (i=0; i<dma_dev->max_tx_chan_num; i++) {
        dma_dev->tx_chan[i]->disable_irq(dma_dev->tx_chan[i]);
    }
    AVMNET_DBG_TX_QUEUE("tx_timeout wake device %s", dev->name);
    netif_wake_queue(dev);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int ifx_ppa_eth_hard_start_xmit(struct sk_buff *skb, struct net_device *dev) {
    int if_id;
    int qid = 0;

    if_id = get_eth_port(dev);
    if (unlikely( if_id < 0 ))
        return -ENODEV;

    eth_xmit(skb, if_id, 0, 2, qid); //  spid - CPU, qid - taken from prio_queue_map

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void ifx_ppa_eth_tx_timeout_HW223(struct net_device *dev) {

    AVMNET_ERR("[%s] not used tx_timeout wake device %s", __func__, dev->name);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int ifx_ppa_eth_hard_start_xmit_HW223(struct sk_buff *skb, struct net_device *dev __attribute__((unused))) {

    AVMNET_INFO("[%s] not used\n", __func__);
    if(skb)
        dev_kfree_skb_any(skb);

    return -1;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static inline int eth_xmit(struct sk_buff *skb,
                           unsigned int eth_ifid,
                           int ch,
                           int spid __attribute__ ((unused)),
                           int prio __attribute__ ((unused))) {
    struct sw_eg_pkt_header pkth = {0};
    avmnet_netdev_priv_t *priv = NULL;
    avmnet_device_t *avm_dev = NULL;
    int len;

    struct skb_shared_info *shinfo;

    len = skb->len;
    if (unlikely(skb->len < ETH_MIN_TX_PACKET_LENGTH )){

	    /*--- 0byte padding behind skb->len, skb->len is not increased ---*/

	    if (skb_pad(skb, ETH_MIN_TX_PACKET_LENGTH - skb->len )){
		    skb = NULL;
		    goto ETH_XMIT_DROP;
	    }
	    len = ETH_MIN_TX_PACKET_LENGTH;
    }

#if defined(DEBUG_MIRROR_PROC) && DEBUG_MIRROR_PROC
    if ( g_mirror_netdev != NULL && (g_wan_itf & (1 << eth_ifid)) )
    {
        struct sk_buff *new_skb = skb_clone(skb, GFP_ATOMIC);

        if ( new_skb != NULL )
        {
            new_skb->dev = g_mirror_netdev;
            dev_queue_xmit(new_skb);
        }
    }
#endif

    dump_skb(skb, DUMP_SKB_LEN, "eth_xmit - raw data avm", eth_ifid, ch, 1, 0);

    if (likely( eth_ifid < num_registered_eth_dev )) {
        g_eth_net_dev[eth_ifid]->trans_start = jiffies;
        priv = netdev_priv(g_eth_net_dev[eth_ifid]);
        avm_dev = priv->avm_dev;
        avm_dev->device_stats.tx_packets++;
        avm_dev->device_stats.tx_bytes += len;
#       if defined(CONFIG_IP_MULTICAST_FASTFORWARD)
        if(likely(priv->mcfw_used)){
            mcfw_portset set;
            mcfw_portset_reset(&set);
            mcfw_portset_port_add(&set, 0);
            (void) mcfw_snoop_send(&priv->mcfw_netdrv, set, skb);
        }
#       endif /*--- #if defined(CONFIG_IP_MULTICAST_FASTFORWARD) ---*/
    }

	if(unlikely( g_stop_datapath ))
        goto ETH_XMIT_DROP;

#if defined (CONFIG_AVM_SCATTER_GATHER)
    /* If packet is not checksummed and device does not support
    * checksumming for this protocol, complete checksumming here.
    */
    if (likely(skb->ip_summed == CHECKSUM_PARTIAL)) {
        if(unlikely(skb_checksum_help(skb))){
            err(KERN_ERR "skb_checksum fails\n");
            goto ETH_XMIT_DROP;
        }
    }
#endif

    if (unlikely( skb_headroom(skb) < sizeof(struct sw_eg_pkt_header) )) {
        struct sk_buff *new_skb;

        new_skb = alloc_skb_tx(len);
        if ( !new_skb )
            goto ETH_XMIT_DROP;
        else {
        	dbg_dma("headroom-bedarf:%d, headroom: %d, headroom_new_skb: %d, length of new skb: %d ", sizeof(struct sw_eg_pkt_header),  skb_headroom(skb), skb_headroom(new_skb),  len);
            skb_put(new_skb, len + sizeof(struct sw_eg_pkt_header));
            memcpy(new_skb->data + sizeof(struct sw_eg_pkt_header), skb->data, len);
            dev_kfree_skb_any(skb);
            skb = new_skb;
        }
    }
    else {
        skb_push(skb, sizeof(struct sw_eg_pkt_header));
    }

    len += sizeof(struct sw_eg_pkt_header);

    pkth.dpid_en        = 1;
    pkth.dpid           = 1;    // TODO wird von PPA benutzt um eth0/eth1 zu unterscheiden
    pkth.spid           = 2;    //  CPU
    pkth.port_map_en    = 1;
    pkth.port_map_sel   = 1;
    pkth.port_map       = ( 1 << if_id_to_mac_nr( eth_ifid ) );
    pkth.lrn_dis        = 0;
    pkth.class_en       = 0;
    pkth.class          = 0;

    //dbg("port_map: %#x", (unsigned int)pkth.port_map);

#if 0
#if defined(AVM_DESTINATION_TEST) && AVM_DESTINATION_TEST
    pkth.dpid_en        = 1;
    pkth.dpid           = 1;
    pkth.spid           = spid;    //  CPU
    pkth.port_map_en    = 1;
    pkth.port_map_sel   = 1;
    pkth.port_map       = 1;
    pkth.lrn_dis        = 0;
    pkth.class_en       = 0;
    pkth.class          = 0;

#else
    pkth.spid           = spid;
    pkth.dpid           = eth_ifid;
    pkth.lrn_dis        = 0;
    pkth.class_en       = 1;
    pkth.class          = prio;
    if ( pkth.dpid < 2 )
        pkth.dpid_en    = g_egress_direct_forward_enable;
#endif
#endif

    memcpy(skb->data, &pkth, sizeof(struct sw_eg_pkt_header));

	/*--- skb_pad linearizes our skb; so we shouldnt lookup shinfo before this line ---*/
    shinfo = skb_shinfo(skb);
#if defined (CONFIG_AVM_SCATTER_GATHER)
    if(likely(shinfo && shinfo->nr_frags > 0 )) {

        unsigned long lock_flags;
        // this skb is scattered
        dbg_trace_ppa_data("send first fragment (nr_frags = %d)",shinfo->nr_frags );

        lock_flags = acquire_channel_lock(g_dma_device_ppe, ch );
        if(unlikely(send_skb_scattered_first(skb, g_dma_device_ppe , ch))) {
            err("sending first fragment failed");
            free_channel_lock(g_dma_device_ppe, ch, lock_flags );
            goto ETH_XMIT_DROP;
        }
        if(unlikely(send_skb_scattered_successors(skb, g_dma_device_ppe, ch))) {
            err("ah we could just send some of the fragmets, this must not happen!");
            // we cannot do any skb free now :-(
            free_channel_lock(g_dma_device_ppe, ch, lock_flags );
            return 0;
        }
        free_channel_lock(g_dma_device_ppe, ch, lock_flags );
    }
#else
    if(unlikely(shinfo && ( shinfo->nr_frags > 0 ))) {
        err("we don't support Scatter Gather, but we got a scattered SKB");
        goto ETH_XMIT_DROP;
    }
#endif
     else {

        // this skb is not scattered
        dbg_trace_ppa_data("send complete packet as one fragment");
        if (unlikely( dma_device_write(g_dma_device_ppe, skb->data, len, skb, ch) != len )) {
            dbg("dma write failed");
            goto ETH_XMIT_DROP;
        }
    }

    return 0;

ETH_XMIT_DROP:
	dbg("drop");
    if(skb)
        dev_kfree_skb_any(skb);
    if (avm_dev)
        avm_dev->device_stats.tx_dropped++;
    return -1;
}


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

int ifx_ppa_eth_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
    int port;

    port = get_eth_port(dev);
    if ( port < 0 )
        return -ENODEV;

    switch ( cmd )
    {
    case SIOCETHTOOL:
        return ethtool_ioctl(dev, ifr);

    /* IOCTLs to control WLAN-Offload-Target (Reset/MDIO) */
    case SIOCGMIIREG: {
        unsigned int cpmac_magpie_mdio_read(unsigned short regadr, unsigned short phyadr);
        struct mii_ioctl_data *mii_data = if_mii(ifr);

        mii_data->val_out = cpmac_magpie_mdio_read(mii_data->reg_num, mii_data->phy_id);
    }
        break;
    case SIOCSMIIREG: {
        void cpmac_magpie_mdio_write(unsigned short regadr, unsigned short phyadr, unsigned short data);
        struct mii_ioctl_data *mii_data = if_mii(ifr);

        cpmac_magpie_mdio_write(mii_data->reg_num, mii_data->phy_id, mii_data->val_in);
    }
        break;
    case SCORPION_RESET: {
        void cpmac_magpie_reset(enum avm_cpmac_magpie_reset value);
        cpmac_magpie_reset(CPMAC_MAGPIE_RESET_PULSE);
    }
        break;

    default:
        return -EOPNOTSUPP;
    }

    return 0;
}

/*  Description:
 *    Handle ioctl command SIOCETHTOOL.
 *  Input:
 *    dev --- struct net_device *, device responsing to the command.
 *    ifr --- struct ifreq *, interface request structure to pass parameters
 *            or result.
 *  Output:
 *    int --- 0:    Success
 *            else: Error Code (-EFAULT, -EOPNOTSUPP)
 */
static inline int ethtool_ioctl(struct net_device *dev __attribute__ ((unused)), struct ifreq *ifr)
{
    struct ethtool_cmd cmd;

    if ( copy_from_user(&cmd, ifr->ifr_data, sizeof(cmd)) )
        return -EFAULT;

    switch ( cmd.cmd )
    {
    case ETHTOOL_GSET:      /*  get hardware information        */
        {
            memset(&cmd, 0, sizeof(cmd));



            if ( copy_to_user(ifr->ifr_data, &cmd, sizeof(cmd)) )
                return -EFAULT;
        }
        break;
    case ETHTOOL_SSET:      /*  force the speed and duplex mode */
        {
            if ( !capable(CAP_NET_ADMIN) )
                return -EPERM;

            if ( cmd.autoneg == AUTONEG_ENABLE )
            {
                /*  set property and start autonegotiation                                  */
                /*  have to set mdio advertisement register and restart autonegotiation     */
                /*  which is a very rare case, put it to future development if necessary.   */
            }
            else
            {
            }
        }
        break;
    case ETHTOOL_GDRVINFO:  /*  get driver information          */
        {
            struct ethtool_drvinfo info;

            memset(&info, 0, sizeof(info));
            strncpy(info.driver, "AVM Treiber", sizeof(info.driver) - 1);
            strncpy(info.bus_info, "N/A", sizeof(info.bus_info) - 1);
            info.regdump_len = 0;
            info.eedump_len = 0;
            info.testinfo_len = 0;
            if ( copy_to_user(ifr->ifr_data, &info, sizeof(info)) )
                return -EFAULT;
        }
        break;
    case ETHTOOL_NWAY_RST:  /*  restart auto negotiation        */
        break;
    default:
        return -EOPNOTSUPP;
    }

    return 0;
}

/*------------------------------------------------------------------------------------------*\
 * little helper functions
\*------------------------------------------------------------------------------------------*/

static inline struct sk_buff *alloc_skb_tx(int len)
{
    struct sk_buff *skb;

    dbg_dma("headroom?!");
    skb = dev_alloc_skb(len + DMA_ALIGNMENT * 2);
    if ( skb )
    {
        skb_reserve(skb, (
        		            ~( (u32)skb->data + (DMA_ALIGNMENT - 1) )
        		            & (DMA_ALIGNMENT - 1)
        		          ) + DMA_ALIGNMENT
        		    );
        *((u32 *)skb->data - 1) = (u32)skb;
#ifndef CONFIG_MIPS_UNCACHED
        dma_cache_wback((u32)skb->data - sizeof(u32), sizeof(u32));
#endif
    }

    return skb;
}

#if defined(DEBUG_DUMP_FLAG_HEADER) && DEBUG_DUMP_FLAG_HEADER
void dump_flag_header(int fwcode, struct flag_header *header, const char *title, int is_wlan __attribute__ ((unused))) {
//    static char * mpoa_type_str[] = {
//        "EoA w/o FCS",
//        "EoA w FCS",
//        "PPPoA",
//        "IPoA"
//    };
	static char * is_vlan_str[] = { "nil", "single tag", "double tag",
			"reserved" };

	if (!(g_dbg_datapath & DBG_ENABLE_MASK_DUMP_FLAG_HEADER))
		return;

	printk("%s\n", title);

	switch (fwcode) {
	case FWCODE_ROUTING_BRIDGING_ACC_A4:
	case FWCODE_ROUTING_ACC_A5:
    case FWCODE_ROUTING_ACC_D5:
        {
            printk("  ipv4_rout_vld  = %Xh (%s)\n", (u32) header->ipv4_rout_vld,
                    header->ipv4_rout_vld ? "routing" : "bridging");
            printk("  ipv4_mc_vld    = %Xh (%s)\n", (u32) header->ipv4_mc_vld,
                    header->ipv4_mc_vld ? "multicast" : "uni-cast");
            printk("  proc_type      = %Xh (%s)\n", (u32) header->proc_type,
                    header->proc_type ? "bridging" : "routing");
            printk("  tcpudp_err     = %Xh\n", (u32) header->tcpudp_err);
            printk("  tcpudp_chk     = %Xh\n", (u32) header->tcpudp_chk);
            printk("  is_udp         = %Xh\n", (u32) header->is_udp);
            printk("  is_tcp         = %Xh\n", (u32) header->is_tcp);
            printk("  ip_inner_offset   = %Xh\n", (u32)header->ip_inner_offset);
            printk("  is_pppoes         = %Xh\n", (u32)header->is_pppoes);
            printk("  is_inner_ipv6     = %Xh\n", (u32)header->is_inner_ipv6);
            printk("  is_inner_ipv4     = %Xh\n", (u32)header->is_inner_ipv4);
            printk("  is_vlan           = %Xh (%s)\n", (u32)header->is_vlan, is_vlan_str[header->is_vlan]);
            printk("  rout_index        = %Xh\n", (u32)header->rout_index);

            printk("  dest_list      = %Xh\n", (u32)header->dest_list);
            printk("  src_itf        = %Xh\n", (u32)header->src_itf);
            printk("  tcp_rstfin     = %Xh\n", (u32)header->tcp_rstfin);
            printk("  dsl_wanqid            = %Xh\n", (u32)header->qid);
            printk("  temp_dest_list = %Xh\n", (u32)header->temp_dest_list);
            printk("  src_dir        = %Xh (from %s side)\n", (u32)header->src_dir, header->src_dir ? "WAN" : "LAN");
            printk("  acc_done       = %Xh\n", (u32)header->acc_done);
            printk("  is_outer_ipv6     = %Xh\n", (u32)header->is_outer_ipv6);
            printk("  is_outer_ipv4     = %Xh\n", (u32)header->is_outer_ipv4);
            printk("  is_tunnelled      = %Xh\n", (u32)header->is_tunnel);
            printk("  packet length     = %Xh\n", (u32)header->pkt_len);
            printk("  pl_byteoff        = %Xh\n", (u32)header->pl_byteoff);
            printk("  mpoa type         = %Xh\n", (u32)header->mpoa_type);
            printk("  ip_outer_offset   = %Xh\n", (u32)header->ip_outer_offset);
            printk("  avm_rout_index    = %Xh\n", (u32)header->avm_rout_index);
            printk("  avm_is_lan        = %Xh\n", (u32)header->avm_is_lan);
            printk("  avm_is_mc         = %Xh\n", (u32)header->avm_is_mc);
        }
		break;
	}
}


EXPORT_SYMBOL(dump_flag_header);
#endif

#if defined(DEBUG_DUMP_PMAC_HEADER) && DEBUG_DUMP_PMAC_HEADER

void dump_pmac_header(struct pmac_header *header,
		const char *title) {
	if (!(g_dbg_datapath & DBG_ENABLE_MASK_DUMP_PMAC_HEADER))
		return;

	printk("%s\n", title);
	printk(" ip offset = %#x\n", (u32) header->IPOFF);
	printk(" portmap   = %#x\n", (u32) header->PORTMAP);
	printk(" slpid     = %#x\n", (u32) header->SLPID);
	printk(" is tagged = %#x\n", (u32) header->IS_TAG);
	printk(" PPPoES    = %#x\n", (u32) header->PPPoES);
	printk(" ipv4      = %#x\n", (u32) header->IPv4);
	printk(" ipv6      = %#x\n", (u32) header->IPv6);
	printk(" mirrored  = %#x\n", (u32) header->MRR);
	printk(" PKTLEN HI = %#x\n", (u32) header->PKT_LEN_HI);
	printk(" PKTLEN LO = %#x\n", (u32) header->PKT_LEN_LO);
	printk(" SPPID     = %#x\n", (u32) header->SPPID);
	printk(" Eg Tr Cl  = %#x\n", (u32) header->Eg_Tr_CLASS);

}
EXPORT_SYMBOL(dump_pmac_header);
#endif


int mac_entry_setting(unsigned char *mac __attribute__ ((unused)),
                      uint32_t fid __attribute__ ((unused)),
                      uint32_t portid __attribute__ ((unused)),
                      uint32_t agetime __attribute__ ((unused)),
                      uint32_t st_entry __attribute__ ((unused)),
                      uint32_t action __attribute__ ((unused)))
{
    return IFX_SUCCESS;
}

EXPORT_SYMBOL(mac_entry_setting);


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
#if defined(AVMNET_DBG_TX_QUEUE_ENABLE)
int mac_dev_event(struct notifier_block  *nb, unsigned long event, void  *dev) {
    struct net_device * netdev = (struct net_device *)dev;
    char *event_name = "UNKNOWN EVENT";

    if (  strncmp(netdev->name, "wasp", 5) != 0 ) {
        return NOTIFY_DONE;
    }

    AVMNET_DBG_TX_QUEUE_RATE("[%s] event %lu name %s \n",__func__, event, netdev->name);

    switch (event) {
        case NETDEV_UP:
            event_name = "NETDEV_UP";
            break;
        case NETDEV_DOWN:
            /**
             * XXX: how to handle multiple magpies*/
            event_name = "NETDEV_DOWN";
            break;
        case NETDEV_REBOOT:
            event_name = "NETDEV_REBOOT";
            break;
        case NETDEV_CHANGE:
            event_name = "NETDEV_CHANGE";
            break;
        case NETDEV_REGISTER:
            event_name = "NETDEV_REGISTER";
            break;
        case NETDEV_UNREGISTER:
            event_name = "NETDEV_UNREGISTER";
            break;
        case NETDEV_CHANGEMTU:
            event_name = "NETDEV_CHANGEMTU";
            break;
        case NETDEV_CHANGEADDR:
            event_name = "NETDEV_CHANGEADDR";
            break;
        case NETDEV_GOING_DOWN:
            event_name = "NETDEV_GOING_DOWN";
            break;
        case NETDEV_CHANGENAME:
            event_name = "NETDEV_CHANGENAME";
            break;
        case NETDEV_FEAT_CHANGE:
            event_name = "NETDEV_FEAT_CHANGE";
            break;
        case NETDEV_BONDING_FAILOVER:
            event_name = "NETDEV_BONDING_FAILOVER";
            break;
        case NETDEV_PRE_UP:
            event_name = "NETDEV_PRE_UP";
            break;
        case NETDEV_BONDING_OLDTYPE:
            event_name = "NETDEV_BONDING_OLDTYPE";
            break;
        case NETDEV_BONDING_NEWTYPE:
            event_name = "NETDEV_BONDING_NEWTYPE";
            break;
        case NETDEV_NOTIFY_PEERS:
            event_name = "NETDEV_NOTIFY_PEERS";
            break;

        default:
            break;
    }
    AVMNET_DBG_TX_QUEUE_RATE("%s: q[0]->state: %#lx <%s>, dev->flags: %#x, dev->state: %#lx\n",
          event_name,
          netdev_get_tx_queue(netdev, 0)->state,
          netif_queue_stopped(netdev)?"stopped":"running",
          netdev->flags, netdev->state);

#if defined(MAC_DEV_EVENT_DUMP_STACK)
    dump_stack();
#endif

    return NOTIFY_DONE;
}


struct notifier_block mac_notifier = {
    .notifier_call = mac_dev_event,
};
#endif

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------------------*\
 * init
\*------------------------------------------------------------------------------------------*/
int ppe_eth_init(void) {
    int i;

    printk("Loading AVM Net Common Datapath Driver for 7Port Switch...... ");

#if defined(AVMNET_DBG_TX_QUEUE_ENABLE)
    register_netdevice_notifier(&mac_notifier);
#endif

#if defined(INIT_HW)
    printk(KERN_ERR "[%s] init_hw()\n", __func__);
    init_hw();
#endif

    init_dummy_netdev(&dummy_dev);

    for_each_possible_cpu(i) {
        struct softnet_data *queue;

        queue = &per_cpu(g_7port_eth_queue, i);

        skb_queue_head_init(&queue->input_pkt_queue);
        queue->completion_queue = NULL;
        INIT_LIST_HEAD(&queue->poll_list);

        netif_napi_add(&dummy_dev, &queue->backlog, process_backlog_common, 64);
        napi_enable(&queue->backlog);
    }

    printk(KERN_ERR "[%s] ifx_proc_file_create()\n", __func__);
	ifx_proc_file_create();

    printk(KERN_ERR "[%s] dma_setup_init()\n", __func__);
    dma_setup_init();

    return 0;
}

/*------------------------------------------------------------------------------------------*\
 * this function is called, when Datapath (A5/E5) modules are unloaded
\*------------------------------------------------------------------------------------------*/
void reinit_7port_common_eth(void) {
	unsigned int i;
	int j;

    printk(KERN_ERR "Re-init AVM Net Common Datapath Driver 7Port Switch ...... ");
#if defined(INIT_HW)
    init_hw();
#endif

	/* common xmit-Funktionen registrieren */
    dbg_ppe_init(KERN_ERR "[%s] setup common xmit/timeout/ioctl functions \n", __FUNCTION__);
	for (i = 0; i < num_registered_eth_dev; i++) {
		struct net_device *dev;
		union {
			const struct net_device_ops *ro;
			struct net_device_ops *rw;
		} ops;

		avmnet_netdev_priv_t *priv;
		dev = g_eth_net_dev[i];
		priv = netdev_priv(dev);
		ops.ro = dev->netdev_ops;
		ops.rw->ndo_start_xmit = &ifx_ppa_eth_hard_start_xmit;
		ops.rw->ndo_tx_timeout = &ifx_ppa_eth_tx_timeout;
		ops.rw->ndo_do_ioctl = &ifx_ppa_eth_ioctl;
		dev->features = priv->avm_dev->net_dev_features;
	}

    dma_setup_init();

	/* (re-)aktivieren der DMA-Ports */

	dbg("show_dma");
    show_fdma_sdma();
	enable_fdma_sdma();
	dbg("show_dma");
	show_fdma_sdma();

	/* (re-)aktivieren der MACs */
	// TKL: done in ppe_[ae]5_exit by calling resume()
	// avmnet_swi_7port_reinit_macs(avmnet_hw_config_entry->config);

	/* disable learning, flush mac table*/
	avmnet_swi_7port_disable_learning(avmnet_hw_config_entry->config);

	/* remove wan itf mask*/
	avmnet_swi_7port_set_ethwan_mask(0);

    // restore NAPI poll function. Caller must disable NAPI before calling.
    for_each_possible_cpu(j){
        struct softnet_data *queue;

        queue = &per_cpu(g_7port_eth_queue, j);

        queue->backlog.poll = process_backlog_common;
        napi_enable(&queue->backlog);
    }

	/* disable drop xmit packets*/
	// TKL: done in ppe_[ae]5_exit now
	//g_stop_datapath = 0;

	enable_dma_channel();

	printk(KERN_ERR "[%s] Succeeded!\n", __func__ );
}
EXPORT_SYMBOL(reinit_7port_common_eth);

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

#if defined(INIT_HW)
static inline void init_hw(void) {

	if (ppe_hw_init & INIT_HW_PPECONF){
	    dbg_ppe_init(KERN_ERR "[%s] setup_ppe_conf()\n", __func__);
		setup_ppe_conf();
	}

	if ( ppe_hw_init & INIT_HW_PMU ){
	    dbg_ppe_init(KERN_ERR "[%s] init_pmu()\n", __func__);
		init_pmu();
	}

	if ( ppe_hw_init & INIT_HW_INTERNAL_TANTOS ){
	    dbg_ppe_init(KERN_ERR "[%s] init_internal_tantos()\n", __func__);
    	init_internal_tantos();
	}

	dump_dplus("initial config");

	if ( ppe_hw_init & INIT_HW_DPLUS ){
	    dbg_ppe_init(KERN_ERR "[%s] init_dplus_noppa()\n", __func__);
		init_dplus_noppa();
		dump_dplus("init done");
	}

	if ( ppe_hw_init & INIT_HW_DSL_HW ){
	    dbg_ppe_init(KERN_ERR "[%s] init_dsl_hw()\n", __func__);
		init_dsl_hw();
	}

	if ( ppe_hw_init & INIT_HW_PDMA ){
	    dbg_ppe_init(KERN_ERR "[%s] init_pdma()\n", __func__);
#if defined(CONFIG_VR9)
		init_pdma();
#endif
#if defined(CONFIG_AR10)
		init_ema();
#endif
	}
	if ( ppe_hw_init & INIT_HW_MAILBOX ){
	    dbg_ppe_init(KERN_ERR "[%s] init_mailbox()\n", __func__);
		init_mailbox();
	}

	if ( ppe_hw_init & INIT_HW_SHARE_BUFFER ){
	    dbg_ppe_init(KERN_ERR "[%s] clear_share_buffer()\n", __func__);
		clear_share_buffer();
	}

	printk( KERN_ERR "[%s] ppe_hw_init=%#x successful\n", __func__, ppe_hw_init);
}


static inline void setup_ppe_conf(void) {
	// disable PPE and MIPS access to DFE memory
	u32 conf = SW_READ_REG32(RCU_PPE_CONF);
#if defined(CONFIG_AR10)
	// We need to reserve Memory for DSL-Subsystem AR10: BIT31 == 0
    conf &= 0x7FFFFFFF;
#endif
#if defined(CONFIG_VR9)
	// We need to reserve Memory for DSL-Subsystem VR9: BIT30:27 == 0
    conf &= ~0x78000000;
#endif
	SW_WRITE_REG32(conf, RCU_PPE_CONF);
}

static inline void sw_clr_rmon_counter(int port) {
	int i;

	if (port >= 0 && port < 7) {
		SW_WRITE_REG32(0, ETHSW_BM_PCFG_REG(port));
		SW_WRITE_REG32(3, ETHSW_BM_RMON_CTRL_REG(port));
		for (i = 1000; (SW_READ_REG32(ETHSW_BM_RMON_CTRL_REG(port)) & 3) != 0 && i > 0; i--)
			;
		if (i == 0)
			SW_WRITE_REG32(0, ETHSW_BM_RMON_CTRL_REG(port));
		SW_WRITE_REG32(1, ETHSW_BM_PCFG_REG(port));
	}
}

static inline void init_pmu(void) {
	//   5 - DMA, should be enabled in DMA driver
	//   9 - DSL DFE/AFE
	//  15 - AHB
	//  19 - SLL
	//  21 - PPE TC
	//  22 - PPE EMA
	//  23 - PPE DPLUS Master
	//  24 - PPE DPLUS Slave

	//  28 - SWITCH
	//  29 - PPE_TOP

	DSL_DFE_PMU_SETUP(IFX_PMU_ENABLE);
	PPE_TPE_PMU_SETUP(IFX_PMU_ENABLE);
	PPE_SLL01_PMU_SETUP(IFX_PMU_ENABLE);
	PPE_TC_PMU_SETUP(IFX_PMU_ENABLE);
	PPE_EMA_PMU_SETUP(IFX_PMU_ENABLE);
	PPE_DPLUS_PMU_SETUP(IFX_PMU_ENABLE);
	PPE_DPLUSS_PMU_SETUP(IFX_PMU_ENABLE);
	SWITCH_PMU_SETUP(IFX_PMU_ENABLE);
	PPE_TOP_PMU_SETUP(IFX_PMU_ENABLE);
	PPE_QSB_PMU_SETUP(IFX_PMU_ENABLE);
}

static inline void init_internal_tantos(void) {
	int i;

	SW_WRITE_REG32((1 << 15), GLOB_CTRL_REG); //enable Switch

	for (i = 0; i < 7; i++)
		sw_clr_rmon_counter(i);

	//*FDMA_PCTRL_REG(6) &= ~0x01;//  disable port (FDMA)
	*FDMA_PCTRL_REG(6) |= 0x02; //  insert special tag

	SW_WRITE_REG32( 0x01DC, PMAC_HD_CTL_REG); //  PMAC Head

	for (i = 6; i < 12; i++)
		*PCE_PCTRL_REG(i, 0) |= 1 << 11; //  ingress special tag

	SW_WRITE_REG32( (1518 + 8 + 4 * 2), MAC_FLEN_REG); //  MAC frame + 8-byte special tag + 4-byte VLAN tag * 2

	*MAC_CTRL_REG(6, 2) |= 1 << 3; //  enable jumbo frame

//    for ( i = 0; i < 7; i++ )
//        *PCE_PCTRL_REG(i, 2) |= 3;  //  highest class, mapped to RX channel 0

	*PCE_PMAP_REG(1) = 0x7F; //  monitoring
	//*PCE_PMAP_REG(2) = 0x7F & ~g_pmac_ewan; //  broadcast and unknown multicast
	*PCE_PMAP_REG(2) = 0x7F; //  broadcast and unknown multicast
	//*PCE_PMAP_REG(3) = 0x7F & ~g_pmac_ewan; //  unknown uni-cast
	*PCE_PMAP_REG(3) = 0x7F; //  unknown uni-cast

	SW_WRITE_REG32(g_pmac_ewan, PMAC_EWAN_REG);

	SW_WRITE_REG32(0x8F, PMAC_RX_IPG_REG);
}

void dump_dplus(const char *print_header __attribute__((unused))) {
	dbg_ppe_init("[%s] %s\n", __func__, print_header);
	dbg_ppe_init("DM_RXDB      : %#x\n", *DM_RXDB      );
	dbg_ppe_init("DM_RXCB      : %#x\n", *DM_RXCB      );
	dbg_ppe_init("DM_RXCFG     : %#x\n", *DM_RXCFG     );
	dbg_ppe_init("DM_RXPGCNT   : %#x\n", *DM_RXPGCNT   );
	dbg_ppe_init("DM_RXPKTCNT  : %#x\n", *DM_RXPKTCNT  );
	dbg_ppe_init("DS_RXDB      : %#x\n", *DS_RXDB      );
	dbg_ppe_init("DS_RXCB      : %#x\n", *DS_RXCB      );
	dbg_ppe_init("DS_RXCFG     : %#x\n", *DS_RXCFG     );
	dbg_ppe_init("DS_RXPGCNT   : %#x\n", *DS_RXPGCNT   );

}
EXPORT_SYMBOL(dump_dplus);

static inline void init_dplus_noppa(void) {
	*DM_RXCFG &= ~0x80000000; //  disable DMRX
	*DS_RXCFG &= ~0x00008000; //  disable DSRX

#define DO_RESET_DPLUS
#if defined(DO_RESET_DPLUS)
	//  reset D+
#define RST_DPLUS_BITS  (1 << 4)                //  Slave only
//#define RST_DPLUS_BITS  ((1 << 10) | (1 << 4))  //  Master & Slave
	*PP32_SRST &= ~RST_DPLUS_BITS;
	udelay(100);
	*PP32_SRST |= RST_DPLUS_BITS;
	udelay(100);

	*DM_RXCFG &= ~0x80000000; //  disable DMRX
	*DS_RXCFG &= ~0x00008000; //  disable DSRX
#endif

	*DM_RXPGCNT = 0x00002000; //  clear page pointer & counter, disable bypass
	*DS_RXPGCNT = 0x00004000; //  clear page pointer & counter, vpage update by ival
#if defined(CONFIG_AR10)
	// We need to reserve Memory for DSL-Subsystem - so RXDB-Adresses change (even in passthrough mode)
    *DM_RXDB = 0x1450;
	*DS_RXDB = 0x1450;

	*DM_RXCB = 0x143C;
	*DS_RXCB = 0x143C;
#endif

#if defined(CONFIG_VR9)
	// We need to reserve Memory for DSL-Subsystem - so RXDB-Adresses change (even in passthrough mode)
	*DM_RXDB = 0x2550;
	*DS_RXDB = 0x2550;

	*DM_RXCB = 0x27D0;
	*DS_RXCB = 0x27D0;
#endif

	*DM_RXPGCNT = 0x00100000; // set page pointer, enable bypass
	*DS_RXPGCNT = 0x00100000; // set page pointer, vpage update by DMRX_EOF_IN_INT: DMRX_CURR_PAGE_VAL

	*DM_RXPKTCNT = 0;

	*DM_RXCFG = 0x80007014;
	*DS_RXCFG = 0x0000f014;
	wmb();
	udelay(100);
}

#if defined(CONFIG_VR9)
static inline void init_pdma(void) {
	*PDMA_CFG = 0x08;
	*SAR_PDMA_RX_CMDBUF_CFG = 0x00203580;
	*SAR_PDMA_RX_FW_CMDBUF_CFG = 0x004035A0;
}
#endif

#if defined(CONFIG_AR10)
static inline void init_ema(void)
{
    *EMA_CMDCFG  = (EMA_CMD_BUF_LEN << 16) | (EMA_CMD_BASE_ADDR >> 2);
    *EMA_DATACFG = (EMA_DATA_BUF_LEN << 16) | (EMA_DATA_BASE_ADDR >> 2);
    *EMA_IER     = 0x000000FF;
    *EMA_CFG     = EMA_READ_BURST | (EMA_WRITE_BURST << 2);
}
#endif

static inline void init_mailbox(void) {
	*MBOX_IGU1_ISRC = 0xFFFFFFFF;
	*MBOX_IGU1_IER = 0x00000000; //  Don't need to enable RX interrupt, EMA driver handle RX path.
	dbg("MBOX_IGU1_IER = 0x%08X", *MBOX_IGU1_IER);
}


static inline void clear_share_buffer(void) {
#if defined(CONFIG_VR9) && 0  //TODO ist das hier moeglich?!
	volatile u32 *p;
	unsigned int i;

	p = SB_RAM0_ADDR(0);
	for (i = 0;
			i < SB_RAM0_DWLEN + SB_RAM1_DWLEN + SB_RAM2_DWLEN + SB_RAM3_DWLEN;
			i++)
		SW_WRITE_REG32(0, p++);

	p = SB_RAM6_ADDR(0);
	for (i = 0; i < SB_RAM6_DWLEN; i++)
		SW_WRITE_REG32(0, p++);
#endif

#if defined(CONFIG_AR10)
    volatile u32 *p = SB_RAM0_ADDR(0);
    unsigned int i;

    //  write all zeros only
    for ( i = 0; i < SB_RAM0_DWLEN + SB_RAM1_DWLEN + SB_RAM2_DWLEN + SB_RAM3_DWLEN + SB_RAM4_DWLEN; i++ )
        *p++ = 0;

    //  Configure share buffer master selection
    *SB_MST_PRI0 = 1;
    *SB_MST_PRI1 = 1;
#endif
}

static inline void init_dsl_hw(void) {
	/*  clear sync state    */
#if defined(CONFIG_VR9)
	*SFSM_STATE0 = 0;
	*SFSM_STATE1 = 0;
#endif
}
#endif /* #if defined(INIT_HW) */

#if defined(ENABLE_DBG_PROC) && ENABLE_DBG_PROC
#if defined(DEBUG_PP32_PROC) && DEBUG_PP32_PROC


/*
 * Syntax (Switch read/write):
 *  r sw hex_addr
 *  w sw hex_addr hex_data
 *
 * Syntax (PP32 read/write):
 *  r hex_addr
 *  w hex_addr hex_data
 */

int proc_mem_input(char *line, void *data __maybe_unused){
	char *p1, *p2;
	int len;
	int colon;
	volatile unsigned long *p;
	unsigned long dword;

	int i, n, l;

	len = strlen(line);

	p1 = line;
	p2 = NULL;
	colon = 1;
	while (ifx_get_token(&p1, &p2, &len, &colon)) {
		if (ifx_stricmp(p1, "w") == 0 || ifx_stricmp(p1, "write") == 0
				|| ifx_stricmp(p1, "r") == 0 || ifx_stricmp(p1, "read") == 0)
			break;

		p1 = p2;
		colon = 1;
	}

	if (*p1 == 'w') {
		ifx_ignore_space(&p2, &len);
		if (p2[0] == 's' && p2[1] == 'w' && (p2[2] == ' ' || p2[2] == '\t')) {
			unsigned long temp;

			p2 += 3;
			len -= 3;
			ifx_ignore_space(&p2, &len);
			temp = ifx_get_number(&p2, &len, 1);
			p = (unsigned long *) VR9_SWIP_MACRO_REG(temp);
		} else {
		    PPA_FPI_ADDR conv;
			conv.addr_orig = ifx_get_number(&p2, &len, 1);
			ifx_ppa_drv_dp_sb_addr_to_fpi_addr_convert(&conv, 0);
			p = (unsigned long *)conv.addr_fpi;
		}

		if ((u32) p >= KSEG0)
			while (1) {
				ifx_ignore_space(&p2, &len);
				if (!len
						|| !((*p2 >= '0' && *p2 <= '9')
								|| (*p2 >= 'a' && *p2 <= 'f')
								|| (*p2 >= 'A' && *p2 <= 'F')))
					break;

				*p++ = (u32) ifx_get_number(&p2, &len, 1);
			}
	} else if (*p1 == 'r') {
		ifx_ignore_space(&p2, &len);
		if (p2[0] == 's' && p2[1] == 'w' && (p2[2] == ' ' || p2[2] == '\t')) {
			unsigned long temp;

			p2 += 3;
			len -= 3;
			ifx_ignore_space(&p2, &len);
			temp = ifx_get_number(&p2, &len, 1);
			p = (unsigned long *) VR9_SWIP_MACRO_REG(temp);
		} else {
		    PPA_FPI_ADDR conv;
			conv.addr_orig = ifx_get_number(&p2, &len, 1);
			ifx_ppa_drv_dp_sb_addr_to_fpi_addr_convert(&conv, 0);
			p = (unsigned long *)conv.addr_fpi;
		}

		if ((u32) p >= KSEG0) {
			ifx_ignore_space(&p2, &len);
			n = (int) ifx_get_number(&p2, &len, 0);
			if (n) {
				char str[32] = { 0 };
				char *pch = str;
				int k;
				char c;

				n += (l = ((int) p >> 2) & 0x03);
				p = (unsigned long *) ((u32) p & ~0x0F);
				for (i = 0; i < n; i++) {
					if ((i & 0x03) == 0) {
						printk("%08X:", (u32) p);
						pch = str;
					}
					if (i < l) {
						printk("         ");
						sprintf(pch, "    ");
					} else {
						dword = *p;
						printk(" %08X", (u32) dword);
						for (k = 0; k < 4; k++) {
							c = ((char*) &dword)[k];
							pch[k] = c < ' ' ? '.' : c;
						}
					}
					p++;
					pch += 4;
					if ((i & 0x03) == 0x03) {
						pch[0] = 0;
						printk(" ; %s\n", str);
					}
				}
				if ((n & 0x03) != 0x00) {
					for (k = 4 - (n & 0x03); k > 0; k--)
						printk("         ");
					pch[0] = 0;
					printk(" ; %s\n", str);
				}
			}
		}
	}

	return 0;
}


#if defined(CONFIG_VR9)
static DEFINE_SPINLOCK(pp32_proc_spinlock);
#endif

struct register_info {
      volatile unsigned int *register_addr;
      unsigned int result;
      unsigned int smp_cpu_id;
};

static void read_register_on_this_cpu( void *info ){
    struct register_info *my_info;
   mb();
    my_info = (struct register_info*)info;
    if ( my_info ){
        printk(KERN_ERR "[%s] info: register_addr=%#x\n", __func__, (unsigned int)my_info->register_addr );
        printk(KERN_ERR "[%s] info: pre_result=%#x\n", __func__, (unsigned int)my_info->result );

        if ( my_info->register_addr ){
            my_info->result = *(my_info->register_addr);
            printk(KERN_ERR "[%s] info: post_result=%#x\n", __func__, (unsigned int)my_info->result );
        }
        else {
            printk(KERN_ERR "[%s] register_addr is null!\n", __func__);
        }
        my_info->smp_cpu_id = smp_processor_id();
    } else {
       printk(KERN_ERR "[%s] info is null!\n", __func__);
    }
}

unsigned int read_register_on_other_cpu( unsigned int register_addr ){
   struct register_info my_info;

   printk(KERN_ERR "[%s] info: register_addr=%#x\n", __func__, register_addr );
   my_info.result = 0x23234242;
   my_info.register_addr = (volatile unsigned int *)register_addr;
   mb();
   smp_call_function(read_register_on_this_cpu , &my_info, 1);
   mb();
   return my_info.result;
}

#if defined(CONFIG_VR9)
void proc_pp32_output(struct seq_file *m, void *data __maybe_unused){
    static const char *stron = " on";
    static const char *stroff = "off";

    int cur_context;
    int f_stopped;
    char str[256];
    int strlength;
    unsigned int i;
    unsigned int j;

    int pp32;
    unsigned long sys_flag;

    spin_lock_irqsave(&pp32_proc_spinlock, sys_flag);

    for ( pp32 = 0; pp32 < 2; pp32++ )
    {
        f_stopped = 0;

        seq_printf(m,  "===== pp32 core %d =====\n", pp32);

        if ( (*PP32_FREEZE & (1 << (pp32 << 4))) != 0 )
        {
            sprintf(str, "freezed");
            f_stopped = 1;
        }
        else if ( PP32_CPU_USER_STOPPED(pp32) || PP32_CPU_USER_BREAKIN_RCV(pp32) || PP32_CPU_USER_BREAKPOINT_MET(pp32) )
        {
            strlength = 0;
            if ( PP32_CPU_USER_STOPPED(pp32) )
                strlength += sprintf(str + strlength, "stopped");
            if ( PP32_CPU_USER_BREAKPOINT_MET(pp32) )
                strlength += sprintf(str + strlength, strlength ? " | breakpoint" : "breakpoint");
            if ( PP32_CPU_USER_BREAKIN_RCV(pp32) )
                sprintf(str + strlength, strlength ? " | breakin" : "breakin");
            f_stopped = 1;
        }
        else if ( PP32_CPU_CUR_PC(pp32) == PP32_CPU_CUR_PC(pp32) )
        {
            unsigned int pc_value[64] = {0};

            f_stopped = 1;
            for ( i = 0; f_stopped && i < NUM_ENTITY(pc_value); i++ )
            {
                pc_value[i] = PP32_CPU_CUR_PC(pp32);
                for ( j = 0; j < i; j++ )
                    if ( pc_value[j] != pc_value[i] )
                    {
                        f_stopped = 0;
                        break;
                    }
            }
            if ( f_stopped )
                sprintf(str, "hang");
        }
        if ( !f_stopped )
            sprintf(str, "running");
        cur_context = PP32_BRK_CUR_CONTEXT(pp32);
        {
            unsigned int own_pc;
            unsigned int smp_pc;
            udelay(1000);
            own_pc = PP32_CPU_CUR_PC(pp32);
            udelay(1000);
            smp_pc = read_register_on_other_cpu((unsigned int)PP32_DEBUG_REG_ADDR(pp32,0x0D00)) >> 16;
            udelay(1000);
            seq_printf(m,  "Context: %d, PC: 0x%04x, PC_SMP_CPU: 0x%04x, %s\n", cur_context,
                    own_pc,
                    smp_pc,
                    str);
        }
        if ( PP32_CPU_USER_BREAKPOINT_MET(pp32) )
        {
            strlength = 0;
            if ( PP32_BRK_PC_MET(pp32, 0) )
                strlength += sprintf(str + strlength, "pc0");
            if ( PP32_BRK_PC_MET(pp32, 1) )
                strlength += sprintf(str + strlength, strlength ? " | pc1" : "pc1");
            if ( PP32_BRK_DATA_ADDR_MET(pp32, 0) )
                strlength += sprintf(str + strlength, strlength ? " | daddr0" : "daddr0");
            if ( PP32_BRK_DATA_ADDR_MET(pp32, 1) )
                strlength += sprintf(str + strlength, strlength ? " | daddr1" : "daddr1");
            if ( PP32_BRK_DATA_VALUE_RD_MET(pp32, 0) )
            {
                strlength += sprintf(str + strlength, strlength ? " | rdval0" : "rdval0");
                if ( PP32_BRK_DATA_VALUE_RD_LO_EQ(pp32, 0) )
                {
                    if ( PP32_BRK_DATA_VALUE_RD_GT_EQ(pp32, 0) )
                        strlength += sprintf(str + strlength, " ==");
                    else
                        strlength += sprintf(str + strlength, " <=");
                }
                else if ( PP32_BRK_DATA_VALUE_RD_GT_EQ(pp32, 0) )
                    strlength += sprintf(str + strlength, " >=");
            }
            if ( PP32_BRK_DATA_VALUE_RD_MET(pp32, 1) )
            {
                strlength += sprintf(str + strlength, strlength ? " | rdval1" : "rdval1");
                if ( PP32_BRK_DATA_VALUE_RD_LO_EQ(pp32, 1) )
                {
                    if ( PP32_BRK_DATA_VALUE_RD_GT_EQ(pp32, 1) )
                        strlength += sprintf(str + strlength, " ==");
                    else
                        strlength += sprintf(str + strlength, " <=");
                }
                else if ( PP32_BRK_DATA_VALUE_RD_GT_EQ(pp32, 1) )
                    strlength += sprintf(str + strlength, " >=");
            }
            if ( PP32_BRK_DATA_VALUE_WR_MET(pp32, 0) )
            {
                strlength += sprintf(str + strlength, strlength ? " | wtval0" : "wtval0");
                if ( PP32_BRK_DATA_VALUE_WR_LO_EQ(pp32, 0) )
                {
                    if ( PP32_BRK_DATA_VALUE_WR_GT_EQ(pp32, 0) )
                        strlength += sprintf(str + strlength, " ==");
                    else
                        strlength += sprintf(str + strlength, " <=");
                }
                else if ( PP32_BRK_DATA_VALUE_WR_GT_EQ(pp32, 0) )
                    strlength += sprintf(str + strlength, " >=");
            }
            if ( PP32_BRK_DATA_VALUE_WR_MET(pp32, 1) )
            {
                strlength += sprintf(str + strlength, strlength ? " | wtval1" : "wtval1");
                if ( PP32_BRK_DATA_VALUE_WR_LO_EQ(pp32, 1) )
                {
                    if ( PP32_BRK_DATA_VALUE_WR_GT_EQ(pp32, 1) )
                        sprintf(str + strlength, " ==");
                    else
                        sprintf(str + strlength, " <=");
                }
                else if ( PP32_BRK_DATA_VALUE_WR_GT_EQ(pp32, 1) )
                    sprintf(str + strlength, " >=");
            }
            seq_printf(m,  "break reason: %s\n", str);
        }

        if ( f_stopped )
        {
            seq_printf(m,  "General Purpose Register (Context %d):\n", cur_context);
            for ( i = 0; i < 4; i++ )
            {
                for ( j = 0; j < 4; j++ )
                    seq_printf(m,  "   %2d: %08x", i + j * 4, *PP32_GP_CONTEXTi_REGn(pp32, cur_context, i + j * 4));
                seq_printf(m,  "\n");
            }
        }

        seq_printf(m,  "break out on: break in - %s, stop - %s\n",
                                            PP32_CTRL_OPT_BREAKOUT_ON_BREAKIN(pp32) ? stron : stroff,
                                            PP32_CTRL_OPT_BREAKOUT_ON_STOP(pp32) ? stron : stroff);
        seq_printf(m,  "     stop on: break in - %s, break point - %s\n",
                                            PP32_CTRL_OPT_STOP_ON_BREAKIN(pp32) ? stron : stroff,
                                            PP32_CTRL_OPT_STOP_ON_BREAKPOINT(pp32) ? stron : stroff);
        seq_printf(m,  "breakpoint (smp_processor_id = %d):\n", smp_processor_id());
    	udelay(1000);
        seq_printf(m,  "     pc0: 0x%08x, %s\n", *PP32_BRK_PC(pp32, 0), PP32_BRK_GRPi_PCn(pp32, 0, 0) ? "group 0" : "off");
    	udelay(1000);
        seq_printf(m,  "     pc0: 0x%08x \n", read_register_on_other_cpu((unsigned int)PP32_BRK_PC(pp32, 0)));
    	udelay(1000);
        seq_printf(m,  "     pc1: 0x%08x, %s\n", *PP32_BRK_PC(pp32, 1), PP32_BRK_GRPi_PCn(pp32, 1, 1) ? "group 1" : "off");
        seq_printf(m,  "  daddr0: 0x%08x, %s\n", *PP32_BRK_DATA_ADDR(pp32, 0), PP32_BRK_GRPi_DATA_ADDRn(pp32, 0, 0) ? "group 0" : "off");
        seq_printf(m,  "  daddr1: 0x%08x, %s\n", *PP32_BRK_DATA_ADDR(pp32, 1), PP32_BRK_GRPi_DATA_ADDRn(pp32, 1, 1) ? "group 1" : "off");
        seq_printf(m,  "  rdval0: 0x%08x\n", *PP32_BRK_DATA_VALUE_RD(pp32, 0));
        seq_printf(m,  "  rdval1: 0x%08x\n", *PP32_BRK_DATA_VALUE_RD(pp32, 1));
        seq_printf(m,  "  wrval0: 0x%08x\n", *PP32_BRK_DATA_VALUE_WR(pp32, 0));
        seq_printf(m,  "  wrval1: 0x%08x\n", *PP32_BRK_DATA_VALUE_WR(pp32, 1));
    }

    spin_unlock_irqrestore(&pp32_proc_spinlock, sys_flag);
}


int proc_pp32_input(char *line, void *data __maybe_unused){
	char *p;
	u32 addr;

	int pp32 = 0;
	unsigned long sys_flag;

	int rlen = strlen(line);

   spin_lock_irqsave(&pp32_proc_spinlock, sys_flag);

   for (p = line; *p && *p <= ' '; p++, rlen--)
		;
	if (!*p){
        spin_unlock_irqrestore(&pp32_proc_spinlock, sys_flag);
		return 0;
	}

	if (ifx_strincmp(p, "pp32 ", 5) == 0) {
		p += 5;
		rlen -= 5;

		while (rlen > 0 && *p >= '0' && *p <= '9') {
			pp32 += *p - '0';
			p++;
			rlen--;
		}
		while (rlen > 0 && *p && *p <= ' ') {
			p++;
			rlen--;
		}
	}

	if(pp32 >= PP32_NUM_CORES){
	    printk("only %d PP32 cores on system.\n", PP32_NUM_CORES);
        spin_unlock_irqrestore(&pp32_proc_spinlock, sys_flag);
	    return -EINVAL;
	}

	if (ifx_stricmp(p, "restart") == 0)
		*PP32_FREEZE &= ~(1 << (pp32 << 4));
	else if (ifx_stricmp(p, "freeze") == 0)
		*PP32_FREEZE |= 1 << (pp32 << 4);
	else if (ifx_stricmp(p, "start") == 0)
		*PP32_CTRL_CMD(pp32) = PP32_CTRL_CMD_RESTART;
	else if (ifx_stricmp(p, "stop") == 0)
		*PP32_CTRL_CMD(pp32) = PP32_CTRL_CMD_STOP;
	else if (ifx_stricmp(p, "step") == 0)
		*PP32_CTRL_CMD(pp32) = PP32_CTRL_CMD_STEP;
	else if (ifx_strincmp(p, "pc0 ", 4) == 0) {
	    printk(KERN_ERR "[%s] %d\n", __func__, __LINE__);
		p += 4;
		rlen -= 4;
		if (ifx_stricmp(p, "off") == 0) {
			*PP32_BRK_TRIG(pp32) = PP32_BRK_GRPi_PCn_OFF(0, 0);
			*PP32_BRK_PC_MASK(pp32, 0) = PP32_BRK_CONTEXT_MASK_EN;
			*PP32_BRK_PC(pp32, 0) = 0;
		} else {
			addr = ifx_get_number(&p, &rlen, 1);
			*PP32_BRK_PC(pp32, 0) = addr;
			*PP32_BRK_PC_MASK(pp32, 0) = PP32_BRK_CONTEXT_MASK_EN
					| PP32_BRK_CONTEXT_MASK(0) | PP32_BRK_CONTEXT_MASK(1)
					| PP32_BRK_CONTEXT_MASK(2) | PP32_BRK_CONTEXT_MASK(3);
			*PP32_BRK_TRIG(pp32) = PP32_BRK_GRPi_PCn_ON(0, 0);
    	    printk(KERN_ERR "[%s] %d\n", __func__, __LINE__);
		}
	} else if (ifx_strincmp(p, "pc1 ", 4) == 0) {
		p += 4;
		rlen -= 4;
		if (ifx_stricmp(p, "off") == 0) {
			*PP32_BRK_TRIG(pp32) = PP32_BRK_GRPi_PCn_OFF(1, 1);
			*PP32_BRK_PC_MASK(pp32, 1) = PP32_BRK_CONTEXT_MASK_EN;
			*PP32_BRK_PC(pp32, 1) = 0;
		} else {
			addr = ifx_get_number(&p, &rlen, 1);
			*PP32_BRK_PC(pp32, 1) = addr;
			*PP32_BRK_PC_MASK(pp32, 1) = PP32_BRK_CONTEXT_MASK_EN
					| PP32_BRK_CONTEXT_MASK(0) | PP32_BRK_CONTEXT_MASK(1)
					| PP32_BRK_CONTEXT_MASK(2) | PP32_BRK_CONTEXT_MASK(3);
			*PP32_BRK_TRIG(pp32) = PP32_BRK_GRPi_PCn_ON(1, 1);
		}
	} else if (ifx_strincmp(p, "daddr0 ", 7) == 0) {
		p += 7;
		rlen -= 7;
		if (ifx_stricmp(p, "off") == 0) {
			*PP32_BRK_TRIG(pp32) = PP32_BRK_GRPi_DATA_ADDRn_OFF(0, 0);
			*PP32_BRK_DATA_ADDR_MASK(pp32, 0) = PP32_BRK_CONTEXT_MASK_EN;
			*PP32_BRK_DATA_ADDR(pp32, 0) = 0;
		} else {
			addr = ifx_get_number(&p, &rlen, 1);
			*PP32_BRK_DATA_ADDR(pp32, 0) = addr;
			*PP32_BRK_DATA_ADDR_MASK(pp32, 0) = PP32_BRK_CONTEXT_MASK_EN
					| PP32_BRK_CONTEXT_MASK(0) | PP32_BRK_CONTEXT_MASK(1)
					| PP32_BRK_CONTEXT_MASK(2) | PP32_BRK_CONTEXT_MASK(3);
			*PP32_BRK_TRIG(pp32) = PP32_BRK_GRPi_DATA_ADDRn_ON(0, 0);
		}
	} else if (ifx_strincmp(p, "daddr1 ", 7) == 0) {
		p += 7;
		rlen -= 7;
		if (ifx_stricmp(p, "off") == 0) {
			*PP32_BRK_TRIG(pp32) = PP32_BRK_GRPi_DATA_ADDRn_OFF(1, 1);
			*PP32_BRK_DATA_ADDR_MASK(pp32, 1) = PP32_BRK_CONTEXT_MASK_EN;
			*PP32_BRK_DATA_ADDR(pp32, 1) = 0;
		} else {
			addr = ifx_get_number(&p, &rlen, 1);
			*PP32_BRK_DATA_ADDR(pp32, 1) = addr;
			*PP32_BRK_DATA_ADDR_MASK(pp32, 1) = PP32_BRK_CONTEXT_MASK_EN
					| PP32_BRK_CONTEXT_MASK(0) | PP32_BRK_CONTEXT_MASK(1)
					| PP32_BRK_CONTEXT_MASK(2) | PP32_BRK_CONTEXT_MASK(3);
			*PP32_BRK_TRIG(pp32) = PP32_BRK_GRPi_DATA_ADDRn_ON(1, 1);
		}
	}
	else if ( ifx_stricmp(p, "trace") == 0 )
	{
	    unsigned int pc0;
	    unsigned short *pc_stack;
	    unsigned int pc_stack_size = 2048;
	    unsigned int pc_sp = 0;
	    unsigned int pc_sc;
	    unsigned int pc_sc_max = 10000;
	    unsigned int i;

	    if ( (*PP32_FREEZE & (1 << (pp32 << 4))) != 0 )
	        printk("freezed");
	    else
	    {
	        if ( (*PP32_CPU_STATUS(pp32) & 0x07) == 0 )
	            //  stop pp32
	            *PP32_CTRL_CMD(pp32) = PP32_CTRL_CMD_STOP;

	        pc_stack = (unsigned short *)kmalloc(sizeof(*pc_stack) * pc_stack_size, GFP_KERNEL);
	        if ( pc_stack != NULL )
	        {
	            memset(pc_stack, 0, sizeof(*pc_stack) * pc_stack_size);

	            pc0 = *PP32_BRK_PC(pp32, 0);
	            pc_sc = 0;
	            do
	            {
	                pc_stack[pc_sp++] = (unsigned short)PP32_CPU_CUR_PC(pp32);
	                if ( pc_sp == pc_stack_size )
	                    pc_sp = 0;
	                pc_sc++;

	                *PP32_CTRL_CMD(pp32) = PP32_CTRL_CMD_STEP;
	                if ( *PP32_BRK_TRIG(pp32) )
	                    *PP32_CTRL_OPT(pp32) = PP32_CTRL_OPT_STOP_ON_BREAKPOINT_ON;
	                else
	                    *PP32_CTRL_OPT(pp32) = PP32_CTRL_OPT_STOP_ON_BREAKPOINT_OFF;
	            } while ( PP32_CPU_CUR_PC(pp32) != pc0 && pc_sc < pc_sc_max );
	            pc_stack[pc_sp++] = (unsigned short)PP32_CPU_CUR_PC(pp32);
	            if ( pc_sp == pc_stack_size )
	                pc_sp = 0;
	            pc_sc++;

	            if ( PP32_CPU_CUR_PC(pp32) != pc0 )
	                printk("can not find pc0 0x%04x in %d loops\n", pc0, pc_sc_max);
	            else
	            {
	                for ( i = 0; pc_sc-- > 0 && i < pc_stack_size; i++ )
	                {
	                    if ( pc_sp-- == 0 )
	                        pc_sp = pc_stack_size - 1;
	                    printk("%8d: %04x\n", pc_sc, (unsigned int)pc_stack[pc_sp]);
	                }
	            }

	            kfree(pc_stack);
	        }
	        else
	            printk("no memory\n");
	    }

	} else {


	    printk("echo \"<command>\" > /proc/eth/pp32\n");
	    printk("  command:\n");
	    printk("    start  - run pp32\n");
	    printk("    stop   - stop pp32\n");
	    printk("    step   - run pp32 with one step only\n");
	    printk("    pc0    - pc0 <addr>/off, set break point PC0\n");
	    printk("    pc1    - pc1 <addr>/off, set break point PC1\n");
	    printk("    daddr0 - daddr0 <addr>/off, set break point data address 0\n");
	    printk("    daddr0 - daddr1 <addr>/off, set break point data address 1\n");
	    printk("    trace  - trace to break point PC0\n");
	    printk("    help   - print this screen\n");
	}

	if (*PP32_BRK_TRIG(pp32))
	    *PP32_CTRL_OPT(pp32) = PP32_CTRL_OPT_STOP_ON_BREAKPOINT_ON;
	else
	    *PP32_CTRL_OPT(pp32) = PP32_CTRL_OPT_STOP_ON_BREAKPOINT_OFF;

    spin_unlock_irqrestore(&pp32_proc_spinlock, sys_flag);
	return 0;
}
#endif //#if defined(CONFIG_VR9)

#if defined(CONFIG_AR10)
void proc_pp32_output(struct seq_file *m, void *data __maybe_unused){
    static const char *stron = " on";
    static const char *stroff = "off";
    int strlength;
    int cur_context;
    int f_stopped = 0;
    char str[256];
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
    seq_printf(m,  "Context: %d, PC: 0x%04x, %s\n", cur_context, PP32_CPU_CUR_PC, str);

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
        seq_printf(m,  "break reason: %s\n", str);
    }

    if ( f_stopped )
    {
        seq_printf(m,  "General Purpose Register (Context %d):\n", cur_context);
        for ( i = 0; i < 4; i++ )
        {
            for ( j = 0; j < 4; j++ )
                seq_printf(m,  "   %2d: %08x", i + j * 4, *PP32_GP_CONTEXTi_REGn(cur_context, i + j * 4));
            seq_printf(m,  "\n");
        }
    }

    seq_printf(m,  "break out on: break in - %s, stop - %s\n",
            PP32_CTRL_OPT_BREAKOUT_ON_BREAKIN ? stron : stroff,
                    PP32_CTRL_OPT_BREAKOUT_ON_STOP ? stron : stroff);
    seq_printf(m,  "     stop on: break in - %s, break point - %s\n",
            PP32_CTRL_OPT_STOP_ON_BREAKIN ? stron : stroff,
                    PP32_CTRL_OPT_STOP_ON_BREAKPOINT ? stron : stroff);
    seq_printf(m,  "breakpoint:\n");
    seq_printf(m,  "     pc0: 0x%08x, %s\n", *PP32_BRK_PC(0), PP32_BRK_GRPi_PCn(0, 0) ? "group 0" : "off");
    seq_printf(m,  "     pc1: 0x%08x, %s\n", *PP32_BRK_PC(1), PP32_BRK_GRPi_PCn(1, 1) ? "group 1" : "off");
    seq_printf(m,  "  daddr0: 0x%08x, %s\n", *PP32_BRK_DATA_ADDR(0), PP32_BRK_GRPi_DATA_ADDRn(0, 0) ? "group 0" : "off");
    seq_printf(m,  "  daddr1: 0x%08x, %s\n", *PP32_BRK_DATA_ADDR(1), PP32_BRK_GRPi_DATA_ADDRn(1, 1) ? "group 1" : "off");
    seq_printf(m,  "  rdval0: 0x%08x\n", *PP32_BRK_DATA_VALUE_RD(0));
    seq_printf(m,  "  rdval1: 0x%08x\n", *PP32_BRK_DATA_VALUE_RD(1));
    seq_printf(m,  "  wrval0: 0x%08x\n", *PP32_BRK_DATA_VALUE_WR(0));
    seq_printf(m,  "  wrval1: 0x%08x\n", *PP32_BRK_DATA_VALUE_WR(1));

}

int proc_pp32_input(char *line, void *data __maybe_unused){
    char *p;
    u32 addr;

    int rlen = strlen(line);

    for ( p = line; *p && *p <= ' '; p++, rlen-- );
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

    return 0;
}
#endif //#if definde(CONFIG_AR10)
#endif
#endif //#if defined(ENABLE_DBG_PROC) && ENABLE_DBG_PROC
