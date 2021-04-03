
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

#include <linux/sched.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/moduleparam.h>
#include <linux/workqueue.h>
#include <linux/atmdev.h>
#include <avm/net/net.h>
#if __has_include(<avm/sammel/simple_proc.h>)
#include <avm/sammel/simple_proc.h>
#else
#include <linux/simple_proc.h>
#endif

#include <avmnet_debug.h>
#include <avmnet_module.h>
#include <avmnet_config.h>
#include <avmnet_multicast.h>

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

#include <avm/net/ifx_ppa_api.h>
#include <avm/net/ifx_ppa_ppe_hal.h>
#include "swi_ifx_common.h"
#include "ifx_ppe_drv_wrapper.h"

#ifdef CONFIG_AVM_PA
struct avm_pa_virt_rx_dev *ppa_virt_rx_devs[MAX_PPA_PUSHTHROUGH_DEVICES];
struct avm_pa_virt_tx_dev *ppa_virt_tx_devs[MAX_PPA_PUSHTHROUGH_DEVICES];
EXPORT_SYMBOL(ppa_virt_rx_devs);
EXPORT_SYMBOL(ppa_virt_tx_devs);
#endif

int (*ppa_pushthrough_hook)(uint32_t, struct sk_buff*, int32_t , uint32_t) = NULL;
EXPORT_SYMBOL(ppa_pushthrough_hook);

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

int check_if_avmnet_enables_ppa(void) {
	avmnet_module_t *this = avmnet_hw_config_entry->config;
    return !(this->initdata.swi.flags & SWI_DISABLE_LANTIQ_PPA);
}
EXPORT_SYMBOL(check_if_avmnet_enables_ppa);

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avm_wan_mode = AVM_WAN_MODE_NOT_CONFIGURED;
EXPORT_SYMBOL(avm_wan_mode);

int g_dbg_datapath = DBG_ENABLE_MASK_ERR ;
module_param(g_dbg_datapath, int, S_IRUSR | S_IWUSR); //kernel_commandline: swi_ifx_common.g_dbg_datapath
EXPORT_SYMBOL(g_dbg_datapath);

volatile int g_datapath_mod_count = 0;
EXPORT_SYMBOL(g_datapath_mod_count);

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

char common_proc_read_buff[COMMON_BUFF_LEN];
EXPORT_SYMBOL(common_proc_read_buff);

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

inline int avm_dev_to_mac_nr(avmnet_device_t *avm_dev)
{
    return avm_dev->mac_module->initdata.mac.mac_nr;
}
EXPORT_SYMBOL(avm_dev_to_mac_nr);

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

#ifdef CONFIG_AVM_PA
static void proc_avm_ata_dev_output(struct seq_file *m, void *data __maybe_unused){
	char avm_ata_dev_name[NETDEV_NAME_LEN];

	if (avmnet_swi_ifx_common_get_ethwan_ata_devname(avm_ata_dev_name))
		seq_printf(m, "%s", avm_ata_dev_name);
	else
		seq_printf(m, "NONE");

}
#endif

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int (*in_avm_ata_mode_hook)(void) = NULL;
EXPORT_SYMBOL(in_avm_ata_mode_hook);
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

int in_avm_ata_mode(void) {
	// printk(KERN_ERR "[%s] hook =%pF\n", __FUNCTION__,in_avm_ata_mode_hook );
    if( !in_avm_ata_mode_hook ) return 0;
    return in_avm_ata_mode_hook();
}
EXPORT_SYMBOL(in_avm_ata_mode);
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

static void proc_avm_wan_mode_output(struct seq_file *m, void *data __maybe_unused){
	switch (avm_wan_mode) {
	case AVM_WAN_MODE_NOT_CONFIGURED:
		seq_printf(m, "NONE");
		break;
	case AVM_WAN_MODE_ATM:
		seq_printf(m, "ATM");
		break;
	case AVM_WAN_MODE_PTM:
		seq_printf(m, "PTM");
		break;
	case AVM_WAN_MODE_ETH:
		seq_printf(m, "ETH");
		break;
	case AVM_WAN_MODE_KDEV:
		seq_printf(m, "KDEV");
		break;
	default:
		seq_printf(m, "UNKNOWN");
		break;
	}
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

#if defined(ENABLE_DBG_PROC) && ENABLE_DBG_PROC
#define proc_dbg(format, arg...) // printk(KERN_ERR "[%s/%d] " format, __func__, __LINE__, ##arg)

struct dbg_filter {
    unsigned int flag;
    const char *description;
    const char *keyword;
};

static const struct dbg_filter dbg_filters[] = {
    { DBG_ENABLE_MASK_ERR                   , "error print","err" },
    { DBG_ENABLE_MASK_DEBUG_PRINT           , "debug print","dbg" },
    { DBG_ENABLE_MASK_ASSERT                , "assert","assert" },
    { DBG_ENABLE_MASK_DUMP_ATM_TX           , "dump atm tx","atmtx" },
    { DBG_ENABLE_MASK_DUMP_ATM_RX           , "dump atm rx","atmrx" },
    { DBG_ENABLE_MASK_DUMP_SKB_RX           , "dump rx skb","rx" },
    { DBG_ENABLE_MASK_DUMP_SKB_TX           , "dump tx skb","tx" },
    { DBG_ENABLE_MASK_DUMP_FLAG_HEADER      , "dump flag header","flag" },
    { DBG_ENABLE_MASK_DUMP_EG_HEADER        , "dump egress header","egress" },
    { DBG_ENABLE_MASK_DUMP_INIT             , "dump init","init" },
    { DBG_ENABLE_MASK_MAC_SWAP              , "mac swap","swap" },
    { DBG_ENABLE_MASK_DUMP_PMAC_HEADER      , "dump pmac header","pmac" },
    { DBG_ENABLE_MASK_DUMP_QOS              , "dump qos","qos" },
    { DBG_ENABLE_MASK_DEBUG_DMA             , "debug dma","dma" },
    { DBG_ENABLE_MASK_DEBUG_MAILBOX         , "debug mailbox","mailbox" },
    { DBG_ENABLE_MASK_DEBUG_ALIGNMENT       , "debug alignment","alignment" },
    { DBG_ENABLE_MASK_DEBUG_TRACE_PPA_DATA  , "debug trace","trace" },
    { DBG_ENABLE_MASK_DEBUG_VDEV            , "debug vdev","vdev" },
};

static const char *proc_dbg_level0_keywords[] = {"disable", "enable" };

static void proc_dbg_output(struct seq_file *m, void *data){
    unsigned int i;
    unsigned int *mask_p = data;
    proc_dbg("seq_print_mask mask_p=%p\n", mask_p);
    for (i = 0; i < ARRAY_SIZE(dbg_filters); i++){
        seq_printf(m, "%10s - ( %20s ): %15s\n",
            dbg_filters[i].keyword,
            dbg_filters[i].description,
            (((*mask_p) & dbg_filters[i].flag)?"enabled": "disabled"));
    }
}


/*
*  char *line: input will be modified (strsep replaces space by \0)
*  int *debug_mask will be adopted by cmd in input line
*  function returns 0, if valid cmdline
*/
static int proc_dbg_input(char *line, void *data){
    unsigned int *debug_mask = data;
    unsigned int parse_level = 0;
    unsigned int result_enable = 0;
    unsigned int result_mask = 0;

    if ( line ){

        char *tok, *parse_pos;
        proc_dbg("line to parse has %lu bytes: %s\n", strlen(line), line);

        parse_pos = line;
        while ((tok = strsep( &parse_pos, " "))){
            unsigned int i;
            proc_dbg("tok: %s\n", tok);

            if ( parse_level == 0 ){

                for (i = 0; i < ARRAY_SIZE(proc_dbg_level0_keywords); i++){
                    if (strncmp(proc_dbg_level0_keywords[i], tok, strlen(proc_dbg_level0_keywords[i])) == 0) {
                        proc_dbg(" %s matched %s\n", tok, proc_dbg_level0_keywords[i]) ;
                        if ((i == 0) || (i == 1)){
                            result_enable = (i == 1);
                            break;
                        }
                    }
                    else {
                       proc_dbg(" %s did not match %s\n", tok, proc_dbg_level0_keywords[i]) ;
                    }
                }
                if ( i == ARRAY_SIZE( proc_dbg_level0_keywords ) )
                    // no level0 keyword matched
                    return 1;

            }
            else if ( parse_level == 1){
                for (i = 0; i < ARRAY_SIZE(dbg_filters); i++){
                    if (strncmp(dbg_filters[i].keyword, tok, strlen(dbg_filters[i].keyword)) == 0) {
                        result_mask = dbg_filters[i].flag;
                        proc_dbg("result_mask=%#x, (%s)\n", result_mask, proc_dbg_level0_keywords[result_enable]);
                        if (result_enable)
                            *debug_mask |= result_mask;
                        else
                            *debug_mask &= ~result_mask;
                        return 0;
                    }
                }
            }
            else {
                return 1;
            }

            parse_level += 1;
        }
        return 1;

    } else {

        return 1;
    }

}
#endif
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

void ifx_proc_file_create(void) {
   proc_mkdir("eth", NULL);

   add_simple_proc_file( "eth/avm_wan_mode", NULL, proc_avm_wan_mode_output, NULL);

#ifdef CONFIG_AVM_PA
   add_simple_proc_file( "eth/avm_ata_dev", NULL, proc_avm_ata_dev_output, NULL);
#endif

#if defined(ENABLE_DBG_PROC) && ENABLE_DBG_PROC
   add_simple_proc_file( "eth/dbg", proc_dbg_input, proc_dbg_output, &g_dbg_datapath);
#endif

#if defined(DEBUG_PP32_PROC) && DEBUG_PP32_PROC
   add_simple_proc_file( "eth/pp32", proc_pp32_input, proc_pp32_output, NULL);
   add_simple_proc_file( "eth/mem", proc_mem_input, NULL, NULL);

#endif

}


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

#if defined(DEBUG_DUMP_SKB) && DEBUG_DUMP_SKB
void dump_skb(struct sk_buff *skb, u32 len, const char *title, int port, int ch, int is_tx, int enforce) {
    unsigned int i;

    if ( !enforce && !(g_dbg_datapath & (is_tx ? DBG_ENABLE_MASK_DUMP_SKB_TX : DBG_ENABLE_MASK_DUMP_SKB_RX)) )
        return;

    if ( skb->len < len )
        len = skb->len;

    if ( len > (ETH_PKT_BUF_SIZE  + 100 ))
    {
        printk("too big data length: skb = %08x, skb->data = %08x, skb->len = %d\n", (u32)skb, (u32)skb->data, skb->len);
        return;
    }

    if ( ch >= 0 )
        printk("%s (port %d, ch %d)\n", title, port, ch);
    else
        printk("%s\n", title);
    printk("  skb->data = %08X, skb->tail = %08X, skb->len = %d, skb_prio=%d\n", (u32)skb->data, (u32)skb->tail, (int)skb->len, skb->priority);
    for ( i = 1; i <= len; i++ )
    {
        if ( i % 16 == 1 )
            printk("  %#x  %4d:", (((u32)skb->data) + i - 1), (i - 1));
        printk(" %02X", (int)(*((char*)skb->data + i - 1) & 0xFF));
        if ( i % 16 == 0 )
            printk("\n");
    }
    if ( (i - 1) % 16 != 0 )
        printk("\n");
}
EXPORT_SYMBOL(dump_skb);
#endif

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static avmnet_device_t *lookup_table[MAX_ETH_INF];
void init_avm_dev_lookup(void) {

    unsigned long flags;
    int i;

    avmnet_device_t **avm_devices;
    local_irq_save(flags);
    avm_devices = avmnet_hw_config_entry->avm_devices;

    for (i = 0; i < MAX_ETH_INF; i++) {
        lookup_table[i] = NULL;
    }

    for(i = 0; i < avmnet_hw_config_entry->nr_avm_devices; i++){
        if(avm_devices[i]->mac_module) {
            if(avm_devices[i]->mac_module->initdata.mac.mac_nr < MAX_ETH_INF) {
                AVMNET_INFO("[%s] set up lookup_table mac %d - %s\n", __func__, 
                            avm_devices[i]->mac_module->initdata.mac.mac_nr, 
                            avm_devices[i]->device_name);
                lookup_table[avm_devices[i]->mac_module->initdata.mac.mac_nr] = avm_devices[i];
            } else {
                panic("invalid mac_nr");
            }
        }
    }

    AVMNET_INFO("[%s] set up lookup_table\n", __FUNCTION__);
    local_irq_restore(flags);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
avmnet_device_t *mac_nr_to_avm_dev(int mac_nr) {

    if (likely(mac_nr < MAX_ETH_INF))
        return lookup_table[mac_nr];
    return 0;
}
EXPORT_SYMBOL(mac_nr_to_avm_dev);

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

/*
 * returns: 
 *	0-10MBit 
 *	1-100MBit 
 *	2-1GBit 
 *	3-undefined ---
 */
unsigned int speed_on_mac_nr( int mac_nr ){
	avmnet_device_t *avm_dev = mac_nr_to_avm_dev(mac_nr);
	if (! avm_dev )
		return 3;
	return avm_dev->status.Details.speed;
}
EXPORT_SYMBOL(speed_on_mac_nr);

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

/*
 * setup wan dev by name in AVM_PA flags, and remove wan_port from multicast returns mac_wan_mask
 */
int avmnet_swi_ifx_common_set_ethwan_dev_by_name( const char *devname ) {
	int wan_phy_mask = 0;
	int i;

	if (!devname)
	    devname = "";

	AVMNET_INFO("[%s] %s\n", __func__, devname);

    for(i = 0; i < avmnet_hw_config_entry->nr_avm_devices; i++){
#ifdef CONFIG_AVM_PA
        struct avm_pa_pid_hwinfo *hw_info = NULL;
        if (avmnet_hw_config_entry->avm_devices[i]->device)
                hw_info = avm_pa_pid_get_hwinfo(avmnet_hw_config_entry->avm_devices[i]->device->avm_pa.devinfo.pid_handle);
#endif

    	if (avmnet_hw_config_entry->avm_devices[i]->device_name &&
    	        ( strncmp( avmnet_hw_config_entry->avm_devices[i]->device_name, devname, NETDEV_NAME_LEN ) == 0)) {
    	    if ( avmnet_hw_config_entry->avm_devices[i]->mac_module ) {

    	        // add if to wan_phy_mask
    		    wan_phy_mask |= ( 1 << avmnet_hw_config_entry->avm_devices[i]->mac_module->initdata.mac.mac_nr );

#ifdef CONFIG_AVM_PA
    		    //set ATA FLAG in AVM_PA HWINFO
    		    if (hw_info){
                	AVMNET_INFO("[%s] set hw info ATA\n", __func__);
                    hw_info->flags |= AVMNET_DEVICE_IFXPPA_ETH_WAN;
    		    }
#endif

    	    }

    	    // unregister ATA DEV in MULTICAST-Device-Group
            if(avmnet_hw_config_entry->avm_devices[i]->device != NULL) { /* NULL before netdev initialization */
                avmnet_mcfw_disable( avmnet_hw_config_entry->avm_devices[i]->device );
            }

    	} else {
#ifdef CONFIG_AVM_PA
            //reset ATA FLAG in AVM_PA HWINFO
            if ( avmnet_hw_config_entry->avm_devices[i]->mac_module
                    && hw_info
                    && (hw_info->flags & AVMNET_DEVICE_IFXPPA_ETH_WAN )){
                AVMNET_INFO("[%s] reset hw info ATA\n", __func__);
                hw_info->flags &= ~AVMNET_DEVICE_IFXPPA_ETH_WAN;
            }
#endif

    	    // register ATA DEV in MULTICAST-Device-Group
            if(avmnet_hw_config_entry->avm_devices[i]->device != NULL) { /* NULL before netdev initialization */
                avmnet_mcfw_enable( avmnet_hw_config_entry->avm_devices[i]->device );
            }
    	}
    }
    return wan_phy_mask;
}
EXPORT_SYMBOL(avmnet_swi_ifx_common_set_ethwan_dev_by_name);

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
#ifdef CONFIG_AVM_PA
int avmnet_swi_ifx_common_get_ethwan_ata_devname( char *devname ) {
	int i;
	int wan_phy_mask = 0;

	for(i = 0; i < avmnet_hw_config_entry->nr_avm_devices; i++){
		struct avm_pa_pid_hwinfo *hw_info = NULL;

		if ( !avmnet_hw_config_entry->avm_devices[i]->mac_module )
			continue;

		if (avmnet_hw_config_entry->avm_devices[i]->device)
			hw_info = avm_pa_pid_get_hwinfo(avmnet_hw_config_entry->avm_devices[i]->device->avm_pa.devinfo.pid_handle);

		if (hw_info && (hw_info->flags & AVMNET_DEVICE_IFXPPA_ETH_WAN)){

			wan_phy_mask = ( 1 << avmnet_hw_config_entry->avm_devices[i]->mac_module->initdata.mac.mac_nr );

			if ( avmnet_hw_config_entry->avm_devices[i]->device_name )
				snprintf(devname, NETDEV_NAME_LEN , "%s", avmnet_hw_config_entry->avm_devices[i]->device_name );

			return wan_phy_mask;
		}
	}

	*devname = 0;
	return 0;
}
EXPORT_SYMBOL(avmnet_swi_ifx_common_get_ethwan_ata_devname);
#endif


#ifdef CONFIG_AVM_PA
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
#define PUSHTHROUGH_DATA_PATH_NOT_READY   2
#define PUSHTHROUGH_DEV_UNKNOWN           1
#define PACKET_SENT                       0
#define PACKET_DROPPED                   -1

/*------------------------------------------------------------------------------------------*\
 * Returns:
 *  0 <= Packet was absorbed by HW-PPA (sent or dropped)
 *  0 >  Packet has to be processed by AVM_PA
\*------------------------------------------------------------------------------------------*/
int ifx_ppa_try_to_accelerate( avm_pid_handle pid_handle, struct sk_buff *skb ){
    struct avm_pa_pid_hwinfo *hwinfo = NULL;
    struct avm_pa_virt_rx_dev *virtdev = NULL;
    int hw_dev_id;

    hwinfo = avm_pa_pid_get_hwinfo( pid_handle );
    if (hwinfo)
        virtdev = hwinfo->virt_rx_dev;

    if ( !virtdev
        || ( virtdev->hw_dev_id > MAX_PPA_PUSHTHROUGH_DEVICES - 1)
        || ( virtdev != ppa_virt_rx_devs[ virtdev->hw_dev_id ] )){

            dbg( "hw_dev_id is not registerd for avm_pid_handle=%d", pid_handle );
            if (virtdev)
                virtdev->hw_pkt_try_to_acc_virt_chan_not_ready ++;

            return PUSHTHROUGH_DEV_UNKNOWN;
    }
    hw_dev_id = virtdev->hw_dev_id;

    dbg_trace_ppa_data("virt_dev_id %d",hw_dev_id );
    if ( ppa_pushthrough_hook ){
        int ret_val;

        dbg_trace_ppa_data("virt_dev_id %d",hw_dev_id );
        /* datapath module is ready for pushthrough
         * returns
         *  0  := packet sent
         *  -1 := packet dropped
         */
        ret_val = ppa_pushthrough_hook( hw_dev_id, skb, 0, 0 );
        if (ret_val < 0 )
            virtdev->hw_pkt_try_to_acc_dropped++;
        else
            virtdev->hw_pkt_try_to_acc++;

        return ret_val;

    } else {

        virtdev->hw_pkt_try_to_acc_virt_chan_not_ready ++;
        // datapath module is not ready for pushthrough, call rx_second_part from device
        return PUSHTHROUGH_DATA_PATH_NOT_READY;

    }

}

EXPORT_SYMBOL(ifx_ppa_try_to_accelerate);
/*------------------------------------------------------------------------------------------*\
 * Returns
 *  ( virt_dev_id >= 0 ): success
 *  (               -1 ): failed
\*------------------------------------------------------------------------------------------*/

int ifx_ppa_alloc_virtual_rx_device( avm_pid_handle pid_handle ){
    struct avm_pa_pid_hwinfo *hwinfo;
    struct avm_pa_virt_rx_dev *virtdev;
    int i;

    virtdev = kmalloc(sizeof(struct avm_pa_virt_rx_dev), GFP_KERNEL);

    if (!virtdev)
        return ALLOC_VIRT_DEV_FAILED;

    memset(virtdev, 0, sizeof(struct avm_pa_virt_rx_dev));
    virtdev->pid_handle = pid_handle;

    dbg("try to alloc virtual rx for pid %d", pid_handle);
    for (i=0; i < MAX_PPA_PUSHTHROUGH_DEVICES; i++){
        if ( ppa_virt_rx_devs[i] == NULL ){
            ppa_virt_rx_devs[i] = virtdev;
            virtdev->hw_dev_id = i;
            hwinfo = avm_pa_pid_get_hwinfo( pid_handle );

            if ( !hwinfo ){
                struct avm_pa_pid_hwinfo tmp_hwinfo = {0};
                dbg("hwinfo not set yet -> create new hwinfo for pid=%d", pid_handle);
                avm_pa_pid_set_hwinfo( pid_handle, &tmp_hwinfo );
                hwinfo = avm_pa_pid_get_hwinfo( pid_handle );
            }

            if ( hwinfo ){
            	hwinfo->flags |= AVMNET_DEVICE_IFXPPA_VIRT_RX;
            	hwinfo->virt_rx_dev = virtdev;
            }
            else {
            	kfree(virtdev);
            	return ALLOC_VIRT_DEV_FAILED;
            }
            return i;
        }
    }

    err("alloc virtual rx for pid %d FAILED ", pid_handle);
    if (virtdev)
        kfree(virtdev);
    return ALLOC_VIRT_DEV_FAILED;
}
EXPORT_SYMBOL(ifx_ppa_alloc_virtual_rx_device);


/*------------------------------------------------------------------------------------------*\
 * Returns
 *  ( virt_dev_id >= 0 ): success
 *  (               -1 ): failed
\*------------------------------------------------------------------------------------------*/
int ifx_ppa_alloc_virtual_tx_device(avm_pid_handle pid_handle){
    struct avm_pa_pid_hwinfo *hwinfo;
    struct avm_pa_virt_tx_dev *virtdev;
    int i;

    virtdev = kmalloc(sizeof(struct avm_pa_virt_tx_dev), GFP_KERNEL);

    if (!virtdev)
        return ALLOC_VIRT_DEV_FAILED;

    memset(virtdev, 0, sizeof(struct avm_pa_virt_tx_dev));
    virtdev->pid_handle = pid_handle;

    dbg("try to alloc virtual tx for pid %d", pid_handle);
    for (i=0; i < MAX_PPA_PUSHTHROUGH_DEVICES; i++){
        if ( (ppa_virt_tx_devs[i] == NULL) && (ppa_virt_rx_devs[i] != NULL) ){
            /*--- we only allow a tx-channel if a rx-channel exists ---*/
            if (ppa_virt_rx_devs[i]->pid_handle != pid_handle) {
                goto failed;
            }

            ppa_virt_tx_devs[i] = virtdev;
            virtdev->hw_dev_id = i;
            hwinfo = avm_pa_pid_get_hwinfo( pid_handle );

            if ( !hwinfo ){
                struct avm_pa_pid_hwinfo tmp_hwinfo = {0};
                dbg("hwinfo not set yet -> create new hwinfo for pid=%d", pid_handle);
                avm_pa_pid_set_hwinfo( pid_handle, &tmp_hwinfo );
                hwinfo = avm_pa_pid_get_hwinfo( pid_handle );
            }

            if ( hwinfo ){
            	hwinfo->flags |= AVMNET_DEVICE_IFXPPA_VIRT_TX;
            	hwinfo->virt_tx_dev = virtdev;
            }
            else {
            	kfree(virtdev);
            	return ALLOC_VIRT_DEV_FAILED;
            }
            return i;
        }
    }

failed:
    err("alloc virtual tx for pid %d FAILED ", pid_handle);
    if (virtdev)
        kfree(virtdev);
    return ALLOC_VIRT_DEV_FAILED;
}
EXPORT_SYMBOL(ifx_ppa_alloc_virtual_tx_device);


/*------------------------------------------------------------------------------------------*\
 * Returns
 *  ( virt_dev_id >= 0 ): success
 *  (               -1 ): failed
\*------------------------------------------------------------------------------------------*/
int ifx_ppa_free_virtual_tx_device(avm_pid_handle pid_handle){

    struct avm_pa_pid_hwinfo *hwinfo;
    struct avm_pa_virt_tx_dev *virtdev;
    int virt_dev_id;

    hwinfo = avm_pa_pid_get_hwinfo( pid_handle );
    if (! hwinfo )
        return FREE_VIRT_DEV_FAILED;

    virtdev = hwinfo->virt_tx_dev;
    if (! virtdev )
        return FREE_VIRT_DEV_FAILED;

    virt_dev_id = virtdev->hw_dev_id;
    if ( virt_dev_id < MAX_PPA_PUSHTHROUGH_DEVICES ){
        ppa_virt_tx_devs[ virt_dev_id ] = NULL;
    }
    hwinfo->virt_tx_dev = NULL;
    hwinfo->flags &= ~AVMNET_DEVICE_IFXPPA_VIRT_TX;

    kfree(virtdev);
    return virt_dev_id;
}


/*------------------------------------------------------------------------------------------*\
 * Returns
 *  ( virt_dev_id >= 0 ): success
 *  (               -1 ): failed
\*------------------------------------------------------------------------------------------*/
int ifx_ppa_free_virtual_rx_device(avm_pid_handle pid_handle){

    struct avm_pa_pid_hwinfo *hwinfo;
    struct avm_pa_virt_rx_dev *virtdev;
    int virt_dev_id;

    hwinfo = avm_pa_pid_get_hwinfo( pid_handle );
    if (! hwinfo )
        return FREE_VIRT_DEV_FAILED;

    virtdev = hwinfo->virt_rx_dev;
    if (! virtdev )
        return FREE_VIRT_DEV_FAILED;

    virt_dev_id = virtdev->hw_dev_id;
    if ( virt_dev_id < MAX_PPA_PUSHTHROUGH_DEVICES ){
        ppa_virt_rx_devs[ virt_dev_id ] = NULL;
    }
    hwinfo->virt_rx_dev = NULL;
    hwinfo->flags &= ~AVMNET_DEVICE_IFXPPA_VIRT_RX;

    kfree(virtdev);
    return virt_dev_id;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void ifx_ppa_show_virtual_devices(struct seq_file *seq){
    int i;

    seq_printf(seq, "MAX_PPA_PUSHTHROUGH_DEVICES: %u\n", MAX_PPA_PUSHTHROUGH_DEVICES);
    for (i=0; i < MAX_PPA_PUSHTHROUGH_DEVICES; i++){
        if ( ppa_virt_rx_devs[i] ){
            seq_printf(seq, "virt_rx_dev[%d]: \n", i);
            seq_printf(seq, "   hw_dev_id[rx]        : %lu \n", ppa_virt_rx_devs[i]->hw_dev_id );
            seq_printf(seq, "   ppa_dev_id           : %lu \n", ppa_virt_rx_devs[i]->hw_dev_id + IFX_VIRT_DEV_OFFSET );
            seq_printf(seq, "   slow_cnt             : %lu \n", ppa_virt_rx_devs[i]->hw_pkt_slow_cnt );
            seq_printf(seq, "   try_to_acc           : %lu \n", ppa_virt_rx_devs[i]->hw_pkt_try_to_acc );
            seq_printf(seq, "   try_to_acc_dropped   : %lu \n", ppa_virt_rx_devs[i]->hw_pkt_try_to_acc_dropped );
            seq_printf(seq, "   try_to_acc_not_ready : %lu \n", ppa_virt_rx_devs[i]->hw_pkt_try_to_acc_virt_chan_not_ready );
        }
    }
    for (i=0; i < MAX_PPA_PUSHTHROUGH_DEVICES; i++){
        if ( ppa_virt_tx_devs[i] ){
            seq_printf(seq, "virt_tx_dev[%d]: \n", i);
            seq_printf(seq, "   hw_dev_id[tx]       : %lu \n", ppa_virt_tx_devs[i]->hw_dev_id );
            seq_printf(seq, "   ppa_dev_id          : %lu \n", ppa_virt_tx_devs[i]->hw_dev_id + IFX_VIRT_DEV_OFFSET );
            seq_printf(seq, "   hw_pkt_tx           : %lu \n", ppa_virt_tx_devs[i]->hw_pkt_tx);
            seq_printf(seq, "   hw_pkt_slookup_fail : %lu \n", ppa_virt_tx_devs[i]->hw_pkt_tx_session_lookup_failed);
        }
    }
    return;
}
EXPORT_SYMBOL(ifx_ppa_show_virtual_devices);
#endif

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

void (*ppa_hook_mpoa_setup)(struct atm_vcc *, int, int) = NULL;
EXPORT_SYMBOL(ppa_hook_mpoa_setup);

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

#ifdef AVMNET_VCC_QOS

/*
* returns ret:
* 	ret == 0: setup qos queues successful
* 	ret <  0: setup qos queue failure
*/
int (*vcc_set_nr_qos_queues)(struct atm_vcc *, unsigned int) = NULL;
EXPORT_SYMBOL(vcc_set_nr_qos_queues);


/*
* returns ret:
* 	ret == 0: success
* 	ret != 0: failure
*/
int (*vcc_map_skb_prio_qos_queue)(struct atm_vcc *, unsigned int, unsigned int) = NULL;
EXPORT_SYMBOL(vcc_map_skb_prio_qos_queue);

#endif 

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

#if defined(AVMNET_RATE_SHAPING) && defined(CONFIG_IFX_PPA_QOS)


/*
* returns ret:
* 	ret == 0: success
* 	ret == -1: failure
* 	ret == 1: rate shaping not possible on device - hw acl disabled for tx path
*/
int avmnet_set_wan_tx_rate_shaping( unsigned int kbps ){
    unsigned int wan_portid = 0;
    unsigned int max_ports;
    unsigned int i;
    int res = IFX_SUCCESS;

    PPE_QOS_RATE_SHAPING_CFG *rate_cfg;
    PPE_QOS_ENABLE_CFG *enable_cfg;
    PPA_QOS_STATUS *qos_state;

    rate_cfg = kzalloc(sizeof(PPE_QOS_RATE_SHAPING_CFG), GFP_ATOMIC);
    enable_cfg = kzalloc(sizeof(PPE_QOS_ENABLE_CFG), GFP_ATOMIC);
    qos_state = kzalloc(sizeof(PPA_QOS_STATUS), GFP_ATOMIC);

    if ((!rate_cfg ) || (!enable_cfg) || (!qos_state)){
        res = IFX_FAILURE;
        goto EXIT_RS;
    }

    if (ifx_ppa_drv_get_qos_status(qos_state, 0) != IFX_SUCCESS){
        printk(KERN_ERR "[%s] error looking up qos status \n", __func__);
        res = IFX_FAILURE;
        goto EXIT_RS;
    }

    if ((avm_wan_mode == AVM_WAN_MODE_PTM) || (avm_wan_mode == AVM_WAN_MODE_ETH)){

        wan_portid = qos_state->qos_queue_portid;

        // enable rate shaping it

        dbg( "setctrlrate on wan_portid %u\n", wan_portid);
        enable_cfg->portid = wan_portid;
        enable_cfg->f_enable = 1;
        enable_cfg->overhd_bytes = QOS_OVERHD_BYTES;
        enable_cfg->flag |= PPE_QOS_ENABLE_CFG_FLAG_SETUP_OVERHD_BYTES ;
        ifx_ppa_drv_set_ctrl_qos_rate(enable_cfg, 0);

        if ( ! qos_state->shape_en ){
            // qos was disabled before: reset rate for each queue
        	// and all global ports to MAXIMUM (1GBit/s)
        	// loop till '<=', that means we also setup global port queue
            dbg("reset rate for all queues and global ports\n");
            ifx_ppa_drv_init_qos_rate( 0);
        }

        // set rate for the wanport
        dbg("setrateport %u: %u\n", wan_portid, kbps);
        rate_cfg->burst = 0; //default burst
        rate_cfg->rate_in_kbps = kbps;
        rate_cfg->portid = wan_portid;

        // we might have several ports in PTM case
        max_ports = ( avm_wan_mode == AVM_WAN_MODE_PTM )? MAX_PTM_SHAPED_PORTS : 1;

        for (i = 0; i < max_ports; i++){
        	rate_cfg->queueid = ~i; // If queue id bigger than maximum queue id,
        							// it will be regarded as port based rate shaping,
        							// for port ~id;
        	ifx_ppa_drv_set_qos_rate( rate_cfg , 0);
        }

    } else if (avm_wan_mode == AVM_WAN_MODE_ATM) {
        dbg("wan_tx_rate_shaping not supported in ATM Mode, tx acl will be disabled\n" );

#ifdef CONFIG_AVM_PA
        avm_pa_disable_atm_hw_tx_acl();
#endif
        res = 1;
        goto EXIT_RS;

    } else {
        //TODO implement this!
        printk( KERN_ERR "wan_tx_rate_shaping not supported in avm_wan_mode=%d, he have to implement disabling of tx acl here!\n", avm_wan_mode );
        res = IFX_FAILURE;
        goto EXIT_RS;
    }

EXIT_RS:
    kfree(rate_cfg);
    kfree(enable_cfg);
    kfree(qos_state);
    return res;
}
EXPORT_SYMBOL(avmnet_set_wan_tx_rate_shaping);
/*
* returns ret:
* 	ret == 0: success
* 	ret != 0: failure
*/
int avmnet_disable_wan_tx_rate_shaping(void){

    PPE_QOS_ENABLE_CFG *enable_cfg;
    PPA_QOS_STATUS *qos_state;
    unsigned int wan_portid = 0;
    int res = IFX_SUCCESS;

    enable_cfg = kzalloc(sizeof(PPE_QOS_ENABLE_CFG), GFP_ATOMIC);
    qos_state = kzalloc(sizeof(PPA_QOS_STATUS), GFP_ATOMIC);

    if ((!enable_cfg) || (!qos_state)){
        res = IFX_FAILURE;
        goto EXIT_RS;
    }

    if (ifx_ppa_drv_get_qos_status(qos_state, 0) != IFX_SUCCESS){
        printk(KERN_ERR "[%s] error looking up qos status \n", __func__);
        res = IFX_FAILURE;
        goto EXIT_RS;
    }

    wan_portid = qos_state->qos_queue_portid;

    if ((avm_wan_mode == AVM_WAN_MODE_PTM) || (avm_wan_mode == AVM_WAN_MODE_ETH)){

        dbg( "disable setctrlrate on wan_portid %u\n", wan_portid);
        enable_cfg->portid = wan_portid;
        enable_cfg->f_enable = 0;
        ifx_ppa_drv_set_ctrl_qos_rate(enable_cfg, 0);

    } else if (avm_wan_mode == AVM_WAN_MODE_ATM) {
        dbg(" disable rate shaping in ATM devices, tx acl will be (re)enabled\n" );
#ifdef CONFIG_AVM_PA
        avm_pa_enable_atm_hw_tx_acl();
#endif
    }

EXIT_RS:
    kfree(enable_cfg);
    kfree(qos_state);
    return res;
}
EXPORT_SYMBOL(avmnet_disable_wan_tx_rate_shaping);


/*
* returns ret:
* 	ret == 0: success
* 	ret == -1: failure
* 	ret == 1: rate shaping not possible on device
*/
int avmnet_set_wan_tx_queue_rate_shaping( unsigned int qid, unsigned int kbps ){
    unsigned int wan_portid = 0;
    int res = IFX_SUCCESS;

    PPE_QOS_RATE_SHAPING_CFG *rate_cfg;
    PPE_QOS_ENABLE_CFG *enable_cfg;
    PPA_QOS_STATUS *qos_state;

    rate_cfg = kzalloc(sizeof(PPE_QOS_RATE_SHAPING_CFG), GFP_KERNEL);
    enable_cfg = kzalloc(sizeof(PPE_QOS_ENABLE_CFG), GFP_KERNEL);
    qos_state = kzalloc(sizeof(PPA_QOS_STATUS), GFP_KERNEL);

    if ((!rate_cfg ) || (!enable_cfg) || (!qos_state)){
        res = IFX_FAILURE;
        goto EXIT_RS;
    }

    if (ifx_ppa_drv_get_qos_status(qos_state, 0) != IFX_SUCCESS){
        printk(KERN_ERR "[%s] error looking up qos status \n", __func__);
        res = IFX_FAILURE;
        goto EXIT_RS;
    }

    if ( (avm_wan_mode == AVM_WAN_MODE_PTM) || (avm_wan_mode == AVM_WAN_MODE_ETH ) ){

        wan_portid = qos_state->qos_queue_portid;

        // enable rate shaping it
        dbg("setctrlrate on wan_portid %u\n", wan_portid);
        enable_cfg->portid = wan_portid;
        enable_cfg->f_enable = 1;
        enable_cfg->overhd_bytes = QOS_OVERHD_BYTES;
        enable_cfg->flag |= PPE_QOS_ENABLE_CFG_FLAG_SETUP_OVERHD_BYTES ;
        ifx_ppa_drv_set_ctrl_qos_rate(enable_cfg, 0);

        if ( ! qos_state->shape_en ){
            // qos was disabled before: reset rate for each queue
        	// and all global ports to MAXIMUM (1GBit/s)
        	// loop till '<=', that means we also setup global port queue
            dbg("reset rate for all queues and global ports\n");
            ifx_ppa_drv_init_qos_rate( 0);
        }

        // set rate for qid in the wanport
        dbg( "setrate port=%u, queue=%u: rate=%u\n", wan_portid, qid, kbps);
        rate_cfg->burst = 0; //default burst
        rate_cfg->rate_in_kbps = kbps;
        rate_cfg->portid = wan_portid;
        rate_cfg->queueid = qid;
        ifx_ppa_drv_set_qos_rate( rate_cfg , 0);

    } else if (avm_wan_mode == AVM_WAN_MODE_ATM) {

        dbg("wan_tx_rate_shaping not supported in ATM Mode, tx acl will be disabled\n" );
#ifdef CONFIG_AVM_PA
        avm_pa_disable_atm_hw_tx_acl();
#endif
        res = 1;
        goto EXIT_RS;

    } else {

        //TODO implement this!
        printk( KERN_ERR "wan_tx_queue_rate_shaping not supported in avm_wan_mode=%d, he have to implement disabling of tx acl here!\n", avm_wan_mode );
        res = IFX_FAILURE;
        goto EXIT_RS;
    }

EXIT_RS:
    kfree(rate_cfg);
    kfree(enable_cfg);
    kfree(qos_state);
    return res;
}
EXPORT_SYMBOL(avmnet_set_wan_tx_queue_rate_shaping);
#endif

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/



