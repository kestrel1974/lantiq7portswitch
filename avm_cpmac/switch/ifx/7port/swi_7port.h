#if !defined(__AVM_NET_SWI_VR9_)
#define __AVM_NET_SWI_VR9_

#include <avmnet_module.h>
#include <linux/list.h>
#include <linux/workqueue.h>

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
#include <switch_api/ifx_ethsw_pce.h>
#endif


#if !defined(__BIG_ENDIAN)
#error Bitfelder sind auf Big-Endian ausgelegt
#endif

#define MAX_ETH_INF 	8



#define RMON_COUNT_SIZE         64
#define RMON_PORT_SIZE          7
#define RMON_ASSIGN_OFFSET      40

extern struct net_device *g_eth_net_dev[MAX_ETH_INF];

#include "../common/swi_ifx_common.h"

int proc_pp32_input(char *line, void *data);
void proc_pp32_output(struct seq_file *m, void *data);
int proc_mem_input(char *line, void *data);

int ifx_ppa_eth_init(struct net_device *);
int ifx_ppa_eth_init_e5(struct net_device *);
int ifx_ppa_eth_open(struct net_device *);
int ifx_ppa_eth_stop(struct net_device *);
int ifx_ppa_eth_hard_start_xmit(struct sk_buff *, struct net_device *);
int ifx_ppa_eth_hard_start_xmit_e5(struct sk_buff *, struct net_device *);
int ifx_ppa_eth_qos_hard_start_xmit(struct sk_buff *, struct net_device *);
int ifx_ppa_eth_ioctl(struct net_device *, struct ifreq *, int);
void ifx_ppa_eth_tx_timeout(struct net_device *);
void ifx_ppa_eth_tx_timeout_e5(struct net_device *dev);
struct net_device_stats *ifx_ppa_eth_get_stats(struct net_device *);
void ifx_ppa_setup_eth(struct net_device* dev);
void ifx_ppa_setup_ptm_net_dev(struct net_device* dev);
int ifx_ppa_ptm_init(struct net_device *dev);

int ppe_eth_init(void);
void ifx_ppa_setup_priv(avmnet_device_t *);

void ifx_ppa_eth_tx_timeout_HW223(struct net_device *);
int ifx_ppa_eth_hard_start_xmit_HW223(struct sk_buff *, struct net_device *);
void hw223_late_init(avmnet_device_t *avm_dev);

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/


void avmnet_swi_7port_force_macs_down( avmnet_module_t *this );
void avmnet_swi_7port_reinit_macs(avmnet_module_t *this);

void avmnet_swi_7port_disable_learning(avmnet_module_t *this);

void avmnet_swi_7port_set_ethwan_mask(int wan_phy_mask);

/*
 * returns wan_mask, which has been setup in switch;
 * wan_mask == 0: devname not found
 */
int avmnet_swi_7port_set_ethwan_dev_by_name( const char *devname );

int avmnet_swi_7port_init(avmnet_module_t *this);
int avmnet_swi_7port_setup(avmnet_module_t *this);
int avmnet_swi_7port_exit(avmnet_module_t *this);

unsigned int avmnet_swi_7port_reg_read(avmnet_module_t *this, unsigned int addr,
                                        unsigned int reg);
int avmnet_swi_7port_reg_write(avmnet_module_t *this, unsigned int addr, unsigned int reg,
                                unsigned int val);
int avmnet_swi_7port_lock(avmnet_module_t *this);
void avmnet_swi_7port_unlock(avmnet_module_t *this);
int avmnet_swi_7port_trylock(avmnet_module_t *this);
void avmnet_swi_7port_status_changed(avmnet_module_t *this, avmnet_module_t *child);
int avmnet_swi_7port_poll(avmnet_module_t *this);
int avmnet_swi_7port_setup_irq(avmnet_module_t *this, unsigned int on);
int avmnet_swi_7port_set_status(avmnet_module_t *this, avmnet_device_t *device_id,
                                avmnet_linkstatus_t status);
int avmnet_swi_7port_powerdown(avmnet_module_t *this);
int avmnet_swi_7port_powerup(avmnet_module_t *this);
int avmnet_swi_7port_suspend(avmnet_module_t *this, avmnet_module_t *caller);
int avmnet_swi_7port_resume(avmnet_module_t *this, avmnet_module_t *caller);

struct net_device_stats *avmnet_swi_7port_get_net_device_stats(struct net_device *dev);

struct avmnet_swi_7port_context
{
    int num_avm_devices;
    avmnet_device_t **avm_devices;
    int suspend_cnt;
};

struct pce_7port_pa_port{
    unsigned int mac_nr;
    unsigned int is_cpu_port;
    struct ethhdr header;
    int counter_id;
    unsigned int counter_prev;
    unsigned int counter_post;
};

#ifdef CONFIG_AVM_PA
struct avmnet_7port_pce_session
{
    struct list_head list;
    avm_session_handle handle;
    struct pce_7port_pa_port ingress;
    unsigned int negress;
    struct pce_7port_pa_port egress[AVM_PA_MAX_EGRESS];
    IFX_FLOW_PCE_rule_t pce_rules[AVM_PA_MAX_EGRESS];
    unsigned int removed;
    unsigned int last_report;
};
#endif

#endif
