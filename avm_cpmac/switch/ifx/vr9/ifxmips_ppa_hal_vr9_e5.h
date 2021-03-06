/******************************************************************************
**
** FILE NAME    : ifxmips_ppa_hal_vr9_e5.h
** PROJECT      : UEIP
** MODULES      : MII0/1 Acceleration Package (VR9 PPA E5)
**
** DATE         : 19 OCT 2009
** AUTHOR       : Xu Liang
** DESCRIPTION  : PTM (VDSL) + MII0/1 Driver with Acceleration Firmware (E5)
** COPYRIGHT    :              Copyright (c) 2009
**                          Lantiq Deutschland GmbH
**                   Am Campeon 3; 85579 Neubiberg, Germany
**
**   For licensing information, see the file 'LICENSE' in the root folder of
**   this software module.
**
** HISTORY
** $Date        $Author         $Comment
** 19 OCT 2009  Xu Liang        Initiate Version
*******************************************************************************/

#ifndef IFXMIPS_PPA_HAL_VR9_E5_H
#define IFXMIPS_PPA_HAL_VR9_E5_H

#include <common_routines.h>
#include "../../../../avm_cpmac/switch/ifx/vr9/swi_vr9_reg.h"
/*
 *  Platform and Mode
 */
#define PLATFM_VR9                              1
#define MODE_VR9_E5                             1
#define DEF_PPA_MODE_E5


/*
 *  hash calculation
 */

#define BRIDGING_SESSION_LIST_HASH_BIT_LENGTH   8
#define BRIDGING_SESSION_LIST_HASH_MASK         ((1 << BRIDGING_SESSION_LIST_HASH_BIT_LENGTH) - 1)
#define BRIDGING_SESSION_LIST_HASH_TABLE_SIZE   (1 << BRIDGING_SESSION_LIST_HASH_BIT_LENGTH)
#define BRIDGING_SESSION_LIST_HASH_VALUE(x)     ((x) & BRIDGING_SESSION_LIST_HASH_MASK)
/*
 *  Firmware Constant
 */

#define MAX_IPV6_IP_ENTRIES                     (MAX_IPV6_IP_ENTRIES_PER_BLOCK * MAX_IPV6_IP_ENTRIES_BLOCK)
#define MAX_IPV6_IP_ENTRIES_PER_BLOCK           64
#define MAX_IPV6_IP_ENTRIES_BLOCK               2
#define MAX_ROUTING_ENTRIES                     (MAX_WAN_ROUTING_ENTRIES + MAX_LAN_ROUTING_ENTRIES)
#define MAX_COLLISION_ROUTING_ENTRIES           64
#define MAX_HASH_BLOCK                          8
#define MAX_ROUTING_ENTRIES_PER_HASH_BLOCK      16
#define MAX_WAN_ROUTING_ENTRIES                 (MAX_ROUTING_ENTRIES_PER_HASH_BLOCK * MAX_HASH_BLOCK + MAX_COLLISION_ROUTING_ENTRIES)
#define MAX_LAN_ROUTING_ENTRIES                 (MAX_ROUTING_ENTRIES_PER_HASH_BLOCK * MAX_HASH_BLOCK + MAX_COLLISION_ROUTING_ENTRIES)
#define MAX_WAN_MC_ENTRIES                      32

#if defined(CAP_WAP_CONFIG) && CAP_WAP_CONFIG
#define MAX_CAPWAP_ENTRIES                      5 

#define  ETHER_TYPE                             0x0800
#define  IP_VERSION                             0x4
#define  IP_HEADER_LEN                          0x5
#define  IP_TOTAL_LEN                           0x0
#define  IP_IDENTIFIER                          0x0
#define  IP_FLAG                                0x0
#define  IP_FRAG_OFF                            0x0
#define  IP_PROTO_UDP                           0x11 
#define  UDP_TOTAL_LENGTH                       0x0
#define  CAPWAP_PREAMBLE                        0x0 
#define  CAPWAP_HDLEN                           0x2 
#define  CAPWAP_F_FLAG                          0x0 
#define  CAPWAP_L_FLAG                          0x0
#define  CAPWAP_W_FLAG                          0x0
#define  CAPWAP_M_FLAG                          0x0
#define  CAPWAP_K_FLAG                          0x0
#define  CAPWAP_FLAGS                           0x0
#define  CAPWAP_FRAG_ID                         0x0
#define  CAPWAP_FRAG_OFF                        0x0

#endif


#define MAX_PPPOE_ENTRIES                       8
#define MAX_MTU_ENTRIES                         8
#define MAX_MAC_ENTRIES                         16
#define MAX_OUTER_VLAN_ENTRIES                  32
#define MAX_CLASSIFICATION_ENTRIES              (64 - 1)
#define MAX_6RD_TUNNEL_ENTRIES                  4
#define MAX_DSLITE_TUNNEL_ENTRIES               4
#define MAX_PTM_QOS_PORT_NUM                    4
#define MAX_WAN_QOS_QUEUES                      8

/*
 *  DWORD-Length of Memory Blocks
 */
#define PP32_DEBUG_REG_DWLEN                    0x0030
#define CDM_CODE_MEMORYn_DWLEN(n)               ((n) == 0 ? 0x1000 : 0x0800)
#define CDM_DATA_MEMORY_DWLEN                   CDM_CODE_MEMORYn_DWLEN(1)
#define SB_RAM0_DWLEN                           0x1000
#define SB_RAM1_DWLEN                           0x1000
#define SB_RAM2_DWLEN                           0x1000
#define SB_RAM3_DWLEN                           0x1000
#define SB_RAM6_DWLEN                           0x8000
#define QSB_CONF_REG_DWLEN                      0x0100

/*
 *  Host-PPE Communication Data Address Mapping
 */

#define SB_BUFFER_BOND(idx, __sb_addr)       ((volatile unsigned int *)( \
    (__sb_addr <= 0x1FFF)                            ? PPE_REG_ADDR_BOND(idx, __sb_addr) :          \
    ((__sb_addr >= 0x2000) && (__sb_addr <= 0x2FFF)) ? SB_RAM0_ADDR_BOND(idx, __sb_addr - 0x2000) : \
    ((__sb_addr >= 0x3000) && (__sb_addr <= 0x3FFF)) ? SB_RAM1_ADDR_BOND(idx, __sb_addr - 0x3000) : \
    ((__sb_addr >= 0x4000) && (__sb_addr <= 0x4FFF)) ? SB_RAM2_ADDR_BOND(idx, __sb_addr - 0x4000) : \
    ((__sb_addr >= 0x5000) && (__sb_addr <= 0x5FFF)) ? SB_RAM3_ADDR_BOND(idx, __sb_addr - 0x5000) : \
    ((__sb_addr >= 0x6000) && (__sb_addr <= 0x6FFF)) ? 0: \
    ((__sb_addr >= 0x7000) && (__sb_addr <= 0x7FFF)) ? PPE_REG_ADDR_BOND(idx, __sb_addr - 0x7000) : \
    ((__sb_addr >= 0x8000) && (__sb_addr <= 0xFFFF)) ? SB_RAM6_ADDR_BOND(idx, __sb_addr - 0x8000) : \
    (__sb_addr > 0xFFFF)                             ? 0: \
        PPE_REG_ADDR_BOND(idx, __sb_addr) )) //this should be never used, it's just for our lovely klocwork

#if 0
#define SB_BUFFER_BOND(idx, __sb_addr)       ((volatile unsigned int *)(((__sb_addr) <= 0x1FFF) ? PPE_REG_ADDR_BOND(idx, (__sb_addr)) :          \
                                                                   (((__sb_addr) >= 0x2000) && ((__sb_addr) <= 0x2FFF)) ? SB_RAM0_ADDR_BOND(idx, (__sb_addr) - 0x2000) : \
                                                                   (((__sb_addr) >= 0x3000) && ((__sb_addr) <= 0x3FFF)) ? SB_RAM1_ADDR_BOND(idx, (__sb_addr) - 0x3000) : \
                                                                   (((__sb_addr) >= 0x4000) && ((__sb_addr) <= 0x4FFF)) ? SB_RAM2_ADDR_BOND(idx, (__sb_addr) - 0x4000) : \
                                                                   (((__sb_addr) >= 0x5000) && ((__sb_addr) <= 0x5FFF)) ? SB_RAM3_ADDR_BOND(idx, (__sb_addr) - 0x5000) : \
                                                                   (((__sb_addr) >= 0x7000) && ((__sb_addr) <= 0x7FFF)) ? PPE_REG_ADDR_BOND(idx, (__sb_addr) - 0x7000) : \
                                                                   (((__sb_addr) >= 0x8000) && ((__sb_addr) <= 0xFFFF)) ? SB_RAM6_ADDR_BOND(idx, (__sb_addr) - 0x8000) : \
                                                                0))
#endif

#define FW_VER_ID                               ((volatile struct fw_ver_id *)              SB_BUFFER_BOND(2, 0x2000))
#define FW_VER_FEATURE                                                                        SB_BUFFER_BOND(2, 0x2001)

#define PS_MC_GENCFG3                           ((volatile struct ps_mc_cfg *)              SB_BUFFER_BOND(2, 0x2003))   //  power save and multicast gen config

#define CFG_FEATURES                            SB_BUFFER_BOND(2, 0x2003)    //  reserved for future

/*
 *  Bonding Registers
 */
#define FRAG_READY_NOTICE                       0x2002
#define BM_BS_MBOX_IGU6_ISRS                    SB_BUFFER_BOND(2, 0x2004)    //  on-chip: 0x7220, off-chip: FPI address of 0x7220 on SM
#define BM_BM_MBOX_IGU6_ISRS                    SB_BUFFER_BOND(2, 0x2005)    //  0x7220
// LINK_UP_NOTIFY_ADDR config:
//  on-chip/off-chip BM: 0x721C (__MBOX_IGU5_ISRS), off-chip SM: FPI address of 0x721C on BM
#define LINK_UP_NOTIFY_ADDR                     SB_BUFFER_BOND(2, 0x2006)
// LINK_DOWN_NOTIFY_ADDR config:
//  on-chip/off-chip BM: 0x721D (__MBOX_IGU5_ISRC), off-chip SM: FPI address of 0x721D on BM
#define LINK_DOWN_NOTIFY_ADDR                   SB_BUFFER_BOND(2, 0x2007)
#define BOND_CONF                               ((volatile struct bond_conf *)              SB_BUFFER_BOND(2, 0x2008))
#define US_BG_QMAP                              ((volatile struct bg_map *)                 SB_BUFFER_BOND(2, 0x2009))
#define US_BG_GMAP                              ((volatile struct bg_map *)                 SB_BUFFER_BOND(2, 0x200A))
#define DS_BG_GMAP                              ((volatile struct bg_map *)                 SB_BUFFER_BOND(2, 0x200B))
#define TIME_STAMP                              SB_BUFFER_BOND(2, 0x200C)    //  It starts after PP32 processor core is started. The unit is TimeTick (in TX_QOS_CFG). It is free running and wraps at the maximum value.
#define E1_DES_PDMA_BAR                         SB_BUFFER_BOND(2, 0x200D)
#define B1_DES_PDMA_BAR                         SB_BUFFER_BOND(2, 0x200E)
#define DATAPTR_PDMA_BAR                        SB_BUFFER_BOND(2, 0x200F)
#define US_FRAG_READY_NOTICE_ADDR               SB_BUFFER_BOND(2, 0x2022)
#define E1_FRAG_TX_DESBA                        ((volatile struct e1_frag_descba *)         SB_BUFFER_BOND(2, 0x202A))
#define E1_FRAG_RX_DESBA                        ((volatile struct e1_frag_descba *)         SB_BUFFER_BOND(2, 0x202B))
#define B1_DS_LL_CTXT(i)                        ((volatile struct b1_ds_ll_ctxt *)          SB_BUFFER_BOND(1, 0x26DC + (i) * 4))     /*  i < 9   */
/*
 *  Off-chip Bonding Registers
 */
#define US_CDMA_RX_DES_PDMA_BAR                 SB_BUFFER_BOND(2, 0x2020)
#define US_CDMA_RX_DES_BASE                     SB_BUFFER_BOND(2, 0x2021)
//  following two MIB are defined in "vr9_bonding_addr_def.inc" and "vr9_bonding_fw_data_structure.h"
//#define DS_BOND_GIF_MIB(i)                      ((volatile struct ds_bond_gif_mib *)        SB_BUFFER_BOND(2, 0x2E80 + (i) * 16))    /*  i < 8   */
//#define DS_BG_MIB(i)                            ((volatile struct ds_bg_mib *)              SB_BUFFER_BOND(2, 0x2F20 + (i) * 16))    /*  i < 4   */
#define FP_QID_BASE_PDMA_BAR                    SB_BUFFER_BOND(2, 0x3F11)

/*
 *  Bonding Debug Registers
 */
#define __READ_NUM                              SB_BUFFER_BOND(2, 0x5190)
#define __READ_MAX_CYCLES                       SB_BUFFER_BOND(2, 0x5191)
#define __READ_MIN_CYCLES                       SB_BUFFER_BOND(2, 0x5192)
#define __TOTAL_READ_CYCLES_HI                  SB_BUFFER_BOND(2, 0x5194)
#define __TOTAL_READ_CYCLES_LO                  SB_BUFFER_BOND(2, 0x5195)
#define __WRITE_NUM                             SB_BUFFER_BOND(2, 0x5198)
#define __WRITE_MAX_CYCLES                      SB_BUFFER_BOND(2, 0x5199)
#define __WRITE_MIN_CYCLES                      SB_BUFFER_BOND(2, 0x519A)
#define __TOTAL_WRITE_CYCLES_HI                 SB_BUFFER_BOND(2, 0x519C)
#define __TOTAL_WRITE_CYCLES_LO                 SB_BUFFER_BOND(2, 0x519D)

/*
 *  Generic Registers
 */
#define CFG_STD_DATA_LEN                        ((volatile struct cfg_std_data_len *)       SB_BUFFER_BOND(2, 0x2011))
#define TX_QOS_CFG                              ((volatile struct tx_qos_cfg *)             SB_BUFFER_BOND(1, 0x2012))
#define TX_QOS_CFG_DYNAMIC                      ((volatile struct tx_qos_cfg *)             SB_BUFFER_BOND(2, 0x2012))
#define EG_BWCTRL_CFG                           ((volatile struct eg_bwctrl_cfg *)          SB_BUFFER_BOND(2, 0x2013))
#define PSAVE_CFG                               ((volatile struct psave_cfg *)              SB_BUFFER_BOND(2, 0x2014))
#define CFG_WAN_PORTMAP                         SB_BUFFER_BOND(0, 0x201A)
#define CFG_MIXED_PORTMAP                       SB_BUFFER_BOND(0, 0x201B)
#define WRX_DMACH_ON                            SB_BUFFER_BOND(2, 0x2015)    //  reserved in E5
#define WTX_DMACH_ON                            SB_BUFFER_BOND(2, 0x2016)    //  reserved in E5
#define UTP_CFG                                 SB_BUFFER_BOND(2, 0x2018)    //  bit 0~3 - 0x0F: in showtime, 0x00: not in showtime
#define GPIO_ADDR                               SB_BUFFER_BOND(2, 0x2019)
#define TX_QOS_WFQ_RELOAD_MAP                   SB_BUFFER_BOND(1, 0x3F13)
#define GPIO_MODE                               ((volatile struct gpio_mode *)              SB_BUFFER_BOND(2, 0x201C))
#define GPIO_WM_CFG                             ((volatile struct gpio_wm_cfg *)            SB_BUFFER_BOND(2, 0x201D))
#define TEST_MODE                               ((volatile struct test_mode *)              SB_BUFFER_BOND(2, 0x201F))
#define PSEUDO_IPv4_BASE_ADDR                   SB_BUFFER_BOND(0, 0x2023)
#define ETH_PORTS_CFG                           ((volatile struct eth_ports_cfg *)          SB_BUFFER_BOND(0, 0x2024))
#define LAN_ROUT_TBL_CFG                        ((volatile struct rout_tbl_cfg *)           SB_BUFFER_BOND(0, 0x2026))
#define WAN_ROUT_TBL_CFG                        ((volatile struct rout_tbl_cfg *)           SB_BUFFER_BOND(0, 0x2027))
#define GEN_MODE_CFG1                           ((volatile struct gen_mode_cfg1 *)          SB_BUFFER_BOND(0, 0x2028))
#define GEN_MODE_CFG                            GEN_MODE_CFG1
#define GEN_MODE_CFG2                           ((volatile struct gen_mode_cfg2 *)          SB_BUFFER_BOND(0, 0x2029))
#define KEY_SEL_n(i)                            SB_BUFFER_BOND(0, 0x202C + (i))

//  PTM config
#define RX_BC_CFG(i)                            ((volatile struct rx_bc_cfg *)              SB_BUFFER_BOND(2, 0x3E80 + (i) * 0x20))  /*  i < 2   */
#define TX_BC_CFG(i)                            ((volatile struct tx_bc_cfg *)              SB_BUFFER_BOND(2, 0x3EC0 + (i) * 0x20))  /*  i < 2   */
#define RX_GAMMA_ITF_CFG(i)                     ((volatile struct rx_gamma_itf_cfg *)       SB_BUFFER_BOND(2, 0x3D80 + (i) * 0x20))  /*  i < 4   */
#define TX_GAMMA_ITF_CFG(i)                     ((volatile struct tx_gamma_itf_cfg *)       SB_BUFFER_BOND(2, 0x3E00 + (i) * 0x20))  /*  i < 4   */
#define TX_QUEUE_CFG(i)                         WTX_EG_Q_PORT_SHAPING_CFG(i)    //  i < 9
#define TX_CTRL_K_TABLE(i)                      SB_BUFFER_BOND(2, 0x47F0 + (i))      //  i < 16
//  following MIB for debugging purpose
#define RECEIVE_NON_IDLE_CELL_CNT(i)            SB_BUFFER_BOND(2, 0x34A0 + (i))
#define RECEIVE_IDLE_CELL_CNT(i)                SB_BUFFER_BOND(2, 0x34A2 + (i))
#define TRANSMIT_CELL_CNT(i)                    SB_BUFFER_BOND(2, 0x34A4 + (i))
#define FP_RECEIVE_PKT_CNT                      SB_BUFFER_BOND(1, 0x34A6)            //  off-chip bonding
#define WTX_PTM_EG_Q_PORT_SHAPING_CFG(i)        ((volatile struct wtx_eg_q_shaping_cfg *)   SB_BUFFER_BOND(1, 0x26A4 + (i) * 4)) /*  i < 4   */

#define WTX_QOS_Q_DESC_CFG(i)                   ((volatile struct wtx_qos_q_desc_cfg *)     SB_BUFFER_BOND(1, 0x2FF0 + (i) * 2)) /*  i < 8   */
#define WTX_EG_Q_PORT_SHAPING_CFG(i)            ((volatile struct wtx_eg_q_shaping_cfg *)   SB_BUFFER_BOND(1, 0x2680 + (i) * 4)) /*  i < 1   */

#define WTX_EG_Q_SHAPING_CFG(i)                 ((volatile struct wtx_eg_q_shaping_cfg *)   SB_BUFFER_BOND(1, 0x2684 + (i) * 4)) /*  i < 8   */

#define WAN_RX_MIB_TABLE(i)                     ((volatile struct wan_rx_mib_table *)       SB_BUFFER_BOND(2, 0x5B00 + (i) * 8)) /*  i < 4   */
#define WAN_TX_MIB_TABLE(i)                     ((volatile struct wan_tx_mib_table *)       SB_BUFFER_BOND(1, 0x5B20 + (i) * 8)) /*  i < 8   */

#define DROPPED_PAUSE_FRAME_COUNTER(i)          SB_BUFFER_BOND(0, 0x2FE0 + (i) * 2)      /*  i < 8   */
#define ETH_WAN_TX_MIB_TABLE(i)                 WAN_TX_MIB_TABLE(i)

#define ITF_MIB_TBL(i)                          ((volatile struct itf_mib *)                SB_BUFFER_BOND(0, 0x2030 + (i) * 16))/*  i < 8   */

#define PPPOE_CFG_TBL(i)                        SB_BUFFER_BOND(0, 0x20B0 + (i))          /*  i < 8   */
#define MTU_CFG_TBL(i)                          SB_BUFFER_BOND(0, 0x20B8 + (i))          /*  i < 8   */
#define ROUT_MAC_CFG_TBL(i)                     SB_BUFFER_BOND(0, 0x20C0 + (i) * 2)      /*  i < 16  */
#define TUNNEL_6RD_TBL(i)                       SB_BUFFER_BOND(0, 0x26C8  + (i) * 5)     /*  i < 4    */
#define TUNNEL_DSLITE_TBL(i)                    SB_BUFFER_BOND(0, 0x2FA8 + (i) * 10)     /*  i < 4    */
#define TUNNEL_MAX_ID                           SB_BUFFER_BOND(0, 0x2025)

#define IPv6_IP_IDX_TBL(x, i)                   SB_BUFFER_BOND(0, ((x == 0) ? 0x27C0 : 0x3700) + (i) * 4)    /*  i < 64 */

#define  CFG_CLASS2QID_MAP(i)                   SB_BUFFER_BOND(0, 0x47E4 + (i))          /*  i < 4   */
#define  CFG_QID2CLASS_MAP(i)                   SB_BUFFER_BOND(0, 0x47E8 + (i))          /*  i < 4   */

#define POWERSAVING_PROFILE(i)                  ((volatile struct powersaving_profile *)    SB_BUFFER_BOND(2, 0x5B60 + (i) * 4)) /*  i < 4, TIMER0 (0), DPLUS_IN (1), SFSM (2), RUNNING (3)    */

//-------------------------------------
// Hit Status
//-------------------------------------
#define __IPV4_WAN_HIT_STATUS_HASH_TABLE_BASE       0x20E0
#define __IPV4_WAN_HIT_STATUS_COLLISION_TABLE_BASE  0x2FA0
#define __IPV4_LAN_HIT_STATUS_HASH_TABLE_BASE       0x20F0
#define __IPV4_LAN_HIT_STATUS_COLLISION_TABLE_BASE  0x2FA2
#define __IPV4_WAN_HIT_STATUS_MC_TABLE_BASE         0x2FA4
#define __IPV6_HIT_STATUS_TABLE_BASE                0x2FA6  //  reserved

#define ROUT_LAN_HASH_HIT_STAT_TBL(i)               SB_BUFFER_BOND(0, __IPV4_LAN_HIT_STATUS_HASH_TABLE_BASE + (i))
#define ROUT_LAN_COLL_HIT_STAT_TBL(i)               SB_BUFFER_BOND(0, __IPV4_LAN_HIT_STATUS_COLLISION_TABLE_BASE + (i))
#define ROUT_WAN_HASH_HIT_STAT_TBL(i)               SB_BUFFER_BOND(0, __IPV4_WAN_HIT_STATUS_HASH_TABLE_BASE + (i))
#define ROUT_WAN_COLL_HIT_STAT_TBL(i)               SB_BUFFER_BOND(0, __IPV4_WAN_HIT_STATUS_COLLISION_TABLE_BASE + (i))
#define ROUT_WAN_MC_HIT_STAT_TBL(i)                 SB_BUFFER_BOND(0, __IPV4_WAN_HIT_STATUS_MC_TABLE_BASE + (i))

//-------------------------------------
// Compare and Action table
//-------------------------------------
#define __IPV4_LAN_HASH_ROUT_FWDC_TABLE_BASE        0x3000
#define __IPV4_LAN_HASH_ROUT_FWDA_TABLE_BASE        0x3180

#define __IPV4_LAN_COLLISION_ROUT_FWDC_TABLE_BASE   0x2700
#define __IPV4_LAN_COLLISION_ROUT_FWDA_TABLE_BASE   0x2B80

#define __IPV4_WAN_HASH_ROUT_FWDC_TABLE_BASE        0x2100
#define __IPV4_WAN_HASH_ROUT_FWDA_TABLE_BASE        0x2280

#define __IPV4_WAN_COLLISION_ROUT_FWDC_TABLE_BASE   0x3600
#define __IPV4_WAN_COLLISION_ROUT_FWDA_TABLE_BASE   0x2D00

#define __IPV4_ROUT_MULTICAST_FWDC_TABLE_BASE       0x36C0
#define __IPV4_ROUT_MULTICAST_FWDA_TABLE_BASE       0x2B00

#if defined(RTP_SAMPLING_ENABLE) && RTP_SAMPLING_ENABLE
#define __MULTICAST_RTP_MIB_BASE                    0x2B60
#endif

#if defined(CAP_WAP_CONFIG) && CAP_WAP_CONFIG
#define __CAPWAP_CONFIG_TABLE_BASE                  0x3800
#endif


#define ROUT_LAN_HASH_CMP_TBL(i)                ((volatile struct rout_forward_compare_tbl *)   SB_BUFFER_BOND(0, __IPV4_LAN_HASH_ROUT_FWDC_TABLE_BASE + (i) * 3))
#define ROUT_LAN_HASH_ACT_TBL(i)                ((volatile struct rout_forward_action_tbl *)    SB_BUFFER_BOND(0, __IPV4_LAN_HASH_ROUT_FWDA_TABLE_BASE + (i) * 6))

#define ROUT_LAN_COLL_CMP_TBL(i)                ((volatile struct rout_forward_compare_tbl *)   SB_BUFFER_BOND(0, __IPV4_LAN_COLLISION_ROUT_FWDC_TABLE_BASE + (i) * 3))
#define ROUT_LAN_COLL_ACT_TBL(i)                ((volatile struct rout_forward_action_tbl *)    SB_BUFFER_BOND(0, __IPV4_LAN_COLLISION_ROUT_FWDA_TABLE_BASE + (i) * 6))

#define ROUT_WAN_HASH_CMP_TBL(i)                ((volatile struct rout_forward_compare_tbl *)   SB_BUFFER_BOND(0, __IPV4_WAN_HASH_ROUT_FWDC_TABLE_BASE + (i) * 3))
#define ROUT_WAN_HASH_ACT_TBL(i)                ((volatile struct rout_forward_action_tbl *)    SB_BUFFER_BOND(0, __IPV4_WAN_HASH_ROUT_FWDA_TABLE_BASE + (i) * 6))

#define ROUT_WAN_COLL_CMP_TBL(i)                ((volatile struct rout_forward_compare_tbl *)   SB_BUFFER_BOND(0, __IPV4_WAN_COLLISION_ROUT_FWDC_TABLE_BASE + (i) * 3))
#define ROUT_WAN_COLL_ACT_TBL(i)                ((volatile struct rout_forward_action_tbl *)    SB_BUFFER_BOND(0, __IPV4_WAN_COLLISION_ROUT_FWDA_TABLE_BASE + (i) * 6))

#define ROUT_WAN_MC_CMP_TBL(i)                  ((volatile struct wan_rout_multicast_cmp_tbl *) SB_BUFFER_BOND(0, __IPV4_ROUT_MULTICAST_FWDC_TABLE_BASE + (i) * 2))
#define ROUT_WAN_MC_ACT_TBL(i)                  ((volatile struct wan_rout_multicast_act_tbl *) SB_BUFFER_BOND(0, __IPV4_ROUT_MULTICAST_FWDA_TABLE_BASE + (i) * 2))


#if defined(CAP_WAP_CONFIG) && CAP_WAP_CONFIG

#define CAPWAP_CONFIG_TBL(i)                  ((volatile struct capwap_config_tbl *) SB_BUFFER_BOND(0,__CAPWAP_CONFIG_TABLE_BASE + (i) * 18))

#define CAPWAP_CONFIG                         SB_BUFFER_BOND(0,0x201F)

#endif

#if defined(RTP_SAMPLING_ENABLE) && RTP_SAMPLING_ENABLE

#define MULTICAST_RTP_MIB_TBL(i)                  ((volatile struct rtp_sampling_cnt *) SB_BUFFER_BOND(0,__MULTICAST_RTP_MIB_BASE + (i)))

#endif

#define OUTER_VLAN_TBL(i)                       SB_BUFFER_BOND(0, 0x2F80 + (i))          /*  i < 32  */
#define ROUT_WAN_MC_CNT(i)                      SB_BUFFER_BOND(0, 0x2B40 + (i))          /*  i < 32  */

#define __CLASSIFICATION_CMP_TBL_BASE           0x28C0  //  64 * 4 dwords
#define __CLASSIFICATION_MSK_TBL_BASE           0x29C0  //  64 * 4 dwords
#define __CLASSIFICATION_ACT_TBL_BASE           0x2AC0  //  64 * 1 dwords

#define CLASSIFICATION_CMP_TBL(i)               SB_BUFFER_BOND(0, __CLASSIFICATION_CMP_TBL_BASE + (i) * 4)   //  i < 64
#define CLASSIFICATION_MSK_TBL(i)               SB_BUFFER_BOND(0, __CLASSIFICATION_MSK_TBL_BASE + (i) * 4)   //  i < 64
#define CLASSIFICATION_ACT_TBL(i)               ((volatile struct classification_act_tbl *)SB_BUFFER_BOND(0, __CLASSIFICATION_ACT_TBL_BASE + (i)))   //  i < 64

/*
 *  Qid configuration (How qid copied from dplus slave to CPU via dma)
 *  0   -   qid configured by HW
 *  1   -   wait until previous packet is sent out before processing new one
 *  2   -   qid configured by FW
*/

#define __DPLUS_QID_CONF_PTR                 SB_BUFFER_BOND(2, 0x3F10)

/*
 *  destlist
 */
#define IFX_PPA_DEST_LIST_ETH0                          0x0001
#define IFX_PPA_DEST_LIST_ETH1                          0x0002
#define IFX_PPA_DEST_LIST_CPU0                          0x0004
//#define IFX_PPA_DEST_LIST_EXT_INT1                      0x0008
//#define IFX_PPA_DEST_LIST_EXT_INT2                      0x0010
//#define IFX_PPA_DEST_LIST_EXT_INT3                      0x0020
//#define IFX_PPA_DEST_LIST_EXT_INT4                      0x0040
//#define IFX_PPA_DEST_LIST_EXT_INT5                      0x0080
#define IFX_PPA_DEST_LIST_ATM                           0x080

/*
  * SWITCH PORT DEFINITION
  */
#define IFX_PPA_PORT_ETH0                   0
#define IFX_PPA_PORT_ETH1                   1
#define IFX_PPA_PORT_CPU                    3
#define IFX_PPA_PORT_EXT1                   4
#define IFX_PPA_PORT_EXT2                   5
#define IFX_PPA_PORT_DSL0                   6
#define IFX_PPA_PORT_DSL1                   7
#define IFX_PPA_PORT_PTM                    7
/*
 *  Clock Generation Unit Registers
 */
#define VR9_CGU                                 (KSEG1 | 0x1F103000)
#define VR9_CGU_CLKFSR                          ((volatile unsigned int *)(VR9_CGU + 0x0010))

/*  Helper Macro
 */
#define BITSIZEOF_UINT32                        (sizeof(uint32_t) * 8)
#define BITSIZEOF_UINT16                        (sizeof(uint16_t) * 8)

#if defined(CAP_WAP_CONFIG) && CAP_WAP_CONFIG
#define BITSIZEOF_UINT8                         (sizeof(uint8_t) * 8)
#endif

#define MAX_BRIDGING_ENTRIES                    2048

/*
 * Mac table manipulation action
 */
enum{
    MAC_TABLE_ENTRY_ADD = 1,
    MAC_TABLE_ENTRY_REMOVE,
};

/*
 *  WAN QoS MODE List
 */
enum{
   PPA_WAN_QOS_ETH0 = 0,
   PPA_WAN_QOS_ETH1 = 1,
   PPA_WAN_QOS_PTM0 = 7,
};



/*
 * ####################################
 *              Data Type
 * ####################################
 */

/*
 *  Host-PPE Communication Data Structure
 */
#if defined(__BIG_ENDIAN)

  struct fw_ver_id {//@2000
    //DWORD 0
    unsigned int    family              :4;
    unsigned int    package             :4;
    unsigned int    major               :8;
    unsigned int    middle              :8;
    unsigned int    minor               :8;

    //DWORD 1
    unsigned int    features            :32;
  };

  struct ps_mc_cfg{//@2003  powersave & multicast config
    unsigned int    time_tick           :16;    //  max number of PP32 cycles, the PP32 can sleep
#if defined(MIB_MODE_ENABLE) && MIB_MODE_ENABLE
    unsigned int    res1                :11;
    unsigned int    session_mib_unit    :1;
#else
    unsigned int    res1                :12;
#endif
    unsigned int    class_en            :1;     //  switch class enable
    unsigned int    ssc_mode            :1;     //  source specific comparison (multicast)
    unsigned int    asc_mode            :1;     //  any source comparison (multicast)
    unsigned int    psave_en            :1;     //  enable sleep mode for PPE1 (A5/D5v2) or PPE0 (D5v1)
  };

  struct bond_conf {
    unsigned int    max_frag_size       :16;    //  Maximum fragment size, it configures the maximum fragment size.
                                                //      L2 Trunking Mode - 1568
                                                //      PTM Bonding Mode - 512
    unsigned int    polling_ctrl_cnt    :8;

    unsigned int    dplus_fp_fcs_en     :1;     //  set to 1 for bonding config, 0 for non-bonding config
    unsigned int    bg_num              :3;     //  Bonding Group Number, it configures the maximum number of bonding groups.
    unsigned int    bond_mode           :1;     //  Bonding Mode, 0 - L1 The system is in PTM-TC bonding mode. The packet is fragmented and 16-bit fragment header is added.
                                                //                1 - L2 The system is in L2 trunking mode. The packet is not fragmented and fragment header  is not added.
    unsigned int    e1_bond_en          :1;     //  E1 Bonding Function Enable (1) / Disable (0)
    unsigned int    d5_acc_dis          :1;     //  D5 Acceleration Module Enable (0) / Disable (1)
    unsigned int    d5_b1_en            :1;     //  D5 Bonding Module Enable (1) / Disable (0)
  };

  struct bg_map {
    unsigned char   map3;                       //  US_BG_QMAP: Define the priority channel members of Bonding Group X.
    unsigned char   map2;                       //  US_BG_GMAP/DS_BG_GMAP: Define the Gamma Interface members of Bonding Group X.
    unsigned char   map1;
    unsigned char   map0;
  };

  struct cfg_std_data_len {
    unsigned int res1                   :14;
    unsigned int byte_off               :2;     //  byte offset in RX DMA channel
    unsigned int data_len               :16;    //  data length for standard size packet buffer
  };

  struct tx_qos_cfg {
    unsigned int time_tick              :16;    //  number of PP32 cycles per basic time tick
    unsigned int overhd_bytes           :8;     //  number of overhead bytes per packet in rate shaping
    unsigned int eth1_eg_qnum           :4;     //  number of egress QoS queues (< 8);
    unsigned int eth1_burst_chk         :1;     //  always 1, more accurate WFQ
    unsigned int eth1_qss               :1;     //  1: FW QoS, 0: HW QoS
    unsigned int shape_en               :1;     //  1: enable rate shaping, 0: disable
    unsigned int wfq_en                 :1;     //  1: WFQ enabled, 0: strict priority enabled
  };

  struct eg_bwctrl_cfg {
    unsigned int fdesc_wm               :16;    //  if free descriptors in QoS/Swap channel is less than this watermark, large size packets are discarded
    unsigned int class_len              :16;    //  if packet length is not less than this value, the packet is recognized as large packet
  };

  struct psave_cfg {
    unsigned int res1                   :15;
    unsigned int start_state            :1;     //  1: start from partial PPE reset, 0: start from full PPE reset
    unsigned int res2                   :15;
    unsigned int sleep_en               :1;     //  1: enable sleep mode, 0: disable sleep mode
  };

  struct test_mode {
    unsigned int res1           :30;
    unsigned int mib_clear_mode :1;     //  1: MIB counter is cleared with TPS-TC software reset, 0: MIB counter not cleared
    unsigned int test_mode      :1;     //  1: test mode, 0: normal mode
  };

  struct gpio_mode {
    unsigned int res1           :3;
    unsigned int gpio_bit_bc1   :5;
    unsigned int res2           :3;
    unsigned int gpio_bit_bc0   :5;

    unsigned int res3           :7;
    unsigned int gpio_bc1_en    :1;

    unsigned int res4           :7;
    unsigned int gpio_bc0_en    :1;
  };

  struct gpio_wm_cfg {
    unsigned int stop_wm_bc1    :8;
    unsigned int start_wm_bc1   :8;
    unsigned int stop_wm_bc0    :8;
    unsigned int start_wm_bc0   :8;
  };

  struct eth_ports_cfg {
    unsigned int    wan_vlanid_hi       :12;
    unsigned int    wan_vlanid_lo       :12;
    unsigned int    res1                :4;
    unsigned int    eth1_type           :2; //  reserved in A5
    unsigned int    eth0_type           :2;
  };

  struct rout_tbl_cfg {
    unsigned int    rout_num            :9;
    unsigned int    wan_rout_mc_num     :7; //  reserved in LAN route table config
    unsigned int    res1                :3;
    unsigned int    wan_rout_mc_ttl_en  :1; //  reserved in LAN route table config
    unsigned int    wan_rout_mc_drop    :1; //  reserved in LAN route table config
    unsigned int    ttl_en              :1;
    unsigned int    tcpdup_ver_en       :1;
    unsigned int    ip_ver_en           :1;
    unsigned int    iptcpudperr_drop    :1;
    unsigned int    rout_drop           :1;
    unsigned int    res2                :6;
  };

  struct gen_mode_cfg1 {
    unsigned int    app2_indirect       :1; //  0: direct, 1: indirect
    unsigned int    us_indirect         :1; //  0: direct, 1: indirect
    unsigned int    us_early_discard_en :1; //  0: disable, 1: enable
    unsigned int    classification_num  :6; //  classification table entry number
    unsigned int    ipv6_rout_num       :8;
    unsigned int    res1                :2;
    unsigned int    session_ctl_en      :1; //  session based rate control enable, 0: disable, 1: enable
    unsigned int    wan_hash_alg        :1; //  Hash Algorithm for IPv4 WAN ingress traffic, 0: dest port, 1: dest port + dest IP
    unsigned int    brg_class_en        :1; //  Multiple Field Based Classification and VLAN Assignment Enable for Bridging Traffic, 0: disable, 1: enable
    unsigned int    ipv4_mc_acc_mode    :1; //  Downstream IPv4 Multicast Acceleration Mode, 0: dst IP only, 1: IP pairs plus port pairs
    unsigned int    ipv6_acc_en         :1; //  IPv6 Traffic Acceleration Enable, 0: disable, 1: enable
    unsigned int    wan_acc_en          :1; //  WAN Ingress Acceleration Enable, 0: disable, 1: enable
    unsigned int    lan_acc_en          :1; //  LAN Ingress Acceleration Enable, 0: disable, 1: enable
    unsigned int    res2                :3;
    unsigned int    sw_iso_mode         :1; //  Switch Isolation Mode, 0: not isolated - ETH0/1 treated as single eth interface, 1: isolated - ETH0/1 treated as two eth interfaces
    unsigned int    sys_cfg             :2; //  System Mode, 0: DSL WAN ETH0/1 LAN, 1: res, 2: ETH0 WAN/LAN ETH1 not used, 3: ETH0 LAN ETH1 WAN
  };

  struct gen_mode_cfg2 {
    unsigned int    res1                :24;
    unsigned int    itf_outer_vlan_vld  :8; //  one bit for one interface, 0: no outer VLAN supported, 1: outer VLAN supported
  };

  struct e1_frag_descba {
    unsigned int    bp_desba            :16;//  bonding part base address of RX/TX channel descriptor list
    unsigned int    desba               :16;//  non-bonding part base address of TX channel descriptor list
  };

  struct b1_ds_ll_ctxt {
    unsigned int    tail_ptr    :16;
    unsigned int    head_ptr    :16;

    unsigned int    sid         :14;
    unsigned int    sop         :1;
    unsigned int    eop         :1;
    unsigned int    fh_valid    :1;
    unsigned int    des_num     :15;

    unsigned int    max_des_num     :8;
    unsigned int    TO_buff_thres   :8;
    unsigned int    max_delay       :16;

    unsigned int    timeout;
  };

  struct rx_bc_cfg {
    unsigned int res1           :14;
    unsigned int local_state    :2;     //  0: local receiver is "Looking", 1: local receiver is "Freewheel Sync False", 2: local receiver is "Synced", 3: local receiver is "Freewheel Sync Truee"
    unsigned int res2           :15;
    unsigned int remote_state   :1;     //  0: remote receiver is "Out-of-Sync", 1: remote receiver is "Synced"
    unsigned int to_false_th    :16;    //  the number of consecutive "Miss Sync" for leaving "Freewheel Sync False" to "Looking" (default 3)
    unsigned int to_looking_th  :16;    //  the number of consecutive "Miss Sync" for leaving "Freewheel Sync True" to "Freewheel Sync False" (default 7)
    /*
     *  firmware use only (total 30 dwords)
     */
    unsigned int rx_cw_rdptr;
    unsigned int cw_cnt;
    unsigned int missed_sync_cnt;
    unsigned int bitmap_last_cw[3];         //  Bitmap of the Last Codeword
    unsigned int bitmap_second_last_cw[3];  //  Bitmap of the Second Last Codeword
    unsigned int bitmap_third_last_cw[3];   //  Bitmap of the Third Last Codeword
    unsigned int looking_cw_cnt;
    unsigned int looking_cw_th;
    unsigned int byte_shift_cnt;
    unsigned int byte_shift_val;
    unsigned int res_word1[14];
  };

  struct rx_gamma_itf_cfg {
    unsigned int res1           :31;
    unsigned int receive_state  :1;     //  0: "Out-of-Fragment", 1: "In-Fragment"
    unsigned int res2           :16;
    unsigned int rx_min_len     :8;     //  min length of packet, padding if packet length is smaller than this value
    unsigned int rx_pad_en      :1;     //  0:  padding disabled, 1: padding enabled
    unsigned int res3           :2;
    unsigned int rx_eth_fcs_ver_dis :1; //  0: ETH FCS verification is enabled, 1: disabled
    unsigned int rx_rm_eth_fcs      :1; //  0: ETH FCS field is not removed, 1: ETH FCS field is removed
    unsigned int rx_tc_crc_ver_dis  :1; //  0: TC CRC verification enabled, 1: disabled
    unsigned int rx_tc_crc_size     :2; //  0: 0-bit, 1: 16-bit, 2: 32-bit
    unsigned int rx_eth_fcs_result;     //  if the ETH FCS result matches this magic number, then the packet is valid packet
    unsigned int rx_tc_crc_result;      //  if the TC CRC result matches this magic number, then the packet is valid packet
    unsigned int rx_crc_cfg     :16;    //  TC CRC config, please check the description of SAR context data structure in the hardware spec
    unsigned int res4           :16;
    unsigned int rx_eth_fcs_init_value; //  ETH FCS initialization value
    unsigned int rx_tc_crc_init_value;  //  TC CRC initialization value
    unsigned int res_word1;
    unsigned int rx_max_len_sel :1;     //  0: normal, the max length is given by MAX_LEN_NORMAL, 1: fragment, the max length is given by MAX_LEN_FRAG
    unsigned int res5           :2;
    unsigned int rx_edit_num2   :4;     //  number of bytes to be inserted/removed
    unsigned int rx_edit_pos2   :7;     //  first byte position to be edited
    unsigned int rx_edit_type2  :1;     //  0: remove, 1: insert
    unsigned int rx_edit_en2    :1;     //  0: disable insertion or removal of data, 1: enable
    unsigned int res6           :3;
    unsigned int rx_edit_num1   :4;     //  number of bytes to be inserted/removed
    unsigned int rx_edit_pos1   :7;     //  first byte position to be edited
    unsigned int rx_edit_type1  :1;     //  0: remove, 1: insert
    unsigned int rx_edit_en1    :1;     //  0: disable insertion or removal of data, 1: enable
    unsigned int res_word2[2];
    unsigned int rx_inserted_bytes_1l;
    unsigned int rx_inserted_bytes_1h;
    unsigned int rx_inserted_bytes_2l;
    unsigned int rx_inserted_bytes_2h;
    int rx_len_adj;                     //  the packet length adjustment, it is sign integer
    unsigned int res_word3[16];
  };

  struct tx_bc_cfg {
    unsigned int fill_wm        :16;    //  default 2
    unsigned int uflw_wm        :16;    //  default 2
    /*
     *  firmware use only (total 31 dwords)
     */
    //  Reserved
    unsigned int res0;
    //  FW Internal Use
    unsigned int holding_pages;
    unsigned int ready_pages;
    unsigned int pending_pages;
    unsigned int cw_wrptr;              // TX codeword write pointer for e
    unsigned int res_word[26];
  };

  struct tx_gamma_itf_cfg {
    unsigned int res_word1;
    unsigned int res1           :8;
    unsigned int tx_len_adj     :4;     //  4 * (not TX_ETH_FCS_GEN_DIS) + TX_TC_CRC_SIZE
    unsigned int tx_crc_off_adj :4;     //  4 + TX_TC_CRC_SIZE
    unsigned int tx_min_len     :8;     //  min length of packet, if length is less than this value, packet is padded
    unsigned int res2           :3;
    unsigned int tx_eth_fcs_gen_dis :1; //  0: ETH FCS generation enabled, 1: disabled
    unsigned int res3           :2;
    unsigned int tx_tc_crc_size :2;     //  0: 0-bit, 1: 16-bit, 2: 32-bit
    unsigned int res4           :24;
    unsigned int queue_mapping  :8;     //  TX queue attached to this Gamma interface
    unsigned int res_word2;
    unsigned int tx_crc_cfg     :16;    //  TC CRC config, please check the description of SAR context data structure in the hardware spec
    unsigned int res5           :16;
    unsigned int tx_eth_fcs_init_value; //  ETH FCS initialization value
    unsigned int tx_tc_crc_init_value;  //  TC CRC initialization value
    /*
     *  firmware use only (total 25 dwords)
     */
    //  FW Internal Use
    unsigned int curr_qid;
    unsigned int fill_pkt_state;
    unsigned int post_pkt_state;
    unsigned int curr_pdma_context_ptr;
    unsigned int curr_sar_context_ptr;
    unsigned int des_addr;
    unsigned int des_qid;
    unsigned int rem_data;
    unsigned int rem_crc;
    //  bonding fields
    unsigned int rem_fh_len;
    unsigned int des_dw0;
    unsigned int des_dw1;
    unsigned int des_bp_dw;
    //  Reserved
    unsigned int res_word3[2];
  };

  struct wtx_qos_q_desc_cfg {
    unsigned int    threshold           :8;
    unsigned int    length              :8;
    unsigned int    addr                :16;
    unsigned int    rd_ptr              :16;
    unsigned int    wr_ptr              :16;
  };

  struct wtx_eg_q_shaping_cfg {
    unsigned int    t                   :8;
    unsigned int    w                   :24;
    unsigned int    s                   :16;
    unsigned int    r                   :16;
    unsigned int    res1                :8;
    unsigned int    d                   :24;  //ppe internal variable
    unsigned int    res2                :8;
    unsigned int    tick_cnt            :8;   //ppe internal variable
    unsigned int    b                   :16;  //ppe internal variable
  };

  struct rout_forward_compare_tbl {
    /*  0h  */
    unsigned int    src_ip              :32;
    /*  1h  */
    unsigned int    dest_ip             :32;
    /*  2h  */
    unsigned int    src_port            :16;
    unsigned int    dest_port           :16;
  };

#define SESSION_MIB_COUNT_LEN 24

  struct rout_forward_action_tbl {
    /*  0h  */
    unsigned int    new_port            :16;
    unsigned int    new_dest_mac54      :16;
    /*  1h  */
    unsigned int    new_dest_mac30      :32;
    /*  2h  */
    unsigned int    new_ip              :32;
    /*  3h  */
    unsigned int    rout_type           :2;
    unsigned int    new_dscp            :6;
    unsigned int    mtu_ix              :3;
    unsigned int    in_vlan_ins         :1; //  Inner VLAN Insertion Enable
    unsigned int    in_vlan_rm          :1; //  Inner VLAN Remove Enable
    unsigned int    new_dscp_en         :1;
    unsigned int    entry_vld           :1;
    unsigned int    protocol            :1;
    unsigned int    dest_list           :8;
    unsigned int    pppoe_mode          :1;
    unsigned int    pppoe_ix            :3; //  not valid for WAN entry
    unsigned int    new_src_mac_ix      :4;
    /*  4h  */
    unsigned int    new_in_vci          :16;//  New Inner VLAN Tag to be inserted
    unsigned int    encap_tunnel        :1; //  encapsulate tunnel
    unsigned int    out_vlan_ix         :5; //  New Outer VLAN Tag pointed by this field to be inserted
    unsigned int    out_vlan_ins        :1; //  Outer VLAN Insertion Enable
    unsigned int    out_vlan_rm         :1; //  Outer VLAN Remove Enable
    unsigned int    tnnl_hdr_idx        :2; //  tunnel header index
    unsigned int    mpoa_type           :2; //  not valid for WAN entry, reserved in D5
    unsigned int    dest_qid            :4; //  in D5, dest_qid for both sides, in A5 DSL WAN Qid (PVC) for LAN side, dest_qid for WAN side
    /*  5h  */
    unsigned int    reserved            :8;
    unsigned int    bytes               :SESSION_MIB_COUNT_LEN; /* UGW5.4: 24 */
  };

  struct wan_rout_multicast_cmp_tbl {
    /*  0h  */
    unsigned int    wan_dest_ip         :32;
    /*  1h  */
    unsigned int    wan_src_ip          :32;
  };

#if defined(RTP_SAMPLING_ENABLE) && RTP_SAMPLING_ENABLE

   struct rtp_sampling_cnt {
    unsigned int    pkt_cnt            :16;
    unsigned int    seq_no             :16;
  };

#endif


#if defined(CAP_WAP_CONFIG) && CAP_WAP_CONFIG
  struct capwap_config_tbl {
    /*  0h  */
    unsigned int    us_max_frag_size    :16;
    unsigned int    us_dest_list        :8;
    unsigned int    qid                 :4;
    unsigned int    rsvd                :2;
    unsigned int    is_ipv4header       :1;
    unsigned int    acc_en              :1;
    /*  1h  */
    unsigned int    rsvd_1;
    /*  2h  */
    unsigned int    rsvd_2;
    /*  3h  */
    unsigned int    ds_mib;
    /*  4h  */
    unsigned int    us_mib;
    /*  5h  */
    unsigned int    da_mac_hi;
    /*  6h  */
    unsigned int    da_mac_lo           :16;
    unsigned int    sa_mac_hi           :16;
    /*  7h  */
    unsigned int    sa_mac_lo;

    /*  8h  */
    unsigned int    eth_type            :16;
    unsigned int    ver                 :4;
    unsigned int    header_len          :4;
    unsigned int    tos                 :8;
    /*  9h  */
    unsigned int    total_len           :16;
    unsigned int    identifier          :16;
    /*  10h  */
    unsigned int    ip_flags            :3;
    unsigned int    ip_frag_off         :13;
    unsigned int    ttl                 :8;
    unsigned int    protocol            :8;
    /*  11h  */
    unsigned int    ip_checksum         :16;
    unsigned int    src_ip_hi           :16;
    /*  12h  */
    unsigned int    src_ip_lo           :16;
    unsigned int    dst_ip_hi           :16;
    /*  13h  */
    unsigned int    dst_ip_lo           :16;
    unsigned int    src_port            :16;
    /*  14h  */
    unsigned int    dst_port            :16;
    unsigned int    udp_ttl_len         :16;
    /*  15h  */
    unsigned int    udp_checksum        :16;
    unsigned int    preamble            :8;
    unsigned int    hlen                :5;
    unsigned int    rid_hi              :3;
    /*  16h  */
    unsigned int    rid_lo              :2;
    unsigned int    wbid                :5;
    unsigned int    t_flag              :1;
    unsigned int    f_flag              :1;
    unsigned int    l_flag              :1;
    unsigned int    w_flag              :1;
    unsigned int    m_flag              :1;
    unsigned int    k_flag              :1;
    unsigned int    flags               :3;
    unsigned int    frag_id             :16;
    /*  17h  */
    unsigned int    frag_off            :13;
    unsigned int    capwap_rsw          :3;
    unsigned int    payload             :16;

  };
#endif

  struct wan_rout_multicast_act_tbl {
    /*  0h  */
    unsigned int    rout_type           :2; //  0: no IP level editing, 1: IP level editing (TTL)
    unsigned int    new_dscp            :6;
    unsigned int    res2                :3;
    unsigned int    in_vlan_ins         :1; //  Inner VLAN Insertion Enable
    unsigned int    in_vlan_rm          :1; //  Inner VLAN Remove Enable
    unsigned int    new_dscp_en         :1;
    unsigned int    entry_vld           :1;
    unsigned int    new_src_mac_en      :1;
    unsigned int    dest_list           :8;
    unsigned int    pppoe_mode          :1;
#if defined(RTP_SAMPLING_ENABLE) && RTP_SAMPLING_ENABLE
    unsigned int    res3                :2;
    unsigned int    sample_en           :1;
#else
    unsigned int    res3                :3;
#endif
    unsigned int    new_src_mac_ix      :4;
    /*  1h  */
    unsigned int    new_in_vci          :16;
    unsigned int    tunnel_rm           :1;
    unsigned int    out_vlan_ix         :5;
    unsigned int    out_vlan_ins        :1;
    unsigned int    out_vlan_rm         :1;
    unsigned int    res5                :4;
    unsigned int    dest_qid            :4;
  };

  struct classification_act_tbl {
    unsigned int    new_in_vci          :16;
    unsigned int    fw_cpu              :1; //  0: forward to original destination, 1: forward to CPU without modification
    unsigned int    out_vlan_ix         :5;
    unsigned int    out_vlan_ins        :1;
    unsigned int    out_vlan_rm         :1;
    unsigned int    res1                :2;
    unsigned int    in_vlan_ins         :1;
    unsigned int    in_vlan_rm          :1;
    unsigned int    dest_qid            :4;
  };

  struct dmrx_dba {
    unsigned int    res                 :16;
    unsigned int    dbase               :16;
  };

  struct dmrx_cba {
    unsigned int    res                 :16;
    unsigned int    cbase               :16;
  };

  struct dmrx_cfg {
    unsigned int    sen                 :1;
    unsigned int    res0                :5;
    unsigned int    trlpg               :1;
    unsigned int    hdlen               :7;
    unsigned int    res1                :3;
    unsigned int    endian              :1;
    unsigned int    psize               :2;
    unsigned int    pnum                :12;
  };

  struct dmrx_pgcnt {
    unsigned int    pgptr               :12;
    unsigned int    dval                :5;
    unsigned int    dsrc                :2;
    unsigned int    dcmd                :1;
    unsigned int    upage               :12;
  };

  struct dmrx_pktcnt {
    unsigned int    res                 :17;
    unsigned int    dsrc                :2;
    unsigned int    dcmd                :1;
    unsigned int    upkt                :12;
  };

  struct dsrx_dba {
    unsigned int    res                 :16;
    unsigned int    dbase               :16;
  };

  struct dsrx_cba {
    unsigned int    res                 :16;
    unsigned int    cbase               :16;
  };

  struct dsrx_cfg {
    unsigned int    res0                :16;
    unsigned int    dren                :1;
    unsigned int    endian              :1;
    unsigned int    psize               :2;
    unsigned int    pnum                :12;
  };

  struct dsrx_pgcnt {
    unsigned int    pgptr               :12;
    unsigned int    ival                :5;
    unsigned int    isrc                :2;
    unsigned int    icmd                :1;
    unsigned int    upage               :12;
  };

  struct ctrl_dmrx_2_fw {
    unsigned int    pg_val              :8;
    unsigned int    byte_off            :8;
    unsigned int    res                 :5;
    unsigned int    cos                 :2;
    unsigned int    bytes_cnt           :8;
    unsigned int    eof                 :1;
  };

  struct ctrl_fw_2_dsrx {
    unsigned int    pkt_status          :4; //  pg_val = (pkt_status << 4) | flag_len
    unsigned int    flag_len            :4;
    unsigned int    byte_off            :8;
    unsigned int    acc_sel             :2;
    unsigned int    fcs_en              :1;
    unsigned int    res                 :1;
    unsigned int    cos                 :3;
    unsigned int    bytes_cnt           :8;
    unsigned int    eof                 :1;
  };

  struct SFSM_dba {
    unsigned int    res                 :17;
    unsigned int    dbase               :15;
  };

  struct SFSM_cba {
    unsigned int    res                 :15;
    unsigned int    cbase               :17;
  };

  struct SFSM_cfg {
    unsigned int    res                 :14;
    unsigned int    rlsync              :1;
    unsigned int    endian              :1;
    unsigned int    idlekeep            :1;
    unsigned int    sen                 :1;
    unsigned int    res1                :6;
    unsigned int    pnum                :8;
  };

  struct SFSM_pgcnt {
    unsigned int    res                 :14;
    unsigned int    dsrc                :1;
    unsigned int    pptr                :8;
    unsigned int    dcmd                :1;
    unsigned int    upage               :8;
  };

  struct FFSM_dba {
    unsigned int    res                 :17;
    unsigned int    dbase               :15;
  };

  struct FFSM_cfg {
    unsigned int    res                 :12;
    unsigned int    rstptr              :1;
    unsigned int    clvpage             :1;
    unsigned int    fidle               :1;
    unsigned int    endian              :1;
    unsigned int    res1                :8;
    unsigned int    pnum                :8;
  };

  struct FFSM_pgcnt {
    unsigned int    res                 :1;
    unsigned int    bptr                :7;
    unsigned int    pptr                :8;
    unsigned int    res0                :1;
    unsigned int    ival                :6;
    unsigned int    icmd                :1;
    unsigned int    vpage               :8;
  };

  struct PTM_CW_CTRL {
    unsigned int    state               :1;
    unsigned int    bad                 :1;
    unsigned int    ber                 :9;
    unsigned int    spos                :7;
    unsigned int    ffbn                :7;
    unsigned int    shrt                :1;
    unsigned int    preempt             :1;
    unsigned int    cwer                :2;
    unsigned int    cwid                :3;
  };

  struct sll_cmd1 { //0x900
    unsigned int res0                   :8;
    unsigned int mtype                  :1;
    unsigned int esize                  :4;
    unsigned int ksize                  :4;
    unsigned int res1                   :2;
    unsigned int embase                 :13;
  };

  struct sll_cmd0 {
    unsigned int res0                   :6;
    unsigned int cmd                    :1;
    unsigned int eynum                  :9;
    unsigned int res1                   :3;
    unsigned int eybase                 :13;
  };

  struct sll_result{
    unsigned int res0                   :21;
    unsigned int vld                    :1;
    unsigned int fo                     :1;
    unsigned int index                  :9;
  };
#else
#endif

struct wan_rx_mib_table {
    unsigned int    res1[2];
    unsigned int    wrx_dropdes_pdu;
    unsigned int    wrx_total_bytes;
    unsigned int    res2[4];
    //  wrx_total_pdu is implemented with hardware counter (not used by PTM TC)
    //  check register "TC_RX_MIB_CMD"
    //  "HEC_INC" used to increase preemption Gamma interface (wrx_total_pdu)
    //  "AIIDLE_INC" used to increase normal Gamma interface (wrx_total_pdu)
};

struct wan_tx_mib_table {
    unsigned int    wrx_total_pdu;
    unsigned int    wrx_total_bytes;
    unsigned int    wtx_total_pdu;
    unsigned int    wtx_total_bytes;

    unsigned int    wtx_cpu_drop_small_pdu;
    unsigned int    wtx_cpu_drop_pdu;
    unsigned int    wtx_fast_drop_small_pdu;
    unsigned int    wtx_fast_drop_pdu;
};

struct itf_mib {
    unsigned int    ig_fast_brg_pkts;           // 0 bridge ?
    unsigned int    ig_fast_brg_bytes;          // 1 ?

    unsigned int    ig_fast_rt_ipv4_udp_pkts;   // 2 IPV4 routing
    unsigned int    ig_fast_rt_ipv4_tcp_pkts;   // 3
    unsigned int    ig_fast_rt_ipv4_mc_pkts;    // 4
    unsigned int    ig_fast_rt_ipv4_bytes;      // 5

    unsigned int    ig_fast_rt_ipv6_udp_pkts;   // 6 IPV6 routing
    unsigned int    ig_fast_rt_ipv6_tcp_pkts;   // 7
    unsigned int    ig_fast_rt_ipv6_mc_pkts;    // 8
    unsigned int    ig_fast_rt_ipv6_bytes;      // 9

    unsigned int    res1;                       // A
    unsigned int    ig_cpu_pkts;                // B
    unsigned int    ig_cpu_bytes;               // C

    unsigned int    ig_drop_pkts;               // D
    unsigned int    ig_drop_bytes;              // E

    unsigned int    eg_fast_pkts;               // F
};

struct ds_bond_gif_mib {
    unsigned int    total_rx_frag_cnt;          //  0
    unsigned int    total_rx_byte_cnt;          //  1
    unsigned int    overflow_frag_cnt;          //  2
    unsigned int    overflow_byte_cnt;          //  3
    unsigned int    out_of_range_frag_cnt;      //  4
    unsigned int    missing_frag_cnt;           //  5
    unsigned int    timeout_frag_cnt;           //  6
    unsigned int    res0[9];
};

struct ds_bg_mib {
    unsigned int    conform_pkt_cnt;            //  0
    unsigned int    conform_frag_cnt;           //  1
    unsigned int    conform_byte_cnt;           //  2
    unsigned int    no_sop_pkt_cnt;             //  3
    unsigned int    no_sop_frag_cnt;            //  4
    unsigned int    no_sop_byte_cnt;            //  5
    unsigned int    no_eop_pkt_cnt;             //  6
    unsigned int    no_eop_frag_cnt;            //  7
    unsigned int    no_eop_byte_cnt;            //  8
    unsigned int    oversize_pkt_cnt;           //  9
    unsigned int    oversize_frag_cnt;          //  A
    unsigned int    oversize_byte_cnt;          //  B
    unsigned int    noncosec_pkt_cnt;           //  C
    unsigned int    noncosec_frag_cnt;          //  D
    unsigned int    noncosec_byte_cnt;          //  E
    unsigned int    res0;                       //  F
};

struct powersaving_profile {
    unsigned int    wakeup;                     //  number of wakeup from TIMER0 (0), DPLUS_IN (1), SFSM (2), reserved for RUNNING (3)
    unsigned int    cycles_lo;                  //  lower DWORD of sleep cycle before wakeup from TIMER0 (0), DPLUS_IN (1), SFSM (2), lower DWORD of running cycle for RUNNING (3)
    unsigned int    cycles_hi;                  //  higher DWORD of sleep cycle before wakeup from TIMER0 (0), DPLUS_IN (1), SFSM (2), higher DWORD of running cycle for RUNNING (3)
    unsigned int    res1;
};

struct mac_tbl_item {
    struct mac_tbl_item    *next;
    int                     ref;

    unsigned char           mac[PPA_ETH_ALEN];
    unsigned int            mac0;
    unsigned int            mac1;
    unsigned int            age;
    unsigned int            timestamp;
};

#endif /* IFXMIPS_PPA_HAL_AR9_A5_H */
