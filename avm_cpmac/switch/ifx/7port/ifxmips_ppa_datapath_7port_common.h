#if !defined(IFXMIPS_PPA_DATAPATH_VR9_COMMON)
#define IFXMIPS_PPA_DATAPATH_VR9_COMMON

#include <ifx_dma_core.h>

/*
 *  DMA Kram aus 7 Port Treiber
 */

#define DMA_ALIGNMENT                   128
#define IFX_TX_TIMEOUT                  (10 * HZ)
#define DMA_TX_BURST_LEN                DMA_BURSTL_4DW
#define DMA_RX_BURST_LEN                DMA_BURSTL_4DW
#define ETH_MIN_TX_PACKET_LENGTH        ETH_ZLEN
#define DMA_CHAN_NR_DIRECTPATH_CPU_TO_PPE  3
#define DMA_CHAN_NR_DEFAULT_CPU_TX         2

extern volatile int g_stop_datapath;
extern unsigned int num_registered_eth_dev;
extern int g_dbg_datapath;
extern struct net_device *g_eth_net_dev[MAX_ETH_INF];
extern struct dma_device_info  *g_dma_device_ppe;
DECLARE_PER_CPU(struct softnet_data, g_7port_eth_queue);

extern int stop_7port_dma_gracefully(void);
extern void show_fdma_sdma(void);
extern void dump_dplus(const char *print_header);
extern void avmnet_swi_7port_drop_everything(void);

void enable_fdma_sdma(void);
void disable_fdma_sdma(int min_port, int max_port);
void disable_cpuport_flow_control(void);
void force_mac_links_down(void);
void dump_dma_device( struct dma_device_info *dma_device, char *additional_notes);

void dma_activate_poll(struct dma_device_info *dma_dev __attribute__ ((unused))) ;
void dma_inactivate_poll(struct dma_device_info *dma_dev __attribute__ ((unused))) ;

/*------------------------------------------------------------------------------------------*\
 * header wird benutzt auf dem Weg zum Switch ( wird eingefuegt von CPU bzw. PPE )
\*------------------------------------------------------------------------------------------*/
struct sw_eg_pkt_header {
    //  byte 0
    unsigned int    res1                :5;
    unsigned int    spid                :3;
    //  byte 1
    unsigned int    crc_gen_dis         :1; //  0: enable CRC generation, 1: disable CRC generation
    unsigned int    res2                :4;
    unsigned int    dpid                :3;
    //  byte 2
    unsigned int    port_map_en         :1; //  0: ignore Dest Eth Port Map, 1: use Dest Eth Port Map (field port_map)
    unsigned int    port_map_sel        :1; //  0: field port_map is Dest Port Mask, 1: field port_map is Dest Port Map
    unsigned int    lrn_dis             :1; //  0/1: enable/disable source MAC address learning
    unsigned int    class_en            :1; //  0/1: disable/enable Target Traffic Class (field class)
    unsigned int    class               :4; //  Target Traffic Class
    //  byte 3
    unsigned int    res3                :1;
    unsigned int    port_map            :6;
    unsigned int    dpid_en             :1; //  0/1: disable/enable field dpid
};

/*------------------------------------------------------------------------------------------*\
 * header wird benutzt auf dem Weg zum PPE ( wird eingefuegt vom Switch )
 \*------------------------------------------------------------------------------------------*/
struct pmac_header {
	// Byte 0
	unsigned char b0_resv_7_6 :2; /*!< Reserved bits*/
	unsigned char IPOFF :6; /*!< IP Offset */
	// Byte 1
	unsigned char PORTMAP :8; /*!< Destination Group and Port Map */
	//in PP32 hier gehts los (0)
	// Byte 2
	unsigned char SLPID :3; /*!< Source Logical Port ID */
	unsigned char b2_reserved_4_3 :2; /*!< Reserved bits*/
	unsigned char IS_TAG :1; /*!< Is Tagged */
	unsigned char b2_reserved_1_0 :2; /*!< Reserved bits (evtl mpc / tap)*/
	// Byte 3
	unsigned char b3_reserved_7_4 :4; /*!< (2 bit-qid) + (2bit ?) Reserved bits*/
	unsigned char PPPoES :1; /*!< PPPoE Session Packet */
	unsigned char IPv6 :1; /*!< IPv6 Packet */
	unsigned char IPv4 :1; /*!< IPv4 Packet */
	unsigned char MRR :1; /*!< Mirrored */
	// Byte 4
	unsigned char b4_resv_7_6 :2; /*!< Reserved bits*/
	unsigned char PKT_LEN_HI :6; /* Packet Length High Bits */
	// Byte 5
	unsigned char PKT_LEN_LO :8; /* Packet Length Low Bits */
	// in PP32 hier gehts los (1)
	// Byte 6
	unsigned char b6_resv_7_0 :8; /*!< Reserved bits*/
	// Byte 7
	unsigned char b7_resv_7 :1; /*!< Reserved bits*/
	unsigned char SPPID :3; /*!< Source Physical Port ID */
	unsigned char Eg_Tr_CLASS :4; /*!< Traffic Class */
};

/*------------------------------------------------------------------------------------------*\
 * header wird benutzt auf dem Weg zur CPU ( wird eingefuegt vom PPE ),
 * danach folgt PMAC-Header, dann das Paket
\*------------------------------------------------------------------------------------------*/
struct flag_header {
    //  0 - 3h
    unsigned int    ipv4_rout_vld       :1;
    unsigned int    ipv4_mc_vld         :1;
    unsigned int    proc_type           :1; //  0: routing, 1: bridging
    unsigned int    res1                :1; //  mixed
    unsigned int    tcpudp_err          :1; //  reserved in A4
    unsigned int    tcpudp_chk          :1; //  reserved in A4
    unsigned int    is_udp              :1;
    unsigned int    is_tcp              :1;
    unsigned int    res2                :1;
    unsigned int    ip_inner_offset     :7; //offset from the start of the Ethernet frame to the IP field(if there's more than one IP/IPv6 header, it's inner one)
    unsigned int    is_pppoes           :1; //  2h
    unsigned int    is_inner_ipv6       :1;
    unsigned int    is_inner_ipv4       :1;
    unsigned int    is_vlan             :2; //  0: nil, 1: single tag, 2: double tag, 3: reserved
    unsigned int    rout_index          :11;

    //  4 - 7h
    unsigned int    dest_list           :8;
    unsigned int    src_itf             :3; //  7h
    unsigned int    tcp_rstfin          :1; //  7h
    unsigned int    qid                 :4; //  for fast path, indicate destination priority queue, for CPU path, QID determined by Switch
    unsigned int    temp_dest_list      :8; //  only for firmware use
    unsigned int    src_dir             :1; //  0: LAN, 1: WAN
    unsigned int    acc_done            :1;
    unsigned int    res3                :2;
    unsigned int    is_outer_ipv6       :1; //if normal ipv6 packet, only is_inner_ipv6 is set
    unsigned int    is_outer_ipv4       :1;
    unsigned int    is_tunnel           :2; //0-1 reserved, 2: 6RD, 3: Ds-lite

    // 8 - 11h
    unsigned int    sppid               :3; //switch port id
    unsigned int    pkt_len             :13;//packet length
    unsigned int    pl_byteoff          :8; //bytes between flag header and frame payload
    unsigned int    mpoa_type           :2;
    unsigned int    ip_outer_offset     :6; //offset from the start of the Ethernet frame to the IP field

    // 12 - 15h
    unsigned int    tc                  :4;  // switch traffic class
    unsigned int    avm_is_lan          :1;  // if acc_done is set, avm_is_lan classifies rout->entry
    unsigned int    avm_is_mc           :1;  // if acc_done is set, avm_is_mc classifies rout->entry
    unsigned int    avm_is_coll         :1;  // if acc_done is set, avm_is_coll classifies rout->entry  (entry is collision entry!)
    unsigned int    avm_7port_dst_map   :6;  // only for firmware use
    unsigned int    avm_rout_index      :19; // if acc_done is set, avm_rout_index contains rout->entry ( we might need recalculation, if avm_is_coll is set )
  };

  struct wlan_flag_header {
    unsigned int    res1                :1;
    unsigned int    aal5_raw            :1; //  0: ETH frame with FCS, 1: AAL5 CPCS-SDU
    unsigned int    mpoa_type           :2; //  0: EoA without FCS, 1: EoA with FCS, 2: PPPoA, 3:IPoA
    unsigned int    res2                :1;
    unsigned int    dsl_qid             :3; //  not valid for WLAN
    unsigned int    res3                :5;
    unsigned int    src_itf             :3;
    unsigned int    payload_overlap     :16;//  This flag header is 16 bits only. This field overlapps with payload, and is for 32-bit align purpose.
  };


typedef struct fw_dbg {
	char *cmd;
	void (*pfunc)(char **tokens, unsigned int token_num);
} fw_dbg_t;



#if defined(DEBUG_DUMP_PMAC_HEADER) && DEBUG_DUMP_PMAC_HEADER
	void dump_pmac_header(struct pmac_header *header, const char *title);
#else
	#define dump_pmac_header(a, b)
#endif

#if defined(DEBUG_DUMP_FLAG_HEADER) && DEBUG_DUMP_FLAG_HEADER
void dump_flag_header(int, struct flag_header *, const char *,  int);
#else
#define dump_flag_header(a, b, c, d)
#endif

#define FWCODE_ROUTING_ACC_D2                   0x02
#define FWCODE_BRIDGING_ACC_D3                  0x03
#define FWCODE_ROUTING_BRIDGING_ACC_D4          0x04
#define FWCODE_ROUTING_BRIDGING_ACC_A4          0x14
#define FWCODE_ROUTING_ACC_D5                   0x05
#define FWCODE_ROUTING_ACC_A5                   0x15

void reinit_7port_common_eth(void);

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
#if defined (CONFIG_AVM_SCATTER_GATHER)
inline static int send_skb_scattered_first(struct sk_buff *skb, struct dma_device_info* dma_dev, int ch) {
        int write_len;
        int dma_write_res;
        write_len = skb_headlen(skb);
        dma_write_res = dma_device_write(dma_dev,
                        skb->data,
                        write_len | IFX_DMA_NOT_EOP | IFX_DMA_LOCK_BY_UPPER_LAYER ,  /* not end of packet */
                        NULL,
                        ch) ;
        return dma_write_res != write_len;

}
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 2, 0)
#define skb_frag_address(_frag) \
	(page_address(shinfo->frags[i].page) + shinfo->frags[i].page_offset)
#endif

inline static int send_skb_scattered_successors(struct sk_buff *skb, struct dma_device_info* dma_dev, int ch) {

    struct skb_shared_info *shinfo = skb_shinfo(skb);
    int i;

    for(i = 0 ; i < shinfo->nr_frags ; i++) {
        int len = shinfo->frags[i].size;
        int len_sop_eop_lock = len;
        struct sk_buff *store_skb_in_dma;

        unsigned char *source;
        len_sop_eop_lock |= IFX_DMA_NOT_SOP ;  /* this is not start_of_package */
        len_sop_eop_lock |= IFX_DMA_LOCK_BY_UPPER_LAYER ;
        if( i < shinfo->nr_frags - 1 ) {
            store_skb_in_dma = NULL;
            len_sop_eop_lock |= IFX_DMA_NOT_EOP;  /* this is not end_of_package */
        } else {
            store_skb_in_dma = skb;
        }
	source = skb_frag_address(&shinfo->frags[i]);

        dbg_trace_ppa_data("send fragment %d (%d bytes) [total fragments = start_fragment + %d]\n",  i, len, shinfo->nr_frags);

        if (dma_device_write(
                    dma_dev,
                    source,
                    len_sop_eop_lock,
                    store_skb_in_dma, ch) != len) {
            return 1; /* dma didn't consume len bytes, this must never happen! */
        }
    }
    return 0;
}

#endif /*--- #if defined (CONFIG_AVM_SCATTER_GATHER) ---*/

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/


#endif /* --- IFXMIPS_PPA_DATAPATH_VR9_COMMON --- */
