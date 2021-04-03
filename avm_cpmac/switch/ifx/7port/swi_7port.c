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
#include <linux/compiler.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/workqueue.h>
#include <ifx_pmu.h>
#include <ifx_rcu.h>

#include <avmnet_debug.h>
#include <avmnet_module.h>
#include <avmnet_config.h>
#include <avmnet_common.h>
#include <linux/avm_hw_config.h>
#include <asm/prom.h>

#ifdef CONFIG_AVM_PA
#include <linux/if_vlan.h>
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
#include <switch_api/ifx_ethsw_pce.h>
#endif

#include <avm/net/ifx_ppa_ppe_hal.h>
#include <switch_api/ifx_ethsw_api.h>
#include <switch_api/ifx_ethsw_kernel_api.h>
#include <switch_api/ifx_ethsw_flow_core.h>
#include <asm-generic/ioctl.h>
#include <ifx_gpio.h>
#include "swi_7port.h"
#include "swi_7port_reg.h"
#include "ifxmips_ppa_datapath_7port_common.h"
#include "../common/swi_ifx_common.h"
#include "mac_7port.h"
#include "../../../management/avmnet_links.h"

#if !defined(CONFIG_IFX_ETHSW_API) || (CONFIG_IFX_ETHSW_API == 'n')
#include "switch_api/ifx_ethsw_pce.h"
#endif
struct net_device *g_eth_net_dev[MAX_ETH_INF] = { [ 0 ... (MAX_ETH_INF - 1) ] = NULL };

EXPORT_SYMBOL(g_eth_net_dev);

extern volatile int g_stop_datapath;
extern unsigned int num_registered_eth_dev;
static unsigned int add_session_mutex_failure = 0;
static unsigned int report_session_mutex_failure = 0;

static void write_mdio_sched(unsigned int phyAddr, unsigned int regAddr, unsigned int data);
static unsigned short read_mdio_sched(unsigned int phyAddr, unsigned int regAddr);

#if !defined(CONFIG_IFX_ETHSW_API) || (CONFIG_IFX_ETHSW_API == 'n')
static void switch_reset(unsigned int on);
#endif

// functions for proc-files
static int read_rmon_all_show(struct seq_file *seq, void *data);
static int read_rmon_all_open(struct inode *inode, struct file *file);
static int mac_table_show(struct seq_file *seq, void *data);
static int mac_table_open(struct inode *inode, struct file *file);
static ssize_t proc_mac_table_write(struct file *filp, const char __user *buff, size_t len, loff_t *offset);

static int proc_mirror_show(struct seq_file *seq, void *data);
static int proc_mirror_open(struct inode *inode, struct file *file);
static ssize_t proc_mirror_write(struct file *filp, const char __user *buff, size_t len, loff_t *offset);

static int proc_ssm_open(struct inode *inode, struct file *file);
static int proc_ssm_show(struct seq_file *seq, void *data);

static void set_pce_set_bc_mc_class(const unsigned char class);
static void set_pce_critical_mac(const u8 *mac, const u8 port, const u8 class);

static const struct file_operations mac_table_fops =
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 32)
   .owner   = THIS_MODULE,
#endif
   .open    = mac_table_open,
   .read    = seq_read,
   .write   = proc_mac_table_write,
   .llseek  = seq_lseek,
   .release = single_release,
};

static const struct file_operations read_rmon_all_fops =
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 32)
   .owner   = THIS_MODULE,
#endif
   .open    = read_rmon_all_open,
   .read    = seq_read,
   .llseek  = seq_lseek,
   .release = single_release,
};

static const struct file_operations proc_mirror_fops =
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 32)
   .owner   = THIS_MODULE,
#endif
   .open    = proc_mirror_open,
   .read    = seq_read,
   .write   = proc_mirror_write,
   .llseek  = seq_lseek,
   .release = single_release,
};

static const struct file_operations proc_ssm_fops =
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 32)
   .owner   = THIS_MODULE,
#endif
   .open    = proc_ssm_open,
   .read    = seq_read,
   .release = single_release,
};

#ifdef CONFIG_AVM_PA
/*------------------------------------------------------------------------------------------*\
 * PCE PA
\*------------------------------------------------------------------------------------------*/
static int setup_pce(void);
int avmnet_7port_pce_add_session(struct avm_pa_session *avm_session);
int avmnet_7port_pce_remove_session(struct avm_pa_session *avm_session);
int avmnet_7port_pce_remove_session_entry(struct avmnet_7port_pce_session *session);
int avmnet_7port_pce_session_stats(struct avm_pa_session *session,
					                struct avm_pa_session_stats *stats);
static int pa_sessions_open(struct inode *inode, struct file *file);
static int pa_sessions_show(struct seq_file *seq, void *data __attribute__ ((unused)) );

static IFX_PCE_t pce_table;
static struct list_head pce_sessions;

// assignable RMON counters used by PA rules
#define PA_RMON_COUNTERS 23
#define PA_RMON_CNT_OFF 41

// array telling which RMON counters are currently used
static int rmon_taken[] = {[0 ... (PA_RMON_COUNTERS - 1)] = -1};

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,37)
DECLARE_MUTEX(swi_pa_mutex);
#else
DEFINE_SEMAPHORE(swi_pa_mutex);
#endif


static struct avm_hardware_pa_instance avmnet_pce_pa = {
   .features = HW_PA_CAPABILITY_BRIDGING | HW_PA_CAPABILITY_LAN2LAN,
   .add_session = avmnet_7port_pce_add_session,
   .remove_session = avmnet_7port_pce_remove_session,
   .session_stats = avmnet_7port_pce_session_stats,
   .name = "avmnet_7port_pce",
};

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

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,37)
DECLARE_MUTEX(swi_7port_mutex);
#else
DEFINE_SEMAPHORE(swi_7port_mutex);
#endif

#if defined(NETDEV_PORT_STATS_READ_RMON)
struct net_device_stats port_stats[MAX_ETH_INF];
#endif

static unsigned int rmon_data[RMON_PORT_SIZE][RMON_COUNT_SIZE];
static char *rmon_names[RMON_COUNT_SIZE] = {
    "Transmit size 64Bytes Frame Count      :",
    "Transmit size 65-127Bytes Frame Count  :",
    "Transmit size 128-255Bytes Frame Count :",
    "Transmit size 256-511Bytes Frame Count :",
    "Transmit size 512-1023Bytes Frame Count:",
    "Transmit size >1024Bytes Frame Count   :",
    "Transmit Unicast Frame Count           :",
    "Transmit Multicast Frame Count         :",
    "Transmit Single Collision Count        :",
    "Transmit Multiple Collision Count      :",
    "Transmit Late Collision Count          :",
    "Transmit Excessive Collision Count     :",
    "Transmit Frame Count                   :",
    "Transmit Pause Frame Count             :",
    "Transmit Good Byte Count (Low)         :",
    "Transmit Good Byte Count(High)         :",
    "Transmit Dropped Frame Count           :",
    "Transmit Queue Discard frames          :",
    "Receive size 64Bytes Frame Count       :",
    "Receive size 65-127Bytes Frame Count   :",
    "Receive size 128-255Bytes Frame Count  :",
    "Receive size 256-511Bytes Frame Count  :",
    "Receive size 512-1023Bytes Frame Count :",
    "Receive size >1024Bytes Frame Count    :",
    "Receive Discard (Tail-Drop) frame Count:",
    "Receive Drop (Filter) frame Count      :",
    "Receive Alignment errors Count         :",
    "Receive Oversize good Count            :",
    "Receive Oversize bad Count             :",
    "Receive Undersize good Count           :",
    "Receive Undersize bad Count            :",
    "Receive Frame Count                    :",
    "Receive Pause good Count               :",
    "Receive CRC errors Count               :",
    "Receive Multicast Frame Count          :",
    "Receive Unicast Frame Count            :",
    "Receive Good Byte Count (Low)          :",
    "Receive Good Byte Count(High)          :",
    "Receive Bad Byte Count (Low)           :",
    "Receive Bad Byte Count(High)           :",
    "Assignable Counter 0                   :",
    "Assignable Counter 1                   :",
    "Assignable Counter 2                   :",
    "Assignable Counter 3                   :",
    "Assignable Counter 4                   :",
    "Assignable Counter 5                   :",
    "Assignable Counter 6                   :",
    "Assignable Counter 7                   :",
    "Assignable Counter 8                   :",
    "Assignable Counter 9                   :",
    "Assignable Counter 10                  :",
    "Assignable Counter 11                  :",
    "Assignable Counter 12                  :",
    "Assignable Counter 13                  :",
    "Assignable Counter 14                  :",
    "Assignable Counter 15                  :",
    "Assignable Counter 16                  :",
    "Assignable Counter 17                  :",
    "Assignable Counter 18                  :",
    "Assignable Counter 19                  :",
    "Assignable Counter 20                  :",
    "Assignable Counter 21                  :",
    "Assignable Counter 22                  :",
    "Assignable Counter 23                  :",
};

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

void swi_set_mdio_polling(int on) {
    AVMNET_INFO("[%s] %s\n", __FUNCTION__, (on ? "on" : "off"));
    if(on){
        int mdc_cfg = SW_READ_REG32(MDC_CFG_0_REG);
        if(mdc_cfg){
            AVMNET_ERR("[%s] polling already running: %#x\n", __FUNCTION__, mdc_cfg);
        }else{
            /*--- SW_WRITE_REG32 (((1 << 4)|(1 << 2)), MDC_CFG_0_REG); ---*/
//            SW_WRITE_REG32(((1 << 4)|(1 << 2)|(1 << 1)|(1 << 0)), MDC_CFG_0_REG);
              SW_WRITE_REG32(((1 << 0)|(1 << 1)|(1 << 2)|(1 << 3)|(1 << 4)), MDC_CFG_0_REG);
        }
    }else{
        SW_WRITE_REG32(0, MDC_CFG_0_REG);
    }
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
#if !defined(CONFIG_AR10) // erstmal nur bei VR9
int fix_phy_clock_config(void){

    int gpio_cfg;
    unsigned int cgu_if_reg, hwrev = 0, hwsubrev = 0;
    char *s;
    struct _avm_hw_config *hw_config;

    hw_config = avm_get_hw_config_table();
    while(hw_config &&
          hw_config->name != NULL &&
          hw_config->manufactor_hw_config.manufactor_lantiq_gpio_config.module_id
              != IFX_GPIO_MODULE_EXTPHY_25MHZ_CLOCK
          )
    {
        ++hw_config;
    }

    if(hw_config == NULL || hw_config->name == NULL){
        AVMNET_WARN("[%s]: Could not read external PHY clock pin data from avm_hw_config!\n", __func__);
        return -ENXIO;
    }

    s = prom_getenv("HWRevision");
    if (s) {
        hwrev = simple_strtoul(s, NULL, 10);
    }

    if((s == NULL) || (hwrev > 255)) {
        AVMNET_ERR("[%s] error: %s HWRevision detected in environment variables: %s\n", __func__, (s ? "invalid" : "no"), s);
        BUG_ON(1);
        return -EFAULT;
    }

    s = prom_getenv("HWSubRevision");
    if (s) {
        hwsubrev = simple_strtoul(s, NULL, 10);
    }
    if(hwsubrev > 255) {
        AVMNET_ERR("[%s] error: %s HWSubRevision detected in environment variables: %s\n", __func__, (s ? "invalid" : "no"), s);
        BUG_ON(1);
        return -EFAULT;
    }

    /*
     * Sonderbehandlung 3370. Bei HWSubRev <= 4 gibt der VR9 den 25MHz-Takt
     * an die PHYs, ab HWSubRev 5 liefern die PHYs den Takt. Die HW-Conf fuer
     * die 3370 geht von einer HWSubRev <= 4 aus.
     */
    if ((hwrev == 175 || hwrev == 177) && hwsubrev > 4){
        ifx_gpio_dir_in_set(hw_config->value, IFX_GPIO_MODULE_EXTPHY_25MHZ_CLOCK);
        hw_config->manufactor_hw_config.manufactor_lantiq_gpio_config.config =
                                        IFX_GPIO_IOCTL_PIN_CONFIG_DIR_IN
                                      | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL0_SET
                                      | IFX_GPIO_IOCTL_PIN_CONFIG_ALTSEL1_CLEAR;
    }

    /*
     * CGU_IF_CLK wird im Urlader nicht korrekt gesetzt
     */
    gpio_cfg = hw_config->manufactor_hw_config.manufactor_lantiq_gpio_config.config;

    cgu_if_reg = SW_READ_REG32(IFX_CGU_IF_CLK);
    cgu_if_reg &= ~(0x7 << 2);
    if(gpio_cfg & IFX_GPIO_IOCTL_PIN_CONFIG_DIR_OUT){
        cgu_if_reg |= 0x2 << 2;
    }else{
        cgu_if_reg |= 0x4 << 2;
    }
    SW_WRITE_REG32(cgu_if_reg, IFX_CGU_IF_CLK);
    return 0;
}
#endif

/*--------------------------------------------------------------------------------*\
\*--------------------------------------------------------------------------------*/
#define SSB_WR 1
#define SSB_RD 2
#define ETHSW_SSB_MODE  (3)
#define ETHSW_SSB_ADDR  (4)
#define ETHSW_SSB_DATA  (5)

#define SSM_MAX_ERROR   (20)
unsigned int ssm_error[SSM_MAX_ERROR][2];
int ssm_test(void) {

    unsigned int i, j, Data;
    unsigned int k = 0;
    unsigned short pattern, pattern_array[] = { 0xF0F0, 0x0F0F };
#define MEM_PATTERN_CNT (sizeof(pattern_array) / sizeof(unsigned short))

    for (j = 0; j < MEM_PATTERN_CNT; j++) {
        pattern = pattern_array[j];
        /*--------------------------------------------------------------------------------*\
         * ein Test für den Speicher im Switch 32k
        \*--------------------------------------------------------------------------------*/
        for ( i = 0; i < (1<<15); i++) {
            *VR9_SWIP_MACRO_REG(ETHSW_SSB_ADDR) = i & 0xFFFF;
            *VR9_SWIP_MACRO_REG(ETHSW_SSB_DATA) = pattern;
            *VR9_SWIP_MACRO_REG(ETHSW_SSB_MODE) = SSB_WR;
            while( *VR9_SWIP_MACRO_REG(ETHSW_SSB_MODE) & SSB_WR);
        }

        for ( i = 0; i < (1<<15); i++) {
            *VR9_SWIP_MACRO_REG(ETHSW_SSB_ADDR) = i & 0xFFFF;
            *VR9_SWIP_MACRO_REG(ETHSW_SSB_MODE) = SSB_RD;
            while( *VR9_SWIP_MACRO_REG(ETHSW_SSB_MODE) & SSB_RD);
            Data = *VR9_SWIP_MACRO_REG(ETHSW_SSB_DATA);
            if (Data != pattern) {
                AVMNET_DEBUG("[ssm] <ERROR> addr 0x%x pattern 0x%x - 0x%x\n", i, pattern, Data);
                if (k < SSM_MAX_ERROR) {
                    ssm_error[k][0] = i;
                    ssm_error[k][1] = Data;
                    k++;
                } else {
                    break;
                }
            }
        }
    }
    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_swi_7port_init(avmnet_module_t *this)
{
    struct avmnet_swi_7port_context *my_ctx;
    int i, result;


    AVMNET_ERR("[%s] Init on module %s called.\n", __func__, this->name);

    ssm_test();

    my_ctx = kzalloc(sizeof(struct avmnet_swi_7port_context), GFP_KERNEL);
    if(my_ctx == NULL){
        AVMNET_ERR("[%s] init of avmnet-module %s failed.\n", __func__, this->name);
        return -ENOMEM;
    }

    this->priv = my_ctx;

#if !defined(CONFIG_AR10) // erstmal nur bei VR9
    fix_phy_clock_config();
#endif

    for(i = 0; i < this->num_children; ++i){
        result = this->children[i]->init(this->children[i]);
        if(result != 0){
            AVMNET_ERR("Module %s: init() failed on child %s\n", this->name, this->children[i]->name);
            return result;
        }
    }

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

int avmnet_swi_7port_setup(avmnet_module_t *this)
{
    int i, result;
    /*--- struct avmnet_swi_7port_context *ctx = (struct avmnet_swi_7port_context *) this->priv; ---*/

    AVMNET_INFO("[%s] Setup on module %s called.\n", __func__, this->name);

    /*
     * do cunning setup-stuff
     */

#   if !defined(CONFIG_IFX_ETHSW_API) || (CONFIG_IFX_ETHSW_API == 'n')
#error
    /* Power up and enable Switch  */
    PPE_TOP_PMU_SETUP(IFX_PMU_ENABLE);
    PPE_DPLUS_PMU_SETUP(IFX_PMU_ENABLE);
    PPE_DPLUSS_PMU_SETUP(IFX_PMU_ENABLE);
    SWITCH_PMU_SETUP(IFX_PMU_ENABLE);
    GPHY_PMU_SETUP(IFX_PMU_ENABLE);

    AVMNET_INFOTRC("[%s] Resetting VR9 switch module\n", __func__);
    switch_reset(1);
    mdelay(10);
    switch_reset(0);

    SW_WRITE_REG32 ((SW_READ_REG32(GLOB_CTRL_REG) | GLOB_CTRL_SE), GLOB_CTRL_REG);

    AVMNET_INFOTRC("[%s] calling IFX_VR9_Switch_PCE_Micro_Code_Int\n", __func__);
    IFX_VR9_Switch_PCE_Micro_Code_Int();

    AVMNET_INFOTRC("[%s] call ppe_eth_init\n", __FUNCTION__);
    ppe_eth_init();

    /* Disable MDIO auto polling mode for all ports */
    SW_WRITE_REG32 (0x0, MDC_CFG_0_REG);

    /* Reset MDIO-hardware, enable it and set clock rate to 2.5MHz */
    SW_WRITE_REG32 ((MDC_CFG_1_RES | MDC_CFG_1_MCEN | MDC_CFG_1_2_5MHZ), MDC_CFG_1_REG);
#   else /*--- #if !defined(CONFIG_IFX_ETHSW_API) || (CONFIG_IFX_ETHSW_API == 'n') ---*/
    {
        int ret __attribute__((unused));
        union ifx_sw_param api_param;
        IFX_ETHSW_HANDLE handle;

        handle = ifx_ethsw_kopen("/dev/switch_api/0");

        AVMNET_INFOTRC("[%s] call switch_api IFX_ETHSW_ENABLE \n", __func__);
        memset(&api_param.register_access, 0x00, sizeof(api_param.register_access));
        api_param.register_access.nRegAddr = 0;
        api_param.register_access.nData = 0x00;
        ret = ifx_ethsw_kioctl(handle, IFX_ETHSW_ENABLE, (unsigned int) &api_param);
        ifx_ethsw_kclose(handle);

        AVMNET_INFOTRC("[%s] call switch_api IFX_ETHSW_ENABLE returned %d \n", __func__, ret);

    }

    if(this->initdata.swi.flags & AVMNET_CONFIG_FLAG_MDIOPOLLING){
        swi_set_mdio_polling(1);
    } else {
        swi_set_mdio_polling(0);
    }
    disable_cpuport_flow_control();

    ppe_eth_init();
#   endif /*--- #else ---*/ /*--- #if !defined(CONFIG_IFX_ETHSW_API) || (CONFIG_IFX_ETHSW_API == 'n') ---*/

    avmnet_swi_7port_disable_learning(this);

    /* remove wan itf mask*/
    avmnet_swi_7port_set_ethwan_dev_by_name( NULL );

    /* Call set-up in all children */
    for(i = 0; i < this->num_children; ++i){
        result = this->children[i]->setup(this->children[i]);
        if(result != 0){
            AVMNET_ERR("Module %s: setup() failed on child %s\n", this->name, this->children[i]->name);
            return result;
        }
    }

    init_avm_dev_lookup();

    avmnet_cfg_register_module(this);
    avmnet_cfg_add_seq_procentry(this, "rmon_all", &read_rmon_all_fops);
    avmnet_cfg_add_seq_procentry(this, "mac_table", &mac_table_fops);
    avmnet_cfg_add_seq_procentry(this, "mirror_port", &proc_mirror_fops);

    avmnet_cfg_add_seq_procentry(this, "ssm", &proc_ssm_fops);

#if 0
    AVMNET_INFOTRC("[%s] doing initial poll.\n", __func__);
    for(i = 0; i < this->num_children; ++i){
        result = this->children[i]->poll(this->children[i]);
        if(result < 0){
            // handle error
        }
    }
#endif

#ifdef CONFIG_AVM_PA
    INIT_LIST_HEAD(&pce_sessions);
    avm_pa_multiplexer_init();
    ifx_pce_table_init(&pce_table);
    setup_pce();
    avm_pa_multiplexer_register_instance( &avmnet_pce_pa );
    avmnet_cfg_add_seq_procentry(this, "pa_pce_sessions", &pa_sessions_fops);

    /* 
     * class 0 --> DMA-Chan 21 <--- default
     * class 1 --> DMA-Chan 20 <--- broadcast multicast
     * class 2 --> DMA-Chan 20 
     * class 3 --> DMA-Chan  0 <--- wifi offload
     */

    for(i = 0; i < this->num_children; ++i){
	avmnet_module_t *mac = this->children[i];
	if (mac->num_children && mac->children[0] && mac->children[0]->device_id){
		avmnet_device_t *dev = mac->children[0]->device_id;
		if (dev->flags & AVMNET_DEVICE_FLAG_PHYS_OFFLOAD_LINK){
    			set_pce_critical_mac(dev->offload_mac, 
					     mac->initdata.mac.mac_nr, 3);
		}
	}
    }

    set_pce_set_bc_mc_class(1);

#endif

    return 0;
}

/*------------------------------------------------------------------------------------------*\
 * wird beim neuaufsetzen des DMAs benutzt
\*------------------------------------------------------------------------------------------*/
void avmnet_swi_7port_force_macs_down( avmnet_module_t *this ) {

  	AVMNET_ERR("[%s] force mac link down on module %s called. Please use suspend instead.\n", __func__, this->name);
  	dump_stack();

  	this->suspend(this, this);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void avmnet_swi_7port_reinit_macs(avmnet_module_t *this) {

    AVMNET_ERR("[%s] Re-init on module %s called. Please use resume instead.\n", __func__, this->name);
    dump_stack();

    this->resume(this, this);
}
EXPORT_SYMBOL(avmnet_swi_7port_reinit_macs);


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_swi_7port_suspend(avmnet_module_t *this, avmnet_module_t *caller)
{
    unsigned int i;
    struct avmnet_swi_7port_context *ctx;

    ctx = (struct avmnet_swi_7port_context *) this->priv;

    if(this->lock(this)){
        AVMNET_ERR("[%s] Interrupted while waiting for lock.\n", __func__);
        return -EINTR;
    }

    AVMNET_INFO("[%s] Called on module %s by %s.\n", __func__, this->name, caller != NULL ? caller->name:"unknown");

    if(ctx->suspend_cnt == 0){
        for(i = 0; i < num_registered_eth_dev; ++i){
            if(g_eth_net_dev[i] == NULL){
                continue;
            }

            if(!netif_queue_stopped(g_eth_net_dev[i])){
                AVMNET_TRC("[%s] stopping queues for device %s\n", __func__, g_eth_net_dev[i]->name);
                netif_tx_stop_all_queues(g_eth_net_dev[i]);
            }
        }
        for(i = 0; i < (unsigned int)this->num_children; ++i){
            if(this->children[i]->suspend && this->children[i] != caller){
                int wait_cycles = 0;
                while( this->children[i]->suspend(this->children[i], this)){
                    wait_cycles++;
                    schedule();
                }
                if (wait_cycles)
                    AVMNET_ERR("[%s] had to wait %d cycles to get mac lock\n", __func__, wait_cycles );
            }
        }
    }

    ctx->suspend_cnt++;

    this->unlock(this);
    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_swi_7port_resume(avmnet_module_t *this, avmnet_module_t *caller)
{
    unsigned int i;
    struct avmnet_swi_7port_context *ctx;

    ctx = (struct avmnet_swi_7port_context *) this->priv;

    AVMNET_INFO("[%s] Called on module %s by %s.\n", __func__, this->name, caller != NULL ? caller->name:"unknown");

    if(this->lock(this)){
        AVMNET_INFO("[%s] Interrupted while waiting for lock.\n", __func__);
        return -EINTR;
    }

    if(ctx->suspend_cnt == 0){
        AVMNET_ERR("[%s] unbalanced call on %s by %s, suspend_cnt = 0.\n", __func__, this->name, caller != NULL ? caller->name:"unknown");
        this->unlock(this);
        return -EFAULT;
    }

    ctx->suspend_cnt--;

    if(ctx->suspend_cnt == 0){
        for(i = 0; i < (unsigned int)this->num_children; ++i){
            if(this->children[i]->resume && this->children[i] != caller){
                int wait_cycles = 0;
                while( this->children[i]->resume(this->children[i], this) ){
                    wait_cycles ++;
                    schedule();
                }
                if (wait_cycles)
                    AVMNET_ERR("[%s] had to wait %d cycles to get mac lock\n", __func__, wait_cycles );
            }
        }

        for(i = 0; i < num_registered_eth_dev; ++i){
            struct net_device *dev = g_eth_net_dev[i];
            avmnet_netdev_priv_t *priv = NULL;
            avmnet_device_t *avm_dev = NULL;
            if(dev == NULL){
                continue;
            }
            priv = netdev_priv(dev);
            if (priv == NULL){
                continue;
            }

            avm_dev = priv->avm_dev;
            if (avm_dev == NULL){
                continue;
            }

            if( netif_queue_stopped(dev)
                && netif_carrier_ok(dev)
                && g_stop_datapath == 0
                && !( avm_dev->flags & AVMNET_DEVICE_FLAG_DMA_TX_QUEUE_FULL )
               )
            {
                AVMNET_DBG_TX_QUEUE("starting queues for device %s,",  g_eth_net_dev[i]->name);
                AVMNET_DBG_TX_QUEUE("  queue_stopped = %d",    netif_queue_stopped(dev));
                AVMNET_DBG_TX_QUEUE("  netif_carrier_ok = %d", netif_carrier_ok(dev));
                AVMNET_DBG_TX_QUEUE("  g_stop_datapath = %d",  g_stop_datapath);
                AVMNET_DBG_TX_QUEUE("  TX_QUEUE_FULL = %d",  avm_dev->flags & AVMNET_DEVICE_FLAG_DMA_TX_QUEUE_FULL );
                netif_tx_wake_all_queues(g_eth_net_dev[i]);
            } else {
                AVMNET_DBG_TX_QUEUE("NOT starting queues for device %s,", g_eth_net_dev[i]->name );
                AVMNET_DBG_TX_QUEUE("  queue_stopped = %d",    netif_queue_stopped(dev));
                AVMNET_DBG_TX_QUEUE("  netif_carrier_ok = %d", netif_carrier_ok(dev));
                AVMNET_DBG_TX_QUEUE("  g_stop_datapath = %d",  g_stop_datapath);
                AVMNET_DBG_TX_QUEUE("  TX_QUEUE_FULL = %d",  avm_dev->flags & AVMNET_DEVICE_FLAG_DMA_TX_QUEUE_FULL );
            }
        }
    }

    this->unlock(this);
    return 0;
}
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

static void setup_non_ethernet_deault_port(void){

    IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t pData;
    // age = 0xd;

    memset(&pData, 0, sizeof(IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t));

    pData.val[0] = ( 1 << 6 ); //cpu portmap
	pData.val[1] = 1; //static entry
    pData.table = IFX_ETHSW_PCE_MAC_BRIDGE_INDEX;
    pData.table_index = 0;
	pData.valid = 1;

    pData.key[0] = 0; // mac
    pData.key[1] = 0; // mac
    pData.key[2] = 0; // mac
    pData.key[3] = 0; // nfid

    ifx_ethsw_xwayflow_pce_table_cam_write(NULL, &pData);

}


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void avmnet_swi_7port_disable_learning(avmnet_module_t *this) {

    if(this->initdata.swi.flags & SWI_DISABLE_LEARNING){
        int i, value;

        AVMNET_INFOTRC("[%s] setup forward on exceeded learning limit (bLearningLimitAction = FORWARD)\n", __func__);
        ifx_ethsw_ll_DirectAccessWrite(NULL,
                                       VR9_PCE_GCTRL_0_PLIMMOD_OFFSET,
                                       VR9_PCE_GCTRL_0_PLIMMOD_SHIFT,
                                       VR9_PCE_GCTRL_0_PLIMMOD_SIZE,
                                       1);

        for(i = 0; i < this->num_children; ++i){
            if(this->children[i]->type == avmnet_modtype_mac){
                int mac_nr = this->children[i]->initdata.mac.mac_nr;
                AVMNET_INFOTRC("[%s] setup learning limit = 0 and drop ( UnicastUnknown, MulticastUnknown ) for MAC-Port %d\n", __func__, mac_nr);
                ifx_ethsw_ll_DirectAccessWrite(NULL,
                                               (VR9_PCE_PCTRL_1_LRNLIM_OFFSET + (0xA * mac_nr)),
                                                VR9_PCE_PCTRL_1_LRNLIM_SHIFT,
                                                VR9_PCE_PCTRL_1_LRNLIM_SIZE, 0);

                /*--- unicast ---*/
                ifx_ethsw_ll_DirectAccessRead(NULL,
                                              VR9_PCE_PMAP_3_UUCMAP_OFFSET,
                                              VR9_PCE_PMAP_3_UUCMAP_SHIFT,
                                              VR9_PCE_PMAP_3_UUCMAP_SIZE,
                                              &value);
                value &= ~(1 << mac_nr);
                ifx_ethsw_ll_DirectAccessWrite(NULL,
                                               VR9_PCE_PMAP_3_UUCMAP_OFFSET,
                                               VR9_PCE_PMAP_3_UUCMAP_SHIFT,
                                               VR9_PCE_PMAP_3_UUCMAP_SIZE,
                                               value);

                /*--- multicast ---*/
                ifx_ethsw_ll_DirectAccessRead(NULL,
                                              VR9_PCE_PMAP_2_DMCPMAP_OFFSET,
                                              VR9_PCE_PMAP_2_DMCPMAP_SHIFT,
                                              VR9_PCE_PMAP_2_DMCPMAP_SIZE,
                                              &value);
                value &= ~(1 << mac_nr);
                ifx_ethsw_ll_DirectAccessWrite(NULL,
                                               VR9_PCE_PMAP_2_DMCPMAP_OFFSET,
                                               VR9_PCE_PMAP_2_DMCPMAP_SHIFT,
                                               VR9_PCE_PMAP_2_DMCPMAP_SIZE,
                                               value);
            }
        }

        AVMNET_INFOTRC("[%s] setup learning limit = 0 for CPU-Port and virtual (DSL) Ports \n", __func__);
        for (i = 6; i < 12; i++){
        	ifx_ethsw_ll_DirectAccessWrite(NULL,
                                       (VR9_PCE_PCTRL_1_LRNLIM_OFFSET + (0xA * i)),
                                        VR9_PCE_PCTRL_1_LRNLIM_SHIFT,
                                        VR9_PCE_PCTRL_1_LRNLIM_SIZE, 0);
        }

        /*--- unicast ---*/
        ifx_ethsw_ll_DirectAccessRead(NULL,
                                      VR9_PCE_PMAP_3_UUCMAP_OFFSET,
                                      VR9_PCE_PMAP_3_UUCMAP_SHIFT,
                                      VR9_PCE_PMAP_3_UUCMAP_SIZE,
                                      &value);

        value |= 1 << 6;
        ifx_ethsw_ll_DirectAccessWrite(NULL,
                                       VR9_PCE_PMAP_3_UUCMAP_OFFSET,
                                       VR9_PCE_PMAP_3_UUCMAP_SHIFT,
                                       VR9_PCE_PMAP_3_UUCMAP_SIZE,

                                       value);
        AVMNET_ERR("[%s] Configuring CPU-port to receive all unknown unicast frames %#x\n", __func__, value);

        /*--- multicast ---*/
        ifx_ethsw_ll_DirectAccessRead(NULL,
                                      VR9_PCE_PMAP_2_DMCPMAP_OFFSET,
                                      VR9_PCE_PMAP_2_DMCPMAP_SHIFT,
                                      VR9_PCE_PMAP_2_DMCPMAP_SIZE,
                                      &value);

        value |= 1 << 6;
        ifx_ethsw_ll_DirectAccessWrite(NULL,
                                       VR9_PCE_PMAP_2_DMCPMAP_OFFSET,
                                       VR9_PCE_PMAP_2_DMCPMAP_SHIFT,
                                       VR9_PCE_PMAP_2_DMCPMAP_SIZE,
                                       value);
        AVMNET_ERR("[%s] Configuring CPU-port to receive all unknown multicast frames %#x\n", __func__, value);
        // clear mirror port map
        ifx_ethsw_ll_DirectAccessWrite(NULL,
                                       VR9_PCE_PMAP_1_MPMAP_OFFSET,
                                       VR9_PCE_PMAP_1_MPMAP_SHIFT,
                                       VR9_PCE_PMAP_1_MPMAP_SIZE,
                                       0);

        AVMNET_INFOTRC("[%s] Flush MAC Table \n", __func__);
        ifx_ethsw_ll_DirectAccessWrite(NULL,
                                       VR9_PCE_GCTRL_0_MTFL_OFFSET,
                                       VR9_PCE_GCTRL_0_MTFL_SHIFT,
                                       VR9_PCE_GCTRL_0_MTFL_SIZE,
                                       1);

        setup_non_ethernet_deault_port();

    }


}
EXPORT_SYMBOL(avmnet_swi_7port_disable_learning);

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

void avmnet_swi_7port_set_ethwan_mask(int wan_phy_mask) {
	AVMNET_DEBUG("[%s] %#x.\n", __func__, wan_phy_mask);
    *PMAC_EWAN_REG = wan_phy_mask & 0x0000003F;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_swi_7port_get_ethwan_mask(void) {
    return *PMAC_EWAN_REG & 0x0000003F;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
/*
 * sets devname to be AVM_ATA DEV; resets all other ethernet devices
 */
int avmnet_swi_7port_set_ethwan_dev_by_name( const char *devname ) {
    int wan_mask;

    // set AVM_PA flags
    wan_mask = avmnet_swi_ifx_common_set_ethwan_dev_by_name( devname );

    // set Bitmask in Switch
    avmnet_swi_7port_set_ethwan_mask( wan_mask );
    return wan_mask;
}
EXPORT_SYMBOL(avmnet_swi_7port_set_ethwan_dev_by_name);

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

int avmnet_swi_7port_exit(avmnet_module_t *this)
{
    int i, result;
    struct avmnet_swi_7port_context *ctx;

    AVMNET_INFO("[%s] Init on module %s called.\n", __func__, this->name);

    for(i = 0; i < this->num_children; ++i){
        result = this->children[i]->exit(this->children[i]);
        if(result != 0){
            AVMNET_WARN("Module %s: exit() failed on child %s\n", this->name, this->children[i]->name);
        }
    }

    /*
     * clean up our mess
     */
    ctx = (struct avmnet_swi_7port_context *) this->priv;

    this->priv = NULL;
    kfree(ctx);

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_swi_7port_setup_irq(avmnet_module_t *this __attribute__ ((unused)),
                                unsigned int on __attribute__ ((unused)))
{
    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
unsigned int avmnet_swi_7port_reg_read(avmnet_module_t *this  __maybe_unused,
                                     unsigned int addr, unsigned int reg)
{
    unsigned int data = 0xFFFF;

    if(in_interrupt()){
        AVMNET_ERR("[%s] in_interrupt()! Abort.\n", __func__);
        dump_stack();
        return data;
    }

    data = read_mdio_sched(addr, reg);
    AVMNET_DEBUG("[%s] addr %#x  reg %#x -> %#x\n", __FUNCTION__, addr, reg, data);

    return data;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_swi_7port_reg_write(avmnet_module_t *this  __maybe_unused, unsigned int addr,
                                unsigned int reg, unsigned int val)
{
    unsigned int result = 0;

    if(in_interrupt()){
        AVMNET_ERR("[%s] in_interrupt()! Abort.\n", __func__);
        dump_stack();
        return -EFAULT;
    }

    write_mdio_sched(addr, reg, val);
    AVMNET_DEBUG("[%s] addr %#x  reg %#x  data %#x\n", __FUNCTION__, addr, reg, val);

    return result;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_swi_7port_lock(avmnet_module_t *this __attribute__ ((unused)))
{
    if(in_interrupt()){
        AVMNET_ERR("[%s] in_interrupt()! Abort.\n", __func__);
        dump_stack();
        return -EFAULT;
    }

    return down_interruptible(&swi_7port_mutex);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void avmnet_swi_7port_unlock(avmnet_module_t *this __attribute__ ((unused)))
{
    up(&swi_7port_mutex);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_swi_7port_trylock(avmnet_module_t *this __attribute__ ((unused)))
{
    return down_trylock(&swi_7port_mutex);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void avmnet_swi_7port_status_changed(avmnet_module_t *this, avmnet_module_t *caller)
{
    int i;
    // struct avmnet_swi_7port_context *my_ctx = (struct avmnet_swi_7port_context *) this->priv;
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
int avmnet_swi_7port_set_status(avmnet_module_t *this __attribute__ ((unused)),
                              avmnet_device_t *device_id, avmnet_linkstatus_t status)
{
    avmnet_linkstatus_t old;
    struct avmnet_swi_7port_context *ctx __maybe_unused;

    ctx = (struct avmnet_swi_7port_context *) this->priv;

    if(device_id != NULL){
        old = device_id->status;

        if(old.Status != status.Status){
            AVMNET_INFOTRC("[%s] status change for device %s\n", __func__, device_id->device_name);
            AVMNET_INFOTRC("[%s] old status: powerup: %x link %x flowctrl: %x duplex: %x speed: %x\n", __func__, old.Details.powerup, old.Details.link, old.Details.flowcontrol, old.Details.fullduplex, old.Details.speed);
            AVMNET_INFOTRC("[%s] new status: powerup: %x link %x flowctrl: %x duplex: %x speed: %x\n", __func__, status.Details.powerup, status.Details.link, status.Details.flowcontrol, status.Details.fullduplex, status.Details.speed);

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

                if( ! this->trylock(this)){
                    if(  netif_queue_stopped(device_id->device)
                       && g_stop_datapath == 0
                       && ctx->suspend_cnt == 0
                       && !( device_id->flags & AVMNET_DEVICE_FLAG_DMA_TX_QUEUE_FULL ))
                    {
                        AVMNET_DEBUG("[%s] %s starting queues\n", __func__, device_id->device->name);
                        netif_tx_wake_all_queues(device_id->device);
                        AVMNET_DBG_TX_QUEUE_STATUS("starting queues for device %s,",  device_id->device->name);
                        AVMNET_DBG_TX_QUEUE_STATUS("  queue_stopped = %d",    netif_queue_stopped(device_id->device));
                        AVMNET_DBG_TX_QUEUE_STATUS("  netif_carrier_ok = %d", netif_carrier_ok(device_id->device));
                        AVMNET_DBG_TX_QUEUE_STATUS("  g_stop_datapath = %d",  g_stop_datapath);
                        AVMNET_DBG_TX_QUEUE_STATUS("  TX_QUEUE_FULL = %d",  device_id->flags & AVMNET_DEVICE_FLAG_DMA_TX_QUEUE_FULL );
                    } else {
                        AVMNET_DBG_TX_QUEUE_STATUS("NOT starting queues for device %s,",  device_id->device->name);
                        AVMNET_DBG_TX_QUEUE_STATUS("  queue_stopped = %d",    netif_queue_stopped(device_id->device));
                        AVMNET_DBG_TX_QUEUE_STATUS("  netif_carrier_ok = %d", netif_carrier_ok(device_id->device));
                        AVMNET_DBG_TX_QUEUE_STATUS("  g_stop_datapath = %d",  g_stop_datapath);
                        AVMNET_DBG_TX_QUEUE_STATUS("  TX_QUEUE_FULL = %d",    device_id->flags & AVMNET_DEVICE_FLAG_DMA_TX_QUEUE_FULL );
                    }
                    this->unlock(this);
                } else {
                    AVMNET_DBG_TX_QUEUE_STATUS("cannot acquire lock for device %s",  device_id->device->name);
                }
            }else{
                /*
                 * no link. Stop tx queues, update carrier flag and clear MAC table for this
                 * port.
                 */
                if(netif_carrier_ok(device_id->device)){
                    netif_carrier_off(device_id->device);
                }

                if(!netif_queue_stopped(device_id->device)){
                    AVMNET_DEBUG("[%s] %s stopping queues\n", __func__, device_id->device->name);
                    netif_tx_stop_all_queues(device_id->device);
                }

                AVMNET_DBG_TX_QUEUE_STATUS("no link on %s",  device_id->device->name);
            }
        }

    }else{
        AVMNET_ERR("[%s] received status update for invalid device\n", __func__);
        return -EINVAL;
    }

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

static void write_mdio_sched(unsigned int phyAddr, unsigned int regAddr, unsigned int data)
{
    unsigned int reg;

    do{
        schedule();
        /*--- reg = SW_READ_REG32(MDIO_CTRL_REG); ---*/
        reg = IFX_REG_R32(MDIO_CTRL_REG);
    }while(reg & MDIO_CTRL_MBUSY);

    reg = MDIO_READ_WDATA(data);
    /*--- SW_WRITE_REG32(reg, MDIO_WRITE_REG); ---*/
    IFX_REG_W32(reg, MDIO_WRITE_REG);
    reg = (MDIO_CTRL_OP_WR | MDIO_CTRL_PHYAD_SET(phyAddr) | MDIO_CTRL_REGAD(regAddr));
    reg |= MDIO_CTRL_MBUSY;
    /*--- SW_WRITE_REG32(reg, MDIO_CTRL_REG); ---*/
    IFX_REG_W32(reg, MDIO_CTRL_REG);

    do{
        schedule();
        /*--- reg = SW_READ_REG32(MDIO_CTRL_REG); ---*/
        reg = IFX_REG_R32(MDIO_CTRL_REG);
    }while(reg & MDIO_CTRL_MBUSY);

    return;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static unsigned short read_mdio_sched(unsigned int phyAddr, unsigned int regAddr)
{
    unsigned int reg;
    unsigned short data;

    do{
        schedule();
        /*--- reg = SW_READ_REG32(MDIO_CTRL_REG); ---*/
        reg = IFX_REG_R32(MDIO_CTRL_REG);
    }while(reg & MDIO_CTRL_MBUSY);

    reg = (MDIO_CTRL_OP_RD | MDIO_CTRL_PHYAD_SET(phyAddr) | MDIO_CTRL_REGAD(regAddr));
    reg |= MDIO_CTRL_MBUSY;

    /*--- SW_WRITE_REG32( reg, MDIO_CTRL_REG); ---*/
    IFX_REG_W32(reg, MDIO_CTRL_REG);
    do{
        schedule();
        /*--- reg = SW_READ_REG32(MDIO_CTRL_REG); ---*/
        reg = IFX_REG_R32(MDIO_CTRL_REG);
    }while(reg & MDIO_CTRL_MBUSY);

    /*--- reg = SW_READ_REG32(MDIO_CTRL_REG); ---*/
    reg = IFX_REG_R32(MDIO_READ_REG);
    data = MDIO_READ_RDATA(reg);

    return data;
}
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

int avmnet_swi_7port_poll(avmnet_module_t *this)
{
    int i, result;

#ifdef CONFIG_AVM_PA
    // clear any session that has been marked for deletion
    if(down_trylock(&swi_pa_mutex) == 0){
        struct avmnet_7port_pce_session *session, *next;

        list_for_each_entry_safe(session, next, &pce_sessions, list){
            if(session->removed){
                AVMNET_INFO("[%s] Found removed PA session %d, cleaning up.\n", __func__, session->handle);
                avmnet_7port_pce_remove_session_entry(session);
            }
        }

        up(&swi_pa_mutex);
    }
#endif // CONFIG_AVM_PA

    for(i = 0; i < this->num_children; ++i){
        if(this->children[i]->poll){
            result = this->children[i]->poll(this->children[i]);
            if(result != 0){
                AVMNET_WARN("Module %s: poll() failed on child %s\n", this->name, this->children[i]->name);
            }
        }
    }

    return 0;
}
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

#if !defined(CONFIG_IFX_ETHSW_API) || (CONFIG_IFX_ETHSW_API == 'n')
static void switch_reset(unsigned int on)
{
    unsigned int rst_bit;

    rst_bit = (1 << 21);

    if(on){
        ifx_rcu_rst_req_write(rst_bit, rst_bit);
    }else{
        ifx_rcu_rst_req_write(0, rst_bit);
    }
}
#endif


int avmnet_swi_7port_powerdown(avmnet_module_t *this __attribute__ ((unused)) )
{
    return 0;
}

int avmnet_swi_7port_powerup(avmnet_module_t *this __attribute__ ((unused)) )
{
    return 0;
}

#if defined(NETDEV_PORT_STATS_READ_RMON)
static uint64_t read_port_counter(unsigned int port, unsigned int offset,
                                  unsigned int is_double)
{
    uint64_t value;
    uint64_t val0, val1;

    while((SW_READ_REG32(ETHSW_BM_RAM_CTRL_REG) & BM_RAM_CTRL_BAS))
        ;

    SW_WRITE_REG32(offset, ETHSW_BM_RAM_ADDR_REG);
    SW_WRITE_REG32((BM_RAM_CTRL_BAS | port), ETHSW_BM_RAM_CTRL_REG);

    while((SW_READ_REG32(ETHSW_BM_RAM_CTRL_REG) & BM_RAM_CTRL_BAS))
        ;

    val0 = SW_READ_REG32(ETHSW_BM_RAM_VAL_0_REG);
    val1 = SW_READ_REG32(ETHSW_BM_RAM_VAL_1_REG);
    value = (val1 << 16) | (val0);

    if(is_double){
        ++offset;

        while((SW_READ_REG32(ETHSW_BM_RAM_CTRL_REG) & BM_RAM_CTRL_BAS))
            ;

        SW_WRITE_REG32(offset, ETHSW_BM_RAM_ADDR_REG);
        SW_WRITE_REG32((BM_RAM_CTRL_BAS | port), ETHSW_BM_RAM_CTRL_REG);

        while((SW_READ_REG32(ETHSW_BM_RAM_CTRL_REG) & BM_RAM_CTRL_BAS))
            ;

        val0 = SW_READ_REG32(ETHSW_BM_RAM_VAL_0_REG);
        val1 = SW_READ_REG32(ETHSW_BM_RAM_VAL_1_REG);
        value |= (val1 << 48) | (val0 << 32);
    }

    return value;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static void update_device_stats(unsigned int port)
{
#if 0
    if(in_interrupt()){
        AVMNET_ERR("[%s] in_interrupt()! Abort.\n", __func__);
        dump_stack();
        return;
    }
#endif

    if(down_trylock(&swi_7port_mutex)){
        AVMNET_INFO("[%s] semaphore locked, deferring stats update\n", __func__);
        return;
    }

    port_stats[port].rx_packets = read_port_counter(port, 0x1f, 0);
    port_stats[port].tx_packets = read_port_counter(port, 0x0c, 0);
    port_stats[port].rx_bytes = read_port_counter(port, 0x24, 1);
    port_stats[port].tx_bytes = read_port_counter(port, 0x0e, 1);

    port_stats[port].rx_crc_errors = read_port_counter(port, 0x21, 0);
    port_stats[port].rx_errors =  port_stats[port].rx_crc_errors;
    port_stats[port].rx_errors += read_port_counter(port, 0x1e, 0);
    port_stats[port].rx_errors += read_port_counter(port, 0x1c, 0);
    port_stats[port].rx_errors += read_port_counter(port, 0x1a, 0);

    port_stats[port].rx_dropped = read_port_counter(port, 0x18, 0);
    port_stats[port].tx_dropped = read_port_counter(port, 0x10, 0);

    port_stats[port].multicast = read_port_counter(port, 0x22, 0);
    //port_stats[port].collisions = read_port_counter(port, 0x18);


    up(&swi_7port_mutex);

}
#endif

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

#if defined(NETDEV_PORT_STATS_READ_RMON)
struct net_device_stats *avmnet_swi_7port_get_net_device_stats(struct net_device *dev)
{
    avmnet_device_t **avm_devp = netdev_priv(dev);
    unsigned int mac_nr = 0;

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

    // FIXME: will return stats for port 0 if no MAC module is found

    return &(port_stats[mac_nr]);
}
#else

struct net_device_stats *avmnet_swi_7port_get_net_device_stats(struct net_device *dev){

    avmnet_device_t **avm_devp = netdev_priv(dev);
    return &(*avm_devp)->device_stats;

}

#endif

static int get_rmon_data(int port)
{
    int i;
    unsigned int val0, val1, data;

    if(in_interrupt()){
        AVMNET_ERR("[%s] in_interrupt()! Abort.\n", __func__);
        dump_stack();
        return -EFAULT;
    }

    if(down_interruptible(&swi_7port_mutex)){
        AVMNET_WARN("[%s] interrupted while waiting for semaphore\n", __func__);
        return -EFAULT;
    }

    while((SW_READ_REG32(ETHSW_BM_RAM_CTRL_REG) & BM_RAM_CTRL_BAS))
        ;

    for(i = 0; i < RMON_COUNT_SIZE; i++){
        SW_WRITE_REG32(i, ETHSW_BM_RAM_ADDR_REG);
        SW_WRITE_REG32((BM_RAM_CTRL_BAS | port), ETHSW_BM_RAM_CTRL_REG);

        while((SW_READ_REG32(ETHSW_BM_RAM_CTRL_REG) & BM_RAM_CTRL_BAS))
            ;

        val0 = SW_READ_REG32(ETHSW_BM_RAM_VAL_0_REG);
        val1 = SW_READ_REG32(ETHSW_BM_RAM_VAL_1_REG);
        data = (val1 << 16) | (val0);

        rmon_data[port][i] = data;
    }

    up(&swi_7port_mutex);

    return 0;
}

static inline u64 con32(u32 high, u32 low){
	return ((u64)high << 32) | (u64)low;
}

struct avmnet_hw_rmon_counter *create_7port_rmon_cnt(avmnet_device_t *avm_dev){

    int mac_port;
    struct avmnet_hw_rmon_counter *res;

    if (!avm_dev || !avm_dev->mac_module)
        return NULL;

    res = kzalloc(sizeof(struct avmnet_hw_rmon_counter),GFP_KERNEL);
    if (!res)
        return NULL;

    mac_port = avm_dev->mac_module->initdata.mac.mac_nr;
    get_rmon_data(mac_port);

    res->rx_pkts_good = rmon_data[mac_port][31];
    res->tx_pkts_good = rmon_data[mac_port][12];

    res->rx_bytes_good = con32(rmon_data[mac_port][37], 
                               rmon_data[mac_port][36]);
    res->tx_bytes_good = con32(rmon_data[mac_port][15], 
                               rmon_data[mac_port][14]);

    res->rx_pkts_pause = rmon_data[mac_port][32];
    res->tx_pkts_pause = rmon_data[mac_port][13];

    res->rx_pkts_dropped = rmon_data[mac_port][24] +
        rmon_data[mac_port][25];
    res->tx_pkts_dropped = rmon_data[mac_port][16];

    res->rx_bytes_error = con32(rmon_data[mac_port][39],
                                rmon_data[mac_port][38]);

    return res;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static int read_rmon_all_show(struct seq_file *seq, void *data __attribute__ ((unused)) )
{
    int i;
    // avmnet_module_t *this __attribute__ ((unused));

    // this = (avmnet_module_t *) seq->private;

    // fetch current data from RMON tables
    for(i = 0; i < RMON_PORT_SIZE; ++i){
        get_rmon_data(i);
    }

    seq_printf(seq, "RMON Counter for Port:                     MII 0    MII 1    MII 2    MII 3    MII 4    MII 5    MII 6\n");
    seq_printf(seq, "======================                   ======== ======== ======== ======== ======== ======== ========\n");
    for(i = 0; i < RMON_COUNT_SIZE; ++i){
        seq_printf(seq, "%s %08x %08x %08x %08x %08x %08x %08x\n", rmon_names[i],
                   rmon_data[0][i], rmon_data[1][i], rmon_data[2][i],
                   rmon_data[3][i], rmon_data[4][i], rmon_data[5][i],
                   rmon_data[6][i]);
    }

//    ifx_pce_print_tables(&pce_table);
//    ifx_pce_tm_print_tables(&(pce_table.pce_sub_tbl));

    return 0;
}

static int read_rmon_all_open(struct inode *inode, struct file *file)
{
    avmnet_module_t *this;

    this = (avmnet_module_t *) PDE_DATA(inode);

    return single_open(file, read_rmon_all_show, this);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static ssize_t proc_mac_table_write(struct file *filp __maybe_unused, const char __user *buff,
                                    size_t len, loff_t *offset __attribute__ ((unused)) )
{
    char mybuff[128];
    // struct seq_file *seq;
    // avmnet_module_t *this;
    int mac, reg, value;
    int status;

    if(len >= 128){
        return -ENOMEM;
    }

    // seq = (struct seq_file *) filp->private_data;
    // this = (avmnet_module_t *) seq->private;

    copy_from_user(&mybuff[0], buff, len);
    mybuff[len] = 0;

    status = len;

    if(sscanf(mybuff, "read_pctrl %d", &mac) == 1){

    	if (mac < 0 || mac > 11 ){
    		status = -EINVAL;
    	} else {
    		unsigned int i;
    		for (i = 0; i < 3; i++){
    			ifx_ethsw_ll_DirectAccessRead(NULL, (0x480 + i + (0xA * mac)), 0, 16, &value);
    			printk(KERN_ERR "[mac=%d reg=%d] = %#x\n", mac, i, value);
    		}
    	}

    }
    else if(sscanf(mybuff, "write_pctrl %i %i %i", &mac, &reg, &value) == 3){

    	if ( ( mac < 0 ) || ( mac > 11 ) || (reg < 0 ) || (reg > 2)){
    		status = -EINVAL;
    	} else {
    		ifx_ethsw_ll_DirectAccessWrite(NULL, (0x480 + reg + (0xA * mac)), 0, 16, value);
    		printk(KERN_ERR "[write_ctl mac=%d, reg=%d] = %#x\n", mac, reg, value);
    	}

    }
    else if(strncmp(mybuff, "reset_table", 11 ) == 0) {
    	avmnet_swi_7port_disable_learning(avmnet_hw_config_entry->config);

    }
    else if(strncmp(mybuff, "flush_table", 11 ) == 0) {
        ifx_ethsw_ll_DirectAccessWrite(NULL,
                                       VR9_PCE_GCTRL_0_MTFL_OFFSET,
                                       VR9_PCE_GCTRL_0_MTFL_SHIFT,
                                       VR9_PCE_GCTRL_0_MTFL_SIZE,
                                       1);
    }
    else if(strncmp(mybuff, "setup_non_ethernet", 18) == 0) {
        setup_non_ethernet_deault_port();

    } else {
        status = -EINVAL;
    }

    if(status == -EINVAL){
        printk(KERN_ERR "[%s] Usage:\n", __func__);
        printk(KERN_ERR "[%s]     read_pctrl <mac> \n", __func__);
        printk(KERN_ERR "[%s]     write_pctrl <mac> <reg> <value>\n", __func__);
        printk(KERN_ERR "[%s]     flush_table \n", __func__);
        printk(KERN_ERR "[%s]     setup_non_ethernet \n", __func__);
        printk(KERN_ERR "[%s]     reset_table \n", __func__);
    }

    return status;
}



static int mac_table_show(struct seq_file *seq, void *data __attribute__ ((unused)) )
{
    IFX_ETHSW_XWAYFLOW_PCE_TABLE_ENTRY_t pData;
    unsigned int portmap, age, index;

    seq_printf(seq, "MAC table entries:\n");
    index = 0;
    do {
        pData.table = IFX_ETHSW_PCE_MAC_BRIDGE_INDEX;
        pData.table_index = index;

        ifx_ethsw_xwayflow_pce_table_read(NULL, &pData);
        if (pData.valid != 0){
            if(pData.val[1] & 0x1){
                portmap = pData.val[0];
                age = 0;
            } else {
                portmap = (pData.val[0] >> 4) & 0xF;
                age = pData.val[0] & 0xF;
            }

            seq_printf(seq, "%04d: %02x:%02x:%02x:%02x:%02x:%02x Portmap: %04x nFID: %x Age: %03ud\n",
                       pData.table_index,
                       pData.key[2] >> 8,
                       pData.key[2] & 0xFF,
                       pData.key[1] >> 8,
                       pData.key[1] & 0xFF,
                       pData.key[0] >> 8,
                       pData.key[0] & 0xFF,
                       portmap,
                       pData.key[3] & 0x3F,
                       age);
        }

        ++index;
    }while(pData.table_index < IFX_HW_MAC_TABLE_MAX);

    return 0;
}

static int mac_table_open(struct inode *inode, struct file *file)
{
    avmnet_module_t *this;

    this = (avmnet_module_t *) PDE_DATA(inode);

    return single_open(file, mac_table_show, this);
}

#ifdef CONFIG_AVM_PA
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
#if 0
static avmnet_device_t* get_avm_dev_from_pid(avm_pid_handle pid)
{
    int i;
    avmnet_device_t **avm_devices = avmnet_hw_config_entry->avm_devices;

    for(i = 0; i < avmnet_hw_config_entry->nr_avm_devices; ++i){
        	if(avm_devices[i]->device && 
               ( avm_devices[i]->device->avm_pa.devinfo.pid_handle == pid ) ) {
            	return avm_devices[i];
        	}
    }
    return NULL;
}
#endif

static int setup_pce(void)
{
    IFX_FLOW_PCE_rule_t rule;
    unsigned int index;

    memset(&rule, 0, sizeof(IFX_FLOW_PCE_rule_t));
    rule.pattern.bProtocolEnable = 1;
    rule.pattern.nProtocol = 0x06; // TCP
    rule.pattern.nProtocolMask = 0;


    rule.pattern.bAppDataMSB_Enable = 1;
    rule.pattern.bAppMaskRangeMSB_Select = 0;
    rule.pattern.nAppDataMSB = 2; // SYN
    rule.pattern.nAppMaskRangeMSB = 0xe;

    rmon_taken[0] = 4711;
    rule.action.ePortMapAction = IFX_FLOW_PCE_ACTION_PORTMAP_REGULAR;

    rule.action.bRMON_Action = 1;
    rule.action.nRMON_Id = 1; // nRMON_Id starts at 1

    rule.pattern.bEnable = 1;

    // find a free slot in the rules table
    index = 0;
    while(index < IFX_PCE_TBL_SIZE){
        if(pce_table.pce_tbl_used[index] == 0)
            break;

        ++index;
    }

    // no free slot found, clean up and abort
    if(index >= IFX_PCE_TBL_SIZE){
        AVMNET_ERR("[%s] No free slot in PCE table found.\n", __func__);
        return -EFAULT;
    }

    rule.pattern.nIndex = index;
    if(ifx_pce_rule_write(&pce_table, &rule) != 0){
        AVMNET_ERR("[%s] ifx_pce_rule_write() failed! Punting.\n", __func__);
        return -EFAULT;
    }

//    ifx_pce_print_tables(&pce_table);
//    ifx_pce_tm_print_tables(&(pce_table.pce_sub_tbl));

    rule.pattern.bAppDataMSB_Enable = 1;
    rule.pattern.bAppMaskRangeMSB_Select = 0;
    rule.pattern.nAppDataMSB = 1; // FIN
    rule.pattern.nAppMaskRangeMSB = 0xe;

    rmon_taken[1] = 4711;
    rule.action.ePortMapAction = IFX_FLOW_PCE_ACTION_PORTMAP_REGULAR;

    rule.action.bRMON_Action = 1;
    rule.action.nRMON_Id = 2; // nRMON_Id starts at 1

    rule.pattern.bEnable = 1;

    // find a free slot in the rules table
    while(index < IFX_PCE_TBL_SIZE){
        if(pce_table.pce_tbl_used[index] == 0)
            break;

        ++index;
    }

    // no free slot found, clean up and abort
    if(index >= IFX_PCE_TBL_SIZE){
        AVMNET_ERR("[%s] No free slot in PCE table found.\n", __func__);
        return -EFAULT;
    }

    rule.pattern.nIndex = index;
    if(ifx_pce_rule_write(&pce_table, &rule) != 0){
        AVMNET_ERR("[%s] ifx_pce_rule_write() failed! Punting.\n", __func__);
        return -EFAULT;
    }

//    ifx_pce_print_tables(&pce_table);
//    ifx_pce_tm_print_tables(&(pce_table.pce_sub_tbl));


    return 0;
}
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static int create_pce_rules(struct avmnet_7port_pce_session *session)
{
    unsigned int i, val0, val1, counter_val;
    int counter_id;

    IFX_FLOW_PCE_rule_t *rule;
    for(i = 0; i < session->negress; ++i){
        rule = &(session->pce_rules[i]);


        // match ingress port
        rule->pattern.bPortIdEnable = 1;
        rule->pattern.nPortId = session->ingress.mac_nr;

#if 0
        if (session->ingress.is_cpu_port){
            // match ingress MAC for CPU Port Rules (eg. distinguish Guest from Trusted WLAN)
            AVMNET_INFO("[%s] match ingress MAC for CPU Port Rules (eg. distinguish Guest from Trusted WLAN)\n", __func__);
            rule->pattern.bMAC_SrcEnable = 1;
            rule->pattern.nMAC_SrcMask = 0;
            memcpy(&(rule->pattern.nMAC_Src), &(session->ingress.header.h_source), ETH_ALEN);
        }
#endif

        // match egress MAC
        rule->pattern.bMAC_DstEnable = 1;
        rule->pattern.nMAC_DstMask = 0;
        memcpy(&(rule->pattern.nMAC_Dst), &(session->egress[i].header.h_dest), ETH_ALEN);

        // create specific forward portmap for this session
        rule->action.ePortMapAction = IFX_FLOW_PCE_ACTION_PORTMAP_ALTERNATIVE;
        rule->action.nForwardPortMap = (1 << session->egress[i].mac_nr);

        // set up an RMON counter target only for the first egress rule
        // there are 24 counter registers available, so we only set one up
        // if we can find a free register
        if(i == 0){

            // find a counter register not assigned to a PA session
            for(counter_id = 0; counter_id < PA_RMON_COUNTERS; ++counter_id){
                if(rmon_taken[counter_id] < 0)
                    break;
            }

            if(counter_id >= PA_RMON_COUNTERS){
                // no counter available
                session->ingress.counter_id = -1;
            } else {
                // found an available register
                rmon_taken[counter_id] = session->handle;
                session->ingress.counter_id = counter_id;

                // read out and remember current counter value
                while((SW_READ_REG32(ETHSW_BM_RAM_CTRL_REG) & BM_RAM_CTRL_BAS))
                    ;

                // assignable counters start at offset RMON_CNT_OFF (40)
                SW_WRITE_REG32(counter_id + PA_RMON_CNT_OFF, ETHSW_BM_RAM_ADDR_REG);
                SW_WRITE_REG32((BM_RAM_CTRL_BAS | ((session->ingress.mac_nr < 6)?(session->ingress.mac_nr):6)), ETHSW_BM_RAM_CTRL_REG);

                while((SW_READ_REG32(ETHSW_BM_RAM_CTRL_REG) & BM_RAM_CTRL_BAS))
                    ;

                val0 = SW_READ_REG32(ETHSW_BM_RAM_VAL_0_REG);
                val1 = SW_READ_REG32(ETHSW_BM_RAM_VAL_1_REG);
                counter_val = (val1 << 16) | (val0);

                session->ingress.counter_prev = counter_val;

                // activate RMON counting
                rule->action.bRMON_Action = 1;
                rule->action.nRMON_Id = counter_id + 1; // nRMON_Id starts at 1
                // printk("[%s] Rmon counter id: %d\n", __func__,rule->action.nRMON_Id );
            }
        }

        rule->pattern.bEnable = 1;
    }

    return 0;
}

static int set_pce_rules(struct avmnet_7port_pce_session *session)
{
    unsigned int i, index;
    int status = AVM_PA_TX_SESSION_ADDED;
    int need_cleanup;

    // add session rules to PCE
    need_cleanup = -1;
    for(i = 0; i < session->negress; ++i){

        // find a free slot in the rules table
        index = 0;
        while(index < IFX_PCE_TBL_SIZE){
            if(pce_table.pce_tbl_used[index] == 0)
                break;

            ++index;
        }

        // no free slot found, clean up and abort
        if(index >= IFX_PCE_TBL_SIZE){
            AVMNET_INFO("[%s] No free slot in PCE table found.\n", __func__);
            status = AVM_PA_TX_ERROR_SESSION;
            need_cleanup = i - 1;

            break;
        }

        // add rule and remember its table index
        session->pce_rules[i].pattern.nIndex = index;
        if(ifx_pce_rule_write(&pce_table, &(session->pce_rules[i])) != 0){
            AVMNET_ERR("[%s] ifx_pce_rule_write() failed! Punting.\n", __func__);
            status = AVM_PA_TX_ERROR_SESSION;
            need_cleanup = i - 1;

            break;
        }
    }

    // if something went wrong while adding a rule, remove all rules
    // added so far
    while(need_cleanup >= 0){
        if(ifx_pce_pattern_delete(&pce_table,
               session->pce_rules[need_cleanup].pattern.nIndex)!= 0)
        {
            AVMNET_ERR("[%s] Removing partial rule-set failed!\n", __func__);
        }

        --need_cleanup;
    }


    return status;
}



/*
 * setup own traffic class (and dma channel) for BC- and MC-packets,
 * as we cannot use storm control 
 *
 * as we cannot afford more PCE rules, we only detect the following MACs
 *
 * x1:xx:xx:xx:xx:xx
 * x3:xx:xx:xx:xx:xx
 * xf:xx:xx:xx:xx:xx
 *
 */
static void set_pce_set_bc_mc_class(const unsigned char class)
{
	unsigned int index;
	unsigned int i;
	u8 second_nibble[] = {0x01, 0x03, 0x0f};

	for (i=0; i < ARRAY_SIZE(second_nibble); i++){

		IFX_FLOW_PCE_rule_t d_rule;
		memset(&d_rule, 0, sizeof(IFX_FLOW_PCE_rule_t));
	
		d_rule.pattern.bMAC_DstEnable = 1;
		d_rule.pattern.nMAC_Dst[0] = (second_nibble[i] & 0x0f);
		d_rule.pattern.nMAC_DstMask = ~(1 <<1);

		d_rule.action.eTrafficClassAction =
			IFX_FLOW_PCE_ACTION_TRAFFIC_CLASS_ALTERNATIVE;

		d_rule.action.nTrafficClassAlternate = class;
		d_rule.pattern.bEnable = 1;
		
		index = 0;
		while (index < IFX_PCE_TBL_SIZE) {
			if (pce_table.pce_tbl_used[index] == 0)
				break;
			++index;
		}

		// no free slot found, clean up and abort
		if (index >= IFX_PCE_TBL_SIZE) {
			AVMNET_ERR("[%s] No free slot in PCE table found.\n", __func__);
			return;
		}

		d_rule.pattern.nIndex = index;

		pr_info("[%s] setting mac_addr (%pM) class %u\n",
		       __func__, d_rule.pattern.nMAC_Dst, (u32)class);

		if (ifx_pce_rule_write(&pce_table, &d_rule) != 0) {
			pr_err("[%s] setting mac_addr (%pM) class %u failed\n",
			       __func__, d_rule.pattern.nMAC_Dst, (u32)class);
		}
	}

}

/*
 * setup a critical drop precedence for packets with this
 * (s-mac && s-port) || d-mac
 */
static void set_pce_critical_mac(const u8 *mac_addr,
			  const u8 mac_port,
			  const u8 class)
{
	unsigned int index;

	IFX_FLOW_PCE_rule_t s_rule;
	IFX_FLOW_PCE_rule_t d_rule;

	memset(&s_rule, 0, sizeof(IFX_FLOW_PCE_rule_t));
	memset(&d_rule, 0, sizeof(IFX_FLOW_PCE_rule_t));

	pr_info("[%s] setting critical mac_addr rule for %pM, sw-m-port=%u\n",
	       __func__, mac_addr, (u32)mac_port);

	s_rule.pattern.bPortIdEnable = 1;
	s_rule.pattern.nPortId = mac_port;

	s_rule.pattern.bMAC_SrcEnable = 1;
	memcpy(s_rule.pattern.nMAC_Src, mac_addr, 6);
	s_rule.pattern.nMAC_SrcMask = (uint16_t)(~GENMASK(ETH_ALEN * 2, 0));

	s_rule.action.eTrafficClassAction =
	        IFX_FLOW_PCE_ACTION_TRAFFIC_CLASS_ALTERNATIVE;

	s_rule.action.nTrafficClassAlternate = class;

	s_rule.pattern.bEnable = 1;

	d_rule.pattern.bMAC_DstEnable = 1;
	memcpy(d_rule.pattern.nMAC_Dst, mac_addr, 6);
	d_rule.pattern.nMAC_DstMask = (uint16_t)(~GENMASK(ETH_ALEN * 2, 0));

	d_rule.pattern.bEnable = 1;

	// find a free slot in the rules table
	index = 0;
	while (index < IFX_PCE_TBL_SIZE) {
		if (pce_table.pce_tbl_used[index] == 0)
			break;

		++index;
	}

	// no free slot found, clean up and abort
	if (index >= IFX_PCE_TBL_SIZE) {
		AVMNET_ERR("[%s] No free slot in PCE table found.\n", __func__);
		return;
	}

	s_rule.pattern.nIndex = index;

#if 0
	/* 
	 * AVM/CBU: eCritFrameAction seems to have no effect, probably because
	 * of DMA-drop.
	 * We need DMA-drop (otherwise we suffer of HOL blocking in PPE). 
	 */

	s_rule.action.eCritFrameAction =
	        IFX_FLOW_PCE_ACTION_CRITICAL_FRAME_CRITICAL;
	d_rule.action.eCritFrameAction =
	        IFX_FLOW_PCE_ACTION_CRITICAL_FRAME_CRITICAL;
#endif

	if (ifx_pce_rule_write(&pce_table, &s_rule) != 0) {
		pr_err("[%s] setting critical mac_addr (%pM) s_rule failed\n",
		       __func__, mac_addr);
	}

	// find a free slot in the rules table
	index = 0;
	while (index < IFX_PCE_TBL_SIZE) {
		if (pce_table.pce_tbl_used[index] == 0)
			break;

		++index;
	}

	// no free slot found, clean up and abort
	if (index >= IFX_PCE_TBL_SIZE) {
		AVMNET_ERR("[%s] No free slot in PCE table found.\n", __func__);
		return;
	}

	d_rule.pattern.nIndex = index;

	if (ifx_pce_rule_write(&pce_table, &d_rule) != 0) {
		pr_err("[%s] setting critical mac_addr (%pM) d_rule failed\n",
		       __func__, mac_addr);
	}
}

static const char *match_type_str(enum avm_pa_match_type type)
{
    switch(type){
        case AVM_PA_ETH: return "AVM_PA_ETH";
        case AVM_PA_VLAN: return "AVM_PA_VLAN";
        case AVM_PA_PPPOE: return "AVM_PA_PPPOE";
        case AVM_PA_PPP: return "AVM_PA_PPP";
        case AVM_PA_IPV4: return "AVM_PA_IPV4";
        case AVM_PA_IPV6: return "AVM_PA_IPV6";
        case AVM_PA_PORTS: return "AVM_PA_PORTS";
        case AVM_PA_ICMPV4: return "AVM_PA_ICMPV4";
        case AVM_PA_ICMPV6: return "AVM_PA_ICMPV6";
        case AVM_PA_LLC_SNAP: return "AVM_PA_LLC_SNAP";
        case AVM_PA_LISP: return "AVM_PA_LISP";
        case AVM_PA_L2TP: return "AVM_PA_L2TP";
        case AVM_PA_GRE: return "AVM_PA_GRE";
        case AVM_PA_NUM_MATCH_TYPES: return "AVM_PA_NUM_MATCH_TYPES";
        case AVM_PA_ETH_PROTO: return "AVM_PA_ETH_PROTO";
        case AVM_PA_IP_PROTO: return "AVM_PA_IP_PROTO";
    }
    return "INVALID";
}

static bool bsession_meets_contraints(struct avm_pa_session *avm_session)
{
    unsigned n;
    struct avm_pa_egress *egress;
    unsigned negress=0;

    if(avm_session->bsession == NULL){
        AVMNET_INFO("[%s] Refusing non-bridging PA session\n", __func__);
        return false;
    }
    
    for (n=0; n < avm_session->ingress.nmatch; n++) {
        struct avm_pa_match_info *p = &avm_session->ingress.match[n];
        if (p->type != AVM_PA_ETH){
            AVMNET_INFO("[%s] bsession ingress-match %s is not supported\n", 
                   __func__, match_type_str(p->type));
            return false;
        }
    }
    avm_pa_for_each_egress(egress, avm_session) {
        negress++;

        for (n=0; n < egress->match.nmatch; n++) {
            struct avm_pa_match_info *p = &egress->match.match[n];
            if (p->type != AVM_PA_ETH){
                AVMNET_INFO("[%s] bsession egress-match %s is not supported\n", 
                       __func__, match_type_str(p->type));
                return false;
            }
        }
    }
    if (negress != 1){
        AVMNET_INFO("[%s] bsession with %d egress devices is not supported\n",
               __func__, negress);
        return false;
    }
    
    AVMNET_INFO("[%s] ok\n", __func__);
    return true;
}
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_7port_pce_add_session(struct avm_pa_session *avm_session) {
    unsigned int negress;
    int mac_nr;
    int status;
    struct avm_pa_pid_hwinfo *ingress_hw;
    struct avm_pa_pid_hwinfo *egress_hw;
    struct avmnet_7port_pce_session *new_session;
    struct avm_pa_egress *avm_egress;

    status = AVM_PA_TX_ERROR_SESSION;
    new_session = NULL;
    
    if(bsession_meets_contraints(avm_session) == false){
        AVMNET_INFO("[%s] contstraints not met\n", __func__);
        goto err_out;
    }
    ingress_hw = avm_pa_pid_get_hwinfo( avm_session->ingress_pid_handle );

    if (!ingress_hw || !(ingress_hw->flags & ( AVMNET_DEVICE_IFXPPA_VIRT_RX | AVMNET_DEVICE_IFXPPA_ETH_LAN )) ){
        AVMNET_INFO("[%s] Unknown ingress_hw type\n", __func__);
        goto err_out;
    }

    if ( (ingress_hw->flags & AVMNET_DEVICE_IFXPPA_ETH_WAN ) && in_avm_ata_mode()) {
        AVMNET_INFO("[%s] no bridging acceleration for AVM ATA device [ingress]\n", __func__);
        goto err_out;
    }

    new_session = kzalloc(sizeof(struct avmnet_7port_pce_session), GFP_ATOMIC);
    if(new_session == NULL){
        AVMNET_WARN("[%s] Could not allocate memory for new session.\n", __func__);
        goto err_out;
    }

    if(down_trylock(&swi_pa_mutex)){
        AVMNET_INFO("[%s] PA mutex taken, aborting.\n", __func__);
        ++add_session_mutex_failure;
        goto err_out;
    }

    new_session->handle = avm_session->session_handle;
    new_session->ingress.mac_nr = ingress_hw->mac_nr;

    if ( ingress_hw->flags & AVMNET_DEVICE_IFXPPA_VIRT_RX ){
        new_session->ingress.mac_nr = ingress_hw->virt_rx_dev->hw_dev_id + IFX_VIRT_DEV_EXTENDED_OFFSET;
        new_session->ingress.is_cpu_port = 1;
    }

    AVMNET_INFO("[%s] Adding new ingress device for pid handle %d on port %d\n",
                 __func__, avm_session->ingress_pid_handle, new_session->ingress.mac_nr);

    memcpy(&(new_session->ingress.header),
#if defined(AVM_PA_BSESSION_STRUCT) && AVM_PA_BSESSION_STRUCT == 2
           (struct ethhdr *) avm_session->bsession->hdr,
#else
           &(avm_session->bsession->ethh),
#endif
           sizeof(struct ethhdr));

    negress = 0;
    avm_pa_for_each_egress(avm_egress, avm_session) {
        egress_hw = avm_pa_pid_get_hwinfo( avm_egress->pid_handle );

        if( egress_hw == NULL ||
            !(egress_hw->flags & ( AVMNET_DEVICE_IFXPPA_VIRT_TX | AVMNET_DEVICE_IFXPPA_ETH_LAN )))
        {
            AVMNET_INFO("[%s] Ignoring unknown egress device for pid handle %d\n", __func__, avm_session->ingress_pid_handle);
            continue;
        }

        if ( (egress_hw->flags & AVMNET_DEVICE_IFXPPA_ETH_WAN ) && in_avm_ata_mode()) {
            AVMNET_INFO("[%s] no hw bridging acceleration for AVM ATA device [egress]\n", __func__);
            continue;
        }

        if ( egress_hw->flags & AVMNET_DEVICE_IFXPPA_VIRT_TX ) {
            AVMNET_INFO("[%s] no hw bridging acceleration for virt_channel_tx devices\n", __func__);
            continue;
        }

        mac_nr = egress_hw->mac_nr;

        /* if both ingress and egress are LAN ports, refuse acceleration
         * from fast to slow port */
        if(((ingress_hw->flags & egress_hw->flags) & AVMNET_DEVICE_IFXPPA_ETH_LAN)
           && speed_on_mac_nr(ingress_hw->mac_nr) > speed_on_mac_nr(egress_hw->mac_nr))
        {
            AVMNET_INFO( "[%s] no acc from high speed to low speed lan port (srcmac =%d, dstmac =%d) \n",
                         __FUNCTION__ , ingress_hw->mac_nr, egress_hw->mac_nr);
            continue;
        }

        if(ingress_hw->flags & AVMNET_DEVICE_IFXPPA_VIRT_RX){
            if(speed_on_mac_nr(mac_nr) == 2){
                AVMNET_INFO( "[%s] do acc VIRT_RX -> 1000mbit lan/ata (dstmac =%d) \n", __FUNCTION__ , mac_nr );
            }else{
                AVMNET_INFO( "[%s] no acc for VIRT_RX -> 100/10mbit lan/ata (dstmac =%d) \n", __FUNCTION__ , mac_nr );
                continue;
            }

        }

        AVMNET_INFO("[%s] Egress MAC: %d\n", __func__, mac_nr);
        AVMNET_INFO("[%s] Adding new egress device for pid handle %d on port %d\n",
                __func__, avm_session->ingress_pid_handle, mac_nr);

        new_session->egress[negress].mac_nr = mac_nr;
        memcpy(&(new_session->egress[negress].header),
#if defined(AVM_PA_BSESSION_STRUCT) && AVM_PA_BSESSION_STRUCT == 2
               (struct ethhdr *) avm_session->bsession->hdr,
#else
               &(avm_session->bsession->ethh),
#endif
               sizeof(struct ethhdr));

        ++negress;
    }
    new_session->negress = negress;

    if(negress == 0){
        AVMNET_INFO("[%s] No egress ports for session found, aborting\n", __func__);
        goto err_out_locked;
    }

    create_pce_rules(new_session);
    status = set_pce_rules(new_session);

    AVMNET_INFO("[%s] Adding session result: %#x\n", __func__, status);

    if(status == AVM_PA_TX_SESSION_ADDED){
        avm_session->hw_session = new_session;
        list_add_tail(&(new_session->list), &pce_sessions);
    }

err_out_locked:
    up(&swi_pa_mutex);

err_out:
    if(new_session != NULL && status != AVM_PA_TX_SESSION_ADDED){
        kfree(new_session);
    }

    return status;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

static int unset_pce_rules(struct avmnet_7port_pce_session *session)
{
    unsigned int i, val0, val1, counter;
    int status = AVM_PA_TX_OK;

    for(i = 0; i < session->negress; ++i){
        if(ifx_pce_pattern_delete(&pce_table, session->pce_rules[i].pattern.nIndex) != 0){
            AVMNET_ERR("[%s] Deleting PCE rule failed!\n", __func__);
            status = AVM_PA_TX_ERROR_SESSION;
        }
    }

    //
    if(session->ingress.counter_id >= 0 && session->ingress.counter_id < PA_RMON_COUNTERS){
        while((SW_READ_REG32(ETHSW_BM_RAM_CTRL_REG) & BM_RAM_CTRL_BAS))
            ;

        SW_WRITE_REG32(session->ingress.counter_id + PA_RMON_CNT_OFF, ETHSW_BM_RAM_ADDR_REG);
        SW_WRITE_REG32((BM_RAM_CTRL_BAS | ((session->ingress.mac_nr < 6)?(session->ingress.mac_nr):6)), ETHSW_BM_RAM_CTRL_REG);

        while((SW_READ_REG32(ETHSW_BM_RAM_CTRL_REG) & BM_RAM_CTRL_BAS))
            ;

        val0 = SW_READ_REG32(ETHSW_BM_RAM_VAL_0_REG);
        val1 = SW_READ_REG32(ETHSW_BM_RAM_VAL_1_REG);
        counter = (val1 << 16) | (val0);
        session->ingress.counter_post = counter;

        rmon_taken[session->ingress.counter_id] = -1;

        AVMNET_INFO("[%s] Session %u mac_nr: %u pre: %u post:%u\n", __func__, session->handle,
                session->ingress.mac_nr, session->ingress.counter_prev, session->ingress.counter_post);
    }

    return status;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

int avmnet_7port_pce_remove_session(struct avm_pa_session *avm_session){

    struct avmnet_7port_pce_session *pce_session;
    int status;

    pce_session = (struct avmnet_7port_pce_session *) avm_session->hw_session;
    avm_session->hw_session = NULL;

    AVMNET_INFO("[%s] delete session %p", __func__, pce_session);

    if(down_trylock(&swi_pa_mutex)){
        // List of active PA sessions is locked, just mark this one as to
        // be removed and let our heart beat func clean it up
        pce_session->removed = 1;
        return AVM_PA_TX_OK;
    } else {
        status = avmnet_7port_pce_remove_session_entry(pce_session);
        up(&swi_pa_mutex);
    }

    return status;
}

/*------------------------------------------------------------------------------------------*\
 * free an PA session in the list
 * Must be called with the swi_pa_mutex held!
\*------------------------------------------------------------------------------------------*/
int avmnet_7port_pce_remove_session_entry(struct avmnet_7port_pce_session *session)
{
    int status;

    AVMNET_INFO("[%s] Freeing Session %p\n", __func__, session);

    list_del(&(session->list));
    status = unset_pce_rules(session);
    kfree(session);

    return status;
}

/*------------------------------------------------------------------------------------------*\
 * read an PA session count
 * Must be called with the swi_7port_mutex held!
\*------------------------------------------------------------------------------------------*/
static uint32_t read_rmon_pce_counter(struct avmnet_7port_pce_session *pce_session)
{
	unsigned int val0, val1;
	uint32_t counter = 0;


	if(pce_session->ingress.counter_id >= 0 &&
	   pce_session->ingress.counter_id < PA_RMON_COUNTERS) {
		while((SW_READ_REG32(ETHSW_BM_RAM_CTRL_REG) & BM_RAM_CTRL_BAS))
			;

		SW_WRITE_REG32(pce_session->ingress.counter_id + PA_RMON_CNT_OFF, ETHSW_BM_RAM_ADDR_REG);
		SW_WRITE_REG32((BM_RAM_CTRL_BAS | ((pce_session->ingress.mac_nr < 6)?(pce_session->ingress.mac_nr):6)), ETHSW_BM_RAM_CTRL_REG);

		while((SW_READ_REG32(ETHSW_BM_RAM_CTRL_REG) & BM_RAM_CTRL_BAS))
			;

		val0 = SW_READ_REG32(ETHSW_BM_RAM_VAL_0_REG);
		val1 = SW_READ_REG32(ETHSW_BM_RAM_VAL_1_REG);
		counter = (val1 << 16) | (val0);

		// check for counter overflow
		if(counter >= pce_session->ingress.counter_prev){
			counter -= pce_session->ingress.counter_prev;
		} else {
			counter = ~(pce_session->ingress.counter_prev - counter);
			++counter;
		}
	}
	return counter;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static int pa_sessions_show(struct seq_file *seq, void *data __attribute__ ((unused)) )
{
    struct avmnet_7port_pce_session *session;
    struct pce_7port_pa_port *port;
    avmnet_device_t *avmdev;
    unsigned int i;
    uint32_t counter;

    if(in_interrupt()){
        AVMNET_ERR("[%s] in_interrupt()! Abort.\n", __func__);
        dump_stack();
        return -EFAULT;
    }

    if(down_interruptible(&swi_pa_mutex)){
        AVMNET_WARN("[%s] Interrupted while waiting for mutex, aborting\n", __func__);

        return -EFAULT;
    }

    // ifx_pce_print_tables(&pce_table);
    // ifx_pce_tm_print_tables(&(pce_table.pce_sub_tbl));

    seq_printf(seq, "Active PA PCE sessions:\n");

    list_for_each_entry(session, &pce_sessions, list){

        port = &(session->ingress);
        avmdev = mac_nr_to_avm_dev(port->mac_nr);

        if(down_interruptible(&swi_7port_mutex)){
            AVMNET_WARN("[%s] interrupted while waiting for semaphore\n", __func__);
            return -1;
        }
        counter = read_rmon_pce_counter(session);
        up(&swi_7port_mutex);

        if((add_session_mutex_failure > 0) || (report_session_mutex_failure > 0))
            seq_printf(seq, "Mutex failcount: add_session=%u, report_session=%u \n",
                       add_session_mutex_failure, report_session_mutex_failure);

        seq_printf(seq, "Session handle: %d ; Counterid=%d; Accelerated Packets: %u\n",
                   session->handle, session->ingress.counter_id, counter);

        seq_printf(seq, "  Ingress MAC: %02x:%02x:%02x:%02x:%02x:%02x HW-Port: %d Dev: %s [mac_nr=%d]\n",
                port->header.h_source[0], port->header.h_source[1], port->header.h_source[2],
                port->header.h_source[3], port->header.h_source[4], port->header.h_source[5],
                port->mac_nr, avmdev ? (avmdev->device->name) : "no_avmnet_dev", port->mac_nr);

        for(i = 0; i < session->negress; ++i){
            port = &(session->egress[i]);
            avmdev = mac_nr_to_avm_dev(port->mac_nr);

            seq_printf(seq, "\n");
            seq_printf(seq, "  Egress  MAC: %02x:%02x:%02x:%02x:%02x:%02x HW-Port: %d Dev: %s [mac_nr=%d]\n",
                    port->header.h_dest[0], port->header.h_dest[1], port->header.h_dest[2],
                    port->header.h_dest[3], port->header.h_dest[4], port->header.h_dest[5],
                    port->mac_nr, avmdev ? (avmdev->device->name) : "no_avmnet_dev", port->mac_nr);
        }

    }

    up(&swi_pa_mutex);

    return 0;
}

static int pa_sessions_open(struct inode *inode, struct file *file)
{
    avmnet_module_t *this;

    this = (avmnet_module_t *) PDE_DATA(inode);

    return single_open(file, pa_sessions_show, this);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_7port_pce_session_stats(struct avm_pa_session *session,
				   struct avm_pa_session_stats *stats)
{
	uint32_t count;
	uint32_t diff;
	struct avmnet_7port_pce_session *pce_session = session->hw_session;

	stats->validflags = 0;
	stats->tx_bytes = 0;
	stats->tx_pkts = 0;

	if (!pce_session)
		return -1;

	if(down_trylock(&swi_7port_mutex)){
		AVMNET_INFO("[%s] no statistics in this cycle (mutex in use)\n", __func__);
		report_session_mutex_failure += 1;
	        return -1;
	}
	count = read_rmon_pce_counter(pce_session);
	up(&swi_7port_mutex);

	diff = count - pce_session->last_report;
	pce_session->last_report = count;

	if (diff > 0){
		stats->validflags |= AVM_PA_SESSION_STATS_VALID_HIT;
		stats->validflags |= AVM_PA_SESSION_STATS_VALID_PKTS;
		stats->tx_pkts = diff;
	}

	return 0;
}

#endif

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static int proc_ssm_show(struct seq_file *seq, void *data __attribute__ ((unused)) )
{
    unsigned int i;

    if (ssm_error[0][0]) {
            seq_printf(seq, "PS 0x%x MPS 0x%x FAB 0x%x 0x%x\n", *(volatile u32*)(IFX_MPS + 0x344), 
                                                                *(volatile u32*)(IFX_MPS + 0x34C), 
                                                                *(volatile u32*)(IFX_MPS + 0x354), 
                                                                *(volatile u32*)(IFX_MPS + 0x358));
    }

    for ( i = 0; i < SSM_MAX_ERROR; i++) {
        if (ssm_error[i][0]) {
            seq_printf(seq, "0x%x 0x%x\n", ssm_error[i][0], ssm_error[i][1]);
        }
    }
    return 0;
}

static int proc_ssm_open(struct inode *inode, struct file *file)
{
    avmnet_module_t *this;

    this = (avmnet_module_t *) PDE_DATA(inode);

    return single_open(file, proc_ssm_show, this);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static int proc_mirror_show(struct seq_file *seq, void *data __attribute__ ((unused)) )
{
    unsigned int mac;
    uint16_t pctrl, pmap;

    pmap = SW_READ_REG32(PCE_PMAP_REG(1));

    seq_printf(seq, "MPMAP: 0x%04x\n", pmap);

    for(mac = 0; mac <= 6; ++mac){
        pctrl = SW_READ_REG32(PCE_PCTRL_REG(mac, 3));
        seq_printf(seq, "MAC %d: %s %s \n", mac, (pctrl & (1 << 10)) ? "rxall": ((pctrl & (1 << 9))? "rx": "  "), (pctrl & (1 << 8)) ? "tx":"  ");
    }

    return 0;
}

static int proc_mirror_open(struct inode *inode, struct file *file)
{
    avmnet_module_t *this;

    this = (avmnet_module_t *) PDE_DATA(inode);

    return single_open(file, proc_mirror_show, this);
}

static ssize_t proc_mirror_write(struct file *filp, const char __user *buff,
                                    size_t len, loff_t *offset __attribute__ ((unused)) )
{
    char mybuff[128], action[16], direction[16];
    struct seq_file *seq;
    avmnet_module_t *this;
    unsigned int mac, add, rx;
    int status;
    struct avmnet_swi_7port_context *ctx;
    uint16_t pctrl, pmap, tmp = 0;

    if(len >= 128){
        return -ENOMEM;
    }

    seq = (struct seq_file *) filp->private_data;
    this = (avmnet_module_t *) seq->private;
    ctx = (struct avmnet_swi_7port_context *) this->priv;

    copy_from_user(&mybuff[0], buff, len);
    mybuff[len] = 0;

    status = len;

    if(sscanf(mybuff, "mport %4s %u", &action[0], &mac) == 2){
        if(strncmp(&action[0], "add", 4) == 0){
            add = 1;
        } else if(strncmp(&action[0], "del", 4) == 0){
            add = 0;
        }else{
            printk(KERN_ERR "[%s] unable to parse action %s\n", __func__, &action[0]);
            status = -EINVAL;
            goto err_out;
        }

        if(mac > 6){
            status = -EINVAL;
            goto err_out;
        }

        if(this->lock(this)){
            AVMNET_WARN("[%s] Interrupted while waiting for mutex, aborting\n", __func__);

            return -EFAULT;
        }

        pmap = SW_READ_REG32(PCE_PMAP_REG(1));

        if(add){
            pmap |= (1 << mac);
        }else{
            pmap &= ~(1 << mac);
        }

        SW_WRITE_REG32(pmap, PCE_PMAP_REG(1));

        this->unlock(this);

    } else if(sscanf(mybuff, "mirror %4s %u %6s", &action[0], &mac, &direction[0]) == 3){
        if(strncmp(&action[0], "add", 4) == 0){
            add = 1;
        } else if(strncmp(&action[0], "del", 4) == 0){
            add = 0;
        }else{
            printk(KERN_ERR "[%s] unable to parse action %s\n", __func__, &action[0]);
            status = -EINVAL;
            goto err_out;
        }

        if(strncmp(&direction[0], "rxall", 6) == 0){
            rx = 2;
        } else if(strncmp(&direction[0], "rx", 3) == 0){
            rx = 1;
        } else if(strncmp(&direction[0], "tx", 3) == 0){
            rx = 0;
        } else {
            printk(KERN_ERR "[%s] unable to parse direction %s\n", __func__, &direction[0]);
            status = -EINVAL;
            goto err_out;
        }

        if(mac > 6){
            status = -EINVAL;
            goto err_out;
        }

        switch(rx){
            case 0:
                tmp |= (1 << 8);
                break;
            case 2:
                tmp |= (1 << 10)| (1 << 7)| (1 << 6)| (1 << 5)| (1 << 4)| (1 << 3)| (1 << 2)| (1 << 1)| (1 << 0);
            case 1:
                tmp |= (1 << 9);
                break;
        }

        if(this->lock(this)){
            AVMNET_WARN("[%s] Interrupted while waiting for mutex, aborting\n", __func__);

            return -EFAULT;
        }

        pctrl = SW_READ_REG32(PCE_PCTRL_REG(mac, 3));

        if(add){
            pctrl |= tmp;
        } else {
            pctrl &= ~(tmp);
        }

        SW_WRITE_REG32(pctrl, PCE_PCTRL_REG(mac, 3));

        this->unlock(this);
    } else {
        status = -EINVAL;
    }

err_out:

    if(status == -EINVAL){
        printk(KERN_ERR "[%s] Usage:\n", __func__);
        printk(KERN_ERR "[%s]     mport  <add|del> <mac-nr>\n", __func__);
        printk(KERN_ERR "[%s]          add/delete port to/from portmap reveiving mirrored frames\n", __func__);
        printk(KERN_ERR "[%s]     mirror <add|del> <mac-nr> <rx|tx|rxall>\n", __func__);
        printk(KERN_ERR "[%s]          enable/disable mirroring of rx/tx frames from this port (rxall mirrors even invalid frames, which will be dropped)\n", __func__);
    }

    return status;
}


