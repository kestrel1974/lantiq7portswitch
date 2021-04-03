#ifndef _IFXMIPS_PPA_DATAPATH_COMMON
#define _IFXMIPS_PPA_DATAPATH_COMMON

#define ENABLE_THIS_PROC_FILE 1

#if __has_include(<avm/sammel/simple_proc.h>)
#include <avm/sammel/simple_proc.h>
#else
#include <linux/simple_proc.h>
#endif

/*------------------------------------------------------------------------------------------*\
 * Constraint Checks
\*------------------------------------------------------------------------------------------*/
#if !( defined(PTM_DATAPATH) || defined(ATM_DATAPATH))
#error //Datapath has to be defined for this header
#endif

#if ( defined(CONFIG_AR10) || defined(CONFIG_VR9))
#define DATAPATH_7PORT
#elif ( defined(CONFIG_AR9) )
#define DATAPATH_3PORT
#else
#error //Architecture has to be defined for this header
#endif



#if defined(CONFIG_IFX_PPA_DIRECTPATH_TX_QUEUE_SIZE)
#error //CONFIG_IFX_PPA_DIRECTPATH_TX_QUEUE_SIZE not supported by AVM
#endif
#if defined(CONFIG_IFX_PPA_DIRECTPATH_TX_QUEUE_PKTS)
#error //CONFIG_IFX_PPA_DIRECTPATH_TX_QUEUE_PKTS not supported by AVM
#endif
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

/*
 *  Proc File Functions
 */
static INLINE void proc_file_create(void);
static INLINE void proc_file_delete(void);

#if defined(PTM_DATAPATH)
#define PTM_DEFAULT_DEV 0
#define SB_BUFFER(x) SB_BUFFER_BOND(2, x)
static int proc_status_input(char *line, void *data __maybe_unused);
static void proc_status_output(struct seq_file *m, void *private_data __maybe_unused);
static int proc_wanmib_input(char *line, void *data __maybe_unused);
static void proc_wanmib_output(struct seq_file *m, void *private_data __maybe_unused);
#endif // #if defined(PTM_DATAPATH)

static void proc_ver_output(struct seq_file *, void *);
#if defined(DEBUG_FIRMWARE_TABLES_PROC) && DEBUG_FIRMWARE_TABLES_PROC
#if defined(PROC_WRITE_ROUTE) && PROC_WRITE_ROUTE
static void print_route( struct seq_file *m, int i, int f_is_lan, struct rout_forward_compare_tbl *pcompare, struct rout_forward_action_tbl *pwaction);
static int proc_route_input(char *line, void *data);
static void proc_route_output(struct seq_file *, void *);
#endif //#if defined(PROC_WRITE_ROUTE) && PROC_WRITE_ROUTE
static int proc_genconf_input(char *line, void *data);
static void proc_genconf_output(struct seq_file *, void *);
#if defined(ATM_DATAPATH)
static void proc_queue_output(struct seq_file *, void *);
#endif
static void proc_pppoe_output(struct seq_file *, void *);
static void proc_mtu_output(struct seq_file *, void *);
static int proc_hit_input(char *line, void *data);
static void proc_hit_output(struct seq_file *, void *);
static void proc_mac_output(struct seq_file *, void *);
static void proc_out_vlan_output(struct seq_file *, void *);
static void proc_ipv6_ip_output(struct seq_file *, void *);
#if defined(ATM_DATAPATH)
static int proc_mpoa_input(char *line, void *data);
#endif // #if defined(ATM_DATAPATH)
static void proc_mc_output(struct seq_file *, void *);
#endif // #if defined(PROC_WRITE_ROUTE) && PROC_WRITE_ROUTE

#if defined(DEBUG_HTU_PROC) && DEBUG_HTU_PROC
  static void proc_htu_output(struct seq_file *, void *);
#endif

static int proc_burstlen_input(char *line, void *data);
static void proc_burstlen_output(struct seq_file *, void *);

#if defined(ATM_DATAPATH)
static int proc_mib_input(char *line, void *data);
static void proc_mib_output(struct seq_file *, void *);
static void clear_mib_counter(unsigned int mib_itf );
#endif

#if defined(DEBUG_FW_PROC) && DEBUG_FW_PROC
static int proc_fw_input(char *line, void *data);
static void proc_fw_output(struct seq_file *, void *);
static int proc_fwdbg_input(char *line, void *data);
static void fw_dbg_start(char *cmdbuf);
static void fwdbg_help(char **tokens, unsigned int token_num);
static void dump_ipv4_hdr(struct iphdr * iph);
static void dump_ipv6_hdr(struct ipv6hdr *iph6);
#endif

#if defined(DEBUG_SEND_PROC) && DEBUG_SEND_PROC
static int proc_send_input(char *line, void *data);
static void proc_send_output(struct seq_file *, void *);
#endif // #if defined(DEBUG_SEND_PROC) && DEBUG_SEND_PROC

static INLINE unsigned int sw_get_rmon_counter(int, int);
static INLINE void sw_clr_rmon_counter(int);

static int proc_port_mib_input(char *line, void *data);
static void proc_port_mib_output(struct seq_file *, void *);

static int proc_directforwarding_input(char *line, void *data);
static void proc_directforwarding_output(struct seq_file *, void *);

static int proc_clk_input(char *line, void *data);
static void proc_clk_output(struct seq_file *, void *);

static void proc_tx_dma_descr_output(struct seq_file *, void *);

static int proc_prio_input(char *line, void *data);
static void proc_prio_output(struct seq_file *, void *);

static int proc_ewan_input(char *line, void *data);
static void proc_ewan_output(struct seq_file *, void *);

static void proc_qos_output(struct seq_file *, void *);

#if defined(ENABLE_CONFIGURABLE_DSL_VLAN) && ENABLE_CONFIGURABLE_DSL_VLAN
static int proc_dsl_vlan_input(char *line, void *data);
static void proc_dsl_vlan_output(struct seq_file *, void *);

static INLINE int ppe_set_vlan(int, unsigned short);
static INLINE int sw_set_vlan_bypass(int, unsigned int, int);
#endif // #if defined(ENABLE_CONFIGURABLE_DSL_VLAN) && ENABLE_CONFIGURABLE_DSL_VLAN

#if defined(DEBUG_MIRROR_PROC) && DEBUG_MIRROR_PROC
static int proc_mirror_input(char *line, void *data);
static void proc_mirror_output(struct seq_file *, void *);
#endif

#if defined(PTM_DATAPATH)
static void proc_gamma_output(struct seq_file *m, void *private_data __maybe_unused);
#endif

static int proc_class_input(char *line, void *data);
static void proc_class_output(struct seq_file *, void *);

static int proc_mccfg_input(char *line, void *data);
static void proc_mccfg_output(struct seq_file *, void *);

#if defined(CONFIG_VR9)
static int proc_powersaving_input(char *line, void *data __maybe_unused);
static void proc_powersaving_output(struct seq_file *m, void *private_data __maybe_unused);
#endif

static void proc_dsl_rx_bytes_output(struct seq_file *m, void *private_data __maybe_unused);
static void proc_dsl_tx_bytes_output(struct seq_file *m, void *private_data __maybe_unused);

static void proc_dsl_rx_pdus_output(struct seq_file *m, void *private_data __maybe_unused);
static void proc_dsl_tx_pdus_output(struct seq_file *m, void *private_data __maybe_unused);
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

/*
 *  Proc File help functions
 */
#if defined(DEBUG_FIRMWARE_TABLES_PROC) && DEBUG_FIRMWARE_TABLES_PROC
static INLINE void get_ip_port(char **, int *, unsigned int *);
static INLINE void get_mac(char **, int *, unsigned int *);
static INLINE char *get_wanitf(int);
#if defined(RTP_SAMPLING_ENABLE) && RTP_SAMPLING_ENABLE
static INLINE int print_mc(struct seq_file *, int i, struct wan_rout_multicast_cmp_tbl *, struct wan_rout_multicast_act_tbl *, struct rtp_sampling_cnt *,uint32_t, uint32_t);
#else
static INLINE int print_mc(struct seq_file *, int i, struct wan_rout_multicast_cmp_tbl *, struct wan_rout_multicast_act_tbl *, uint32_t , uint32_t );
#endif // #if defined(RTP_SAMPLING_ENABLE) && RTP_SAMPLING_ENABLE
#endif //#if defined(DEBUG_FIRMWARE_TABLES_PROC) && DEBUG_FIRMWARE_TABLES_PROC
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

/*
 *  Diverse shared datapath functions
 */
static int avm_get_eth_qid(unsigned int mac_nr, unsigned int skb_priority);
#if defined(ATM_DATAPATH)
static int avm_get_atm_qid(struct atm_vcc *vcc, unsigned int prio);
#endif
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

/*
 *  Buffer Management
 */
static struct sk_buff* skb_break_away_from_protocol_avm(struct sk_buff *);

static INLINE void __get_skb_from_dbg_pool(struct sk_buff *, const char *, unsigned int);
#define get_skb_from_dbg_pool(skb)  __get_skb_from_dbg_pool(skb, __FUNCTION__, __LINE__)

static INLINE void __free_skb_clear_dataptr( unsigned int dataptr, const char *func_name, unsigned int line_num);

#define free_skb_clear_dataptr(descr)   \
	do { \
		smp_mb(); \
		__free_skb_clear_dataptr((descr)->dataptr, __FUNCTION__, __LINE__); \
	} while(0)

#define free_skb_clear_dataptr_shift(descr, shift)    \
	do { \
		smp_mb(); \
		__free_skb_clear_dataptr((((descr)->dataptr) << (shift)), __FUNCTION__, __LINE__); \
	} while(0)


static INLINE void __put_skb_to_dbg_pool(struct sk_buff *, const char *, unsigned int);
#define put_skb_to_dbg_pool(skb)    __put_skb_to_dbg_pool(skb, __FUNCTION__, __LINE__)

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
/*
 *  Diverse shared data
 */
static int g_eth_prio_queue_map[8];
static int g_wan_queue_class_map[8];    //  So far, this table is shared by all interfaces.
#if defined(DATAPATH_7PORT)
static unsigned char mib_port_array[] = {0,1,2,3,4,5,6};
#endif
#if defined(ATM_DATAPATH)
static unsigned int g_mib_itf = 0xC3;
#endif

static struct simple_proc_file simple_proc_array[] = {
#if defined(PTM_DATAPATH)
   { "dsl_tc/status", proc_status_input, proc_status_output, NULL,  ENABLE_THIS_PROC_FILE },
   { "eth/wanmib", proc_wanmib_input, proc_wanmib_output, NULL,  ENABLE_THIS_PROC_FILE },
#endif
#if defined(ATM_DATAPATH)
   { "eth/mib",proc_mib_input, proc_mib_output, NULL, ENABLE_THIS_PROC_FILE },
#endif
   { "eth/ver", NULL, proc_ver_output, NULL, ENABLE_THIS_PROC_FILE },
#if defined(DEBUG_FIRMWARE_TABLES_PROC) && DEBUG_FIRMWARE_TABLES_PROC
#if defined(PROC_WRITE_ROUTE) && PROC_WRITE_ROUTE
   { "eth/route", proc_route_input, proc_route_output, NULL, ENABLE_THIS_PROC_FILE },
#endif // #if defined(PROC_WRITE_ROUTE) && PROC_WRITE_ROUTE

#if defined(ATM_DATAPATH)
   { "eth/mpoa", proc_mpoa_input, NULL, NULL, ENABLE_THIS_PROC_FILE },
#endif // #if defined(ATM_DATAPATH)
   { "eth/mc", NULL, proc_mc_output, NULL, ENABLE_THIS_PROC_FILE },
   { "eth/genconf", proc_genconf_input, proc_genconf_output, NULL, ENABLE_THIS_PROC_FILE },
#if defined(ATM_DATAPATH) && ATM_DATAPATH
   { "eth/queue", NULL, proc_queue_output, NULL, ENABLE_THIS_PROC_FILE },
#endif
   { "eth/pppoe", NULL, proc_pppoe_output, NULL, ENABLE_THIS_PROC_FILE },
   { "eth/mtu", NULL, proc_mtu_output, NULL, ENABLE_THIS_PROC_FILE },
   { "eth/hit", proc_hit_input, proc_hit_output, NULL, ENABLE_THIS_PROC_FILE },
   { "eth/mac", NULL, proc_mac_output, NULL, ENABLE_THIS_PROC_FILE },
   { "eth/out_vlan", NULL, proc_out_vlan_output, NULL, ENABLE_THIS_PROC_FILE },
   { "eth/ipv6_ip", NULL, proc_ipv6_ip_output, NULL, ENABLE_THIS_PROC_FILE },
#endif // #if defined(DEBUG_FIRMWARE_TABLES_PROC) && DEBUG_FIRMWARE_TABLES_PROC
#if defined(DEBUG_HTU_PROC) && DEBUG_HTU_PROC
   { "eth/htu", NULL, proc_htu_output, NULL, ENABLE_THIS_PROC_FILE },
#endif // #if defined(DEBUG_HTU_PROC) && DEBUG_HTU_PROC
   { "eth/burstlen", proc_burstlen_input, proc_burstlen_output, NULL, ENABLE_THIS_PROC_FILE },

#if defined(DEBUG_FW_PROC) && DEBUG_FW_PROC
   { "eth/fw", proc_fw_input, proc_fw_output, NULL, ENABLE_THIS_PROC_FILE },
   { "eth/tfwdbg", proc_fwdbg_input, NULL, NULL, ENABLE_THIS_PROC_FILE },
#endif

#if defined(DEBUG_SEND_PROC) && DEBUG_SEND_PROC
   { "eth/send", proc_send_input, proc_send_output, NULL, ENABLE_THIS_PROC_FILE },
#endif
#if defined(DATAPATH_7PORT)
   { "eth/port0mib", proc_port_mib_input, proc_port_mib_output, &mib_port_array[0], ENABLE_THIS_PROC_FILE },
   { "eth/port1mib", proc_port_mib_input, proc_port_mib_output, &mib_port_array[1], ENABLE_THIS_PROC_FILE },
   { "eth/port2mib", proc_port_mib_input, proc_port_mib_output, &mib_port_array[2], ENABLE_THIS_PROC_FILE },
   { "eth/port3mib", proc_port_mib_input, proc_port_mib_output, &mib_port_array[3], ENABLE_THIS_PROC_FILE },
   { "eth/port4mib", proc_port_mib_input, proc_port_mib_output, &mib_port_array[4], ENABLE_THIS_PROC_FILE },
   { "eth/port5mib", proc_port_mib_input, proc_port_mib_output, &mib_port_array[5], ENABLE_THIS_PROC_FILE },
   { "eth/port6mib", proc_port_mib_input, proc_port_mib_output, &mib_port_array[6], ENABLE_THIS_PROC_FILE },
#endif // #if defined(DATAPATH_7PORT)

   { "eth/direct_forwarding", proc_directforwarding_input, proc_directforwarding_output, NULL, ENABLE_THIS_PROC_FILE },
   { "eth/clk", proc_clk_input, proc_clk_output, NULL, ENABLE_THIS_PROC_FILE },
   { "eth/qos_tx_dma_descr", NULL, proc_tx_dma_descr_output, NULL, ENABLE_THIS_PROC_FILE },

   { "eth/prio", proc_prio_input, proc_prio_output, NULL, ENABLE_THIS_PROC_FILE },
#if defined(DATAPATH_7PORT)
   { "eth/ewan", proc_ewan_input, proc_ewan_output, NULL, ENABLE_THIS_PROC_FILE },
#endif // #if defined(DATAPATH_7PORT)
   { "eth/qos", NULL, proc_qos_output, NULL, ENABLE_THIS_PROC_FILE },
#if defined(ENABLE_CONFIGURABLE_DSL_VLAN) && ENABLE_CONFIGURABLE_DSL_VLAN
   { "eth/dsl_vlan", proc_dsl_vlan_input, proc_dsl_vlan_output, NULL, ENABLE_THIS_PROC_FILE },
#endif
#if defined(PTM_DATAPATH)
   { "eth/gamma", NULL, proc_gamma_output, NULL, ENABLE_THIS_PROC_FILE },
#endif
#if defined(DEBUG_MIRROR_PROC) && DEBUG_MIRROR_PROC
   { "eth/mirror", proc_mirror_input, proc_mirror_output, NULL, ENABLE_THIS_PROC_FILE },
#endif
   { "eth/class", proc_class_input, proc_class_output, NULL, ENABLE_THIS_PROC_FILE },
   { "eth/mccfg", proc_mccfg_input, proc_mccfg_output, NULL, ENABLE_THIS_PROC_FILE },

#if defined(CONFIG_IFX_PPE_E5_OFFCHIP_BONDING) && defined(PTM_DATAPATH)
   { "eth/dev", proc_dev_input, proc_dev_output, NULL, ENABLE_THIS_PROC_FILE },
#endif

#if defined(DEBUG_BONDING_PROC) && DEBUG_BONDING_PROC
   { "eth/bonding", proc_bonding_input, proc_bonding_output, NULL, ENABLE_THIS_PROC_FILE },
#endif

#if defined(CONFIG_VR9)
   { "eth/powersaving", proc_powersaving_input, proc_powersaving_output, NULL, ENABLE_THIS_PROC_FILE },
#endif

   { "eth/dsl_rx_bytes", NULL, proc_dsl_rx_bytes_output, NULL, ENABLE_THIS_PROC_FILE },
   { "eth/dsl_tx_bytes", NULL, proc_dsl_tx_bytes_output, NULL, ENABLE_THIS_PROC_FILE },
   { "eth/dsl_rx_pdus", NULL, proc_dsl_rx_pdus_output, NULL, ENABLE_THIS_PROC_FILE },
   { "eth/dsl_tx_pdus", NULL, proc_dsl_tx_pdus_output, NULL, ENABLE_THIS_PROC_FILE },
   {}
};


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static void proc_file_create(void) {
   int i;

#if defined(PTM_DATAPATH)
   proc_mkdir("dsl_tc", NULL);
#endif

   //Enable/Disable proc
   for(i = 0; simple_proc_array[i].path != NULL; i++){

      if(g_eth_wan_mode && !g_wanqos_en && strcmp(simple_proc_array[i].path, "eth/qos") == 0){
         simple_proc_array[i].enabled = 0;
      }
      else if(!g_ipv6_acc_en && strcmp(simple_proc_array[i].path, "eth/ipv6_ip") == 0){
         simple_proc_array[i].enabled = 0;
      }
      else if(g_eth_wan_mode && strcmp(simple_proc_array[i].path, "eth/gamma") == 0){
         simple_proc_array[i].enabled = 0;
      }
#ifdef CONFIG_IFX_PPE_E5_OFFCHIP_BONDING
      else if(g_dsl_bonding != 2 && strcmp(simple_proc_array[i].path, "eth/dev") == 0){
         simple_proc_array[i].enabled = 0;
      }
#endif
   }

   add_simple_proc_file_array( simple_proc_array );
}

static INLINE void proc_file_delete(void) {

	remove_simple_proc_file_array( simple_proc_array );
#if defined(PTM_DATAPATH)
	remove_proc_entry("dsl_tc", NULL);
#endif

}
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

static int print_fw_ver(char *buf, int buf_len) {
    static char * fw_ver_family_str[] = {
        "reserved - 0",
        "Danube",
        "Twinpass",
        "Amazon-SE",
        "reserved - 4",
        "AR9",
        "GR9",
        "VR9",
        "AR10",
        "VRX218",
    };

    static char * fw_ver_package_str[] = {
        "Reserved - 0",
        "A1",
        "B1 - PTM_BONDING",
        "E1",
        "A5",
        "D5",
        "D5v2",
        "E5",
    };

	static char *fw_feature[] = {
		"  ATM/PTM TC-Layer                Support",
		"  ATM/PTM TC-Layer Retransmission Support",
		"  ATM/PTM TC-Layer Bonding        Support",
		"  L2 Trunking                     Support",
		"  Packet Acceleration             Support",
		"  IPv4                            Support",
		"  IPv6                            Support",
		"  6RD                             Support",
		"  DS-Lite                         Support",
		"  Unified FW QoS                  Support",

	};

    int len = 0;
    unsigned int i;

    len += snprintf(buf + len, buf_len - len, "PPE firmware info:\n");
    len += snprintf(buf + len, buf_len - len,     "  Version ID: %d.%d.%d.%d.%d\n", (int)FW_VER_ID->family, (int)FW_VER_ID->package, (int)FW_VER_ID->major, (int)FW_VER_ID->middle, (int)FW_VER_ID->minor);
    if ( FW_VER_ID->family >= NUM_ENTITY(fw_ver_family_str) )
        len += snprintf(buf + len, buf_len - len, "  Family    : reserved - %d\n", (int)FW_VER_ID->family);
    else
        len += snprintf(buf + len, buf_len - len, "  Family    : %s\n", fw_ver_family_str[FW_VER_ID->family]);

    if ( FW_VER_ID->package >= NUM_ENTITY(fw_ver_package_str) )
        len += snprintf(buf + len, buf_len - len, "  FW Package: reserved - %d\n", (int)FW_VER_ID->package);
    else
        len += snprintf(buf + len, buf_len - len, "  FW Package: %s\n", fw_ver_package_str[FW_VER_ID->package]);

    len += snprintf(buf + len, buf_len - len,     "  Release   : %u.%u.%u\n", (int)FW_VER_ID->major, (int)FW_VER_ID->middle, (int)FW_VER_ID->minor);

    len += snprintf(buf + len, buf_len - len, "PPE firmware feature:\n");

    for(i = 0; i < NUM_ENTITY(fw_feature); i ++){
        if(*FW_VER_FEATURE & (1 << (31-i))){
            len += snprintf(buf + len, buf_len - len, "%s\n", fw_feature[i]);
        }
    }

    return len;
}


static int print_driver_ver(char *buf, int buf_len, char *title, unsigned int family, unsigned int type, unsigned int itf, unsigned int mode, unsigned int major, unsigned int mid, unsigned int minor)
{
    static const char * dr_ver_family_str[] = {
        NULL,
        "Danube",
        "Twinpass",
        "Amazon-SE",
        NULL,
        "AR9",
        "VR9",
        "AR10",
        NULL,
    };
    static const char * dr_ver_type_str[] = {
        "Normal Data Path",
        "Indirect-Fast Path",
        "HAL",
        "Hook",
        "OS Adatpion Layer",
        "PPA API"
    };
    static const char * dr_ver_interface_str[] = {
        "MII0",
        "MII1",
        "ATM",
        "PTM"
    };
    static const char * dr_ver_accmode_str[] = {
        "Routing",
        "Bridging",
    };

    int len = 0;
    unsigned int bit;
    unsigned int i, j;

    len += snprintf(buf + len, buf_len - len, "%s:\n", title);
    len += snprintf(buf + len, buf_len - len, "  Version ID: %d.%d.%d.%d.%d.%d.%d\n", family, type, itf, mode, major, mid, minor);
    len += snprintf(buf + len, buf_len - len, "  Family    : ");
    for ( bit = family, i = j = 0; bit != 0 && i < NUM_ENTITY(dr_ver_family_str); bit >>= 1, i++ )
        if ( (bit & 0x01) && dr_ver_family_str[i] != NULL )
        {
            if ( j )
                len += snprintf(buf + len, buf_len - len, " | %s", dr_ver_family_str[i]);
            else
                len += snprintf(buf + len, buf_len - len, "%s", dr_ver_family_str[i]);
            j++;
        }
    if ( j )
        len += snprintf(buf + len, buf_len - len, "\n");
    else
        len += snprintf(buf + len, buf_len - len, "N/A\n");
    len += snprintf(buf + len, buf_len - len, "  DR Type   : ");
    for ( bit = type, i = j = 0; bit != 0 && i < NUM_ENTITY(dr_ver_type_str); bit >>= 1, i++ )
        if ( (bit & 0x01) && dr_ver_type_str[i] != NULL )
        {
            if ( j )
                len += snprintf(buf + len, buf_len - len, " | %s", dr_ver_type_str[i]);
            else
                len += snprintf(buf + len, buf_len - len, "%s", dr_ver_type_str[i]);
            j++;
        }
    if ( j )
        len += snprintf(buf + len, buf_len - len, "\n");
    else
        len += snprintf(buf + len, buf_len - len, "N/A\n");
    len += snprintf(buf + len, buf_len - len, "  Interface : ");
    for ( bit = itf, i = j = 0; bit != 0 && i < NUM_ENTITY(dr_ver_interface_str); bit >>= 1, i++ )
        if ( (bit & 0x01) && dr_ver_interface_str[i] != NULL )
        {
            if ( j )
                len += snprintf(buf + len, buf_len - len, " | %s", dr_ver_interface_str[i]);
            else
                len += snprintf(buf + len, buf_len - len, "%s",  dr_ver_interface_str[i]);
            j++;
        }
    if ( j )
        len += snprintf(buf + len, buf_len - len, "\n");
    else
        len += snprintf(buf + len, buf_len - len, "N/A\n");
    len += snprintf(buf + len, buf_len - len, "  Mode      : ");
    for ( bit = mode, i = j = 0; bit != 0 && i < NUM_ENTITY(dr_ver_accmode_str); bit >>= 1, i++ )
        if ( (bit & 0x01) && dr_ver_accmode_str[i] != NULL )
        {
            if ( j )
                len += snprintf(buf + len, buf_len - len, " | %s", dr_ver_accmode_str[i]);
            else
                len += snprintf(buf + len, buf_len - len, "%s",  dr_ver_accmode_str[i]);
            j++;
        }
    if ( j )
        len += snprintf(buf + len, buf_len - len, "\n");
    else
        len += snprintf(buf + len, buf_len - len, "N/A\n");
    len += snprintf(buf + len, buf_len - len, "  Release   : %d.%d.%d\n", major, mid, minor);

    return len;
}

static void print_dsl_dma_desc(struct seq_file *m, volatile u32 *desc_base, int desc_pos) {
    u32 *p;

    p = (u32*)desc_base;
    if ( desc_pos == 0 ) {
        seq_printf(m,"DSL Tx descriptor list:\n" );

        seq_printf(m," no  address        data pointer command bits (Own, Complete, SoP, EoP, Offset) \n");
        seq_printf(m,"---------------------------------------------------------------------------------\n");
    }
    seq_printf(m,"%3d  ",desc_pos);
    seq_printf(m,"0x%08x     ", (u32)(p + (desc_pos * 2)));
    seq_printf(m,"%08x     ", *(p + (desc_pos * 2 + 1)));
    seq_printf(m,"%08x     ", *(p + (desc_pos * 2)));

    if(*(p + (desc_pos * 2)) & 0x80000000)
        seq_printf(m, "C "); // ist bei A5 Treiber invers: 1 == CPU
    else
    	seq_printf(m, "D "); // ist bei A5 Treiber invers: 0 == DMA
    if(*(p + (desc_pos * 2)) & 0x40000000)
        seq_printf(m, "C ");
    else
        seq_printf(m, "c ");
    if(*(p + (desc_pos * 2)) & 0x20000000)
        seq_printf(m, "S ");
    else
        seq_printf(m, "s ");
    if(*(p + (desc_pos * 2)) & 0x10000000)
        seq_printf(m, "E ");
    else
        seq_printf(m, "e ");
    seq_printf(m,"%02x ", (*(p + (desc_pos * 2)) & 0x0F800000) >> 23);
    if( g_cpu_to_wan_tx_desc_pos  == desc_pos )
        seq_printf(m,"<- CURR");
    seq_printf(m,"\n");
}

static void proc_tx_dma_descr_output(struct seq_file *m, void *private_data __maybe_unused){

    int j;
    __sync();

    for (j = 0; j < CPU_TO_WAN_TX_DESC_NUM; j++) {
        print_dsl_dma_desc(m,  (volatile u32 *)CPU_TO_WAN_TX_DESC_BASE, j);
    }
}


static void proc_ver_output(struct seq_file *m, void *u __maybe_unused)
{
	char *page;
	size_t count = seq_get_buf(m, &page);
	int len = 0;

	len += print_driver_ver(page + len, count - len,
			"PPE datapath driver info", VER_FAMILY, VER_DRTYPE, VER_INTERFACE,
			VER_ACCMODE, VER_MAJOR, VER_MID, VER_MINOR);
	len += print_fw_ver(page + len, count - len);

	seq_commit(m, len);
}

#if defined(ATM_DATAPATH)
static void proc_mib_output(struct seq_file *m, void *u __maybe_unused) {

   char *itf_name[] = {"eth0", "eth1", "cpu", "ext_int1", "ext_int2", "ext_int3", "", "atm"};
   char *row_name[] = {"ig_fast_brg_pkts", "ig_fast_brg_bytes", "ig_fast_rt_ipv4_udp_pkts",
                        "ig_fast_rt_ipv4_tcp_pkts", "ig_fast_rt_ipv4_mc_pkts", "ig_fast_rt_ipv4_bytes",
                        "ig_fast_rt_ipv6_udp_pkts", "ig_fast_rt_ipv6_tcp_pkts", "ig_fast_rt_ipv6_mc_pkts",
                        "ig_fast_rt_ipv6_bytes", NULL, "ig_cpu_pkts", "ig_cpu_bytes", "ig_drop_pkts",
                        "ig_drop_bytes", "eg_fast_pkts"};
   unsigned int i, j, k, h;

	seq_puts(m, "Ethernet\n");

	seq_puts(m, "  Firmware  (");
	for (i = h = 0; i < NUM_ENTITY(itf_name); i++)
		if ((g_mib_itf & (1 << i)))
			seq_printf(m, h++ == 0 ? "%s" : ", %s", itf_name[i]);
	seq_puts(m, ")\n");

	for (i = k = 0; i < NUM_ENTITY(row_name); i++) {
		if (!row_name[i])
			continue;

		j = strlen(row_name[i]);
		k = (k < j) ? j : k;
	}

	k += 6;

	for (i = 0; i < NUM_ENTITY(row_name); i++) {
		if (row_name[i] == NULL)
			continue;

		j = seq_printf(m, "    %s: ", row_name[i]);

		for (j = k - j; j > 0; j--)
			seq_puts(m, " ");

		for (j = h = 0; j < 8; j++)
			if ((g_mib_itf & (1 << j)))
				seq_printf(m, h++ == 0 ? "%10u" : ", %10u",
					   ((volatile unsigned *)ITF_MIB_TBL(j))[i]);
		seq_printf(m, "\n");
	}

	seq_printf(m, "DSL WAN MIB\n");
	seq_printf(m, "  wrx_drophtu_cell: %10u\n", DSL_WAN_MIB_TABLE->wrx_drophtu_cell);
	seq_printf(m, "  wrx_dropdes_pdu:  %10u\n", DSL_WAN_MIB_TABLE->wrx_dropdes_pdu);
	seq_printf(m, "  wrx_correct_pdu:  %10u\n", DSL_WAN_MIB_TABLE->wrx_correct_pdu);
	seq_printf(m, "  wrx_err_pdu:      %10u\n", DSL_WAN_MIB_TABLE->wrx_err_pdu);
	seq_printf(m, "  wrx_dropdes_cell: %10u\n", DSL_WAN_MIB_TABLE->wrx_dropdes_cell);
	seq_printf(m, "  wrx_correct_oam_cell: %10u\n", DSL_WAN_MIB_TABLE->wrx_correct_cell);
	seq_printf(m, "  wrx_err_cell:     %10u\n", DSL_WAN_MIB_TABLE->wrx_err_cell);
	seq_printf(m, "  wrx_total_byte:   %10u\n", DSL_WAN_MIB_TABLE->wrx_total_byte);
	seq_printf(m, "  wtx_total_oam_cell:   %10u\n", DSL_WAN_MIB_TABLE->wtx_total_cell);
	seq_printf(m, "  wtx_total_cell:   %10u\n", DSL_WAN_MIB_TABLE->wtx_total_cell);
	seq_printf(m, "  wtx_total_byte:   %10u\n", DSL_WAN_MIB_TABLE->wtx_total_byte);

	seq_printf(m, "dma_alignment_atm_good_count: %d\n", dma_alignment_atm_good_count);
	seq_printf(m, "dma_alignment_atm_bad_count: %d\n", dma_alignment_atm_bad_count);
	seq_printf(m, "dma_alignment_eth_good_count: %d\n", dma_alignment_eth_good_count);
	seq_printf(m, "dma_alignment_eth_bad_count: %d\n", dma_alignment_eth_bad_count);
	seq_printf(m, "oam_cell_drop_count: %u\n", oam_cell_drop_count);
#if (defined(CONFIG_VR9) || defined(CONFIG_AR10))
	seq_printf(m, "padded_frame_count : %d\n", padded_frame_count);
#endif

}

static void clear_mib_counter(unsigned int mib_itf ) {
    int i;

    if ( mib_itf == 0x80000000 )
        mib_itf = 0x800000FF;
    if ( (mib_itf & 0x01) ) {
        printk(KERN_ERR "avm: we don't support clear eth mib counter\n");
    }
    if ( (mib_itf & 0x02) ) {

        printk(KERN_ERR "avm: we don't support clear eth mib counter\n");
    }
    if ( (mib_itf & 0xC0) )
    {
        memset(&g_atm_priv_data.wrx_total_byte, 0, (u32)&g_atm_priv_data.prev_mib - (u32)&g_atm_priv_data.wrx_total_byte);
        for ( i = 0; i < ATM_QUEUE_NUMBER; i++ )
		memset((void *)&g_atm_priv_data.connection[i].rx_packets, 0,
		       (u32)&g_atm_priv_data.connection[i].port -
		       (u32)&g_atm_priv_data.connection[i].rx_packets);
        memset((void *)DSL_WAN_MIB_TABLE, 0, sizeof(struct dsl_wan_mib_table));
        memset((void *)DSL_QUEUE_RX_MIB_TABLE(0), 0, sizeof(*DSL_QUEUE_RX_MIB_TABLE(0)) * 16);
        memset((void *)DSL_QUEUE_TX_MIB_TABLE(0), 0, sizeof(*DSL_QUEUE_TX_MIB_TABLE(0)) * 16);
        memset((void *)DSL_QUEUE_TX_DROP_MIB_TABLE(0), 0, sizeof(*DSL_QUEUE_TX_DROP_MIB_TABLE(0)) * 16);
        for ( i = 0; i < ATM_QUEUE_NUMBER; i++ )
        {
		memset((void *)g_atm_priv_data.connection[i].prio_tx_packets,
		       0, 8 * sizeof(u32));
        }
    }
    for ( i = 0; i < 8; i++ )
	{
        if ( (mib_itf & (1 << i)) )
        {
            void *ptmp = (void *)ITF_MIB_TBL(i);
            memset(ptmp, 0, sizeof(struct itf_mib));
        }
	}
}



static int proc_mib_input(char *line, void *data __maybe_unused) {
    char *p1, *p2;
    int len;
    int colon;
    // char local_buf[1024];

    unsigned int mib_itf = 0;   //  clear - 80h, on - 40h, off - 20h, queue - 10h, pvc - 08h
    char *itf_name[] = {"eth0", "eth1", "cpu", "ext_int1", "ext_int2", "ext_int3", "", "atm"};
    unsigned int i;

    len = strlen(line);
    p1 = line;
    p2 = NULL;
    colon = 1;
    while ( ifx_get_token(&p1, &p2, &len, &colon) )
    {
        if ( ifx_stricmp(p1, "queue") == 0 ) {
            printk("QId:     rx_pdu   rx_bytes     tx_pdu   tx_bytes tx_dropped_pdu\n");
            for ( i = 0; i < 16; i++ )
                printk(" %2d: %10u %10u %10u %10u %10u\n", i,
                        DSL_QUEUE_RX_MIB_TABLE(i)->pdu, DSL_QUEUE_RX_MIB_TABLE(i)->bytes,
                        DSL_QUEUE_TX_MIB_TABLE(i)->pdu, DSL_QUEUE_TX_MIB_TABLE(i)->bytes,
                        DSL_QUEUE_TX_DROP_MIB_TABLE(i)->pdu);
            mib_itf = 0x10000000;
        }
        else if ( ifx_stricmp(p1, "pvc") == 0 ) {
            u32 vpi, vci;
            int conn;

            ifx_ignore_space(&p2, &len);
            vpi = ifx_get_number(&p2, &len, 0);
            ifx_ignore_space(&p2, &len);
            vci = ifx_get_number(&p2, &len, 0);

            conn = find_vpivci(vpi, vci);

            printk("vpi = %u, vci = %u, conn = %d\n", vpi, vci, conn);

            if ( conn >= 0 )
            {
                u32 sw_tx_queue_table;
                int sw_tx_queue;
                unsigned int aal5VccTxPDU = 0, aal5VccTxBytes = 0, aal5VccTxDroppedPDU = 0;

                printk("driver:\n");
                printk("  rx_packets    = %u\n", g_atm_priv_data.connection[conn].rx_packets);
                printk("  rx_bytes      = %u\n", g_atm_priv_data.connection[conn].rx_bytes);
                printk("  rx_errors     = %u\n", g_atm_priv_data.connection[conn].rx_errors);
                printk("  rx_dropped    = %u\n", g_atm_priv_data.connection[conn].rx_dropped);
                printk("  tx_packets    = %u\n", g_atm_priv_data.connection[conn].tx_packets);
                printk("  tx_bytes      = %u\n", g_atm_priv_data.connection[conn].tx_bytes);
                printk("  tx_errors     = %u\n", g_atm_priv_data.connection[conn].tx_errors);
                printk("  tx_dropped    = %u\n", g_atm_priv_data.connection[conn].tx_dropped);
                printk("  prio_tx_packets:");
                for ( i = 0; i < 8; i++ )
                    printk(" %u", g_atm_priv_data.connection[conn].prio_tx_packets[i]);
                printk("\n");
                printk("firmware:\n");
                printk("  rx_pdu        = %u\n", DSL_QUEUE_RX_MIB_TABLE(conn)->pdu);
                printk("  rx_bytes      = %u\n", DSL_QUEUE_RX_MIB_TABLE(conn)->bytes);
                for ( sw_tx_queue_table = g_atm_priv_data.connection[conn].sw_tx_queue_table, sw_tx_queue = 0;
                      sw_tx_queue_table != 0;
                      sw_tx_queue_table >>= 1, sw_tx_queue++ )
                    if ( (sw_tx_queue_table & 0x01) )
                    {
                        aal5VccTxPDU        += DSL_QUEUE_TX_MIB_TABLE(sw_tx_queue)->pdu;
                        aal5VccTxBytes      += DSL_QUEUE_TX_MIB_TABLE(sw_tx_queue)->bytes;
                        aal5VccTxDroppedPDU += DSL_QUEUE_TX_DROP_MIB_TABLE(sw_tx_queue)->pdu;
                    }
                printk("  tx_pdu        = %u\n", aal5VccTxPDU);
                printk("  tx_bytes      = %u\n", aal5VccTxBytes);
                printk("  tx_dropped_pdu= %u\n", aal5VccTxDroppedPDU);
            }

            mib_itf = 0x08000000;
        }
        else if ( ifx_stricmp(p1, "on") == 0 || ifx_stricmp(p1, "enable") == 0 )
            mib_itf = 0x40000000;
        else if ( ifx_stricmp(p1, "off") == 0 || ifx_stricmp(p1, "disable") == 0 )
            mib_itf = 0x20000000;
        else if ( ifx_stricmp(p1, "clear") == 0 || ifx_stricmp(p1, "clean") == 0 )
            mib_itf = 0x80000000;
        else if ( (mib_itf & 0xE0000000) )
        {
            if ( ifx_stricmp(p1, "all") == 0 )
                mib_itf |= 0xFF;
            else if ( strlen(p1) == 1 && *p1 >= '0' && *p1 <= '7' )
                mib_itf |= 1 << (*p1 - '0');
            else
            {
                for ( i = 0; i < NUM_ENTITY(itf_name); i++ )
                    if ( ifx_stricmp(p1, itf_name[i]) == 0 )
                    {
                        if ( i < 6 )
                            mib_itf |= 1 << i;
                        else
                            mib_itf |= 3 << 6;
                        break;
                    }
            }
        }

        p1 = p2;
        colon = 1;
    }

    if ( (mib_itf & 0x40000000) )
        g_mib_itf |= mib_itf & 0xFF;
    else if ( (mib_itf & 0x20000000) )
        g_mib_itf &= ~(mib_itf & 0xFF);
    else if ( (mib_itf & 0x80000000) ) {
        clear_mib_counter(mib_itf);
    }
    else if ( !(mib_itf & 0xFF000000) ) {
        printk("echo <on|off|clear> <all|eth0|eth1|atm|cpu|ext_int?> > /proc/eth/mib\n");
        printk("echo queue > /proc/eth/mib\n");
        printk("echo pvc <vpi>.<vci> > /proc/eth/mib\n");
    }

    return 0;
}
#endif // #if defined(ATM_DATAPATH)

#if defined(DEBUG_FIRMWARE_TABLES_PROC) && DEBUG_FIRMWARE_TABLES_PROC
#if defined(PROC_WRITE_ROUTE) && PROC_WRITE_ROUTE
static void print_route( struct seq_file *m, int i, int f_is_lan, struct rout_forward_compare_tbl *pcompare, struct rout_forward_action_tbl *pwaction)
{

#if defined(ATM_DATAPATH)
	static const char *dest_list[] = {"ETH0", "ETH1", "CPU0", "EXT_INT1", "EXT_INT2", "EXT_INT3", "res", "ATM"};
	static const char *mpoa_type[] = {"EoA without FCS", "EoA with FCS", "PPPoA", "IPoA"};
#elif defined(PTM_DATAPATH)
   static const char *dest_list[] = {"ETH0", "ETH1", "CPU0", "EXT_INT1", "EXT_INT2", "EXT_INT3", "EXT_INT4", "PTM"};
#endif

   u32 bit;
   int j, k;

   seq_printf(m,  "  entry %d\n", i);
   seq_printf(m,  "    compare (0x%08X)\n", (u32)pcompare);
   seq_printf(m,  "      src:  %d.%d.%d.%d:%d\n", pcompare->src_ip >> 24, (pcompare->src_ip >> 16) & 0xFF, (pcompare->src_ip >> 8) & 0xFF, pcompare->src_ip & 0xFF, pcompare->src_port);
   seq_printf(m,  "      dest: %d.%d.%d.%d:%d\n", pcompare->dest_ip >> 24, (pcompare->dest_ip >> 16) & 0xFF, (pcompare->dest_ip >> 8) & 0xFF, pcompare->dest_ip & 0xFF, pcompare->dest_port);
   seq_printf(m,  "    action  (0x%08X)\n", (u32)pwaction);
   seq_printf(m,  "      new %s    %d.%d.%d.%d:%d\n", f_is_lan ? "src :" : "dest:", pwaction->new_ip >> 24, (pwaction->new_ip >> 16) & 0xFF, (pwaction->new_ip >> 8) & 0xFF, pwaction->new_ip & 0xFF, pwaction->new_port);
   seq_printf(m,  "      new MAC :    %02X:%02X:%02X:%02X:%02X:%02X\n", (pwaction->new_dest_mac54 >> 8) & 0xFF, pwaction->new_dest_mac54 & 0xFF, pwaction->new_dest_mac30 >> 24, (pwaction->new_dest_mac30 >> 16) & 0xFF, (pwaction->new_dest_mac30 >> 8) & 0xFF, pwaction->new_dest_mac30 & 0xFF);
   switch ( pwaction->rout_type )
   {
   case 1: seq_printf(m,  "      route type:  IP\n"); break;
   case 2: seq_printf(m,  "      route type:  NAT\n"); break;
   case 3: seq_printf(m,  "      route type:  NAPT\n"); break;
   default: seq_printf(m,  "      route type:  NULL\n");
   }
   if ( pwaction->new_dscp_en )
      seq_printf(m,  "      new DSCP:    %d\n", pwaction->new_dscp);
   else
      seq_printf(m,  "      new DSCP:    original (not modified)\n");
   seq_printf(m,  "      MTU index:   %d\n", pwaction->mtu_ix);
   if ( pwaction->in_vlan_ins )
      seq_printf(m,  "      VLAN insert: enable, VCI 0x%04x\n", pwaction->new_in_vci);
   else
      seq_printf(m,  "      VLAN insert: disable\n");
   seq_printf(m,  "      VLAN remove: %s\n", pwaction->in_vlan_rm ? "enable" : "disable");
   if ( !pwaction->dest_list )
      seq_printf(m,  "      dest list:   none\n");
   else
   {
      seq_printf(m,  "      dest list:   ");
      for ( bit = 1, j = k = 0; bit < 1 << 8; bit <<= 1, j++ )
         if ( (pwaction->dest_list & bit) )
         {
            if ( k )
               seq_printf(m,  ", ");
            seq_printf(m,  dest_list[j]);
            k = 1;
         }
      seq_printf(m,  "\n");
   }
   if ( pwaction->pppoe_mode )
   {
      seq_printf(m,  "      PPPoE mode:  termination\n");
      if ( f_is_lan )
         seq_printf(m,  "      PPPoE index: %d\n", pwaction->pppoe_ix);
   }
   else
      seq_printf(m,  "      PPPoE mode:  transparent\n");
   seq_printf(m,  "      new src MAC index: %d\n", pwaction->new_src_mac_ix);
   if(f_is_lan){
      seq_printf(m,       "      encap tunnel: %s\n", pwaction->encap_tunnel ? "yes":"no");

      seq_printf(m,       "      tunnel iphdr idx: %d\n", pwaction->tnnl_hdr_idx);
   }else{
      seq_printf(m,       "      decap tunnel: %s\n", pwaction->encap_tunnel ? "yes":"no");
   }
   if ( pwaction->out_vlan_ins )
      seq_printf(m,  "      outer VLAN insert: enable, index %d\n", pwaction->out_vlan_ix);
   else
      seq_printf(m,  "      outer VLAN insert: disable\n");
#if defined(ATM_DATAPATH)
   if ( f_is_lan || GEN_MODE_CFG1->sys_cfg != 0 /* not DSL WAN mode */){
      seq_printf(m,  "      outer VLAN remove: %s\n", pwaction->out_vlan_rm ? "enable" : "disable");
   }
   else {
      seq_printf(m,  "      outer VLAN remove: %s (forced to be enable for ATM PVC differentiation)\n", pwaction->out_vlan_rm ? "enable" : "disable");
   }
   if ( f_is_lan ) {
      seq_printf(m,  "      mpoa type:   %s\n", mpoa_type[pwaction->mpoa_type]);
      seq_printf(m,  "      dslwan qid:  %d (conn %d)\n", pwaction->dest_qid & 0xFF, (pwaction->dest_qid >> 8) & 0xFF);
   }
   else {
      seq_printf(m,  "      dest qid (dslwan qid): %d\n", pwaction->dest_qid);
   }
#elif defined(PTM_DATAPATH)
   seq_printf(m,  "      outer VLAN remove: %s\n", pwaction->out_vlan_rm ? "enable" : "disable");
   seq_printf(m,  "      dest qid (dslwan qid): %d\n", pwaction->dest_qid);
#endif

   seq_printf(m,  "      tcp:         %s\n", pwaction->protocol ? "yes" : "no (UDP)");
   seq_printf(m,  "      entry valid: %s\n", pwaction->entry_vld ? "yes" : "no");
#if defined(MIB_MODE_ENABLE) && MIB_MODE_ENABLE
   {
      if(PS_MC_GENCFG3->session_mib_unit == 1)
         seq_printf(m,           "      accl packets: %u\n", pwaction->bytes);
      else
         seq_printf(m,           "      accl bytes: %u\n", pwaction->bytes * 32);
   }

#else
   seq_printf(m,           "      accl bytes: %u\n", pwaction->bytes);
#endif

}

static void proc_route_output(struct seq_file *m, void *private_data __maybe_unused){
    struct rout_forward_compare_tbl *pcompare;
    struct rout_forward_action_tbl *paction;
    unsigned int i;

    __sync();

    seq_printf(m,  "Wan Routing Table\n");

    pcompare = (struct rout_forward_compare_tbl *)ROUT_WAN_HASH_CMP_TBL(0);
    paction = (struct rout_forward_action_tbl *)ROUT_WAN_HASH_ACT_TBL(0);
    for ( i = 0; i < MAX_ROUTING_ENTRIES_PER_HASH_BLOCK * MAX_HASH_BLOCK; i++ )
    {
        if ( *(u32*)pcompare && *((u32*)pcompare + 1) )
        {
            print_route(m, i, 0, pcompare, paction);
        }

        pcompare++;
        paction++;
    }

    pcompare = (struct rout_forward_compare_tbl *)ROUT_WAN_COLL_CMP_TBL(0);
    paction = (struct rout_forward_action_tbl *)ROUT_WAN_COLL_ACT_TBL(0);
    for ( i = 0; i < WAN_ROUT_TBL_CFG->rout_num; i++ )
    {
        if ( *(u32*)pcompare && *((u32*)pcompare + 1) )
        {
            print_route(m, i + MAX_ROUTING_ENTRIES_PER_HASH_BLOCK * MAX_HASH_BLOCK, 0, pcompare, paction);
        }

        pcompare++;
        paction++;
    }

    seq_printf(m,  "Lan Routing Table\n");

    pcompare = (struct rout_forward_compare_tbl *)ROUT_LAN_HASH_CMP_TBL(0);
    paction = (struct rout_forward_action_tbl *)ROUT_LAN_HASH_ACT_TBL(0);
    for ( i = 0; i < MAX_ROUTING_ENTRIES_PER_HASH_BLOCK * MAX_HASH_BLOCK; i++ )
    {
        if ( *(u32*)pcompare && *((u32*)pcompare + 1) )
        {
            print_route(m, i, 1, pcompare, paction);
        }

        pcompare++;
        paction++;
    }

    pcompare = (struct rout_forward_compare_tbl *)ROUT_LAN_COLL_CMP_TBL(0);
    paction = (struct rout_forward_action_tbl *)ROUT_LAN_COLL_ACT_TBL(0);
    for ( i = 0; i < LAN_ROUT_TBL_CFG->rout_num; i++ )
    {
        if ( *(u32*)pcompare && *((u32*)pcompare + 1) )
        {
            print_route(m, i + MAX_ROUTING_ENTRIES_PER_HASH_BLOCK * MAX_HASH_BLOCK, 1, pcompare, paction);
        }

        pcompare++;
        paction++;
    }
}

static int proc_route_input(char *line, void *data __maybe_unused) {
	static const char *command[] = {
			"add",      //  0
			"del",      //  1
			"LAN",      //  2
			"WAN",      //  3
			"new",      //  4
			"src",      //  5
			"dest",     //  6
			"MAC",      //  7
			"route",    //  8
			"type",     //  9
			"DSCP",     //  10
			"MTU",      //  11
			"index",    //  12
			"VLAN",     //  13
			"insert",   //  14
			"remove",   //  15
			"list",     //  16
			"PPPoE",    //  17
			"mode",     //  18
			"ch",       //  19
			"id",       //  20
			"delete",   //  21
			"disable",  //  22
			"enable",   //  23
			"transparent",  //  24
			"termination",  //  25
			"NULL",     //  26
			"IPv4",     //  27
			"NAT",      //  28
			"NAPT",     //  29
			"entry",    //  30
			"tcp",      //  31
			"help",     //  32
			"vci",      //  33
			"yes",      //  34
			"no",       //  35
			"qid",      //  36
			"outer",    //  37
			"IP",       //  38
			"IPv6",     //  39
		};


	static const char *dest_list[] = {"ETH0", "ETH1", "CPU0", "EXT_INT1", "EXT_INT2", "EXT_INT3", "EXT_INT4", "ATM"};
	static const int dest_list_strlen[] = {4, 4, 4, 8, 8, 8, 8, 3};

    int state;              //  1: new,
    int prev_cmd;
    int operation;          //  1: add, 2: delete
    int type;               //  1: LAN, 2: WAN, 0: auto detect
    int entry;
    struct rout_forward_compare_tbl compare_tbl;
    struct rout_forward_action_tbl action_tbl;
    uint32_t ipv6_src_ip[4] = {0}, ipv6_dst_ip[4] = {0};
    int is_ipv6 = 0;
    unsigned int val[20];
//    char common_proc_read_buff[1024];
    int len;
    char *p1, *p2;
    int colon;
    unsigned int i, j;
    u32 mask;
    u32 bit;
    u32 *pu1, *pu2, *pu3;

    len = strlen(line);
    state = 0;
    prev_cmd = 0;
    operation = 0;
    type = 0;
    entry = -1;

    memset(&compare_tbl, 0, sizeof(compare_tbl));
    memset(&action_tbl, 0, sizeof(action_tbl));

    p1 = line;
    colon = 1;
    while ( ifx_get_token(&p1, &p2, &len, &colon) )
    {
        for ( i = 0; i < sizeof(command) / sizeof(*command); i++ )
            if ( ifx_stricmp(p1, command[i]) == 0 )
            {
                switch ( i )
                {
                case 0:
                    if ( !state && !operation )
                    {
                        operation = 1;
//                      printk("add\n");
                    }
                    state = prev_cmd = 0;
                    break;
                case 1:
                case 21:
                    if ( !state && !operation )
                    {
                        operation = 2;
//                      printk("delete\n");
                    }
                    state = prev_cmd = 0;
                    break;
                case 2:
                    if ( !state && !type )
                    {
                        type = 1;
//                      printk("lan\n");
                    }
                    state = prev_cmd = 0;
                    break;
                case 3:
                    if ( !state && !type )
                    {
                        type = 2;
//                      printk("wan\n");
                    }
                    state = prev_cmd = 0;
                    break;
                case 4:
                    if ( !state )
                    {
                        state = 1;
                        prev_cmd = 4;
                    }
                    else
                        state = prev_cmd = 0;
                    break;
                case 5:
                    if ( state == 1 )
                    {
                        if ( prev_cmd == 4 )
                        {
                            //  new src
                            if ( !type )
                                type = 1;

                            //  check for "new src mac index"
                            ifx_ignore_space(&p2, &len);
                            if ( ifx_strincmp(p2, "mac ", 4) == 0 )
                            {
                                state = 2;
                                prev_cmd = 5;
                                break;
                            }
                            else
                            {
                                get_ip_port(&p2, &len, val);
//                              printk("new src: %d.%d.%d.%d:%d\n", val[0], val[1], val[2], val[3], val[4]);
                                action_tbl.new_ip = (val[0] << 24) | (val[1] << 16) | (val[2] << 8) | val[3];
                                action_tbl.new_port = val[4];
                            }
                        }
                        else
                            state = 0;
                    }
                    if ( state == 0 )
                    {
                        //  src
                        get_ip_port(&p2, &len, val);
//                      printk("src: %d.%d.%d.%d:%d\n", val[0], val[1], val[2], val[3], val[4]);
                        if ( val[5] == 4 )
                        {
                            if ( is_ipv6 )
                                ipv6_src_ip[0] = (val[0] << 24) | (val[1] << 16) | (val[2] << 8) | val[3];
                            else
                                compare_tbl.src_ip = (val[0] << 24) | (val[1] << 16) | (val[2] << 8) | val[3];
                        }
                        else
                        {
                            is_ipv6 = 1;
                            ipv6_src_ip[0] = (val[0] << 24) | (val[1] << 16) | (val[2] << 8) | val[3];
                            ipv6_src_ip[1] = (val[6] << 24) | (val[7] << 16) | (val[8] << 8) | val[9];
                            ipv6_src_ip[2] = (val[10] << 24) | (val[11] << 16) | (val[12] << 8) | val[13];
                            ipv6_src_ip[3] = (val[14] << 24) | (val[15] << 16) | (val[16] << 8) | val[17];
                        }
                        compare_tbl.src_port = val[4];
                    }
                    state = prev_cmd = 0;
                    break;
                case 6:
                    if ( state == 1 )
                    {
                        if ( prev_cmd == 4 )
                        {
                            //  new dest
                            if ( !type )
                                type = 2;

                            get_ip_port(&p2, &len, val);
//                          printk("new dest: %d.%d.%d.%d:%d\n", val[0], val[1], val[2], val[3], val[4]);
                            action_tbl.new_ip = (val[0] << 24) | (val[1] << 16) | (val[2] << 8) | val[3];
                            action_tbl.new_port = val[4];
                        }
                        else
                            state = 0;
                    }
                    if ( state == 0 )
                    {
                        if ( !colon )
                        {
                            int llen;

                            llen = len;
                            p1 = p2;
                            while ( llen && *p1 <= ' ' )
                            {
                                llen--;
                                p1++;
                            }
                            if ( llen && (*p1 == ':' || (*p1 >= '0' && *p1 <= '9')) )
                                colon = 1;
                        }
                        if ( colon )
                        {
                            //  dest
                            get_ip_port(&p2, &len, val);
//                          printk("dest: %d.%d.%d.%d:%d\n", val[0], val[1], val[2], val[3], val[4]);
                            if ( val[5] == 4 )
                            {
                                if ( is_ipv6 )
                                    ipv6_dst_ip[0] = (val[0] << 24) | (val[1] << 16) | (val[2] << 8) | val[3];
                                else
                                    compare_tbl.dest_ip = (val[0] << 24) | (val[1] << 16) | (val[2] << 8) | val[3];
                            }
                            else
                            {
                                is_ipv6 = 1;
                                ipv6_dst_ip[0] = (val[0] << 24) | (val[1] << 16) | (val[2] << 8) | val[3];
                                ipv6_dst_ip[1] = (val[6] << 24) | (val[7] << 16) | (val[8] << 8) | val[9];
                                ipv6_dst_ip[2] = (val[10] << 24) | (val[11] << 16) | (val[12] << 8) | val[13];
                                ipv6_dst_ip[3] = (val[14] << 24) | (val[15] << 16) | (val[16] << 8) | val[17];
                            }
                            compare_tbl.dest_port = val[4];
                        }
                        else
                        {
                            state = 1;
                            prev_cmd = 6;
                            break;
                        }
                    }
                    state = prev_cmd = 0;
                    break;
                case 7:
                    if ( state == 1 && prev_cmd == 4 )
                    {
                        //  new MAC
                        get_mac(&p2, &len, val);
//                      printk("new MAC: %02X.%02X.%02X.%02X:%02X:%02X\n", val[0], val[1], val[2], val[3], val[4], val[5]);
                        action_tbl.new_dest_mac54 = (val[0] << 8) | val[1];
                        action_tbl.new_dest_mac30 = (val[2] << 24) | (val[3] << 16) | (val[4] << 8) | val[5];
                    }
                    else if ( state == 2 && prev_cmd == 5 )
                    {
                        state = 3;
                        prev_cmd = 7;
                        break;
                    }
                    state = prev_cmd = 0;
                    break;
                case 8:
                    if ( !state )
                    {
                        state = 1;
                        prev_cmd = 8;
                    }
                    else
                        state = prev_cmd = 0;
                    break;
                case 9:
                    if ( state == 1 && prev_cmd == 8 )
                    {
                        state = 2;
                        prev_cmd = 9;
                    }
                    else
                        state = prev_cmd = 0;
                    break;
                case 10:
                    if ( state == 1 && prev_cmd == 4 )
                    {
                        ifx_ignore_space(&p2, &len);
                        if ( len && *p2 >= '0' && *p2 <= '9' )
                        {
                            //  new DSCP
                            val[0] = ifx_get_number(&p2, &len, 0);
//                          printk("new DSCP: %d\n", val[0]);
                            if ( !action_tbl.new_dscp_en )
                            {
                                action_tbl.new_dscp_en = 1;
                                action_tbl.new_dscp = val[0];
                            }
                        }
                        else if ( (len == 8 || (len > 8 && (p2[8] <= ' ' || p2[8] == ','))) && ifx_strincmp(p2, "original", 8) == 0 )
                        {
                            p2 += 8;
                            len -= 8;
                            //  new DSCP original
//                          printk("new DSCP: original\n");
                            //  the reset value is 0, so don't need to do anything
                        }
                    }
                    state = prev_cmd = 0;
                    break;
                case 11:
                    if ( !state )
                    {
                        state = 1;
                        prev_cmd = 11;
                    }
                    else
                        state = prev_cmd = 0;
                    break;
                case 12:
                    if ( state == 1 )
                    {
                        if ( prev_cmd == 11 )
                        {
                            //  MTU index
                            ifx_ignore_space(&p2, &len);
                            val[0] = ifx_get_number(&p2, &len, 0);
//                          printk("MTU index: %d\n", val[0]);
                            action_tbl.mtu_ix = val[0];
                        }
                        else if ( prev_cmd == 13 )
                        {
                            //  VLAN insert enable
                            //  VLAN index
                            ifx_ignore_space(&p2, &len);
                            val[0] = ifx_get_number(&p2, &len, 0);
//                          printk("VLAN insert: enable, index %d\n", val[0]);
                            if ( !action_tbl.in_vlan_ins )
                            {
                                action_tbl.in_vlan_ins = 1;
                                action_tbl.new_in_vci = val[0];
                            }
                        }
                        else if ( prev_cmd == 17 )
                        {
                            //  PPPoE index
                            ifx_ignore_space(&p2, &len);
                            val[0] = ifx_get_number(&p2, &len, 0);
//                          printk("PPPoE index: %d\n", val[0]);
                            action_tbl.pppoe_ix = val[0];
                        }
                    }
                    else if ( state == 3 && prev_cmd == 7 )
                    {
                        //  new src mac index
                        ifx_ignore_space(&p2, &len);
                        val[0] = ifx_get_number(&p2, &len, 0);
                        action_tbl.new_src_mac_ix = val[0];
                    }
                    else if ( state == 2 && prev_cmd == 13 )
                    {
                        //  outer VLAN enable
                        //  outer VLAN index
                        ifx_ignore_space(&p2, &len);
                        val[0] = ifx_get_number(&p2, &len, 0);
//                      printk("outer VLAN insert: enable, index %d\n", val[0]);
                        if ( !action_tbl.out_vlan_ins )
                        {
                            action_tbl.out_vlan_ins = 1;
                            action_tbl.out_vlan_ix = val[0];
                        }
                    }
                    state = prev_cmd = 0;
                    break;
                case 13:
                    if ( !state )
                    {
                        state = 1;
                        prev_cmd = 13;
                    }
                    else if ( state == 1 && prev_cmd == 37 )
                    {
                        state = 2;
                        prev_cmd = 13;
                        printk("outer vlan\n");
                    }
                    else
                        state = prev_cmd = 0;
                    break;
                case 14:
                    if ( (state == 1 || state == 2) && prev_cmd == 13 )
                    {
                        state++;
                        prev_cmd = 14;
                    }
                    else
                        state = prev_cmd = 0;
                    break;
                case 15:
                    if ( (state == 1 || state == 2) && prev_cmd == 13 )
                    {
                        state++;
                        prev_cmd = 15;
                    }
                    else
                        state = prev_cmd = 0;
                    break;
                case 16:
                    if ( state == 1 && prev_cmd == 6 )
                    {
                        mask = 0;
                        do
                        {
                            ifx_ignore_space(&p2, &len);
                            if ( !len )
                                break;
                            for ( j = 0, bit = 1; j < sizeof(dest_list) / sizeof(*dest_list); j++, bit <<= 1 )
                                if ( (len == dest_list_strlen[j] || (len > dest_list_strlen[j] && (p2[dest_list_strlen[j]] <= ' ' || p2[dest_list_strlen[j]] == ','))) && ifx_strincmp(p2, dest_list[j], dest_list_strlen[j]) == 0 )
                                {
                                    p2 += dest_list_strlen[j];
                                    len -= dest_list_strlen[j];
                                    mask |= bit;
                                    break;
                                }
                        } while ( j < sizeof(dest_list) / sizeof(*dest_list) );
//                      if ( mask )
//                      {
//                          //  dest list
//                          printk("dest list:");
//                          for ( j = 0, bit = 1; j < sizeof(dest_list) / sizeof(*dest_list); j++, bit <<= 1 )
//                              if ( (mask & bit) )
//                              {
//                                  printk(" %s", dest_list[j]);
//                              }
//                          printk("\n");
//                      }
//                      else
//                          printk("dest list: none\n");
                        action_tbl.dest_list = mask;
                    }
                    state = prev_cmd = 0;
                    break;
                case 17:
                    if ( !state )
                    {
                        state = 1;
                        prev_cmd = 17;
                    }
                    else
                        state = prev_cmd = 0;
                    break;
                case 18:
                    if ( state == 1 && prev_cmd == 17 )
                    {
                        state = 2;
                        prev_cmd = 18;
                    }
                    else
                        state = prev_cmd = 0;
                    break;
                case 19:
                    if ( state == 1 && prev_cmd == 6 )
                    {
                        state = 2;
                        prev_cmd = 19;
                    }
                    else
                        state = prev_cmd = 0;
                    break;
                case 20:
                    if ( state == 2 && prev_cmd == 19 )
                    {
                        //  dest ch id
                        ifx_ignore_space(&p2, &len);
                        val[0] = ifx_get_number(&p2, &len, 0);
//                      printk("dest ch id: %d\n", val[0]);
                        //action_tbl.dest_chid = val[0];
                    }
                    state = prev_cmd = 0;
                    break;
                case 22:
                case 23:
                    if ( state == 2 )
                    {
                        if ( prev_cmd == 14 )
                        {
                            //  VLAN insert
//                          printk("VLAN insert: %s (%d)", command[i], i - 22);
                            if ( (i - 22) )
                            {
                                ifx_ignore_space(&p2, &len);
                                if ( len > 5 && (p2[5] <= ' ' || p2[5] == ':') && ifx_strincmp(p2, "index", 5) == 0 )
                                {
                                    p2 += 6;
                                    len -= 6;
                                    //  VLAN index
                                    ifx_ignore_space(&p2, &len);
                                    val[0] = ifx_get_number(&p2, &len, 0);
//                                  printk(", index %d", val[0]);
                                    if ( !action_tbl.in_vlan_ins )
                                    {
                                        action_tbl.in_vlan_ins = 1;
                                        //action_tbl.vlan_ix = val[0];
                                        action_tbl.new_in_vci = val[0];
                                    }
                                }
                                else if ( len > 3 && (p2[3] <= ' ' || p2[3] == ':') && ifx_strincmp(p2, "vci", 3) == 0 )
                                {
                                    p2 += 4;
                                    len -= 4;
                                    //  New VCI
                                    ifx_ignore_space(&p2, &len);
                                    val[0] = ifx_get_number(&p2, &len, 1);
//                                  printk(", vci 0x%04X", val[0]);
                                    if ( !action_tbl.in_vlan_ins )
                                    {
                                        action_tbl.in_vlan_ins = 1;
                                        action_tbl.new_in_vci = val[0];
                                    }
                                }
                            }
                            else
                            {
                                action_tbl.in_vlan_ins = 0;
                                action_tbl.new_in_vci = 0;
                            }
//                          printk("\n");
                        }
                        else if ( prev_cmd == 15 )
                        {
                            //  VLAN remove
//                          printk("VLAN remove: %s (%d)\n", command[i], i - 22);
                            action_tbl.in_vlan_rm = i - 22;
                        }
                    }
                    else if ( state == 3 )
                    {
                        if ( prev_cmd == 14 )
                        {
                            //  outer vlan insert
//                          printk("outer VLAN insert: %s (%d)", command[i], i - 22);
                            if ( (i - 22) )
                            {
                                ifx_ignore_space(&p2, &len);
                                if ( len > 5 && (p2[5] <= ' ' || p2[5] == ':') && ifx_strincmp(p2, "index", 5) == 0 )
                                {
                                    p2 += 6;
                                    len -= 6;
                                    //  outer VLAN index
                                    ifx_ignore_space(&p2, &len);
                                    val[0] = ifx_get_number(&p2, &len, 0);
//                                  printk(", index %d", val[0]);
                                    if ( !action_tbl.out_vlan_ins )
                                    {
                                        action_tbl.out_vlan_ins = 1;
                                        action_tbl.out_vlan_ix = val[0];
                                    }
                                }
                            }
                            else
                            {
                                action_tbl.out_vlan_ins = 0;
                                action_tbl.out_vlan_ix = 0;
                            }
//                          printk("\n");
                        }
                        else if ( prev_cmd == 15 )
                        {
                            //  outer VLAN remove
//                          printk("outer VLAN remove: %s (%d)\n", command[i], i - 22);
                            action_tbl.out_vlan_rm = i - 22;
                        }
                    }
                    state = prev_cmd = 0;
                    break;
                case 24:
                case 25:
                    if ( state == 2 && prev_cmd == 18 )
                    {
                        //  PPPoE mode
//                      printk("PPPoE mode: %s (%d)\n", command[i], i - 24);
                        action_tbl.pppoe_mode = i - 24;
                    }
                    state = prev_cmd = 0;
                    break;
                case 38:
                case 39:
                    i = 27;
                case 26:
                case 27:
                case 28:
                case 29:
                    if ( state == 2 && prev_cmd == 9 )
                    {
                        //  route type
//                      printk("route type: %s (%d)\n", command[i], i - 26);
                        action_tbl.rout_type = i - 26;
                    }
                    state = prev_cmd = 0;
                    break;
                case 30:
                    if ( !state )
                    {
                        if ( entry < 0 )
                        {
                            ifx_ignore_space(&p2, &len);
                            if ( len && *p2 >= '0' && *p2 <= '9' )
                            {
                                entry = ifx_get_number(&p2, &len, 0);
                                //  entry
//                              printk("entry: %d\n", entry);
                            }
                        }
                    }
                    state = prev_cmd = 0;
                    break;
                case 31:  // if this flag is not presented, it's UDP by default
                    if ( !state )
                    {
                        state = 1;
                        prev_cmd = 31;
                        action_tbl.protocol = 1;
                    }
                    else
                        state = prev_cmd = 0;
                    break;
                case 32:
                    printk("add\n");
                    printk("  LAN/WAN entry ???\n");
                    printk("    compare\n");
                    printk("      src:  ???.???.???.???:????\n");
                    printk("      dest: ???.???.???.???:????\n");
                    printk("    action\n");
                    printk("      new src/dest:      ???.???.???.???:????\n");
                    printk("      new MAC:           ??:??:??:??:??:?? (HEX)\n");
                    printk("      route type:        NULL/IP/NAT/NAPT\n");
                    printk("      new DSCP:          original/??\n");
                    printk("      MTU index:         ??\n");
                    printk("      VLAN insert:       disable/enable, VCI ???? (HEX)\n");
                    printk("      VLAN remove:       disable/enable\n");
                    printk("      dest list:         ETH0/ETH1/CPU0/EXT_INT?\n");
                    printk("      PPPoE mode:        transparent/termination\n");
                    printk("      PPPoE index:       ??\n");
                    printk("      new src MAC index: ??\n");
                    printk("      outer VLAN insert: disable/enable, index ??\n");
                    printk("      outer VLAN remove: disable/enable\n");
                    printk("      tcp:               yes/no\n");
                    printk("      dest qid:          ??\n");
                    printk("\n");
                    printk("delete\n");
                    printk("  LAN/WAN entry ???\n");
                    printk("    compare\n");
                    printk("      src:  ???.???.???.???:????\n");
                    printk("      dest: ???.???.???.???:????\n");
                    printk("\n");
                    state = prev_cmd = 0;
                    break;
                case 33:
                    if ( state == 1 && prev_cmd == 13 )
                    {
                        //  vlan vci
                        ifx_ignore_space(&p2, &len);
                        val[0] = ifx_get_number(&p2, &len, 1);
                        if ( !action_tbl.in_vlan_ins )
                        {
                            action_tbl.in_vlan_ins = 1;
                            action_tbl.new_in_vci = val[0];
                        }
                    }
                    state = prev_cmd = 0;
                    break;
                case 34:
                    if ( state == 1 && prev_cmd == 31 )
                        //  tcp yes
                        action_tbl.protocol = 1;
                    state = prev_cmd = 0;
                    break;
                case 35:
                    if ( state == 1 && prev_cmd == 31 )
                        //  tcp no
                        action_tbl.protocol = 0;
                    state = prev_cmd = 0;
                    break;
                case 36:
                    if ( state == 1 && prev_cmd == 6 )
                    {
                        //  dest qid
                        ifx_ignore_space(&p2, &len);
                        val[0] = ifx_get_number(&p2, &len, 0);
                        action_tbl.dest_qid = val[0];
                    }
                    state = prev_cmd = 0;
                    break;
                case 37:
                    if ( !state )
                    {
                        state = 1;
                        prev_cmd = 37;
                    }
                    else
                        state = prev_cmd = 0;
                    break;
                default:
                    state = prev_cmd = 0;
                }

                break;
            }

        if ( i == sizeof(command) / sizeof(*command) )
            state = prev_cmd = 0;

        p1 = p2;
        colon = 1;
    }

    if ( operation == 2 )
    {
        u32 is_lan = 0;

        //  delete
        pu1 = (u32*)&compare_tbl;
        pu2 = NULL;
        pu3 = NULL;
        if ( entry < 0 )
        {
            //  search the entry number
            if ( *pu1 && pu1[1] )
            {
                if ( (!type || type == 1) )
                {
                    //  LAN
		    for (entry = 0; (unsigned)entry < MAX_ROUTING_ENTRIES_PER_HASH_BLOCK * MAX_HASH_BLOCK; entry++) {
                        pu2 = (u32*)ROUT_LAN_HASH_CMP_TBL(entry);
                        if ( *pu2 == *pu1 && pu2[1] == pu1[1] && pu2[2] == pu1[2] )
                        {
                            pu3 = (u32*)ROUT_LAN_HASH_ACT_TBL(entry);
                            break;
                        }
                    }
                    if ( entry == MAX_ROUTING_ENTRIES_PER_HASH_BLOCK * MAX_HASH_BLOCK )
                    {
                        for ( entry = 0; entry < LAN_ROUT_TBL_CFG->rout_num; entry++ )
                        {
                            pu2 = (u32*)ROUT_LAN_COLL_CMP_TBL(entry);
                            if ( *pu2 == *pu1 && pu2[1] == pu1[1] && pu2[2] == pu1[2] )
                            {
                                pu3 = (u32*)ROUT_LAN_COLL_ACT_TBL(entry);
                                break;
                            }
                        }
                        if ( entry == LAN_ROUT_TBL_CFG->rout_num )
                            pu2 = NULL;
                        else
                            entry += MAX_ROUTING_ENTRIES_PER_HASH_BLOCK * MAX_HASH_BLOCK;
                    }
                    if ( pu3 != NULL )
                        is_lan = 1;
                }
                if ( (!type && !pu2) || type == 2 )
                {
                    //  WAN
		    for (entry = 0; (unsigned)entry < MAX_ROUTING_ENTRIES_PER_HASH_BLOCK * MAX_HASH_BLOCK; entry++)
                    {
                        pu2 = (u32*)ROUT_WAN_HASH_CMP_TBL(entry);
                        if ( *pu2 == *pu1 && pu2[1] == pu1[1] && pu2[2] == pu1[2] )
                        {
                            pu3 = (u32*)ROUT_WAN_HASH_ACT_TBL(entry);
                            break;
                        }
                    }
                    if ( entry == MAX_ROUTING_ENTRIES_PER_HASH_BLOCK * MAX_HASH_BLOCK )
                    {
                        for ( entry = 0; entry < WAN_ROUT_TBL_CFG->rout_num; entry++ )
                        {
                            pu2 = (u32*)ROUT_WAN_COLL_CMP_TBL(entry);
                            if ( *pu2 == *pu1 && pu2[1] == pu1[1] && pu2[2] == pu1[2] )
                            {
                                pu3 = (u32*)ROUT_WAN_COLL_ACT_TBL(entry);
                                break;
                            }
                        }
                        if ( entry == WAN_ROUT_TBL_CFG->rout_num )
                            pu2 = NULL;
                        else
                            entry += MAX_ROUTING_ENTRIES_PER_HASH_BLOCK * MAX_HASH_BLOCK;
                    }
                    if ( pu3 != NULL )
                        is_lan = 0;
                }
            }
        }
        else
        {
            if ( *pu1 && pu1[1] )
            {
                pu3 = NULL;
                //  check compare
                if ( !type || type == 1 )
                {
                    //  LAN
		    if (entry < (int)(MAX_ROUTING_ENTRIES_PER_HASH_BLOCK * MAX_HASH_BLOCK)) {
                        pu2 = (u32*)ROUT_LAN_HASH_CMP_TBL(entry);
                        if ( *pu2 != *pu1 || pu2[1] != pu1[1] || pu2[2] != pu1[2] )
                            pu2 = NULL;
                        else
                            pu3 = (u32*)ROUT_LAN_HASH_ACT_TBL(entry);
		    } else if (entry < (int)(MAX_ROUTING_ENTRIES_PER_HASH_BLOCK * MAX_HASH_BLOCK + LAN_ROUT_TBL_CFG->rout_num)) {
                        pu2 = (u32*)ROUT_LAN_COLL_CMP_TBL(entry - MAX_ROUTING_ENTRIES_PER_HASH_BLOCK * MAX_HASH_BLOCK);
                        if ( *pu2 != *pu1 || pu2[1] != pu1[1] || pu2[2] != pu1[2] )
                            pu2 = NULL;
                        else
                            pu3 = (u32*)ROUT_LAN_COLL_ACT_TBL(entry - MAX_ROUTING_ENTRIES_PER_HASH_BLOCK * MAX_HASH_BLOCK);
                    }
                    if ( pu3 != NULL )
                        is_lan = 1;
                }
                if ( !type || type == 2 )
                {
                    //  WAN
		    if (entry < (int)(MAX_ROUTING_ENTRIES_PER_HASH_BLOCK * MAX_HASH_BLOCK)) {
                        pu2 = (u32*)ROUT_WAN_HASH_CMP_TBL(entry);
                        if ( *pu2 != *pu1 || pu2[1] != pu1[1] || pu2[2] != pu1[2] )
                            pu2 = NULL;
                        else
                            pu3 = (u32*)ROUT_WAN_HASH_ACT_TBL(entry);
		    } else if (entry < (int)(MAX_ROUTING_ENTRIES_PER_HASH_BLOCK * MAX_HASH_BLOCK + WAN_ROUT_TBL_CFG->rout_num)) {
                        pu2 = (u32*)ROUT_WAN_COLL_CMP_TBL(entry - MAX_ROUTING_ENTRIES_PER_HASH_BLOCK * MAX_HASH_BLOCK);
                        if ( *pu2 != *pu1 || pu2[1] != pu1[1] || pu2[2] != pu1[2] )
                            pu2 = NULL;
                        else
                            pu3 = (u32*)ROUT_WAN_COLL_ACT_TBL(entry - MAX_ROUTING_ENTRIES_PER_HASH_BLOCK * MAX_HASH_BLOCK);
                    }
                    if ( pu3 != NULL )
                        is_lan = 0;
                }
            }
            else if ( !*pu1 && !pu1[1] )
            {
                if ( type == 1 )
                {
		    if (entry < (int)(MAX_ROUTING_ENTRIES_PER_HASH_BLOCK * MAX_HASH_BLOCK)) {
                        pu2 = (u32*)ROUT_LAN_HASH_CMP_TBL(entry);
                        pu3 = (u32*)ROUT_LAN_HASH_ACT_TBL(entry);
		    } else if (entry < (int)(MAX_ROUTING_ENTRIES_PER_HASH_BLOCK * MAX_HASH_BLOCK + LAN_ROUT_TBL_CFG->rout_num)) {
                        pu2 = (u32*)ROUT_LAN_COLL_CMP_TBL(entry - MAX_ROUTING_ENTRIES_PER_HASH_BLOCK * MAX_HASH_BLOCK);
                        pu3 = (u32*)ROUT_LAN_COLL_ACT_TBL(entry - MAX_ROUTING_ENTRIES_PER_HASH_BLOCK * MAX_HASH_BLOCK);
                    }
                    if ( pu3 != NULL )
                        is_lan = 1;
                }
                else if ( type == 2 )
                {
		    if (entry < (int)(MAX_ROUTING_ENTRIES_PER_HASH_BLOCK * MAX_HASH_BLOCK)) {
                        pu2 = (u32*)ROUT_WAN_HASH_CMP_TBL(entry);
                        pu3 = (u32*)ROUT_WAN_HASH_ACT_TBL(entry);
		    } else if (entry < (int)(MAX_ROUTING_ENTRIES_PER_HASH_BLOCK * MAX_HASH_BLOCK + WAN_ROUT_TBL_CFG->rout_num)) {
                        pu2 = (u32*)ROUT_WAN_COLL_CMP_TBL(entry - MAX_ROUTING_ENTRIES_PER_HASH_BLOCK * MAX_HASH_BLOCK);
                        pu3 = (u32*)ROUT_WAN_COLL_ACT_TBL(entry - MAX_ROUTING_ENTRIES_PER_HASH_BLOCK * MAX_HASH_BLOCK);
                    }
                    if ( pu3 != NULL )
                        is_lan = 0;
                }
            }
        }
        if ( pu2 && pu3 )
        {
            is_lan <<= 31;

            if ( ifx_ppa_drv_hal_generic_hook == NULL )
            {
                printk("HAL is not loaded - verify only, no firmware table operation\n");
                printk("del_routing_entry(%d - %08x)\n", entry, (u32)entry | is_lan);
            }
            else
            {
                PPE_ROUTING_INFO route = {0};
                int ret;

                route.entry = (u32)entry | is_lan;
                ret = ifx_ppa_drv_hal_generic_hook(PPA_GENERIC_HAL_DEL_ROUTE_ENTRY, &route, 0);
                printk("%s entry %d deleted %s\n", is_lan ? "LAN" : "WAN", entry, ret == IFX_SUCCESS ? "successfully" : "fail");
            }
        }
    }
    else if ( operation == 1 && type && ((!is_ipv6 && *(u32*)&compare_tbl && *((u32*)&compare_tbl + 1)) || (is_ipv6 && *((u32*)&compare_tbl + 2))) )
    {
        if ( ifx_ppa_drv_hal_generic_hook == NULL )
        {
            // char str_to_print[1024];

            printk("HAL is not loaded - verify only, no firmware table operation\n");

            if ( is_ipv6 )
            {
                printk("ipv6 src ip:  %d.%d.%d.%d", ipv6_src_ip[0] >> 24, (ipv6_src_ip[0] >> 16) & 0xFF, (ipv6_src_ip[0] >> 8) & 0xFF, ipv6_src_ip[0] & 0xFF);
                printk(".%d.%d.%d.%d", ipv6_src_ip[1] >> 24, (ipv6_src_ip[1] >> 16) & 0xFF, (ipv6_src_ip[1] >> 8) & 0xFF, ipv6_src_ip[1] & 0xFF);
                printk(".%d.%d.%d.%d", ipv6_src_ip[2] >> 24, (ipv6_src_ip[2] >> 16) & 0xFF, (ipv6_src_ip[2] >> 8) & 0xFF, ipv6_src_ip[2] & 0xFF);
                printk(".%d.%d.%d.%d", ipv6_src_ip[3] >> 24, (ipv6_src_ip[3] >> 16) & 0xFF, (ipv6_src_ip[3] >> 8) & 0xFF, ipv6_src_ip[3] & 0xFF);
                printk("\n");
                printk("ipv6 dest ip: %d.%d.%d.%d", ipv6_dst_ip[0] >> 24, (ipv6_dst_ip[0] >> 16) & 0xFF, (ipv6_dst_ip[0] >> 8) & 0xFF, ipv6_dst_ip[0] & 0xFF);
                printk(".%d.%d.%d.%d", ipv6_dst_ip[1] >> 24, (ipv6_dst_ip[1] >> 16) & 0xFF, (ipv6_dst_ip[1] >> 8) & 0xFF, ipv6_dst_ip[1] & 0xFF);
                printk(".%d.%d.%d.%d", ipv6_dst_ip[2] >> 24, (ipv6_dst_ip[2] >> 16) & 0xFF, (ipv6_dst_ip[2] >> 8) & 0xFF, ipv6_dst_ip[2] & 0xFF);
                printk(".%d.%d.%d.%d", ipv6_dst_ip[3] >> 24, (ipv6_dst_ip[3] >> 16) & 0xFF, (ipv6_dst_ip[3] >> 8) & 0xFF, ipv6_dst_ip[3] & 0xFF);
                printk("\n");
            }
            // print_route(str_to_print, 0, type == 1 ? 1 : 0, &compare_tbl, &action_tbl);
            // printk(str_to_print);
        }
        else
        {
            PPE_ROUTING_INFO route = {0};
            u8 test_mac[PPA_ETH_ALEN] = {0};

            test_mac[0] = (action_tbl.new_dest_mac54 >> 8) & 0xFF;
            test_mac[1] = action_tbl.new_dest_mac54 & 0xFF;
            test_mac[2] = (action_tbl.new_dest_mac30 >> 24) & 0xFF;
            test_mac[3] = (action_tbl.new_dest_mac30 >> 16) & 0xFF;
            test_mac[4] = (action_tbl.new_dest_mac30 >> 8) & 0xFF;
            test_mac[5] = action_tbl.new_dest_mac30 & 0xFF;

            route.f_is_lan = type == 1 ? 1 : 0;
            if ( is_ipv6 )
            {
                route.src_ip.f_ipv6 = 1;
                memcpy(route.src_ip.ip.ip6, ipv6_src_ip, 16);
                route.dst_ip.f_ipv6 = 1;
                memcpy(route.dst_ip.ip.ip6, ipv6_dst_ip, 16);
            }
            else
            {
                route.src_ip.f_ipv6 = 0;
                route.src_ip.ip.ip = compare_tbl.src_ip;
                route.dst_ip.f_ipv6 = 0;
                route.dst_ip.ip.ip = compare_tbl.dest_ip;
                route.new_ip.f_ipv6 = 0;
                route.new_ip.ip.ip = action_tbl.new_ip;
            }
            route.src_port              = compare_tbl.src_port;
            route.dst_port              = compare_tbl.dest_port;
            route.new_port              = action_tbl.new_port;
            route.f_is_tcp              = action_tbl.protocol;
            route.route_type            = action_tbl.rout_type;
            memcpy(route.new_dst_mac, test_mac, 6);
            route.src_mac.mac_ix        = action_tbl.new_src_mac_ix;
            route.mtu_info.mtu_ix       = action_tbl.mtu_ix;
            route.pppoe_mode            = action_tbl.pppoe_mode;
            route.pppoe_info.pppoe_ix   = action_tbl.pppoe_ix;
            route.f_new_dscp_enable     = action_tbl.new_dscp_en;
            route.new_dscp              = action_tbl.new_dscp;
            route.f_vlan_ins_enable     = action_tbl.in_vlan_ins;
            route.new_vci               = action_tbl.new_in_vci;
            route.f_vlan_rm_enable      = action_tbl.in_vlan_rm;
            route.f_out_vlan_ins_enable = action_tbl.out_vlan_ins;
            route.out_vlan_info.vlan_entry = action_tbl.out_vlan_ix;
            route.f_out_vlan_rm_enable  = action_tbl.out_vlan_rm;
            route.dslwan_qid            = action_tbl.dest_qid;
            route.dest_list             = action_tbl.dest_list;

            if ( ifx_ppa_drv_hal_generic_hook(PPA_GENERIC_HAL_ADD_ROUTE_ENTRY, &route, 0) == IFX_SUCCESS )
                printk("%s entry add successfully: entry = %d\n", (route.entry & (1 << 31)) ? "LAN" : "WAN", route.entry & ~(1 << 31));
            else
                printk("%s entry add fail\n", type == 1 ? "LAN" : "WAN");
        }
    }
    else
        printk("No operation: operation = %d, type = %d, is_ipv6 = %d\n", operation, type, is_ipv6);

    return 0;
}
#endif // #if defined(PROC_WRITE_ROUTE) && PROC_WRITE_ROUTE


static void proc_mc_output(struct seq_file *m, void *priv_data __maybe_unused) {
    volatile struct wan_rout_multicast_cmp_tbl *pcompare;
    volatile struct wan_rout_multicast_act_tbl *paction;
#if defined(RTP_SAMPLING_ENABLE) && RTP_SAMPLING_ENABLE
    volatile struct rtp_sampling_cnt *rtp_mib;
#endif
    int i;
    __sync();

    seq_printf(m, "Multicast Table\n");

    pcompare = ROUT_WAN_MC_CMP_TBL(0);
    paction = ROUT_WAN_MC_ACT_TBL(0);
#if defined(RTP_SAMPLING_ENABLE) && RTP_SAMPLING_ENABLE
    rtp_mib= MULTICAST_RTP_MIB_TBL(0);
#endif
    for ( i = 0; i < MAX_WAN_MC_ENTRIES; i++ ) {
        if ( pcompare->wan_dest_ip )
        {
            print_mc( m, i,
                             (struct wan_rout_multicast_cmp_tbl *)pcompare,
                             (struct wan_rout_multicast_act_tbl *)paction,
#if defined(RTP_SAMPLING_ENABLE) && RTP_SAMPLING_ENABLE
                           (struct rtp_sampling_cnt *)rtp_mib,
#endif
                             (uint32_t) ROUT_WAN_MC_CNT(i),
                             (uint32_t) *ROUT_WAN_MC_CNT(i)
                           );
        }

        pcompare++;
        paction++;
#if defined(RTP_SAMPLING_ENABLE) && RTP_SAMPLING_ENABLE
        rtp_mib++;
#endif
    }

}

static void proc_genconf_output(struct seq_file *m, void *priv_data __maybe_unused){

    int i;
    unsigned long bit;

#if (defined(CONFIG_AR10) || defined(CONFIG_VR9)) && defined(CAP_WAP_CONFIG) && CAP_WAP_CONFIG
    int num = 0;
#endif

    __sync();

#if defined(ATM_DATAPATH)
    seq_printf(m, "CFG_WRX_HTUTS    (0x%08X): %d\n", (u32)CFG_WRX_HTUTS, *CFG_WRX_HTUTS);
    seq_printf(m, "CFG_WRDES_DELAY  (0x%08X): %d\n", (u32)CFG_WRDES_DELAY, *CFG_WRDES_DELAY);
    seq_printf(m, "WRX_EMACH_ON     (0x%08X): AAL - %s, OAM - %s\n", (u32)WRX_EMACH_ON, (*WRX_EMACH_ON & 0x01) ? "on" : "off",
                                                                                                      (*WRX_EMACH_ON & 0x02) ? "on" : "off");
    seq_printf(m, "WTX_EMACH_ON     (0x%08X):", (u32)WTX_EMACH_ON);
    for ( i = 0, bit = 1; i < 15; i++, bit <<= 1 )
    {
        seq_printf(m, " %d - %s", i, (*WTX_EMACH_ON & bit) ? "on " : "off");
        if ( i == 3 || i == 7 || i == 11 )
            seq_printf(m, "\n                               ");
    }
    seq_printf(m, "\n");
    seq_printf(m, "WRX_HUNT_BITTH   (0x%08X): %d\n", (u32)WRX_HUNT_BITTH, *WRX_HUNT_BITTH);
#endif // #if defined(ATM_DATAPATH)

    seq_printf(m, "ETH_PORTS_CFG    (0x%08X): WAN VLAN ID Hi - %d, WAN VLAN ID Lo - %d\n", (u32)ETH_PORTS_CFG, ETH_PORTS_CFG->wan_vlanid_hi, ETH_PORTS_CFG->wan_vlanid_lo);

    seq_printf(m, "LAN_ROUT_TBL_CFG (0x%08X): num - %d, TTL check - %s, no hit drop - %s\n",
                                                            (u32)LAN_ROUT_TBL_CFG,
                                                            LAN_ROUT_TBL_CFG->rout_num,
                                                            LAN_ROUT_TBL_CFG->ttl_en ? "on" : "off",
                                                            LAN_ROUT_TBL_CFG->rout_drop  ? "on" : "off"
                                                            );
    seq_printf(m, "                               IP checksum - %s, TCP/UDP checksum - %s, checksum error drop - %s\n",
                                                            LAN_ROUT_TBL_CFG->ip_ver_en ? "on" : "off",
                                                            LAN_ROUT_TBL_CFG->tcpdup_ver_en ? "on" : "off",
                                                            LAN_ROUT_TBL_CFG->iptcpudperr_drop ? "on" : "off"
                                                            );

    seq_printf(m, "WAN_ROUT_TBL_CFG (0x%08X): num - %d, TTL check - %s, no hit drop - %s\n",
                                                            (u32)WAN_ROUT_TBL_CFG,
                                                            WAN_ROUT_TBL_CFG->rout_num,
                                                            WAN_ROUT_TBL_CFG->ttl_en ? "on" : "off",
                                                            WAN_ROUT_TBL_CFG->rout_drop  ? "on" : "off"
                                                            );
    seq_printf(m, "                               MC num - %d, MC drop - %s\n",
                                                            WAN_ROUT_TBL_CFG->wan_rout_mc_num,
                                                            WAN_ROUT_TBL_CFG->wan_rout_mc_drop ? "on" : "off"
                                                            );
    seq_printf(m, "                               IP checksum - %s, TCP/UDP checksum - %s, checksum error drop - %s\n",
                                                            WAN_ROUT_TBL_CFG->ip_ver_en ? "on" : "off",
                                                            WAN_ROUT_TBL_CFG->tcpdup_ver_en ? "on" : "off",
                                                            WAN_ROUT_TBL_CFG->iptcpudperr_drop ? "on" : "off"
                                                            );

    seq_printf(m, "GEN_MODE_CFG1    (0x%08X): App2 - %s, U/S - %s, U/S early discard - %s\n"
                                "                               classification table entry - %d, IPv6 route entry - %d\n"
                                "                               session based rate control - %s, IPv4 WAN ingress hash alg - %s\n"
                                "                               multiple field based classification and VLAN assignment for bridging - %s\n"
                                "                               D/S IPv4 multicast acceleration - %s, IPv6 acceleration - %s\n"
                                "                               WAN acceleration - %s, LAN acceleration - %s, switch isolation - %s\n",
                                                            (u32)GEN_MODE_CFG1,
                                                            GEN_MODE_CFG1->app2_indirect ? "indirect" : "direct",
                                                            GEN_MODE_CFG1->us_indirect ? "indirect" : "direct",
                                                            GEN_MODE_CFG1->us_early_discard_en ? "enable" : "disable",
                                                            GEN_MODE_CFG1->classification_num,
                                                            GEN_MODE_CFG1->ipv6_rout_num,
                                                            GEN_MODE_CFG1->session_ctl_en ? "enable" : "disable",
                                                            GEN_MODE_CFG1->wan_hash_alg ? "src IP" : "dest port",
                                                            GEN_MODE_CFG1->brg_class_en ? "enable" : "disable",
                                                            GEN_MODE_CFG1->ipv4_mc_acc_mode ? "IP/port pairs" : "dst IP only",
                                                            GEN_MODE_CFG1->ipv6_acc_en ? "enable" : "disable",
                                                            GEN_MODE_CFG1->wan_acc_en ? "enable" : "disable",
                                                            GEN_MODE_CFG1->lan_acc_en ? "enable" : "disable",
                                                            GEN_MODE_CFG1->sw_iso_mode ? "isolated" : "not isolated"
                                                            );
    seq_printf(m, "WAN Interfaces:                %s \n", get_wanitf(1));
    seq_printf(m, "LAN Interfaces:                %s \n", get_wanitf(0));
    seq_printf(m, "GEN_MODE_CFG2    (0x%08X):", (u32)GEN_MODE_CFG2);
    for ( i = 0, bit = 1; i < 8; i++, bit <<= 1 )
    {
        seq_printf(m, " %d - %s", i, (GEN_MODE_CFG2->itf_outer_vlan_vld & bit) ? "on " : "off");
        if ( i == 3 )
            seq_printf(m, "\n                              ");
    }
    seq_printf(m, "\n");
    seq_printf(m, "TX_QOS_CFG       (0x%08X): time_tick - %d, overhd_bytes - %d, eth1_eg_qnum - %d\n"
                                "                               eth1_burst_chk - %s, eth1 - %s, rate shaping - %s, WFQ - %s\n",
#if defined(ATM_DATAPATH)
                                (u32)TX_QOS_CFG,
                                TX_QOS_CFG->time_tick,
                                TX_QOS_CFG->overhd_bytes,
                                TX_QOS_CFG->eth1_eg_qnum,
                                TX_QOS_CFG->eth1_burst_chk ? "on" : "off",
                                TX_QOS_CFG->eth1_qss ? "FW QoS" : "HW QoS",
                                TX_QOS_CFG->shape_en ? "on" : "off",
                                TX_QOS_CFG->wfq_en ? "on" : "off");
#elif defined(PTM_DATAPATH)
                                (u32)TX_QOS_CFG_DYNAMIC,
                                TX_QOS_CFG_DYNAMIC->time_tick,
                                TX_QOS_CFG_DYNAMIC->overhd_bytes,
                                TX_QOS_CFG_DYNAMIC->eth1_eg_qnum,
                                TX_QOS_CFG_DYNAMIC->eth1_burst_chk ? "on" : "off",
                                TX_QOS_CFG_DYNAMIC->eth1_qss ? "FW QoS" : "HW QoS",
                                TX_QOS_CFG_DYNAMIC->shape_en ? "on" : "off",
                                TX_QOS_CFG_DYNAMIC->wfq_en ? "on" : "off");
#endif

    seq_printf(m, "KEY_SEL_n        (0x%08X): %08x %08x %08x %08x\n",
                                                            (u32)KEY_SEL_n(0),
                                                            *KEY_SEL_n(0), *KEY_SEL_n(1), *KEY_SEL_n(2), *KEY_SEL_n(3));

#if defined(MIB_MODE_ENABLE) && MIB_MODE_ENABLE
    seq_printf(m, "MIB Mode - %s\n",
                                                         PS_MC_GENCFG3->session_mib_unit
                                                         ? "Packet" : "Byte"
                                                            );
#endif

#if (defined(CONFIG_AR10) || defined(CONFIG_VR9)) && defined(CAP_WAP_CONFIG) && CAP_WAP_CONFIG
    for ( i = 0; i < MAX_CAPWAP_ENTRIES; i++ )
    {
         if( CAPWAP_CONFIG_TBL(i)->acc_en == 1 )
            num +=1;
    }

    seq_printf(m, "Maximum CAPWAP tunnel- %d\n", num);
#endif


}

static int proc_genconf_input(char *line, void *data __maybe_unused){

    char *p;
    int rlen;

    int f_wan_hi = 0;

    rlen = strlen(line);

    for ( p = line; *p && *p <= ' '; p++, rlen-- );
    if ( !*p )
    {
        return 0;
    }

    if ( ifx_strincmp(p, "wan hi ", 7) == 0 )
    {
        p += 7;
        rlen -= 7;
        f_wan_hi = 1;
    }
    else if ( ifx_strincmp(p, "wan high ", 9) == 0 )
    {
        p += 9;
        rlen -= 9;
        f_wan_hi = 1;
    }
    else if ( ifx_strincmp(p, "wan lo ", 7) == 0 )
    {
        p += 7;
        rlen -= 7;
        f_wan_hi = -1;
    }
    else if ( ifx_strincmp(p, "wan low ", 8) == 0 )
    {
        p += 8;
        rlen -= 8;
        f_wan_hi = -1;
    }
    else if ( ifx_strincmp(p, "eth0 type ", 10) == 0 )
    {
        p += 10;
        rlen -= 10;
        if ( ifx_stricmp(p, "lan") == 0 )
            ETH_PORTS_CFG->eth0_type = 0;
        else if ( ifx_stricmp(p, "wan") == 0 )
            ETH_PORTS_CFG->eth0_type = 1;
        else if ( ifx_stricmp(p, "mix") == 0 )
            ETH_PORTS_CFG->eth0_type = 2;
    }

    if ( f_wan_hi )
    {
        int num;

        num = ifx_get_number(&p, &rlen, 0);
        if ( f_wan_hi > 0 )
            ETH_PORTS_CFG->wan_vlanid_hi = num;
        else
            ETH_PORTS_CFG->wan_vlanid_lo = num;
    }

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

#if defined(ATM_DATAPATH) && ATM_DATAPATH
static void proc_queue_output(struct seq_file *m, void *priv_data __maybe_unused) {
    static const char *mpoa_type_str[] = {"EoA w/o FCS", "EoA w FCS", "PPPoA", "IPoA"};

    struct wrx_queue_config rx;
    struct wtx_queue_config tx;
    char qmap_str[64];
    char qmap_flag;
    int qmap_str_len;
    int i, k;
    unsigned int bit;


    __sync();

    seq_printf(m, "RX Queue Config (0x%08X):\n", (u32)WRX_QUEUE_CONFIG(0));

    for ( i = 0; i < 16; i++ )
    {
        rx = *WRX_QUEUE_CONFIG(i);
        seq_printf(m,  "  %d: MPoA type - %s, MPoA mode - %s, IP version %d\n", i, mpoa_type_str[rx.mpoa_type], rx.mpoa_mode ? "LLC" : "VC mux", rx.ip_ver ? 6 : 4);
        seq_printf(m,  "     Oversize - %d, Undersize - %d, Max Frame size - %d\n", rx.oversize, rx.undersize, rx.mfs);
        seq_printf(m,  "     uu mask - 0x%02X, cpi mask - 0x%02X, uu exp - 0x%02X, cpi exp - 0x%02X\n", rx.uumask, rx.cpimask, rx.uuexp, rx.cpiexp);
        if ( rx.vlan_ins )
            seq_printf(m, "     new_vlan = 0x%08X\n", rx.new_vlan);

    }

    seq_printf(m, "TX Queue Config (0x%08X):\n", (u32)WTX_QUEUE_CONFIG(0));

    for ( i = 0; i < 15; i++ )
    {
        tx = *WTX_QUEUE_CONFIG(i);
        qmap_flag = 0;
        qmap_str_len = 0;
        for ( k = 0, bit = 1; k < 8; k++, bit <<= 1 )
            if ( (tx.same_vc_qmap & bit) )
            {
                if ( qmap_flag++ )
                    qmap_str_len += sprintf(qmap_str + qmap_str_len, ", ");
                qmap_str_len += sprintf(qmap_str + qmap_str_len, "%d", k);
            }
        seq_printf(m, "  %d: uu - 0x%02X, cpi - 0x%02X, same VC queue map - %s\n", i, tx.uu, tx.cpi, qmap_flag ? qmap_str : "null");
        seq_printf(m,  "     bearer channel - %d, QSB ID - %d, MPoA mode - %s\n", tx.sbid, tx.qsb_vcid, tx.mpoa_mode ? "LLC" : "VC mux");
        seq_printf(m,  "     ATM header - 0x%08X\n", tx.atm_header);

    }


}
#endif // #if defined(ATM_DATAPATH) && ATM_DATAPATH


static void proc_pppoe_output(struct seq_file *m, void *private_data __maybe_unused) {
    int i;

    __sync();

    seq_printf(m, "PPPoE Table (0x%08X) - Session ID:\n", (u32)PPPOE_CFG_TBL(0));

    for ( i = 0; i < 8; i++ )
    {
        seq_printf(m, "  %d: %d\n", i, *PPPOE_CFG_TBL(i));
    }
}

static void proc_mtu_output(struct seq_file *m, void *priv_data __maybe_unused){

    int i;

    __sync();

    seq_printf(m, "MTU Table (0x%08X):\n", (u32)MTU_CFG_TBL(0));
    for ( i = 0; i < 8; i++ )
    {
        seq_printf(m, "  %d: %d\n", i, *MTU_CFG_TBL(i));
    }
}

static void proc_hit_output(struct seq_file *m, void *private_data __maybe_unused){

    int i;
    int n;
    unsigned long bit;

    __sync();

    seq_printf(m, "Unicast Routing Hit Table (0x%08X):\n", (u32)ROUT_LAN_HASH_HIT_STAT_TBL(0));

    seq_printf(m, "             1 2 3 4 5 6 7 8 9 10\n");


    n = 1;
    for ( i = 0; i < 8; i++ )
        for ( bit = 0x80000000; bit; bit >>= 1 )
        {
            if ( n % 10 == 1 )
                seq_printf(m, "  %3d - %3d:", n, n + 9);

            seq_printf(m, " %d", (*ROUT_LAN_HASH_HIT_STAT_TBL(i) & bit) ? 1 : 0);

            if ( n++ % 10 == 0 )
            {
                seq_printf(m, "\n");

            }
        }

    if ( n % 10 != 0 )
    {
        seq_printf(m, "\n");

    }

    seq_printf(m, "Multicast Routing Hit Table (0x%08X):\n", (u32)ROUT_WAN_MC_HIT_STAT_TBL(0));
    seq_printf(m, "             1 2 3 4 5 6 7 8 9 10\n");

    n = 1;
    for ( i = 0; i < 1; i++ )
        for ( bit = 0x80000000; bit; bit >>= 1 )
        {
            if ( n % 10 == 1 )
                seq_printf(m, "  %3d - %3d:", n, n + 9);

            seq_printf(m, " %d", (*ROUT_WAN_MC_HIT_STAT_TBL(i) & bit) ? 1 : 0);

            if ( n++ % 10 == 0 )
            {
                seq_printf(m, "\n");

            }
        }

    if ( n % 10 != 0 )
    {
        seq_printf(m, "\n");

    }
}

static int proc_hit_input(char *line, void *data __maybe_unused){

    if ( ifx_stricmp(line, "clear") == 0 || ifx_stricmp(line, "clean") == 0 )
    {
        ppa_memset((void*)ROUT_LAN_HASH_HIT_STAT_TBL(0), 0, 64);
        ppa_memset((void*)ROUT_LAN_COLL_HIT_STAT_TBL(0), 0, 2);
        ppa_memset((void*)ROUT_WAN_HASH_HIT_STAT_TBL(0), 0, 64);
        ppa_memset((void*)ROUT_WAN_COLL_HIT_STAT_TBL(0), 0, 2);
        ppa_memset((void*)ROUT_WAN_MC_HIT_STAT_TBL(0), 0, 2);
    }

    return 0;
}

#if defined(ATM_DATAPATH)
static int proc_mpoa_input(char *line, void *data __maybe_unused){
    int conn = -1;
    int mpoa_type = -1;
    int mpoa_mode = -1;

    sscanf(line, "%d %d %d", &conn, &mpoa_type, &mpoa_mode);
    printk(KERN_ERR "got con=%d type=%d mode=%d\n", conn, mpoa_type, mpoa_mode);
    if ( conn >= 0  && mpoa_type >= 0 && mpoa_mode >= 0 )
    	mpoa_setup_conn(conn, mpoa_type, mpoa_mode);

    return 0;
}
#endif

static void proc_mac_output(struct seq_file *m, void *priv_data __maybe_unused){
    int i;
    unsigned int mac52, mac10;

    __sync();

    seq_printf(m, "MAC Table:\n");
    seq_printf(m, "  ROUT_MAC_CFG_TBL (0x%08X)\n", (u32)ROUT_MAC_CFG_TBL(0));

    for ( i = 0; i < 16; i++ )
    {
        mac52 = *ROUT_MAC_CFG_TBL(i);
        mac10 = *(ROUT_MAC_CFG_TBL(i) + 1);

        seq_printf(m, "    %2d: %02X:%02X:%02X:%02X:%02X:%02X\n", i + 1, mac52 >> 24, (mac52 >> 16) & 0xFF, (mac52 >> 8) & 0xFF, mac52 & 0xFF, mac10 >> 24, (mac10 >> 16) & 0xFF);
    }

}

static void proc_out_vlan_output(struct seq_file *m, void *priv_data __maybe_unused) {
    int i;

    __sync();

    seq_printf(m, "Outer VLAN Table (0x%08X):\n", (u32)OUTER_VLAN_TBL(0));

    for ( i = 0; i < 32; i++ )
    {
        seq_printf(m, "  %d: 0x%08X\n", i, *OUTER_VLAN_TBL(i));
    }

}

static void proc_ipv6_ip_output(struct seq_file *m, void *priv_data __maybe_unused) {
    int i, j, x;
    volatile char *p;
    __sync();

    seq_printf(m, "IPv6 IP Table:\n");

    for ( x = 0; x < MAX_IPV6_IP_ENTRIES_BLOCK; x++ )
        for ( i = 0; i < MAX_IPV6_IP_ENTRIES_PER_BLOCK; i++ )
            if ( IPv6_IP_IDX_TBL(x, i)[0] != 0 || IPv6_IP_IDX_TBL(x, i)[1] != 0 || IPv6_IP_IDX_TBL(x, i)[2] != 0 || IPv6_IP_IDX_TBL(x, i)[3] != 0 )
            {
                p = (volatile char *)IPv6_IP_IDX_TBL(x, i);
                seq_printf(m, "%3d - %u", x * MAX_IPV6_IP_ENTRIES_PER_BLOCK + i, (unsigned int)p[0]);
                for ( j = 1; j < 16; j++ )
                    seq_printf(m, ".%u", (unsigned int)p[j]);
                seq_printf(m, "\n");

            }
}


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

#if defined(DEBUG_HTU_PROC) && DEBUG_HTU_PROC
static void print_htu(struct seq_file *m, int i) {

	if (HTU_ENTRY(i)->vld) {
		seq_printf(m,  "%2d. valid\n", i);
		seq_printf(m, "    entry  0x%08x - pid %01x vpi %02x vci %04x pti %01x\n",
				*(u32*) HTU_ENTRY(i), HTU_ENTRY(i)->pid, HTU_ENTRY(i)->vpi,
				HTU_ENTRY(i)->vci, HTU_ENTRY(i)->pti);
		seq_printf(m, "    mask   0x%08x - pid %01x vpi %02x vci %04x pti %01x\n",
				*(u32*) HTU_MASK(i), HTU_MASK(i)->pid_mask,
				HTU_MASK(i)->vpi_mask, HTU_MASK(i)->vci_mask,
				HTU_MASK(i)->pti_mask);
		seq_printf(m,  "    result 0x%08x - type: %s, qid: %d",
				*(u32*) HTU_RESULT(i), HTU_RESULT(i)->type ? "cell" : "AAL5",
				HTU_RESULT(i)->qid);
		if (HTU_RESULT(i)->type)
			seq_printf(m,  ", cell id: %d, verification: %s",
					HTU_RESULT(i)->cellid, HTU_RESULT(i)->ven ? "on" : "off");
		seq_printf(m,  "\n");
	} else
		seq_printf(m,  "%2d. invalid\n", i);

}

static void proc_htu_output(struct seq_file *m, void *priv_data __maybe_unused){

	int htuts = *CFG_WRX_HTUTS;
	int i;

	__sync();

	for (i = 0; i < htuts; i++) {
		print_htu(m, i);
	}
}
#endif // #if defined(DEBUG_HTU_PROC) && DEBUG_HTU_PROC

static INLINE void get_ip_port(char **p, int *len, unsigned int *val)
{
	int i;
	unsigned int tmp_val[17] = {0};
	int is_ipv6 = 0;

	memset(val, 0, sizeof(*val) * 6);

	for ( i = 0; i < 4; i++ )
	{
		ifx_ignore_space(p, len);
		if ( !*len )
		break;
		val[i] = ifx_get_number(p, len, 0);
	}

	if ( **p == '.' )
	{
		is_ipv6 = 1;
		for ( i = 0; i < 16 - 4; i++ )
		{
			ifx_ignore_space(p, len);
			if ( !*len )
			break;
			tmp_val[i] = ifx_get_number(p, len, 0);
		}
	}

	ifx_ignore_space(p, len);
	if ( *len )
	val[4] = ifx_get_number(p, len, 0);

	if ( is_ipv6 )
	{
		val[5] = 6;
		for ( i = 0; i < 16 - 4; i++ )
		val[i + 6] = tmp_val[i];
	}
	else
	val[5] = 4;
}

static INLINE void get_mac(char **p, int *len, unsigned int *val)
{
	int i;

	memset(val, 0, sizeof(*val) * 6);

	for ( i = 0; i < 6; i++ )
	{
		ifx_ignore_space(p, len);
		if ( !*len )
		break;
		val[i] = ifx_get_number(p, len, 1);
	}
}
#endif


/*------------------------------------------------------------------------------------------*\
 * proc_read_dbg u. proc_write_dbg sind bei uns im common teil
\*------------------------------------------------------------------------------------------*/



static void proc_burstlen_output(struct seq_file *m, void *private_data __maybe_unused){
	u32 dma_ps;
	u32 dma_pctrl;
	unsigned long sys_flag;

	local_irq_save(sys_flag);
	dma_ps = *(volatile u32 *) 0xBE104140;
	*(volatile u32 *) 0xBE104140 = 0;
	dma_pctrl = *(volatile u32 *) 0xBE104144;
	*(volatile u32 *) 0xBE104140 = dma_ps;
	local_irq_restore(sys_flag);

	seq_printf(m, "DMA-PPE burst length: Rx %d, Tx %d\n",
			1 << ((dma_pctrl >> 2) & 0x03), 1 << ((dma_pctrl >> 4) & 0x03));
}

static int proc_burstlen_input(char *line, void *data __maybe_unused){
	char *p1, *p2;
	int colon = 0;
	int mask = 0x3C; //  rx: 0x0C, tx: 0x30, all: 0x3C
	int burstlen = 0;
	int burstlen_mask = 0;
	int f_help = 0;
	u32 dma_ps;
	int len;
	unsigned long sys_flag;

	len = strlen(line);
	p1 = line;
	while (ifx_get_token(&p1, &p2, &len, &colon)) {
		if (ifx_stricmp(p1, "rx") == 0)
			burstlen_mask |= (mask = 0x0C);
		else if (ifx_stricmp(p1, "tx") == 0)
			burstlen_mask |= (mask = 0x30);
		else if (ifx_stricmp(p1, "all") == 0)
			burstlen_mask |= (mask = 0x3C);
		else if (strcmp(p1, "2") == 0)
			burstlen = 0x14 & mask;
		else if (strcmp(p1, "4") == 0)
			burstlen = 0x28 & mask;
		else if (strcmp(p1, "8") == 0)
			burstlen = 0x3C & mask;
		else {
			f_help = 1;
			break;
		}

		p1 = p2;
	}

	if (!burstlen_mask && burstlen)
		burstlen_mask = 0x3C;

	if (!burstlen_mask || !burstlen)
		f_help = 1;

	if (!f_help) {
		local_irq_save(sys_flag);
		dma_ps = *(volatile u32 *) 0xBE104140;
		*(volatile u32 *) 0xBE104140 = 0;
		*(volatile u32 *) 0xBE104144 = (*(volatile u32 *) 0xBE104144
				& ~burstlen_mask) | burstlen;
		*(volatile u32 *) 0xBE104140 = dma_ps;
		local_irq_restore(sys_flag);
	} else {
		printk("echo [rx/tx/all] <2/4/8> > /proc/eth/burstlen\n");
	}

	return 0;
}

#if defined(DEBUG_FW_PROC) && DEBUG_FW_PROC
static void proc_fw_output(struct seq_file *m, void *private_data __maybe_unused) {

#if (defined(CONFIG_AR9) || defined(CONFIG_AR10))
    u32 tir4;
    u32 tir4_sll_state;
    u32 tir4_dplus_rx_on;
#endif

    seq_printf(m, "Firmware\n");

#if (defined(CONFIG_AR0) || defined(CONFIG_AR10))
    tir4 = *TIR(4);
    tir4_sll_state = (tir4 >> 21) & 7;
    tir4_dplus_rx_on = (tir4 >> 20) & 1;

    seq_printf(m, "\nQoS:\n");
    seq_printf(m, "  DSLWAN_TX_SWAP_RDPTR = %04X, %04X, %04X, %04X, %04X, %04X, %04X, %04X\n %04X, %04X, %04X, %04X, %04X, %04X, %04X\n",
                                            * __DSLWAN_TXDES_SWAP_PTR(0), * __DSLWAN_TXDES_SWAP_PTR(1),
                                            * __DSLWAN_TXDES_SWAP_PTR(2), * __DSLWAN_TXDES_SWAP_PTR(3),
                                            * __DSLWAN_TXDES_SWAP_PTR(4), * __DSLWAN_TXDES_SWAP_PTR(5),
                                            * __DSLWAN_TXDES_SWAP_PTR(6), * __DSLWAN_TXDES_SWAP_PTR(7),
                                            * __DSLWAN_TXDES_SWAP_PTR(8), * __DSLWAN_TXDES_SWAP_PTR(9),
                                            * __DSLWAN_TXDES_SWAP_PTR(10), * __DSLWAN_TXDES_SWAP_PTR(11),
                                            * __DSLWAN_TXDES_SWAP_PTR(12), * __DSLWAN_TXDES_SWAP_PTR(13),
                                            * __DSLWAN_TXDES_SWAP_PTR(14));

    seq_printf(m, "  DSLWAN_TX_PKT_CNT    = %04X, %04X, %04X, %04X, %04X, %04X, %04X, %04X\n %04X, %04X, %04X, %04X, %04X, %04X, %04X\n ",
                                            * __DSLWAN_TX_PKT_CNT(0), * __DSLWAN_TX_PKT_CNT(1),
                                            * __DSLWAN_TX_PKT_CNT(2), * __DSLWAN_TX_PKT_CNT(3),
                                            * __DSLWAN_TX_PKT_CNT(4), * __DSLWAN_TX_PKT_CNT(5),
                                            * __DSLWAN_TX_PKT_CNT(6), * __DSLWAN_TX_PKT_CNT(7),
                                            * __DSLWAN_TX_PKT_CNT(8), * __DSLWAN_TX_PKT_CNT(9),
                                            * __DSLWAN_TX_PKT_CNT(10), * __DSLWAN_TX_PKT_CNT(11),
                                            * __DSLWAN_TX_PKT_CNT(12), * __DSLWAN_TX_PKT_CNT(13),
                                            * __DSLWAN_TX_PKT_CNT(14));


    seq_printf(m, "  ENETS PGNUM             = %d\n",  __ENETS_PGNUM);
    seq_printf(m, "  ENETS DBUF BASE ADDR     = %08X\n",  (unsigned int)__ERX_DBUF_BASE);
    seq_printf(m, "  ENETS CBUF BASE ADDR     = %08X\n",  (unsigned int)__ERX_CBUF_BASE);
#endif

    seq_printf(m, "  QOSD_DPLUS_RDPTR     = %04X\n",
                                            * __DSLWAN_FP_RXDES_SWAP_RDPTR);

    seq_printf(m, "  QOSD_CPUTX_RDPTR     = %04X\n",
                                            * __CPU_TXDES_SWAP_RDPTR);

    seq_printf(m, "  DPLUS_RXDES_RDPTR    = %04X\n",
                                            * __DSLWAN_FP_RXDES_DPLUS_WRPTR);


    seq_printf(m, "  pre_dplus_ptr    = %04X\n", *PRE_DPLUS_PTR);

    seq_printf(m, "  pre_dplus_cnt    = %04X\n", (*DM_RXPKTCNT) & 0xff);

    seq_printf(m, "  dplus_ptr        = %04X\n", *DPLUS_PTR);

    seq_printf(m, "  dplus_cnt        = %04X\n", *DPLUS_CNT);

    seq_printf(m, "  DPLUS_RX_ON      = %d\n",  *DPLUS_RX_ON);

    seq_printf(m, "  ISR_IS           = %08X\n ",  *ISR_IS);

#if defined(ATM_DATAPATH)
    seq_printf(m, "\nQoS Mib:\n");
    seq_printf(m, "  cputx_pkts:             %u\n",   *__CPU_TO_DSLWAN_TX_PKTS);
    seq_printf(m, "  cputx_bytes:            %u\n",   *__CPU_TO_DSLWAN_TX_BYTES);
    seq_printf(m, "  cputx_drop_pkts:        %u\n",   *__CPU_TX_SWAPPER_DROP_PKTS);
    seq_printf(m, "  cputx_drop_bytess:      %u\n",   *__CPU_TX_SWAPPER_DROP_BYTES);

    seq_printf(m, "  dslwan_fp_drop_pkts:    %u\n",   *__DSLWAN_FP_SWAPPER_DROP_PKTS );
    seq_printf(m, "  dslwan_fp_drop_bytes:   %u\n",   *__DSLWAN_FP_SWAPPER_DROP_BYTES );

    seq_printf(m, "  dslwan_tx_qf_drop_pkts:  (%u, %u, %u, %u, %u, %u, %u, %u ,%u, %u, %u, %u, %u, %u, %u)\n",
                 *__DSLWAN_TX_THRES_DROP_PKT_CNT(0),  *__DSLWAN_TX_THRES_DROP_PKT_CNT(1),  *__DSLWAN_TX_THRES_DROP_PKT_CNT(2) ,  *__DSLWAN_TX_THRES_DROP_PKT_CNT(3) ,
                 *__DSLWAN_TX_THRES_DROP_PKT_CNT(4),  *__DSLWAN_TX_THRES_DROP_PKT_CNT(5),  *__DSLWAN_TX_THRES_DROP_PKT_CNT(6) ,  *__DSLWAN_TX_THRES_DROP_PKT_CNT(7),
                 *__DSLWAN_TX_THRES_DROP_PKT_CNT(8),  *__DSLWAN_TX_THRES_DROP_PKT_CNT(9),  *__DSLWAN_TX_THRES_DROP_PKT_CNT(10),  *__DSLWAN_TX_THRES_DROP_PKT_CNT(11),
                 *__DSLWAN_TX_THRES_DROP_PKT_CNT(12),  *__DSLWAN_TX_THRES_DROP_PKT_CNT(13),  *__DSLWAN_TX_THRES_DROP_PKT_CNT(14));
#endif // #if defined(ATM_DATAPATH)

#if defined (PTM_DATAPATH)
    {
        unsigned char * ls_txt[] = {
            "Looking",
            "Freewheel Sync False",
            "Synced",
            "Freewheel Sync True"
        };
        unsigned char * rs_txt[] = {
            "Out-of-Sync",
            "Synced"
        };

        seq_printf(m,  "\nE1 firmware:\n");

        seq_printf(m,  "SFSM Status:  bc0                         bc1  \n\n");
        seq_printf(m,  "Local State = %d [%-22s], %d [%-22s]\n"
                                         "RemoteState = %d [%-22s], %d [%-22s]\n",
                RX_BC_CFG(0)->local_state, ls_txt[RX_BC_CFG(0)->local_state],
                RX_BC_CFG(1)->local_state, ls_txt[RX_BC_CFG(1)->local_state],
                RX_BC_CFG(0)->remote_state, rs_txt[RX_BC_CFG(0)->remote_state],
                RX_BC_CFG(1)->remote_state, rs_txt[RX_BC_CFG(1)->remote_state]);
        seq_printf(m,  "  dbase     = 0x%04x  ( 0x%08x )    , 0x%04x  ( 0x%08x )\n",
                SFSM_DBA(0)->dbase, (unsigned int )SB_BUFFER_BOND(2, SFSM_DBA(0)->dbase + 0x2000),
                SFSM_DBA(1)->dbase, (unsigned int )SB_BUFFER_BOND(2, SFSM_DBA(1)->dbase + 0x2000)  );
        seq_printf(m,  "  cbase     = 0x%04x  ( 0x%08x )    , 0x%04x  ( 0x%08x )\n",
                SFSM_CBA(0)->cbase, (unsigned int )SB_BUFFER_BOND(2, SFSM_CBA(0)->cbase + 0x2000),
                SFSM_CBA(1)->cbase, (unsigned int )SB_BUFFER_BOND(2, SFSM_CBA(1)->cbase + 0x2000));
        seq_printf(m,  "  sen       = %-26d, %d\n", SFSM_CFG(0)->sen, SFSM_CFG(1)->sen );
        seq_printf(m,  "  idlekeep  = %-26d, %d\n", SFSM_CFG(0)->idlekeep, SFSM_CFG(1)->idlekeep );
        seq_printf(m,  "  pnum      = %-26d, %d\n", SFSM_CFG(0)->pnum, SFSM_CFG(1)->pnum );
        seq_printf(m,  "  pptr      = %-26d, %d\n", SFSM_PGCNT(0)->pptr, SFSM_PGCNT(1)->pptr);
        seq_printf(m,  "  upage     = %-26d, %d\n", SFSM_PGCNT(0)->upage, SFSM_PGCNT(1)->upage);
        seq_printf(m,  "  rdptr     = %-26d, %d\n", RX_BC_CFG(0)->rx_cw_rdptr, RX_BC_CFG(1)->rx_cw_rdptr );
        seq_printf(m,  "\n");


        seq_printf(m,  "FFSM Status:  bc0                         bc1  \n\n");
        seq_printf(m,  "  dbase     = 0x%04x  ( 0x%08x )    , 0x%04x  ( 0x%08x )\n",
                FFSM_DBA(0)->dbase, (unsigned int )SB_BUFFER_BOND(2, FFSM_DBA(0)->dbase + 0x2000),
                FFSM_DBA(1)->dbase, (unsigned int )SB_BUFFER_BOND(2, FFSM_DBA(1)->dbase + 0x2000) );
        seq_printf(m,  "  pnum      = %-26d, %d\n", FFSM_CFG(0)->pnum, FFSM_CFG(1)->pnum );
        seq_printf(m,  "  vpage     = %-26d, %d\n", FFSM_PGCNT(0)->vpage, FFSM_PGCNT(1)->vpage );
        seq_printf(m,  "  ival      = %-26d, %d\n", FFSM_PGCNT(0)->ival, FFSM_PGCNT(1)->ival );
        seq_printf(m,  "  fill_wm   = %-26d, %d\n", TX_BC_CFG(0)->fill_wm, TX_BC_CFG(1)->fill_wm );
        seq_printf(m,  "  uflw_wm   = %-26d, %d\n", TX_BC_CFG(0)->uflw_wm, TX_BC_CFG(1)->uflw_wm );
        seq_printf(m,  "  hold_pgs  = %-26d, %d\n", TX_BC_CFG(0)->holding_pages, TX_BC_CFG(1)->holding_pages );
        seq_printf(m,  "  rdy_pgs   = %-26d, %d\n", TX_BC_CFG(0)->ready_pages, TX_BC_CFG(1)->ready_pages );
        seq_printf(m,  "  pend_pgs  = %-26d, %d\n", TX_BC_CFG(0)->pending_pages, TX_BC_CFG(1)->pending_pages );
        seq_printf(m,  "  tc_wrptr  = %-26d, %d\n", TX_BC_CFG(0)->cw_wrptr, TX_BC_CFG(1)->cw_wrptr );

        seq_printf(m,  "\n");
    }

#endif // #if defined(PTM_DATAPATH)

}


static int proc_fw_input(char *line, void *data __maybe_unused) {
#if defined(ATM_DATAPATH)
    int i;
#endif

    if ( ifx_stricmp(line, "clear") == 0 || ifx_stricmp(line, "clean") == 0 )
    {
#if defined(ATM_DATAPATH)
       for( i=0;i<15;i++)
               * __DSLWAN_TX_THRES_DROP_PKT_CNT(i) = 0;
#endif
#if defined(PTM_DATAPATH)
       * __DSLWAN_TX_THRES_DROP_PKT_CNT0 = 0;
       * __DSLWAN_TX_THRES_DROP_PKT_CNT1 = 0;
       * __DSLWAN_TX_THRES_DROP_PKT_CNT2 = 0;
       * __DSLWAN_TX_THRES_DROP_PKT_CNT3 = 0;
       * __DSLWAN_TX_THRES_DROP_PKT_CNT4 = 0;
       * __DSLWAN_TX_THRES_DROP_PKT_CNT5 = 0;
       * __DSLWAN_TX_THRES_DROP_PKT_CNT6 = 0;
       * __DSLWAN_TX_THRES_DROP_PKT_CNT7 = 0;

#endif

        * __CPU_TO_DSLWAN_TX_PKTS  = 0;
        * __CPU_TO_DSLWAN_TX_BYTES = 0;


        * __CPU_TX_SWAPPER_DROP_PKTS    = 0;
        * __CPU_TX_SWAPPER_DROP_BYTES   = 0;
        * __DSLWAN_FP_SWAPPER_DROP_PKTS = 0;
        * __DSLWAN_FP_SWAPPER_DROP_PKTS = 0;
    }
    return 0;
}

static int INLINE is_char_in_set(char ch, char * set)
{

    while(*set) {
        if (ch == *set)
            return 1;
        set ++;
    }

    return 0;
}

static int tokenize(char * in, char * sep, char **tokens, int max_token_num, int * finished)
{
    int token_num = 0;

    * finished = 0;

    while (*in && token_num < max_token_num) {

        // escape all seperators
        while (*in && is_char_in_set(*in, sep)) {
            in ++;
        }

        // break if no more chars left
        if(! *in) {
            break;
        }

        // found a new token, remember start position
        tokens[token_num ++] = in;

        // search end of token
        in ++;
        while (*in && ! is_char_in_set(*in, sep)) {
            in ++;
        }

        if(! *in) {
            // break if no more chars left
            break;
        }
        else {
            // tokenize
            *in = '\0';
            in ++;
        }

    }

    if ( ! *in )
        * finished = 1;

    return token_num;
}

static void fwdbg_read_ethbuf(char **tokens, unsigned int token_num)
{
    unsigned int start_pg = 0;
    unsigned int print_pg_num;
    unsigned int num;
    unsigned int pg_size;
    volatile unsigned int *dbase, *cbase;
    unsigned int pnum, i;
    unsigned int *cw;


    dbase = (volatile unsigned int *)SB_BUFFER( ( ( struct dmrx_dba * )DM_RXDB)->dbase + 0x2000 );
    cbase = (volatile unsigned int *)SB_BUFFER( ( ( struct dmrx_cba * )DM_RXCB)->cbase + 0x2000);
    pnum = ( ( struct dmrx_cfg * )DM_RXCFG)->pnum;

    i = 0;
    start_pg = i < (unsigned int)token_num  ? simple_strtoul(tokens[i ++ ], NULL, 0) : start_pg;
    print_pg_num = i < (unsigned int)token_num ? simple_strtoul(tokens[i ++ ], NULL, 0) : pnum;

    start_pg %= pnum;
    print_pg_num = print_pg_num > pnum ? pnum : print_pg_num;

    pg_size =  ((( struct  dmrx_cfg * )DM_RXCFG)->psize + 1) * 32 / 4;


    printk("Share buffer data/ctrl buffer:\n\n");
    for(i = start_pg, num = 0 ; num < print_pg_num ; num ++, i = (i + 1) % pnum ) {

        struct ctrl_dmrx_2_fw * p_ctrl_dmrx;
        struct ctrl_fw_2_dsrx * p_ctrl_dsrx;
        unsigned int j = 0;

        cw = (unsigned int *)dbase + i * pg_size;

        p_ctrl_dmrx = (struct ctrl_dmrx_2_fw *) ( &cbase[i]);
        p_ctrl_dsrx = (struct ctrl_fw_2_dsrx *) ( &cbase[i]);


        for(j =0; j < pg_size; j=j+4) {

            if(j==0) {

                printk("Pg_id %2d: %08x %08x %08x %08x ", i, cw[0], cw[1], cw[2], cw[3]);
                printk("pg_val: %x, byte_off: %x, cos: %x \n", p_ctrl_dmrx->pg_val, p_ctrl_dmrx->byte_off, p_ctrl_dmrx->cos);
            }


            else if(j==4) {
                printk("          %08x %08x %08x %08x ",  cw[j], cw[j+1], cw[j+2], cw[j+3]);
                printk("bytes_cnt: %x, eof: %x \n", p_ctrl_dmrx->bytes_cnt, p_ctrl_dmrx->eof);
            }

            else if(j==12) {
                printk("          %08x %08x %08x %08x ",  cw[j], cw[j+1], cw[j+2], cw[j+3]);
#if defined(PTM_DATAPATH)
                printk("pg_val: %x, byte_off: %x, acc_sel: %x\n", ((unsigned int)p_ctrl_dsrx->pkt_status << 4) | p_ctrl_dsrx->flag_len, p_ctrl_dsrx->byte_off,p_ctrl_dsrx->acc_sel );
#elif defined(ATM_DATAPATH)
                printk("pg_val: %x, byte_off: %x, acc_sel: %x\n", p_ctrl_dsrx->pg_val, p_ctrl_dsrx->byte_off,p_ctrl_dsrx->acc_sel );
#endif

            }

            else if(j==16) {
                printk("          %08x %08x %08x %08x ",  cw[j], cw[j+1], cw[j+2], cw[j+3]);
                printk("cos: %x, bytes_cnt: %x, eof: %x\n", p_ctrl_dsrx->cos, p_ctrl_dsrx->bytes_cnt,p_ctrl_dsrx->eof );
            }

            else {
                printk("          %08x %08x %08x %08x \n",  cw[j], cw[j+1], cw[j+2], cw[j+3]);
            }

        }

            printk("\n");

    }

    return;
}



static void fwdbg_clear_ethbuf(char **tokens, unsigned int token_num)
{
    uint32_t i = 0;
    uint32_t pnum, pg_num, pg_size, start_pg = 0;
    volatile unsigned int *dbase, *cbase;

    dbase = (volatile unsigned int *)SB_BUFFER( ( ( struct dmrx_dba * )DM_RXDB)->dbase + 0x2000 );
    cbase = (volatile unsigned int *)SB_BUFFER( ( ( struct dmrx_cba * )DM_RXCB)->cbase + 0x2000);
    pnum = ((struct dmrx_cfg * )DM_RXCFG)->pnum;
    pg_num = pnum;
    pg_size =  ((( struct  dmrx_cfg * )DM_RXCFG)->psize + 1) * 32 / 4;

    start_pg = i < (unsigned int)token_num ? simple_strtoul(tokens[i++], NULL, 0) : start_pg;
    pg_num = i < (unsigned int)token_num ? simple_strtoul(tokens[i++], NULL, 0) : pg_num;
    start_pg %= pnum;
    pg_num = pg_num > pnum ? pnum : pg_num;


    dbase = dbase + start_pg * pg_size;
    cbase = cbase + start_pg * pg_size;

    ppa_memset((void *)dbase, 0, pg_num * pg_size * sizeof(uint32_t));
    ppa_memset((void *)cbase, 0, pg_num * sizeof(uint32_t));

    return;

}

static void fwdbg_dump_sthdr(char **tokens, unsigned int token_num)
{
    //uint32_t i = 0;
    uint32_t pnum,pg_size,pg_idx;
    volatile unsigned int *dbase;
    struct flag_header *pfhdr;
    unsigned char *paddr;

    dbase = (volatile unsigned int *)SB_BUFFER( ( ( struct dmrx_dba * )DM_RXDB)->dbase + 0x2000 );

    if(token_num < 1){
        fw_dbg_start("help dump_st_hdr");
        return;
    }

    pnum = ((struct dmrx_cfg * )DM_RXCFG)->pnum;
    pg_size = ((( struct  dmrx_cfg * )DM_RXCFG)->psize + 1) * 32 / 4;
    pg_idx = simple_strtoul(tokens[0], NULL, 0);
    if(pg_idx >= pnum){
        printk("Error : page NO %d is equal or bigger than total page number: %d\n", pg_idx, pnum);
        return;
    }

    pfhdr = (struct flag_header *)(dbase + pg_idx * pg_size);
    dump_flag_header(g_fwcode, pfhdr, __FUNCTION__, 0);

    if(token_num > 1 && ifx_strincmp(tokens[1], "iphdr", strlen(tokens[1])) == 0){
        if(pfhdr->is_inner_ipv4){
            printk("First header (IPv4)\n");
            paddr = (unsigned char *)pfhdr;
            paddr += 8 + pfhdr->pl_byteoff + pfhdr->ip_inner_offset;
            dump_ipv4_hdr((struct iphdr *)paddr);

        }else if(pfhdr->is_inner_ipv6){
            printk("First header (IPv6)\n");
            paddr = (unsigned char *)pfhdr;
            paddr += 8 + pfhdr->pl_byteoff + pfhdr->ip_inner_offset;
            dump_ipv6_hdr((struct ipv6hdr *)paddr);
        }

        printk("------------------------------------\n");
        if(pfhdr->is_outer_ipv4){
            printk("Second header (IPv4)\n");
            paddr = (unsigned char *)pfhdr;
            paddr += 8 + pfhdr->pl_byteoff + pfhdr->ip_outer_offset;
            dump_ipv4_hdr((struct iphdr *)paddr);
        }else if(pfhdr->is_outer_ipv6){
            printk("Second header (IPv6)\n");
            paddr = (unsigned char *)pfhdr;
            paddr += 8 + pfhdr->pl_byteoff + pfhdr->ip_outer_offset;
            dump_ipv6_hdr((struct ipv6hdr *)paddr);
        }

        printk("------------------------------------\n");
    }
    return;

}

//echo dump_rg [dmrx | dsrx | SLL]
static void fwdbg_dump_rg(char **tokens, unsigned int token_num)
{
    uint32_t i;

    if(token_num < 1){
        fw_dbg_start("help dump_rg");
        return;
    }

    if(ifx_strincmp(tokens[0],"dmrx", strlen(tokens[0])) == 0){

        printk("DMRX_DBA(0x612): 0x%x\n", ((struct dmrx_dba * )DM_RXDB)->dbase);
        printk("DMRX_CBA(0x613): 0x%x\n", (( struct dmrx_cba * )DM_RXCB)->cbase);
        printk("DMRX_CFG(0x614): \n");
            printk("\tSEN: \t0x%x\n", ((struct dmrx_cfg * )DM_RXCFG)->sen);
            printk("\tTRLPG: \t0x%x\n", ((struct dmrx_cfg * )DM_RXCFG)->trlpg);
            printk("\tHDLEN:\t0x%x\n", ((struct dmrx_cfg * )DM_RXCFG)->hdlen);
            printk("\tENDIAN:\t0x%x\n", ((struct dmrx_cfg * )DM_RXCFG)->endian);
            printk("\tPSIZE:\t0x%x\n", ((struct dmrx_cfg * )DM_RXCFG)->psize);
            printk("\tPNUM:\t0x%x\n", ((struct dmrx_cfg * )DM_RXCFG)->pnum);
        printk("DMRX_PGCNT(0x615): \n");
            printk("\tPGPTR: \t0x%x\n", ((struct dmrx_pgcnt * )DM_RXPGCNT)->pgptr);
            printk("\tDSRC: \t0x%x\n", ((struct dmrx_pgcnt * )DM_RXPGCNT)->dsrc);
            printk("\tDVAL: \t0x%x\n", ((struct dmrx_pgcnt * )DM_RXPGCNT)->dval);
            printk("\tDCMD: \t0x%x\n", ((struct dmrx_pgcnt * )DM_RXPGCNT)->dcmd);
            printk("\tUPAGE: \t0x%x\n", ((struct dmrx_pgcnt * )DM_RXPGCNT)->upage);
        printk("DMRX_PKTCNT(0x616): \n");
            printk("\tDSRC: \t0x%x\n", ((struct dmrx_pktcnt * )DM_RXPKTCNT)->dsrc);
            printk("\tDCMD: \t0x%x\n", ((struct dmrx_pktcnt * )DM_RXPKTCNT)->dcmd);
            printk("\tUPKT: \t0x%x\n", ((struct dmrx_pktcnt * )DM_RXPKTCNT)->upkt);

    }else if(ifx_strincmp(tokens[0],"dsrx", strlen(tokens[0])) == 0){
        printk("DSRX_DB(0x710): 0x%x\n", ((struct dsrx_dba * )DS_RXDB)->dbase);
        printk("DSRX_DB(0x711): 0x%x\n", ((struct dsrx_cba * )DS_RXCB)->cbase);
        printk("DSRX_CFG(0x712): \n");
            printk("\tDREN: \t0x%x\n", ((struct dsrx_cfg * )DS_RXCFG)->dren);
            printk("\tENDIAN:\t0x%x\n", ((struct dsrx_cfg * )DS_RXCFG)->endian);
            printk("\tPSIZE:\t0x%x\n", ((struct dsrx_cfg * )DS_RXCFG)->psize);
            printk("\tPNUM:\t0x%x\n", ((struct dsrx_cfg * )DS_RXCFG)->pnum);
        printk("DSRX_PGCNT(0x713): \n");
            printk("\tPGPTR: \t0x%x\n", ((struct dsrx_pgcnt* )DS_RXPGCNT)->pgptr);
            printk("\tISRC: \t0x%x\n", ((struct dsrx_pgcnt* )DS_RXPGCNT)->isrc);
            printk("\tIVAL: \t0x%x\n", ((struct dsrx_pgcnt* )DS_RXPGCNT)->ival);
            printk("\tICMD: \t0x%x\n", ((struct dsrx_pgcnt* )DS_RXPGCNT)->icmd);
            printk("\tVPAGE: \t0x%x\n", ((struct dsrx_pgcnt* )DS_RXPGCNT)->upage);

    }else if(ifx_strincmp(tokens[0],"sll", strlen(tokens[0])) == 0){
        unsigned int keynum = ((struct sll_cmd1 *)SLL_CMD1)->ksize;

        printk("SLL_CMD0(0x901):\n");
            printk("\tCMD: \t0x%x\n", ((struct sll_cmd0* )SLL_CMD0)->cmd);
            printk("\tENUM: \t0x%x\n", ((struct sll_cmd0* )SLL_CMD0)->eynum);
            printk("\tEYBASE: \t0x%x\n", ((struct sll_cmd0* )SLL_CMD0)->eybase);
        printk("Key size: 0x%x\n", keynum);
        for(i = 0; i < keynum; i ++){
            printk("Key[%d]:    0x%x\n", i, *(uint32_t*)SLL_KEY(i));
        }
        printk("SLL_RESULT(0x920) \n");
            printk("\tVLD: \t0x%x\n", ((struct sll_result* )SLL_RESULT)->vld);
            printk("\tFOUND: \t0x%x\n", ((struct sll_result* )SLL_RESULT)->fo);
            printk("\tINDEX: \t0x%x\n", ((struct sll_result* )SLL_RESULT)->index);
    }else{
        fw_dbg_start("help dump_rg");
    }

    return;
}

static void fwdbg_route(char **tokens, unsigned int token_num)
{
    unsigned long addr, *paddr;
    unsigned int i;
    volatile struct rout_forward_action_tbl *action_tbl = NULL;
    int val[16], len;

    if(! token_num ){
        goto print_help;
    }

    if(ifx_strincmp(tokens[0], "help", strlen(tokens[0])) == 0){
            goto print_help;;
    }else{
            addr = simple_strtoul(tokens[0], NULL, 16);
            if(!addr){
                goto print_help;
            }
            paddr = (unsigned long *)sb_addr_to_fpi_addr_convert(addr);
            action_tbl = (struct rout_forward_action_tbl*)paddr;
    }

    for(i = 1; i < token_num - 1; i += 2){
        if(ifx_strincmp(tokens[i], "newMAC", strlen(tokens[i])) == 0){
            len = strlen(tokens[i + 1]);
            get_mac(&tokens[i + 1], &len, val);
//                      printk("new MAC: %02X.%02X.%02X.%02X:%02X:%02X\n", val[0], val[1], val[2], val[3], val[4], val[5]);
            action_tbl->new_dest_mac54 = (val[0] << 8) | val[1];
            action_tbl->new_dest_mac30 = (val[2] << 24) | (val[3] << 16) | (val[4] << 8) | val[5];

        }else if(ifx_strincmp(tokens[i], "newip", strlen(tokens[i])) == 0){
            len = strlen(tokens[i + 1]);
            get_ip_port(&tokens[i + 1], &len, val);
//                          printk("new dest: %d.%d.%d.%d:%d\n", val[0], val[1], val[2], val[3], val[4]);
            action_tbl->new_ip = (val[0] << 24) | (val[1] << 16) | (val[2] << 8) | val[3];
            action_tbl->new_port = val[4];
        }else if(ifx_strincmp(tokens[i], "routetype", strlen(tokens[i])) == 0){
            if(ifx_strincmp(tokens[i + 1], "NULL", strlen(tokens[i + 1])) == 0){
                action_tbl->rout_type = 0;

            }else if(ifx_strincmp(tokens[i + 1], "IP", strlen(tokens[i + 1])) == 0
                || ifx_strincmp(tokens[i + 1], "ROUTE", strlen(tokens[i + 1])) == 0){
                action_tbl->rout_type = 1;

            }if(ifx_strincmp(tokens[i + 1], "NAT", strlen(tokens[i + 1])) == 0){
                action_tbl->rout_type = 2;
            }if(ifx_strincmp(tokens[i + 1], "NAPT", strlen(tokens[i + 1])) == 0){
                action_tbl->rout_type = 3;
            }
        }else if(ifx_strincmp(tokens[i], "newDSCP", strlen(tokens[i])) == 0){
            action_tbl->new_dscp = simple_strtoul(tokens[i + 1], NULL, 0);
            //action_tbl->new_dscp_en = 1;

        }else if(ifx_strincmp(tokens[i], "MTUidx", strlen(tokens[i])) == 0){
            action_tbl->mtu_ix = simple_strtoul(tokens[i + 1], NULL, 0);

        }else if(ifx_strincmp(tokens[i], "VLANins", strlen(tokens[i])) == 0){
            action_tbl->in_vlan_ins = simple_strtoul(tokens[i + 1], NULL, 0);

        }else if(ifx_strincmp(tokens[i], "VLANrm", strlen(tokens[i])) == 0){
            action_tbl->in_vlan_rm = simple_strtoul(tokens[i + 1], NULL, 0);

        }else if(ifx_strincmp(tokens[i], "newDSCPen", strlen(tokens[i])) == 0){
            action_tbl->new_dscp_en = simple_strtoul(tokens[i + 1], NULL, 0);

        }else if(ifx_strincmp(tokens[i], "entry_vld", strlen(tokens[i])) == 0){
            action_tbl->entry_vld = simple_strtoul(tokens[i + 1], NULL, 0);

        }else if(ifx_strincmp(tokens[i], "TCP", strlen(tokens[i])) == 0){
            action_tbl->protocol = simple_strtoul(tokens[i + 1], NULL, 0);

        }else if(ifx_strincmp(tokens[i], "destlist", strlen(tokens[i])) == 0){
            if(ifx_strincmp(tokens[i + 1], "ETH0", strlen(tokens[i + 1])) == 0){
                action_tbl->dest_list |= 1 << 0;
            }else if(ifx_strincmp(tokens[i + 1], "ETH1", strlen(tokens[i + 1])) == 0){
                action_tbl->dest_list |= 1 << 1;
            }else if(ifx_strincmp(tokens[i + 1], "CPU0", strlen(tokens[i + 1])) == 0){
                action_tbl->dest_list |= 1 << 2;
            }else if(ifx_strincmp(tokens[i + 1], "EXT_INT1", strlen(tokens[i + 1])) == 0){
                action_tbl->dest_list |= 1 << 3;
            }else if(ifx_strincmp(tokens[i + 1], "EXT_INT2", strlen(tokens[i + 1])) == 0){
                action_tbl->dest_list |= 1 << 4;
            }else if(ifx_strincmp(tokens[i + 1], "EXT_INT3", strlen(tokens[i + 1])) == 0){
                action_tbl->dest_list |= 1 << 5;
            }else if(ifx_strincmp(tokens[i + 1], "DSL_WAN", strlen(tokens[i + 1])) == 0){
                action_tbl->dest_list |= 1 << 7;
            }
        }else if(ifx_strincmp(tokens[i], "PPPoEmode", strlen(tokens[i])) == 0){
            action_tbl->pppoe_mode = simple_strtoul(tokens[i + 1], NULL, 0);

        }else if(ifx_strincmp(tokens[i], "PPPoEidx", strlen(tokens[i])) == 0){
            action_tbl->pppoe_ix = simple_strtoul(tokens[i + 1], NULL, 0);

        }else if(ifx_strincmp(tokens[i], "newMACidx", strlen(tokens[i])) == 0){
            action_tbl->new_src_mac_ix = simple_strtoul(tokens[i + 1], NULL, 0);

        }else if(ifx_strincmp(tokens[i], "VCI", strlen(tokens[i])) == 0){
            action_tbl->new_in_vci = simple_strtoul(tokens[i + 1], NULL, 0);

        }else if(ifx_strincmp(tokens[i], "outvlanins", strlen(tokens[i])) == 0){
            action_tbl->out_vlan_ins = simple_strtoul(tokens[i + 1], NULL, 0);

        }else if(ifx_strincmp(tokens[i], "outvlanrm", strlen(tokens[i])) == 0){
            action_tbl->out_vlan_rm = simple_strtoul(tokens[i + 1], NULL, 0);

        }else if(ifx_strincmp(tokens[i], "outvlanidx", strlen(tokens[i])) == 0){
            action_tbl->out_vlan_ix = simple_strtoul(tokens[i + 1], NULL, 0);

        }else if(ifx_strincmp(tokens[i], "mpoatype", strlen(tokens[i])) == 0){
            action_tbl->mpoa_type = simple_strtoul(tokens[i + 1], NULL, 0);

        }else if(ifx_strincmp(tokens[i], "destqid", strlen(tokens[i])) == 0){
            action_tbl->dest_qid = simple_strtoul(tokens[i + 1], NULL, 0);

        }else if(ifx_strincmp(tokens[i], "tunnelidx", strlen(tokens[i])) == 0){
            action_tbl->tnnl_hdr_idx = simple_strtoul(tokens[i + 1], NULL, 0);

        }else if(ifx_strincmp(tokens[i], "tunnelen", strlen(tokens[i])) == 0){
            action_tbl->encap_tunnel = simple_strtoul(tokens[i + 1], NULL, 0);

        }
    }

    return;

print_help:

    printk("route address [name value] ... \n");
    printk("      newIP(PORT)       ??:??:??:??:????\n");
    printk("      newMAC:           ??:??:??:??:??:?? (HEX)\n");
    printk("      routetype:        NULL/IP/NAT/NAPT\n");
    printk("      newDSCP:          ??\n");
    printk("      newDSCPen:        0|1(disable|enable)\n");
    printk("      MTUidx:           ??\n");
    printk("      VLANins:          0|1(disable/enable)\n");
    printk("      VCI:              ???? (HEX)\n");
    printk("      VLANrm:           0|1(disable/enable)\n");
    printk("      outvlanins:       0|1(disable/enable)\n");
    printk("      outvlanidx:       ??\n");
    printk("      outvlanrm:        0|1(disable/enable)\n");
    printk("      destlist:         ETH0/ETH1/CPU0/EXT_INT/DSL_WAN?\n");
    printk("      PPPoEmode:        0|1(transparent/termination)\n");
    printk("      PPPoEidx:         ??\n");
    printk("      newMACidx:        ??\n");
    printk("      tcp:              0|1(no(UDP)/yes)\n");
    printk("      destqid:          ??\n");
    printk("      entryvld:         0|1(invalid/valid)\n");
    printk("      mpoatype:         ??\n");
    printk("      tunnelidx         ??\n");
    printk("      tunnelen          0|1(disable | enable)\n");

    return;
}

static void dump_ipv4_hdr(struct iphdr * iph)
{
    ASSERT(iph != NULL, "ipv4 hdr point is NULL");

    printk("Version         :%d\n", iph->version);
    printk("Header len      :%d\n", iph->ihl);
    printk("TOS             :%d\n", iph->tos);
    printk("Total len       :%d\n", iph->tot_len);
    printk("Id              :%d\n", iph->id);
    printk("Flags              \n");
    printk("  Don't Fragment:%d\n", iph->frag_off & (1 << 14));
    printk("  More Fragments:%d\n", iph->frag_off & (1 << 13));
    printk("Fragment offset :%d\n", iph->frag_off & ~0x0700);
    printk("TTL             :%d\n", iph->ttl);
    printk("Protocol        :%d\n", iph->protocol);
    printk("Header checksum :%d\n", iph->check);
    printk("Src IP address  :%d.%d.%d.%d\n", iph->saddr >> 24,iph->saddr >> 16 & 0xFF,
                                             iph->saddr >> 8 & 0xFF, iph->saddr & 0xFF);
    printk("Dst IP address  :%d.%d.%d.%d\n", iph->daddr >> 24,iph->daddr >> 16 & 0xFF,
                                             iph->daddr >> 8 & 0xFF, iph->daddr & 0xFF);

    return;
}

static void dump_ipv6_hdr(struct ipv6hdr *iph6)
{
    int i;
    ASSERT(iph6 != NULL, "ipv6 hdr point is NULL");

    printk("Version         :%d\n", iph6->version);
    printk("Traffic clase   :%d\n", iph6->priority << 4 | (iph6->flow_lbl[0] & 0xFF00));
    printk("Flow label      :%d\n", (iph6->flow_lbl[0] & 0xFF)<<16 | (iph6->flow_lbl[1] << 8) | iph6->flow_lbl[2]);
    printk("Payload Len     :%d\n", iph6->payload_len);
    printk("Next Header     :%d\n", iph6->nexthdr);
    printk("Hop limit       :%d\n", iph6->hop_limit);
    printk("Src Address     \n");
    for( i  = 0; i < 16; i ++){
        printk("%s%x", i == 0 ? "    " : i % 4 == 0 ? "." : "", (unsigned int)iph6->saddr.s6_addr + i);
    }
    printk("\n");
    printk("Dst Address     \n");
    for( i  = 0; i < 16; i ++){
        printk("%s%x", i == 0 ? "    " : i % 4 == 0 ? "." : "", (unsigned int)iph6->daddr.s6_addr + i);
    }
    printk("\n");

    return;
}

static void fwdbg_dump_iphdr(char **tokens, unsigned int token_num)
{
    unsigned long addr;
    unsigned long *paddr;

    if(token_num < 2){
        goto print_help;
    }

    addr = simple_strtoul(tokens[1], NULL, 16);
    if(!addr){
        goto print_help;
    }
    paddr = (unsigned long *)sb_addr_to_fpi_addr_convert(addr);

    if(ifx_strincmp(tokens[0], "ipv4", strlen(tokens[0])) == 0){
        dump_ipv4_hdr((struct iphdr *)paddr);
    }else if(ifx_strincmp(tokens[0], "ipv6", strlen(tokens[0])) == 0){
        dump_ipv6_hdr((struct ipv6hdr *)paddr);
    }else{
        goto print_help;
    }

    return;
print_help:
    fw_dbg_start("help dump_ip_hdr");
    return;
}

#if defined(CONFIG_AR10)
static void fwdbg_read_rx_cb(char **tokens, unsigned int token_num)
{
    unsigned int pg_num = 0;
    unsigned int pg_idx = 0;
    unsigned int bc_idx = 0;
    unsigned int pg_max_num;
    unsigned int idx;
    volatile unsigned int *dbase, *cbase, *cw;
    volatile struct ATM_CW_CTRL *p_ctrl;
    const int cw_page_size = 14; //one page has 14 codeword

    if(token_num < 1 || token_num > 3){
        fw_dbg_start("help read_rx_cb");
        return;
    }

    idx = 0;
    if(ifx_strincmp("bc0", tokens[idx],3) == 0){
        bc_idx = 0;
    }
    else{
        bc_idx = 1;
    }

    pg_max_num = ((struct SFSM_cfg *)SFSM_CFG(bc_idx))->pnum;

    idx ++;
    if(token_num > idx){
        pg_idx = simple_strtoul(tokens[idx], NULL, 0);
        if(pg_idx >= pg_max_num){
            printk("start page no [%d] is bigger than max page no [%d] \n",
                pg_idx, pg_max_num);
            return;
        }
    }

    idx ++;
    if(token_num > idx){
        pg_num = simple_strtoul(tokens[idx], NULL, 0);
        if(pg_num >= pg_max_num){
            pg_num = pg_max_num;
        }
    }

    if(!pg_num){
        pg_num = pg_max_num;
    }

    printk("ATM RX BC[%d] CELL data/ctrl buffer:\n", bc_idx);
    printk("Start from page[%d], page nums:%d\n", pg_idx, pg_num);

    dbase = SB_BUFFER(((struct SFSM_dba*)SFSM_DBA(bc_idx))->dbase + 0x2000);
    cbase = SB_BUFFER(((struct SFSM_cba*)SFSM_CBA(bc_idx))->cbase + 0x2000);

    for(idx = 0; idx < pg_num; idx ++){
        cw = dbase + ((pg_idx + idx) % pg_max_num) * cw_page_size;
        p_ctrl = (volatile struct ATM_CW_CTRL *)(cbase + (pg_idx + idx) % pg_max_num);

        printk("PAGE %2d:%08x %08x                         ", idx+pg_idx, cw[0], cw[1]);
        printk("cvm=%d [%s], cvc=%d, bsm=%d, csp=%d\n",
                p_ctrl->cvm, p_ctrl->cvm == 0 ? "No_match": "match",
                p_ctrl->cvc, p_ctrl->bsm, p_ctrl->csp);
        printk("       :%08x %08x %08x %08x         ", cw[2],cw[3],cw[4],cw[5]);
        printk("idle=%d, drop=%d, ber=%d, state=%d\n",
                p_ctrl->idle, p_ctrl->drop, p_ctrl->ber, p_ctrl->state);
        printk("       :%08x %08x %08x %08x \n",  cw[6], cw[7],cw[8],cw[9]);
        printk("       :%08x %08x %08x %08x \n",  cw[10], cw[11],cw[12],cw[13]);
    }

    return;

}

static void fwdbg_read_tx_cb(char * * tokens, unsigned int token_num)
{
    unsigned int pg_num = 0;
    unsigned int pg_idx = 0;
    unsigned int bc_idx = 0;
    unsigned int pg_max_num;
    unsigned int idx;
    volatile unsigned int *dbase, *cw;
    const int cw_page_size = 14; //one page has 14 codeword

    //[bc0|bc1] [start_pg] [print_pg_num]

    if(token_num < 1 || token_num > 3){
        fw_dbg_start("help read_tx_cb");
        return;
    }

    idx = 0;
    if(ifx_strincmp("bc0", tokens[idx],3) == 0){
        bc_idx = 0;
    }
    else{
        bc_idx = 1;
    }

    pg_max_num = ((struct FFSM_cfg *)FFSM_CFG(bc_idx))->pnum;

    idx ++;
    if(token_num > idx){
        pg_idx = simple_strtoul(tokens[idx], NULL, 0);
        if(pg_idx >= pg_max_num){
            printk("start page no [%d] is bigger than max page no [%d] \n",
                pg_idx, pg_max_num);
            return;
        }
    }

    idx ++;
    if(token_num > idx){
        pg_num = simple_strtoul(tokens[idx], NULL, 0);
        if(pg_num >= pg_max_num){
            pg_num = pg_max_num;
        }
    }

    if(!pg_num){
        pg_num = pg_max_num;
    }

    printk("ATM TX BC[%d] CELL data/ctrl buffer:\n\n", bc_idx);
    printk("Start from page[%d], page nums:%d\n", pg_idx, pg_num);

    dbase = SB_BUFFER(((struct FFSM_dba*)FFSM_DBA(bc_idx))->dbase + 0x2000);

    for(idx = 0; idx < pg_num; idx ++){
        cw = dbase + ((pg_idx + idx) % pg_max_num) * cw_page_size;

        printk("PAGE %2d:%08x %08x\n",            idx + pg_idx,  cw[0], cw[1]);
        printk("       :%08x %08x %08x %08x \n",  cw[2],  cw[3], cw[4], cw[5]);
        printk("       :%08x %08x %08x %08x \n",  cw[6],  cw[7], cw[8], cw[9]);
        printk("       :%08x %08x %08x %08x \n",  cw[10], cw[11],cw[12],cw[13]);
    }

    return;
}

static void fwdbg_clear_cb(char * * tokens, unsigned int token_num)
{
    unsigned int pg_num = 0;
    unsigned int pg_idx = 0;
    unsigned int bc_idx = 0;
    unsigned int pg_max_num;
    unsigned int is_rx;
    unsigned int idx;
    volatile unsigned int *dbase, *cbase, *cw;
    const int cw_page_size = 14; //one page has 14 codeword

    //[bc0|bc1] [start_pg] [print_pg_num]

    if(token_num < 2 || token_num > 4){
        goto PARAM_ERROR;
    }

    idx = 0;
    if(ifx_strincmp("rx", tokens[idx], 2) == 0){
        is_rx = 1;
    }else if(ifx_strincmp("tx", tokens[idx],2) == 0){
        is_rx = 0;
    }else{
        goto PARAM_ERROR;
    }

    idx ++;
    if(ifx_strincmp("bc0", tokens[idx],3) == 0){
        bc_idx = 0;
    }
    else{
        bc_idx = 1;
    }

    if(is_rx){
        pg_max_num = ((struct SFSM_cfg *)SFSM_CFG(bc_idx))->pnum;
    }
    else{
        pg_max_num = ((struct FFSM_cfg *)FFSM_CFG(bc_idx))->pnum;
    }

    idx ++;
    if(token_num > idx){
        pg_idx = simple_strtoul(tokens[idx], NULL, 0);

        if(pg_idx >= pg_max_num){
            printk("start page no [%d] is bigger than max page no [%d] \n",
                pg_idx, pg_max_num);
            goto PARAM_ERROR;
        }
    }

    idx ++;
    if(token_num > idx){
        pg_num = simple_strtoul(tokens[idx], NULL, 0);
        if(pg_num >= pg_max_num){
            pg_num = pg_max_num;
        }
    }

    if(!pg_num){
        pg_num = pg_max_num;
    }

    if(is_rx){
        dbase = SB_BUFFER(((struct SFSM_dba*)SFSM_DBA(bc_idx))->dbase + 0x2000);
        cbase = SB_BUFFER(((struct SFSM_cba*)SFSM_CBA(bc_idx))->cbase + 0x2000);
    }else{
        dbase = SB_BUFFER(((struct FFSM_dba*)FFSM_DBA(bc_idx))->dbase + 0x2000);
        cbase = NULL;
    }

    for(idx = 0; idx < pg_num; idx ++){
        cw = dbase + ((pg_idx + idx) % pg_max_num) * cw_page_size;

        memset((void *)cw, 0, sizeof(unsigned int) * cw_page_size);
        if(cbase){
            cw = cbase + (pg_idx + idx) % pg_max_num;
            memset((void *)cw, 0, sizeof(unsigned int));
        }
    }

    return;

PARAM_ERROR:
    fw_dbg_start("help clear_cb");
    return;
}

#endif // #if defined(CONFIG_AR10)


static void fwdbg_help(char **tokens, unsigned int token_num)
{
    const char *proc_file = "/proc/eth/tfwdbg";
    const char *cmds[] = {
            "help","route",
            "read_ethbuf", "read_rx_cb","read_tx_cb",
            "clear_cb", "clear_ethbuf",
            "dump_st_hdr", "dump_ip_hdr","dump_rg",
            NULL,
    };

    int i;

    if(!token_num){//print commands only
        for(i = 0; cmds[i] != NULL; i ++){
            printk(cmds[i]);
            printk("\t");
            if(i % 3 == 0)
                printk("\n");
        }

        printk("\n\n");
        printk("echo help cmd > %s for details\n", proc_file);
        return;
    }

    if(ifx_strincmp(tokens[0], "read_ethbuf", strlen(tokens[0])) == 0){
        printk("echo read_ethbuf [start_pg] [print_pg_num] > %s \n", proc_file);
        printk("   :clear eth rx data and control buffer  \n");

    }
#if defined(CONFIG_AR10)
    else if(ifx_strincmp(tokens[0], "read_rx_cb", strlen(tokens[0])) == 0){
        printk("echo read_rx_cb [bc0|bc1] [start_pg] [print_pg_num] > %s \n", proc_file);
        printk("   :read rx cell/codeword buffer for bc0 or bc1  \n");

    }else if(ifx_strincmp(tokens[0], "read_tx_cb", strlen(tokens[0])) == 0){
        printk("echo read_tx_cb [bc0|bc1] [start_pg] [print_pg_num] > %s \n", proc_file);
        printk("   :read tx cell/codeword buffer for bc0 or bc1  \n");

    }
#endif // #if defined(CONFIG_AR10)
#if defined(CONFIG_VR9)
    else if(ifx_strincmp(tokens[0], "read_cb", strlen(tokens[0])) == 0){
            printk("echo read_rx_cb [rx|tx] [bc0|bc1] [start_pg] [print_pg_num] > %s \n", proc_file);
            printk("   :read rx or tx (default is both rx & tx, bare channel 0 & 1) cell/codeword buffer for bc0 or bc1  \n");

    }
#endif // #if defined(CONFIG_VR9)
    else if(ifx_strincmp(tokens[0], "clear_cb", strlen(tokens[0])) == 0){
        printk("echo clear_cb [rx|tx] [bc0|bc1] [start_pg] [clear_pg_num] > %s \n", proc_file);
        printk("   :clear rx or tx (or both) cell/codeword buffer for bc0 or bc1  \n");

    }else if(ifx_strincmp(tokens[0], "clear_ethbuf", strlen(tokens[0])) == 0){
        printk("echo clear_ethbuf [start_pg] [pg_num] > %s \n", proc_file);
        printk("   :clear eth rx data and control buffer  \n");

    }else if(ifx_strincmp(tokens[0], "dump_st_hdr", strlen(tokens[0])) == 0){
        printk("echo dump_st_hdr start_pg(Hex) [iphdr]> %s \n", proc_file);
        printk("   :dump status header of the packet in the provided page and dump ipv4/ipv6 header indicated by flag header  \n");

    }else if(ifx_strincmp(tokens[0], "dump_ip_hdr", strlen(tokens[0])) == 0){
        printk("echo dump_ip_hdr [ipv4 | ipv6] [share_buffer_address] [idx] > %s \n", proc_file);
        printk("   :dump ipv4 or ipv6 header in the given share buffer  \n");

    }else if(ifx_strincmp(tokens[0], "dump_rg", strlen(tokens[0])) == 0){
        printk("echo dump_rg [dmrx | dsrx | sll] %s \n", proc_file);
        printk("   :dump given registers  \n");

    }else if(ifx_strincmp(tokens[0], "route", strlen(tokens[0])) == 0){
        printk("echo route address [name value] %s \n",proc_file);
        printk("   :change route entry's contents \n");
        printk("   :echo route help for details \n");
    }else{
        fwdbg_help((char **)NULL, 0);
    }

}

static void fw_dbg_start(char *cmdbuf) {
    char * tokens[32];
    int finished, token_num;
    int i;

    fw_dbg_t cmds[]={{"help",fwdbg_help},
                     {"read_ethbuf", fwdbg_read_ethbuf},
                     {"clear_ethbuf",fwdbg_clear_ethbuf},
                     {"dump_st_hdr", fwdbg_dump_sthdr},
                     {"dump_rg",fwdbg_dump_rg},
                     {"route", fwdbg_route},
                     {"dump_ip_hdr", fwdbg_dump_iphdr},
#if defined(CONFIG_AR10)
                     {"read_rx_cb",  fwdbg_read_rx_cb},
                     {"read_tx_cb",  fwdbg_read_tx_cb},
                     {"clear_cb",    fwdbg_clear_cb},
#endif
                     {NULL, NULL}};


    token_num = tokenize(cmdbuf, " \t", tokens, 32, &finished);
    if (token_num <= 0) {
        return;
    }

    for(i = 0; cmds[i].cmd != NULL; i ++){
        if(ifx_strincmp(tokens[0], cmds[i].cmd, strlen(tokens[0])) == 0){
            cmds[i].pfunc(&tokens[1],token_num - 1);
            break;
        }
    }

    if(cmds[i].cmd == NULL){
        fw_dbg_start("help");
    }

    return;
}


static int proc_fwdbg_input(char *line, void *data __maybe_unused){

    fw_dbg_start(line);
    return 0;
}
#endif // #if defined(DEBUG_FW_PROC) && DEBUG_FW_PROC

#if defined(DEBUG_SEND_PROC) && DEBUG_SEND_PROC
static int ppe_send_cell(int conn, void *cell) {
	struct sk_buff *skb;
	unsigned long sys_flag;
	volatile struct tx_descriptor *desc;
	struct tx_descriptor reg_desc;
	struct sk_buff *skb_to_free;

	/*  allocate sk_buff    */
	skb = alloc_skb_tx(DMA_PACKET_SIZE);
	if (skb == NULL)
		return -ENOMEM;
	ATM_SKB(skb)->vcc = NULL;

	/*  copy data   */
	skb_put(skb, CELL_SIZE);
	my_memcpy(skb->data, cell, CELL_SIZE);

#ifndef CONFIG_MIPS_UNCACHED
	/*  write back to physical memory   */
	dma_cache_wback((u32)skb->data, skb->len);
#endif

	/*  allocate descriptor */
    spin_lock_irqsave(&g_cpu_to_wan_tx_spinlock, sys_flag);
	desc = CPU_TO_WAN_TX_DESC_BASE + g_cpu_to_wan_tx_desc_pos;
	if (!desc->own) //  PPE hold
	{
        spin_unlock_irqrestore(&g_cpu_to_wan_tx_spinlock, sys_flag);
		err("Cell not alloc tx connection");
		return -EIO;
	}
	if (++g_cpu_to_wan_tx_desc_pos == CPU_TO_WAN_TX_DESC_NUM)
		g_cpu_to_wan_tx_desc_pos = 0;
    spin_unlock_irqrestore(&g_cpu_to_wan_tx_spinlock, sys_flag);

	/*  load descriptor from memory */
	reg_desc = *desc;

	/*  free previous skb   */
	ASSERT((reg_desc.dataptr & (DMA_TX_ALIGNMENT - 1)) == 0,
			"reg_desc.dataptr (0x%#x) must be 8 DWORDS aligned",
			reg_desc.dataptr);
	free_skb_clear_dataptr(&reg_desc);
	put_skb_to_dbg_pool(skb);

	/*  setup descriptor    */
	reg_desc.dataptr = (u32) skb->data & (0x1FFFFFFF ^ (DMA_TX_ALIGNMENT - 1)); //  byte address
	reg_desc.byteoff = (u32) skb->data & (DMA_TX_ALIGNMENT - 1);
	reg_desc.datalen = skb->len;
	reg_desc.mpoa_pt = 1;
	reg_desc.mpoa_type = 0; //  this flag should be ignored by firmware
	reg_desc.pdu_type = 1; //  cell
	reg_desc.qid = conn;
	reg_desc.own = 0;
	reg_desc.c = 0;
	reg_desc.sop = reg_desc.eop = 1;

	dump_atm_tx_desc(&reg_desc);
	if (g_dbg_datapath & DBG_ENABLE_MASK_DUMP_ATM_TX)
		dump_skb(skb, DUMP_SKB_LEN, "ppe_send_cell", 7, 0, 1, 1);

	/*  write discriptor to memory and write back cache */
	*((volatile u32 *) desc + 1) = *((u32 *) &reg_desc + 1);
	*(volatile u32 *) desc = *(u32 *) &reg_desc;

	adsl_led_flash();

	return 0;
}

static void proc_send_output(struct seq_file *m, void *private_data __maybe_unused){
	seq_printf(m, "echo \"[<qid> qid] [<pvc> vpi.vci] <dword0> <dword1> ...... <dword12>\" > /proc/eth/send\n");
}

static int proc_send_input(char *line, void *data __maybe_unused){
	int conn = -1;
	int dword_counter;
	unsigned int cell[16];
	int ret;
	int vpi = 0, vci = 0;
	int f_enable_irq = 0;
	int f_continue_send = 0;

	int rlen = strlen(line);
	char *p = line;

	if (ifx_strincmp(p, "qid ", 4) == 0) {
		p += 4;
		rlen -= 4;
		ifx_ignore_space(&p, &rlen);

		conn = ifx_get_number(&p, &rlen, 0);
	} else if (ifx_strincmp(p, "pvc ", 4) == 0) {
		p += 4;
		rlen -= 4;
		ifx_ignore_space(&p, &rlen);

		vpi = ifx_get_number(&p, &rlen, 0);
		if (rlen && *p == '.') {
			p++;
			rlen--;
		} else
			ifx_ignore_space(&p, &rlen);
		vci = ifx_get_number(&p, &rlen, 0);
		conn = find_vpivci(vpi, vci);
		if (conn < 0)
			printk("no such pvc %d.%d\n", vpi, vci);
	}

	if (!f_continue_send) {
		if (conn >= 0 && conn <= 14) {
			for (dword_counter = 0;
					dword_counter < sizeof(cell) / sizeof(cell[0]);
					dword_counter++) {
				ifx_ignore_space(&p, &rlen);
				if (rlen <= 0)
					break;
				cell[dword_counter] = ifx_get_number(&p, &rlen, 1);
			}
			if (dword_counter < 13)
				printk("not enough data to compose cell\n");
			else {
				if (!g_atm_priv_data.connection[conn].vcc) {
					struct wrx_queue_config wrx_queue_config = { 0 };
					struct wtx_queue_config wtx_queue_config = { 0 };
					struct uni_cell_header *cell_header;

					printk("need open queue by self\n");
					/*  setup RX queue cfg and TX queue cfg */
					wrx_queue_config.mpoa_type = 3; //  set IPoA as default
					wrx_queue_config.ip_ver = 0; //  set IPv4 as default
					wrx_queue_config.mpoa_mode = 0; //  set VC-mux as default
					wrx_queue_config.oversize = aal5r_max_packet_size;
					wrx_queue_config.undersize = aal5r_min_packet_size;
					wrx_queue_config.mfs = aal5r_max_packet_size;
					wtx_queue_config.same_vc_qmap = 0x00; //  up to now, one VC with multiple TX queue has not been implemented
					wtx_queue_config.sbid =
							g_atm_priv_data.connection[conn].port;
					wtx_queue_config.qsb_vcid = conn + QSB_QUEUE_NUMBER_BASE; //  qsb qid = firmware qid + 1
					wtx_queue_config.mpoa_mode = 0; //  set VC-mux as default
					wtx_queue_config.qsben = 1; //  reserved in A4, however put 1 for backward compatible
					cell_header =
							(struct uni_cell_header *) ((u32 *) &wtx_queue_config
									+ 1);
					cell_header->clp = 0;
					cell_header->pti = ATM_PTI_US0;
					cell_header->vci = 1;
					cell_header->vpi = 4;
					cell_header->gfc = 0;
					*WRX_QUEUE_CONFIG(conn) = wrx_queue_config;
					*WTX_QUEUE_CONFIG(conn) = wtx_queue_config;
					if (g_atm_priv_data.connection_table == 0) {
						f_enable_irq = 1;
						// turn_on_dma_rx(31);
					}
				}

				if (dword_counter > 13)
					printk("too much data, last %d dwords ignored\n",
							dword_counter - 13);
				if ((ret = ppe_send_cell(conn, cell)))
					printk("ppe_send_cell(%d, %08x) return %d\n", conn,
							(unsigned int) cell, ret);

				if (f_enable_irq) {
					volatile int j;
					for (j = 0; j < 100000; j++)
						;
					//turn_off_dma_rx(31); dma laueft immer
				}
			}
		} else
			printk("the queue (%d) is wrong\n", conn);
	}
	return 0;
}

#endif // #if defined(DEBUG_SEND_PROC) && DEBUG_SEND_PROC


#if defined(DATAPATH_7PORT)

#define RX_TOTAL_PCKNT                  0x1F
#define RX_UNICAST_PCKNT                0x23
#define RX_MULTICAST_PCKNT              0x22
#define RX_CRC_ERR_PCKNT                0x21
#define RX_UNDERSIZE_GOOD_PCKNT         0x1D
#define RX_OVER_SIZE_GOOD_PCKNT         0x1B
#define RX_UNDERSIZE_ERR_PCKNT          0x1E
#define RX_GOOD_PAUSE_PCKNT             0x20
#define RX_OVER_SIZE_ERR_PCKNT          0x1C
#define RX_ALLIGN_ERR_PCKNT             0x1A
#define RX_FILTERED_PCKNT               0x19
#define RX_DISCARD_CONGESTION_PCKNT     0x11

#define RX_SIZE_64B_PCKNT               0x12
#define RX_SIZE_65_127B_PCKNT           0x13
#define RX_SIZE_128_255B_PCKNT          0x14
#define RX_SIZE_256_511B_PCKNT          0x15
#define RX_SIZE_512_1023B_PCKNT         0x16
#define RX_SIZE_1024B_MAX_PCKNT         0x17

#define TX_TOTAL_PCKNT                  0x0C
#define TX_UNICAST_PCKNT                0x06
#define TX_MULTICAST_PCKNT              0x07

#define TX_SINGLE_COLLISION_CNT         0x08
#define TX_MULTIPLE_COLLISION_CNT       0x09
#define TX_LATE_COLLISION_CNT           0x0A
#define TX_EXCESSIVE_COLLISION_CNT      0x0B
#define TX_PAUSE_PCKNT                  0x0D

#define TX_SIZE_64B_PCKNT               0x00
#define TX_SIZE_65_127B_PCKNT           0x01
#define TX_SIZE_128_255B_PCKNT          0x02
#define TX_SIZE_256_511B_PCKNT          0x03
#define TX_SIZE_512_1023B_PCKNT         0x04
#define TX_SIZE_1024B_MAX_PCKNT         0x05

#define TX_DROP_PCKNT                   0x10
#define RX_DROP_PCKNT                   0x18

#define RX_GOOD_BYTE_CNT_LOW            0x24
#define RX_GOOD_BYTE_CNT_HIGH           0x25

#define RX_BAD_BYTE_CNT_LOW             0x26
#define RX_BAD_BYTE_CNT_HIGH            0x27

#define TX_GOOD_BYTE_CNT_LOW            0x0E
#define TX_GOOD_BYTE_CNT_HIGH           0x0F

static INLINE unsigned int sw_get_rmon_counter(int port, int addr) {
	*ETHSW_BM_RAM_ADDR_REG = addr;
	*ETHSW_BM_RAM_CTRL_REG = 0x8000 | port;
	while ((*ETHSW_BM_RAM_CTRL_REG & 0x8000))
		;

	return (*ETHSW_BM_RAM_VAL_1_REG << 16) | (*ETHSW_BM_RAM_VAL_0_REG & 0xFFFF);
}

static INLINE void sw_clr_rmon_counter(int port) {
	int i;

	if (port >= 0 && port < 7) {
		*ETHSW_BM_PCFG_REG(port) = 0;
		*ETHSW_BM_RMON_CTRL_REG(port) = 3;
		for (i = 1000; (*ETHSW_BM_RMON_CTRL_REG(port) & 3) != 0 && i > 0; i--)
			;
		if (i == 0)
			*ETHSW_BM_RMON_CTRL_REG(port) = 0;
		*ETHSW_BM_PCFG_REG(port) = 1;
	}
}

static void proc_port_mib_output(struct seq_file *m, void *private_data ){

   int port = *(unsigned char*)private_data;
	unsigned int counter;
	unsigned long long byteCnt;

	seq_printf(m, "\n\tPort [%d] counters\n\n", port);

	seq_printf(m, "Rx Total PckCnt     :");
	counter = sw_get_rmon_counter(port, RX_TOTAL_PCKNT);
	seq_printf(m, "0x%08x\t", counter);
	seq_printf(m, "Rx Unicast PckCnt   :");
	counter = sw_get_rmon_counter(port, RX_UNICAST_PCKNT);
	seq_printf(m, "0x%08x\t", counter);
	seq_printf(m, "Rx Multicast PckCnt :");
	counter = sw_get_rmon_counter(port, RX_MULTICAST_PCKNT);
	seq_printf(m, "0x%08x\n", counter);

	seq_printf(m, "Tx Total PckCnt     :");
	counter = sw_get_rmon_counter(port, TX_TOTAL_PCKNT);
	seq_printf(m, "0x%08x\t", counter);
	seq_printf(m, "Tx Unicase PckCnt   :");
	counter = sw_get_rmon_counter(port, TX_UNICAST_PCKNT);
	seq_printf(m, "0x%08x\t", counter);
	seq_printf(m, "Tx Multicase PckCnt :");
	counter = sw_get_rmon_counter(port, TX_MULTICAST_PCKNT);
	seq_printf(m, "0x%08x\n", counter);

	seq_printf(m, "Rx 64B PckCnt       :");
	counter = sw_get_rmon_counter(port, RX_SIZE_64B_PCKNT);
	seq_printf(m, "0x%08x\t", counter);
	seq_printf(m, "Rx [65-127B] PckCnt :");
	counter = sw_get_rmon_counter(port, RX_SIZE_65_127B_PCKNT);
	seq_printf(m, "0x%08x\t", counter);
	seq_printf(m, "Rx [128~255B] PckCnt:");
	counter = sw_get_rmon_counter(port, RX_SIZE_128_255B_PCKNT);
	seq_printf(m, "0x%08x\n", counter);
	seq_printf(m, "Rx [256~511B] PckCnt:");
	counter = sw_get_rmon_counter(port, RX_SIZE_256_511B_PCKNT);
	seq_printf(m, "0x%08x\t", counter);
	seq_printf(m, "Rx [512~1023B]PckCnt:");
	counter = sw_get_rmon_counter(port, RX_SIZE_512_1023B_PCKNT);
	seq_printf(m, "0x%08x\t", counter);
	seq_printf(m, "Rx [ >1024B] PckCnt :");
	counter = sw_get_rmon_counter(port, RX_SIZE_1024B_MAX_PCKNT);
	seq_printf(m, "0x%08x\n", counter);

	seq_printf(m, "Tx [64B] PckCnt     :");
	counter = sw_get_rmon_counter(port, TX_SIZE_64B_PCKNT);
	seq_printf(m, "0x%08x\t", counter);
	seq_printf(m, "Tx [65~127B] PckCnt :");
	counter = sw_get_rmon_counter(port, TX_SIZE_65_127B_PCKNT);
	seq_printf(m, "0x%08x\t", counter);
	seq_printf(m, "Tx [128~255B] PckCnt:");
	counter = sw_get_rmon_counter(port, TX_SIZE_128_255B_PCKNT);
	seq_printf(m, "0x%08x\n", counter);
	seq_printf(m, "Tx [256~511B] PckCnt:");
	counter = sw_get_rmon_counter(port, TX_SIZE_256_511B_PCKNT);
	seq_printf(m, "0x%08x\t", counter);
	seq_printf(m, "Tx [512~1023B]PckCnt:");
	counter = sw_get_rmon_counter(port, TX_SIZE_512_1023B_PCKNT);
	seq_printf(m, "0x%08x\t", counter);
	seq_printf(m, "Tx [>1024B] PckCnt  :");
	counter = sw_get_rmon_counter(port, TX_SIZE_1024B_MAX_PCKNT);
	seq_printf(m, "0x%08x\n", counter);

	seq_printf(m, "Rx CRC err PckCnt   :");
	counter = sw_get_rmon_counter(port, RX_CRC_ERR_PCKNT);
	seq_printf(m, "0x%08x\t", counter);

	seq_printf(m, "Rx Unsize err PCkCnt:");
	counter = sw_get_rmon_counter(port, RX_UNDERSIZE_ERR_PCKNT);
	seq_printf(m, "0x%08x\t", counter);

	seq_printf(m, "Rx Ovsize err PckCnt:");
	counter = sw_get_rmon_counter(port, RX_OVER_SIZE_ERR_PCKNT);
	seq_printf(m, "0x%08x\n", counter);

	seq_printf(m, "Rx UnsizeGood PckCnt:");
	counter = sw_get_rmon_counter(port, RX_UNDERSIZE_GOOD_PCKNT);
	seq_printf(m, "0x%08x\t", counter);

	seq_printf(m, "Rx OvsizeGood PckCnt:");
	counter = sw_get_rmon_counter(port, RX_OVER_SIZE_GOOD_PCKNT);
	seq_printf(m, "0x%08x\t", counter);

	seq_printf(m, "Rx Good Pause PckCnt:");
	counter = sw_get_rmon_counter(port, RX_GOOD_PAUSE_PCKNT);
	seq_printf(m, "0x%08x\n", counter);

	seq_printf(m, "Rx Align err PckCnt :");
	counter = sw_get_rmon_counter(port, RX_ALLIGN_ERR_PCKNT);
	seq_printf(m, "0x%08x\t", counter);

	seq_printf(m, "Rx filterd errPckCnt:");
	counter = sw_get_rmon_counter(port, RX_FILTERED_PCKNT);
	seq_printf(m, "0x%08x\t", counter);

	seq_printf(m, "Tx Single col Cnt   :");
	counter = sw_get_rmon_counter(port, TX_SINGLE_COLLISION_CNT);
	seq_printf(m, "0x%08x\n", counter);

	seq_printf(m, "Tx Multiple col Cnt :");
	counter = sw_get_rmon_counter(port, TX_MULTIPLE_COLLISION_CNT);
	seq_printf(m, "0x%08x\t", counter);

	seq_printf(m, "Tx Late col  Cnt    :");
	counter = sw_get_rmon_counter(port, TX_LATE_COLLISION_CNT);
	seq_printf(m, "0x%08x\t", counter);

	seq_printf(m, "Tx Excessive col Cnt:");
	counter = sw_get_rmon_counter(port, TX_EXCESSIVE_COLLISION_CNT);
	seq_printf(m, "0x%08x\n", counter);

	seq_printf(m, "Tx  Pause Cnt       :");
	counter = sw_get_rmon_counter(port, TX_PAUSE_PCKNT);
	seq_printf(m, "0x%08x\n", counter);

	seq_printf(m, "Tx Drop PckCnt      :");
	counter = sw_get_rmon_counter(port, TX_DROP_PCKNT);
	seq_printf(m, "0x%08x\t", counter);

	seq_printf(m, "Rx Drop PckCnt      :");
	counter = sw_get_rmon_counter(port, RX_DROP_PCKNT);
	seq_printf(m, "0x%08x\n", counter);

	seq_printf(m, "Rx Good Byte Cnt    :");
	byteCnt = sw_get_rmon_counter(port, RX_GOOD_BYTE_CNT_HIGH);
	byteCnt <<= 32;
	byteCnt += sw_get_rmon_counter(port, RX_GOOD_BYTE_CNT_LOW);
	seq_printf(m, "0x%llx \t", byteCnt);

	seq_printf(m, "Tx Good Byte Cnt    :");
	byteCnt = sw_get_rmon_counter(port, TX_GOOD_BYTE_CNT_HIGH);
	byteCnt <<= 32;
	byteCnt += sw_get_rmon_counter(port, TX_GOOD_BYTE_CNT_LOW);
	seq_printf(m, "0x%llx \n", byteCnt);

	seq_printf(m, "Rx Bad Byte Cnt     :");
	byteCnt = sw_get_rmon_counter(port, RX_BAD_BYTE_CNT_HIGH);
	byteCnt <<= 32;
	byteCnt += sw_get_rmon_counter(port, RX_BAD_BYTE_CNT_LOW);
	seq_printf(m, "0x%llx \t", byteCnt);

	seq_printf(m, "Rx Discard Cong Cnt :");
	byteCnt = sw_get_rmon_counter(port, RX_DISCARD_CONGESTION_PCKNT);
	seq_printf(m, "0x%llx \n", byteCnt);

}

static int proc_port_mib_input(char *line, void *data){
	int port;
	char *p = line;

	if (ifx_stricmp(p, "clean all") == 0 || ifx_stricmp(p, "clear all") == 0) {
		for (port = 0; port < 7; port++)
			sw_clr_rmon_counter(port);

	} else if (ifx_stricmp(p, "clean") == 0 || ifx_stricmp(p, "clear") == 0) {
		port = *(unsigned char*)data;
		if (port >= 0 && port < 7)
			sw_clr_rmon_counter(port);
	}
	return 0;
}
#endif // #if defined(DATAPATH_7PORT)


static void proc_directforwarding_output(struct seq_file *m, void *private_data __maybe_unused){
	seq_printf(m, "ingress direct forwarding - %s\n",
			g_ingress_direct_forward_enable ? "enable" : "disable");
	seq_printf(m, "egress direct forwarding  - %s\n",
			g_egress_direct_forward_enable ? "enable" : "disable");
}

static int proc_directforwarding_input(char *line, void *data __maybe_unused){

	if (ifx_stricmp(line, "ingress enable") == 0) {
		if (!g_ingress_direct_forward_enable) {
			g_ingress_direct_forward_enable = 1;
			//egress is enabled in eth_xmit
		}
	} else if (ifx_stricmp(line, "egress enable") == 0) {
		if (!g_egress_direct_forward_enable) {
			g_egress_direct_forward_enable = 1;
			//egress is enabled in eth_xmit
		}
	} else if (ifx_stricmp(line, "ingress disable") == 0) {
		if (g_ingress_direct_forward_enable) {
			g_ingress_direct_forward_enable = 0;
		}
	} else if (ifx_stricmp(line, "egress disable") == 0) {
		if (g_egress_direct_forward_enable) {
			g_egress_direct_forward_enable = 0;
			//egress is enabled in eth_xmit
		}
	} else {
		printk(
				"echo <ingress/egress> <enable/disable> > /proc/eth/direct_forwarding\n");
		printk(
				"enable  : pure routing configuration, switch forward packets between port0/1 and cpu port directly w/o learning\n");
		printk(
				"disable : bridging/mix configuration, switch learn MAC address and make decision on which port packet forward to\n");
	}
	return 0;
}

#if defined(CONFIG_AR9)
static void proc_clk_output(struct seq_file *m, void *private_data __maybe_unused){

   seq_printf( m, (*AMAZON_S_CGU_SYS & (1 << 7)) ? "PPE clock - 250M\n" : "PPE clock - 300M\n");
}

static int proc_clk_input(char *line, void *data __maybe_unused){

   unsigned int clk_mhz = 0;

   if (ifx_stricmp(line, "300M") == 0 || ifx_stricmp(line, "300") == 0) {
      clk_mhz = 300;
   } else if (ifx_stricmp(line, "250M") == 0 || ifx_stricmp(line, "250") == 0) {
      clk_mhz = 250;
   }


   if (clk_mhz) {
        ppe_clk_change( clk_mhz, FLAG_PPE_CLK_CHANGE_ARG_AVM_DIRCT );
   } else {
      printk("echo <400/393/250M> > /proc/eth/clk\n");
      printk("  300M - PPE clock 300MHz\n");
      printk("  250M - PPE clock 250MHz\n");
   }
   return 0;
}

#endif

#if defined(CONFIG_AR10)
static void proc_clk_output(struct seq_file *m, void *private_data __maybe_unused){
	unsigned int clk;
	int valid = 0;

	switch ((*AR10_CGU_CLKFSR >> 16) & 0x07) {
	case 1:
		clk = 250;
		valid = 1;
		break;
	case 2:
		clk = 393;
		valid = 1;
		break;
	case 4:
		clk = 400;
		valid = 1;
		break;
	default:
		clk = 0;
	}

	if (valid)
		seq_printf(m, "PPE clock - %uM\n", clk);
	else
		seq_printf(m,
				"PPE clock - %uM (unknown option %u)\n", clk,
				(*AR10_CGU_CLKFSR >> 16) & 0x07);
}

static int proc_clk_input(char *line, void *data __maybe_unused){

   unsigned int clk_mhz = 0;

	if (ifx_stricmp(line, "400M") == 0 || ifx_stricmp(line, "400") == 0) {
		clk_mhz = 400;
	} else if (ifx_stricmp(line, "393M") == 0 || ifx_stricmp(line, "393") == 0) {
		clk_mhz = 393;
	} else if (ifx_stricmp(line, "250M") == 0 || ifx_stricmp(line, "250") == 0) {
		clk_mhz = 250;
	}


	if (clk_mhz) {
        ppe_clk_change( clk_mhz, FLAG_PPE_CLK_CHANGE_ARG_AVM_DIRCT );
	} else {
		printk("echo <400/393/250M> > /proc/eth/clk\n");
		printk("  400M - PPE clock 400MHz\n");
		printk("  393M - PPE clock 393MHz (this is not recommended, due to high jitter)\n");
		printk("  250M - PPE clock 250MHz\n");
	}
	return 0;
}
#endif // #if defined(CONFIG_AR10)

#if defined(CONFIG_VR9)
static void proc_clk_output(struct seq_file *m, void *private_data __maybe_unused){
	unsigned int clk;
	int valid = 0;

	switch ((*VR9_CGU_CLKFSR >> 16) & 0x07) {
	case 0:
		clk = 500;
		valid = 1;
		break;
	case 1:
		clk = 432;
		valid = 1;
		break;
	case 2:
		valid = 1;
	default:
		clk = 288;
	}

	if (valid)
		seq_printf(m, "PPE clock - %uM\n", clk);
	else
		seq_printf(m, "PPE clock - %uM (unknown option %u)\n", clk,
				(*VR9_CGU_CLKFSR >> 16) & 0x07);

}

static int proc_clk_input(char *line, void *data __maybe_unused){

   unsigned int clk_mhz = 0;

	if (ifx_stricmp(line, "500M") == 0 || ifx_stricmp(line, "500") == 0) {
		clk_mhz = 500;
	} else if (ifx_stricmp(line, "432M") == 0 || ifx_stricmp(line, "432") == 0) {
		clk_mhz = 432;
	} else if (ifx_stricmp(line, "288M") == 0 || ifx_stricmp(line, "288") == 0) {
		clk_mhz = 288;
	}

	if (clk_mhz){
		ppe_clk_change(clk_mhz, FLAG_PPE_CLK_CHANGE_ARG_AVM_DIRCT);
	} else {
		printk("echo <500/432/288M> > /proc/eth/clk\n");
		printk("  500M - PPE clock 500MHz\n");
		printk("  432M - PPE clock 432MHz\n");
		printk("  288M - PPE clock 288MHz\n");
	}

	return 0;
}
#endif //#if defined(CONFIG_VR9)


static void proc_prio_output(struct seq_file *m, void *private_data __maybe_unused){
#if defined(ATM_DATAPATH)
	unsigned int i;
#endif
	unsigned int j;

	seq_printf(m, "Priority to Queue Map:\n");
	seq_printf(m, "  prio\t\t:  0  1  2  3  4  5  6  7\n");
	seq_printf(m, "  ethX\t\t:");
	for (j = 0; j < NUM_ENTITY(g_eth_prio_queue_map); j++)
	   seq_printf(m, "  %2d", g_eth_prio_queue_map[j]);
	seq_printf(m, "\n");

#if defined(ATM_DATAPATH)
	for (i = 0; i < ATM_QUEUE_NUMBER; i++) {
		if (g_atm_priv_data.connection[i].vcc != NULL) {
			int sw_tx_queue_to_virt_queue_map[ATM_SW_TX_QUEUE_NUMBER] = { 0 };
			u32 sw_tx_queue_table;
			int sw_tx_queue;
			int virt_queue;

			for (sw_tx_queue_table =
					g_atm_priv_data.connection[i].sw_tx_queue_table, sw_tx_queue =
					virt_queue = 0; sw_tx_queue_table != 0;
					sw_tx_queue_table >>= 1, sw_tx_queue++)
				if ((sw_tx_queue_table & 0x01))
					sw_tx_queue_to_virt_queue_map[sw_tx_queue] = virt_queue++;

			seq_printf(m, "  pvc %d.%d \t:",
					(int) g_atm_priv_data.connection[i].vcc->vpi,
					(int) g_atm_priv_data.connection[i].vcc->vci);
			for (j = 0; j < 8; j++)
				seq_printf(m, " %2d",
								sw_tx_queue_to_virt_queue_map[g_atm_priv_data.connection[i].prio_queue_map[j]]);
			seq_printf(m, "\n");
			seq_printf(m, "      phys qid\t:");
			for (j = 0; j < 8; j++)
				seq_printf(m, " %2d",
						g_atm_priv_data.connection[i].prio_queue_map[j]);
			seq_printf(m, "\n");
		}
	}
#elif defined(PTM_DATAPATH)
   if ( g_eth_wan_mode == 0 )
   {
       seq_printf(m,    "  ptm0     :");
       for ( j = 0; j < NUM_ENTITY(g_ptm_prio_queue_map); j++ )
           seq_printf(m, " %2d", g_ptm_prio_queue_map[j]);
       seq_printf(m,    "\n");
   }
#endif // XTM_DATAPATH

    //  firmware queue to switch class map (this is shared by both LAN/WAN interfaces)
    seq_printf(m,        "Queue to Class Map:\n");
    seq_printf(m,        "  queue\t\t:  0  1  2  3  4  5  6  7\n");
    seq_printf(m,        "  eth0/1\t:");
    for ( j = 0; j < NUM_ENTITY(g_wan_queue_class_map); j++ )
        seq_printf(m,    " %2d", g_wan_queue_class_map[j]);
    seq_printf(m,        "\n");
    //  skb->priority to switch class map (this is calculated based on aboved two tables)
    seq_printf(m,        "Priority to Class Map (derived from two tables above):\n");
    seq_printf(m,        "  prio\t\t:  0  1  2  3  4  5  6  7\n");
    seq_printf(m,    "  ethX\t\t:" );
    for ( j = 0; j < NUM_ENTITY(g_eth_prio_queue_map); j++ )
        seq_printf(m," %2d", g_wan_queue_class_map[g_eth_prio_queue_map[j]]);
    seq_printf(m,    "\n");
}


#if defined(ATM_DATAPATH)
static int proc_prio_input(char *line, void *data __maybe_unused){
    int len;
    char *p1, *p2;
    int colon = 1;
    int port = -1;
    int conn = -1;
    int vpi, vci;
    int prio = -1;
    int queue = -1;

    len = strlen(line);
    p1 = line;
    while ( ifx_get_token(&p1, &p2, &len, &colon) )
    {
        if ( ifx_stricmp(p1, "help") == 0 )
        {
            printk("echo <eth0/eth1/pvc vpi.vci> prio xx queue xx [prio xx queue xx] > /proc/eth/prio\n");
            printk("echo pvc vpi.vci <add/del> > /proc/eth/prio\n");
            break;
        }
        else if ( ifx_stricmp(p1, "eth0") == 0 )
        {
            port = 0;
            prio = queue = -1;
            dbg("port = 0");
        }
        else if ( ifx_stricmp(p1, "eth1") == 0 )
        {
            port = 1;
            prio = queue = -1;
            dbg("port = 1");
        }
        else if ( ifx_stricmp(p1, "pvc") == 0 )
        {
            ifx_ignore_space(&p2, &len);
            vpi = ifx_get_number(&p2, &len, 0);
            ifx_ignore_space(&p2, &len);
            vci = ifx_get_number(&p2, &len, 0);
            conn = find_vpivci(vpi, vci);
            dbg("vpi = %u, vci = %u, conn = %d", vpi, vci, conn);
            prio = queue = -1;
        }
        else if ( (port != -1) || conn >= 0 )
        {
            if ( ifx_stricmp(p1, "p") == 0 || ifx_stricmp(p1, "prio") == 0 )
            {
                ifx_ignore_space(&p2, &len);
                prio = ifx_get_number(&p2, &len, 0);
                dbg("prio = %d", prio);
		if (((port == 0) || (port == 1)) && (prio >= 0) &&
		    (prio < (int)NUM_ENTITY(g_eth_prio_queue_map))) {
                    if ( queue >= 0 )
                        g_eth_prio_queue_map[prio] = queue;
                }
                else if ( conn >= 0 && prio >= 0 && prio < 8 )
                {
                    sw_tx_prio_to_queue(conn, prio, queue, NULL);
                }
                else
                {
                    err("prio (%d) is out of range 0 - %d", prio, conn >= 0 ? 7 : NUM_ENTITY(g_eth_prio_queue_map) - 1);
                }
            }
            else if ( ifx_stricmp(p1, "q") == 0 || ifx_stricmp(p1, "queue") == 0 )
            {
                ifx_ignore_space(&p2, &len);
                queue = ifx_get_number(&p2, &len, 0);
                dbg("queue = %d", queue);
                if ( port >= 0 && port <= 1 )
                {
                    if ( queue >= 0 && queue < ((g_wan_itf & (1 << port)) && g_wanqos_en ? __ETH_WAN_TX_QUEUE_NUM : __ETH_VIRTUAL_TX_QUEUE_NUM) ) {
                        if ( prio >= 0 )
                            g_eth_prio_queue_map[prio] = queue;
                    }
                    else
                    {
                        err("queue (%d) is out of range 0 - %d", queue, (g_wan_itf & (1 << port)) && g_wanqos_en ? __ETH_WAN_TX_QUEUE_NUM - 1 : __ETH_VIRTUAL_TX_QUEUE_NUM - 1);
                    }
                }
                else if ( conn >= 0 )
                {
                    int num_of_virt_queue;

                    if ( sw_tx_prio_to_queue(conn, prio, queue, &num_of_virt_queue) != 0 )
                    {
                        err("queue (%d) is out of range 0 - %d", queue, num_of_virt_queue - 1);
                    }
                }
            }
            else if ( conn >= 0 && ifx_stricmp(p1, "add") == 0 )
            {
                int ret = sw_tx_queue_add(conn);

                if ( ret == 0 )
                {
                    dbg("add tx queue successfully");
                }
                else
                {
                    err("failed in adding tx queue: %d", ret);
                }
            }
            else if ( conn >= 0 && (ifx_stricmp(p1, "del") == 0 || ifx_stricmp(p1, "rem") == 0) )
            {
                int ret = sw_tx_queue_del(conn);

                if ( ret == 0 )
                {
                    dbg("remove tx queue successfully");
                }
                else
                {
                    err("failed in removing tx queue: %d", ret);
                }
            }
            else
            {
                err("unknown command (%s)", p1);
            }
        }
        else
        {
            err("unknown command (%s)", p1);
        }

        p1 = p2;
        colon = 1;
    }

    return 0;
}

#elif defined(PTM_DATAPATH)
static int proc_prio_input(char *line, void *data __maybe_unused){
    int len;
    char *p1, *p2;
    int colon = 1;
    int port = -1;
    int prio = -1;
    int queue = -1;

    len = strlen(line);
    p1 = line;
    while ( ifx_get_token(&p1, &p2, &len, &colon) )
    {
       if ( ifx_stricmp(p1, "help") == 0 ) {
          printk("echo <eth0/eth1/ptm0> prio xx queue xx [prio xx queue xx] > /proc/eth/prio\n");
          break;
       }
       else if ( ifx_stricmp(p1, "eth0") == 0 ) {
          port = 0;
          prio = queue = -1;
          dbg("port = 0");
       }
       else if ( ifx_stricmp(p1, "eth1") == 0 ) {
          port = 1;
          prio = queue = -1;
          dbg("port = 1");
       }
       else if ( ifx_stricmp(p1, "ptm0") == 0 ) {
          if ( g_eth_wan_mode == 0 )
          {
             port = 2;
             prio = queue = -1;
             dbg("port = 2 (PTM0)");
          }
       }
       else if ( ifx_stricmp(p1, "pvc") == 0 )
       {
          err("atm is not valid");
          prio = queue = -1;
       }
       else if ( port != ~0 )
       {
          if ( ifx_stricmp(p1, "p") == 0 || ifx_stricmp(p1, "prio") == 0 )
          {
             ifx_ignore_space(&p2, &len);
             prio = ifx_get_number(&p2, &len, 0);
             dbg("prio = %d", prio);
             if ( port >= 0 && port <= 1 )
             {
                if ( prio >= 0 && prio < (int)NUM_ENTITY(g_eth_prio_queue_map) )
                {
                   if ( queue >= 0 )
                      g_eth_prio_queue_map[prio] = queue;
                }
                else
                {
                   err("prio (%d) is out of range 0 - %d", prio, NUM_ENTITY(g_eth_prio_queue_map) - 1);
                }
             }
             else if ( port == 2 )
             {
                if ( prio >= 0 && prio <  (int)NUM_ENTITY(g_ptm_prio_queue_map) )
                {
                   if ( queue >= 0 )
                      g_ptm_prio_queue_map[prio] = queue;
                }
                else
                {
                   err("prio (%d) is out of range 0 - %d", prio, NUM_ENTITY(g_ptm_prio_queue_map) - 1);
                }
             }
          }
          else if ( ifx_stricmp(p1, "q") == 0 || ifx_stricmp(p1, "queue") == 0 )
          {
             ifx_ignore_space(&p2, &len);
             queue = ifx_get_number(&p2, &len, 0);
             dbg("queue = %d", queue);
             if ( port >= 0 && port <= 1 )
             {
                if ( queue >= 0 && queue < ((g_wan_itf & (1 << port)) && g_wanqos_en ? __ETH_WAN_TX_QUEUE_NUM : __ETH_VIRTUAL_TX_QUEUE_NUM) )
                {
                   if ( prio >= 0 )
                      g_eth_prio_queue_map[prio] = queue;
                }
                else
                {
                   err("queue (%d) is out of range 0 - %d", queue, (g_wan_itf & (1 << port)) && g_wanqos_en ? __ETH_WAN_TX_QUEUE_NUM - 1 : __ETH_VIRTUAL_TX_QUEUE_NUM - 1);
                }
             }
             else if ( port == 2 )
             {
                if ( queue >= 0 && queue < g_wanqos_en )
                {
                   if ( prio >= 0 )
                      g_ptm_prio_queue_map[prio] = queue;
                }
                else
                {
                   err("queue (%d) is out of range 0 - %d", queue, g_wanqos_en - 1);
                }
             }
          }
          else
          {
             err("unknown command (%s)", p1);
          }
       }
       else
       {
          err("unknown command (%s)", p1);
       }

       p1 = p2;
       colon = 1;
    }

    return 0;
}


#endif // XTM_DATAPATH

#if defined(DATAPATH_7PORT)
static void proc_ewan_output(struct seq_file *m, void *private_data __maybe_unused){
	int i, j;
	unsigned int bit;

	g_pmac_ewan = *PMAC_EWAN_REG;

	seq_printf(m, "EWAN: ");
	for (i = j = 0, bit = 1; i < 7; i++, bit <<= 1)
		if ((g_pmac_ewan & bit)) {
			seq_printf(m,
					(const char *) (j ? " | P%d" : "P%d"), i);
			j++;
		}
	seq_printf(m, (const char *) (j ? "\n" : "null\n"));
}

static int proc_ewan_input(char *line, void *data __maybe_unused){
	int len;
	char *p1, *p2;
	int colon = 1;

	unsigned int got_ports = 0;
	unsigned int ports = 0;
	char *ports_name[] = { "p0", "p1", "p2", "p3", "p4", "p5", "null" };
	unsigned int i;

	len = strlen(line);
	p1 = line;
	while (ifx_get_token(&p1, &p2, &len, &colon)) {
		if (ifx_stricmp(p1, "help") == 0) {
			printk("echo <p0|p1|p2|p3|p4|p5> > /proc/eth/ewan\n");
			break;
		} else {
			for (i = 0; i < NUM_ENTITY(ports_name); i++)
				if (ifx_stricmp(p1, ports_name[i]) == 0) {

               if ( i == 6 ) {
#if defined(CONFIG_VR9)
                   *PMAC_EWAN_REG = g_pmac_ewan = 0;
#endif
                   return 0;
               }

					ports |= 1 << i;
					got_ports++;
					break;
				}
		}

		p1 = p2;
		colon = 1;
	}

	if (got_ports)
		*PMAC_EWAN_REG = g_pmac_ewan = ports;

	return 0;
}
#endif // #if defined(DATAPATH_7PORT)

static void proc_qos_output(struct seq_file *m, void *private_data __maybe_unused){
    int i;
    struct wtx_eg_q_shaping_cfg qos_cfg;
#if defined(ATM_DATAPATH)
    struct eth_wan_mib_table qos_queue_mib;
    volatile struct tx_qos_cfg tx_qos_cfg = *TX_QOS_CFG;
#elif defined(PTM_DATAPATH)
    struct wan_tx_mib_table qos_queue_mib;
    volatile struct tx_qos_cfg tx_qos_cfg = *TX_QOS_CFG_DYNAMIC;
#endif //XTM_DATAPATH

    volatile struct wtx_qos_q_desc_cfg qos_q_desc_cfg;
    unsigned int portid;
    int global_shapers, gs;

    __sync();

    seq_printf(m, "\n  qos         : %s\n  wfq         : %s\n  Rate shaping: %s\n\n",
                    tx_qos_cfg.eth1_qss ?"enabled":"disabled",
                    tx_qos_cfg.wfq_en?"enabled":"disabled",
                    tx_qos_cfg.shape_en ?"enabled":"disabled");
    seq_printf(m, "  Ticks  =%u,    overhd    =%u,       qnum=%u  @%p\n", (u32)tx_qos_cfg.time_tick, (u32)tx_qos_cfg.overhd_bytes, (u32)tx_qos_cfg.eth1_eg_qnum, TX_QOS_CFG );
    seq_printf(m, "  PPE clk=%u MHz, basic tick=%u\n", (u32)cgu_get_pp32_clock() / 1000000, TX_QOS_CFG->time_tick / (cgu_get_pp32_clock() / 1000000));

    if ( tx_qos_cfg.eth1_eg_qnum )
    {
        if ( ifx_ppa_drv_hal_generic_hook != NULL )
        {
            PPE_QOS_RATE_SHAPING_CFG rate_cfg = {0};
            PPE_QOS_WFQ_CFG wfq_cfg = {0};

            if ( g_eth_wan_mode == 0 )
                portid = 7;
            else if ( (g_wan_itf & 0x02) )
                portid = 1;
            else if ( (g_wan_itf & 0x01) )
                portid = 0;
            else
                portid = ~0;

            seq_printf(m, "\n     Cfg:    T     R     S -->  Bit-rate(kbps)      Weight --> Level       Address       d/w      tick_cnt   b/S\n");
            for ( i = 0; i < 8; i++ )
            {
#ifdef CONFIG_IFX_PPA_QOS_RATE_SHAPING
                rate_cfg.portid  = portid;
                rate_cfg.queueid = i;
                ifx_ppa_drv_hal_generic_hook(PPA_GENERIC_HAL_GET_QOS_RATE_SHAPING_CFG, &rate_cfg, 0);
#endif

#ifdef CONFIG_IFX_PPA_QOS_WFQ
                wfq_cfg.portid  = portid;
                wfq_cfg.queueid = i;
#if defined(ATM_DATAPATH)
                ifx_ppa_drv_hal_generic_hook(PPA_GENERIC_HAL_GET_QOS_WFQ_CFG, &rate_cfg, 0);
#elif defined(PTM_DATAPATH)
                ifx_ppa_drv_hal_generic_hook(PPA_GENERIC_HAL_GET_QOS_WFQ_CFG, &wfq_cfg, 0);
#endif // XTM_DATAPATH

#endif //  #ifdef CONFIG_IFX_PPA_QOS_WFQ
                qos_cfg = *WTX_EG_Q_SHAPING_CFG(i);

                seq_printf(m, "\n    Q %2u:  %03u  %05u  %05u   %07u            %08u   %03u        @0x%p   %08u    %03u     %05u\n", i, qos_cfg.t, qos_cfg.r, qos_cfg.s, rate_cfg.rate_in_kbps, qos_cfg.w, wfq_cfg.weight_level, WTX_EG_Q_SHAPING_CFG(i), qos_cfg.d, qos_cfg.tick_cnt, qos_cfg.b);
            }

            //  PTM WAN mode has 4 port based rate shaping, while ETH WAN mode has one.
	    if (portid < 2)
		    global_shapers = 1;
	    else if (portid == 7)
		    global_shapers = 4;
	    else
		    global_shapers = 0;



            for(gs = 0; gs < global_shapers; gs++) {
  #ifdef CONFIG_IFX_PPA_QOS_RATE_SHAPING
                rate_cfg.portid  = portid;
                rate_cfg.queueid = (portid==7)?(~gs):i;
                ifx_ppa_drv_hal_generic_hook(PPA_GENERIC_HAL_GET_QOS_RATE_SHAPING_CFG, &rate_cfg, 0);
  #endif
                qos_cfg = *WTX_EG_Q_PORT_SHAPING_CFG(gs);

	    if (gs == 0)
                seq_printf(m, "\n Global_Shapers:\n");

                seq_printf(m, "\n    S %2u:  %03u  %05u  %05u   %07u                                  @0x%p   %08u    %03u     %05u\n", gs,
			   qos_cfg.t, qos_cfg.r, qos_cfg.s, rate_cfg.rate_in_kbps, WTX_EG_Q_PORT_SHAPING_CFG(gs), qos_cfg.d, qos_cfg.tick_cnt, qos_cfg.b);
            }
        }

        seq_printf(m, "\n  MIB : rx_pkt/rx_bytes         tx_pkt/tx_bytes       cpu_small_drop/cpu_drop  fast_small_drop/fast_drop_cnt\n");
        for ( i = 0; i < 8; i++ )
        {
            qos_queue_mib = *ETH_WAN_TX_MIB_TABLE(i);

            seq_printf(m, "    %2u: %010u/%010u  %010u/%010u  %010u/%010u  %010u/%010u  @0x%p\n", i,
                qos_queue_mib.wrx_total_pdu, qos_queue_mib.wrx_total_bytes,
                qos_queue_mib.wtx_total_pdu, qos_queue_mib.wtx_total_bytes,
                qos_queue_mib.wtx_cpu_drop_small_pdu, qos_queue_mib.wtx_cpu_drop_pdu,
                qos_queue_mib.wtx_fast_drop_small_pdu, qos_queue_mib.wtx_fast_drop_pdu,
                ETH_WAN_TX_MIB_TABLE(i));

        }

        //QOS queue descriptor
        seq_printf(m, "\n  Desc: threshold  num    base_addr  rd_ptr   wr_ptr\n");
        for ( i = 0; i < 8; i++ )
        {
            qos_q_desc_cfg = *WTX_QOS_Q_DESC_CFG(i);

            seq_printf(m, "    %2u: 0x%02x       0x%02x   0x%04x     0x%04x   0x%04x  @0x%p\n", i,
                qos_q_desc_cfg.threshold,
                qos_q_desc_cfg.length,
                qos_q_desc_cfg.addr,
                qos_q_desc_cfg.rd_ptr,
                qos_q_desc_cfg.wr_ptr,
                WTX_QOS_Q_DESC_CFG(i) );

        }
    }

}

#if defined(ENABLE_CONFIGURABLE_DSL_VLAN) && ENABLE_CONFIGURABLE_DSL_VLAN
static void proc_dsl_vlan_output(struct seq_file *m, void *private_data __maybe_unused){
	unsigned int i;

	seq_printf(m, "PVC to DSL VLAN map:\n");
	for (i = 0; i < ATM_QUEUE_NUMBER; i++) {
		if (HTU_ENTRY(i + OAM_HTU_ENTRY_NUMBER)->vld == 0) {
			seq_printf(m, "  %2d: invalid\n", i);
			continue;
		}

		seq_printf(m, "  %2d: PVC %u.%u\tVLAN 0x%04X\n", i,
				(unsigned int) g_atm_priv_data.connection[i].vcc->vpi,
				(unsigned int) g_atm_priv_data.connection[i].vcc->vci,
				(unsigned int) WRX_QUEUE_CONFIG(i)->new_vlan);
	}
	seq_printf(m, "DSL VLAN to PVC map:\n");
	for (i = 0; i < NUM_ENTITY(g_dsl_vlan_qid_map); i += 2) {
		if (g_dsl_vlan_qid_map[i] == 0) {
			seq_printf(m, "  %2d: invalid\n", i);
			continue;
		}
		seq_printf(m, "  %2d: VLAN 0x%04X\tQID %d\n", i >> 1,
				(unsigned int) g_dsl_vlan_qid_map[i],
				(unsigned int) g_dsl_vlan_qid_map[i + 1]);
	}
}

static int proc_dsl_vlan_input(char *line, void *data __maybe_unused){
	int len;
	char *p1, *p2;
	int colon = 1;
	int conn = -1;
	unsigned int vpi, vci;
	unsigned int vlan_tag = ~0U;
	int eth0 = 0, eth1 = 0;
	int bypass = 0, enable = 0;
	len = strlen(line);
	p1 = line;
	while (ifx_get_token(&p1, &p2, &len, &colon)) {
		if (ifx_stricmp(p1, "help") == 0) {
			printk(
					"echo <pvc vpi.vci> <vlan xx> [bypass [enable|disbable]] > /proc/eth/dsl_vlan\n");
			printk(
					"  vlan   - 16-bit vlan tag, including PCP, CFI, and VID.\n");
			printk(
					"  bypass - traffic from ATM to MII0 is done by internal switch, no CPU involvement.\n");
			conn = -1;
			break;
		} else if (bypass == 1) {
			if (ifx_stricmp(p1, "enable") == 0)
				enable = 1;
			else if (ifx_stricmp(p1, "disable") == 0)
				enable = -1;
			else if (ifx_stricmp(p1, "eth0") == 0) {
				if (eth0 == 0)
					eth0 = enable == 0 ? 1 : enable;
			} else if (ifx_stricmp(p1, "eth1") == 0) {
				if (eth1 == 0)
					eth1 = enable == 0 ? 1 : enable;
			} else {
				printk("unknown command (%s)\n", p1);
				conn = -1;
				break;
			}
		} else {
			if (bypass == 1 && eth0 == 0)
				eth0 = enable >= 0 ? 1 : -1;

			bypass = 0;
			enable = 0;

			if (ifx_stricmp(p1, "pvc") == 0) {
				ifx_ignore_space(&p2, &len);
				vpi = ifx_get_number(&p2, &len, 0);
				ifx_ignore_space(&p2, &len);
				vci = ifx_get_number(&p2, &len, 0);
				conn = find_vpivci(vpi, vci);
				printk("vpi = %u, vci = %u, conn = %d\n", vpi, vci, conn);
			} else if (vlan_tag == ~0U && ifx_stricmp(p1, "vlan") == 0) {
				ifx_ignore_space(&p2, &len);
				vlan_tag = ifx_get_number(&p2, &len, 0) & 0xFFFF;
				printk("VLAN tag = 0x%04X\n", vlan_tag);
			} else if (ifx_stricmp(p1, "bypass") == 0)
				bypass = 1;
			else {
				printk("unknown command (%s)\n", p1);
				conn = -1;
				break;
			}
		}

		p1 = p2;
		colon = 1;
	}
	if (bypass == 1 && eth0 == 0)
		eth0 = enable >= 0 ? 1 : -1;

	if (conn >= 0 && vlan_tag != ~0U) {
		if (ppe_set_vlan(conn, (unsigned short) vlan_tag) != 0)
			conn = -1;
	}
	if (conn >= 0) {
		vlan_tag = (unsigned int) WRX_QUEUE_CONFIG(conn)->new_vlan;
		if (eth0 != 0)
			sw_set_vlan_bypass(0, vlan_tag, eth0 < 0 ? 1 : 0);
		if (eth1 != 0)
			sw_set_vlan_bypass(1, vlan_tag, eth0 < 0 ? 1 : 0);
	}

	return 0;
}

static INLINE int ppe_set_vlan(int conn, unsigned short vlan_tag) {
	int candidate = -1;
	unsigned int i;

	for (i = 0; i < NUM_ENTITY(g_dsl_vlan_qid_map); i += 2) {
		if (candidate < 0 && g_dsl_vlan_qid_map[i] == 0)
			candidate = i;
		else if (g_dsl_vlan_qid_map[i] == (unsigned short) vlan_tag) {
			if (conn != g_dsl_vlan_qid_map[i + 1]) {
				err("VLAN tag 0x%04X is in use", vlan_tag);
				return -1; //  quit (error), VLAN tag is used by other PVC
			}
			return 0; //  quit (success), VLAN tag is assigned already
		}
	}
	//  candidate must be valid, because size of g_dsl_vlan_qid_map is equal to max number of PVCs
	g_dsl_vlan_qid_map[candidate + 1] = conn;
	g_dsl_vlan_qid_map[candidate] = vlan_tag;
	WRX_QUEUE_CONFIG(conn)->new_vlan = vlan_tag;

	return 0;
}

static INLINE int sw_set_vlan_bypass(int itf __attribute__((unused)), unsigned int vlan_tag __attribute__((unused)), int is_del __attribute__((unused))) {
	if (itf == 1) {
		err("does not support eth1 VLAN bypass!");
		return -1;
	}
	return 0;
}
#endif

#if defined(DEBUG_MIRROR_PROC) && DEBUG_MIRROR_PROC

static void proc_mirror_output(struct seq_file *m, void *private_data __maybe_unused){
	char devname[IFNAMSIZ + 1] = { 0 };

	if (g_mirror_netdev == NULL)
		strcpy(devname, "NONE");
	else
		strncpy(devname, g_mirror_netdev->name, IFNAMSIZ);

	seq_printf(m, "mirror: %s\n", devname);
}


static int proc_mirror_input(char *line, void *data __maybe_unused){

	char *p = line;
	if (g_mirror_netdev != NULL) {
		dev_put(g_mirror_netdev);
		g_mirror_netdev = NULL;
	}
	if (ifx_stricmp(p, "none") == 0)
		printk("disable mirror\n");
	else {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,32)
		g_mirror_netdev = dev_get_by_name(p);
#else
		g_mirror_netdev = dev_get_by_name(&init_net, p);
#endif
		if (g_mirror_netdev != NULL)
			printk("mirror: %s\n", p);
		else
			printk("mirror: none, can't find device \"%s\"\n", p);
	}
	return 0;
}

#endif

static void proc_class_output(struct seq_file *m, void *private_data __maybe_unused){

    seq_printf(m, "Switch Class %s", PS_MC_GENCFG3->class_en ?  "Enable" : "Disable");
#if defined(CONFIG_VR9)
    seq_printf(m, ", QID CONF: %d\n", *__DPLUS_QID_CONF_PTR);
#endif
    seq_printf(m, "\nCFG_CLASS2QID_MAP (0x%08x): %08x %08x %08x %08x\n", (unsigned int)CFG_CLASS2QID_MAP(0), *CFG_CLASS2QID_MAP(0), *CFG_CLASS2QID_MAP(1), *CFG_CLASS2QID_MAP(2), *CFG_CLASS2QID_MAP(3));
    seq_printf(m, "CFG_QID2CLASS_MAP (0x%08x): %08x %08x %08x %08x\n", (unsigned int)CFG_QID2CLASS_MAP(0), *CFG_QID2CLASS_MAP(0), *CFG_QID2CLASS_MAP(1), *CFG_QID2CLASS_MAP(2), *CFG_QID2CLASS_MAP(3));

}

static int proc_class_input(char *line, void *data __maybe_unused) {
    char *p1, *p2;
    int colon = 1;
    int i, j, len;

    len = strlen(line);
    p1 = line;
    while ( ifx_get_token(&p1, &p2, &len, &colon) )
    {
        if ( ifx_stricmp(p1, "CFG_CLASS2QID_MAP") == 0 || ifx_stricmp(p1, "CLASS2QID") == 0 )
        {
            for ( i = 0; i < 4; i++ )
            {
                ifx_ignore_space(&p2, &len);
                if ( !len )
                    break;
                *CFG_CLASS2QID_MAP(i) = ifx_get_number(&p2, &len, 1);
            }
            break;
        }
        else if ( ifx_stricmp(p1, "CFG_QID2CLASS_MAP") == 0 || ifx_stricmp(p1, "QID2CLASS") == 0 ) {
            for ( i = 0; i < 4; i++ )
            {
                ifx_ignore_space(&p2, &len);
                if ( !len )
                    break;
                *CFG_QID2CLASS_MAP(i) = ifx_get_number(&p2, &len, 1);
                if ( i == 0 ) {
                    for ( j = 0; j < 8; j++ ){
                        // ccb: I guess this was wrong in AR10_A5: g_wan_queue_class_map[i * 8 + j] = (*CFG_QID2CLASS_MAP(i) >> (j * 4)) & 0x0F;
                        g_wan_queue_class_map[j] = (*CFG_QID2CLASS_MAP(i) >> (j * 4)) & 0x0F;
                    }
                }
            }
            break;

        } else if ( ifx_stricmp(p1, "Enable") == 0 || ifx_stricmp(p1, "Disable") == 0){
            PS_MC_GENCFG3->class_en = ifx_stricmp(p1, "Enable") == 0 ? 1 : 0;
            break;
        }
#if defined(CONFIG_VR9)
        else if ( ifx_stricmp(p1, "QIDCONF") == 0){
            ifx_ignore_space(&p2, &len);
            i = ifx_get_number(&p2, &len, 0);
            if(i < 0 || i > 3){ //illegal input
                break;
            }
            *__DPLUS_QID_CONF_PTR = i;
            break;
        }
#endif // #if defined(CONFIG_VR9)
        else {
            printk("echo <CLASS2QID | QID2CLASS> [hex numbers] > /proc/eth/class\n");
            printk("echo <Enable | Disable > /proc/eth/class\n");
            break;
        }

        p1 = p2;
        colon = 1;
    }

    return 0;
}


static void proc_mccfg_output(struct seq_file *m, void *private_data __maybe_unused){

    seq_printf(m, "SSC: %s, ASC: %s \n",
                  PS_MC_GENCFG3->ssc_mode ? "Enable" : "Disable",
                  PS_MC_GENCFG3->asc_mode ? "Enable" : "Disable");

}

static int proc_mccfg_input(char *line, void *data __maybe_unused) {
    int len;
    char *p1 = NULL;
    char *p2 = NULL;
    int colon = 0;
    int val_en = -1;
    int is_ssc = -1;

    len = strlen(line);

    p1 = line;
    ifx_get_token(&p1, &p2, &len, &colon);
    p2 ++;
    len --;

    if ( ifx_stricmp(p1, "Enable") == 0 || ifx_stricmp(p1, "Disable") == 0 )
    {
        val_en = (ifx_stricmp(p1, "Enable") == 0) ? 1 : 0;

        p1 = p2;
        ifx_get_token(&p1, &p2, &len, &colon);

        if(len && (ifx_stricmp(p1, "asc") == 0 || ifx_stricmp(p1, "ssc") == 0)){
            is_ssc = (ifx_stricmp(p1, "ssc") == 0) ? 1 : 0;
        }

        if(is_ssc >= 0 && val_en >= 0){
            if(is_ssc){
                PS_MC_GENCFG3->ssc_mode = val_en;
            }else{
                PS_MC_GENCFG3->asc_mode = val_en;
            }
            return 0;
        }
    }

    printk("echo <Enable | Disable> [ssc | asc] > /proc/eth/mccfg\n");
    return 0;

}



#if defined(DEBUG_FIRMWARE_TABLES_PROC) && DEBUG_FIRMWARE_TABLES_PROC
static INLINE char *get_wanitf(int iswan)
{
    static char itfs[64];
    char *allitfs[] = {
        "ETH0",
        "ETH1",
        "", //reserve for CPU
        "EXT1",
        "EXT2",
        "EXT3",
        "EXT4",
#if defined(ATM_DATAPATH)
        "EXT5",
#elif defined(PTM_DATAPATH)
        "PTM",
#endif
        NULL
    };

    int wan_itf = *CFG_WAN_PORTMAP;
    int wan_mixmap = *CFG_MIXED_PORTMAP;
    int i, len = 0;

#if defined(ATM_DATAPATH)
    int max_exts_idx = 3 + 2;
#elif defined(PTM_DATAPATH)
    int max_exts_idx = 4 + 2;
#endif

    memset(itfs, 0, sizeof(itfs));
    if(wan_mixmap != 0 && !iswan){ //mix mode dont have lan
        return itfs;
    }

    for(i = 0; allitfs[i] != NULL ; i ++){
        if(i == 2) continue; //skip CPU port

        if((iswan && (wan_itf & (1 << i))) || (!iswan && !(wan_itf & (1 << i)))){
            if(i > max_exts_idx){
                if(!iswan){
                    break; //DSL cannot be lan
                }
                else{
                    len += sprintf(itfs + len, "%s ", "DSL");
                    break; //DSL only listed once although in A5 we have 2 ports support DSL
                }
            }

            len += sprintf(itfs + len, "%s ", allitfs[i]);

            if(iswan && (wan_mixmap &  (1 << i))){
                len += sprintf(itfs + len, "(Mixed) ");
            }
        }
    }

    return itfs;
}


#endif // #if defined(DEBUG_FIRMWARE_TABLES_PROC) && DEBUG_FIRMWARE_TABLES_PROC

#if defined(RTP_SAMPLING_ENABLE) && RTP_SAMPLING_ENABLE
static INLINE int print_mc(struct seq_file *m, int i, struct wan_rout_multicast_cmp_tbl *pcompare, struct wan_rout_multicast_act_tbl *paction, struct rtp_sampling_cnt *rtp_mib,uint32_t mc_mib_addr, uint32_t rt_mc_cnt)
#else
static INLINE int print_mc(struct seq_file *m, int i, struct wan_rout_multicast_cmp_tbl *pcompare, struct wan_rout_multicast_act_tbl *paction, uint32_t mc_mib_addr, uint32_t rt_mc_cnt)
#endif
{
    static const char *rout_type[] = {"no IP level editing", "TTL editing", "reserved - 2", "reserved - 3"};
    static const char *dest_list[] = {"ETH0", "ETH1", "CPU0", "EXT_INT1", "EXT_INT2", "EXT_INT3", "res", "ATM"};


    u32 bit;
    int j, k;

    seq_printf(m,          "  entry %d - %s\n", i, paction->entry_vld ? "valid" : "invalid");
    seq_printf(m,          "    compare (0x%08X)\n", (u32)pcompare);
    seq_printf(m,          "      wan_dest_ip:  %d.%d.%d.%d\n", (pcompare->wan_dest_ip >> 24) & 0xFF,  (pcompare->wan_dest_ip >> 16) & 0xFF,  (pcompare->wan_dest_ip >> 8) & 0xFF,  pcompare->wan_dest_ip & 0xFF);
    seq_printf(m,          "      wan_src_ip:   %d.%d.%d.%d\n", (pcompare->wan_src_ip >> 24) & 0xFF,  (pcompare->wan_src_ip >> 16) & 0xFF,  (pcompare->wan_src_ip >> 8) & 0xFF,  pcompare->wan_src_ip & 0xFF);
    seq_printf(m,          "    action  (0x%08X)\n", (u32)paction);
    seq_printf(m,          "      rout_type:    %s\n", rout_type[paction->rout_type]);
    if ( paction->new_dscp_en )
        seq_printf(m,      "      new DSCP:     %d\n", paction->new_dscp);
    else
        seq_printf(m,      "      new DSCP:     original (not modified)\n");
    if ( paction->in_vlan_ins )
        seq_printf(m,      "      VLAN insert:  enable, VCI 0x%04x\n", paction->new_in_vci);
    else
        seq_printf(m,      "      VLAN insert:  disable\n");
    seq_printf(m,          "      VLAN remove:  %s\n", paction->in_vlan_rm ? "enable" : "disable");
    if ( !paction->dest_list )
        seq_printf(m,      "      dest list:    none\n");
    else
    {
        seq_printf(m,      "      dest list:   ");
        for ( bit = 1, j = k = 0; bit < 1 << 8; bit <<= 1, j++ )
            if ( (paction->dest_list & bit) )
            {
                if ( k )
                    seq_printf(m, ", ");
                seq_printf(m, dest_list[j]);
                k = 1;
            }
        seq_printf(m, "\n");
    }
    seq_printf(m,          "      PPPoE mode:   %s\n", paction->pppoe_mode ? "termination" : "transparent");

#if defined(RTP_SAMPLING_ENABLE) && RTP_SAMPLING_ENABLE
    seq_printf(m,          "      RTP sampling    : %s\n", paction->sample_en ? "enable" : "disable");
   if(paction->sample_en)
   {
    seq_printf(m,          "      RTP packet count: %d\n", rtp_mib->pkt_cnt);
    seq_printf(m,          "      RTP seq number  : %d\n", rtp_mib->seq_no);
   }
#endif

    if ( paction->new_src_mac_en )
        seq_printf(m,      "      new src MAC index: %d\n", paction->new_src_mac_ix);
    else
        seq_printf(m,      "      new src MAC index: disabled\n");
    if ( paction->out_vlan_ins )
        seq_printf(m,      "      outer VLAN insert: enable, index %d\n", paction->out_vlan_ix);
    else
        seq_printf(m,      "      outer VLAN insert: disable\n");
    seq_printf(m,          "      outer VLAN remove: %s\n", paction->out_vlan_rm ? "enable" : "disable");
    seq_printf(m,          "      dest_qid:          %d\n", paction->dest_qid);
    seq_printf(m,          "      Tunnel Remove    : %s\n", paction->tunnel_rm ? "enable" : "disable");
    seq_printf(m,          "    MIB: (0x%08x)\n", mc_mib_addr);
#if defined(MIB_MODE_ENABLE) && MIB_MODE_ENABLE
    {
         if(PS_MC_GENCFG3->session_mib_unit == 1)
            seq_printf(m,          "      acc packets      : %u\n", rt_mc_cnt);
         else
            seq_printf(m,          "      acc bytes        : %u\n", rt_mc_cnt);
    }

#else

    seq_printf(m,          "      acc bytes        : %u\n", rt_mc_cnt);
#endif

    return 0;
}

#ifdef CONFIG_VR9
static void proc_powersaving_output(struct seq_file *m, void *private_data __maybe_unused){
    char *profile[] = {"TIMER0", "DPLUS_IN", "SFSM", "RUNNING"};
    char *field[] = {"wakeup", "cycles_lo", "cycles_hi", NULL};
    volatile unsigned int *p;
    unsigned int i, j;

    seq_printf(m, "PS_MC_GENCFG3 (0x%08x): time_tick - %d, basic tick - %dus, enable - %s\n", (unsigned int)PS_MC_GENCFG3, PS_MC_GENCFG3->time_tick, PS_MC_GENCFG3->time_tick * 1000 / (cgu_get_pp32_clock() / 1000), PS_MC_GENCFG3->psave_en ? "yes" : "no");
    for ( i = 0; i < NUM_ENTITY(field); i++ )
        if ( field[i] != NULL )
        {
            if ( i == 0 || strlen(field[i - 1]) < 8 )
                seq_printf(m, "\t");
            seq_printf(m, "\t%s", field[i]);
        }
    seq_printf(m, "\n");
    for ( j = 0, p = (volatile unsigned int *)POWERSAVING_PROFILE(0); j < NUM_ENTITY(profile); j++ )
    {
        seq_printf(m, "%s", profile[j]);
        if ( strlen(profile[j]) < 8 )
            seq_printf(m, "\t");
        for ( i = 0; i < NUM_ENTITY(field); i++, p++ )
            if ( field[i] != NULL )
                seq_printf(m, "\t0x%08x", *p);
        seq_printf(m, "\n");
    }
}


static int proc_powersaving_input(char *line, void *data __maybe_unused) {
    int len;
    char *p1, *p2;
    int colon = 1;
    int i;

    len = strlen(line);
    p1 = line;
    while ( ifx_get_token(&p1, &p2, &len, &colon) )
    {
        if ( ifx_stricmp(p1, "clear") == 0 || ifx_stricmp(p1, "clean") == 0 )
        {
            volatile unsigned int *p = (volatile unsigned int *)POWERSAVING_PROFILE(0);

            for ( i = 0; i < 16; i++ )
                *p++ = 0;
        }
        else if ( ifx_stricmp(p1, "enable") == 0 || ifx_stricmp(p1, "on") == 0 )
        {

            ifx_ignore_space(&p2, &len);
            if ( len && *p2 >= '0' && *p2 <= '9' )
                PS_MC_GENCFG3->time_tick = ifx_get_number(&p2, &len, 0);
            if ( PS_MC_GENCFG3->time_tick < 0x100 )
            {
#if defined(PTM_DATAPATH)
                PS_MC_GENCFG3->time_tick = cgu_get_pp32_clock() / 250000;
#else
                PS_MC_GENCFG3->time_tick = cgu_get_pp32_clock() / 125000;
#endif
                printk("time_tick is too small, replaced with %d\n", PS_MC_GENCFG3->time_tick);
            }
            PS_MC_GENCFG3->psave_en = 1;
#if defined(PTM_DATAPATH)
            PSAVE_CFG->sleep_en = 1;
#endif
        }
        else if ( ifx_stricmp(p1, "disable") == 0 || ifx_stricmp(p1, "off") == 0 )
        {
            PS_MC_GENCFG3->psave_en = 0;
#if defined(PTM_DATAPATH)
            PSAVE_CFG->sleep_en = 0;
#endif
        }
        else
        {
            printk("echo <on [time_tick] | off | clear> > /proc/eth/powersaving\n");
            break;
        }

        p1 = p2;
        colon = 1;
    }

    return 0;
}
#endif // #ifdef CONFIG_VR9

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

static void proc_dsl_rx_bytes_output(struct seq_file *m, void *private_data __maybe_unused){
	unsigned int rx_bytes = 0;
#if defined(PTM_DATAPATH)
	rx_bytes = WAN_RX_MIB_TABLE(PTM_DEFAULT_DEV)->wrx_total_bytes;
#elif defined(ATM_DATAPATH)
	rx_bytes = DSL_WAN_MIB_TABLE->wrx_total_byte;
#endif
	seq_printf(m, "%u\n", rx_bytes);
}

static void proc_dsl_tx_bytes_output(struct seq_file *m, void *private_data __maybe_unused){
	unsigned int sum_queues_tx_bytes = 0;
#if defined(PTM_DATAPATH)
	unsigned int i;
	for ( i = 0; i < 8; i++ ) {
		sum_queues_tx_bytes += (WAN_TX_MIB_TABLE(i)->wtx_total_bytes);
	}
#elif defined(ATM_DATAPATH)
	sum_queues_tx_bytes = DSL_WAN_MIB_TABLE->wtx_total_byte;
#endif
	seq_printf(m, "%u\n", sum_queues_tx_bytes);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

static void proc_dsl_rx_pdus_output(struct seq_file *m, void *private_data __maybe_unused){
	unsigned int rx_pdu = 0;
#if defined(PTM_DATAPATH)
	int i= 0;
	volatile unsigned int *wrx_total_pdu[4]         = {DREG_AR_AIIDLE_CNT0,  DREG_AR_HEC_CNT0,    DREG_AR_AIIDLE_CNT1, DREG_AR_HEC_CNT1};
	for ( i = 0; i < 4; i++ ) {
		rx_pdu += ( IFX_REG_R32(wrx_total_pdu[i]) - last_wrx_total_pdu[PPE_ID][i]);
	}
#elif defined(ATM_DATAPATH)
	rx_pdu = DSL_WAN_MIB_TABLE->wrx_correct_pdu;
#endif
	seq_printf(m, "%u\n", rx_pdu);
}

static void proc_dsl_tx_pdus_output(struct seq_file *m, void *private_data __maybe_unused){
	unsigned int sum_queues_tx_pdu = 0;
#if defined(PTM_DATAPATH)
	unsigned int i;
	for ( i = 0; i < 8; i++ ) {
		sum_queues_tx_pdu += (WAN_TX_MIB_TABLE(i)->wtx_total_pdu);
	}
#elif defined(ATM_DATAPATH)
	sum_queues_tx_pdu = DSL_WAN_MIB_TABLE->wtx_total_pdu;
#endif
	seq_printf(m, "%u\n", sum_queues_tx_pdu);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
#ifdef PTM_DATAPATH

static void proc_status_output(struct seq_file *m, void *private_data __maybe_unused){

	char *dsl_bonding_str[] = {"non-bonding", "onchip bonding", "offchip bonding"};
	unsigned int i;

	seq_printf(m,  "DSL Bonding Capability: %s\n", dsl_bonding_str[g_dsl_bonding]);
	if ( g_dsl_bonding)
	{
		seq_printf(m,  BOND_CONF->bond_mode == L1_PTM_TC_BOND_MODE ? "Bonding Mode: PTM TC\n" : "Bonding Mode: ETH Trunk\n");
	}
	seq_printf(m,  "Upstream Data Rate:  ");
	for ( i = 0; i < NUM_ENTITY(g_datarate_us); i++ )
		seq_printf(m,  "\t%d", g_datarate_us[i]);
	seq_printf(m,  "\n");
	seq_printf(m,  "Downstream Data Rate:");
	for ( i = 0; i < NUM_ENTITY(g_datarate_ds); i++ )
		seq_printf(m,  "\t%d", g_datarate_ds[i]);
	seq_printf(m,  "\n");
	seq_printf(m,  "User Specified Max_Delay:");
	for ( i = 0; i < NUM_ENTITY(g_max_delay); i++ )
		seq_printf(m,  "\t%u", g_max_delay[i]);
	seq_printf(m,  "\n");

}
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

static int proc_status_input(char *input_str, void *data __maybe_unused) {
    char *p1, *p2;
    int len;
    int colon;

    char *env_var_str[] = {"help", "DSL_LINE_NUMBER", "DSL_TC_LAYER_STATUS", "DSL_EFM_TC_CONFIG_US", "DSL_BONDING_STATUS", "DSL_INTERFACE_STATUS", "DSL_DATARATE_US_BC", "DSL_DATARATE_DS_BC", "MAX_DELAY"};
    int line = -1, down = 0, upstr = 0, downstr = 0;
    int datarate_us[BEARER_CHANNEL_PER_LINE] = {0}, datarate_ds[BEARER_CHANNEL_PER_LINE] = {0};
    unsigned int num, bc;
    unsigned int i;

    len = strlen(input_str);

    p1 = input_str;
    p2 = NULL;
    colon = 1;
    while ( ifx_get_token(&p1, &p2, &len, &colon) )
    {
        for ( i = 0; i < NUM_ENTITY(env_var_str) && ifx_strincmp(p1, env_var_str[i], strlen(env_var_str[i])) != 0; i++ );
        switch ( i )
        {
        case 0: //  help
            printk("Usage:\n");
            printk("  echo <command> [parameters] > /proc/dsl_tc/status\n");
            printk("  commands:\n");
            printk("    DSL_LINE_NUMBER      - parameter: line number\n");
            printk("    DSL_TC_LAYER_STATUS  - parameter: ATM, EFM\n");
            printk("    DSL_EFM_TC_CONFIG_US - parameter: normal, preemption\n");
            printk("    DSL_BONDING_STATUS   - parameter: inactive, active\n");
            printk("    DSL_INTERFACE_STATUS - parameter: down, ready, handshake, training, up\n");
            printk("    DSL_DATARATE_US_BC?  - ?: bearer channel number 0/1, parameter: data rate in bps\n");
            printk("    DSL_DATARATE_DS_BC?  - ?: bearer channel number 0/1, parameter: data rate in bps\n");
            printk("    MAX_DELAY            - parameter: user specified max delay in micorsecond\n");
            break;
        case 1: //  DSL_LINE_NUMBER
            ifx_ignore_space(&p2, &len);
            num = ifx_get_number(&p2, &len, 0);
            if ( num < LINE_NUMBER )
                line = num;
            break;
        case 2: //  DSL_TC_LAYER_STATUS
            //  TODO
            break;
        case 3: //  DSL_EFM_TC_CONFIG_US
            //  TODO
            break;
        case 4: //  DSL_BONDING_STATUS
            if ( g_dsl_bonding )
            {
                p1 = p2;
                colon = 1;
                if ( ifx_get_token(&p1, &p2, &len, &colon) )
                {
                    if ( ifx_stricmp(p1, "inactive") == 0 )
                        switch_to_eth_trunk();
                    else if ( ifx_stricmp(p1, "active") == 0 )
                        switch_to_ptmtc_bonding();
                }
            }
            break;
        case 5: //  DSL_INTERFACE_STATUS
            if ( line >= 0 )
            {
                p1 = p2;
                colon = 1;
                if ( ifx_get_token(&p1, &p2, &len, &colon) && ifx_stricmp(p1, "down") == 0 && g_line_showtime[line] != 0 )
                {
                    g_line_showtime[line] = 0;
                    down = 1;
                }
            }
            break;
        case 6: //  DSL_DATARATE_US_BC
            bc = (unsigned int)(p1[strlen(env_var_str[i])] - '0');
            if ( line >= 0 && bc < BEARER_CHANNEL_PER_LINE )
            {
                ifx_ignore_space(&p2, &len);
                num = ifx_get_number(&p2, &len, 0);
                if ( num != 0 )
                {
                    datarate_us[bc] = num;
                    upstr = 1;
                }
            }
            break;
        case 7: //  DSL_DATARATE_DS_BC
            bc = (unsigned int)(p1[strlen(env_var_str[i])] - '0');
            if ( line >= 0 && bc < BEARER_CHANNEL_PER_LINE )
            {
                ifx_ignore_space(&p2, &len);
                num = ifx_get_number(&p2, &len, 0);
                if ( num != 0 )
                {
                    datarate_ds[bc] = num;
                    downstr = 1;
                }
            }
            break;
        case 8: //  MAX_DELAY
            if ( line >= 0 )
            {
                ifx_ignore_space(&p2, &len);
                num = ifx_get_number(&p2, &len, 0);
                g_max_delay[line] = num;
                if ( g_dsl_bonding )
                    update_max_delay(line);
            }
            break;
        }

        p1 = p2;
        colon = 1;
    }

    if ( line >= 0 )
    {
        if ( down )
        {
            for ( i = 0, num = 0; i < LINE_NUMBER; num += g_line_showtime[i], i++ );
            if ( num == 0 )
            {
                dsl_showtime_exit();
                for ( i = 0; i < NUM_ENTITY(g_datarate_us); i++ )
                    g_datarate_us[i] = 0;
                for ( i = 0; i < NUM_ENTITY(g_datarate_ds); i++ )
                    g_datarate_ds[i] = 0;
            }
        }
        else
        {
            if ( upstr )
            {
                for ( i = 0; i < BEARER_CHANNEL_PER_LINE; i++ )
                    if ( datarate_us[i] != 0 )
                        g_datarate_us[line * BEARER_CHANNEL_PER_LINE + i] = datarate_us[i];
                g_line_showtime[line] = 1;
                g_showtime = 1; //  equivalent to dsl_showtime_enter
            }
            if ( downstr )
            {
                for ( i = 0; i < BEARER_CHANNEL_PER_LINE; i++ )
                    if ( datarate_ds[i] != 0 )
                        g_datarate_ds[line * BEARER_CHANNEL_PER_LINE + i] = datarate_ds[i];
                if ( g_dsl_bonding )
                    update_max_delay(line);
            }
        }
    }

    return 0;
}


static void proc_gamma_output(struct seq_file *m, void *private_data __maybe_unused){

	seq_printf(m,
			"PSAVE_CFG     (0x%08X): start_state - %s, sleep_en - %s\n",
			(unsigned int) PSAVE_CFG,
			PSAVE_CFG->start_state ? "partial reset" : "full reset",
			PSAVE_CFG->sleep_en ? "on" : "off");
	seq_printf(m,
			"EG_BWCTRL_CFG (0x%08X): fdesc_wm - %d, class_len - %d\n",
			(unsigned int) EG_BWCTRL_CFG, EG_BWCTRL_CFG->fdesc_wm,
			EG_BWCTRL_CFG->class_len);
	seq_printf(m,
			"TEST_MODE     (0x%08X): mib_clear_mode - %s, test_mode - %s\n",
			(unsigned int) TEST_MODE, TEST_MODE->mib_clear_mode ? "on" : "off",
			TEST_MODE->test_mode ? "on" : "off");
	seq_printf(m,  "RX_BC_CFG:\t0x%08x\t0x%08x\n",
			(unsigned int) RX_BC_CFG(0), (unsigned int) RX_BC_CFG(1));
	seq_printf(m,  "  local_state:    %8d\t  %8d\n",
			(unsigned int) RX_BC_CFG(0)->local_state,
			(unsigned int) RX_BC_CFG(1)->local_state);
	seq_printf(m,  "  remote_state:   %8d\t  %8d\n",
			(unsigned int) RX_BC_CFG(0)->remote_state,
			(unsigned int) RX_BC_CFG(1)->remote_state);
	seq_printf(m,  "  to_false_th:    %8d\t  %8d\n",
			(unsigned int) RX_BC_CFG(0)->to_false_th,
			(unsigned int) RX_BC_CFG(1)->to_false_th);
	seq_printf(m,  "  to_looking_th:  %8d\t  %8d\n",
			(unsigned int) RX_BC_CFG(0)->to_looking_th,
			(unsigned int) RX_BC_CFG(1)->to_looking_th);
	seq_printf(m,  "TX_BC_CFG:\t0x%08x\t0x%08x\n",
			(unsigned int) TX_BC_CFG(0), (unsigned int) TX_BC_CFG(1));
	seq_printf(m,  "  fill_wm:\t  %8d\t  %8d\n",
			(unsigned int) TX_BC_CFG(0)->fill_wm,
			(unsigned int) TX_BC_CFG(1)->fill_wm);
	seq_printf(m,  "  uflw_wm:\t  %8d\t  %8d\n",
			(unsigned int) TX_BC_CFG(0)->uflw_wm,
			(unsigned int) TX_BC_CFG(1)->uflw_wm);
	seq_printf(m,
			"RX_GAMMA_ITF_CFG:\t0x%08x\t0x%08x\t0x%08x\t0x%08x\n",
			(unsigned int) RX_GAMMA_ITF_CFG(0),
			(unsigned int) RX_GAMMA_ITF_CFG(1),
			(unsigned int) RX_GAMMA_ITF_CFG(2),
			(unsigned int) RX_GAMMA_ITF_CFG(3));
	seq_printf(m,
			"  receive_state: \t  %8d\t  %8d\t  %8d\t  %8d\n",
			(unsigned int) RX_GAMMA_ITF_CFG(0)->receive_state,
			(unsigned int) RX_GAMMA_ITF_CFG(1)->receive_state,
			(unsigned int) RX_GAMMA_ITF_CFG(2)->receive_state,
			(unsigned int) RX_GAMMA_ITF_CFG(3)->receive_state);
	seq_printf(m,
			"  rx_min_len:    \t  %8d\t  %8d\t  %8d\t  %8d\n",
			(unsigned int) RX_GAMMA_ITF_CFG(0)->rx_min_len,
			(unsigned int) RX_GAMMA_ITF_CFG(1)->rx_min_len,
			(unsigned int) RX_GAMMA_ITF_CFG(2)->rx_min_len,
			(unsigned int) RX_GAMMA_ITF_CFG(3)->rx_min_len);
	seq_printf(m,
			"  rx_pad_en:     \t  %8d\t  %8d\t  %8d\t  %8d\n",
			(unsigned int) RX_GAMMA_ITF_CFG(0)->rx_pad_en,
			(unsigned int) RX_GAMMA_ITF_CFG(1)->rx_pad_en,
			(unsigned int) RX_GAMMA_ITF_CFG(2)->rx_pad_en,
			(unsigned int) RX_GAMMA_ITF_CFG(3)->rx_pad_en);
	seq_printf(m,
			"  rx_eth_fcs_ver_dis:\t  %8d\t  %8d\t  %8d\t  %8d\n",
			(unsigned int) RX_GAMMA_ITF_CFG(0)->rx_eth_fcs_ver_dis,
			(unsigned int) RX_GAMMA_ITF_CFG(1)->rx_eth_fcs_ver_dis,
			(unsigned int) RX_GAMMA_ITF_CFG(2)->rx_eth_fcs_ver_dis,
			(unsigned int) RX_GAMMA_ITF_CFG(3)->rx_eth_fcs_ver_dis);
	seq_printf(m,
			"  rx_rm_eth_fcs: \t  %8d\t  %8d\t  %8d\t  %8d\n",
			(unsigned int) RX_GAMMA_ITF_CFG(0)->rx_rm_eth_fcs,
			(unsigned int) RX_GAMMA_ITF_CFG(1)->rx_rm_eth_fcs,
			(unsigned int) RX_GAMMA_ITF_CFG(2)->rx_rm_eth_fcs,
			(unsigned int) RX_GAMMA_ITF_CFG(3)->rx_rm_eth_fcs);
	seq_printf(m,
			"  rx_tc_crc_ver_dis:\t  %8d\t  %8d\t  %8d\t  %8d\n",
			(unsigned int) RX_GAMMA_ITF_CFG(0)->rx_tc_crc_ver_dis,
			(unsigned int) RX_GAMMA_ITF_CFG(1)->rx_tc_crc_ver_dis,
			(unsigned int) RX_GAMMA_ITF_CFG(2)->rx_tc_crc_ver_dis,
			(unsigned int) RX_GAMMA_ITF_CFG(3)->rx_tc_crc_ver_dis);
	seq_printf(m,
			"  rx_tc_crc_size:\t  %8d\t  %8d\t  %8d\t  %8d\n",
			(unsigned int) RX_GAMMA_ITF_CFG(0)->rx_tc_crc_size,
			(unsigned int) RX_GAMMA_ITF_CFG(1)->rx_tc_crc_size,
			(unsigned int) RX_GAMMA_ITF_CFG(2)->rx_tc_crc_size,
			(unsigned int) RX_GAMMA_ITF_CFG(3)->rx_tc_crc_size);
	seq_printf(m,
			"  rx_eth_fcs_result:\t0x%8X\t0x%8X\t0x%8X\t0x%8X\n",
			(unsigned int) RX_GAMMA_ITF_CFG(0)->rx_eth_fcs_result,
			(unsigned int) RX_GAMMA_ITF_CFG(1)->rx_eth_fcs_result,
			(unsigned int) RX_GAMMA_ITF_CFG(2)->rx_eth_fcs_result,
			(unsigned int) RX_GAMMA_ITF_CFG(3)->rx_eth_fcs_result);
	seq_printf(m,
			"  rx_tc_crc_result:\t0x%8X\t0x%8X\t0x%8X\t0x%8X\n",
			(unsigned int) RX_GAMMA_ITF_CFG(0)->rx_tc_crc_result,
			(unsigned int) RX_GAMMA_ITF_CFG(1)->rx_tc_crc_result,
			(unsigned int) RX_GAMMA_ITF_CFG(2)->rx_tc_crc_result,
			(unsigned int) RX_GAMMA_ITF_CFG(3)->rx_tc_crc_result);
	seq_printf(m,
			"  rx_crc_cfg:    \t  %8d\t  %8d\t  %8d\t  %8d\n",
			(unsigned int) RX_GAMMA_ITF_CFG(0)->rx_crc_cfg,
			(unsigned int) RX_GAMMA_ITF_CFG(1)->rx_crc_cfg,
			(unsigned int) RX_GAMMA_ITF_CFG(2)->rx_crc_cfg,
			(unsigned int) RX_GAMMA_ITF_CFG(3)->rx_crc_cfg);
	seq_printf(m,
			"  rx_eth_fcs_init_value:0x%08X\t0x%08X\t0x%08X\t0x%08X\n",
			(unsigned int) RX_GAMMA_ITF_CFG(0)->rx_eth_fcs_init_value,
			(unsigned int) RX_GAMMA_ITF_CFG(1)->rx_eth_fcs_init_value,
			(unsigned int) RX_GAMMA_ITF_CFG(2)->rx_eth_fcs_init_value,
			(unsigned int) RX_GAMMA_ITF_CFG(3)->rx_eth_fcs_init_value);
	seq_printf(m,
			"  rx_tc_crc_init_value:\t0x%08X\t0x%08X\t0x%08X\t0x%08X\n",
			(unsigned int) RX_GAMMA_ITF_CFG(0)->rx_tc_crc_init_value,
			(unsigned int) RX_GAMMA_ITF_CFG(1)->rx_tc_crc_init_value,
			(unsigned int) RX_GAMMA_ITF_CFG(2)->rx_tc_crc_init_value,
			(unsigned int) RX_GAMMA_ITF_CFG(3)->rx_tc_crc_init_value);
	seq_printf(m,
			"  rx_max_len_sel:\t  %8d\t  %8d\t  %8d\t  %8d\n",
			(unsigned int) RX_GAMMA_ITF_CFG(0)->rx_max_len_sel,
			(unsigned int) RX_GAMMA_ITF_CFG(1)->rx_max_len_sel,
			(unsigned int) RX_GAMMA_ITF_CFG(2)->rx_max_len_sel,
			(unsigned int) RX_GAMMA_ITF_CFG(3)->rx_max_len_sel);
	seq_printf(m,
			"  rx_edit_num2:  \t  %8d\t  %8d\t  %8d\t  %8d\n",
			(unsigned int) RX_GAMMA_ITF_CFG(0)->rx_edit_num2,
			(unsigned int) RX_GAMMA_ITF_CFG(1)->rx_edit_num2,
			(unsigned int) RX_GAMMA_ITF_CFG(2)->rx_edit_num2,
			(unsigned int) RX_GAMMA_ITF_CFG(3)->rx_edit_num2);
	seq_printf(m,
			"  rx_edit_pos2:  \t  %8d\t  %8d\t  %8d\t  %8d\n",
			(unsigned int) RX_GAMMA_ITF_CFG(0)->rx_edit_pos2,
			(unsigned int) RX_GAMMA_ITF_CFG(1)->rx_edit_pos2,
			(unsigned int) RX_GAMMA_ITF_CFG(2)->rx_edit_pos2,
			(unsigned int) RX_GAMMA_ITF_CFG(3)->rx_edit_pos2);
	seq_printf(m,
			"  rx_edit_type2: \t  %8d\t  %8d\t  %8d\t  %8d\n",
			(unsigned int) RX_GAMMA_ITF_CFG(0)->rx_edit_type2,
			(unsigned int) RX_GAMMA_ITF_CFG(1)->rx_edit_type2,
			(unsigned int) RX_GAMMA_ITF_CFG(2)->rx_edit_type2,
			(unsigned int) RX_GAMMA_ITF_CFG(3)->rx_edit_type2);
	seq_printf(m,
			"  rx_edit_en2:   \t  %8d\t  %8d\t  %8d\t  %8d\n",
			(unsigned int) RX_GAMMA_ITF_CFG(0)->rx_edit_en2,
			(unsigned int) RX_GAMMA_ITF_CFG(1)->rx_edit_en2,
			(unsigned int) RX_GAMMA_ITF_CFG(2)->rx_edit_en2,
			(unsigned int) RX_GAMMA_ITF_CFG(3)->rx_edit_en2);
	seq_printf(m,
			"  rx_edit_num1:  \t  %8d\t  %8d\t  %8d\t  %8d\n",
			(unsigned int) RX_GAMMA_ITF_CFG(0)->rx_edit_num1,
			(unsigned int) RX_GAMMA_ITF_CFG(1)->rx_edit_num1,
			(unsigned int) RX_GAMMA_ITF_CFG(2)->rx_edit_num1,
			(unsigned int) RX_GAMMA_ITF_CFG(3)->rx_edit_num1);
	seq_printf(m,
			"  rx_edit_pos1:  \t  %8d\t  %8d\t  %8d\t  %8d\n",
			(unsigned int) RX_GAMMA_ITF_CFG(0)->rx_edit_pos1,
			(unsigned int) RX_GAMMA_ITF_CFG(1)->rx_edit_pos1,
			(unsigned int) RX_GAMMA_ITF_CFG(2)->rx_edit_pos1,
			(unsigned int) RX_GAMMA_ITF_CFG(3)->rx_edit_pos1);
	seq_printf(m,
			"  rx_edit_type1: \t  %8d\t  %8d\t  %8d\t  %8d\n",
			(unsigned int) RX_GAMMA_ITF_CFG(0)->rx_edit_type1,
			(unsigned int) RX_GAMMA_ITF_CFG(1)->rx_edit_type1,
			(unsigned int) RX_GAMMA_ITF_CFG(2)->rx_edit_type1,
			(unsigned int) RX_GAMMA_ITF_CFG(3)->rx_edit_type1);
	seq_printf(m,
			"  rx_edit_en1:   \t  %8d\t  %8d\t  %8d\t  %8d\n",
			(unsigned int) RX_GAMMA_ITF_CFG(0)->rx_edit_en1,
			(unsigned int) RX_GAMMA_ITF_CFG(1)->rx_edit_en1,
			(unsigned int) RX_GAMMA_ITF_CFG(2)->rx_edit_en1,
			(unsigned int) RX_GAMMA_ITF_CFG(3)->rx_edit_en1);
	seq_printf(m,
			"  rx_inserted_bytes_1l:\t0x%08X\t0x%08X\t0x%08X\t0x%08X\n",
			(unsigned int) RX_GAMMA_ITF_CFG(0)->rx_inserted_bytes_1l,
			(unsigned int) RX_GAMMA_ITF_CFG(1)->rx_inserted_bytes_1l,
			(unsigned int) RX_GAMMA_ITF_CFG(2)->rx_inserted_bytes_1l,
			(unsigned int) RX_GAMMA_ITF_CFG(3)->rx_inserted_bytes_1l);
	seq_printf(m,
			"  rx_inserted_bytes_1h:\t0x%08X\t0x%08X\t0x%08X\t0x%08X\n",
			(unsigned int) RX_GAMMA_ITF_CFG(0)->rx_inserted_bytes_1h,
			(unsigned int) RX_GAMMA_ITF_CFG(1)->rx_inserted_bytes_1h,
			(unsigned int) RX_GAMMA_ITF_CFG(2)->rx_inserted_bytes_1h,
			(unsigned int) RX_GAMMA_ITF_CFG(3)->rx_inserted_bytes_1h);
	seq_printf(m,
			"  rx_inserted_bytes_2l:\t0x%08X\t0x%08X\t0x%08X\t0x%08X\n",
			(unsigned int) RX_GAMMA_ITF_CFG(0)->rx_inserted_bytes_2l,
			(unsigned int) RX_GAMMA_ITF_CFG(1)->rx_inserted_bytes_2l,
			(unsigned int) RX_GAMMA_ITF_CFG(2)->rx_inserted_bytes_2l,
			(unsigned int) RX_GAMMA_ITF_CFG(3)->rx_inserted_bytes_2l);
	seq_printf(m,
			"  rx_inserted_bytes_2h:\t0x%08X\t0x%08X\t0x%08X\t0x%08X\n",
			(unsigned int) RX_GAMMA_ITF_CFG(0)->rx_inserted_bytes_2h,
			(unsigned int) RX_GAMMA_ITF_CFG(1)->rx_inserted_bytes_2h,
			(unsigned int) RX_GAMMA_ITF_CFG(2)->rx_inserted_bytes_2h,
			(unsigned int) RX_GAMMA_ITF_CFG(3)->rx_inserted_bytes_2h);
	seq_printf(m,
			"RX_GAMMA_ITF_CFG:\t0x%08x\t0x%08x\t0x%08x\t0x%08x\n",
			(unsigned int) TX_GAMMA_ITF_CFG(0),
			(unsigned int) TX_GAMMA_ITF_CFG(1),
			(unsigned int) TX_GAMMA_ITF_CFG(2),
			(unsigned int) TX_GAMMA_ITF_CFG(3));
	seq_printf(m,
			"  tx_len_adj:    \t  %8d\t  %8d\t  %8d\t  %8d\n",
			(unsigned int) TX_GAMMA_ITF_CFG(0)->tx_len_adj,
			(unsigned int) TX_GAMMA_ITF_CFG(1)->tx_len_adj,
			(unsigned int) TX_GAMMA_ITF_CFG(2)->tx_len_adj,
			(unsigned int) TX_GAMMA_ITF_CFG(3)->tx_len_adj);
	seq_printf(m,
			"  tx_crc_off_adj:\t  %8d\t  %8d\t  %8d\t  %8d\n",
			(unsigned int) TX_GAMMA_ITF_CFG(0)->tx_crc_off_adj,
			(unsigned int) TX_GAMMA_ITF_CFG(1)->tx_crc_off_adj,
			(unsigned int) TX_GAMMA_ITF_CFG(2)->tx_crc_off_adj,
			(unsigned int) TX_GAMMA_ITF_CFG(3)->tx_crc_off_adj);
	seq_printf(m,
			"  tx_min_len:    \t  %8d\t  %8d\t  %8d\t  %8d\n",
			(unsigned int) TX_GAMMA_ITF_CFG(0)->tx_min_len,
			(unsigned int) TX_GAMMA_ITF_CFG(1)->tx_min_len,
			(unsigned int) TX_GAMMA_ITF_CFG(2)->tx_min_len,
			(unsigned int) TX_GAMMA_ITF_CFG(3)->tx_min_len);
	seq_printf(m,
			"  tx_eth_fcs_gen_dis:\t  %8d\t  %8d\t  %8d\t  %8d\n",
			(unsigned int) TX_GAMMA_ITF_CFG(0)->tx_eth_fcs_gen_dis,
			(unsigned int) TX_GAMMA_ITF_CFG(1)->tx_eth_fcs_gen_dis,
			(unsigned int) TX_GAMMA_ITF_CFG(2)->tx_eth_fcs_gen_dis,
			(unsigned int) TX_GAMMA_ITF_CFG(3)->tx_eth_fcs_gen_dis);
	seq_printf(m,
			"  tx_tc_crc_size:\t  %8d\t  %8d\t  %8d\t  %8d\n",
			(unsigned int) TX_GAMMA_ITF_CFG(0)->tx_tc_crc_size,
			(unsigned int) TX_GAMMA_ITF_CFG(1)->tx_tc_crc_size,
			(unsigned int) TX_GAMMA_ITF_CFG(2)->tx_tc_crc_size,
			(unsigned int) TX_GAMMA_ITF_CFG(3)->tx_tc_crc_size);
	seq_printf(m,
			"  tx_crc_cfg:    \t0x%08X\t0x%08X\t0x%08X\t0x%08X\n",
			(unsigned int) TX_GAMMA_ITF_CFG(0)->tx_crc_cfg,
			(unsigned int) TX_GAMMA_ITF_CFG(1)->tx_crc_cfg,
			(unsigned int) TX_GAMMA_ITF_CFG(2)->tx_crc_cfg,
			(unsigned int) TX_GAMMA_ITF_CFG(3)->tx_crc_cfg);
	seq_printf(m,
			"  tx_eth_fcs_init_value:0x%08X\t0x%08X\t0x%08X\t0x%08X\n",
			(unsigned int) TX_GAMMA_ITF_CFG(0)->tx_eth_fcs_init_value,
			(unsigned int) TX_GAMMA_ITF_CFG(1)->tx_eth_fcs_init_value,
			(unsigned int) TX_GAMMA_ITF_CFG(2)->tx_eth_fcs_init_value,
			(unsigned int) TX_GAMMA_ITF_CFG(3)->tx_eth_fcs_init_value);
	seq_printf(m,
			"  tx_tc_crc_init_value:\t0x%08X\t0x%08X\t0x%08X\t0x%08X\n",
			(unsigned int) TX_GAMMA_ITF_CFG(0)->tx_tc_crc_init_value,
			(unsigned int) TX_GAMMA_ITF_CFG(1)->tx_tc_crc_init_value,
			(unsigned int) TX_GAMMA_ITF_CFG(2)->tx_tc_crc_init_value,
			(unsigned int) TX_GAMMA_ITF_CFG(3)->tx_tc_crc_init_value);
	seq_printf(m,
			"  queue_mapping: \t0x%08X\t0x%08X\t0x%08X\t0x%08X\n",
			(unsigned int) TX_GAMMA_ITF_CFG(0)->queue_mapping,
			(unsigned int) TX_GAMMA_ITF_CFG(1)->queue_mapping,
			(unsigned int) TX_GAMMA_ITF_CFG(2)->queue_mapping,
			(unsigned int) TX_GAMMA_ITF_CFG(3)->queue_mapping);

}


static void proc_wanmib_output(struct seq_file *m, void *private_data __maybe_unused){

    volatile unsigned int *wrx_total_pdu[4]         = {DREG_AR_AIIDLE_CNT0,  DREG_AR_HEC_CNT0,    DREG_AR_AIIDLE_CNT1, DREG_AR_HEC_CNT1};
    volatile unsigned int *wrx_crc_err_pdu[4]       = {GIF0_RX_CRC_ERR_CNT,  GIF1_RX_CRC_ERR_CNT, GIF2_RX_CRC_ERR_CNT, GIF3_RX_CRC_ERR_CNT};
    volatile unsigned int *wrx_cv_cw_cnt[4]         = {GIF0_RX_CV_CNT,       GIF1_RX_CV_CNT,      GIF2_RX_CV_CNT,      GIF3_RX_CV_CNT};
    volatile unsigned int *wrx_bc_overdrop_cnt[2]   = {DREG_B0_OVERDROP_CNT, DREG_B1_OVERDROP_CNT};
    int i;

    seq_printf(m,  "RX (Bearer Channels):\n");
    seq_printf(m,  "   wrx_bc_overdrop:");
    for ( i = 0; i < 2; i++ )
    {
        if ( i != 0 )
            seq_printf(m,  ",             ");
        seq_printf(m,  "%10u", IFX_REG_R32(wrx_bc_overdrop_cnt[i]) - last_wrx_bc_overdrop_cnt[PPE_ID][i]);
    }
    seq_printf(m,  "\n");
    seq_printf(m,  "   wrx_bc_user_cw: ");
    for ( i = 0; i < 2; i++ )
    {
        if ( i != 0 )
            seq_printf(m,  ",             ");
        seq_printf(m,  "%10u", IFX_REG_R32(RECEIVE_NON_IDLE_CELL_CNT(i)));
    }
    seq_printf(m,  "\n");
    seq_printf(m,  "   wrx_bc_idle_cw: ");
    for ( i = 0; i < 2; i++ )
    {
        if ( i != 0 )
            seq_printf(m,  ",             ");
        seq_printf(m,  "%10u", IFX_REG_R32(RECEIVE_IDLE_CELL_CNT(i)));
    }
    seq_printf(m,  "\n");

    seq_printf(m,  "RX (Gamma Interfaces):\n");
    seq_printf(m,  "  wrx_total_pdu:   ");
    for ( i = 0; i < 4; i++ )
    {
        if ( i != 0 )
            seq_printf(m,  ", ");
        seq_printf(m,  "%10u", IFX_REG_R32(wrx_total_pdu[i]) - last_wrx_total_pdu[PPE_ID][i]);
    }
    seq_printf(m,  "\n");
    seq_printf(m,  "  wrx_dropdes_pdu: ");
    for ( i = 0; i < 4; i++ )
    {
        if ( i != 0 )
            seq_printf(m,  ", ");
        seq_printf(m,  "%10u", WAN_RX_MIB_TABLE(i)->wrx_dropdes_pdu);
    }
    seq_printf(m,  "\n");
    seq_printf(m,  "  wrx_crc_err_pdu: ");
    for ( i = 0; i < 4; i++ )
    {
        if ( i != 0 )
            seq_printf(m,  ", ");
        seq_printf(m,  "%10u", IFX_REG_R32(wrx_cv_cw_cnt[i]) - last_wrx_cv_cw_cnt[PPE_ID][i]);
    }
    seq_printf(m,  "\n");
    seq_printf(m,  "  wrx_violated_cw: ");
    for ( i = 0; i < 4; i++ )
    {
        if ( i != 0 )
            seq_printf(m,  ", ");
        seq_printf(m,  "%10u", IFX_REG_R32(wrx_crc_err_pdu[i]) - last_wrx_crc_error_total_pdu[PPE_ID][i]);
    }
    seq_printf(m,  "\n");
    seq_printf(m,  "  wrx_total_bytes: ");
    for ( i = 0; i < 4; i++ )
    {
        if ( i != 0 )
            seq_printf(m,  ", ");
        seq_printf(m,  "%10u", WAN_RX_MIB_TABLE(i)->wrx_total_bytes);
    }
    seq_printf(m,  "\n");

    seq_printf(m,  "TX (Bearer Channels):\n");
    seq_printf(m,  "  total_tx_cw:     %10u,             %10u\n", IFX_REG_R32(TRANSMIT_CELL_CNT(0)), IFX_REG_R32(TRANSMIT_CELL_CNT(1)));

    seq_printf(m,  "TX (QoS Queues):\n");
    seq_printf(m,  "  wrx_total_pdu:           ");
    for ( i = 0; i < 8; i++ )
    {
        if ( i != 0 )
            seq_printf(m,  ", ");
        seq_printf(m,  "%10u", WAN_TX_MIB_TABLE(i)->wrx_total_pdu);
    }
    seq_printf(m,  "\n");
    seq_printf(m,  "  wrx_total_bytes:         ");
    for ( i = 0; i < 8; i++ )
    {
        if ( i != 0 )
            seq_printf(m,  ", ");
        seq_printf(m,  "%10u", WAN_TX_MIB_TABLE(i)->wrx_total_bytes);
    }
    seq_printf(m,  "\n");
    seq_printf(m,  "  wtx_total_pdu:           ");
    for ( i = 0; i < 8; i++ )
    {
        if ( i != 0 )
            seq_printf(m,  ", ");
        seq_printf(m,  "%10u", WAN_TX_MIB_TABLE(i)->wtx_total_pdu);
    }
    seq_printf(m,  "\n");
    seq_printf(m,  "  wtx_total_bytes:         ");
    for ( i = 0; i < 8; i++ )
    {
        if ( i != 0 )
            seq_printf(m,  ", ");
        seq_printf(m,  "%10u", WAN_TX_MIB_TABLE(i)->wtx_total_bytes);
    }
    seq_printf(m,  "\n");
    seq_printf(m,  "  wtx_cpu_drop_small_pdu:  ");
    for ( i = 0; i < 8; i++ )
    {
        if ( i != 0 )
            seq_printf(m,  ", ");
        seq_printf(m,  "%10u", WAN_TX_MIB_TABLE(i)->wtx_cpu_drop_small_pdu);
    }
    seq_printf(m,  "\n");
    seq_printf(m,  "  wtx_cpu_drop_pdu:        ");
    for ( i = 0; i < 8; i++ )
    {
        if ( i != 0 )
            seq_printf(m,  ", ");
        seq_printf(m,  "%10u", WAN_TX_MIB_TABLE(i)->wtx_cpu_drop_pdu);
    }
    seq_printf(m,  "\n");
    seq_printf(m,  "  wtx_fast_drop_small_pdu: ");
    for ( i = 0; i < 8; i++ )
    {
        if ( i != 0 )
            seq_printf(m,  ", ");
        seq_printf(m,  "%10u", WAN_TX_MIB_TABLE(i)->wtx_fast_drop_small_pdu);
    }
    seq_printf(m,  "\n");
    seq_printf(m,  "  wtx_fast_drop_pdu:       ");
    for ( i = 0; i < 8; i++ )
    {
        if ( i != 0 )
            seq_printf(m,  ", ");
        seq_printf(m,  "%10u", WAN_TX_MIB_TABLE(i)->wtx_fast_drop_pdu);
    }
    seq_printf(m,  "\n");


    seq_printf(m,  "dma_alignment_ptm_good_count: %d\n", dma_alignment_ptm_good_count );
    seq_printf(m,  "dma_alignment_ptm_bad_count: %d\n", dma_alignment_ptm_bad_count   );
    seq_printf(m,  "dma_alignment_ethwan_qos_good_count: %d\n", dma_alignment_ethwan_qos_good_count );
    seq_printf(m,  "dma_alignment_ethwan_qos_bad_count: %d\n", dma_alignment_ethwan_qos_bad_count   );
    seq_printf(m,  "dma_alignment_eth_good_count: %d\n", dma_alignment_eth_good_count );
    seq_printf(m,  "dma_alignment_eth_bad_count: %d\n", dma_alignment_eth_bad_count   );
#if (defined(CONFIG_VR9) || defined(CONFIG_AR10))
    seq_printf(m,  "padded_frame_count: %d\n", padded_frame_count);
#endif

#if defined(DEBUG_SPACE_BETWEEN_HEADER_AND_DATA)
    seq_printf(m,  "min_fheader_byteoff: %d\n", min_fheader_byteoff);
#endif

}

static int proc_wanmib_input(char *line, void *data __maybe_unused) {

	char *p = line;

    /*--- volatile unsigned int *wrx_total_pdu[4]         = {DREG_AR_AIIDLE_CNT0,  DREG_AR_HEC_CNT0,    DREG_AR_AIIDLE_CNT1, DREG_AR_HEC_CNT1}; ---*/
    /*--- volatile unsigned int *wrx_crc_err_pdu[4]       = {GIF0_RX_CRC_ERR_CNT,  GIF1_RX_CRC_ERR_CNT, GIF2_RX_CRC_ERR_CNT, GIF3_RX_CRC_ERR_CNT}; ---*/
    /*--- volatile unsigned int *wrx_cv_cw_cnt[4]         = {GIF0_RX_CV_CNT,       GIF1_RX_CV_CNT,      GIF2_RX_CV_CNT,      GIF3_RX_CV_CNT}; ---*/
    /*--- volatile unsigned int *wrx_bc_overdrop_cnt[2]   = {DREG_B0_OVERDROP_CNT, DREG_B1_OVERDROP_CNT}; ---*/
	/*--- int i; ---*/

	if (ifx_stricmp(p, "clear") == 0 || ifx_stricmp(p, "clean") == 0) {
		// die Schleife wirft leider die Warnung: 'Null-Argument, wo Nicht-Null erwartet (Argument 1)'
#if 0
	    for ( i = 0; i < 4; i++ ) {
	        last_wrx_total_pdu[PPE_ID][i]           = IFX_REG_R32(wrx_total_pdu[i]);
	        last_wrx_crc_error_total_pdu[PPE_ID][i] = IFX_REG_R32(wrx_crc_err_pdu[i]);
	        last_wrx_cv_cw_cnt[PPE_ID][i]           = IFX_REG_R32(wrx_cv_cw_cnt[i]);
	        if ( i < 2 )
	        {
	            last_wrx_bc_overdrop_cnt[PPE_ID][i] = IFX_REG_R32(wrx_bc_overdrop_cnt[i]);
	            *RECEIVE_NON_IDLE_CELL_CNT(i)       = 0;
	            *RECEIVE_IDLE_CELL_CNT(i)           = 0;
	            *TRANSMIT_CELL_CNT(i)               = 0;
	        }
	        ppa_memset((void* )WAN_RX_MIB_TABLE(i), 0, sizeof(*WAN_RX_MIB_TABLE(i)));
	    }
	    for ( i = 0; i < 8; i++ )
	        ppa_memset((void*)WAN_TX_MIB_TABLE(i), 0, sizeof(*WAN_TX_MIB_TABLE(i)));

#else
		memset((void* )WAN_RX_MIB_TABLE(0), 0, sizeof(*WAN_RX_MIB_TABLE(0)));
		memset((void* )WAN_RX_MIB_TABLE(1), 0, sizeof(*WAN_RX_MIB_TABLE(1)));
		memset((void* )WAN_RX_MIB_TABLE(2), 0, sizeof(*WAN_RX_MIB_TABLE(2)));
		memset((void* )WAN_RX_MIB_TABLE(3), 0, sizeof(*WAN_RX_MIB_TABLE(3)));

		memset((void*)WAN_TX_MIB_TABLE(0), 0, sizeof(*WAN_TX_MIB_TABLE(0)));
		memset((void*)WAN_TX_MIB_TABLE(1), 0, sizeof(*WAN_TX_MIB_TABLE(1)));
		memset((void*)WAN_TX_MIB_TABLE(2), 0, sizeof(*WAN_TX_MIB_TABLE(2)));
		memset((void*)WAN_TX_MIB_TABLE(3), 0, sizeof(*WAN_TX_MIB_TABLE(3)));
		memset((void*)WAN_TX_MIB_TABLE(4), 0, sizeof(*WAN_TX_MIB_TABLE(4)));
		memset((void*)WAN_TX_MIB_TABLE(5), 0, sizeof(*WAN_TX_MIB_TABLE(5)));
		memset((void*)WAN_TX_MIB_TABLE(6), 0, sizeof(*WAN_TX_MIB_TABLE(6)));
		memset((void*)WAN_TX_MIB_TABLE(7), 0, sizeof(*WAN_TX_MIB_TABLE(7)));
#endif
	}

	return 0;
}

#endif // #ifdef PTM_DATAPATH


#if defined(ATM_DATAPATH)
static int avm_get_atm_qid(struct atm_vcc *vcc, unsigned int prio) {
    int res = 0;
    int conn = find_vcc(vcc);

    if (conn >= 0 && conn < ATM_QUEUE_NUMBER){
       res |= g_atm_priv_data.connection[conn].prio_queue_map[prio > 7 ? 7 : prio];
       res |= (conn << 8);
    }
    return res;
}
#elif defined(PTM_DATAPATH)
static int avm_get_atm_qid(struct atm_vcc *vcc __maybe_unused, 
                unsigned int skb_priority  __maybe_unused) {
    //we need this dummy for PTM driver
    return -1;
}
#endif //XTM_DATAPATH

static int avm_get_eth_qid(unsigned int mac_nr __maybe_unused,
			   unsigned int skb_priority) {
    int qid;
    unsigned int priority = (skb_priority & TC_H_MIN_MASK);

    /* AVMGRM: Map priority from TC priority */
    if (priority >= NUM_ENTITY( g_eth_prio_queue_map) )
        qid = g_eth_prio_queue_map[NUM_ENTITY(g_eth_prio_queue_map) - 1];
    else
        qid = g_eth_prio_queue_map[priority];

    return qid;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

static struct sk_buff* skb_break_away_from_protocol_avm(struct sk_buff *skb) {
	skb = skb_share_check(skb, GFP_ATOMIC);
	if (skb == NULL)
		return NULL;

	skb_dst_drop(skb);
#ifdef CONFIG_XFRM
	secpath_put(skb->sp);
	skb->sp = NULL;
#endif
	/*
	 *  ccb: use orphan instead of lantiq concept
	 */
	skb_orphan(skb);
	return skb;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static INLINE void __get_skb_from_dbg_pool(
		struct sk_buff *skb __attribute__((unused)),
		const char *func_name __attribute__((unused)),
		unsigned int line_num __attribute__((unused)))
{
#if defined(DEBUG_SKB_SWAP) && DEBUG_SKB_SWAP
    int i;

    for ( i = 0; i < NUM_ENTITY(g_dbg_skb_swap_pool) && g_dbg_skb_swap_pool[i] != skb; i++ );
    if ( i == NUM_ENTITY(g_dbg_skb_swap_pool) || (unsigned int)skb < KSEG0 )
    {
        err("%s:%d: skb (0x%08x) not in g_dbg_skb_swap_pool", func_name, line_num, (unsigned int)skb);
    }
    else
        g_dbg_skb_swap_pool[i] = NULL;
#endif
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static INLINE void __put_skb_to_dbg_pool(struct sk_buff *skb __attribute__((unused)),
		const char *func_name __attribute__((unused)), unsigned int line_num __attribute__((unused))) {
#if defined(DEBUG_SKB_SWAP) && DEBUG_SKB_SWAP
	int i;

	for ( i = 0; i < NUM_ENTITY(g_dbg_skb_swap_pool) && g_dbg_skb_swap_pool[i] != NULL; i++ );
	if ( i == NUM_ENTITY(g_dbg_skb_swap_pool) )
	{
		err("%s:%d: g_dbg_skb_swap_pool overrun", func_name, line_num);
	}
	else
	g_dbg_skb_swap_pool[i] = skb;
#endif
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/


static INLINE struct sk_buff **dmadataptr_to_skbptr(unsigned int dataptr){
    unsigned int skb_dataptr;

    skb_dataptr = (dataptr - 4) | KSEG1;
    return (struct sk_buff **)skb_dataptr;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/


static INLINE void __free_skb_clear_dataptr(unsigned int dataptr, const char *func_name, unsigned int line_num) {
    struct sk_buff **skbp;
    struct sk_buff *skb;


    /*
     * usually, CPE memory is less than 256M bytes
     * so NULL means invalid pointer
     */
    if ( dataptr == 0 ) {
        DBG_SKB_FREE("dataptr is 0, caller=%s at line %d\n", func_name, line_num);
        return;
    }

    skbp = dmadataptr_to_skbptr( dataptr );
    skb = *skbp;
    if (!skb){
        DBG_SKB_FREE("implicit skb at dma dataptr %#x is 0, caller=%s at line %d\n", dataptr, func_name, line_num);
    	return;
    }
    *skbp = NULL;

    dev_kfree_skb_any(skb);

    __get_skb_from_dbg_pool(skb, func_name, line_num);

	ASSERT((unsigned int)skb >= KSEG0, "%s:%d: invalid skb - skb = %#08x, dataptr = %#08x", func_name, line_num, (unsigned int)skb, dataptr);
	ASSERT( (((unsigned int)skb->data & (0x1FFFFFFF ^ (DMA_RX_ALIGNMENT - 1))) | KSEG1) == (dataptr | KSEG1) ||
			(((unsigned int)skb->data & (0x1FFFFFFF ^ (DMA_TX_ALIGNMENT - 1))) | KSEG1) == (dataptr | KSEG1),
			"%s:%d: invalid skb - skb = %#08x, skb->data = %#08x, dataptr = %#08x",
			func_name, line_num, (unsigned int)skb, (unsigned int)skb->data, dataptr
			);

	smp_mb();

    return;
}


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

#if defined(DATAPATH_7PORT)
/*
 * dma u. directpath Routinen unterscheiden sich auf dem AR9 
 */

static inline int dma_rx_int_handler(struct dma_device_info* dma_dev, int dma_chan_no)
{
    unsigned char* buf = NULL;
    int len = 0;
    struct sk_buff *skb = NULL;
    struct softnet_data *queue;
    unsigned long flags;

    len = dma_device_read(dma_dev, &buf, (void**)&skb, dma_chan_no);

    if(unlikely(skb == NULL)){
        if (net_ratelimit())
            err( "%s[%d]: Can't restore the Packet !!!\n", __func__,__LINE__);
        return 0;
    }

    dbg_dma( "[%s] dma_device_read len:%d, skb->len:%d\n",__func__,len, skb->len);

    get_skb_from_dbg_pool(skb);

    if(unlikely(g_stop_datapath)){
        dbg("[%s]: datapath to be stopped, dropping skb.\n", __func__);
        dev_kfree_skb_any(skb);
        return 0;
    }

    if(unlikely((len > (skb->end - skb->data)) || (len < 64))){
        err( "%s[%d]: Packet is too large/small (%d)!!!\n", __func__,__LINE__,len);
        dev_kfree_skb_any(skb);
        return 0;
    }

    /* do not remove CRC */
    // len -= 4;
    if(unlikely(len > (skb->end - skb->tail))){
        err( "%s[%d]: len:%d end:%p tail:%p Err!!!\n", __func__, __LINE__, len, skb->end, skb->tail);
        dev_kfree_skb_any(skb);
        return 0;
    }

    skb_put(skb, len);

    if(buf){
        dump_skb(skb, DUMP_SKB_LEN, "dma_rx_int_handler", 0, dma_chan_no, 0, 0);
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

    return 0;
}
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

static int dma_int_handler(struct dma_device_info *dma_dev, int status, int chan_no) {
	int ret = 0;
	unsigned long sys_flag;

	switch (status) {
	case RCV_INT:
		ret = dma_rx_int_handler(dma_dev, chan_no);
		break;
	case TX_BUF_FULL_INT:
		if (chan_no == DMA_CHAN_NR_DEFAULT_CPU_TX) {
			unsigned int dev_nr;
			dbg("eth0/1 TX buffer full!");
			spin_lock_irqsave(&g_eth_tx_spinlock, sys_flag);

			for (dev_nr = 0; dev_nr < num_registered_eth_dev; dev_nr++){
				struct net_device *dev = g_eth_net_dev[dev_nr];
				avmnet_netdev_priv_t *priv = netdev_priv(dev);
				avmnet_device_t *avm_dev = avm_dev = priv->avm_dev;
				avm_dev->flags |= AVMNET_DEVICE_FLAG_DMA_TX_QUEUE_FULL;
				netif_stop_queue(dev);
				AVMNET_DBG_TX_QUEUE_RATE("TX_BUF_FULL_INT stop device %s" ,dev->name);
			}

			if (g_dma_device_ppe->tx_chan[DMA_CHAN_NR_DEFAULT_CPU_TX]->control == IFX_DMA_CH_ON){
				g_dma_device_ppe->tx_chan[DMA_CHAN_NR_DEFAULT_CPU_TX]
					->enable_irq( g_dma_device_ppe->tx_chan[DMA_CHAN_NR_DEFAULT_CPU_TX]);
			}
			spin_unlock_irqrestore(&g_eth_tx_spinlock, sys_flag);

		}
#ifdef CONFIG_AVM_PA
		else if (chan_no == DMA_CHAN_NR_DIRECTPATH_CPU_TO_PPE) {
			unsigned int i;
			dbg("directpath TX (CPU->PPE) buffer full!");
			// hier wird der Spinlock g_directpath_tx_spinlock bereits gehalten
			//
			// man kommt immer aus dem Kontext
			// ppe_directpath_send->eth_xmit->dma_device_write 
			if (!g_directpath_dma_full) {
				g_directpath_dma_full = 1;
				if (g_dma_device_ppe->tx_chan[DMA_CHAN_NR_DIRECTPATH_CPU_TO_PPE]->control == IFX_DMA_CH_ON){
					g_dma_device_ppe->tx_chan[DMA_CHAN_NR_DIRECTPATH_CPU_TO_PPE]
						->enable_irq( g_dma_device_ppe->tx_chan[DMA_CHAN_NR_DIRECTPATH_CPU_TO_PPE]);
				}
				for (i = 0; i < MAX_PPA_PUSHTHROUGH_DEVICES; i++)
					if ( ppa_virt_rx_devs[i] )
						avm_pa_rx_channel_suspend( ppa_virt_rx_devs[i]->pid_handle );
			}
		}
#endif /* CONFIG_AVM_PA */
		else {
			err( "incorrect DMA TX channel: %d (0 - 1 is reserved for fast path)", chan_no);
		}
		break;
	case TRANSMIT_CPT_INT:
		if (chan_no == DMA_CHAN_NR_DEFAULT_CPU_TX ) {
			unsigned int dev_nr;
			dbg("eth0/1 TX buffer released!");
			spin_lock_irqsave(&g_eth_tx_spinlock, sys_flag);
			g_dma_device_ppe->tx_chan[DMA_CHAN_NR_DEFAULT_CPU_TX]
				->disable_irq(g_dma_device_ppe->tx_chan[DMA_CHAN_NR_DEFAULT_CPU_TX]);

			for (dev_nr = 0; dev_nr < num_registered_eth_dev; dev_nr++){
				struct net_device *dev = g_eth_net_dev[dev_nr];
				avmnet_netdev_priv_t *priv = netdev_priv(dev);
				avmnet_device_t *avm_dev = avm_dev = priv->avm_dev;
				avm_dev->flags &= ~AVMNET_DEVICE_FLAG_DMA_TX_QUEUE_FULL;
				netif_wake_queue( dev );
				AVMNET_DBG_TX_QUEUE_RATE("TRANSMIT_CPT_INT wake device %s" ,dev->name);
			}

			spin_unlock_irqrestore(&g_eth_tx_spinlock, sys_flag);
		}
#ifdef CONFIG_AVM_PA
		else if (chan_no == DMA_CHAN_NR_DIRECTPATH_CPU_TO_PPE) {
			unsigned int i;
			dbg("directpath TX (CPU->PPE) buffer released");
			spin_lock_irqsave(&g_directpath_tx_spinlock, sys_flag);
			if (g_directpath_dma_full) {
				g_directpath_dma_full = 0;
				g_dma_device_ppe->tx_chan[DMA_CHAN_NR_DIRECTPATH_CPU_TO_PPE]
					->disable_irq( g_dma_device_ppe->tx_chan[DMA_CHAN_NR_DIRECTPATH_CPU_TO_PPE]);
				for (i = 0; i < MAX_PPA_PUSHTHROUGH_DEVICES; i++)
					if ( ppa_virt_rx_devs[i] )
						avm_pa_rx_channel_resume( ppa_virt_rx_devs[i]->pid_handle );
			}
			spin_unlock_irqrestore(&g_directpath_tx_spinlock, sys_flag);
		}
#endif /* CONFIG_AVM_PA */
		else {
			err(
			    "incorrect DMA TX channel: %d (0 - 1 is reserved for fast path)",
			    chan_no);
		}
		break;
	default:
		err("unkown DMA interrupt event - %d", status);
	}

	return ret;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

#ifdef CONFIG_AVM_PA
static int ppe_directpath_send(uint32_t off, struct sk_buff *skb,
		int32_t len __attribute__ ((unused)),
		uint32_t flags __attribute__ ((unused))) {

    unsigned long sys_flag;
    uint32_t if_id = PPA_VDEV_ID(off);
    int res = 0;

    //  Careful, no any safety check here.
    //  Parameters must be correct.

    dbg_trace_ppa_data("directpath send if:%d", if_id);
    if ( skb_headroom(skb) < sizeof(struct sw_eg_pkt_header) )
    {
        struct sk_buff *new_skb;

        new_skb = alloc_skb_tx(skb->len + sizeof(struct sw_eg_pkt_header));
        if ( !new_skb )
        {
            dev_kfree_skb_any(skb);
            ppa_virt_rx_devs[ off ]->hw_pkt_try_to_acc_dropped++;
            return -1;
        }
        else
        {
            skb_reserve(new_skb, sizeof(struct sw_eg_pkt_header));
            skb_put(new_skb, skb->len);
            memcpy(new_skb->data, skb->data, skb->len);
            dev_kfree_skb_any(skb);
            skb = new_skb;
        }
    }

    spin_lock_irqsave(&g_directpath_tx_spinlock, sys_flag);
    if (eth_xmit(skb, if_id /* dummy */, DMA_CHAN_NR_DIRECTPATH_CPU_TO_PPE, if_id /* spid */, 0 /* lowest priority */)) {
        if ( ppa_virt_rx_devs[ off ] ){
            ppa_virt_rx_devs[ off ]->hw_pkt_try_to_acc_dropped++;
        }
        res = -1;
    } else {
        res = 0;
    }

    spin_unlock_irqrestore(&g_directpath_tx_spinlock, sys_flag);
    return res;
}

#else/* CONFIG_AVM_PA */

static int ppe_directpath_send(uint32_t off __maybe_unused,
			        struct sk_buff *skb __maybe_unused,
				int32_t len __maybe_unused,
				uint32_t flags __maybe_unused)
{
	BUG();
	return 0;
}
#endif /* CONFIG_AVM_PA */

#endif // defined(DATAPATH_7PORT)

#endif // #ifndef _IFXMIPS_PPA_DATAPATH_COMMON
