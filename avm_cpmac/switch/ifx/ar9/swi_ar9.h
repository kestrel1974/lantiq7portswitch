#if !defined(__AVMNET_SWI_AR9_)
#define __AVMNET_SWI_AR9_

#include <linux/netdevice.h>

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
#endif

#include <avmnet_module.h>

#define MAX_ETH_INF 2

extern struct net_device *g_eth_net_dev[MAX_ETH_INF];

#include "../common/swi_ifx_common.h"

extern int num_ar9_eth_devs;
extern struct dma_device_info *g_dma_device_ppe;
extern volatile int g_stop_datapath;
DECLARE_PER_CPU(struct softnet_data, g_ar9_eth_queue);

int proc_read_pp32(char *page, char **start, off_t off, int count, int *eof, void *data);
int proc_write_pp32(struct file *file, const char *buf, unsigned long count, void *data);
int proc_write_mem(struct file *file, const char *buf, unsigned long count, void *data);

int avmnet_swi_ar9_init(avmnet_module_t *this);
int avmnet_swi_ar9_setup(avmnet_module_t *this);
int avmnet_swi_ar9_exit(avmnet_module_t *this);
void reinit_ar9_common_eth(avmnet_module_t *this);

unsigned int avmnet_swi_ar9_reg_read(avmnet_module_t *this, unsigned int addr, unsigned int reg);
int avmnet_swi_ar9_reg_write(avmnet_module_t *this, unsigned int addr, unsigned int reg, unsigned int val);
int avmnet_swi_ar9_lock(avmnet_module_t *this);
void avmnet_swi_ar9_unlock(avmnet_module_t *this);
int avmnet_swi_ar9_trylock(avmnet_module_t *this);
void avmnet_swi_ar9_status_changed(avmnet_module_t *this, avmnet_module_t *child);
int avmnet_swi_ar9_poll(avmnet_module_t *this);
int avmnet_swi_ar9_setup_irq(avmnet_module_t *this, unsigned int on);
int avmnet_swi_ar9_set_status(avmnet_module_t *this, avmnet_device_t *device_id, avmnet_linkstatus_t status);
int avmnet_swi_ar9_powerdown(avmnet_module_t *this);
int avmnet_swi_ar9_powerup(avmnet_module_t *this);
int avmnet_swi_ar9_suspend(avmnet_module_t *this, avmnet_module_t *caller);
int avmnet_swi_ar9_resume(avmnet_module_t *this, avmnet_module_t *caller);
struct net_device_stats *avmnet_swi_ar9_get_net_device_stats(struct net_device *dev);
int avmnet_swi_ar9_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd);
void avmnet_swi_ar9_tx_timeout(struct net_device *dev);
int avmnet_swi_ar9_start_xmit(struct sk_buff *skb, struct net_device *dev);
int avmnet_swi_ar9_set_mac_address(struct net_device *dev, void *p);
int avmnet_swi_ar9_eth_init(struct net_device *dev);
void avmnet_swi_ar9_setup_eth(struct net_device *dev);
void avmnet_swi_ar9_setup_eth_priv(avmnet_device_t *avm_dev);
int avmnet_swi_ar9_get_itf_mask(const char *);
void avmnet_swi_ar9_disable_learning(avmnet_module_t *this);
void avmnet_swi_ar9_reinit_macs(avmnet_module_t *this);
int avmnet_swi_ar9_hw_setup(avmnet_module_t *this);
int dma_shutdown_gracefully_ar9(void);

void eth_inactivate_poll(struct dma_device_info* dma_dev __attribute__ ((unused)));
void eth_activate_poll(struct dma_device_info* dma_dev __attribute__ ((unused)));

struct avmnet_swi_ar9_context
{
    struct semaphore mutex;
    int suspend_cnt;
};

struct ar9_pa_port{
    unsigned int mac_nr;
    struct ethhdr header;
    int mac_tbl_idx;
};

struct avmnet_ar9_pa_session
{
    struct list_head list;
    avm_session_handle session_id;
    struct avm_pa_session *pa_session;
    int removed;

    struct ar9_pa_port ingress;
    unsigned int negress;
    struct ar9_pa_port egress[AVM_PA_MAX_EGRESS];
};

struct mac_entry
{
    unsigned char addr[ETH_ALEN];
    unsigned int fid;
    unsigned int portmap;
    unsigned int timeout;
    unsigned long endtime;
    unsigned int refCount;
};


#endif
