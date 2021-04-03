
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/netdevice.h>
#include <linux/vmalloc.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <avm/net/ifx_ppa_stack_al.h>
#include <avm/net/ifx_ppa_ppe_hal.h>
#include <avm/net/ifx_ppa_api.h>
#include <net/pkt_sched.h>

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

#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/if_ether.h>
#include <linux/if_vlan.h>
#include <linux/if_pppox.h>
#endif

#include <avmnet_config.h>

#include "swi_ifx_common.h"
#include "ifx_ppe_drv_wrapper.h"

#define PPE_SESSION_LIST_PROCFILE_NAME "ppe_sessions"
#define PPE_SESSIONS_BRIEF_PROCFILE_NAME "ppe_sessions_brief"
#define MAX_QID 7
#define LANTIQ_LAN_DEV 0
#define LANTIQ_WAN_DEV 1

/*------------------------------------------------------------------------------------------*\
 * Debug
\*------------------------------------------------------------------------------------------*/
#define SET_DBG_FLAG(item, flag)
#if defined DISABLE_SESSION_DBG
#define session_debug(dbg_domain, fmt, arg...)
#else
#define DBG_ENABLE_MASK_ERROR (1 << 0)
#define DBG_ENABLE_MASK_ADD_SESSION (1 << 1)
#define DBG_ENABLE_MASK_ADD_SESSION_TRACE (1 << 2)
#define DBG_ENABLE_MASK_DEL_SESSION (1 << 3)
#define DBG_ENABLE_MASK_STAT_LAN_SESSION (1 << 4)
#define DBG_ENABLE_MASK_STAT_WAN_SESSION (1 << 5)
#define DBG_ENABLE_MASK_ADD_HWSESSION (1 << 6)
#define DBG_ENABLE_MASK_DEL_HWSESSION (1 << 7)
#define DBG_ENABLE_MASK_DUMP_FLAGS (1 << 8)
#define DBG_ENABLE_MASK_VDEV_SESSION (1 << 9)
#define DBG_ENABLE_MASK_DUMP_ROUTING_SESSION (1 << 10)
#define DBG_ENABLE_MASK_DBG_PROC (1 << 11)

static int dbg_mode = 0;
#define session_debug(dbg_domain, fmt, arg...)                                 \
	{                                                                      \
		if (unlikely(dbg_mode & dbg_domain))                           \
			printk(KERN_ERR "[%s]" fmt, __func__, ##arg);          \
	}
#define session_error(fmt, arg...)                                             \
	{                                                                      \
		if (unlikely(dbg_mode & DBG_ENABLE_MASK_ERROR))                \
			printk(KERN_ERR "[%s]" fmt "\n", __func__, ##arg);     \
	}
module_param(dbg_mode, int, S_IRUSR | S_IWUSR);

#endif

/*------------------------------------------------------------------------------------------*\
 * Local Datatypes
\*------------------------------------------------------------------------------------------*/
struct session_list_item {
	uint16_t session_id;
	uint16_t ip_proto;
	uint16_t ip_tos;
	PPA_IPADDR src_ip;
	uint16_t src_port;
	uint8_t src_mac[PPA_ETH_ALEN];
	PPA_IPADDR dst_ip;
	uint16_t dst_port;
	uint8_t dst_mac[PPA_ETH_ALEN];
	uint8_t new_src_mac[PPA_ETH_ALEN]; // eingefuegt von avm
	PPA_IPADDR nat_ip; //  IP address to be replaced by NAT if NAT applies
	uint16_t nat_port; //  Port to be replaced by NAT if NAT applies
	uint16_t pktlen;
	uint16_t num_adds; //  Number of attempts to add session
	uint32_t last_hit_time; //  Updated by bookkeeping thread
	uint32_t hit_count; //  Updated by bookkeeping thread
	uint32_t new_dscp;
	uint16_t pppoe_session_id;
	uint16_t new_vci;
	uint32_t out_vlan_tag;
	uint32_t mtu;
	uint16_t dslwan_qid;
	uint16_t dest_ifid;
	uint16_t dest_port_map; // 6 bit Port-map Relevant fuer WAN->LAN Sessions (wird vom PPA in sw_eg_pkt_header->port_map eingetragen)
	uint32_t flags; //  Internal flag : SESSION_IS_REPLY, SESSION_IS_TCP,
		//                  SESSION_ADDED_IN_HW, SESSION_CAN_NOT_ACCEL
		//                  SESSION_VALID_NAT_IP, SESSION_VALID_NAT_PORT,
		//                  SESSION_VALID_VLAN_INS, SESSION_VALID_VLAN_RM,
		//                  SESSION_VALID_OUT_VLAN_INS, SESSION_VALID_OUT_VLAN_RM,
		//                  SESSION_VALID_PPPOE, SESSION_VALID_NEW_SRC_MAC,
		//                  SESSION_VALID_MTU, SESSION_VALID_NEW_DSCP,
		//                  SESSION_VALID_DSLWAN_QID,
		//                  SESSION_TX_ITF_IPOA, SESSION_TX_ITF_PPPOA
		//                  SESSION_LAN_ENTRY, SESSION_WAN_ENTRY, SESSION_IS_IPV6
	uint32_t routing_entry;
	uint32_t pppoe_entry;
	uint32_t mtu_entry;
#define NO_MAC_ENTRY ~0U
	uint32_t src_mac_entry;
	uint32_t out_vlan_entry;
#if defined(SKB_PRIORITY_DEBUG) && SKB_PRIORITY_DEBUG
	uint32_t priority; //skb priority. for debugging only
#endif
	uint64_t acc_bytes; //bytes handled by PPE acceleration
	uint32_t last_mib_count; //last updated bytes handled by PPE acceleration
	uint32_t last_mc_packets;
	uint32_t tunnel_idx; //tunnel idx for PPE action table
	PPA_IPADDR tun_saddr;
	PPA_IPADDR tun_daddr;
	uint8_t collision_flag; // 1 mean the entry is in collsion table or no hashed table, like ASE/Danube
};

/*------------------------------------------------------------------------------------------*\
 * Local Functions
\*------------------------------------------------------------------------------------------*/
static int32_t ppa_hw_add_session(struct session_list_item *p_item);
static int32_t ppa_hw_add_mc_session(struct session_list_item *p_item);
static void ppa_hw_del_session(struct session_list_item *p_item);
static void ppa_hw_del_mc_session(struct session_list_item *p_item);
static int read_session_list_open(struct inode *inode, struct file *file);
static int read_sessions_brief_open(struct inode *inode, struct file *file);
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

static const struct file_operations read_session_list_fops = {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 32)
	.owner = THIS_MODULE,
#endif
	.open = read_session_list_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

static const struct file_operations read_sessions_brief_fops = {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 32)
	.owner = THIS_MODULE,
#endif
	.open = read_sessions_brief_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

/*------------------------------------------------------------------------------------------*\
 * Session List
\*------------------------------------------------------------------------------------------*/

static struct session_list_item session_list[CONFIG_AVM_PA_MAX_SESSION];
static atomic_t session_valid[CONFIG_AVM_PA_MAX_SESSION];
static atomic_t session_active[CONFIG_AVM_PA_MAX_SESSION];

/*------------------------------------------------------------------------------------------*\
 * Lookup Tables for VDEV Traffic
\*------------------------------------------------------------------------------------------*/
#define RW_LOCK_DECLARE unsigned long flags

static DEFINE_RWLOCK(lan_table_lock);
static DEFINE_RWLOCK(wan_table_lock);
static DEFINE_RWLOCK(mc_table_lock);

#define LAN_TABLE_WRITE_LOCK() write_lock_irqsave(&lan_table_lock, flags)
#define WAN_TABLE_WRITE_LOCK() write_lock_irqsave(&wan_table_lock, flags)
#define MC_TABLE_WRITE_LOCK() write_lock_irqsave(&mc_table_lock, flags)

#define LAN_TABLE_WRITE_UNLOCK() write_unlock_irqrestore(&lan_table_lock, flags)
#define WAN_TABLE_WRITE_UNLOCK() write_unlock_irqrestore(&wan_table_lock, flags)
#define MC_TABLE_WRITE_UNLOCK() write_unlock_irqrestore(&mc_table_lock, flags)

#define LAN_TABLE_READ_LOCK() read_lock_irqsave(&lan_table_lock, flags)
#define WAN_TABLE_READ_LOCK() read_lock_irqsave(&wan_table_lock, flags)
#define MC_TABLE_READ_LOCK() read_lock_irqsave(&mc_table_lock, flags)

#define LAN_TABLE_READ_UNLOCK() read_unlock_irqrestore(&lan_table_lock, flags)
#define WAN_TABLE_READ_UNLOCK() read_unlock_irqrestore(&wan_table_lock, flags)
#define MC_TABLE_READ_UNLOCK() read_unlock_irqrestore(&mc_table_lock, flags)

static avm_session_handle *avm_sessionh_table_lan = NULL;
unsigned int avm_sessionh_table_lan_size = 0;

static avm_session_handle *avm_sessionh_table_wan = NULL;
unsigned int avm_sessionh_table_wan_size = 0;

static avm_session_handle *avm_sessionh_table_mc = NULL;
unsigned int avm_sessionh_table_mc_size = 0;

/*------------------------------------------------------------------------------------------*\
 * setup use_avm_dest_port_map-Parameter: kann gesetzt werden in:
 *  - /sys/module/ifx_ppa_mini_session/parameters/dest_port_map u.
 *  - kernel_cmd_line: ifx_ppa_api.dest_port_map
\*------------------------------------------------------------------------------------------*/

#if defined(CONFIG_AR9)
int use_avm_dest_port_map = 0;
#else
int use_avm_dest_port_map = 1;
#endif
module_param(use_avm_dest_port_map, int, S_IRUSR | S_IWUSR);

static int disable_lan_sessions = 0;
module_param(disable_lan_sessions, int, S_IRUSR | S_IWUSR);

static int disable_wan_sessions = 0;
module_param(disable_wan_sessions, int, S_IRUSR | S_IWUSR);

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

struct session_list_item *
	insert_session_item(struct session_list_item *new_item)
{
	uint16_t session_pos = new_item->session_id;
	struct session_list_item *res;

	BUG_ON(session_pos >= CONFIG_AVM_PA_MAX_SESSION);

	if (atomic_add_return(1, &session_valid[session_pos]) == 1) {
		res = memcpy(&session_list[session_pos], new_item,
			     sizeof(struct session_list_item));
		atomic_set(&session_active[session_pos], 1);
	} else {
		// we won't replace a valid entry here!
		printk(KERN_ERR "we won't replace a valid entry here\n");
		return NULL;
	}
	return res;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void invalidate_session_item(struct session_list_item *new_item)
{
	uint16_t session_pos = new_item->session_id;

	BUG_ON(session_pos >= CONFIG_AVM_PA_MAX_SESSION);

	atomic_set(&session_active[session_pos], 0);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

void remove_session_item(struct session_list_item *new_item)
{
	uint16_t session_pos = new_item->session_id;

	BUG_ON(session_pos >= CONFIG_AVM_PA_MAX_SESSION);

	memset(&session_list[session_pos], 0, sizeof(struct session_list_item));
	atomic_set(&session_valid[session_pos], 0);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static inline void avm_sessionh_table_remove(unsigned int rout_table_entry,
					     unsigned int is_lan,
					     unsigned int is_mc)
{
	unsigned int index = rout_table_entry & 0x7FFFFFFF;
	RW_LOCK_DECLARE;

	if (is_mc) {
		if (likely(index < avm_sessionh_table_mc_size)) {
			MC_TABLE_WRITE_LOCK();
			avm_sessionh_table_mc[index] = INVALID_SESSION;
			MC_TABLE_WRITE_UNLOCK();
		}
	} else if (is_lan) {
		if (likely(index < avm_sessionh_table_lan_size)) {
			LAN_TABLE_WRITE_LOCK();
			avm_sessionh_table_lan[index] = INVALID_SESSION;
			LAN_TABLE_WRITE_UNLOCK();
		}
	} else {
		if (likely(index < avm_sessionh_table_wan_size)) {
			WAN_TABLE_WRITE_LOCK();
			avm_sessionh_table_wan[index] = INVALID_SESSION;
			WAN_TABLE_WRITE_UNLOCK();
		}
	}
	return;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

static inline void avm_sessionh_table_set(unsigned int rout_table_entry,
					  avm_session_handle sessionh,
					  unsigned int is_lan,
					  unsigned int is_mc)
{
	unsigned int index = rout_table_entry & 0x7FFFFFFF;
	RW_LOCK_DECLARE;

	if (is_mc) {
		if (likely(index < avm_sessionh_table_mc_size)) {
			MC_TABLE_WRITE_LOCK();
			avm_sessionh_table_mc[index] = sessionh;
			MC_TABLE_WRITE_UNLOCK();

		} else {
			printk(KERN_ERR
			       "no space for index %d in mc session table\n",
			       rout_table_entry);
		}
		return;
	}

	if (is_lan) {
		if (likely(index < avm_sessionh_table_lan_size)) {
			LAN_TABLE_WRITE_LOCK();
			avm_sessionh_table_lan[index] = sessionh;
			LAN_TABLE_WRITE_UNLOCK();

		} else {
			printk(KERN_ERR
			       "no space for index %d in lan session table\n",
			       rout_table_entry);
		}
	} else {
		if (likely(index < avm_sessionh_table_wan_size)) {
			WAN_TABLE_WRITE_LOCK();
			avm_sessionh_table_wan[index] = sessionh;
			WAN_TABLE_WRITE_UNLOCK();

		} else {
			printk(KERN_ERR
			       "no space for index %d in wan session table\n",
			       rout_table_entry);
		}
	}
	return;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

avm_session_handle avm_session_handle_fast_lookup(unsigned int rout_table_entry,
						  unsigned int is_lan,
						  unsigned int is_mc)
{
	unsigned int index = rout_table_entry & 0x7FFFFFFF;
	RW_LOCK_DECLARE;

	avm_session_handle res = INVALID_SESSION;
	if (is_mc) {
		if (likely(index < avm_sessionh_table_mc_size)) {
			MC_TABLE_READ_LOCK();
			res = avm_sessionh_table_mc[index];
			MC_TABLE_READ_UNLOCK();
#if defined(CONFIG_DEBUG_AVM_PPA_VDEV)
			avm_session_hit_count_mc_table[index] += 1;
#endif
		}
	} else if (is_lan) {
		if (likely(index < avm_sessionh_table_lan_size)) {
			LAN_TABLE_READ_LOCK();
			res = avm_sessionh_table_lan[index];
			LAN_TABLE_READ_UNLOCK();
#if defined(CONFIG_DEBUG_AVM_PPA_VDEV)
			avm_session_hit_count_lan_table[index] += 1;
#endif
		}
	} else {
		if (likely(index < avm_sessionh_table_wan_size)) {
			WAN_TABLE_READ_LOCK();
			res = avm_sessionh_table_wan[index];
			WAN_TABLE_READ_UNLOCK();
#if defined(CONFIG_DEBUG_AVM_PPA_VDEV)
			avm_session_hit_count_wan_table[index] += 1;
#endif
		}
	}
//todo implement slow lookup...
#if 0
SLOW_LOOKUP:
    if (res == INVALID_SESSION )
        return avm_session_handle_slow_lookup(rout_table_entry, is_lan, is_mc);
    else
#endif
	return res;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

static int avm_pa_ifx_session_stats(struct avm_pa_session *session,
				    struct avm_pa_session_stats *stats)
{
	struct session_list_item *p_item;
	uint32_t stat_debug_flag = 0;
	PPE_ROUTING_INFO route = { 0 };
	PPE_MC_INFO mc = { 0 };

	stats->validflags = 0;

	if (!session || (session->session_handle >= CONFIG_AVM_PA_MAX_SESSION))
		return -1;

	if (!atomic_read(&session_active[session->session_handle]))
		return -1;

	p_item = &session_list[session->session_handle];

	if (p_item->flags & SESSION_LAN_ENTRY)
		stat_debug_flag = DBG_ENABLE_MASK_STAT_LAN_SESSION;
	if (p_item->flags & SESSION_WAN_ENTRY)
		stat_debug_flag = DBG_ENABLE_MASK_STAT_WAN_SESSION;

	if (p_item->flags & SESSION_MC_ENTRY) {
		mc.p_entry = p_item->routing_entry;
		ifx_ppa_drv_test_and_clear_mc_hit_stat(&mc, 0);
		if (mc.f_hit) {
			stats->validflags |= AVM_PA_SESSION_STATS_VALID_HIT;
			session_debug(DBG_ENABLE_MASK_VDEV_SESSION,
				      "%lu: hit routing_entry=%d (MULTICAST)\n",
				      jiffies,
				      p_item->routing_entry & 0x7FFFFFFF);
			p_item->last_hit_time = ppa_get_time_in_sec();
			p_item->hit_count += 1;

#if defined(RTP_SAMPLING_ENABLE) && RTP_SAMPLING_ENABLE
			mc.sample_en = 1;
			ifx_ppa_drv_get_mc_rtp_sampling_cnt(&mc);

			if (mc.rtp_pkt_cnt > p_item->last_mc_packets) {
				stats->validflags |=
					AVM_PA_SESSION_STATS_VALID_PKTS;
				stats->tx_pkts = mc.rtp_pkt_cnt -
						 p_item->last_mc_packets;
			} else {
				stats->tx_pkts = 0;
			}
#endif
			if (stats->validflags &
			    AVM_PA_SESSION_STATS_VALID_PKTS) {
				stats->validflags |=
					AVM_PA_SESSION_STATS_VALID_BYTES;
				stats->tx_bytes =
					stats->tx_pkts * p_item->pktlen;
			} else {
				stats->tx_bytes = 0;
			}

			p_item->last_mc_packets = mc.rtp_pkt_cnt;
			p_item->acc_bytes += stats->tx_bytes;
		}
	} else {
		route.entry = p_item->routing_entry;
		ifx_ppa_drv_test_and_clear_hit_stat(&route, 0);
		if (route.f_hit) {
			stats->validflags |= AVM_PA_SESSION_STATS_VALID_HIT;
			session_debug(DBG_ENABLE_MASK_VDEV_SESSION,
				      "%lu: hit routing_entry=%d (%s)\n",
				      jiffies,
				      p_item->routing_entry & 0x7FFFFFFF,
				      (p_item->flags & SESSION_LAN_ENTRY) ?
					      "LAN" :
					      "WAN");
			session_debug(stat_debug_flag, "session %d was hit\n",
				      p_item->session_id);
			p_item->last_hit_time = ppa_get_time_in_sec();
			p_item->hit_count += 1;

			ifx_ppa_drv_get_routing_entry_bytes(&route, 0);

			stats->validflags |= AVM_PA_SESSION_STATS_VALID_BYTES;
			if (route.mib_count >= p_item->last_mib_count) {
				stats->tx_bytes =
					(uint64_t)(route.mib_count -
						   p_item->last_mib_count) *
					(uint64_t)route.mib_factor;
			} else {
				session_debug(stat_debug_flag, "doing WRAP\n");
				stats->tx_bytes =
					((uint64_t)route.mib_count +
					 (uint64_t)route.mib_max -
					 (uint64_t)p_item->last_mib_count) *
					(uint64_t)route.mib_factor;
			}
			session_debug(
				stat_debug_flag,
				"last_mib_count=%#08x, mib_count=%#08x, bytes_since_last_report=%010llu, session_byte_count=%010llu\n",
				p_item->last_mib_count, route.mib_count,
				stats->tx_bytes,
				p_item->acc_bytes + stats->tx_bytes);

			p_item->last_mib_count = route.mib_count;
			p_item->acc_bytes += stats->tx_bytes;
			session_debug(stat_debug_flag,
				      "report_bytes=%010llu,\n",
				      stats->tx_bytes);
		} else if (p_item->session_id) {
			session_debug(stat_debug_flag,
				      "report empty session %d\n",
				      session->session_handle);
		}
	}
	return 0;
}
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

inline void update_atm_flag(struct avm_pa_pid_hwinfo *info)
{
	if (info->atmvcc)
		info->flags |= AVMNET_DEVICE_IFXPPA_ATM_WAN;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void dump_list_item(struct session_list_item *p_item, char *comment)
{
	int8_t strbuf[64];
	if (!(dbg_mode & DBG_ENABLE_MASK_DUMP_ROUTING_SESSION))
		return;

	if (comment)
		printk("dump_list_item - %s\n", comment);
	else
		printk("dump_list_item\n");
	printk("  session          = %d\n", (uint32_t)p_item->session_id);
	printk("  ip_proto         = %08X\n", p_item->ip_proto);
	printk("  src_ip           = %s\n",
	       ppa_get_pkt_ip_string(p_item->src_ip,
				     p_item->flags & SESSION_IS_IPV6, strbuf));
	printk("  src_port         = %d\n", p_item->src_port);
	printk("  src_mac[6]       = %s\n",
	       ppa_get_pkt_mac_string(p_item->src_mac, strbuf));
	printk("  dst_ip           = %s\n",
	       ppa_get_pkt_ip_string(p_item->dst_ip,
				     p_item->flags & SESSION_IS_IPV6, strbuf));
	printk("  dst_port         = %d\n", p_item->dst_port);
	printk("  dst_mac[6]       = %s\n",
	       ppa_get_pkt_mac_string(p_item->dst_mac, strbuf));
	printk("  nat_ip           = %s\n",
	       ppa_get_pkt_ip_string(p_item->nat_ip,
				     p_item->flags & SESSION_IS_IPV6, strbuf));
	printk("  nat_port         = %d\n", p_item->nat_port);
	printk("  new_src_mac[6]   = %s\n",
	       ppa_get_pkt_mac_string(p_item->new_src_mac, strbuf));
	printk("  last_hit_time    = %d\n", p_item->last_hit_time);
	printk("  hit_count        = %d\n", p_item->hit_count);
	printk("  num_adds         = %d\n", p_item->num_adds);
	printk("  pppoe_session_id = %d\n", p_item->pppoe_session_id);
	printk("  new_dscp         = %d\n", p_item->new_dscp);
	printk("  new_vci          = %08X\n", p_item->new_vci);
	printk("  mtu              = %d\n", p_item->mtu);
	printk("  flags            = %08X\n", p_item->flags);
	printk("  routing_entry    = %08X\n", p_item->routing_entry);
	printk("  pppoe_entry      = %08X\n", p_item->pppoe_entry);
	printk("  mtu_entry        = %08X\n", p_item->mtu_entry);
	printk("  src_mac_entry    = %08X\n", p_item->src_mac_entry);
	printk("  wan_qid (QoS)    = %08d\n", p_item->dslwan_qid);
	printk("  acc_bytes        = %llu\n", p_item->acc_bytes);
}
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

static int print_session_flags(struct seq_file *seq, char *str, uint32_t flags)
{
	SESSION_FLAGS_ARRAY;

	int seq_res = 0;
	int flag;
	unsigned long bit;
	int i;

	seq_res += seq_printf(seq, str);

	flag = 0;
	for (bit = 1, i = 0; bit; bit <<= 1, i++)
		if ((flags & bit)) {
			if (flag++)
				seq_res += seq_printf(seq, ", ");
			seq_res += seq_printf(seq, session_flags_array[i]);
		}
	if (flag)
		seq_res += seq_printf(seq, "\n");
	else
		seq_res += seq_printf(seq, "NULL\n");

	return seq_res;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static int read_session_list_seq_show(struct seq_file *seq,
				      void *data __attribute__((unused)))
{
	int seq_res = 0;
	int8_t strbuf[64];
	struct session_list_item *p_item_start = &session_list[0];
	struct session_list_item *p_item = (struct session_list_item *)data;
	struct session_list_item item;
	size_t session_pos = p_item - p_item_start;

	session_debug(DBG_ENABLE_MASK_DBG_PROC, "check_no %zu", session_pos);

	session_debug(DBG_ENABLE_MASK_DBG_PROC, " ... got lock, entry is %s\n",
		      atomic_read(&session_active[session_pos]) ? "valid" :
								  "invalid");

	if (!atomic_read(&session_active[session_pos]))
		return 0;

	memcpy(&item, p_item, sizeof(struct session_list_item));

	seq_res += seq_printf(
		seq, "===================               ============\n");
	seq_res += seq_printf(seq, "  session                        = %d\n",
			      (uint32_t)item.session_id);
	seq_res += seq_printf(seq, "  ip_proto                       = %08X\n",
			      item.ip_proto);
	seq_res += seq_printf(
		seq, "  src_ip                         = %s\n",
		ppa_get_pkt_ip_string(item.src_ip, item.flags & SESSION_IS_IPV6,
				      strbuf));
	seq_res += seq_printf(seq, "  src_port                       = %d\n",
			      item.src_port);
	seq_res += seq_printf(seq, "  src_mac[6]                     = %s\n",
			      ppa_get_pkt_mac_string(item.src_mac, strbuf));
	seq_res += seq_printf(seq, "  dst_ifid                       = %hu\n",
			      item.dest_ifid);
	seq_res += seq_printf(seq, "  dst_port_map                   = %#hx\n",
			      item.dest_port_map);
	seq_res += seq_printf(
		seq, "  dst_ip                         = %s\n",
		ppa_get_pkt_ip_string(item.dst_ip, item.flags & SESSION_IS_IPV6,
				      strbuf));
	seq_res += seq_printf(seq, "  dst_port                       = %d\n",
			      item.dst_port);
	seq_res += seq_printf(seq, "  dst_mac[6]                     = %s\n",
			      ppa_get_pkt_mac_string(item.dst_mac, strbuf));
	seq_res += seq_printf(
		seq, "  nat_ip                         = %s\n",
		ppa_get_pkt_ip_string(item.nat_ip, item.flags & SESSION_IS_IPV6,
				      strbuf));
	seq_res += seq_printf(seq, "  nat_port                       = %d\n",
			      item.nat_port);
	seq_res += seq_printf(seq, "  new_src_mac[6]                 = %s\n",
			      ppa_get_pkt_mac_string(item.new_src_mac, strbuf));
	seq_res += seq_printf(seq, "  pkt_len                        = %u\n",
			      item.pktlen);
	seq_res += seq_printf(seq, "  last_hit_time                  = %d\n",
			      item.last_hit_time);
	seq_res += seq_printf(seq, "  hit_count                      = %d\n",
			      item.hit_count);
	seq_res += seq_printf(seq, "  num_adds                       = %d\n",
			      item.num_adds);
	seq_res += seq_printf(seq, "  pppoe_session_id               = %d\n",
			      item.pppoe_session_id);
	seq_res += seq_printf(seq, "  new_dscp                       = %d\n",
			      item.new_dscp);
	seq_res += seq_printf(seq, "  new_vci                        = %08X\n",
			      item.new_vci);
	seq_res += seq_printf(seq, "  mtu                            = %d\n",
			      item.mtu);
	seq_res += seq_printf(seq, "  flags                          =");
	seq_res += print_session_flags(seq, " ", item.flags);
	seq_res += seq_printf(seq, "  routing_entry                  = %08X\n",
			      item.routing_entry);
	seq_res += seq_printf(seq, "  pppoe_entry                    = %08X\n",
			      item.pppoe_entry);
	seq_res += seq_printf(seq, "  mtu_entry                      = %08X\n",
			      item.mtu_entry);
	seq_res += seq_printf(seq, "  src_mac_entry                  = %08X\n",
			      item.src_mac_entry);
	seq_res += seq_printf(seq, "  wan_qid (QoS)                  = %08d\n",
			      item.dslwan_qid);
	seq_res += seq_printf(seq, "  acc_bytes                      = %llu\n",
			      item.acc_bytes);

	session_debug(DBG_ENABLE_MASK_DBG_PROC, "seq_res =%d\n", seq_res);

	return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

static void *read_session_list_seq_start(struct seq_file *s
					 __attribute__((unused)),
					 loff_t *pos)
{
	unsigned int session_list_pos = *pos;

	session_debug(DBG_ENABLE_MASK_DBG_PROC,
		      "file_pos = %lld session_list_pos = %u\n", *pos,
		      session_list_pos);

	if (session_list_pos < CONFIG_AVM_PA_MAX_SESSION) {
		return &session_list[session_list_pos];
	} else {
		*pos = 0;
		return NULL;
	}
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

static void *read_session_list_seq_next(struct seq_file *s
					__attribute__((unused)),
					void *v, loff_t *pos)
{
	struct session_list_item *p_item = v;

	if (p_item == &session_list[CONFIG_AVM_PA_MAX_SESSION - 1]) {
		session_debug(DBG_ENABLE_MASK_DBG_PROC,
			      " we reached the end -> return NULL \n");
		return NULL;
	} else {
		(*pos)++;
		p_item += 1;
		session_debug(DBG_ENABLE_MASK_DBG_PROC,
			      "return session_list_pos=%d\n",
			      (p_item - &session_list[0]));
		return p_item;
	}
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

static void read_session_list_seq_stop(struct seq_file *s
				       __attribute__((unused)),
				       void *v)
{
	session_debug(DBG_ENABLE_MASK_DBG_PROC,
		      "session_list_pos=%p , session_list_ptr=%p \n", v,
		      &session_list[0]);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static struct seq_operations read_session_list_seq_ops = {
	.start = read_session_list_seq_start,
	.next = read_session_list_seq_next,
	.stop = read_session_list_seq_stop,
	.show = read_session_list_seq_show
};

static int read_session_list_open(struct inode *inode __attribute__((unused)),
				  struct file *file)
{
#if 0
    avmnet_module_t *this;
    this = (avmnet_module_t *) PDE(inode)->data;
#endif

	return seq_open(file, &read_session_list_seq_ops);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

static int read_sessions_brief_show(struct seq_file *seq,
				    void *data __attribute__((unused)))
{
	uint32_t i;

	for (i = 0; i < CONFIG_AVM_PA_MAX_SESSION; i++) {
		if (atomic_read(&session_active[i]))
			seq_printf(seq, "%03d: %llu bytes\n", i,
				   session_list[i].acc_bytes);
	}
	return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static int read_sessions_brief_open(struct inode *inode __attribute__((unused)),
				    struct file *file)
{
	return single_open(file, read_sessions_brief_show, NULL);
}
/*------------------------------------------------------------------------------------------*\
 * ADD_SESSION_STATES:
\*------------------------------------------------------------------------------------------*/
#define AASS_INGRESS_IPV4 (1 << 0)
#define AASS_INGRESS_IPV6 (1 << 1)
#define AASS_INGRESS_IPV6V4 (1 << 2)
#define AASS_INGRESS_IPV4V6 (1 << 3)
#define AASS_INGRESS_PPPOE (1 << 4)
#define AASS_INGRESS_PPPOA (1 << 5)
#define AASS_INGRESS_IPOA (1 << 6)
#define AASS_INGRESS_ETHERNET (1 << 7)
#define AASS_EGRESS_IPV4 (1 << 8)
#define AASS_EGRESS_IPV6 (1 << 9)
#define AASS_EGRESS_IPV6V4 (1 << 10)
#define AASS_EGRESS_IPV4V6 (1 << 11)
#define AASS_EGRESS_PPPOE (1 << 12)
#define AASS_EGRESS_PPPOA (1 << 13)
#define AASS_EGRESS_IPOA (1 << 14)
#define AASS_EGRESS_ETHERNET (1 << 15)

#define MAX_SUPPORTED_VLAN_TAGS 2
#define MAX_IP_HDR 2

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

void print_flags(unsigned int flags)
{
	if (!flags || !(dbg_mode & DBG_ENABLE_MASK_ADD_SESSION))
		return;

	printk(KERN_ERR "flags: ");
	if (flags & AASS_INGRESS_IPV4)
		printk(" INGRESS_IPV4 ");
	if (flags & AASS_INGRESS_IPV6)
		printk(" INGRESS_IPV6 ");
	if (flags & AASS_INGRESS_IPV6V4)
		printk(" INGRESS_IPV6V4 ");
	if (flags & AASS_INGRESS_IPV4V6)
		printk(" INGRESS_IPV4V6 ");
	if (flags & AASS_INGRESS_PPPOE)
		printk(" INGRESS_PPPOE ");
	if (flags & AASS_INGRESS_PPPOA)
		printk(" INGRESS_PPPOA ");
	if (flags & AASS_INGRESS_IPOA)
		printk(" INGRESS_IPOA ");
	if (flags & AASS_INGRESS_ETHERNET)
		printk(" INGRESS_ETHERNET ");
	if (flags & AASS_EGRESS_IPV4)
		printk(" EGRESS_IPV4 ");
	if (flags & AASS_EGRESS_IPV6)
		printk(" EGRESS_IPV6 ");
	if (flags & AASS_EGRESS_IPV6V4)
		printk(" EGRESS_IPV6V4 ");
	if (flags & AASS_EGRESS_IPV4V6)
		printk(" EGRESS_IPV4V6 ");
	if (flags & AASS_EGRESS_PPPOE)
		printk(" EGRESS_PPPOE ");
	if (flags & AASS_EGRESS_PPPOA)
		printk(" EGRESS_PPPOA ");
	if (flags & AASS_EGRESS_IPOA)
		printk(" EGRESS_IPOA ");
	if (flags & AASS_EGRESS_ETHERNET)
		printk(" EGRESS_ETHERNET ");
	printk(KERN_ERR "\n ");
}

/*------------------------------------------------------------------------------------------*\
 *
\*------------------------------------------------------------------------------------------*/
int avm_pa_add_ifx_session(struct avm_pa_session *avm_session)
{
	int aass = 0; //AVM_ADD_SESSION_STATE
	uint16_t outgoing_vlan_tci[MAX_SUPPORTED_VLAN_TAGS];

	uint16_t nr_vlan_out = 0;
	uint16_t nr_vlan_in = 0;
	struct session_list_item item = { 0 };
	struct session_list_item *p_item = &item;
	int hw_ret;
	int n;
	uint16_t session_id;
	struct avm_pa_pid_hwinfo *ingress_hw;
	struct avm_pa_pid_hwinfo *egress_hw;
	struct avm_pa_egress *avm_egress = avm_pa_first_egress(avm_session);
	unsigned int priority;
	size_t in_ip_cnt = 0; /* ingress ip header count */
	size_t eg_ip_cnt = 0; /* egress ip header count */
	u32 *in_ip_saddr_ptr[MAX_IP_HDR];
	u32 *in_ip_daddr_ptr[MAX_IP_HDR];
	u32 *eg_ip_saddr_ptr[MAX_IP_HDR];
	u32 *eg_ip_daddr_ptr[MAX_IP_HDR];

	session_debug(DBG_ENABLE_MASK_ADD_SESSION, "\n------------------\n");
	session_debug(DBG_ENABLE_MASK_ADD_SESSION, "avm_pa->handle %u\n",
		      (u32)avm_session->session_handle);

	/* constraints */
	if (!avm_session) {
		session_debug(DBG_ENABLE_MASK_ADD_SESSION,
			      "error: no avm_pa_session\n");
		goto ERR;
	}

	session_id = avm_session->session_handle;

	if (!session_id) {
		session_debug(DBG_ENABLE_MASK_ADD_SESSION,
			      "error: no session_id\n");
		goto ERR;
	}

	if (session_id >= CONFIG_AVM_PA_MAX_SESSION) {
		session_debug(DBG_ENABLE_MASK_ADD_SESSION,
			      "error: invalid session_id %d\n", session_id);
		goto ERR;
	}

	if (avm_session->bsession != NULL) {
		session_debug(DBG_ENABLE_MASK_ADD_SESSION,
			      "Refusing bridging session in IFX PA\n");
		goto ERR;
	}

	if (avm_session->negress != 1) {
		session_debug(DBG_ENABLE_MASK_ADD_SESSION,
			      "multiple egress not supported\n");
		goto ERR;
	}

	p_item->session_id = session_id;
#ifdef AVM_PA_MATCH_HAS_PKTLEN
	p_item->pktlen = avm_session->ingress.pktlen;
#else
	p_item->pktlen = 0;
#endif

	if (avm_session->ingress.casttype == AVM_PA_IS_MULTICAST)
		p_item->flags |= SESSION_MC_ENTRY;

	switch (avm_session->ingress.pkttype & AVM_PA_PKTTYPE_PROTO_MASK) {
	case IPPROTO_TCP:
		p_item->flags |= SESSION_IS_TCP;
		break;
	case IPPROTO_UDP:
		p_item->flags &= ~SESSION_IS_TCP;
		break;
	default:
		session_debug(DBG_ENABLE_MASK_ADD_SESSION,
			      "no support for PKTTYPE != ( UDP | TCP )\n");
		goto ERR_FREE_PITEM;
	}

	ingress_hw = avm_pa_pid_get_hwinfo(avm_session->ingress_pid_handle);
	egress_hw = avm_pa_pid_get_hwinfo(avm_egress->pid_handle);

	if (!ingress_hw || !egress_hw) {
		session_debug(
			DBG_ENABLE_MASK_ADD_SESSION,
			"hw_pointer not valid: ingress_hw=%p, egress_hw=%p \n",
			ingress_hw, egress_hw);
		goto ERR_FREE_PITEM;
	}

	update_atm_flag(ingress_hw);
	update_atm_flag(egress_hw);

	if (egress_hw->flags & AVMNET_DEVICE_IFXPPA_DISABLE_TX_ACL) {
		// this flag is set by kdsld for ADSL with WAN_TX_Rateshaping,
		// as ifx_ppa doesn't support rateshaping in ADSL Mode
		session_debug(DBG_ENABLE_MASK_ADD_SESSION,
			      " upstream acceleration disabled \n");
		goto ERR_FREE_PITEM;
	}

	p_item->dest_port_map = 0;

	// get outgoing priority from AVM_PA Session
	priority = avm_egress->output.priority;
	priority = (priority & TC_H_MIN_MASK);
	if (priority > MAX_QID)
		priority = MAX_QID;

	// OUTGOING VIRTUAL CHANNEL (e.g. WLAN)

	if (egress_hw->flags & AVMNET_DEVICE_IFXPPA_VIRT_TX) {
		p_item->dest_ifid =
			egress_hw->virt_tx_dev->hw_dev_id + IFX_VIRT_DEV_OFFSET;
		p_item->dest_port_map = 0;

		session_debug(DBG_ENABLE_MASK_ADD_SESSION,
			      "xxx -> wlan (dest_ifid =%d) \n",
			      p_item->dest_ifid);
	}

	// OUTGOING ETHERNET
	if (egress_hw->flags &
	    (AVMNET_DEVICE_IFXPPA_ETH_LAN | AVMNET_DEVICE_IFXPPA_ETH_WAN)) {
		// XXX -> ETHERNET
		if (use_avm_dest_port_map) {
			p_item->dest_ifid = (egress_hw->flags &
					     AVMNET_DEVICE_IFXPPA_ETH_WAN) ?
						    LANTIQ_WAN_DEV :
						    LANTIQ_LAN_DEV;
			p_item->dest_port_map = (1 << egress_hw->mac_nr);
			p_item->flags |= SESSION_VALID_AVM_PORTMAP;
		} else {
			if (egress_hw->mac_nr < 2) {
				p_item->dest_ifid = egress_hw->mac_nr;
				p_item->dest_port_map = 0;
				p_item->flags &= ~SESSION_VALID_AVM_PORTMAP;
			} else {
				goto ERR_FREE_PITEM;
			}
		}
		p_item->dslwan_qid = ifx_ppa_drv_avm_get_eth_qid(
			egress_hw->mac_nr, priority);

		// we need to disable acceleration for (PCIE-WLAN -> ETHERNET), if
		// port is not running in GE Mode, otherwise we might drop burtsy wlan traffic
		// in our switch
		if (ingress_hw->flags & AVMNET_DEVICE_IFXPPA_VIRT_RX) {
			if (speed_on_mac_nr(egress_hw->mac_nr) == 2) {
				session_debug(
					DBG_ENABLE_MASK_ADD_SESSION,
					"do acc VIRT_RX -> 1000mbit lan/ata (dstmac =%d) \n",
					egress_hw->mac_nr);
			} else {
				session_debug(
					DBG_ENABLE_MASK_ADD_SESSION,
					"no acc for VIRT_RX -> 100/10mbit lan/ata (dstmac =%d) \n",
					egress_hw->mac_nr);
				goto ERR_FREE_PITEM;
			}
		}
		session_debug(DBG_ENABLE_MASK_ADD_SESSION,
			      "xxx -> lan/ata (dstmac =%d) qid=%d \n",
			      egress_hw->mac_nr, p_item->dslwan_qid);
	}

	// INCOMMING ETHERNET

	if ((ingress_hw->flags & AVMNET_DEVICE_IFXPPA_ETH_WAN) &&
	    in_avm_ata_mode()) {
		// ATA -> XXX
		p_item->flags |= SESSION_WAN_ENTRY;
	} else if (ingress_hw->flags & (AVMNET_DEVICE_IFXPPA_ETH_LAN |
					AVMNET_DEVICE_IFXPPA_VIRT_RX)) {
		// (LAN | WLAN) -> XXX
		//TODO ADD WLAN-ATA-Mode-Handling
		p_item->flags |= SESSION_LAN_ENTRY;
	}

	//  DSL DOWNSTREAM

	if (ingress_hw->flags &
	    (AVMNET_DEVICE_IFXPPA_PTM_WAN | AVMNET_DEVICE_IFXPPA_ATM_WAN)) {
		// (V/A)DSL-WAN -> LAN

		p_item->flags |= SESSION_WAN_ENTRY;
		p_item->flags |= SESSION_VALID_DSLWAN_QID;
		p_item->dslwan_qid = 0; //TODO das scheint egal zu sein?!
		session_debug(DBG_ENABLE_MASK_ADD_SESSION,
			      "dslwan -> lan: qid = %d \n", p_item->dslwan_qid);
	}

	//  DSL UPSTREAM

	if (egress_hw->flags & AVMNET_DEVICE_IFXPPA_ATM_WAN) {
		// LAN -> WAN ADSL

		p_item->flags |= SESSION_LAN_ENTRY | SESSION_VALID_DSLWAN_QID;
		p_item->dest_ifid = 7;
		p_item->dslwan_qid = ifx_ppa_drv_avm_get_atm_qid(
			(struct atm_vcc *)egress_hw->atmvcc, priority);
		session_debug(DBG_ENABLE_MASK_ADD_SESSION,
			      "LAN -> ADSL: priority %u qid = %d\n", priority,
			      p_item->dslwan_qid);
	}

	if (egress_hw->flags & AVMNET_DEVICE_IFXPPA_PTM_WAN) {
		// LAN -> WAN VDSL
		p_item->flags |= SESSION_LAN_ENTRY | SESSION_VALID_DSLWAN_QID;
		p_item->dest_ifid = 7;
		p_item->dslwan_qid = ifx_ppa_drv_avm_get_ptm_qid(priority);
		session_debug(DBG_ENABLE_MASK_ADD_SESSION,
			      "LAN -> VDSL: priority=%u, qid %u\n", priority,
			      p_item->dslwan_qid);
	}

	if ((p_item->flags & SESSION_LAN_ENTRY & SESSION_WAN_ENTRY) ||
	    ((p_item->flags & (SESSION_LAN_ENTRY | SESSION_WAN_ENTRY)) == 0)) {
		session_error("session cannot be classified");
		goto ERR_FREE_PITEM;
	}

	if ((p_item->flags & SESSION_LAN_ENTRY) && disable_lan_sessions) {
		session_error("disable_lan_sessions is set");
		goto ERR_FREE_PITEM;
	}

	session_debug(DBG_ENABLE_MASK_ADD_SESSION,
		      "Try to add session with if: ( %d  -> %d)\n",
		      ingress_hw->mac_nr, egress_hw->mac_nr);

	p_item->mtu = avm_egress->mtu;

	/*--------------------*\
	 * Ingress (pkt match)
	 \*--------------------*/
	for (n = 0; n < avm_session->ingress.nmatch; n++) {
		struct avm_pa_match_info *p = &avm_session->ingress.match[n];
		void *hdr = &avm_session->ingress
				     .hdrcopy[p->offset +
					      avm_session->ingress.hdroff];
		int num_supported_ingerss_vlan_tags =
			(ingress_hw->flags & AVMNET_DEVICE_IFXPPA_ATM_WAN) ?
				1 :
				MAX_SUPPORTED_VLAN_TAGS;

		switch (p->type) {
		case AVM_PA_ETH: {
			struct ethhdr *ethh = hdr;
			memcpy(&p_item->src_mac, &ethh->h_source, PPA_ETH_ALEN);
			aass |= AASS_INGRESS_ETHERNET;
			break;
		}

		case AVM_PA_IPV4: {
			struct iphdr *iph = hdr;
			if (in_ip_cnt >= MAX_IP_HDR) {
				session_error(
					" only two ip headers are supported ");
				goto ERR_FREE_PITEM;
			}
			if (aass & AASS_INGRESS_IPV4) {
				session_error(
					" IPv4 in IPv4 is not supported ");
				goto ERR_FREE_PITEM;
			}
			aass |= AASS_INGRESS_IPV4;
			if (aass & AASS_INGRESS_IPV6) {
				aass |= AASS_INGRESS_IPV6V4;
			}
			in_ip_daddr_ptr[in_ip_cnt] = &iph->daddr;
			in_ip_saddr_ptr[in_ip_cnt] = &iph->saddr;
			in_ip_cnt++;
			break;
		}

		case AVM_PA_IPV6: {
			struct ipv6hdr *ip6h = hdr;
			if (in_ip_cnt >= MAX_IP_HDR) {
				session_error(
					" only two ip headers are supported ");
				goto ERR_FREE_PITEM;
			}
			if (aass & AASS_INGRESS_IPV6) {
				session_error(
					" IPv6 in IPv6 is not supported ");
				goto ERR_FREE_PITEM;
			}
			aass |= AASS_INGRESS_IPV6;
			p_item->flags |= SESSION_IS_IPV6;
			if (aass & AASS_INGRESS_IPV4) {
				// IPv6 in IPv4
				aass |= AASS_INGRESS_IPV4V6;
			}
			in_ip_daddr_ptr[in_ip_cnt] = ip6h->daddr.s6_addr32;
			in_ip_saddr_ptr[in_ip_cnt] = ip6h->saddr.s6_addr32;
			in_ip_cnt++;
			break;
		}

		case AVM_PA_PORTS: {
			__be16 *ports = hdr;
			p_item->src_port = ntohs(ports[0]);
			p_item->dst_port = ntohs(ports[1]);
			break;
		}

		case AVM_PA_PPPOE: {
			/* WAN ingress */
			struct pppoe_hdr *ppph = hdr;
			p_item->pppoe_session_id = ppph->sid;
			session_debug(DBG_ENABLE_MASK_ADD_SESSION,
				      "incoming pppoe_id %d\n",
				      p_item->pppoe_session_id);
			aass |= AASS_INGRESS_PPPOE;
			p_item->flags |= SESSION_VALID_PPPOE;
			break;
		}

		case AVM_PA_VLAN: {
			struct vlan_hdr *vlanh = hdr;
			session_debug(DBG_ENABLE_MASK_ADD_SESSION_TRACE,
				      " got incoming vlan id %d\n",
				      ntohs(vlanh->h_vlan_TCI) & VLAN_VID_MASK);
			if (nr_vlan_in < num_supported_ingerss_vlan_tags) {
				// incoming_vlan_tci[nr_vlan_in] = htons(hdr->vlanh.vlan_tci);
				nr_vlan_in++;
			} else {
				session_error(
					"to many incoming vlan tags (>%d) ",
					num_supported_ingerss_vlan_tags);
				goto ERR_FREE_PITEM;
			}
			break;
		}
		case AVM_PA_PPP:
			session_debug(DBG_ENABLE_MASK_ADD_SESSION_TRACE,
				      " ingress PPP -> nothing to do\n");
			break;

		default:
			session_error(" unknown ingress match type %d ",
				      p->type);
			goto ERR_FREE_PITEM;
			break;
		}
	}

	if (avm_session->mod.v4_mod.flags & AVM_PA_V4_MOD_DADDR) {
		if (p_item->flags & SESSION_LAN_ENTRY) {
			session_error(
				"DNAT only supported for WAN-SESSIONS flags:%x",
				p_item->flags);
			goto ERR_FREE_PITEM;
		}
		p_item->nat_ip.ip = avm_session->mod.v4_mod.daddr;
		p_item->flags |= SESSION_VALID_NAT_IP;
	}

	if (avm_session->mod.v4_mod.flags & AVM_PA_V4_MOD_SADDR) {
		if (p_item->flags & SESSION_WAN_ENTRY) {
			session_error(
				"SNAT only supported for LAN-SESSIONS; flags:%x",
				p_item->flags);
			goto ERR_FREE_PITEM;
		}
		p_item->nat_ip.ip = avm_session->mod.v4_mod.saddr;
		p_item->flags |= SESSION_VALID_NAT_IP;
	}

	if (avm_session->mod.v4_mod.flags & AVM_PA_V4_MOD_SPORT) {
		if (p_item->flags & SESSION_WAN_ENTRY) {
			session_error(
				"sport nat-port %d only supported for LAN-SESSIONS",
				p_item->nat_port);
			goto ERR_FREE_PITEM;
		}
		p_item->nat_port = avm_session->mod.v4_mod.sport;
		p_item->flags |= SESSION_VALID_NAT_PORT;

		if (!(p_item->flags & SESSION_VALID_NAT_IP)) {
			// lantiq ppa braucht bei nicht transparentemp nat port auch
			// explizit eine nat ip address (die entspricht der dst_ip)
			p_item->nat_ip.ip = avm_session->mod.v4_mod.saddr;
			p_item->flags |= SESSION_VALID_NAT_IP;
		}
	}

	if (avm_session->mod.v4_mod.flags & AVM_PA_V4_MOD_DPORT) {
		if (p_item->flags & SESSION_LAN_ENTRY) {
			session_error(
				"dport nat-port %d only supported for WAN-SESSIONS",
				p_item->nat_port);
			goto ERR_FREE_PITEM;
		}
		p_item->nat_port = avm_session->mod.v4_mod.dport;
		p_item->flags |= SESSION_VALID_NAT_PORT;

		if (!(p_item->flags & SESSION_VALID_NAT_IP)) {
			// lantiq ppa braucht bei nicht transparentemp nat port auch
			// explizit eine nat ip address (die entspricht der dst_ip)
			p_item->nat_ip.ip = avm_session->mod.v4_mod.daddr;
			p_item->flags |= SESSION_VALID_NAT_IP;
		}
	}

	/*--------------------*\
	 * Egress
	\*--------------------*/
	for (n = 0; n < avm_egress->match.nmatch; n++) {
		struct avm_pa_match_info *p = avm_egress->match.match + n;
		void *hdr =
			&avm_egress->match
				 .hdrcopy[p->offset + avm_egress->match.hdroff];
		int num_supported_egerss_vlan_tags =
			(egress_hw->flags & AVMNET_DEVICE_IFXPPA_ATM_WAN) ?
				1 :
				MAX_SUPPORTED_VLAN_TAGS;

		switch (p->type) {
		case AVM_PA_VLAN: {
			struct vlan_hdr *vlanh = hdr;
			session_debug(DBG_ENABLE_MASK_ADD_SESSION_TRACE,
				      " egress vlan id %d \n",
				      ntohs(vlanh->h_vlan_TCI) & VLAN_VID_MASK);
			if (nr_vlan_out < num_supported_egerss_vlan_tags) {
				outgoing_vlan_tci[nr_vlan_out++] =
					htons(vlanh->h_vlan_TCI);
			} else {
				session_error(
					"to many outgoing vlan tags (>%d) ",
					num_supported_egerss_vlan_tags);
				goto ERR_FREE_PITEM;
			}
			break;
		}

		case AVM_PA_ETH: {
			struct ethhdr *ethh = hdr;
			session_debug(DBG_ENABLE_MASK_ADD_SESSION_TRACE,
				      "egress eth\n");
			memcpy(&p_item->dst_mac, &ethh->h_dest, PPA_ETH_ALEN);
			memcpy(&p_item->new_src_mac, &ethh->h_source,
			       PPA_ETH_ALEN);
			p_item->flags |= SESSION_VALID_NEW_SRC_MAC;
			aass |= AASS_EGRESS_ETHERNET;
			break;
		}

		case AVM_PA_IPV4: {
			struct iphdr *iph = hdr;
			session_debug(DBG_ENABLE_MASK_ADD_SESSION_TRACE,
				      "egress IPv4\n");
			if (eg_ip_cnt >= MAX_IP_HDR) {
				session_error(
					" only two ip headers are supported ");
				goto ERR_FREE_PITEM;
			}
			if (aass & AASS_EGRESS_IPV4) {
				session_error(
					" IPv4 in IPv4 is not supported ");
				goto ERR_FREE_PITEM;
			}
			aass |= AASS_EGRESS_IPV4;
			if (aass & AASS_EGRESS_IPV6) {
				aass |= AASS_EGRESS_IPV6V4;
			}
			eg_ip_daddr_ptr[eg_ip_cnt] = &iph->daddr;
			eg_ip_saddr_ptr[eg_ip_cnt] = &iph->saddr;
			eg_ip_cnt++;

			break;
		}

		case AVM_PA_IPV6: {
			struct ipv6hdr *ip6h = hdr;
			session_debug(DBG_ENABLE_MASK_ADD_SESSION_TRACE,
				      "egress IPv6\n");
			if (eg_ip_cnt >= MAX_IP_HDR) {
				session_error(
					" only two ip headers are supported ");
				goto ERR_FREE_PITEM;
			}
			if (aass & AASS_EGRESS_IPV6) {
				session_error(
					" IPv6 in IPv6 is not supported ");
				goto ERR_FREE_PITEM;
			}
			p_item->flags |= SESSION_IS_IPV6;
			aass |= AASS_EGRESS_IPV6;
			if (aass & AASS_EGRESS_IPV4) {
				aass |= AASS_EGRESS_IPV4V6;
			}
			eg_ip_daddr_ptr[eg_ip_cnt] = ip6h->daddr.s6_addr32;
			eg_ip_saddr_ptr[eg_ip_cnt] = ip6h->saddr.s6_addr32;
			eg_ip_cnt++;

			break;
		}

		case AVM_PA_PORTS:
			session_debug(DBG_ENABLE_MASK_ADD_SESSION_TRACE,
				      "egress ports -> nothing to do\n");
			break;

		case AVM_PA_PPP:
			p_item->flags |= SESSION_TX_ITF_PPPOA;
			aass |= AASS_EGRESS_PPPOA;
			session_debug(DBG_ENABLE_MASK_ADD_SESSION_TRACE,
				      "egress PPP\n");
			break;

		case AVM_PA_PPPOE:
			session_debug(DBG_ENABLE_MASK_ADD_SESSION_TRACE,
				      "egress PPPOE\n");
			{
				struct pppoe_hdr *ppph =
					(struct pppoe_hdr
						 *)(avm_egress->match.hdrcopy +
						    avm_egress->match.hdroff +
						    avm_egress->pppoe_offset);
				u16 outgoing_pppoe_sid = ppph->sid;

				aass |= AASS_EGRESS_PPPOE;
				session_debug(DBG_ENABLE_MASK_ADD_SESSION,
					      "outgoing pppoe_id %d\n",
					      outgoing_pppoe_sid);

				if ((aass & AASS_INGRESS_PPPOE) &&
				    (outgoing_pppoe_sid ==
				     p_item->pppoe_session_id)) {
					// PPPOE NQOS-PATH-THROUGH ppa shall not add additional pppoe header
					session_debug(
						DBG_ENABLE_MASK_ADD_SESSION,
						"outgoing and incoming pppoe_sid %d are equivalent -> Box seems to be trunk port mode\n",
						outgoing_pppoe_sid);
					p_item->flags &= ~SESSION_VALID_PPPOE;
				} else {
					p_item->pppoe_session_id =
						outgoing_pppoe_sid;
					p_item->flags |= SESSION_VALID_PPPOE;
				}
			}
			break;

		default:
			session_error(" unknown egress match type %d ",
				      p->type);
			goto ERR_FREE_PITEM;
			break;
		}
	}

	/*------------------------*\
	 * Setup Tunnel Sessions
	 *
	 *   6RD:    IPv6 in IPv4: (Provider-Backbone kann nur v4)
	 *	                  WAN-Session: IPv4 + IPv6 -----> IPv6
	 *	                               in[0]  in[1]       eg[0]
	 *
	 *	                  LAN-Session: IPV6 ------------> IPv4  + IPv6
	 *	                               in[0]              eg[0]   eg[1]
	 *
	 *   DSlite: IPv4 in IPv6: (Provider-Backbone kann nur v6)
	 *	                  WAN-Session: IPv6 + IPv4 -----> IPv4
	 *	                               in[0]  in[1]       eg[0]
	 *
	 *	                  LAN-Session: IPV4 ------------> IPv6 + IPv4
	 *	                               in[0]              eg[0]  eg[1]
	 *------------------------*/
	if (unlikely(aass & (AASS_INGRESS_IPV4V6 | AASS_EGRESS_IPV4V6 |
			     AASS_INGRESS_IPV6V4 | AASS_EGRESS_IPV6V4))) {
		if ((aass & AASS_INGRESS_IPV4V6) &&
		    (aass & AASS_EGRESS_IPV4V6)) {
			session_error(
				"no hardware support for 6RD pass through");
			goto ERR_FREE_PITEM;
		}

		if ((aass & AASS_INGRESS_IPV6V4) &&
		    (aass & AASS_EGRESS_IPV6V4)) {
			session_error(
				"no hardware support for DSLite pass through");
			goto ERR_FREE_PITEM;
		}

		if ((aass & AASS_INGRESS_IPV4V6) && (aass & AASS_EGRESS_IPV6)) {
			/* 6RD WAN Session */
			session_debug(DBG_ENABLE_MASK_ADD_SESSION,
				      "setup 6RD WAN session");
			p_item->flags |= SESSION_TUNNEL_6RD;

			p_item->tun_daddr.ip = *in_ip_daddr_ptr[0];
			p_item->tun_saddr.ip = *in_ip_saddr_ptr[0];

			memcpy(p_item->dst_ip.ip6, in_ip_daddr_ptr[1], 16);
			memcpy(p_item->src_ip.ip6, in_ip_saddr_ptr[1], 16);

			if (memcmp(in_ip_daddr_ptr[1], eg_ip_daddr_ptr[0],
				   16) ||
			    memcmp(in_ip_saddr_ptr[1], eg_ip_saddr_ptr[0],
				   16)) {
				session_error("bug in 6RD session");
				goto ERR_FREE_PITEM;
			}
		}

		if ((aass & AASS_INGRESS_IPV6) && (aass & AASS_EGRESS_IPV4V6)) {
			/* 6RD LAN Session */
			session_debug(DBG_ENABLE_MASK_ADD_SESSION,
				      "setup 6RD LAN session");
			p_item->flags |= SESSION_TUNNEL_6RD;

			memcpy(p_item->dst_ip.ip6, in_ip_daddr_ptr[0], 16);
			memcpy(p_item->src_ip.ip6, in_ip_saddr_ptr[0], 16);

			p_item->tun_daddr.ip = *eg_ip_daddr_ptr[0];
			p_item->tun_saddr.ip = *eg_ip_saddr_ptr[0];

			if (memcmp(in_ip_daddr_ptr[0], eg_ip_daddr_ptr[1],
				   16) ||
			    memcmp(in_ip_saddr_ptr[0], eg_ip_saddr_ptr[1],
				   16)) {
				session_error("bug in 6RD session");
				goto ERR_FREE_PITEM;
			}
		}

		if ((aass & AASS_INGRESS_IPV6V4) && (aass & AASS_EGRESS_IPV4)) {
			/* DSLite WAN Session */
			session_debug(DBG_ENABLE_MASK_ADD_SESSION,
				      "setup DSLite WAN session");
			p_item->flags |= SESSION_TUNNEL_DSLITE;
			p_item->flags &= ~SESSION_IS_IPV6;

			memcpy(p_item->tun_daddr.ip6, in_ip_daddr_ptr[0], 16);
			memcpy(p_item->tun_saddr.ip6, in_ip_saddr_ptr[0], 16);

			p_item->dst_ip.ip = *in_ip_daddr_ptr[1];
			p_item->src_ip.ip = *in_ip_saddr_ptr[1];

			if (memcmp(in_ip_daddr_ptr[1], eg_ip_daddr_ptr[0], 4) ||
			    memcmp(in_ip_saddr_ptr[1], eg_ip_saddr_ptr[0], 4)) {
				session_error("bug in DSLite session");
				goto ERR_FREE_PITEM;
			}
		}

		if ((aass & AASS_INGRESS_IPV4) && (aass & AASS_EGRESS_IPV6V4)) {
			/* DSLite LAN Session */
			session_debug(DBG_ENABLE_MASK_ADD_SESSION,
				      "setup DSLite LAN session");
			p_item->flags |= SESSION_TUNNEL_DSLITE;
			p_item->flags &= ~SESSION_IS_IPV6;

			p_item->dst_ip.ip = *in_ip_daddr_ptr[0];
			p_item->src_ip.ip = *in_ip_saddr_ptr[0];

			memcpy(p_item->tun_daddr.ip6, eg_ip_daddr_ptr[0], 16);
			memcpy(p_item->tun_saddr.ip6, eg_ip_saddr_ptr[0], 16);

			if (memcmp(in_ip_daddr_ptr[0], eg_ip_daddr_ptr[1], 4) ||
			    memcmp(in_ip_saddr_ptr[0], eg_ip_saddr_ptr[1], 4)) {
				session_error("bug in DSLite session");
				goto ERR_FREE_PITEM;
			}
		}

		/*---------------------------------*\
	 * Sessions with "just" one IP-HDR
	\*---------------------------------*/
	} else {
		if (aass & AASS_INGRESS_IPV4) {
			p_item->dst_ip.ip = *in_ip_daddr_ptr[0];
			p_item->src_ip.ip = *in_ip_saddr_ptr[0];
		} else if (aass & AASS_INGRESS_IPV6) {
			memcpy(p_item->dst_ip.ip6, in_ip_daddr_ptr[0], 16);
			memcpy(p_item->src_ip.ip6, in_ip_saddr_ptr[0], 16);
		} else {
			session_error("neather IPv4 nor IPv6");
			goto ERR_FREE_PITEM;
		}
	}

	/*----------------------*\
	 * Constraints checking
	\*----------------------*/

	if (!(aass & (AASS_EGRESS_ETHERNET | AASS_INGRESS_ETHERNET))) {
		session_error(
			"no valid ethernet - we need at least one device to be ethernet");
		goto ERR_FREE_PITEM;
	}

	if ((p_item->flags & SESSION_LAN_ENTRY) &&
	    (!(aass & (AASS_EGRESS_ETHERNET | AASS_EGRESS_PPPOA |
		       AASS_EGRESS_PPPOE)))) {
		session_debug(DBG_ENABLE_MASK_ADD_SESSION_TRACE,
			      "egress fallback: session has to be IPoA\n");
		p_item->flags |= SESSION_TX_ITF_IPOA;
		aass |= AASS_EGRESS_IPOA; // aass will not be evaluated any more, this is just for debug purpose
	}
	/*--------------------*\
	 *     setup vlan
        \*--------------------*/

	if (nr_vlan_out || nr_vlan_in) {
		session_debug(DBG_ENABLE_MASK_ADD_SESSION,
			      "NR VLAN (in=%d, out=%d)\n", nr_vlan_in,
			      nr_vlan_out);

		if (nr_vlan_out > 0) {
			p_item->out_vlan_tag =
				((ETH_P_8021Q << 16) | outgoing_vlan_tci[0]);
			p_item->flags |= SESSION_VALID_OUT_VLAN_INS;
		}

		if (nr_vlan_out > 1) {
			p_item->new_vci = outgoing_vlan_tci[1];
			p_item->flags |= SESSION_VALID_VLAN_INS;
		}

		if (nr_vlan_in == 2) {
			// In ATM this can not happen, because of num_supported_egerss_vlan_tags
			p_item->flags |= SESSION_VALID_VLAN_RM;
			p_item->flags |= SESSION_VALID_OUT_VLAN_RM;
		}

		if (nr_vlan_in == 1) {
			if (ingress_hw->flags & AVMNET_DEVICE_IFXPPA_ATM_WAN)
				p_item->flags |= SESSION_VALID_VLAN_RM;
			else
				p_item->flags |= SESSION_VALID_OUT_VLAN_RM;
		}
		session_debug(DBG_ENABLE_MASK_ADD_SESSION,
			      "flags=%#x (vlan_tag=%#x, new_vci=%#x)\n",
			      p_item->flags, p_item->out_vlan_tag,
			      ((unsigned int)p_item->new_vci & 0xFFFF));
	}

	// dump_list_item(p_item, "avm_pa_add_ifx_session (after setting)");

	if (p_item->flags & SESSION_MC_ENTRY) {
		if ((hw_ret = ppa_hw_add_mc_session(p_item)) != IFX_SUCCESS) {
			session_debug(
				DBG_ENABLE_MASK_ADD_SESSION,
				"ppa_hw_add_mc_session failed: %d (probably no more space for any sessions)\n",
				hw_ret);
			goto ERR_FREE_PITEM;
		}
	} else if ((hw_ret = ppa_hw_add_session(p_item)) != IFX_SUCCESS) {
		session_debug(
			DBG_ENABLE_MASK_ADD_SESSION,
			"ppa_hw_add_session failed: %d (probably no more space for any sessions)\n",
			hw_ret);
		goto ERR_FREE_PITEM;
	}
	session_debug(DBG_ENABLE_MASK_VDEV_SESSION,
		      "%lu: add routing_entry=%d (%s)%s%s\n", jiffies,
		      p_item->routing_entry & 0x7FFFFFFF,
		      (p_item->flags & SESSION_LAN_ENTRY) ? "LAN" : "WAN",
		      (p_item->flags & SESSION_IS_IPV6) ? "IPV6" : "",
		      (p_item->flags & SESSION_MC_ENTRY) ? " MULTICAST" : "");

	print_flags(aass);
	p_item = insert_session_item(p_item);
	if (!p_item) {
		BUG();
		//TODO remove hw session
		goto ERR_FREE_PITEM;
	}

	avm_session->hw_session = p_item;
	return AVM_PA_TX_SESSION_ADDED; //AVM_PA_SUCCESS;

ERR_FREE_PITEM:
ERR:
	// print_flags( aass );
	return AVM_PA_TX_ERROR_SESSION; //AVM_PA_FAILURE;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avm_pa_remove_ifx_session(struct avm_pa_session *avm_session)
{
	struct session_list_item *p_item;
	unsigned int store_routing_entry;
	unsigned int store_is_lan;
	unsigned int store_is_mc;

	if (!avm_session) {
		printk(KERN_ERR "[%s] avm_session is null!\n", __func__);
		return IFX_FAILURE;
	}
	p_item = avm_session->hw_session;

	if (!p_item) {
		printk(KERN_ERR "[%s] p_item is null!\n", __func__);
		return IFX_FAILURE;
	}

	store_routing_entry = p_item->routing_entry;
	store_is_lan = p_item->flags & SESSION_LAN_ENTRY;
	store_is_mc = p_item->flags & SESSION_MC_ENTRY;

	dump_list_item(p_item, "ifx_ppa_session_delete");

	session_debug(DBG_ENABLE_MASK_DEL_SESSION, "remove session %p\n",
		      p_item);

	invalidate_session_item(
		p_item); // now we will poll no more statistics for this session

	if (store_is_mc)
		ppa_hw_del_mc_session(p_item);
	else
		ppa_hw_del_session(p_item);

	remove_session_item(p_item);

	//TODO PWM integrieren!
	// ifx_ppa_pwm_deactivate_module();

	session_debug(DBG_ENABLE_MASK_VDEV_SESSION,
		      "%lu: remove routing_entry=%d (%s) %s\n", jiffies,
		      store_routing_entry & 0x7FFFFFFF,
		      store_is_lan ? "LAN" : "WAN",
		      store_is_mc ? "MULTICAST" : "");
	avm_sessionh_table_remove(store_routing_entry, store_is_lan,
				  store_is_mc);

	session_debug(DBG_ENABLE_MASK_DEL_SESSION, "deaktivate ppa pwm\n");

	avm_session->hw_session = NULL;
	return IFX_SUCCESS;
}

int avm_pa_change_ifx_session(struct avm_pa_session *avm_session)
{
	struct session_list_item *p_item;
	struct avm_pa_egress *avm_egress;
	int res = AVM_PA_TX_ERROR_EGRESS;

	if (!avm_session) {
		printk(KERN_ERR "[%s] avm_session is null!\n", __func__);
		return res;
	}
	p_item = avm_session->hw_session;

	if (!p_item) {
		printk(KERN_ERR "[%s] p_item is null!\n", __func__);
		return res;
	}
	if (!(p_item->flags & SESSION_MC_ENTRY)) {
		printk(KERN_ERR "[%s] only supported for multicast sessions!\n",
		       __func__);
		res = AVM_PA_TX_ERROR_SESSION;
		goto remove_session;
	}

	avm_pa_for_each_egress(avm_egress, avm_session)
	{
		int j;
		struct avm_pa_pid_hwinfo *mcegress =
			avm_pa_pid_get_hwinfo(avm_egress->pid_handle);

		if (mcegress == NULL) {
			res = AVM_PA_TX_ERROR_EGRESS;
			goto remove_session;
		}

		for (j = 0; j < avm_egress->match.nmatch; j++) {
			struct avm_pa_match_info *p =
				avm_egress->match.match + j;
			if (p->type == AVM_PA_VLAN) {
				session_debug(
					DBG_ENABLE_MASK_ADD_SESSION,
					"change session VLAN not supported delete "
					"session\n");
				res = AVM_PA_TX_ERROR_SESSION;
				goto remove_session;
			}
		}
		p_item->dest_port_map |= (1 << mcegress->mac_nr);
	}
	dump_list_item(p_item, "ifx_ppa_session_change");
	session_debug(DBG_ENABLE_MASK_ADD_SESSION, "change session %p\n",
		      p_item);

	if ((p_item->flags & SESSION_ADDED_IN_HW)) {
		PPE_MC_INFO mc = { 0 };
		uint32_t res;
		//  when init, these entry values are ~0, the max the number which can be detected by these functions
		mc.p_entry = p_item->routing_entry;
		mc.dest_list = 1 << p_item->dest_ifid;
		mc.dest_list |= ((p_item->dest_port_map & 0x3F) << 2);
		mc.update_flags = IFX_PPA_UPDATE_WAN_MC_ENTRY_DEST_LIST;
		res = ifx_ppa_drv_update_wan_mc_entry(&mc, 0);
		session_debug(DBG_ENABLE_MASK_ADD_HWSESSION,
			      "changed HW MC session:avm_pa_id=%d, res=%d\n",
			      p_item->session_id, res);
	}
	session_debug(DBG_ENABLE_MASK_ADD_SESSION, "changed session \n");
	return AVM_PA_TX_EGRESS_ADDED;

remove_session:
	avm_pa_remove_ifx_session(avm_session);
	return res;
}

/*------------------------------------------------------------------------------------------*\
 * ppa_hw_add_session aus ugw 5.2
\*------------------------------------------------------------------------------------------*/
static int32_t ppa_hw_add_session(struct session_list_item *p_item)
{
	PPE_ROUTING_INFO route;

	int ret = IFX_FAILURE;

	//  Only add session in H/w when the called from the post-NAT hook
	ppa_memset(&route, 0, sizeof(route));
	route.src_mac.mac_ix = ~0;
	route.pppoe_info.pppoe_ix = ~0;
	route.out_vlan_info.vlan_entry = ~0;
	route.mtu_info.mtu_ix = ~0;

	route.route_type = (p_item->flags & SESSION_VALID_NAT_IP) ?
				   ((p_item->flags & SESSION_VALID_NAT_PORT) ?
					    IFX_PPA_ROUTE_TYPE_NAPT :
					    IFX_PPA_ROUTE_TYPE_NAT) :
				   IFX_PPA_ROUTE_TYPE_IPV4;
	if ((p_item->flags & SESSION_VALID_NAT_IP)) {
		route.new_ip.f_ipv6 = 0;
		memcpy(&route.new_ip.ip, &p_item->nat_ip,
		       sizeof(route.new_ip
				      .ip)); //since only IPv4 support NAT, translate it to IPv4 format
	}
	if ((p_item->flags & SESSION_VALID_NAT_PORT))
		route.new_port = p_item->nat_port;

	session_debug(DBG_ENABLE_MASK_ADD_SESSION, "flags=%#x\n",
		      p_item->flags);
	if ((p_item->flags & (SESSION_VALID_PPPOE | SESSION_LAN_ENTRY)) ==
	    (SESSION_VALID_PPPOE | SESSION_LAN_ENTRY)) {
		session_debug(DBG_ENABLE_MASK_ADD_SESSION,
			      "try to add pppoe_session_id=%d\n",
			      p_item->pppoe_session_id);
		route.pppoe_info.pppoe_session_id = p_item->pppoe_session_id;
		if (ifx_ppa_drv_add_pppoe_entry(&route.pppoe_info, 0) ==
		    IFX_SUCCESS) {
			session_debug(DBG_ENABLE_MASK_ADD_SESSION,
				      "try to added pppoe_ix=%d\n",
				      route.pppoe_info.pppoe_ix);
			p_item->pppoe_entry = route.pppoe_info.pppoe_ix;
		} else {
			session_debug(DBG_ENABLE_MASK_ADD_HWSESSION,
				      "add pppoe_entry error\n");
			SET_DBG_FLAG(p_item, SESSION_DBG_ADD_PPPOE_ENTRY_FAIL);
			goto SESSION_VALID_PPPOE_ERROR;
		}
	}

	route.mtu_info.mtu = p_item->mtu;
	if (ifx_ppa_drv_add_mtu_entry(&route.mtu_info, 0) == IFX_SUCCESS) {
		p_item->mtu_entry = route.mtu_info.mtu_ix;
		p_item->flags |= SESSION_VALID_MTU;
	} else {
		SET_DBG_FLAG(p_item, SESSION_DBG_ADD_MTU_ENTRY_FAIL);
		goto MTU_ERROR;
	}

	if ((p_item->flags & (SESSION_TUNNEL_6RD | SESSION_LAN_ENTRY)) ==
	    (SESSION_TUNNEL_6RD | SESSION_LAN_ENTRY)) {
		if (p_item->tun_daddr.ip && p_item->tun_saddr.ip) {
			// AVM-WAY
			session_debug(
				DBG_ENABLE_MASK_ADD_HWSESSION,
				"add 6RD entry to FW (avm-way) saddr %#x, daddr %#x\n",
				p_item->tun_saddr.ip, p_item->tun_daddr.ip);

			route.tnnl_info.tunnel_type = SESSION_TUNNEL_6RD;
			route.tnnl_info.src.ip = p_item->tun_saddr.ip;
			route.tnnl_info.dst.ip = p_item->tun_daddr.ip;

		} else {
			session_error(
				"we just support 6RD Tunnel with known address");
			goto MTU_ERROR;
		}

		if (ifx_ppa_drv_add_tunnel_entry(&route.tnnl_info, 0) ==
		    IFX_SUCCESS) {
			p_item->tunnel_idx = route.tnnl_info.tunnel_idx;
		} else {
			session_debug(DBG_ENABLE_MASK_ADD_HWSESSION,
				      "add tunnel 6rd entry error\n");
			goto MTU_ERROR;
		}
	}

	if (p_item->flags & SESSION_TUNNEL_6RD) {
		/*
	     * hal-driver (add_routing_entry)shiftet tunnel_idx wieder zurueck
	     * warum? unklar!
	     */
		route.tnnl_info.tunnel_idx = p_item->tunnel_idx << 1 | 1;
	}

	if ((p_item->flags & (SESSION_TUNNEL_DSLITE | SESSION_LAN_ENTRY)) ==
	    (SESSION_TUNNEL_DSLITE | SESSION_LAN_ENTRY)) {
		session_debug(DBG_ENABLE_MASK_ADD_HWSESSION,
			      "add Dslite entry to FW\n");

		session_debug(
			DBG_ENABLE_MASK_ADD_HWSESSION,
			"add 6RD entry to FW (avm-way) saddr %pI6, daddr %pI6\n",
			p_item->tun_saddr.ip6, p_item->tun_daddr.ip6);

		route.tnnl_info.tunnel_type = SESSION_TUNNEL_DSLITE;

		memcpy(route.tnnl_info.src.ip6, p_item->tun_saddr.ip6, 16);
		memcpy(route.tnnl_info.dst.ip6, p_item->tun_daddr.ip6, 16);

		if (ifx_ppa_drv_add_tunnel_entry(&route.tnnl_info, 0) ==
		    IFX_SUCCESS) {
			p_item->tunnel_idx = route.tnnl_info.tunnel_idx;
		} else {
			session_debug(DBG_ENABLE_MASK_ADD_SESSION,
				      "add tunnel dslite entry error\n");
			goto MTU_ERROR;
		}
	}

	if (p_item->flags & SESSION_TUNNEL_DSLITE) {
		/*
	     * hal-driver (add_routing_entry)shiftet tunnel_idx wieder zurueck
	     * warum? unklar!
	     */
		route.tnnl_info.tunnel_idx = p_item->tunnel_idx << 1 | 1;
	}

	if (p_item->flags & SESSION_VALID_NEW_SRC_MAC) {
		// this is the regular avm way
		ppa_memcpy(&route.src_mac.mac[0], &p_item->new_src_mac[0],
			   sizeof(route.src_mac.mac));

	} else if (!(p_item->flags & SESSION_TX_ITF_IPOA_PPPOA_MASK)) {
		// this is the regular lantiq way
		session_debug(DBG_ENABLE_MASK_ADD_SESSION,
			      "warning doing mac lookup!\n");
		p_item->flags |= SESSION_VALID_NEW_SRC_MAC;
	}

	if (p_item->flags & SESSION_VALID_NEW_SRC_MAC) {
		// Die new_src_mac wird redundant abgelegt
		ppa_memcpy(&p_item->new_src_mac[0], &route.src_mac.mac[0],
			   sizeof(route.src_mac.mac));
		if (ifx_ppa_drv_add_mac_entry(&route.src_mac, 0) ==
		    IFX_SUCCESS) {
			p_item->src_mac_entry = route.src_mac.mac_ix;
			session_debug(
				DBG_ENABLE_MASK_ADD_SESSION,
				"added new mac entry (new_src_mac) ix=%d\n",
				p_item->src_mac_entry);
		} else {
			SET_DBG_FLAG(p_item, SESSION_DBG_ADD_MAC_ENTRY_FAIL);
			goto NEW_SRC_MAC_ERROR;
		}
	}

	if ((p_item->flags & SESSION_VALID_OUT_VLAN_INS)) {
		route.out_vlan_info.vlan_id = p_item->out_vlan_tag;
		if (ifx_ppa_drv_add_outer_vlan_entry(&route.out_vlan_info, 0) ==
		    IFX_SUCCESS) {
			p_item->out_vlan_entry = route.out_vlan_info.vlan_entry;
		} else {
			session_debug(DBG_ENABLE_MASK_ADD_HWSESSION,
				      "add out_vlan_ix error\n");
			SET_DBG_FLAG(p_item,
				     SESSION_DBG_ADD_OUT_VLAN_ENTRY_FAIL);
			goto OUT_VLAN_ERROR;
		}
	}

	if ((p_item->flags & SESSION_VALID_NEW_DSCP))
		route.new_dscp = p_item->new_dscp;

	route.dest_list = 1 << p_item->dest_ifid;

	if (use_avm_dest_port_map && p_item->dest_port_map) {
		route.dest_list |= ((p_item->dest_port_map & 0x3F) << 2);
	}

	route.f_is_lan = (p_item->flags & SESSION_LAN_ENTRY) ? 1 : 0;
	session_debug(
		DBG_ENABLE_MASK_ADD_HWSESSION,
		"merged dest_list = %#x, dest_port_map=%#x, f_is_lan=%d\n",
		route.dest_list, p_item->dest_port_map, route.f_is_lan);
	ppa_memcpy(&route.new_dst_mac[0], &p_item->dst_mac[0], PPA_ETH_ALEN);
	route.dst_port = p_item->dst_port;
	route.src_port = p_item->src_port;
	route.f_is_tcp = (p_item->flags & SESSION_IS_TCP) ? 1 : 0;
	route.f_new_dscp_enable =
		(p_item->flags & SESSION_VALID_NEW_DSCP) ? 1 : 0;
	route.f_vlan_ins_enable =
		(p_item->flags & SESSION_VALID_VLAN_INS) ? 1 : 0;
	route.new_vci = p_item->new_vci;
	route.f_vlan_rm_enable =
		(p_item->flags & SESSION_VALID_VLAN_RM) ? 1 : 0;
	route.pppoe_mode = p_item->flags & SESSION_VALID_PPPOE;
	route.f_out_vlan_ins_enable =
		(p_item->flags & SESSION_VALID_OUT_VLAN_INS) ? 1 : 0;
	route.f_out_vlan_rm_enable =
		(p_item->flags & SESSION_VALID_OUT_VLAN_RM) ? 1 : 0;
	route.dslwan_qid = p_item->dslwan_qid;

#if defined(CONFIG_IFX_PPA_IPv6_ENABLE)
	if (p_item->flags & SESSION_IS_IPV6) {
		route.src_ip.f_ipv6 = 1;
		ppa_memcpy(&route.src_ip.ip.ip6[0], &p_item->src_ip.ip6[0],
			   sizeof(route.src_ip.ip.ip6));

		route.dst_ip.f_ipv6 = 1;
		ppa_memcpy(&route.dst_ip.ip.ip6[0], &p_item->dst_ip.ip6[0],
			   sizeof(route.dst_ip.ip.ip6));
	} else
#endif
	{
		route.src_ip.f_ipv6 = 0;
		route.src_ip.ip.ip = p_item->src_ip.ip;

		route.dst_ip.f_ipv6 = 0;
		route.dst_ip.ip.ip = p_item->dst_ip.ip;

		route.new_ip.f_ipv6 = 0;
	}

	p_item->collision_flag = 1; // for ASE/Danube back-compatible

	/*
     * will be activated after session_hash_table update
     * otherwise fist WLAN-Packets will be dropped, as session_id is unkown by kernel yet
     */
	route.active = 0;

	if ((ret = ifx_ppa_drv_add_routing_entry(&route, 0)) == IFX_SUCCESS) {
		p_item->routing_entry = route.entry;
		p_item->flags |= SESSION_ADDED_IN_HW;
		p_item->collision_flag = route.collision_flag;
		session_debug(DBG_ENABLE_MASK_ADD_HWSESSION,
			      "add_routing_entry successful\n");

		avm_sessionh_table_set(p_item->routing_entry,
				       p_item->session_id,
				       (p_item->flags & SESSION_LAN_ENTRY), 0);

		ifx_ppa_drv_activate_routing_entry(&route, 0);
		return IFX_SUCCESS;
	}

	//  fail in add_routing_entry
	session_debug(DBG_ENABLE_MASK_ADD_HWSESSION,
		      "fail in add_routing_entry\n");
	p_item->out_vlan_entry = ~0;
	ifx_ppa_drv_del_outer_vlan_entry(&route.out_vlan_info, 0);
OUT_VLAN_ERROR:
	p_item->src_mac_entry = NO_MAC_ENTRY;
	ifx_ppa_drv_del_mac_entry(&route.src_mac, 0);
NEW_SRC_MAC_ERROR:
	p_item->mtu_entry = ~0;
	ifx_ppa_drv_del_mtu_entry(&route.mtu_info, 0);
MTU_ERROR:
	p_item->pppoe_entry = ~0;
	ifx_ppa_drv_del_pppoe_entry(&route.pppoe_info, 0);
SESSION_VALID_PPPOE_ERROR:
	return ret;
}

static int32_t ppa_hw_add_mc_session(struct session_list_item *p_item)
{
	PPE_MC_INFO mc;
	PPE_ROUTE_MAC_INFO new_src_mac_info;
	int ret = IFX_FAILURE;

	dump_list_item(p_item, "add_mc_session");

	ppa_memset(&mc, 0, sizeof(mc));
	ppa_memset(&new_src_mac_info, 0, sizeof(new_src_mac_info));
	p_item->src_mac_entry = NO_MAC_ENTRY;
	mc.src_mac_ix = ~0;
	mc.out_vlan_info.vlan_entry = ~0;
	mc.route_type = IFX_PPA_ROUTE_TYPE_IPV4;
	mc.sample_en = 1; /* RTP FLAG */

	session_debug(DBG_ENABLE_MASK_ADD_SESSION, "flags=%#x\n",
		      p_item->flags);

	if ((p_item->flags & SESSION_VALID_OUT_VLAN_INS)) {
		mc.out_vlan_info.vlan_id = p_item->out_vlan_tag;
		if ( ifx_ppa_drv_add_outer_vlan_entry( &mc.out_vlan_info, 0 ) == IFX_SUCCESS ) {
		    p_item->out_vlan_entry = mc.out_vlan_info.vlan_entry;
		} else {
		    session_debug(DBG_ENABLE_MASK_ADD_HWSESSION,"add out_vlan_ix error\n");
		    SET_DBG_FLAG(p_item, SESSION_DBG_ADD_OUT_VLAN_ENTRY_FAIL);
		    goto OUT_VLAN_ERROR;
		}
	}

	if (p_item->flags & SESSION_VALID_NEW_SRC_MAC) {
		ppa_memcpy(&new_src_mac_info.mac[0], &p_item->new_src_mac[0],
			   PPA_ETH_ALEN);

		if (ifx_ppa_drv_add_mac_entry(&new_src_mac_info, 0) ==
		    IFX_SUCCESS) {
			p_item->src_mac_entry = new_src_mac_info.mac_ix;
			mc.src_mac_ix = p_item->src_mac_entry;
			session_debug(
				DBG_ENABLE_MASK_ADD_SESSION,
				"added new mac entry (new_src_mac) ix=%d\n",
				p_item->src_mac_entry);
		} else {
			SET_DBG_FLAG(p_item, SESSION_DBG_ADD_MAC_ENTRY_FAIL);
			goto NEW_SRC_MAC_ERROR;
		}
	}

	if ((p_item->flags & SESSION_VALID_NEW_DSCP))
		mc.new_dscp = p_item->new_dscp;

	mc.dest_list = 1 << p_item->dest_ifid;
	if (use_avm_dest_port_map && p_item->dest_port_map)
		mc.dest_list |= ((p_item->dest_port_map & 0x3F) << 2);

	session_debug(DBG_ENABLE_MASK_ADD_HWSESSION,
		      "merged dest_list = %#x, dest_port_map=%#x\n",
		      mc.dest_list, p_item->dest_port_map);

	mc.f_new_dscp_enable = p_item->flags & SESSION_VALID_NEW_DSCP;
	mc.f_vlan_ins_enable = p_item->flags & SESSION_VALID_VLAN_INS;
	mc.new_vci = p_item->new_vci;
	mc.f_vlan_rm_enable = p_item->flags & SESSION_VALID_VLAN_RM;
	mc.pppoe_mode = p_item->flags & SESSION_VALID_PPPOE;
	mc.f_out_vlan_ins_enable = p_item->flags & SESSION_VALID_OUT_VLAN_INS;
	mc.f_out_vlan_rm_enable = p_item->flags & SESSION_VALID_OUT_VLAN_RM;
	mc.dest_qid = p_item->dslwan_qid;
	mc.src_ip_compare = p_item->src_ip.ip;
	mc.dest_ip_compare = p_item->dst_ip.ip;
	mc.f_src_mac_enable = !!(p_item->flags & SESSION_VALID_NEW_SRC_MAC);

	if ((ret = ifx_ppa_drv_add_wan_mc_entry(&mc, 0)) == IFX_SUCCESS) {
		p_item->routing_entry = mc.p_entry;
		p_item->flags |= SESSION_ADDED_IN_HW;
		session_debug(DBG_ENABLE_MASK_ADD_HWSESSION,
			      "add_wan_mc_entry successful\n");
		avm_sessionh_table_set(p_item->routing_entry,
				       p_item->session_id,
				       (p_item->flags & SESSION_LAN_ENTRY), 1);
		return IFX_SUCCESS;
	}

	ifx_ppa_drv_del_outer_vlan_entry(&mc.out_vlan_info, 0);
OUT_VLAN_ERROR:
	p_item->out_vlan_entry = ~0;
	if (p_item->src_mac_entry != NO_MAC_ENTRY) {
		ifx_ppa_drv_del_mac_entry(&new_src_mac_info, 0);
	}
NEW_SRC_MAC_ERROR:
	p_item->src_mac_entry = NO_MAC_ENTRY;
	session_debug(DBG_ENABLE_MASK_ADD_HWSESSION, "fail in add MC entry\n");
	return ret;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

static void ppa_hw_del_session(struct session_list_item *p_item)
{
	PPE_ROUTING_INFO route = { 0 };

	if ((p_item->flags & SESSION_ADDED_IN_HW)) {
		uint32_t res;
		bool remove_tunnel_entry = false;
		//  when init, these entry values are ~0, the max the number which can be detected by these functions
		route.entry = p_item->routing_entry;
		res = ifx_ppa_drv_del_routing_entry(&route, 0);
		session_debug(DBG_ENABLE_MASK_DEL_HWSESSION,
			      "removed HW session:avm_pa_id=%d, res=%d\n",
			      p_item->session_id, res);
		p_item->routing_entry = ~0;

		route.out_vlan_info.vlan_entry = p_item->out_vlan_entry;
		ifx_ppa_drv_del_outer_vlan_entry(&route.out_vlan_info, 0);
		p_item->out_vlan_entry = ~0;

		route.pppoe_info.pppoe_ix = p_item->pppoe_entry;
		ifx_ppa_drv_del_pppoe_entry(&route.pppoe_info, 0);
		p_item->pppoe_entry = ~0;

		route.mtu_info.mtu_ix = p_item->mtu_entry;
		ifx_ppa_drv_del_mtu_entry(&route.mtu_info, 0);
		p_item->mtu_entry = ~0;

		route.src_mac.mac_ix = p_item->src_mac_entry;
		ifx_ppa_drv_del_mac_entry(&route.src_mac, 0);
		p_item->src_mac_entry = NO_MAC_ENTRY;

		route.tnnl_info.tunnel_idx = p_item->tunnel_idx;

		if ((p_item->flags &
		     (SESSION_TUNNEL_6RD | SESSION_LAN_ENTRY)) ==
		    (SESSION_TUNNEL_6RD | SESSION_LAN_ENTRY)) {
			route.tnnl_info.tunnel_type = SESSION_TUNNEL_6RD;
			remove_tunnel_entry = true;
		} else if ((p_item->flags &
			    (SESSION_TUNNEL_DSLITE | SESSION_LAN_ENTRY)) ==
			   (SESSION_TUNNEL_DSLITE | SESSION_LAN_ENTRY)) {
			route.tnnl_info.tunnel_type = SESSION_TUNNEL_DSLITE;
			remove_tunnel_entry = true;
		}
		if (remove_tunnel_entry)
			ifx_ppa_drv_del_tunnel_entry(&route.tnnl_info, 0);
		p_item->flags &= ~SESSION_ADDED_IN_HW;
	}
}

static void ppa_hw_del_mc_session(struct session_list_item *p_item)
{
	PPE_MC_INFO mc = { 0 };

	if ((p_item->flags & SESSION_ADDED_IN_HW)) {
		uint32_t res;
		//  when init, these entry values are ~0, the max the number which can be detected by these functions
		mc.p_entry = p_item->routing_entry;
		res = ifx_ppa_drv_del_wan_mc_entry(&mc, 0);
		session_debug(DBG_ENABLE_MASK_DEL_HWSESSION,
			      "removed HW MC session:avm_pa_id=%d, res=%d\n",
			      p_item->session_id, res);
		p_item->routing_entry = ~0;

		mc.out_vlan_info.vlan_entry = p_item->out_vlan_entry;
		ifx_ppa_drv_del_outer_vlan_entry(&mc.out_vlan_info, 0);
		p_item->out_vlan_entry = ~0;

		if (p_item->src_mac_entry != NO_MAC_ENTRY) {
			PPE_ROUTE_MAC_INFO mac_info = { 0 };
			mac_info.mac_ix = p_item->src_mac_entry;
			ifx_ppa_drv_del_mac_entry(&mac_info, 0);
			p_item->src_mac_entry = NO_MAC_ENTRY;
		}

		p_item->flags &= ~SESSION_ADDED_IN_HW;
	}
}
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

static struct avm_hardware_pa_instance ifx_pa = {
	.add_session = avm_pa_add_ifx_session,
	.remove_session = avm_pa_remove_ifx_session,
	.change_session = avm_pa_change_ifx_session,
	.session_stats = avm_pa_ifx_session_stats,
	.name = "ifx_ppa",
};

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

void avm_pa_register_ifx_hw_ppa(void)
{
	avm_pa_multiplexer_register_instance(&ifx_pa);
	ifx_lookup_routing_entry = avm_session_handle_fast_lookup;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

void avm_pa_unregister_ifx_hw_ppa(void)
{
	avm_pa_multiplexer_unregister_instance(&ifx_pa);
	ifx_lookup_routing_entry = NULL;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

int __init ifx_ppa_mini_session_init(void)
{
	uint32_t sys_flag;
	size_t i;

	PPE_ACC_ENABLE acc_cfg = { 0 };
	PPE_FAST_MODE_CFG fast_mode = { 0 };
	PPA_MAX_ENTRY_INFO entry;

	if (ifx_ppa_drv_hal_init(0) != IFX_SUCCESS) {
		printk(KERN_ERR
		       "ifx_ppa_drv_hal_init failed, maybe you forgot to load ppa hal driver?!\n");
		return -1;
	}

#ifdef CONFIG_AVM_PA
	if (check_if_avmnet_enables_ppa()) {
		printk("Register LANTIQ_PA @ AVM_PA\n");
		avm_pa_register_ifx_hw_ppa();
	} else {
		printk("Don't register LANTIQ_PA @ AVM_PA\n");
	}
#endif

	printk(KERN_ERR "[%s]", __FUNCTION__);

	/* setup avm_pa sessionh lookup_table */
	printk(KERN_ERR "[%s] avm_pa sessionh_lookup table\n", __FUNCTION__);
	ifx_ppa_drv_get_max_entries(&entry, 0);

	printk(KERN_ERR "max_lan_entries       %d\n", entry.max_lan_entries);
	printk(KERN_ERR "max_wan_entries       %d\n", entry.max_wan_entries);
	printk(KERN_ERR "max_mc_entries        %d\n", entry.max_mc_entries);
	printk(KERN_ERR "max_bridging_entries  %d\n",
	       entry.max_bridging_entries);
	printk(KERN_ERR "max_ipv6_addr_entries %d\n",
	       entry.max_ipv6_addr_entries);
	printk(KERN_ERR "max_fw_queue          %d\n", entry.max_fw_queue);
	printk(KERN_ERR "max_6rd_entries       %d\n", entry.max_6rd_entries);

	avm_sessionh_table_lan_size = entry.max_lan_entries;
	avm_sessionh_table_wan_size = entry.max_wan_entries;
	avm_sessionh_table_mc_size = entry.max_mc_entries;

	avm_sessionh_table_lan = kmalloc(avm_sessionh_table_lan_size *
						 sizeof(avm_session_handle),
					 GFP_KERNEL);
	avm_sessionh_table_wan = kmalloc(avm_sessionh_table_wan_size *
						 sizeof(avm_session_handle),
					 GFP_KERNEL);
	avm_sessionh_table_mc =
		kmalloc(avm_sessionh_table_mc_size * sizeof(avm_session_handle),
			GFP_KERNEL);

	if (!avm_sessionh_table_lan || !avm_sessionh_table_wan ||
	    !avm_sessionh_table_mc) {
		printk(KERN_ERR
		       "cannot alloc lan/wan/mc-lookup table %d, %d  %d\n",
		       entry.max_lan_entries, entry.max_wan_entries,
		       entry.max_mc_entries);
		BUG();
	}
	memset(avm_sessionh_table_lan, INVALID_SESSION,
	       avm_sessionh_table_lan_size * sizeof(avm_session_handle));
	memset(avm_sessionh_table_wan, INVALID_SESSION,
	       avm_sessionh_table_wan_size * sizeof(avm_session_handle));
	memset(avm_sessionh_table_mc, INVALID_SESSION,
	       avm_sessionh_table_mc_size * sizeof(avm_session_handle));

	for (i = 0; i < CONFIG_AVM_PA_MAX_SESSION; i++) {
		memset(&session_list[i], 0, sizeof(struct session_list_item));
		atomic_set(&session_valid[i], 0);
		atomic_set(&session_active[i], 0);
	}

	sys_flag = ppa_disable_int();

	acc_cfg.f_is_lan = 1;
	acc_cfg.f_enable = IFX_PPA_ACC_MODE_ROUTING;
	ifx_ppa_drv_set_acc_mode(&acc_cfg, 0);

	acc_cfg.f_is_lan = 2;
	acc_cfg.f_enable = IFX_PPA_ACC_MODE_ROUTING;
	ifx_ppa_drv_set_acc_mode(&acc_cfg, 0);

	fast_mode.mode = IFX_PPA_SET_FAST_MODE_CPU1_INDIRECT |
			 IFX_PPA_SET_FAST_MODE_ETH1_DIRECT;
	fast_mode.flags =
		IFX_PPA_SET_FAST_MODE_CPU1 | IFX_PPA_SET_FAST_MODE_ETH1;
	if (ifx_ppa_drv_set_fast_mode(&fast_mode, 0) != IFX_SUCCESS)
		printk("ifx_ppa_drv_set_fast_mode fail\n");

	ppa_enable_int(sys_flag);

	avmnet_cfg_add_seq_procentry(avmnet_hw_config_entry->config,
				     PPE_SESSION_LIST_PROCFILE_NAME,
				     &read_session_list_fops);
	avmnet_cfg_add_seq_procentry(avmnet_hw_config_entry->config,
				     PPE_SESSIONS_BRIEF_PROCFILE_NAME,
				     &read_sessions_brief_fops);

	printk("AVM - Lantiq-PPA: minimalist and slim Session Management: --- init successful\n");
	printk("Session-Table Size: %d * %d byte = %d kb\n",
	       CONFIG_AVM_PA_MAX_SESSION, sizeof(struct session_list_item),
	       (CONFIG_AVM_PA_MAX_SESSION * sizeof(struct session_list_item)) /
		       1024);

	return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

void __exit ifx_ppa_mini_session_exit(void)
{
	if (check_if_avmnet_enables_ppa()) {
		avm_pa_unregister_ifx_hw_ppa();
		printk("Unregister LANTIQ_PA @ AVM_PA\n");
	}

	avmnet_cfg_remove_procentry(avmnet_hw_config_entry->config,
				    PPE_SESSION_LIST_PROCFILE_NAME);
	avmnet_cfg_remove_procentry(avmnet_hw_config_entry->config,
				    PPE_SESSIONS_BRIEF_PROCFILE_NAME);

	kfree(avm_sessionh_table_lan);
	avm_sessionh_table_lan = NULL;

	kfree(avm_sessionh_table_wan);
	avm_sessionh_table_wan = NULL;

	kfree(avm_sessionh_table_mc);
	avm_sessionh_table_mc = NULL;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
module_init(ifx_ppa_mini_session_init);
module_exit(ifx_ppa_mini_session_exit);

MODULE_LICENSE("GPL");
