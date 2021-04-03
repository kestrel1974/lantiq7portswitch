#if !defined(__AVM_NET_SWI_IFX_COMMON_H)
#define __AVM_NET_SWI_IFX_COMMON_H

#include <avmnet_module.h>
#include <linux/list.h>
#include <linux/atmdev.h>
#include <linux/workqueue.h>
#include <common_routines.h>

#define SWI_DISABLE_LEARNING   ( 1 << 0 )
#define SWI_DISABLE_LANTIQ_PPA ( 1 << 1 )
#define SWI_DISABLE_PCE_PPA    ( 1 << 2 )

#if defined(CONFIG_VR9)
#include "../7port/swi_7port.h"
#include "../7port/swi_7port_reg.h"
#endif

#if defined(CONFIG_AR10)
#include "../7port/swi_7port.h"
#include "../7port/swi_7port_reg.h"
#endif

#if defined(CONFIG_AR9)
#include "../ar9/swi_ar9.h"
#include "../ar9/swi_ar9_reg.h"
#endif

// #define MAX_ETH_INF      	16
#define ETH_PKT_BUF_SIZE	1568

/*
 *  Config AVM Driver
 */

#define AVM_SETUP_MAC							1

#define INIT_HW                                 1

#define ENABLE_DBG_SKB_FREE						1

#if defined(CONFIG_AVMNET_DEBUG) 
#define DEBUG_DUMP_INIT                         0
#define DEBUG_DUMP_SKB                          1
#define DEBUG_DUMP_FLAG_HEADER                  1
#define DEBUG_DUMP_PMAC_HEADER                  1
#define ENABLE_ASSERT                           1
#define DEBUG_PP32_PROC                         1
#else
#define DEBUG_DUMP_INIT                         0
#define DEBUG_DUMP_SKB                          0
#define DEBUG_DUMP_FLAG_HEADER                  0
#define DEBUG_DUMP_PMAC_HEADER                  0
#define ENABLE_ASSERT                           0
#define DEBUG_PP32_PROC                         0
#endif
/*
 *  Debug Framework
 */
#if defined(CONFIG_AVMNET_DEBUG) 
  #define AVMNET_DEBUG_PRINT                    1
  #define DISABLE_INLINE                        1
#else
  #define AVMNET_DEBUG_PRINT                    0
  #define DISABLE_INLINE                        0
#endif

#if (defined(DEBUG_DUMP_SKB) && DEBUG_DUMP_SKB)                     \
    || (defined(DEBUG_DUMP_FLAG_HEADER) && DEBUG_DUMP_FLAG_HEADER)  \
    || (defined(DEBUG_DUMP_PMAC_HEADER) && DEBUG_DUMP_PMAC_HEADER)  \
    || (defined(DEBUG_DUMP_INIT) && DEBUG_DUMP_INIT)                \
    || (defined(AVMNET_DEBUG_PRINT) && AVMNET_DEBUG_PRINT)          \
    || (defined(ENABLE_ASSERT) && ENABLE_ASSERT)
  #define ENABLE_DBG_PROC                       1

#else
  #define ENABLE_DBG_PROC                       0
#endif

#if defined(ENABLE_DBG_SKB_FREE) && ENABLE_DBG_SKB_FREE
#define DBG_SKB_FREE(format, arg...)                     do { printk(KERN_ERR format "\n", ##arg); } while ( 0 )
#else
#define DBG_SKB_FREE(format, arg...)
#endif

#if defined(ENABLE_DBG_PROC) && ENABLE_DBG_PROC
#define err(format, arg...)                     do { if ( unlikely(g_dbg_datapath & DBG_ENABLE_MASK_ERR) ) printk(KERN_ERR __FILE__ ":%d:%s: " format "\n", __LINE__, __FUNCTION__, ##arg); } while ( 0 )
#else
#define err(format, arg...)                     printk(KERN_ERR __FILE__ ":%d:%s: " format "\n", __LINE__, __FUNCTION__, ##arg);
#endif

#if defined(AVMNET_DEBUG_PRINT) && AVMNET_DEBUG_PRINT
#undef  dbg_dma
#define dbg_dma(format, arg...)                 do { if ( unlikely(g_dbg_datapath & DBG_ENABLE_MASK_DEBUG_DMA) ) printk(KERN_ERR __FILE__ ":%d:%s: " format "\n", __LINE__, __FUNCTION__, ##arg); } while ( 0 )
#else
  #if !defined(dbg_dma)
    #define dbg_dma(format, arg...)
  #endif
#endif

#if defined(DEBUG_DUMP_INIT) && DEBUG_DUMP_INIT
#undef  dbg_ppe_init
#define dbg_ppe_init(format, arg...)                 do { if ( unlikely(g_dbg_datapath & DBG_ENABLE_MASK_DUMP_INIT) ) printk(KERN_ERR __FILE__ ":%d:%s: " format "\n", __LINE__, __FUNCTION__, ##arg); } while ( 0 )
#else
  #if !defined(dbg_ppe_init)
    #define dbg_ppe_init(format, arg...)
  #endif
#endif

#if defined(AVMNET_DEBUG_PRINT) && AVMNET_DEBUG_PRINT
  #undef  dbg
  #undef  dbg_trace_ppa_data
  #define dbg(format, arg...)                   do { if ( unlikely(g_dbg_datapath & DBG_ENABLE_MASK_DEBUG_PRINT) ) printk(KERN_WARNING __FILE__ ":%d:%s: " format "\n", __LINE__, __FUNCTION__, ##arg); } while ( 0 )
  #define dbg_trace_ppa_data(format, arg...)    do { if ( unlikely(g_dbg_datapath & DBG_ENABLE_MASK_DEBUG_TRACE_PPA_DATA) ) printk(KERN_WARNING __FILE__ ":%d:%s: " format "\n", __LINE__, __FUNCTION__, ##arg); } while ( 0 )
#else
  #if !defined(dbg)
    #define dbg(format, arg...)
  #endif
  #if !defined(dbg_trace_ppa_data)
    #define dbg_trace_ppa_data(format, arg...)
  #endif
#endif

#define DUBIOUS_ASSERT(cond, format, arg...) //TODO fix or understand DUBIOUS_ASSERTs

#if defined(ENABLE_ASSERT) && ENABLE_ASSERT
  #define ASSERT(cond, format, arg...)          do { if ( unlikely(g_dbg_datapath & DBG_ENABLE_MASK_ASSERT) && !(cond) ) printk(KERN_ERR __FILE__ ":%d:%s: " format "\n", __LINE__, __FUNCTION__, ##arg); } while ( 0 )
#else
  #define ASSERT(cond, format, arg...)
#endif

#if defined(DEBUG_DUMP_SKB) && DEBUG_DUMP_SKB
  #define DUMP_SKB_LEN                          ~0
#endif

/*
 *  Debug Print Mask
 */
#define DBG_ENABLE_MASK_ERR                     (1 << 0)
#define DBG_ENABLE_MASK_DEBUG_PRINT             (1 << 1)
#define DBG_ENABLE_MASK_ASSERT                  (1 << 2)
#define DBG_ENABLE_MASK_DUMP_ATM_TX             (1 << 3)
#define DBG_ENABLE_MASK_DUMP_ATM_RX             (1 << 4)
#define DBG_ENABLE_MASK_DUMP_SKB_RX             (1 << 5)
#define DBG_ENABLE_MASK_DUMP_SKB_TX             (1 << 6)
#define DBG_ENABLE_MASK_DUMP_FLAG_HEADER        (1 << 7)
#define DBG_ENABLE_MASK_DUMP_EG_HEADER          (1 << 8)
#define DBG_ENABLE_MASK_DUMP_INIT               (1 << 9)
#define DBG_ENABLE_MASK_MAC_SWAP                (1 << 10)
#define DBG_ENABLE_MASK_DUMP_PMAC_HEADER        (1 << 11)
#define DBG_ENABLE_MASK_DUMP_QOS                (1 << 12)
#define DBG_ENABLE_MASK_DEBUG_DMA               (1 << 13)
#define DBG_ENABLE_MASK_DEBUG_MAILBOX           (1 << 14)
#define DBG_ENABLE_MASK_DEBUG_ALIGNMENT         (1 << 15)
#define DBG_ENABLE_MASK_DEBUG_TRACE_PPA_DATA    (1 << 16)
#define DBG_ENABLE_MASK_DEBUG_VDEV              (1 << 17)
#define DBG_ENABLE_MASK_ALL                     (DBG_ENABLE_MASK_ERR | DBG_ENABLE_MASK_DEBUG_PRINT | DBG_ENABLE_MASK_ASSERT \
                                                | DBG_ENABLE_MASK_DUMP_SKB_RX | DBG_ENABLE_MASK_DUMP_SKB_TX                 \
                                                | DBG_ENABLE_MASK_DUMP_PMAC_HEADER | DBG_ENABLE_MASK_DUMP_FLAG_HEADER \
                                                | DBG_ENABLE_MASK_DUMP_EG_HEADER \
                                                | DBG_ENABLE_MASK_DUMP_INIT | DBG_ENABLE_MASK_MAC_SWAP | DBG_ENABLE_MASK_DUMP_QOS \
                                                | DBG_ENABLE_MASK_DEBUG_DMA | DBG_ENABLE_MASK_DUMP_ATM_TX | DBG_ENABLE_MASK_DUMP_ATM_RX \
                                                | DBG_ENABLE_MASK_DEBUG_MAILBOX |DBG_ENABLE_MASK_DEBUG_ALIGNMENT \
                                                | DBG_ENABLE_MASK_DEBUG_TRACE_PPA_DATA | DBG_ENABLE_MASK_DEBUG_VDEV )


#define AVM_WAN_MODE_NOT_CONFIGURED	( 1 << 0 )
#define AVM_WAN_MODE_ATM			( 1 << 1 )
#define AVM_WAN_MODE_PTM			( 1 << 2 )
#define AVM_WAN_MODE_ETH			( 1 << 3 )
#define AVM_WAN_MODE_KDEV			( 1 << 4 ) // kernel attached device (like WLAN, LTE, UTMS or other
                                               // usb-devices which need data beeing routed via the kernel)

#define QOS_OVERHD_BYTES	0
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
#if 0
struct proc_entry_cfg {
	char                    *parent_dir_name;
	char                    *name;
	unsigned int            is_dir;
	int			(*seq_read)(struct seq_file *, void *);
	struct file_operations  fops;
	int                     is_enabled;
	int                     is_permanent;
	struct proc_dir_entry   *proc_dir;
};

#define PEC_DISABLE 0
#define PEC_ENABLE 1

#define pec_dir(_name, _enable)		\
	{				\
		.name = _name,		\
		.is_dir = 1,		\
		.is_enabled = _enable,	\
	}

#define _pec_entry(_name, _seq_read, _read, _write, _enable)	\
	{							\
		.name = _name,					\
		.seq_read = _seq_read,				\
		.fops = {					\
			.read = _read,				\
			.write = _write,			\
		},						\
		.is_enabled = _enable,				\
	}

#define pec_entry(_name, _read, _write, _enable)		\
	_pec_entry(_name, NULL, _read, _write, _enable)

#define pec_seq_entry(_name, _seq_read, _write, _enable)	\
	_pec_entry(_name, _seq_read, NULL, _write, _enable)
#endif

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
#define MAX_PPA_PUSHTHROUGH_DEVICES 3

extern struct avm_pa_virt_rx_dev *ppa_virt_rx_devs[MAX_PPA_PUSHTHROUGH_DEVICES];
extern struct avm_pa_virt_tx_dev *ppa_virt_tx_devs[MAX_PPA_PUSHTHROUGH_DEVICES];

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

#ifdef CONFIG_AVM_PA
extern int ifx_ppa_try_to_accelerate( avm_pid_handle pid_handle, struct sk_buff *skb );

extern int ifx_ppa_alloc_virtual_rx_device( avm_pid_handle pid_handle );
extern int ifx_ppa_alloc_virtual_tx_device( avm_pid_handle pid_handle );
extern int ifx_ppa_free_virtual_rx_device( avm_pid_handle pid_handle );
extern int ifx_ppa_free_virtual_tx_device( avm_pid_handle pid_handle );

extern void ifx_ppa_show_virtual_devices(struct seq_file *seq);

#endif /* CONFIG_AVM_PA */


extern int (*ppa_pushthrough_hook)(uint32_t, struct sk_buff*, int32_t , uint32_t);
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

extern void ifx_proc_file_create(void);

/*
 * setup wan dev by name in AVM_PA flags, returns mac_wan_mask
 */
extern int avmnet_swi_ifx_common_set_ethwan_dev_by_name( const char *devname );

/*
 * return:
 *  0: avm_ata_device not set
 * >0: wan_dev_mask avm_ata_device is set; ata_devname is copied to *devname
 */
extern int avmnet_swi_ifx_common_get_ethwan_ata_devname( char *devname );
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
#define COMMON_BUFF_LEN 4096
extern char common_proc_read_buff[COMMON_BUFF_LEN];

extern int g_dbg_datapath;
extern volatile int g_datapath_mod_count;
extern int avm_wan_mode;

extern int in_avm_ata_mode(void);
extern int check_if_avmnet_enables_ppa(void);
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

#define DUMP_SKB_LEN ~0
#if defined(DEBUG_DUMP_SKB) && DEBUG_DUMP_SKB
	void dump_skb(struct sk_buff *skb, u32 len, const char *title, int port, int ch, int is_tx, int enforce);
#else
 #define dump_skb(a, b, c, d, e, f, g)
#endif

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

void init_avm_dev_lookup(void);
avmnet_device_t *mac_nr_to_avm_dev(int mac_nr);
int avm_dev_to_mac_nr(avmnet_device_t *avm_dev);
unsigned int speed_on_mac_nr( int mac_nr );

/*------------------------------------------------------------------------------------------*\
 * some static functions for all datapath drivers...
\*------------------------------------------------------------------------------------------*/

static inline int get_eth_port(struct net_device *dev) {
    avmnet_netdev_priv_t *priv = netdev_priv( dev );
	return priv->lantiq_device_index;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

static inline int mac_nr_to_if_id(int mac_nr){
    avmnet_device_t *avm_dev = mac_nr_to_avm_dev(mac_nr);
    return get_eth_port(avm_dev->device);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static inline int if_id_to_mac_nr( int if_id ) {
    avmnet_netdev_priv_t *priv = NULL;
    int mac_nr;
    avmnet_device_t *avm_dev = NULL;

    if (unlikely( if_id >= (int) NUM_ENTITY( g_eth_net_dev ) )) {
        err("if_id_to_mac_nr invalid if_id=%d\n", if_id);
        return 0;
    }

    priv = netdev_priv( g_eth_net_dev[if_id] );
    avm_dev = priv->avm_dev;
    mac_nr = avm_dev_to_mac_nr( avm_dev );
    // dbg("if_id_to_mac_nr(%d) mac_nr =%d\n", if_id, mac_nr);
    return mac_nr;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static inline void ifx_ignore_space(char **p, int *len) {
	while (*len && (**p <= ' ' || **p == ':' || **p == '.' || **p == ',')) {
		(*p)++;
		(*len)--;
	}
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static inline int ifx_stricmp(const char *p1, const char *p2) {
	int c1, c2;

	while (*p1 && *p2) {
		c1 = *p1 >= 'A' && *p1 <= 'Z' ? *p1 + 'a' - 'A' : *p1;
		c2 = *p2 >= 'A' && *p2 <= 'Z' ? *p2 + 'a' - 'A' : *p2;
		if ((c1 -= c2))
			return c1;
		p1++;
		p2++;
	}

	return *p1 - *p2;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static inline int ifx_strincmp(const char *p1, const char *p2, int n) {
	int c1 = 0, c2;

	while (n && *p1 && *p2) {
		c1 = *p1 >= 'A' && *p1 <= 'Z' ? *p1 + 'a' - 'A' : *p1;
		c2 = *p2 >= 'A' && *p2 <= 'Z' ? *p2 + 'a' - 'A' : *p2;
		if ((c1 -= c2))
			return c1;
		p1++;
		p2++;
		n--;
	}

	return n ? *p1 - *p2 : c1;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static inline int ifx_get_token(char **p1, char **p2, int *len, int *colon) {
	int tlen = 0;

	while (*len
			&& !((**p1 >= 'A' && **p1 <= 'Z') || (**p1 >= 'a' && **p1 <= 'z')
					|| (**p1 >= '0' && **p1 <= '9'))) {
		(*p1)++;
		(*len)--;
	}
	if (!*len)
		return 0;

	if (*colon) {
		*colon = 0;
		*p2 = *p1;
		while (*len && **p2 > ' ' && **p2 != ',') {
			if (**p2 == ':') {
				*colon = 1;
				break;
			}
			(*p2)++;
			(*len)--;
			tlen++;
		}
		**p2 = 0;
	} else {
		*p2 = *p1;
		while (*len && **p2 > ' ' && **p2 != ',') {
			(*p2)++;
			(*len)--;
			tlen++;
		}
		**p2 = 0;
	}

	return tlen;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static inline unsigned int ifx_get_number(char **p, int *len, int is_hex) {
	int ret = 0;
	int n = 0;

	if ((*p)[0] == '0' && (*p)[1] == 'x') {
		is_hex = 1;
		(*p) += 2;
		(*len) -= 2;
	}

	if (is_hex) {
		while (*len
				&& ((**p >= '0' && **p <= '9') || (**p >= 'a' && **p <= 'f')
						|| (**p >= 'A' && **p <= 'F'))) {
			if (**p >= '0' && **p <= '9')
				n = **p - '0';
			else if (**p >= 'a' && **p <= 'f')
				n = **p - 'a' + 10;
			else if (**p >= 'A' && **p <= 'F')
				n = **p - 'A' + 10;
			ret = (ret << 4) | n;
			(*p)++;
			(*len)--;
		}
	} else {
		while (*len && **p >= '0' && **p <= '9') {
			n = **p - '0';
			ret = ret * 10 + n;
			(*p)++;
			(*len)--;
		}
	}

	return ret;
}

#endif
