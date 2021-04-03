/******************************************************************************
 **
 ** PROJECT      : UEIP
 ** FILE NAME    : ifxmips_ppa_datapath_vr9_e5.c
 ** MODULES      : MII0/1 Acceleration Package (VR9 PPA E5 v1)
 **
 ** DATE         : 19 OCT 2009
 ** AUTHOR       : Xu Liang
 ** DESCRIPTION  : MII0/1 Driver with Acceleration Firmware (D5 v1) + E1
 ** COPYRIGHT    :   Copyright (c) 2006
 **          Infineon Technologies AG
 **          Am Campeon 1-12, 85579 Neubiberg, Germany
 **
 **    This program is free software; you can redistribute it and/or modify
 **    the Free Software Foundation; either version 2 of the License, or
 **    it under the terms of the GNU General Public License as published by
 **    (at your option) any later version.
 **
 ** HISTORY
 ** $Date        $Author         $Comment
 ** 19 OCT 2009  Xu Liang        Initiate Version
** 10 DEC 2012  Manamohan Shetty       Added the support for RTP,MIB mode and CAPWAP 
**                                     Features 
 *******************************************************************************/

/*
 * ####################################
 *              Version No.
 * ####################################
 */


#define VER_FAMILY      0x40        //  bit 0: res
//      1: Danube
//      2: Twinpass
//      3: Amazon-SE
//      4: res
//      5: AR9
//      6: VR9
//      7: AR10
#define VER_DRTYPE      0x03        //  bit 0: Normal Data Path driver
//      1: Indirect-Fast Path driver
//      2: HAL driver
//      3: Hook driver
//      4: Stack/System Adaption Layer driver
//      5: PPA API driver
#define VER_INTERFACE   0x03        //  bit 0: MII 0
//      1: MII 1
//      2: ATM WAN
//      3: PTM WAN
#define VER_ACCMODE     0x01        //  bit 0: Routing
//      1: Bridging
#define VER_MAJOR       0
#define VER_MID         1
#define VER_MINOR       1

#define PTM_DATAPATH 1
#ifdef CONFIG_IFX_ETHSW_API
// #define AVM_DOESNT_USE_IFX_ETHSW_API 1
#endif

#define PPE_HOLD_TIMEOUT               120



/*
 * ####################################
 *              Head File
 * ####################################
 */

/*
 *  Common Head File
 */
#include <linux/vmalloc.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/ctype.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/miscdevice.h>
#include <linux/init.h>
#include <linux/etherdevice.h>  /*  eth_type_trans  */
#include <linux/netdevice.h>
#include <linux/ethtool.h>      /*  ethtool_cmd     */
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#include <asm/uaccess.h>
#include <asm/unistd.h>
#include <asm/irq.h>
#include <asm/delay.h>
#include <asm/io.h>
#include <asm/prom.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/xfrm.h>
#if __has_include(<avm/pa/ifx_multiplexer.h>)
#include <avm/pa/ifx_multiplexer.h>
#else
#include <linux/avm_pa_ifx_multiplexer.h>
#endif
#include <net/pkt_sched.h>
#include <avm/net/net.h>
#include <linux/mii.h>

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
#include <ifx_eth_framework.h>

#include <switch_api/ifx_ethsw_kernel_api.h>
#include <switch_api/ifx_ethsw_api.h>
#include <avm/net/ifx_ppa_stack_al.h>
#include <avm/net/ifx_ppa_api_directpath.h>
#include <avm/net/ifx_ppa_api.h>
#include <avm/net/ifx_ppa_ppe_hal.h>

#include <avmnet_config.h>
#if defined(CONFIG_IP_MULTICAST_FASTFORWARD)
#include <avmnet_multicast.h>
#endif /*--- #if defined(CONFIG_IP_MULTICAST_FASTFORWARD) ---*/

#include "../common/swi_ifx_common.h"
#include "../common/ifx_ppa_datapath.h"
#include "../7port/swi_7port.h"
#include "../7port/swi_7port_reg.h"
#include "ifxmips_ppa_datapath_fw_vr9_d5.h"
#include "ifxmips_ppa_datapath_fw_vr9_e1.h"

#include "ifxmips_ppa_hal_vr9_e5.h"
#include "../7port/ifxmips_ppa_datapath_7port_common.h"

#if defined(CONFIG_LTQ_ETH_OAM) || defined(CONFIG_LTQ_ETH_OAM_MODULE)
#include <avm/net/ltq_eth_oam_handler.h>
#endif
/*
 *  Bonding Data Structure Header File
 */
#include "vr9_bonding_fw_data_structure.h"
#include "vr9_bonding_addr_def.inc"


/*
 * ####################################
 *   Parameters to Configure PPE
 * ####################################
 */

#if defined(DEBUG_SPACE_BETWEEN_HEADER_AND_DATA)
static unsigned int min_fheader_byteoff = ~0;
#endif

static int ethwan = 0;

static int wanitf = ~0;

static int ipv6_acc_en = 1;

static int wanqos_en = MAX_WAN_QOS_QUEUES;
static uint32_t lan_port_seperate_enabled=0, wan_port_seperate_enabled=0;


#ifdef CONFIG_IFX_PPE_E5_OFFCHIP_BONDING
  static int dsl_bonding     = 2;
  static int pci_ppe_addr    = 0x18200000;
  static int bm_addr         = 0x01E00000;
  static int pci_ddr_addr    = 0x18000000;
  static int pci_bm_ppe_addr = 0x19600000;
#else
  static int dsl_bonding     = 0;
#endif

static char *avm_ata_dev = NULL;

static int unknown_session_dump_len = DUMP_SKB_LEN;
module_param(unknown_session_dump_len, int, S_IRUSR | S_IWUSR);

static int queue_gamma_map[4] = { 0xFE, 0x01, 0x00, 0x00 };
static int qos_queue_len[MAX_WAN_QOS_QUEUES] = {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0};


#define MODULE_PARM_ARRAY(a, b)   module_param_array(a, int, NULL, 0)
#define MODULE_PARM(a, b)         module_param(a, int, 0)

MODULE_PARM(ethwan, "i");
MODULE_PARM_DESC(ethwan, "WAN mode, 2 - ETH WAN, 1 - ETH0 mixed, 0 - DSL WAN.");
MODULE_PARM(wanitf, "i");
MODULE_PARM_DESC(
		wanitf,
		"WAN interfaces, bit 0 - ETH0, 1 - ETH1, 2 - reserved for CPU0, 3/4/5 - DirectPath, 6/7 - DSL");

module_param(avm_ata_dev, charp, 0);
MODULE_PARM_DESC(avm_ata_dev, "eth0: ethernet_wan with NAT, ath0: WLAN_WAN with NAT");
MODULE_PARM(ipv6_acc_en, "i");
MODULE_PARM_DESC(ipv6_acc_en, "IPv6 support, 1 - enabled, 0 - disabled.");

MODULE_PARM(wanqos_en, "i");
MODULE_PARM_DESC(wanqos_en, "WAN QoS support, 0 - disabled, others - enabled and the number of queues.");


MODULE_PARM(dsl_bonding, "i");
MODULE_PARM_DESC(dsl_bonding, "DSL Bonding Mode, 0 - disable bonding, 1 - on chip bonding, 2 - off chip bonding");
#ifdef CONFIG_IFX_PPE_E5_OFFCHIP_BONDING
  MODULE_PARM(pci_ppe_addr, "i");
  MODULE_PARM_DESC(pci_ppe_addr, "Base Address of PPE on System Slave from System Master's point of view.");
  MODULE_PARM(bm_addr, "i");
  MODULE_PARM_DESC(bm_addr, "Base Address of Bonding Master from System Master's point of view.");
  MODULE_PARM(pci_ddr_addr, "i");
  MODULE_PARM_DESC(pci_ddr_addr, "Base Address of DDR from System Slave's point of view.");
  MODULE_PARM(pci_bm_ppe_addr, "i");
  MODULE_PARM_DESC(pci_bm_ppe_addr, "Base Address of PPE on Bonding Slave from Bonding Master's point of view.");
#endif

MODULE_PARM_ARRAY(queue_gamma_map, "4-4i");
MODULE_PARM_DESC(queue_gamma_map,
		"TX QoS queues mapping to 4 TX Gamma interfaces.");

MODULE_PARM_ARRAY(qos_queue_len, "1-8i");
MODULE_PARM_DESC(qos_queue_len, "QoS queue's length for each QoS queue");

/*
 * ####################################
 *              Board Type
 * ####################################
 */

#define BOARD_VR9_REFERENCE                     0x01

/*
 * ####################################
 *              Definition
 * ####################################
 */

#define BOARD_CONFIG                            BOARD_VR9_REFERENCE

#define AVM_SETUP_MAC 							1

#define DEBUG_SKB_SWAP                          0

#define ENABLE_P2P_COPY_WITHOUT_DESC            1

#define ENABLE_LED_FRAMEWORK                    0

#ifdef CONFIG_IFX_PPA_NAPI_ENABLE
#define ENABLE_NAPI                           1
#else
#error driver just working with napi support so far
#endif

#define ENABLE_SWITCH_FLOW_CONTROL              0

#define ENABLE_STATS_ON_VCC_BASIS               1

#define ENABLE_MY_MEMCPY                        0

#define ENABLE_FW_MODULE_TEST                   0

#if defined(CONFIG_AVMNET_DEBUG) 
#define DEBUG_DUMP_INIT                         0

#define DEBUG_FIRMWARE_TABLES_PROC              1

#define DEBUG_MEM_PROC                          1

#define DEBUG_FW_PROC                           1

#define DEBUG_MIRROR_PROC                       0

#define DEBUG_BONDING_PROC                      0

#define DEBUG_REDIRECT_FASTPATH_TO_CPU          0

#else // defined(CONFIG_AVMNET_DEBUG) 

#define DEBUG_DUMP_INIT                         0

#define DEBUG_FIRMWARE_TABLES_PROC              1

#define DEBUG_FW_PROC                           0

#define DEBUG_MIRROR_PROC                       0

#define DEBUG_BONDING_PROC                      0

#define DEBUG_REDIRECT_FASTPATH_TO_CPU          0

#endif // defined(CONFIG_AVMNET_DEBUG) 

#define PPE_MAILBOX_IGU0_INT                    INT_NUM_IM2_IRL23

#define PPE_MAILBOX_IGU1_INT                    INT_NUM_IM2_IRL24

#if defined(CONFIG_DSL_MEI_CPE_DRV) && !defined(CONFIG_IFXMIPS_DSL_CPE_MEI)
#define CONFIG_IFXMIPS_DSL_CPE_MEI            1
#endif

#define AVM_NET_TRACE_DUMP				  		0

//  specific board related configuration
#if defined (BOARD_CONFIG)
#if BOARD_CONFIG == BOARD_VR9_REFERENCE
#endif
#endif

#if !defined(DISABLE_INLINE) || !DISABLE_INLINE
#define INLINE                                inline
#else
#define INLINE
#endif

/*
 *  Mailbox Signal Bit
 */
#define DMA_TX_CH1_SIG                          (1 << 31)   //  IGU0
#define DMA_TX_CH0_SIG                          (1 << 30)   //  IGU0
#define CPU_TO_WAN_TX_SIG                       (1 << 17)   //  IGU1
#define CPU_TO_WAN_SWAP_SIG                     (1 << 16)   //  IGU1
#define WAN_RX_SIG(i)                           (1 << 0)    //  IGU1, 0: Downstream DSL Traffic
/*
 *  Eth Mode
 */
#define RGMII_MODE                              0
#define MII_MODE                                1
#define REV_MII_MODE                            2
#define RED_MII_MODE_IC                         3   //  Input clock
#define RGMII_MODE_100MB                        4
#define TURBO_REV_MII_MODE                      6	//  Turbo Rev Mii mode
#define RED_MII_MODE_OC                         7   //  Output clock
#define RGMII_MODE_10MB                         8

/*
 *  Bonding Definition
 */
#define L1_PTM_TC_BOND_MODE                     0
#define L2_ETH_TRUNK_MODE                       1
#define LINE_NUMBER                             2
#define BEARER_CHANNEL_PER_LINE                 2
#ifdef CONFIG_IFX_PPE_E5_OFFCHIP_BONDING
  #define SAVE_AND_SWITCH_PPE_BASE_TO_SM(_SAVED_ADDR)   \
        unsigned int _SAVED_ADDR;                       \
        _SAVED_ADDR = g_ppe_base_addr;                  \
        g_ppe_base_addr = IFX_PPE_ORG(0);

  #define SAVE_AND_SWITCH_PPE_BASE_TO_BM(_SAVED_ADDR)   \
        unsigned int _SAVED_ADDR;                       \
        _SAVED_ADDR = g_ppe_base_addr;                  \
        g_ppe_base_addr = IFX_PPE_ORG(1);

  #define RESTORE_PPE_BASE(_SAVED_ADDR)                 \
        g_ppe_base_addr = _SAVED_ADDR;
  #define PPE_ID                                (g_ppe_base_addr == IFX_PPE_ORG(0) ? 0 : 1)
#else
  #define SAVE_AND_SWITCH_PPE_BASE_TO_SM(_SAVED_ADDR)
  #define SAVE_AND_SWITCH_PPE_BASE_TO_BM(_SAVED_ADDR)
  #define RESTORE_PPE_BASE(_SAVED_ADDR)
  #define PPE_ID                                0
#endif
#define MAX_DESC_PER_GAMMA_ITF                  128


/*
 *  Constant Definition
 */
#define ETH_WATCHDOG_TIMEOUT                    (10 * HZ)
#define ETOP_MDIO_DELAY                         1
#define IDLE_CYCLE_NUMBER                       30000

/*change DMA packet size for IP tunnelling feature */
#define DMA_PACKET_SIZE                         1664        //  ((1590 + 62 + 8) + 31) & ~31
#define DMA_RX_ALIGNMENT                        128
#define DMA_TX_ALIGNMENT                        32
#define DMA_RX_ALIGNED(x)                          (((unsigned int)(x) + DMA_RX_ALIGNMENT - 1) & ~(DMA_RX_ALIGNMENT - 1))
#define DMA_TX_ALIGNED(x)                          (((unsigned int)(x) + DMA_TX_ALIGNMENT - 1) & ~(DMA_TX_ALIGNMENT - 1))
/*
 *  Ethernet Frame Definitions
 */
#define ETH_CRC_LENGTH                          4
#define ETH_MAX_DATA_LENGTH                     MAC_FLEN_MAX_SIZE
#define ETH_MIN_TX_PACKET_LENGTH                ETH_ZLEN

/*
 *  PDMA Settings
 */
//#define EMA_CMD_BUF_LEN                         0x0010
//#define EMA_CMD_BASE_ADDR                       (0x1710 << 2)
//#define EMA_DATA_BUF_LEN                        0x0040
//#define EMA_DATA_BASE_ADDR                      (0x16d0 << 2)
//#define EMA_WRITE_BURST                         0x02
//#define EMA_READ_BURST                          0x02
/*
 *  Firmware Settings
 */
#define CPU_TO_WAN_TX_DESC_NUM                  64  //  WAN CPU TX
#define WAN_TX_DESC_NUM(i)                      __ETH_WAN_TX_QUEUE_LEN
#define CPU_TO_WAN_SWAP_DESC_NUM                32
#define WAN_TX_DESC_NUM_TOTAL                   512
#define WAN_RX_DESC_NUM                         64
#define DMA_TX_CH1_DESC_NUM                     (g_eth_wan_mode ? (g_eth_wan_mode == 3 /* ETH WAN */ && g_wanqos_en ? 64 : DMA_RX_CH1_DESC_NUM) : WAN_RX_DESC_NUM)
#define DMA_RX_CH1_DESC_NUM                     64
#define DMA_RX_CH2_DESC_NUM                     64
#define DMA_TX_CH0_DESC_NUM                     DMA_RX_CH2_DESC_NUM //  DMA_TX_CH0_DESC_NUM is used to replace DMA_TX_CH2_DESC_NUM

//  Bonding Specific
#define E1_FRAG_TX_DESC_NUM_TOTAL               (16 * 8)
#define E1_FRAG_RX_DESC_NUM                     32
#define B1_RX_LINK_LIST_DESC_NUM                (32 * 8)
#define __US_BOND_PKT_DES_BASE(i)               (0x5C00 + (i) * 0x80)
#define US_BOND_PKT_DES_BASE(i)                 ((volatile struct US_BOND_PKT_DES *)SB_BUFFER_BOND(2, __US_BOND_PKT_DES_BASE(i)))
#define __US_BG_CTXT_BASE(i)                    (__US_BG_CTXT + (i) * 4)
#define US_BG_CTXT_BASE(i)                      ((volatile struct US_BG_CTXT *)SB_BUFFER_BOND(2, __US_BG_CTXT_BASE(i)))
#define __DS_BOND_GIF_LL_DES_BASE(i)            (__DS_BOND_GIF_LL_DESBA + (i) * 2)
#define DS_BOND_GIF_LL_DES_BASE(i)              ((volatile struct DS_BOND_GIF_LL_DES *)SB_BUFFER_BOND(2, __DS_BOND_GIF_LL_DES_BASE(i)))
#define DS_BOND_FREE_LL_CTXT_BASE               ((volatile struct DS_BOND_GIF_LL_CTXT *)SB_BUFFER_BOND(2, __DS_BOND_FREE_LL_CTXT))
#define __DS_BOND_GIF_LL_CTXT_BASE(i)           (__DS_BOND_GIF_LL_CTXT + (i) * 4)
#define DS_BOND_GIF_LL_CTXT_BASE(i)             ((volatile struct DS_BOND_GIF_LL_CTXT *)SB_BUFFER_BOND(2, __DS_BOND_GIF_LL_CTXT_BASE(i)))
#define __DS_BG_CTXT_BASE(i)                    (__DS_BG_CTXT + (i) * 8)
#define DS_BG_CTXT_BASE(i)                      ((volatile struct DS_BG_CTXT *)SB_BUFFER_BOND(2, __DS_BG_CTXT_BASE(i)))
#define __DS_BOND_GIF_MIB_BASE(i)               (__DS_BOND_GIF_MIB + i * 16)
#define DS_BOND_GIF_MIB_BASE(i)                 ((volatile struct DS_BOND_GIF_MIB *)SB_BUFFER_BOND(2, __DS_BOND_GIF_MIB_BASE(i)))
#define __DS_BG_MIB_BASE(i)                     (__DS_BG_MIB + i * 16)
#define DS_BG_MIB_BASE(i)                       ((volatile struct DS_BG_MIB *)SB_BUFFER_BOND(2, __DS_BG_MIB_BASE(i)))
#define US_BOND_GIF_MIB(i)                      SB_BUFFER_BOND(2, __US_E1_FRAG_Q_FRAG_MIB + (i))

#define IFX_PPA_PORT_NUM                        8
#define AVM_PPE_CLOCK_DSL						288
#define AVM_PPE_CLOCK_ATA						500


/*
 *  Bits Operation
 */
//#define GET_BITS(x, msb, lsb)                   (((x) & ((1 << ((msb) + 1)) - 1)) >> (lsb))
//#define SET_BITS(x, msb, lsb, value)            (((x) & ~(((1 << ((msb) + 1)) - 1) ^ ((1 << (lsb)) - 1))) | (((value) & ((1 << (1 + (msb) - (lsb))) - 1)) << (lsb)))
/*
 *  Internal Tantos Switch Related
 */
//#define MDIO_OP_READ                            (2 << 10)
//#define MDIO_OP_WRITE                           (1 << 10)
//#define TANTOS_CHIP_ID                          0x2599


/*
 *  DMA/EMA Descriptor Base Address
 */
#define CPU_TO_WAN_TX_DESC_BASE                 ((volatile struct tx_descriptor *)SB_BUFFER_BOND(0, 0x3D00))                 /*         64 each queue    */
#define __ETH_WAN_TX_QUEUE_NUM                  g_wanqos_en
#define __ETH_VIRTUAL_TX_QUEUE_NUM              SWITCH_CLASS_NUM    //  LAN interface, there is no real firmware queue, but only virtual queue maintained by driver, so that skb->priority => firmware QId => switch class can be mapped.
#define SWITCH_CLASS_NUM                        8
#define __ETH_WAN_TX_QUEUE_LEN                  ((WAN_TX_DESC_NUM_TOTAL / __ETH_WAN_TX_QUEUE_NUM) < 256 ? (WAN_TX_DESC_NUM_TOTAL / __ETH_WAN_TX_QUEUE_NUM) : 255)
#define __ETH_WAN_TX_DESC_BASE(i)               (0x5C00 + (i) * 2 * __ETH_WAN_TX_QUEUE_LEN)
#define WAN_QOS_DESC_BASE                       0x5C00
#define WAN_TX_DESC_BASE(i)                     ((volatile struct tx_descriptor *)SB_BUFFER_BOND(2, __ETH_WAN_TX_DESC_BASE(i)))  /*  i < __ETH_WAN_TX_QUEUE_NUM, __ETH_WAN_TX_QUEUE_LEN each queue    */
#define CPU_TO_WAN_SWAP_DESC_BASE               ((volatile struct tx_descriptor *)SB_BUFFER_BOND(0, 0x2E80))                 /*         64 each queue    */
#define WAN_RX_DESC_PPE                         0x2600
#define WAN_RX_DESC_BASE                        ((volatile struct tx_descriptor *)SB_BUFFER_BOND(0, WAN_RX_DESC_PPE))        /*         64 each queue    */
#ifdef CONFIG_IFX_PPE_E5_OFFCHIP_BONDING
  #define DMA_TX_CH1_WITH_BONDING               (g_dsl_bonding != 2 ? (unsigned int)WAN_RX_DESC_BASE : g_bm_addr)
#else
  #define DMA_TX_CH1_WITH_BONDING               WAN_RX_DESC_BASE
#endif
#define DMA_TX_CH1_DESC_BASE                    (g_eth_wan_mode ? (g_eth_wan_mode == 3 /* ETH WAN */ && g_wanqos_en ? ((volatile struct rx_descriptor *)SB_BUFFER_BOND(0, 0x2600)) : DMA_RX_CH1_DESC_BASE) : (volatile struct rx_descriptor *)DMA_TX_CH1_WITH_BONDING)
#define DMA_RX_CH1_DESC_BASE                    ((volatile struct rx_descriptor *)SB_BUFFER_BOND(2, 0x2580))                 /*         64 each queue    */
#define DMA_RX_CH2_DESC_BASE                    ((volatile struct rx_descriptor *)SB_BUFFER_BOND(0, 0x2F00))                 /*         64 each queue    */
#define DMA_TX_CH0_DESC_BASE                    DMA_RX_CH2_DESC_BASE
//  Bonding Specific
#define E1_FRAG_TX_DESC_NB_PPE(i)               (0x3800 + (i) * 2 * 16) //  i < 8
#define E1_FRAG_TX_DESC_B_PPE(i)                (0x3F60 + (i) * 16)     //  i < 8
#define E1_FRAG_TX_DESC_NB_BASE(i)              ((volatile struct tx_descriptor *)SB_BUFFER_BOND(2, E1_FRAG_TX_DESC_NB_PPE(i)))
#define E1_FRAG_RX_DESC_PPE(i)                  (0x3D00 + (i) * 0x40)   //  i < 2
#define E1_FRAG_RX_DESC_BASE(i)                 ((volatile struct rx_descriptor *)SB_BUFFER_BOND(2, E1_FRAG_RX_DESC_PPE(i)))
#define B1_RX_LINK_LIST_DESC_PPE                0x4800
#define B1_RX_LINK_LIST_DESC_BASE               ((volatile struct tx_descriptor *)SB_BUFFER_BOND(2, B1_RX_LINK_LIST_DESC_PPE))


#if defined(DEBUG_FW_PROC) && DEBUG_FW_PROC
/*
 *  Firmware Proc
 */
//  need check with hejun
#define TIR(i)                                ((volatile u32 *)(0xBE200140) + (i))

#define __CPU_TX_SWAPPER_DROP_PKTS            SB_BUFFER_BOND(2, 0x29A8)
#define __CPU_TX_SWAPPER_DROP_BYTES           SB_BUFFER_BOND(2, 0x29A9)
#define __DSLWAN_FP_SWAPPER_DROP_PKTS         SB_BUFFER_BOND(2, 0x29AA)
#define __DSLWAN_FP_SWAPPER_DROP_BYTES        SB_BUFFER_BOND(2, 0x29AB)

#define __CPU_TXDES_SWAP_RDPTR                SB_BUFFER_BOND(2, 0x29AC)
#define __DSLWAN_FP_RXDES_SWAP_RDPTR          SB_BUFFER_BOND(2, 0x29AD)
#define __DSLWAN_FP_RXDES_DPLUS_WRPTR         SB_BUFFER_BOND(2, 0x29AE)

#define __DSLWAN_TX_PKT_CNT0                  SB_BUFFER_BOND(2, 0x29B0)
#define __DSLWAN_TX_PKT_CNT1                  SB_BUFFER_BOND(2, 0x29B1)
#define __DSLWAN_TX_PKT_CNT2                  SB_BUFFER_BOND(2, 0x29B2)
#define __DSLWAN_TX_PKT_CNT3                  SB_BUFFER_BOND(2, 0x29B3)
#define __DSLWAN_TX_PKT_CNT4                  SB_BUFFER_BOND(2, 0x29B4)
#define __DSLWAN_TX_PKT_CNT5                  SB_BUFFER_BOND(2, 0x29B5)
#define __DSLWAN_TX_PKT_CNT6                  SB_BUFFER_BOND(2, 0x29B6)
#define __DSLWAN_TX_PKT_CNT7                  SB_BUFFER_BOND(2, 0x29B7)

#define __DSLWAN_TXDES_SWAP_PTR0              SB_BUFFER_BOND(2, 0x29B8)
#define __DSLWAN_TXDES_SWAP_PTR1              SB_BUFFER_BOND(2, 0x29B9)
#define __DSLWAN_TXDES_SWAP_PTR2              SB_BUFFER_BOND(2, 0x29BA)
#define __DSLWAN_TXDES_SWAP_PTR3              SB_BUFFER_BOND(2, 0x29BB)
#define __DSLWAN_TXDES_SWAP_PTR4              SB_BUFFER_BOND(2, 0x29BC)
#define __DSLWAN_TXDES_SWAP_PTR5              SB_BUFFER_BOND(2, 0x29BD)
#define __DSLWAN_TXDES_SWAP_PTR6              SB_BUFFER_BOND(2, 0x29BE)
#define __DSLWAN_TXDES_SWAP_PTR7              SB_BUFFER_BOND(2, 0x29BF)

// SB_BUFFER_BOND(2, 0x29C0) - SB_BUFFER_BOND(2, 0x29C7)
#define __VLAN_PRI_TO_QID_MAPPING             SB_BUFFER_BOND(2, 0x29C0)

//
// etx MIB: SB_BUFFER_BOND(2, 0x29C8) - SB_BUFFER_BOND(2, 0x29CF)
#define __DSLWAN_TX_THRES_DROP_PKT_CNT0       SB_BUFFER_BOND(2, 0x29C8)
#define __DSLWAN_TX_THRES_DROP_PKT_CNT1       SB_BUFFER_BOND(2, 0x29C9)
#define __DSLWAN_TX_THRES_DROP_PKT_CNT2       SB_BUFFER_BOND(2, 0x29CA)
#define __DSLWAN_TX_THRES_DROP_PKT_CNT3       SB_BUFFER_BOND(2, 0x29CB)
#define __DSLWAN_TX_THRES_DROP_PKT_CNT4       SB_BUFFER_BOND(2, 0x29CC)
#define __DSLWAN_TX_THRES_DROP_PKT_CNT5       SB_BUFFER_BOND(2, 0x29CD)
#define __DSLWAN_TX_THRES_DROP_PKT_CNT6       SB_BUFFER_BOND(2, 0x29CE)
#define __DSLWAN_TX_THRES_DROP_PKT_CNT7       SB_BUFFER_BOND(2, 0x29CF)

#define __CPU_TO_DSLWAN_TX_PKTS               SB_BUFFER_BOND(2, 0x29D0)
#define __CPU_TO_DSLWAN_TX_BYTES              SB_BUFFER_BOND(2, 0x29D1)

#define ACC_ERX_PID                           SB_BUFFER_BOND(2, 0x2B00)
#define ACC_ERX_PORT_TIMES                    SB_BUFFER_BOND(2, 0x2B01)
#define SLL_ISSUED                            SB_BUFFER_BOND(2, 0x2B02)
#define BMC_ISSUED                            SB_BUFFER_BOND(2, 0x2B03)
#define DPLUS_RX_ON                           SB_BUFFER_BOND(2, 0x5003)
#define ISR_IS                                SB_BUFFER_BOND(2, 0x5004)

#define PRESEARCH_RDPTR                       SB_BUFFER_BOND(2, 0x2B06)

#define SLL_ERX_PID                           SB_BUFFER_BOND(2, 0x2B04)
#define SLL_PKT_CNT                           SB_BUFFER_BOND(2, 0x2B08)
#define SLL_RDPTR                             SB_BUFFER_BOND(2, 0x2B0A)

#define EDIT_PKT_CNT                          SB_BUFFER_BOND(2, 0x2B0C)
#define EDIT_RDPTR                            SB_BUFFER_BOND(2, 0x2B0E)

#define DPLUSRX_PKT_CNT                       SB_BUFFER_BOND(2, 0x2B10)
#define DPLUS_RDPTR                           SB_BUFFER_BOND(2, 0x2B12)

#define SLL_STATE_NULL                        0
#define SLL_STATE_DA                          1
#define SLL_STATE_SA                          2
#define SLL_STATE_DA_BC                       3
#define SLL_STATE_ROUTER                      4

#define PRE_DPLUS_PTR                        SB_BUFFER_BOND(2, 0x5001)
#define DPLUS_PTR                            SB_BUFFER_BOND(2, 0x5002)
#define DPLUS_CNT                            SB_BUFFER_BOND(2, 0x5000)
#endif

#define PPA_VDEV_ID(hw_dev_id) (hw_dev_id +   IFX_VIRT_DEV_OFFSET)
#define HW_DEV_ID(ppa_vdev_id) (ppa_vdev_id - IFX_VIRT_DEV_OFFSET)

/*
 * ####################################
 *              Data Type
 * ####################################
 */

/*
 *  Switch Header, Flag Header & Descriptor
 */
#if ! defined(__BIG_ENDIAN)
#error
#endif

struct rx_descriptor {
  //  0 - 3h
  unsigned int    own                 :1; //  0: PPE, 1: MIPS, this value set is for DSL PKT RX and DSL OAM RX, it's special case.
  unsigned int    c                   :1;
  unsigned int    sop                 :1;
  unsigned int    eop                 :1;
  unsigned int    byteoff             :5;
  unsigned int    gid                 :2;
  unsigned int    qid                 :4;
  unsigned int    err                 :1; //  0: no error, 1: error (RA, IL, CRC, USZ, OVZ, MFL, CPI, CPCS-UU)
  unsigned int    datalen             :16;
  //  4 - 7h
  unsigned int    res3                :3;
  unsigned int    dataptr             :29;
};

//    CPU TX is different compare to QoS TX
struct tx_descriptor {
  //  0 - 3h
  unsigned int    own                 :1; //  0: MIPS, 1: PPE, for CPU to WAN TX, it's inverse, 0: PPE, 1: MIPS
  unsigned int    c                   :1;
  unsigned int    sop                 :1;
  unsigned int    eop                 :1;
  unsigned int    byteoff             :5;
  unsigned int    qid                 :4;
  unsigned int    res1                :2;
  unsigned int    small               :1;
  unsigned int    datalen             :16;
  //  4 - 7h
  unsigned int    dataptr             :32;
};

/*
 * ####################################
 *             Declaration
 * ####################################
 */

/*
 *  Network Operations (PTM)
 */
int ifx_ppa_ptm_init(struct net_device *);

static int ifx_ppa_ptm_open(struct net_device *);
static int ifx_ppa_ptm_stop(struct net_device *);
static int ifx_ppa_ptm_hard_start_xmit(struct sk_buff *, struct net_device *);
static int ifx_ppa_ptm_hard_start_xmit_b(struct sk_buff *, struct net_device *); //bonding

static int ifx_ppa_ptm_ioctl(struct net_device *, struct ifreq *, int);
static void ifx_ppa_ptm_tx_timeout(struct net_device *);

static int avm_get_ptm_qid(unsigned int skb_priority);

static int ppe_clk_change(unsigned int arg, unsigned int flags );

/*
 *  Network operations help functions
 */
static INLINE int eth_xmit(struct sk_buff *, int, int, int, int);

/*
 *  ioctl help functions
 */
static INLINE int ethtool_ioctl(struct net_device *, struct ifreq *);

/*
 *  Buffer manage functions
 */
static INLINE struct sk_buff *alloc_skb_rx(void);
static INLINE struct sk_buff *alloc_skb_tx(int);
/*
 *  Mailbox handler
 */
static irqreturn_t mailbox1_irq_handler(int, void *);

/*
 *  Turn On/Off Dma
 */
static INLINE void turn_on_dma_rx(int);
// static INLINE void turn_off_dma_rx(int);

/*
 *  DMA interface functions
 */
static u8 *dma_buffer_alloc(int, int *, void **);
static int dma_buffer_free(u8 *, void *);
#if defined(ENABLE_NAPI) && ENABLE_NAPI
#if 0
static void dma_activate_poll(struct dma_device_info *);
static void dma_inactivate_poll(struct dma_device_info *);
#endif
#endif
static int dma_int_handler(struct dma_device_info *, int, int);
static INLINE int handle_skb_e5(struct sk_buff *skb);
static int process_backlog_e5(struct napi_struct *napi, int quota);

/*
 *  Bonding functions
 */
static void switch_to_eth_trunk(void);
static void switch_to_ptmtc_bonding(void);
static void update_max_delay(int);


/*
 *  Hardware init  & clean-up functions
 */
static INLINE void reset_ppe(void);
static INLINE void init_pmu(void);
static INLINE void start_cpu_port(void);
static INLINE void init_internal_tantos_qos_setting(void);
static INLINE void init_internal_tantos(void);
static INLINE void init_dplus(void);
static INLINE void init_pdma(void);
static INLINE void init_mailbox(void);
static INLINE void clear_share_buffer(void);
static INLINE void clear_cdm(void);
static INLINE void board_init(void);
static INLINE void init_dsl_hw(void);
static INLINE void init_hw(void);
#ifdef CONFIG_IFX_PPE_E5_OFFCHIP_BONDING
  static INLINE void init_bonding_hw(void);
#endif

/*
 *  PP32 specific functions
 */
static INLINE int pp32_download_code(int, const u32 *, unsigned int,
		const u32 *, unsigned int);
static INLINE int pp32_start(int, int);
static INLINE void pp32_stop(int);

/*
 *  Init & clean-up functions
 */
static INLINE int init_local_variables(void);
static INLINE void clear_local_variables(void);
static INLINE void init_ptm_tables(void);
static INLINE void init_ptm_bonding_tables(void);
static INLINE void init_qos(void);
static INLINE void init_qos_queue(void);
static INLINE void init_communication_data_structures(int);
static INLINE int alloc_dma(void);


#ifdef CONFIG_IFX_PPE_E5_OFFCHIP_BONDING
static INLINE int alloc_bonding_dma(void);
#endif

static INLINE void free_dma(void);

/*
 *  local implement memcpy
 */
#if defined(ENABLE_MY_MEMCPY) && ENABLE_MY_MEMCPY
static INLINE void *my_memcpy(unsigned char *, const unsigned char *, unsigned int);
#else
#define my_memcpy             memcpy
#endif

/*
 *  Print Firmware Version ID
 */
static int print_fw_ver(char *, int);
static int print_driver_ver(char *, int, char *, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int);

/*
 *  DSL Status Script Notification
 */

/*
 *  Proc File
 */
#ifdef CONFIG_IFX_PPE_E5_OFFCHIP_BONDING
static void proc_dev_output(struct seq_file *m, void *private_data __maybe_unused);
static int proc_dev_input(char *line, void *data __maybe_unused);
#endif
#if defined(DEBUG_BONDING_PROC) && DEBUG_BONDING_PROC
static void proc_bonding_output(struct seq_file *, void *data);
static int proc_bonding_input(char *line, void *data __maybe_unused);
#endif

static int dma_alignment_ptm_good_count = 0;
static int dma_alignment_ptm_bad_count = 0;
static int dma_alignment_ethwan_qos_good_count = 0;
static int dma_alignment_ethwan_qos_bad_count = 0;
static int dma_alignment_eth_good_count = 0;
static int dma_alignment_eth_bad_count = 0;
static int padded_frame_count = 0;

/*
 *  Debug functions
 */
#if defined(DEBUG_DUMP_INIT) && DEBUG_DUMP_INIT
static INLINE void dump_init(void);
#else
#define dump_init()
#endif
static int swap_mac(unsigned char *);

/*
 *  Directpath Help Functions
 */

/*
 *  Local MAC Address Tracking Functions
 */
static INLINE void register_netdev_event_handler(void);
static INLINE void unregister_netdev_event_handler(void);
static int netdev_event_handler(struct notifier_block *, unsigned long, void *);

/*
 *  Global Functions
 */
static int dsl_showtime_enter(struct port_cell_info *, void *);
static int dsl_showtime_exit(void);


/*
 *  External Functions
 */
//extern int IFX_VR9_Switch_PCE_Micro_Code_Int(void);
#if defined(CONFIG_IFXMIPS_DSL_CPE_MEI) || defined(CONFIG_IFXMIPS_DSL_CPE_MEI_MODULE)
#if !defined(ENABLE_LED_FRAMEWORK) || !ENABLE_LED_FRAMEWORK
/* das ist bei uns aktuell der fall! */
extern int ifx_mei_atm_led_blink(void) __attribute__ ((weak));
#endif
extern int ifx_mei_atm_showtime_check(int *is_showtime,
		struct port_cell_info *port_cell, void **xdata_addr) __attribute__ ((weak));
#else
#if !defined(ENABLE_LED_FRAMEWORK) || !ENABLE_LED_FRAMEWORK
static inline int ifx_mei_atm_led_blink(void) {return IFX_SUCCESS;}
#endif
static inline int ifx_mei_atm_showtime_check(int *is_showtime, struct port_cell_info *port_cell, void **xdata_addr)
{
	if ( is_showtime != NULL )
	*is_showtime = 0;
	return IFX_SUCCESS;
}
#endif


#if defined( AVM_NET_TRACE_DUMP ) && AVM_NET_TRACE_DUMP && defined(CONFIG_AVM_NET_TRACE)
#if __has_include(<avm/net_trace/net_trace.h>)
#include <avm/net_trace/net_trace.h>
#else
#include <linux/avm_net_trace.h>
#endif
#if __has_include(<avm/net_trace/ioctl.h>)
#include <avm/net_trace/ioctl.h>
#else
#include <linux/avm_net_trace_ioctl.h>
#endif

struct avm_net_trace_device ifx_ant_e5 = {
	.name = "ifx_e5_dma2",
	.minor = AVM_NET_TRACE_DYNAMIC_MINOR,
	.iface = 0,
	.type = 1,
	//.pcap_encap = PCAP_ENCAP_ETHER,
	.pcap_encap = 0,
	.pcap_protocol = 0,
};

#endif
/*
 *  External Variables
 */
#if defined(CONFIG_IFXMIPS_DSL_CPE_MEI) || defined(CONFIG_IFXMIPS_DSL_CPE_MEI_MODULE)
/* das ist bei uns der Fall! */
extern int (*ifx_mei_atm_showtime_enter)(struct port_cell_info *, void *) __attribute__ ((weak));
extern int (*ifx_mei_atm_showtime_exit)(void) __attribute__ ((weak));
#else
int (*ifx_mei_atm_showtime_enter)(struct port_cell_info *, void *) = NULL;
EXPORT_SYMBOL(ifx_mei_atm_showtime_enter);
int (*ifx_mei_atm_showtime_exit)(void) = NULL;
EXPORT_SYMBOL(ifx_mei_atm_showtime_exit);
#endif

/*
 * ####################################
 *            Local Variable
 * ####################################
 */
#ifdef CONFIG_IFX_PPE_E5_OFFCHIP_BONDING
  extern unsigned int g_slave_ppe_base_addr; // avm: is defined in 7port driver
  unsigned int g_pci_dev_ppe_addr_off;
//  EXPORT_SYMBOL(g_pci_dev_ppe_addr_off);
  extern unsigned int g_ppe_base_addr; //avm: is defined in 7port driver
  static unsigned int g_bm_addr;
  static unsigned int g_pci_host_ddr_addr;
  static unsigned int g_bm_ppe_int_addr;
#endif

static int g_fwcode = FWCODE_ROUTING_ACC_D5;
static struct fw_ver_id g_fw_ver[2] = { { 0 } };

static struct semaphore g_sem; //  lock used by open/close function

static int g_eth_wan_mode = 0;

static unsigned int g_wan_itf = 1 << 7;

static int g_ipv6_acc_en = 1;

static int g_wanqos_en = 0;

static int g_dsl_bonding = 0;

static int g_queue_gamma_map[4];

static int g_cpu_to_wan_tx_desc_pos = 0;
static DEFINE_SPINLOCK(g_cpu_to_wan_tx_spinlock);
static int g_cpu_to_wan_swap_desc_pos = 0;

static u32 g_mailbox_signal_mask;
static DEFINE_SPINLOCK(g_mailbox_signal_spinlock);

static int g_f_dma_opened = 0;

static volatile int g_showtime = 0;
static void *g_xdata_addr = NULL;

static int g_line_showtime[LINE_NUMBER] = {0};
static unsigned int g_datarate_us[LINE_NUMBER * BEARER_CHANNEL_PER_LINE] = {0};
static unsigned int g_datarate_ds[LINE_NUMBER * BEARER_CHANNEL_PER_LINE] = {0};
static unsigned int g_max_delay[LINE_NUMBER] = {0};

static unsigned int g_dsl_ig_fast_brg_bytes     = 0;
static unsigned int g_dsl_ig_fast_rt_ipv4_bytes = 0;
static unsigned int g_dsl_ig_fast_rt_ipv6_bytes = 0;
static unsigned int g_dsl_ig_cpu_bytes          = 0;
static unsigned int g_dsl_eg_fast_pkts          = 0;
#if defined(ENABLE_LED_FRAMEWORK) && ENABLE_LED_FRAMEWORK
static void *g_data_led_trigger = NULL;
#endif

static struct net_device *g_ptm_net_dev[1]= {0};

static int g_ptm_prio_queue_map[MAX_WAN_QOS_QUEUES];

static DEFINE_SPINLOCK(g_eth_tx_spinlock);

#if !defined (AVM_SETUP_MAC)
static u8 g_my_ethaddr[MAX_ADDR_LEN * 2] = {0};
#endif



//	1:switch forward packets between port0/1 and cpu port directly w/o processing
//	0:pkts go through normal path and are processed by switch central function
static int g_ingress_direct_forward_enable = 0;
static int g_egress_direct_forward_enable = 1;

static int g_pmac_ewan = 0;

#if defined(ENABLE_NAPI) && ENABLE_NAPI
static int g_napi_enable = 1;
#endif

#ifdef CONFIG_AVM_PA
static int g_directpath_dma_full = 0;
static DEFINE_SPINLOCK(g_directpath_tx_spinlock);
#endif /* CONFIG_AVM_PA */

static struct notifier_block g_netdev_event_handler_nb = { 0 };

static unsigned int last_wrx_total_pdu[2][4] = {{0}};
static unsigned int last_wrx_crc_error_total_pdu[2][4] = {{0}};
static unsigned int last_wrx_cv_cw_cnt[2][4] = {{0}};
static unsigned int last_wrx_bc_overdrop_cnt[2][2] = {{0}};

#if defined(DEBUG_SKB_SWAP) && DEBUG_SKB_SWAP
static struct sk_buff *g_dbg_skb_swap_pool[1536 * 100] = {0};
#endif

#if defined(DEBUG_MIRROR_PROC) && DEBUG_MIRROR_PROC
static struct net_device *g_mirror_netdev = NULL;
#endif

#if defined(DEBUG_BONDING_PROC) && DEBUG_BONDING_PROC
  static struct rx_descriptor saved_us_des_array[512 + 64];
  static struct rx_descriptor us_des[512 + 64];

  static struct rx_descriptor saved_ds_des_array[64 + 64 + 256];    //  DMA_TX_CH1_DESC_NUM + E1_FRAG_RX_DESC_NUM * 2 + B1_RX_LINK_LIST_DESC_NUM];  // DMA_TX_CH1, + E1 rx  + Linklist
  static struct rx_descriptor des[64 + 64 + 256];                   //  DMA_TX_CH1_DESC_NUM + E1_FRAG_RX_DESC_NUM * 2 + B1_RX_LINK_LIST_DESC_NUM];  // DMA_TX_CH1, + E1 rx  + Linklist
#endif

/*
 * ####################################
 *           Global Variable
 * ####################################
 */

/*
 * ####################################
 *            Local Function
 * ####################################
 */
#define PP32_REG_ADDR_BEGIN     0x0
#define PP32_REG_ADDR_END       0x1FFF
#define PP32_SB_ADDR_BEGIN      0x2000
#define PP32_SB_ADDR_END        0xFFFF

static inline unsigned long sb_addr_to_fpi_addr_convert(unsigned long sb_addr) {
	if (sb_addr <= PP32_SB_ADDR_END) {
		return (unsigned long) SB_BUFFER_BOND(2, sb_addr);
	} else {
		return sb_addr;
	}
}

/*
 * ####################################
 *        Common Datapath Functions
 * ####################################
 */
#include "../common/ifxmips_ppa_datapath_common.h"
int e5_in_avm_ata_mode(void) {
	// printk(KERN_ERR "[%s]: %s\n", __FUNCTION__, (g_eth_wan_mode == 3 )?"ATA":"NO ATA");
	return (g_eth_wan_mode == 3);
}

int ifx_ppa_ptm_init(struct net_device *dev) {
	char *env;

	ether_setup(dev); /*  assign some members */

	dbg("hard_header_len before: %d", dev->hard_header_len);
	dev->hard_header_len += 4;
	dbg("hard_header_len changed: %d", dev->hard_header_len);
	printk(KERN_ERR "CPU_TO_WAN_TX_DESC_BASE[0] =%#x\n", (unsigned int)CPU_TO_WAN_TX_DESC_BASE);

	/*  hook network operations */

	env = prom_getenv("macdsl");
	if (env) {
		unsigned int a[6];
		sscanf(env, "%x:%x:%x:%x:%x:%x", &a[0], &a[1], &a[2], &a[3], &a[4],
				&a[5]);
		dev->dev_addr[0] = a[0];
		dev->dev_addr[1] = a[1];
		dev->dev_addr[2] = a[2];
		dev->dev_addr[3] = a[3];
		dev->dev_addr[4] = a[4];
		dev->dev_addr[5] = a[5];
		dbg("use urlader mac %2x:%2x:%2x:%2x:%2x:%2x",
				(unsigned int) dev->dev_addr[0],
				(unsigned int) dev->dev_addr[1],
				(unsigned int) dev->dev_addr[2],
				(unsigned int) dev->dev_addr[3],
				(unsigned int) dev->dev_addr[4],
				(unsigned int) dev->dev_addr[5]);
	} else {
		dev->dev_addr[0] = 0x00;
		dev->dev_addr[1] = 0x20;
		dev->dev_addr[2] = 0xda;
		dev->dev_addr[3] = 0x86;
		dev->dev_addr[4] = 0x23;
		dev->dev_addr[5] = 0xee;
		err("WARNING - NO URLADER MAC! - use default mac %2x:%2x:%2x:%2x:%2x:%2x ",
				(unsigned int) dev->dev_addr[0],
				(unsigned int) dev->dev_addr[1],
				(unsigned int) dev->dev_addr[2],
				(unsigned int) dev->dev_addr[3],
				(unsigned int) dev->dev_addr[4],
				(unsigned int) dev->dev_addr[5]);
	}
	return 0;
}

static int ifx_ppa_ptm_open(struct net_device *dev) {
	avmnet_netdev_priv_t *priv = netdev_priv(dev);
	dbg(KERN_ERR "[%s] dev=%p, name=%s, priv=%p\n",
			__FUNCTION__, dev, dev->name, priv);

	if (g_eth_wan_mode)
		return -EIO;

#if defined(CONFIG_IP_MULTICAST_FASTFORWARD)
	avmnet_mcfw_enable(dev);
#endif /*--- #if defined(CONFIG_IP_MULTICAST_FASTFORWARD) ---*/

	down(&g_sem);
	netif_start_queue(dev);

	dbg(KERN_ERR "[%s] napi_enable\n", __FUNCTION__);
	napi_enable(&priv->avm_dev->napi);
	// turn_on_dma_rx(7); //DMA lauft durch
	up(&g_sem);

	return 0;
}

static int ifx_ppa_ptm_stop(struct net_device *dev) {
	avmnet_netdev_priv_t *priv = netdev_priv(dev);

	down(&g_sem);
	// turn_off_dma_rx(7); //DMA lauft durch


	netif_stop_queue(dev);

	dbg(KERN_ERR "[%s] napi_disbale\n", __FUNCTION__);
	napi_disable(&priv->avm_dev->napi);
	up(&g_sem);
	// turn_off_dma_rx(7); //DMA lauft durch

#if defined(CONFIG_IP_MULTICAST_FASTFORWARD)
	avmnet_mcfw_disable(dev);
#endif /*--- #if defined(CONFIG_IP_MULTICAST_FASTFORWARD) ---*/

	return 0;
}

static int ifx_ppa_ptm_hard_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	unsigned long sys_flag;
	unsigned int f_full;
	int desc_base;
	volatile struct tx_descriptor *desc;
	struct tx_descriptor reg_desc = { 0 };
	struct sk_buff *skb_to_free;
	unsigned int byteoff;
	int cache_wback_len;
	avmnet_netdev_priv_t *priv = netdev_priv(dev);
	avmnet_device_t *avm_dev = priv->avm_dev;

	if (g_stop_datapath != 0)
		goto PTM_HARD_START_XMIT_FAIL;

	if (!g_showtime) {
		err("not in showtime");
		goto PTM_HARD_START_XMIT_FAIL;
	}

	/*  allocate descriptor */
	desc_base = -1;
	f_full = 1;
	spin_lock_irqsave(&g_cpu_to_wan_tx_spinlock, sys_flag);
	if (CPU_TO_WAN_TX_DESC_BASE[g_cpu_to_wan_tx_desc_pos].own == 0) {
		desc_base = g_cpu_to_wan_tx_desc_pos;
		if (++g_cpu_to_wan_tx_desc_pos == CPU_TO_WAN_TX_DESC_NUM)
			g_cpu_to_wan_tx_desc_pos = 0;
		if (CPU_TO_WAN_TX_DESC_BASE[g_cpu_to_wan_tx_desc_pos].own == 0)
			f_full = 0;
	}
	if (f_full) {
		dev->trans_start = jiffies;
		netif_stop_queue(dev);
		AVMNET_DBG_TX_QUEUE("full, stop device %s", dev->name);


		if (priv->lantiq_ppe_hold_timeout == 0){
			priv->lantiq_ppe_hold_timeout = jiffies + (PPE_HOLD_TIMEOUT * HZ);
		} else if ( time_after(jiffies, priv->lantiq_ppe_hold_timeout )){
			err("PPE hold timeout reached");
			BUG();
		}

		spin_lock(&g_mailbox_signal_spinlock);
		err("PPE hold, now=%lu, timeout=%lu", jiffies, priv->lantiq_ppe_hold_timeout );
		SW_WRITE_REG32_MASK(0, CPU_TO_WAN_TX_SIG, MBOX_IGU1_ISRC);
		g_mailbox_signal_mask |= CPU_TO_WAN_TX_SIG;
		SW_WRITE_REG32(g_mailbox_signal_mask, MBOX_IGU1_IER);
		spin_unlock(&g_mailbox_signal_spinlock);
	} else {
		priv->lantiq_ppe_hold_timeout = 0;
	}
	spin_unlock_irqrestore(&g_cpu_to_wan_tx_spinlock, sys_flag);
	if (desc_base < 0)
		goto PTM_HARD_START_XMIT_FAIL;
	desc = &CPU_TO_WAN_TX_DESC_BASE[desc_base];

	byteoff = (unsigned int) skb->data & (DMA_TX_ALIGNMENT - 1);
	if (skb_headroom(skb) < sizeof(struct sk_buff *) + byteoff || skb_cloned(skb) || (byteoff > 31)) {

		struct sk_buff *new_skb;

		dma_alignment_ptm_bad_count++;

		ASSERT (byteoff < 32, "byteoff zu gross %d ", byteoff);
		ASSERT(skb_headroom(skb) >= sizeof(struct sk_buff *) + byteoff,
		       "   skb_headroom < sizeof(struct sk_buff *) +  byteoff \n");

#if defined(CONFIG_AVMNET_DEBUG) 
		if ( g_dbg_datapath & DBG_ENABLE_MASK_DEBUG_ALIGNMENT )
			printk( KERN_ERR
				"\n"
				"   skb_headroom :   %d \n"
				"   sizeof(struct sk_buff *): %d \n"
				"   byteoff:         %d \n"
				"   hard_header_len: %d\n"
				"   skb->head:       %p\n"
				"   skb->data:       %p\n"
				"   in_dev:          %s\n",
				skb_headroom(skb),
				sizeof(struct sk_buff *),
				byteoff,
				dev->hard_header_len,
				skb->head,
				skb->data,
				skb->input_dev?skb->input_dev->name:"no_input_device");
#endif

		ASSERT(!skb_cloned(skb), "skb is cloned");

		new_skb = alloc_skb_tx(DMA_PACKET_SIZE);
		if (new_skb == NULL) {
			dbg("no memory");
			goto ALLOC_SKB_TX_FAIL;
		}
		skb_put(new_skb, skb->len);
		memcpy(new_skb->data, skb->data, skb->len);

		/* AVMGRM: priority wegen QoS auch kopieren */
		new_skb->priority = skb->priority;

		dev_kfree_skb_any(skb);
		skb = new_skb;
		byteoff = (unsigned int) skb->data & (DMA_TX_ALIGNMENT - 1);
		/*  write back to physical memory   */
		dma_cache_wback((unsigned long)skb->data, skb->len);
	} else {
		dma_alignment_ptm_good_count++;
		skb_orphan(skb);
	}

	/*
	 * AVM/CBU: JZ-34572 Zero-padding for small packets
	 */
	if (skb->len < ETH_MIN_TX_PACKET_LENGTH ){
		if ( skb_pad(skb, ETH_MIN_TX_PACKET_LENGTH - skb->len )){
			goto PTM_HARD_START_PAD_FAIL;
		}
		skb->len = ETH_MIN_TX_PACKET_LENGTH;
	}

	*(struct sk_buff **) ((unsigned int) skb->data - byteoff
			      - sizeof(struct sk_buff *)) = skb;
	/*  write back to physical memory   */
	dma_cache_wback(
			(unsigned long)skb->data - byteoff - sizeof(struct sk_buff *),
			skb->len + byteoff + sizeof(struct sk_buff *));

	/*  free previous skb   */
	free_skb_clear_dataptr(desc);

	/*  detach from protocol    */
	skb_to_free = skb;
	if( (skb = skb_break_away_from_protocol_avm(skb)) == NULL) {
		skb = skb_to_free;
		goto ALLOC_SKB_TX_FAIL;
	}

	put_skb_to_dbg_pool(skb);

#if defined(DEBUG_MIRROR_PROC) && DEBUG_MIRROR_PROC
	if (g_mirror_netdev != NULL) {
		struct sk_buff *new_skb = skb_copy(skb, GFP_ATOMIC);

		if (new_skb != NULL) {
			new_skb->dev = g_mirror_netdev;
			dev_queue_xmit(new_skb);
		}
	}
#endif

	/*  update descriptor   */
	reg_desc.small =
		(unsigned int) skb->end - (unsigned int) skb->data < DMA_PACKET_SIZE ?
		1 : 0;
	reg_desc.dataptr = (unsigned int) skb->data
		& (0x1FFFFFFF ^ (DMA_TX_ALIGNMENT - 1));
	BUG_ON(skb->len < ETH_ZLEN);
	reg_desc.datalen = skb->len;
	// reg_desc.qid     = qid; //AVMCCB: UGW54 vs. AVM-Concept
	reg_desc.qid = avm_get_ptm_qid(skb->priority);
	reg_desc.byteoff = byteoff;
	reg_desc.own = 1;
	reg_desc.c = 1;
	reg_desc.sop = reg_desc.eop = 1;
	ASSERT (byteoff < 32, "byteoff immernoch zu gross %d", byteoff);

	/*  update MIB  */
	avm_dev->device_stats.tx_packets++;
	avm_dev->device_stats.tx_bytes += reg_desc.datalen;

#if defined(DEBUG_QID)
	{
		static unsigned last_qid = ~0;
		static unsigned qid_count = ~0;
		if (last_qid == reg_desc.qid){
			qid_count++;
		}
		else {
			if (qid_count)
				printk(KERN_ERR "[debug_qid] qid %d was repeated for %d times\n", last_qid, qid_count);
			printk(KERN_ERR "[debug_qid] new qid %d \n", reg_desc.qid);
			qid_count = 0;
			last_qid = reg_desc.qid;
		}


	}
#endif

	dump_skb(skb, DUMP_SKB_LEN, (char *) __func__, 0, reg_desc.qid, 1, 0);

	if ( (cache_wback_len = swap_mac(skb->data)) )
		dma_cache_wback((unsigned long)skb->data, cache_wback_len);


	/*  write discriptor to memory  */
	*((volatile unsigned int *) desc + 1) = *((unsigned int *) &reg_desc + 1);
	wmb();
	*(volatile unsigned int *) desc = *(unsigned int *) &reg_desc;
	// dbg("tx_desc =%#x", (unsigned int)desc);

	dev->trans_start = jiffies;

	return NETDEV_TX_OK;

ALLOC_SKB_TX_FAIL:
PTM_HARD_START_XMIT_FAIL:
	dev_kfree_skb_any(skb);
PTM_HARD_START_PAD_FAIL:
	if (avm_dev)
		avm_dev->device_stats.tx_dropped++;
	return NETDEV_TX_OK;
}

static int ifx_ppa_ptm_hard_start_xmit_b(struct sk_buff *skb, struct net_device *dev __attribute__ ((unused)))
{
    int qid;

    if ( skb->cb[13] == 0x5A )  //  magic number indicating forcing QId (e.g. directpath)
        qid = skb->cb[15];
    else
        qid = avm_get_ptm_qid( skb->priority );

    swap_mac(skb->data);
    eth_xmit(skb, 7 /* DSL */, 2, 2, g_wan_queue_class_map[qid]);
    return 0;
}

static int ifx_ppa_ptm_ioctl(struct net_device *dev __attribute__ ((unused)), struct ifreq *ifr, int fcmd)
{
    switch ( fcmd )
    {
	case IFX_PTM_MIB_CW_GET:
        ((PTM_CW_IF_ENTRY_T *)ifr->ifr_data)->ifRxNoIdleCodewords   = IFX_REG_R32(DREG_AR_CELL0) + IFX_REG_R32(DREG_AR_CELL1);
        ((PTM_CW_IF_ENTRY_T *)ifr->ifr_data)->ifRxIdleCodewords     = IFX_REG_R32(DREG_AR_IDLE_CNT0) + IFX_REG_R32(DREG_AR_IDLE_CNT1);
        ((PTM_CW_IF_ENTRY_T *)ifr->ifr_data)->ifRxCodingViolation   = IFX_REG_R32(DREG_AR_CVN_CNT0) + IFX_REG_R32(DREG_AR_CVN_CNT1) + IFX_REG_R32(DREG_AR_CVNP_CNT0) + IFX_REG_R32(DREG_AR_CVNP_CNT1);
        ((PTM_CW_IF_ENTRY_T *)ifr->ifr_data)->ifTxNoIdleCodewords   = IFX_REG_R32(DREG_AT_CELL0) + IFX_REG_R32(DREG_AT_CELL1);
        ((PTM_CW_IF_ENTRY_T *)ifr->ifr_data)->ifTxIdleCodewords     = IFX_REG_R32(DREG_AT_IDLE_CNT0) + IFX_REG_R32(DREG_AT_IDLE_CNT1);
		break;
    case IFX_PTM_MIB_FRAME_GET:
        {
		PTM_FRAME_MIB_T data = { 0 };
		int i;

            data.RxCorrect = IFX_REG_R32(DREG_AR_HEC_CNT0) + IFX_REG_R32(DREG_AR_HEC_CNT1) + IFX_REG_R32(DREG_AR_AIIDLE_CNT0) + IFX_REG_R32(DREG_AR_AIIDLE_CNT1);
		for (i = 0; i < 4; i++)
			data.RxDropped += WAN_RX_MIB_TABLE(i)->wrx_dropdes_pdu;
		for (i = 0; i < MAX_WAN_QOS_QUEUES; i++)
			data.TxSend += WAN_TX_MIB_TABLE(i)->wtx_total_pdu;

		*((PTM_FRAME_MIB_T *) ifr->ifr_data) = data;
	}
		break;
	case IFX_PTM_CFG_GET:
		//  use bear channel 0 preemption gamma interface settings
		((IFX_PTM_CFG_T *) ifr->ifr_data)->RxEthCrcPresent = 1;
        ((IFX_PTM_CFG_T *)ifr->ifr_data)->RxEthCrcCheck   = RX_GAMMA_ITF_CFG(0)->rx_eth_fcs_ver_dis == 0 ? 1 : 0;
        ((IFX_PTM_CFG_T *)ifr->ifr_data)->RxTcCrcCheck    = RX_GAMMA_ITF_CFG(0)->rx_tc_crc_ver_dis == 0 ? 1 : 0;;
        ((IFX_PTM_CFG_T *)ifr->ifr_data)->RxTcCrcLen      = RX_GAMMA_ITF_CFG(0)->rx_tc_crc_size == 0 ? 0 : (RX_GAMMA_ITF_CFG(0)->rx_tc_crc_size * 16);
        ((IFX_PTM_CFG_T *)ifr->ifr_data)->TxEthCrcGen     = TX_GAMMA_ITF_CFG(0)->tx_eth_fcs_gen_dis == 0 ? 1 : 0;
        ((IFX_PTM_CFG_T *)ifr->ifr_data)->TxTcCrcGen      = TX_GAMMA_ITF_CFG(0)->tx_tc_crc_size == 0 ? 0 : 1;
        ((IFX_PTM_CFG_T *)ifr->ifr_data)->TxTcCrcLen      = TX_GAMMA_ITF_CFG(0)->tx_tc_crc_size == 0 ? 0 : (TX_GAMMA_ITF_CFG(0)->tx_tc_crc_size * 16);
		break;
    case IFX_PTM_CFG_SET:
        {
		int i;

		for (i = 0; i < 4; i++) {
                RX_GAMMA_ITF_CFG(i)->rx_eth_fcs_ver_dis = ((IFX_PTM_CFG_T *)ifr->ifr_data)->RxEthCrcCheck ? 0 : 1;

                RX_GAMMA_ITF_CFG(0)->rx_tc_crc_ver_dis = ((IFX_PTM_CFG_T *)ifr->ifr_data)->RxTcCrcCheck ? 0 : 1;

			switch (((IFX_PTM_CFG_T *) ifr->ifr_data)->RxTcCrcLen) {
                    case 16: RX_GAMMA_ITF_CFG(0)->rx_tc_crc_size = 1; break;
                    case 32: RX_GAMMA_ITF_CFG(0)->rx_tc_crc_size = 2; break;
                    default: RX_GAMMA_ITF_CFG(0)->rx_tc_crc_size = 0;
			}

                TX_GAMMA_ITF_CFG(0)->tx_eth_fcs_gen_dis = ((IFX_PTM_CFG_T *)ifr->ifr_data)->TxEthCrcGen ? 0 : 1;

			if (((IFX_PTM_CFG_T *) ifr->ifr_data)->TxTcCrcGen) {
				switch (((IFX_PTM_CFG_T *) ifr->ifr_data)->TxTcCrcLen) {
                        case 16: TX_GAMMA_ITF_CFG(0)->tx_tc_crc_size = 1; break;
                        case 32: TX_GAMMA_ITF_CFG(0)->tx_tc_crc_size = 2; break;
                        default: TX_GAMMA_ITF_CFG(0)->tx_tc_crc_size = 0;
				}
                }
                else
				TX_GAMMA_ITF_CFG(0)->tx_tc_crc_size = 0;
		}
	}
		break;
    case IFX_PTM_MAP_PKT_PRIO_TO_Q:
        {
		struct ppe_prio_q_map cmd;

		if (copy_from_user(&cmd, ifr->ifr_data, sizeof(cmd)))
			return -EFAULT;

            if (( cmd.pkt_prio < 0 ) || (cmd.pkt_prio >= (int)NUM_ENTITY(g_ptm_prio_queue_map)))
                return -EINVAL;

		if (cmd.qid < 0 || cmd.qid >= g_wanqos_en)
			return -EINVAL;

		g_ptm_prio_queue_map[cmd.pkt_prio] = cmd.qid;
	}
		break;
	default:
		return -EOPNOTSUPP;
	}

	return 0;
}

static void ifx_ppa_ptm_tx_timeout(struct net_device *dev)
{
	unsigned long sys_flag;

	/*  disable TX irq, release skb when sending new packet */
    spin_lock_irqsave(&g_mailbox_signal_spinlock, sys_flag);
	g_mailbox_signal_mask &= ~CPU_TO_WAN_TX_SIG;
    IFX_REG_W32(g_mailbox_signal_mask, MBOX_IGU1_IER);
    spin_unlock_irqrestore(&g_mailbox_signal_spinlock, sys_flag);

	/*  wake up TX queue    */
	netif_wake_queue(dev);

	return;
}


int ifx_ppa_eth_hard_start_xmit_e5(struct sk_buff *skb, struct net_device *dev) {
	int port;
	int qid;

	dbg_dma("[%s] s\n", __FUNCTION__);
	port = get_eth_port(dev);
	if (port < 0){
		return -ENODEV;
	}
    qid = avm_get_eth_qid(port, skb->priority);

	eth_xmit(skb, port, DMA_CHAN_NR_DEFAULT_CPU_TX , 2, g_wan_queue_class_map[qid]); //  spid - CPU, qid - taken from prio_queue_map
	dbg_dma("[%s] e\n", __FUNCTION__);
	return 0;
}


static int ifx_ppa_eth_qos_hard_start_xmit_e5(struct sk_buff *skb, struct net_device *dev) {
	int dpid;
	int qid;
	unsigned long sys_flag;
	volatile struct tx_descriptor *desc;
	struct tx_descriptor reg_desc;
	struct sk_buff *skb_to_free;
	int byteoff;
	int len;
	struct sw_eg_pkt_header pkth = {0};

	avmnet_netdev_priv_t *priv = netdev_priv(dev);
	avmnet_device_t *avm_dev = priv->avm_dev;

	if ( g_stop_datapath != 0 )
		goto ETH_XMIT_DROP;

	if (skb->ip_summed == CHECKSUM_PARTIAL){
		err("Dropping SG Packet in ETHWAN Path");
		goto ETH_XMIT_DROP;
	}

	/*
	 * AVM/CBU: JZ-34572 Zero-padding for small packets
	 */
	if (skb->len < ETH_MIN_TX_PACKET_LENGTH ){
		if ( skb_pad(skb, ETH_MIN_TX_PACKET_LENGTH - skb->len )){
			goto ETH_PAD_DROP;
		}
		skb->len = ETH_MIN_TX_PACKET_LENGTH;
	}

	dpid = get_eth_port(dev);
	if ( dpid < 0 )
		return -ENODEV;

	if ( skb->cb[13] == 0x5A ) {
		// magic number indicating forcing QId (e.g. directpath)
		qid = skb->cb[15];
	} else {
		qid = avm_get_eth_qid( dpid, skb->priority );
	}

#if 0
	// AVMCCB UGW54 vs AVM-Concept
	pkth.spid           = 2;    //  CPU
	//    pkth.dpid           = dpid; //  eth0/eth1
	pkth.dpid           = 1; //  eth0/eth1
	pkth.lrn_dis        = 0;

	pkth.class_en       = PS_MC_GENCFG3->class_en;
	pkth.class          = g_wan_queue_class_map[qid];
	if ( pkth.dpid < 2 )
		pkth.dpid_en    = g_egress_direct_forward_enable;
#else

	// for cpu -> ethernet
	pkth.spid = 2; //  CPU
	pkth.dpid_en = 1;
	pkth.dpid = 1; // 0 or 1 for PORT_MAP_MODE
	pkth.port_map_en = 1;
	pkth.port_map_sel = 1;
	pkth.port_map = (1 << if_id_to_mac_nr(dpid));
	pkth.lrn_dis = 0;
	pkth.class_en = 0;
	pkth.class = 0;
	dbg_dma("port_map: %#x", (unsigned int)pkth.port_map);

#endif
	BUG_ON(skb->len < ETH_MIN_TX_PACKET_LENGTH);
	len = skb->len;

	dump_skb(skb, DUMP_SKB_LEN, __func__, dpid, qid, 1, 0);

	/*  reserve space to put pointer in skb */
	byteoff = (((unsigned int)skb->data - sizeof(pkth)) & (DMA_TX_ALIGNMENT - 1)) + sizeof(pkth);
	//if ( skb_headroom(skb) < sizeof(struct sk_buff *) + byteoff || skb->end - skb->data < 1564 /* 1518 (max ETH frame) + 22 (reserved for firmware) + 10 (AAL5) + 6 (PPPoE) + 4 x 2 (2 VLAN) */ || skb_shared(skb) || skb_cloned(skb) )

	if ( skb_headroom(skb) < sizeof(struct sk_buff *) + byteoff || skb_shared(skb) || skb_cloned(skb) ||
	     (unsigned int)skb->end - (unsigned int)skb->data < DMA_PACKET_SIZE  )
	{
		struct sk_buff *new_skb;

		dma_alignment_ethwan_qos_bad_count++;

		dbg_dma("realloc skb");
		ASSERT(skb_headroom(skb) >= sizeof(struct sk_buff *) + byteoff, "skb_headroom(skb) < sizeof(struct sk_buff *) + byteoff");
		new_skb = alloc_skb_tx(sizeof(pkth) + skb->len < DMA_PACKET_SIZE ? DMA_PACKET_SIZE : sizeof(pkth) + skb->len);  //  may be used by RX fastpath later
		if ( !new_skb )
		{
			err("no memory");
			goto ALLOC_SKB_TX_FAIL;
		}
		skb_put(new_skb, sizeof(pkth) + skb->len);
		memcpy(new_skb->data, &pkth, sizeof(pkth));
		memcpy(new_skb->data + sizeof(pkth), skb->data, skb->len);

		new_skb->priority = skb->priority;

		dev_kfree_skb_any(skb);
		skb = new_skb;
		byteoff = (u32)skb->data & (DMA_TX_ALIGNMENT - 1);

		dump_skb(skb, DUMP_SKB_LEN, "realloc", dpid, qid, 1, 0);
#ifndef CONFIG_MIPS_UNCACHED
		/*  write back to physical memory   */
		dma_cache_wback((u32)skb->data, skb->len);
#endif
	}
	else
	{

		dma_alignment_ethwan_qos_good_count++;
		skb_push(skb, sizeof(pkth));
		byteoff -= sizeof(pkth);
		memcpy(skb->data, &pkth, sizeof(pkth));
		*(struct sk_buff **)((u32)skb->data - byteoff - sizeof(struct sk_buff *)) = skb;
#ifndef CONFIG_MIPS_UNCACHED
		/*  write back to physical memory   */
		dma_cache_wback((u32)skb->data - byteoff - sizeof(struct sk_buff *), skb->len + byteoff + sizeof(struct sk_buff *));
#endif
	}

	/*  allocate descriptor */
	spin_lock_irqsave(&g_cpu_to_wan_tx_spinlock, sys_flag);
	desc = (volatile struct tx_descriptor *)(CPU_TO_WAN_TX_DESC_BASE + g_cpu_to_wan_tx_desc_pos);
	if ( desc->own )    //  PPE hold
	{
		spin_unlock_irqrestore(&g_cpu_to_wan_tx_spinlock, sys_flag);
		err("PPE hold");
		goto NO_FREE_DESC;
	}
	if ( ++g_cpu_to_wan_tx_desc_pos == CPU_TO_WAN_TX_DESC_NUM )
		g_cpu_to_wan_tx_desc_pos = 0;
	spin_unlock_irqrestore(&g_cpu_to_wan_tx_spinlock, sys_flag);

	/*  load descriptor from memory */
	reg_desc = *desc;

	/*  free previous skb   */
	ASSERT((reg_desc.dataptr & 31) == 0, "reg_desc.dataptr (0x%#x) must be 8 DWORDS aligned", reg_desc.dataptr);
	free_skb_clear_dataptr(&reg_desc);

	/*  detach from protocol    */
	skb_to_free = skb;
	if( (skb = skb_break_away_from_protocol_avm(skb)) == NULL) {
		skb = skb_to_free;
		goto ALLOC_SKB_TX_FAIL;
	}

	put_skb_to_dbg_pool(skb);

#if defined(DEBUG_MIRROR_PROC) && DEBUG_MIRROR_PROC
	if ( g_mirror_netdev != NULL )
	{
		struct sk_buff *new_skb = skb_copy(skb, GFP_ATOMIC);

		if ( new_skb != NULL )
		{
			skb_pull(new_skb, sizeof(pkth));
			new_skb->dev = g_mirror_netdev;
			dev_queue_xmit(new_skb);
		}
	}
#endif

	/*  update descriptor   */
	reg_desc.small      = (unsigned int)skb->end - (unsigned int)skb->data < DMA_PACKET_SIZE ? 1 : 0;
	reg_desc.dataptr    = (u32)skb->data & (0x1FFFFFFF ^ (DMA_TX_ALIGNMENT - 1));  //  byte address
	reg_desc.byteoff    = byteoff;
	reg_desc.datalen    = len + sizeof(pkth);
	reg_desc.qid        = qid;
	reg_desc.own        = 1;
	reg_desc.c          = 1;    //  ?
	reg_desc.sop = reg_desc.eop = 1;

	/*  update MIB  */
	dev->trans_start = jiffies;

	dbg_dma("skb->data %#x data_ptr %#x, byteoff %d\n", (u32)skb->data, reg_desc.dataptr, byteoff  );

	/*  update MIB  */
	if (avm_dev){
		avm_dev->device_stats.tx_packets++;
		avm_dev->device_stats.tx_bytes += reg_desc.datalen;
	}

	/*  write discriptor to memory and write back cache */
	*((volatile u32 *)desc + 1) = *((u32 *)&reg_desc + 1);
	*(volatile u32 *)desc = *(u32 *)&reg_desc;

	return 0;

NO_FREE_DESC:
ALLOC_SKB_TX_FAIL:
ETH_XMIT_DROP:
	dev_kfree_skb_any(skb);
ETH_PAD_DROP:
	if (avm_dev)
		avm_dev->device_stats.tx_dropped++;
	return 0;
}

int ifx_ppa_eth_ioctl_e5(struct net_device *dev, struct ifreq *ifr, int fcmd) {
	int port;

	port = get_eth_port(dev);
	if (port < 0)
		return -ENODEV;

	switch (fcmd) {
	case SIOCETHTOOL:
		return ethtool_ioctl(dev, ifr);
	case SET_VLAN_COS: {
		struct vlan_cos_req vlan_cos_req;

		if (copy_from_user(&vlan_cos_req, ifr->ifr_data, sizeof(struct vlan_cos_req)))
			return -EFAULT;
	}
		break;
	case SET_DSCP_COS: {
		struct dscp_cos_req dscp_cos_req;

		if (copy_from_user(&dscp_cos_req, ifr->ifr_data, sizeof(struct dscp_cos_req)))
			return -EFAULT;
	}
		break;
	case ETH_MAP_PKT_PRIO_TO_Q: {
		struct ppe_prio_q_map cmd;

		if (copy_from_user(&cmd, ifr->ifr_data, sizeof(cmd)))
			return -EFAULT;

		if (cmd.pkt_prio < 0 || cmd.pkt_prio >= (int) NUM_ENTITY(g_eth_prio_queue_map))
		    return -EINVAL;

		if (cmd.qid < 0)
			return -EINVAL;
            if ( cmd.qid >= ((g_wan_itf & (1 << port)) && g_wanqos_en && (g_eth_wan_mode == 3) ? __ETH_WAN_TX_QUEUE_NUM : __ETH_VIRTUAL_TX_QUEUE_NUM) )
			return -EINVAL;

		g_eth_prio_queue_map[cmd.pkt_prio] = cmd.qid;
	}
		break;
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

void ifx_ppa_eth_tx_timeout_e5(struct net_device *dev) {
	unsigned long sys_flag;
	//int port;
	avmnet_netdev_priv_t *priv = netdev_priv(dev);
	avmnet_device_t *avm_dev = priv->avm_dev;

	avm_dev->device_stats.tx_errors++;

    spin_lock_irqsave(&g_eth_tx_spinlock, sys_flag);
	g_dma_device_ppe->tx_chan[2]->disable_irq(g_dma_device_ppe->tx_chan[2]);
    AVMNET_DBG_TX_QUEUE("tx_timeout wake device %s" , dev->name);
	netif_wake_queue(g_eth_net_dev[0]);
	if (!(g_eth_wan_mode == 3 /* ETH WAN */&& g_wanqos_en))
		netif_wake_queue(g_eth_net_dev[1]);
    spin_unlock_irqrestore(&g_eth_tx_spinlock, sys_flag);

	return;
}

static INLINE int eth_xmit(struct sk_buff *skb, int dpid, int ch,
		int spid, int prio __attribute__ ((unused))) {
	struct sw_eg_pkt_header pkth = { 0 };
	avmnet_netdev_priv_t *priv = NULL;
	avmnet_device_t *avm_dev = NULL;
    struct skb_shared_info *shinfo;
    int len;

	if (g_stop_datapath != 0)
		goto ETH_XMIT_DROP;

    len = skb->len;
    if (skb->len < ETH_MIN_TX_PACKET_LENGTH ){
	    /*--- 0byte padding behind skb->len, skb->len is not increased, this function linearizes our skb ---*/
	    if ( skb_pad(skb, ETH_MIN_TX_PACKET_LENGTH - skb->len )){
		    skb = NULL;
		    goto ETH_XMIT_DROP;
	    }
	    len = ETH_MIN_TX_PACKET_LENGTH;
	    padded_frame_count++;
    }

#if defined (CONFIG_AVM_SCATTER_GATHER)
    /* If packet is not checksummed and device does not support
    * checksumming for this protocol, complete checksumming here.
    */
    if (skb->ip_summed == CHECKSUM_PARTIAL) {
        if(unlikely(skb_checksum_help(skb))){
            err(KERN_ERR "skb_checksum fails\n");
            goto ETH_XMIT_DROP;
        }
    }
#endif

#if defined(DEBUG_MIRROR_PROC) && DEBUG_MIRROR_PROC
	if (g_mirror_netdev != NULL && (g_wan_itf & (1 << dpid))) {
		struct sk_buff *new_skb = skb_copy(skb, GFP_ATOMIC);

		if (new_skb != NULL) {
			new_skb->dev = g_mirror_netdev;
			dev_queue_xmit(new_skb);
		}
	}
#endif


	dump_skb(skb, DUMP_SKB_LEN, "eth_xmit - raw data avm", dpid, ch, 1, 0);

	if ( spid == 2 ){ // don't track directpath traffic here
		if ( (dpid < (int)NUM_ENTITY(g_eth_net_dev)) || ( dpid == 7 )) {
		    struct net_device *dev = NULL;
		    if ( dpid < (int)NUM_ENTITY(g_eth_net_dev) )
		        dev = g_eth_net_dev[dpid];
		    else if (dpid == 7)
		        dev = g_ptm_net_dev[0];

		    if ( dev ){
                priv = netdev_priv(g_eth_net_dev[dpid]);
                g_eth_net_dev[dpid]->trans_start = jiffies;
                avm_dev = priv->avm_dev;
                if ( ! avm_dev)
                    goto ETH_XMIT_DROP;

                avm_dev->device_stats.tx_packets++;
                avm_dev->device_stats.tx_bytes += len;
#if defined(CONFIG_IP_MULTICAST_FASTFORWARD)
                if(likely(priv->mcfw_used)){
                    mcfw_portset set;
                    mcfw_portset_reset(&set);
                    mcfw_portset_port_add(&set, 0);
                    (void) mcfw_snoop_send(&priv->mcfw_netdrv, set, skb);
                }
#endif /*--- #if defined(CONFIG_IP_MULTICAST_FASTFORWARD) ---*/
		    }
		}
	}
	//TODO add directpath traffic statistics

	if (skb_headroom(skb) < sizeof(struct sw_eg_pkt_header)) {
		struct sk_buff *new_skb;

		dma_alignment_eth_bad_count++;
		new_skb = alloc_skb_tx(len + sizeof(struct sw_eg_pkt_header));
		if (!new_skb)
			goto ETH_XMIT_DROP;
		else {
			skb_put(new_skb, len + sizeof(struct sw_eg_pkt_header));
			memcpy(new_skb->data + sizeof(struct sw_eg_pkt_header), skb->data, len);
    		new_skb->priority = skb->priority;
	        PPA_PORT_SEPARATION_COPY_MARK(skb,new_skb);
			dev_kfree_skb_any(skb);
			skb = new_skb;
		}
	} else {
		skb_push(skb, sizeof(struct sw_eg_pkt_header));
		dma_alignment_eth_good_count++;

		skb_orphan( skb );
	}

	len += sizeof(struct sw_eg_pkt_header);

#if 0
// AVMCCB UGW54 vs AVM-Concept
    pkth.spid           = spid;
    pkth.dpid           = port;
    pkth.lrn_dis        = port == 7 ? 1 : 0;
    
    PPA_PORT_SEPARATION_TX(skb, pkth, lan_port_seperate_enabled, wan_port_seperate_enabled, port );
    
    if ( class >= 0 )
    {
        pkth.class_en   = PS_MC_GENCFG3->class_en;
        pkth.class      = class;
    }
    if ( pkth.dpid < 2 )
        pkth.dpid_en    = g_egress_direct_forward_enable;
    else if ( pkth.dpid == 2 || pkth.dpid == 7 )
        pkth.dpid_en    = 1;

#endif
	if (spid == 2) {

		// for cpu -> ethernet
		pkth.spid = 2; //  CPU
		pkth.dpid_en = 1;
		pkth.dpid = 1; // 0 or 1 for PORT_MAP_MODE
		pkth.port_map_en = 1;
		pkth.port_map_sel = 1;
		pkth.port_map = (1 << if_id_to_mac_nr(dpid));
		pkth.lrn_dis = 0;
		pkth.class_en = 0;
		pkth.class = 0;
		dbg_dma("port_map: %#x", (unsigned int)pkth.port_map);

	} else {

		// for directpath Loop to CPU (might be interrupted by PCE-Rule -> Ethernet , or PPA -> WAN)
		pkth.spid = spid;
		pkth.dpid_en = 0;
		pkth.dpid = 1;          // don_t care
		pkth.port_map_en = 0;   // don_t care
		pkth.port_map_sel = 0;  // don_t care
		pkth.lrn_dis = 1;
		pkth.class_en = 0;
		pkth.class = prio;
		pkth.port_map = (0x3F);

	}

	memcpy(skb->data, &pkth, sizeof(struct sw_eg_pkt_header));

	put_skb_to_dbg_pool(skb);

	/*--- skb_pad linearizes our skb; so we shouldnt lookup shinfo before this line ---*/
	shinfo = skb_shinfo(skb);
#if defined (CONFIG_AVM_SCATTER_GATHER)
    if(shinfo && shinfo->nr_frags > 0 ) {
        unsigned long lock_flags;

        // this skb is scattered
        dbg_trace_ppa_data("send first fragment (nr_frags = %d)",shinfo->nr_frags );
        lock_flags = acquire_channel_lock(g_dma_device_ppe, ch );

        if(unlikely(send_skb_scattered_first(skb, g_dma_device_ppe , ch))) {
            err("sending first fragment failed");
            free_channel_lock(g_dma_device_ppe, ch, lock_flags);
            goto ETH_XMIT_DROP;
        }
        if(unlikely(send_skb_scattered_successors(skb, g_dma_device_ppe, ch))) {
            err("ah we could just send some of the fragmets, this must not happen!");
            // we cannot do any skb free now :-(
            free_channel_lock(g_dma_device_ppe, ch, lock_flags);
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
    	if (dma_device_write(g_dma_device_ppe, skb->data, len, skb, ch) != len)
		goto ETH_XMIT_DROP;
    }

	return 0;

	ETH_XMIT_DROP: 
    if (skb)
        dev_kfree_skb_any(skb);
	if (avm_dev)
		avm_dev->device_stats.tx_dropped++;
	return -1;
}

/*
 *  Description:
 *    Handle ioctl command SIOCETHTOOL.
 *  Input:
 *    dev --- struct net_device *, device responsing to the command.
 *    ifr --- struct ifreq *, interface request structure to pass parameters
 *            or result.
 *  Output:
 *    int --- 0:    Success
 *            else: Error Code (-EFAULT, -EOPNOTSUPP)
 */
static INLINE int ethtool_ioctl(struct net_device *dev __attribute__ ((unused)),
		struct ifreq *ifr) {
	struct ethtool_cmd cmd;

	if (copy_from_user(&cmd, ifr->ifr_data, sizeof(cmd)))
		return -EFAULT;

	switch (cmd.cmd) {
	case ETHTOOL_GSET: /*  get hardware information        */
	{
		memset(&cmd, 0, sizeof(cmd));

		if (copy_to_user(ifr->ifr_data, &cmd, sizeof(cmd)))
			return -EFAULT;
	}
		break;
	case ETHTOOL_SSET: /*  force the speed and duplex mode */
	{
		if (!capable(CAP_NET_ADMIN))
			return -EPERM;

		if (cmd.autoneg == AUTONEG_ENABLE) {
			/*  set property and start autonegotiation                                  */
			/*  have to set mdio advertisement register and restart autonegotiation     */
			/*  which is a very rare case, put it to future development if necessary.   */
		} else {
		}
	}
		break;
	case ETHTOOL_GDRVINFO: /*  get driver information          */
	{
		struct ethtool_drvinfo info;
		char str[32];

		memset(&info, 0, sizeof(info));
		strncpy(info.driver, "Danube Eth Driver (A4)", sizeof(info.driver) - 1);
        snprintf(str, sizeof(str),  "%d.%d.%d.%d.%d", (int)FW_VER_ID->family, (int)FW_VER_ID->package, (int)FW_VER_ID->major, (int)FW_VER_ID->middle,(int)FW_VER_ID->minor);
		strncpy(info.fw_version, str, sizeof(info.fw_version) - 1);
		strncpy(info.bus_info, "N/A", sizeof(info.bus_info) - 1);
		info.regdump_len = 0;
		info.eedump_len = 0;
		info.testinfo_len = 0;
		if (copy_to_user(ifr->ifr_data, &info, sizeof(info)))
			return -EFAULT;
	}
		break;
	case ETHTOOL_NWAY_RST: /*  restart auto negotiation        */
		break;
	default:
		return -EOPNOTSUPP;
	}

	return 0;
}

// ----------------------------  tx_alloc ----------------------------------
static INLINE struct sk_buff *alloc_skb_tx(int len)
{
    struct sk_buff *skb;
    unsigned int align_off;

    skb = dev_alloc_skb(len + DMA_TX_ALIGNMENT);

    if(likely(skb)){
        align_off = DMA_TX_ALIGNMENT - ((u32)skb->data & (DMA_TX_ALIGNMENT - 1));
        skb_reserve(skb, align_off);

        *((u32 *)skb->data - 1) = (u32)skb;
#ifndef CONFIG_MIPS_UNCACHED
        dma_cache_wback((u32)skb->data - sizeof(u32), sizeof(u32));
#endif
    }

    return skb;
}

// ----------------------------  rx_alloc ----------------------------------
static INLINE struct sk_buff *alloc_skb_rx(void)
{
    struct sk_buff *skb;
    unsigned int align_off;

    skb = dev_alloc_skb(DMA_PACKET_SIZE + DMA_RX_ALIGNMENT);
    if(likely(skb)){
        align_off = DMA_RX_ALIGNMENT - ((u32)skb->data & (DMA_RX_ALIGNMENT - 1));
        skb_reserve(skb, align_off);

        *((u32 *)skb->data - 1) = (u32)skb;
#ifndef CONFIG_MIPS_UNCACHED
        dma_cache_wback((u32)skb->data - sizeof(u32), sizeof(u32));
#endif
    }

    return skb;
}



static irqreturn_t mailbox0_irq_handler(int irq __attribute__((unused)), void *dev_id __attribute__((unused))) {
	u32 mailbox_signal = 0;

	mailbox_signal = *MBOX_IGU0_ISR & *MBOX_IGU0_IER;

#if defined(CONFIG_AVMNET_DEBUG) 
	if ( g_dbg_datapath & DBG_ENABLE_MASK_DEBUG_MAILBOX )
		printk(KERN_ERR "you've got mail in box0 ;)\n");
#endif

	if (!mailbox_signal)
		return IRQ_HANDLED;
	*MBOX_IGU0_ISRC = mailbox_signal;

	if ((mailbox_signal & DMA_TX_CH0_SIG)) {
		// g_dma_device_ppe->tx_chan[0]->open(g_dma_device_ppe->tx_chan[0]);
		*MBOX_IGU0_IER &= DMA_TX_CH1_SIG;

#if defined(CONFIG_AVMNET_DEBUG) 
		if ( g_dbg_datapath & DBG_ENABLE_MASK_DEBUG_MAILBOX )
			printk(KERN_ERR "DMA_TX_CH0_SIG\n"); // showtime signal
#endif
	}

	if ((mailbox_signal & DMA_TX_CH1_SIG)) {
		g_dma_device_ppe->tx_chan[1]->open(g_dma_device_ppe->tx_chan[1]);
		*MBOX_IGU0_IER &= DMA_TX_CH0_SIG;

#if defined(CONFIG_AVMNET_DEBUG) 
		if ( g_dbg_datapath & DBG_ENABLE_MASK_DEBUG_MAILBOX )
			printk(KERN_ERR "DMA_TX_CH1_SIG\n");
#endif
	}

	return IRQ_HANDLED;
}

/*
 *  Description:
 *    Handle IRQ of mailbox and despatch to relative handler.
 *  Input:
 *    irq    --- int, IRQ number
 *    dev_id --- void *, argument passed when registering IRQ handler
 *    regs   --- struct pt_regs *, registers' value before jumping into handler
 *  Output:
 *    none
 */
static irqreturn_t mailbox1_irq_handler(int irq __attribute__ ((unused)), void *dev_id __attribute__ ((unused))) {
	u32 mailbox_signal = 0;
    spin_lock(&g_mailbox_signal_spinlock);
#if defined(CONFIG_AVMNET_DEBUG) 
	if ( g_dbg_datapath & DBG_ENABLE_MASK_DEBUG_MAILBOX )
		printk(KERN_ERR "you've got mail in box1 ");
#endif

	mailbox_signal = *MBOX_IGU1_ISR & g_mailbox_signal_mask;
	if (!mailbox_signal)
        goto EXIT_MAILBOX_IRQ_HANDLER;
	*MBOX_IGU1_ISRC = mailbox_signal;

	if ((mailbox_signal & WAN_RX_SIG(0))) //  PTM Packet
	{
#if defined(CONFIG_AVMNET_DEBUG) 
		if ( g_dbg_datapath & DBG_ENABLE_MASK_DEBUG_MAILBOX )
			printk(KERN_ERR "WAN_RX_SIG(0)\n"); //showtime signal
#endif

		g_mailbox_signal_mask &= ~WAN_RX_SIG(0);
		g_dma_device_ppe->tx_chan[1]->open(g_dma_device_ppe->tx_chan[1]);
		*MBOX_IGU1_IER = g_mailbox_signal_mask;
	}

	if ((mailbox_signal & CPU_TO_WAN_SWAP_SIG)) {
		struct sk_buff *new_skb;
		volatile struct tx_descriptor *desc = &CPU_TO_WAN_SWAP_DESC_BASE[g_cpu_to_wan_swap_desc_pos];
		struct tx_descriptor reg_desc = { 0 };

#if defined(CONFIG_AVMNET_DEBUG) 
		if ( g_dbg_datapath & DBG_ENABLE_MASK_DEBUG_MAILBOX )
			printk(KERN_ERR "CPU_TO_WAN_SWAP_SIG\n");
#endif
		while (desc->own == 0) {
			new_skb = alloc_skb_rx();
			if (new_skb == NULL)
				break;

#ifndef CONFIG_MIPS_UNCACHED
			/*  invalidate cache    */
			dma_cache_inv((unsigned long)new_skb->data, DMA_PACKET_SIZE);
#endif

			if (++g_cpu_to_wan_swap_desc_pos == CPU_TO_WAN_SWAP_DESC_NUM)
				g_cpu_to_wan_swap_desc_pos = 0;

			/*  free previous skb   */
			free_skb_clear_dataptr(desc);
			put_skb_to_dbg_pool(new_skb);

			/*  update descriptor   */
			reg_desc.dataptr = (unsigned int) new_skb->data & (0x1FFFFFFF ^ (DMA_TX_ALIGNMENT - 1));
			reg_desc.own = 1;

			/*  write discriptor to memory  */
			*((volatile unsigned int *) desc + 1) = *((unsigned int *) &reg_desc + 1);
			wmb();
			*(volatile unsigned int *) desc = *(unsigned int *) &reg_desc;

			desc = &CPU_TO_WAN_SWAP_DESC_BASE[g_cpu_to_wan_swap_desc_pos];
		}
	}

	if ((mailbox_signal & CPU_TO_WAN_TX_SIG)) {
#if defined(CONFIG_AVMNET_DEBUG) 
		if ( g_dbg_datapath & DBG_ENABLE_MASK_DEBUG_MAILBOX )
			printk(KERN_ERR "CPU_TO_WAN_TX_SIG\n");
#endif
		g_mailbox_signal_mask &= ~CPU_TO_WAN_TX_SIG;
		SW_WRITE_REG32(g_mailbox_signal_mask, MBOX_IGU1_IER);
		if (g_ptm_net_dev[0]) {
			err(" Mailbox: wake ptm dev\n");
			netif_wake_queue(g_ptm_net_dev[0]);
		}
	}

EXIT_MAILBOX_IRQ_HANDLER:
    spin_unlock(&g_mailbox_signal_spinlock);
	return IRQ_HANDLED;
}

static INLINE void turn_on_dma_rx(int mask) {
	unsigned int i;

	if (!g_f_dma_opened) {

		dbg("[%s] turn on all channels\n", __func__);
		ASSERT((u32)g_dma_device_ppe >= 0x80000000, "g_dma_device_ppe = 0x%08X",
				(u32)g_dma_device_ppe);

		for (i = 0; i < g_dma_device_ppe->max_rx_chan_num; i++) {

			ASSERT((u32)g_dma_device_ppe->rx_chan[i] >= 0x80000000, "g_dma_device_ppe->rx_chan[%d] = 0x%08X", i, (u32)g_dma_device_ppe->rx_chan[i]);
			ASSERT(g_dma_device_ppe->rx_chan[i]->control == IFX_DMA_CH_ON, "g_dma_device_ppe->rx_chan[i]->control = %d", g_dma_device_ppe->rx_chan[i]->control);

			if (g_dma_device_ppe->rx_chan[i]->control == IFX_DMA_CH_ON) {
				ASSERT((u32)g_dma_device_ppe->rx_chan[i]->open >= 0x80000000, "g_dma_device_ppe->rx_chan[%d]->open = 0x%08X", i, (u32)g_dma_device_ppe->rx_chan[i]->open);

				//  channel 1, 2 is for fast path
/*---
 * Seems to be a lantiq hack
 *  1) put channel-config to tx-mode
 *  2) then open it (no irq is not enabled but channel is activated)
 *  3) put channnel-config to undefined
 * We use normal open and no_cpu_data flag for setting up directpath channels
 */
#if 0
				if (i == 1 || i == 2)
					g_dma_device_ppe->rx_chan[i]->dir = 1; // 1 : TX-Channel
#endif
				g_dma_device_ppe->rx_chan[i]->open( g_dma_device_ppe->rx_chan[i] );
#if 0
				if (i == 1 || i == 2)
					g_dma_device->rx_chan[i]->dir = 0; // -1: not defined
#endif
			}
		}
	}
	g_f_dma_opened |= 1 << mask;
	dbg("[%s] g_f_dma_opened = %#x\n", __func__, g_f_dma_opened);
}

#if 0
// dma lauft bei uns dauerhaft
static INLINE void turn_off_dma_rx(int mask) {
	int i;

	g_f_dma_opened &= ~(1 << mask);
	if (!g_f_dma_opened) {
		dbg("[%s] turn off all rx channels\n", __func__);
		for (i = 0; i < g_dma_device_ppe->max_rx_chan_num; i++)
			g_dma_device_ppe->rx_chan[i]->close(g_dma_device_ppe->rx_chan[i]);
	}
	dbg("[%s] g_f_dma_opened = %#x\n", __func__, g_f_dma_opened);
}
#endif

static u8 *dma_buffer_alloc(int len __attribute__ ((unused)), int *byte_offset,
		void **opt) {
	u8 *buf;
	struct sk_buff *skb;

	skb = alloc_skb_rx();
	if (!skb)
		return NULL;

	put_skb_to_dbg_pool(skb);

	buf = (u8 *) skb->data;
	*(u32 *) opt = (u32) skb;
	*byte_offset = 0;
	return buf;
}

static int dma_buffer_free(u8 *dataptr __attribute__((unused)), void *opt) {
	//  ! be careful
	//    this method makes kernel crash when free this DMA device
	//    since the pointers used by fastpast which is supposed not visible to CPU
	//    mix up the pointers so that "opt" is not align with "dataptr".

//    ASSERT(*(void **)((((u32)dataptr | KSEG1) & ~0x0F) - 4) == opt, "address is not match: dataptr = %08X, opt = %08X, *(void **)((((u32)dataptr | KSEG1) & ~0x0F) - 4) = %08X", (u32)dataptr, (u32)opt, *(u32 *)((((u32)dataptr | KSEG1) & ~0x0F) - 4));

	if (opt) {
		get_skb_from_dbg_pool((struct sk_buff *)opt);
		dev_kfree_skb_any((struct sk_buff *) opt);
	}

	return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
#if defined(DEBUG_DMA_RX_CHANNEL_COUNTER)
#define MAX_DMA_CHAN 8
int dma_rx_chan_count[MAX_DMA_CHAN];
#endif


static int process_backlog_e5(struct napi_struct *napi, int quota)
{
    struct sk_buff *skb = NULL;
    struct softnet_data *queue = &__get_cpu_var(g_7port_eth_queue);
    unsigned long start_time = jiffies;
    int work = 0;
    unsigned long flags;

    napi->weight = 64;

    do{
        local_irq_save(flags);
        skb = __skb_dequeue(&queue->input_pkt_queue);

        if (!skb){
            __napi_complete(napi);
            local_irq_restore(flags);
            break;
        }
        local_irq_restore(flags);

        handle_skb_e5(skb);
    }while (++work < quota && jiffies == start_time);

    return work;
}

static INLINE int handle_skb_e5(struct sk_buff *skb)
{
	struct flag_header *fheader;
	struct pmac_header *pheader;
	avmnet_netdev_priv_t *priv;
	avmnet_device_t *avm_dev = NULL;
	unsigned int exp_len;
#ifdef CONFIG_AVM_PA
	unsigned int hw_dev_id;
#endif

	dump_skb( skb, DUMP_SKB_LEN, "handle_skb_e5 raw data", 0, -1, 0, 0);
	dump_flag_header(g_fwcode, (struct flag_header *) skb->data, __FUNCTION__, 0);

	// fetch flag_header from skb
	fheader = (struct flag_header *) skb->data;

	// fetch pmac_header from skb
	pheader = (struct pmac_header *) (skb->data + fheader->pl_byteoff);

	dump_pmac_header(pheader, __FUNCTION__);

	/* 
	 * in indirect fast path pheader->len and fheader->len doesn't contain valid packet length, 
	 * so we can do PMAC header check only in unaccelerated packets.
	 */
	if ( ! fheader->acc_done ) {

		/*
		 * calculate expected SKB len.
		 * Payload offset from fheader + packet length from PMAC header + 4 bytes for frame crc
		 */
		exp_len = fheader->pl_byteoff + (pheader->PKT_LEN_HI << 8) + pheader->PKT_LEN_LO + 4u;

		if(exp_len != skb->len ){
			printk(KERN_ERR "[%s] Malformed PMAC header: exp_len=%d, real_len=%d\n", __func__,exp_len,skb->len );
			dump_skb( skb, DUMP_SKB_LEN, "handle_skb_e5", 0, -1, 0, 1);
			dev_kfree_skb_any(skb);
			return 0;
		}
	}

	skb_pull(skb, fheader->pl_byteoff + sizeof(struct pmac_header));

	ASSERT(skb->len >= 60, "packet is too small: %d", skb->len);

#if defined(DEBUG_SPACE_BETWEEN_HEADER_AND_DATA)
	dbg_keep_min(STD_BYTE_OFF + fheader->pl_byteoff - sizeof(struct flag_header), &min_fheader_byteoff);
#endif

	dump_skb(skb, DUMP_SKB_LEN, "handle_skb_e5 packet data", fheader->src_itf, 0, 0, 0);

#ifdef CONFIG_AVM_PA
	//  implement indirect fast path
	if (fheader->acc_done && fheader->dest_list) //  acc_done == 1 && dest_list != 0
	{
		//  Current implementation send packet to highest priority dest only (0 -> 7, 0 is highest)
		//  2 is CPU, so we ignore it here
		if ((fheader->dest_list & (1 << 0))) //  0 - eth0
		{
			eth_xmit(skb, 0, DMA_CHAN_NR_DEFAULT_CPU_TX, 2, fheader->qid);
			return 0;
		} else if ((fheader->dest_list & (1 << 1))) //  1 - eth1
		{
			eth_xmit(skb, 1, DMA_CHAN_NR_DEFAULT_CPU_TX, 2, fheader->qid);
			return 0;
		} else if ((fheader->dest_list & (1 << 7))) //  7 - ptm0
		{
			ifx_ppa_ptm_hard_start_xmit(skb, g_ptm_net_dev[0]);
			return 0;
		} else {

			int dst_id;

			for (dst_id = IFX_VIRT_DEV_OFFSET; dst_id < IFX_VIRT_DEV_OFFSET + MAX_PPA_PUSHTHROUGH_DEVICES; dst_id++) {
				if ((fheader->dest_list & (1 << dst_id))) //  3...5 (einschliesslich 3 u. 5)
				{
					int if_id = HW_DEV_ID(dst_id);

					if ( ifx_lookup_routing_entry && ppa_virt_tx_devs[ if_id ] ){

						int rout_index;
						avm_session_handle avm_pa_session;

						dbg_trace_ppa_data("ACC-Done: ifid=%d, avm_rout_index=%d, is_mc=%d, is_lan=%d, is_coll=%d",
								   if_id,
								   fheader->avm_rout_index,
								   fheader->avm_is_mc,
								   fheader->avm_is_lan,
								   fheader->avm_is_coll );
						ppa_virt_tx_devs[ if_id ]->hw_pkt_tx ++;

						rout_index = fheader->avm_rout_index;
						if ( fheader->avm_is_coll && !fheader->avm_is_mc){

							/*
							 * coll_entry numbers start directly after hash_table numbers
							 * the following should be the same as rout_index |= (1 << 7);
							 */
							rout_index += (MAX_ROUTING_ENTRIES_PER_HASH_BLOCK * MAX_HASH_BLOCK);

						}

						avm_pa_session = ifx_lookup_routing_entry(rout_index, fheader->avm_is_lan, fheader->avm_is_mc);

						if (avm_pa_session != INVALID_SESSION ){
							avm_pa_tx_channel_accelerated_packet( ppa_virt_tx_devs[ if_id ]->pid_handle, avm_pa_session, skb );
						}
						else{
							ppa_virt_tx_devs[ if_id ]->hw_pkt_tx_session_lookup_failed ++;
#if defined(CONFIG_AVMNET_DEBUG) 
							if ( g_dbg_datapath & DBG_ENABLE_MASK_DEBUG_VDEV ){
								// unsigned int src_ip =  (skb->data[26] << 24) | (skb->data[27] << 16) | (skb->data[28] << 8) | (skb->data[29] << 0);
								// dump_sessions_with_src_ip( src_ip );
								dump_skb(skb, unknown_session_dump_len, "unknown session", fheader->src_itf, -1, 0, 1);
							}
#endif
							dev_kfree_skb_any(skb);
						}

						return 0;

					}
				}
			}
			dev_kfree_skb_any(skb);
			return 0;
		}
	}
#endif /* #ifdef CONFIG_AVM_PA */

	switch (fheader->src_itf) {
	case 0: //  incomming LAN IF ( vom Switch klassifizerit )
	case 1: //  incomming WAN IF ( vom Switch klassifizerit )
		avm_dev = mac_nr_to_avm_dev(pheader->SPPID);

		// TKL: FIXME workaround gegen Haenger beim Laden der Wasp-FW, check auf carrier_ok
		if (avm_dev && netif_running(avm_dev->device) && netif_carrier_ok(avm_dev->device)) {
			//  do not need to remove CRC
			//skb->tail -= ETH_CRC_LENGTH;
			//skb->len  -= ETH_CRC_LENGTH;
			unsigned int keep_len = 0;

			dbg_dma("got receive if: avm_name:%s ifname:%s\n", avm_dev->device_name, avm_dev->device->name);
			skb->dev = avm_dev->device;
#           if defined(CONFIG_IP_MULTICAST_FASTFORWARD)
			{
				avmnet_netdev_priv_t *priv = netdev_priv(avm_dev->device);

				if(likely(priv->mcfw_used)) {
					mcfw_snoop_recv(&priv->mcfw_netdrv, 0, skb);
				}
			}
#           endif /*--- #if defined(CONFIG_IP_MULTICAST_FASTFORWARD) ---*/
			skb->protocol = eth_type_trans(skb, avm_dev->device);

			if (skb->len > 0)
				keep_len = skb->len;

#ifdef CONFIG_AVM_PA
			if(likely(avm_pa_dev_receive(AVM_PA_DEVINFO(avm_dev->device), skb) == AVM_PA_RX_ACCELERATED)) {
				avm_dev->device->last_rx = jiffies;
				return 0;
			}
#endif
			avm_dev->device_stats.rx_packets++;
			avm_dev->device_stats.rx_bytes += keep_len;
			netif_receive_skb(skb);

			return 0;
		} else {
			if (!avm_dev){
				err( "[%s] no avm dev for mac %d, rx-chan %d\n", __FUNCTION__, pheader->SPPID, 0 );
			}

			dbg("[%s] src_itf %d, mac_port=%d down drop packet\n", __FUNCTION__, fheader->src_itf, pheader->SPPID);
			if (g_dbg_datapath & DBG_ENABLE_MASK_DEBUG_PRINT ) {
				unsigned int i;
				printk("  skb->data = %08X, skb->tail = %08X, skb->len = %d\n", (u32)skb->data, (u32)skb->tail, (int)skb->len);
				for ( i = 1; i <= skb->len ; i++ ) {
					if ( i % 16 == 1 )
						printk("  %4d:", i - 1);
					printk(" %02X", (int)(*((char*)skb->data + i - 1) & 0xFF));
					if ( i % 16 == 0 )
						printk("\n");
				}
				if ( (i - 1) % 16 != 0 )
					printk("\n");
			}
		}
		break;
	case 7: //  PTM
		if (!g_ptm_net_dev[0]) {
			//in Ethernet WAN mode, ptm device not initialized, but it might get packet for it.
			dbg_dma(KERN_ERR "[%s] ptm netif down drop packet\n", __FUNCTION__);
			break; //just drop this packet
		}
		skb->dev = g_ptm_net_dev[0];
		priv = netdev_priv(skb->dev);
		avm_dev = priv->avm_dev;

		if (netif_running(g_ptm_net_dev[0])) {
			unsigned int keep_len = 0;
			dbg_dma(KERN_ERR "[%s] got a ptm packet", __FUNCTION__);

			skb->protocol = eth_type_trans(skb, g_ptm_net_dev[0]);
			if (skb->len > 0)
				keep_len = skb->len;

#ifdef CONFIG_AVM_PA
			if(likely(
				  avm_pa_dev_receive(AVM_PA_DEVINFO(avm_dev->device), skb) 
				  == AVM_PA_RX_ACCELERATED)) {
				avm_dev->device->last_rx = jiffies;
				return 0;
			}
#endif
			avm_dev->device_stats.rx_packets++;
			avm_dev->device_stats.rx_bytes += keep_len;
			netif_receive_skb(skb);


			dbg_dma(KERN_ERR "[%s] done", __FUNCTION__);
			return 0;
		}
		break;
	default: //  other interface receive (z.b. WLAN )
		// check constraints: fheader->src_itf in HW_DEV_ID
#ifdef CONFIG_AVM_PA
		hw_dev_id = HW_DEV_ID(fheader->src_itf);

		if (hw_dev_id < ARRAY_SIZE(ppa_virt_rx_devs) && ppa_virt_rx_devs[hw_dev_id]){
			dbg_trace_ppa_data("PPE_DIRECTPATH_DATA_RX_ENABLE: src_itf=%d", fheader->src_itf );

			//TODO skb->dev muss von AVM_PA gesetzt werden
			ppa_virt_rx_devs[hw_dev_id]->hw_pkt_slow_cnt ++;
			avm_pa_rx_channel_packet_not_accelerated(ppa_virt_rx_devs[hw_dev_id]->pid_handle, skb);
			return 0;
		}else {
			dbg_trace_ppa_data("!PPE_DIRECTPATH_DATA_RX_ENABLE: src_itf=%d", fheader->src_itf );
		}
#else
		BUG();
#endif
	}

	dbg(KERN_ERR "[%s] netif down drop packet\n", __FUNCTION__);
	if (avm_dev)
		avm_dev->device_stats.rx_dropped++;
	dev_kfree_skb_any(skb);
	return 0;
}


static void __switch_to_eth_thunk(void)
{
    int i;
    struct rx_gamma_itf_cfg rx_gamma_itf_cfg;

    CFG_STD_DATA_LEN->byte_off  = 0;
    BOND_CONF->max_frag_size    = 1536;
    *PDMA_RX_MAX_LEN_REG        = 0x02040604;

    for ( i = 0; i < 4; i++ )
    {
        rx_gamma_itf_cfg = *RX_GAMMA_ITF_CFG(i);

        rx_gamma_itf_cfg.rx_eth_fcs_ver_dis = 0;
        rx_gamma_itf_cfg.rx_rm_eth_fcs      = 1;
        rx_gamma_itf_cfg.rx_edit_num1       = 4;
        rx_gamma_itf_cfg.rx_edit_type1      = 1;
        rx_gamma_itf_cfg.rx_edit_en1        = 1;

        *RX_GAMMA_ITF_CFG(i) = rx_gamma_itf_cfg;
    }

    // no need to change for tx gif config
    BOND_CONF->bond_mode = L2_ETH_TRUNK_MODE;
}

static void switch_to_eth_trunk(void)
{
    // assume no traffic

    unsigned long sys_flag;
#ifdef CONFIG_IFX_PPE_E5_OFFCHIP_BONDING
    unsigned int ppe_base_addr;
#endif

    local_irq_save(sys_flag);
#ifdef CONFIG_IFX_PPE_E5_OFFCHIP_BONDING
    ppe_base_addr = g_ppe_base_addr;
    g_ppe_base_addr = IFX_PPE_ORG(0);
#endif

    __switch_to_eth_thunk();

#ifdef CONFIG_IFX_PPE_E5_OFFCHIP_BONDING
    // config bonding master
    g_ppe_base_addr = IFX_PPE_ORG(1);
    __switch_to_eth_thunk();
    g_ppe_base_addr = ppe_base_addr;
#endif
    local_irq_restore(sys_flag);
}

static void __switch_to_ptmtc_bonding(void)
{
    int i;
    struct rx_gamma_itf_cfg rx_gamma_itf_cfg;

    CFG_STD_DATA_LEN->byte_off  = 2;
    BOND_CONF->max_frag_size    = 0x200;
    *PDMA_RX_MAX_LEN_REG        = 0x02040204;

    for ( i = 0; i < 4; i++ )
    {
        rx_gamma_itf_cfg = *RX_GAMMA_ITF_CFG(i);

        rx_gamma_itf_cfg.rx_eth_fcs_ver_dis = 1;
        rx_gamma_itf_cfg.rx_rm_eth_fcs      = 0;
        rx_gamma_itf_cfg.rx_edit_num1       = 0;
        rx_gamma_itf_cfg.rx_edit_type1      = 0;
        rx_gamma_itf_cfg.rx_edit_en1        = 0;

        *RX_GAMMA_ITF_CFG(i) = rx_gamma_itf_cfg;
    }

    // no need to change for tx gif config
    BOND_CONF->bond_mode = L1_PTM_TC_BOND_MODE;
}

static void switch_to_ptmtc_bonding(void)
{
    // assume no traffic

    unsigned long sys_flag;
#ifdef CONFIG_IFX_PPE_E5_OFFCHIP_BONDING
    unsigned int ppe_base_addr;
#endif

    local_irq_save(sys_flag);
#ifdef CONFIG_IFX_PPE_E5_OFFCHIP_BONDING
    ppe_base_addr = g_ppe_base_addr;
    g_ppe_base_addr = IFX_PPE_ORG(0);
#endif

    __switch_to_ptmtc_bonding();

#ifdef CONFIG_IFX_PPE_E5_OFFCHIP_BONDING
    // config bonding master
    g_ppe_base_addr = IFX_PPE_ORG(1);
    __switch_to_ptmtc_bonding();
    g_ppe_base_addr = ppe_base_addr;
#endif
    local_irq_restore(sys_flag);
}

static void update_max_delay(int line)
{
    unsigned int user_max_delay;
    unsigned int linerate;
    int i, off;
    unsigned int delay_value;

    if ( g_dsl_bonding && line >= 0 && line < LINE_NUMBER )
    {
        delay_value = 1000;
        user_max_delay = (g_max_delay[line] + 8) / 16;
        for ( i = 0, linerate = 0; i < BEARER_CHANNEL_PER_LINE; i++ )
            linerate += g_datarate_ds[line * BEARER_CHANNEL_PER_LINE + i];
        linerate = (linerate + 50) / 100;
        if ( linerate != 0 )
        {
            //  delay_value = B1_DS_LL_CTXT->max_des_num * 512 * 8 / linerate / (16 / 1000000)
            delay_value = MAX_DESC_PER_GAMMA_ITF * 256 * 10000 / linerate;  //  use default value MAX_DESC_PER_GAMMA_ITF rather than reading max_des_num from each B1_DS_LL_CTXT
        }
        if ( user_max_delay != 0 && user_max_delay < delay_value )
            delay_value = user_max_delay;
        switch ( line )
        {
        case 0:
            off = 0;
            break;
        case 1:
        default:
            off = g_dsl_bonding == 1 ?  /* on-chip */ 2 : /* off-chip */ 4;
        }
        for ( i = 0; i < 2; i++ )
            B1_DS_LL_CTXT(1 + off + i)->max_delay = delay_value;
    }
}


static INLINE void reset_ppe(void) {
    //  reset PPE
    ifx_rcu_rst(IFX_RCU_DOMAIN_DSLDSP, IFX_RCU_MODULE_PPA);
    udelay(1000);
    ifx_rcu_rst(IFX_RCU_DOMAIN_DSLTC, IFX_RCU_MODULE_PPA);
    udelay(1000);

    // ifx_rcu_rst(IFX_RCU_DOMAIN_PPE, IFX_RCU_MODULE_PPA); //CCB: Dieser Reset koennte zu den 128 kaputten Paketen fuehren
    // udelay(1000);

    *PP32_SRST &= ~0x000303CF;
    udelay(1000);
    *PP32_SRST |= 0x000303CF;
    udelay(1000);
}

static INLINE void init_pmu(void) {
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

	PPE_TOP_PMU_SETUP(IFX_PMU_ENABLE);
	PPE_SLL01_PMU_SETUP(IFX_PMU_ENABLE);
	PPE_TC_PMU_SETUP(IFX_PMU_ENABLE);
	PPE_EMA_PMU_SETUP(IFX_PMU_ENABLE);
	PPE_TPE_PMU_SETUP(IFX_PMU_ENABLE);
	//  AHB Slave
	DSL_DFE_PMU_SETUP(IFX_PMU_ENABLE);
	PPE_DPLUS_PMU_SETUP(IFX_PMU_ENABLE);
	PPE_DPLUSS_PMU_SETUP(IFX_PMU_ENABLE);
	SWITCH_PMU_SETUP(IFX_PMU_ENABLE);
}

static INLINE void start_cpu_port(void) {
	int i;

	*DS_RXCFG |= 0x00008000; //  enable DSRX
	*DM_RXCFG |= 0x80000000; //  enable DMRX

	for (i = 0; i < 7; i++) {
		IFX_REG_W32_MASK(0, 1, FDMA_PCTRL_REG(i));
		//  start port 0 - 6
		IFX_REG_W32_MASK(0, 1, SDMA_PCTRL_REG(i));
		//  start port 0 - 6
	}
	*PCE_PCTRL_REG(6, 0) |= 0x07; //  clear_dplus
}



static INLINE void init_internal_tantos_qos_setting(void) {
}

static INLINE void init_internal_tantos(void) {
	int i;


	*GLOB_CTRL_REG = 1 << 15; //  enable Switch

	for (i = 0; i < 7; i++)
		sw_clr_rmon_counter(i);

	//*FDMA_PCTRL_REG(6) &= ~0x01;//  disable port (FDMA)
	*FDMA_PCTRL_REG(6) |= 0x02; //  insert special tag

	*PMAC_HD_CTL_REG = 0x01DC; //  PMAC Head

	for (i = 6; i < 12; i++)
		*PCE_PCTRL_REG(i, 0) |= 1 << 11; //  ingress special tag

	*MAC_FLEN_REG = MAC_FLEN_MAX_SIZE + 8 + 4 * 2; //  MAC frame + 8-byte special tag + 4-byte VLAN tag * 2
	*MAC_CTRL_REG(6, 2) |= 1 << 3; //  enable jumbo frame
	for ( i = 0; i < 6; i++) {
	    *MAC_CTRL_REG(i, 2) |= 1 << 3; //  enable jumbo frame
    	}

//    for ( i = 0; i < 7; i++ )
//        *PCE_PCTRL_REG(i, 2) |= 3;  //  highest class, mapped to RX channel 0

	*PCE_PMAP_REG(1) = 0x7F; //  monitoring
	//*PCE_PMAP_REG(2) = 0x7F & ~g_pmac_ewan; //  broadcast and unknown multicast
	*PCE_PMAP_REG(2) = 0x7F; //  broadcast and unknown multicast
	//*PCE_PMAP_REG(3) = 0x7F & ~g_pmac_ewan; //  unknown uni-cast
	*PCE_PMAP_REG(3) = 0x7F; //  unknown uni-cast

	*PMAC_EWAN_REG = g_pmac_ewan;

	*PMAC_RX_IPG_REG = 0x8F;

	//IFX_VR9_Switch_PCE_Micro_Code_Int();

	init_internal_tantos_qos_setting();
}

static INLINE void init_dplus(void)
{
    *DM_RXCFG &= ~0x80000000;   //  disable DMRX
    *DS_RXCFG &= ~0x00008000;   //  disable DSRX

    *DM_RXPGCNT = 0x00002000;   //  clear page pointer & counter
    *DS_RXPGCNT = 0x00004000;   //  clear page pointer & counter

    *DM_RXDB    = 0x2550;
    *DM_RXCB    = 0x27D0;
    *DM_RXPGCNT = 0x00002000;
    *DM_RXPKTCNT= 0x00002000;
    *DM_RXCFG   = 0x00F87014;

	wmb();
	udelay(100);
}

static INLINE void init_pdma(void) {
	//*EMA_CMDCFG  = (EMA_CMD_BUF_LEN << 16) | (EMA_CMD_BASE_ADDR >> 2);
	//*EMA_DATACFG = (EMA_DATA_BUF_LEN << 16) | (EMA_DATA_BASE_ADDR >> 2);
	//*EMA_IER     = 0x000000FF;
	//*EMA_CFG     = EMA_READ_BURST | (EMA_WRITE_BURST << 2);

	*PDMA_CFG = 0x00000001;
	*PDMA_RX_CTX_CFG = 0x00082C00;
	*PDMA_TX_CTX_CFG = 0x00081B00;
    *PDMA_RX_MAX_LEN_REG= g_dsl_bonding ? 0x02040204 : 0x02040604;
	*PDMA_RX_DELAY_CFG = 0x000F003F;

	*SAR_MODE_CFG = 0x00000011;
	*SAR_RX_CTX_CFG = 0x00082A00;
	*SAR_TX_CTX_CFG = 0x00082E00;
	*SAR_POLY_CFG_SET0 = 0x00001021;
	*SAR_POLY_CFG_SET1 = 0x1EDC6F41;
	*SAR_POLY_CFG_SET2 = 0x04C11DB7;
	*SAR_CRC_SIZE_CFG = 0x00000F3E;

	*SAR_PDMA_RX_CMDBUF_CFG = 0x01001900;
	*SAR_PDMA_TX_CMDBUF_CFG = 0x01001A00;
}

static INLINE void init_mailbox(void) {
	*MBOX_IGU0_ISRC = 0xFFFFFFFF;
	*MBOX_IGU0_IER = 0x00000000;
	*MBOX_IGU1_ISRC = 0xFFFFFFFF;
	*MBOX_IGU1_IER = 0x00000000; //  Don't need to enable RX interrupt, EMA driver handle RX path.
	dbg("MBOX_IGU1_IER = 0x%08X", *MBOX_IGU1_IER);
}

static INLINE void clear_share_buffer(void) {
	volatile u32 *p;
	unsigned int i;

	p = SB_RAM0_ADDR_BOND(2, 0);
	for (i = 0; i < SB_RAM0_DWLEN + SB_RAM1_DWLEN + SB_RAM2_DWLEN + SB_RAM3_DWLEN; i++)
		SW_WRITE_REG32(0, p++);

	p = SB_RAM6_ADDR_BOND(2, 0);
	for (i = 0; i < SB_RAM6_DWLEN; i++)
		SW_WRITE_REG32(0, p++);
}

static INLINE void clear_cdm(void) {
}

static INLINE void board_init(void) {
}

static INLINE void init_dsl_hw(void) {
	SW_WRITE_REG32(0x00010040, SFSM_CFG0);
	SW_WRITE_REG32(0x00010040, SFSM_CFG1);
	SW_WRITE_REG32(0x00020000, SFSM_PGCNT0);
	SW_WRITE_REG32(0x00020000, SFSM_PGCNT1);
	SW_WRITE_REG32(0x00000000, DREG_AT_IDLE0);
	SW_WRITE_REG32(0x00000000, DREG_AT_IDLE1);
	SW_WRITE_REG32(0x00000000, DREG_AR_IDLE0);
	SW_WRITE_REG32(0x00000000, DREG_AR_IDLE1);
	SW_WRITE_REG32(0x0000080C, DREG_B0_LADR);
	SW_WRITE_REG32(0x0000080C, DREG_B1_LADR);

	SW_WRITE_REG32(0x000001F0, DREG_AR_CFG0);
	SW_WRITE_REG32(0x000001F0, DREG_AR_CFG1);
	SW_WRITE_REG32(0x000001E0, DREG_AT_CFG0);
	SW_WRITE_REG32(0x000001E0, DREG_AT_CFG1);

	/*  clear sync state    */
	//SW_WRITE_REG32(0, SFSM_STATE0);
	//SW_WRITE_REG32(0, SFSM_STATE1);
	SW_WRITE_REG32_MASK(0, 1 << 14, SFSM_CFG0);
	//  enable SFSM storing
	SW_WRITE_REG32_MASK(0, 1 << 14, SFSM_CFG1);

	SW_WRITE_REG32_MASK(0, 1 << 15, SFSM_CFG0);
	//  HW keep the IDLE cells in RTHA buffer
	SW_WRITE_REG32_MASK(0, 1 << 15, SFSM_CFG1);

	SW_WRITE_REG32(0xF0D10000, FFSM_IDLE_HEAD_BC0);
	SW_WRITE_REG32(0xF0D10000, FFSM_IDLE_HEAD_BC1);
	SW_WRITE_REG32(0x00030028, FFSM_CFG0);
	//  Force_idle
	SW_WRITE_REG32(0x00030028, FFSM_CFG1);
}

static INLINE void setup_ppe_conf(void) {
	// disable PPE and MIPS access to DFE memory
	*RCU_PPE_CONF = *RCU_PPE_CONF & ~0x78000000;
}

static INLINE void init_hw(void) {

    if ( g_eth_wan_mode == 0 /* DSL WAN */ ) {
        ppe_clk_change( AVM_PPE_CLOCK_DSL, FLAG_PPE_CLK_CHANGE_ARG_AVM_DIRCT );
    } else {
        ppe_clk_change( AVM_PPE_CLOCK_ATA, FLAG_PPE_CLK_CHANGE_ARG_AVM_DIRCT );
    }

	setup_ppe_conf(); // ok

	init_pmu();  // ok


	init_internal_tantos(); // ok

	init_dplus(); // reset fehlt?!

	init_dsl_hw(); // ok

	init_pdma(); // ok

	init_mailbox(); // ok - mbox2 hinzu

	clear_share_buffer();  //ok

	clear_cdm(); //ok

	board_init(); //ok
}

#ifdef CONFIG_IFX_PPE_E5_OFFCHIP_BONDING
static INLINE void init_bonding_hw(void)
{
    //  off chip bonding (bonding master)
    g_ppe_base_addr = IFX_PPE_ORG(1);

    setup_ppe_conf();

    //  PPE TOP, PPE EMA/PDMA, PPE TC, PPE SLL
    *IFX_PMU_PWDCR &= ~((1 << 29) | (1 << 22) | (1 << 21) | (1 << 19));

    if ( (*IFX_PMU_PWDCR & (1 << 23)) )
        *DM_RXCFG &= ~0x80000000;   //  disable DMRX
    if ( (*IFX_PMU_PWDCR & (1 << 24)) )
        *DS_RXCFG &= ~0x00008000;   //  disable DSRX

    init_dsl_hw();

    init_pdma();

    init_mailbox();

    clear_share_buffer();

    clear_cdm();

    g_ppe_base_addr = IFX_PPE_ORG(0);
}
#endif

/*
 *  Description:
 *    Download PPE firmware binary code.
 *  Input:
 *    src       --- u32 *, binary code buffer
 *    dword_len --- unsigned int, binary code length in DWORD (32-bit)
 *  Output:
 *    int       --- 0:    Success
 *                  else: Error Code
 */
/*
 *  Description:
 *    Download PPE firmware binary code.
 *  Input:
 *    pp32      --- int, which pp32 core
 *    src       --- u32 *, binary code buffer
 *    dword_len --- unsigned int, binary code length in DWORD (32-bit)
 *  Output:
 *    int       --- IFX_SUCCESS:    Success
 *                  else:           Error Code
 */
static INLINE int pp32_download_code(int pp32, const u32 *code_src,
		unsigned int code_dword_len, const u32 *data_src,
		unsigned int data_dword_len) {
	unsigned int clr, set;
	volatile u32 *dest;

	if (code_src == 0 || ((unsigned long) code_src & 0x03) != 0 || data_src == 0
			|| ((unsigned long) data_src & 0x03) != 0)
		return IFX_ERROR;

	clr = pp32 ? 0xF0 : 0x0F;
	if (code_dword_len <= CDM_CODE_MEMORYn_DWLEN(0))
		set = pp32 ? (3 << 6) : (2 << 2);
	else
		set = 0x00;
	SW_WRITE_REG32_MASK(clr, set, CDM_CFG);

	/*  copy code   */
	dest = CDM_CODE_MEMORY(pp32, 0);
	while (code_dword_len-- > 0)
		SW_WRITE_REG32(*code_src++, dest++);

	/*  copy data   */
	dest = CDM_DATA_MEMORY(pp32, 0);
	while (data_dword_len-- > 0)
		SW_WRITE_REG32(*data_src++, dest++);

	return IFX_SUCCESS;
}

/*
 *  Description:
 *    Initialize and start up PP32.
 *  Input:
 *    none
 *  Output:
 *    int  --- IFX_SUCCESS: Success
 *             else:        Error Code
 */
static INLINE int pp32_start(int pp32, int is_e1) {
	unsigned int mask = 1 << (pp32 << 4);
	int ret;

	/*  download firmware   */
	if (is_e1)
		ret = pp32_download_code(
				pp32,
				vr9_e1_binary_code,
				sizeof(vr9_e1_binary_code )
						/ sizeof(*vr9_e1_binary_code),
				vr9_e1_binary_data,
				sizeof(vr9_e1_binary_data)
						/ sizeof(*vr9_e1_binary_data));
	else
		ret = pp32_download_code(pp32, vr9_d5_binary_code,
				sizeof(vr9_d5_binary_code) / sizeof(*vr9_d5_binary_code),
				vr9_d5_binary_data,
				sizeof(vr9_d5_binary_data) / sizeof(*vr9_d5_binary_data));
	if (ret != IFX_SUCCESS)
		return ret;

	/*  run PP32    */
	*PP32_FREEZE &= ~mask;

	/*  idle for a while to let PP32 init itself    */
	//udelay(10);
	mdelay(1);

	/*  capture version number  */
	g_fw_ver[pp32] = *FW_VER_ID;

	return IFX_SUCCESS;
}

/*
 *  Description:
 *    Halt PP32.
 *  Input:
 *    none
 *  Output:
 *    none
 */
static INLINE void pp32_stop(int pp32) {
	unsigned int mask = 1 << (pp32 << 4);

	/*  halt PP32   */
	*PP32_FREEZE |= mask;
}

static INLINE int init_local_variables(void) {
	unsigned int i, j;

	/*  initialize semaphore used by open and close */
	sema_init(&g_sem, 1);

	if (ethwan == 1)
		g_eth_wan_mode = 2;
	else if (ethwan == 2)
		g_eth_wan_mode = 3;

	err("[%s] g_eth_wan_mode=%d\n", __FUNCTION__, g_eth_wan_mode);
	if (wanitf == ~0 || wanitf == 0) {
		switch (g_eth_wan_mode) {
		case 0: /*  DSL WAN     */
			g_wan_itf = 1 << 7;
			break;
		case 2: /*  Mixed Mode  */
			g_wan_itf = 1 << 0;
			break;
		case 3: /*  ETH WAN     */
			g_wan_itf = 1 << 1;
			break;
		}
	} else {
		g_wan_itf = wanitf;
		switch (g_eth_wan_mode) {
		case 0: /*  DSL WAN     */
			g_wan_itf &= 0xF8;
			g_wan_itf |= 1 << 7;
			break; //  ETH0/1 can not be WAN
		case 2: /*  Mixed Mode  */
			g_wan_itf &= 0x03; //  only ETH0/1 support mixed mode
			if (g_wan_itf == 0x03 || g_wan_itf == 0) { //both ETH0/1 in mixed mode, reset to eth0 mixed mode
				g_wan_itf = 1;
			}
			break;
		case 3: /*  ETH WAN     */
			g_wan_itf &= 0x7B; //  DSL disabled in ETH WAN mode
			if ((g_wan_itf & 0x03) == 0x03) { //both ETH0/1 in WAN mode, remove eth0 wan setting
				g_wan_itf &= ~0x01;
			}
			if ((g_wan_itf & 0x03) == 0) {
				g_wan_itf |= 1 << 1;
			}
			break;
		}

	}

	g_ipv6_acc_en = ipv6_acc_en ? 1 : 0;

	if (g_eth_wan_mode == 0)
		g_wanqos_en = wanqos_en ? wanqos_en : MAX_WAN_QOS_QUEUES;
	else if (g_eth_wan_mode == 3)
		g_wanqos_en = wanqos_en ? wanqos_en : 0;
	if (g_wanqos_en > MAX_WAN_QOS_QUEUES)
		g_wanqos_en = MAX_WAN_QOS_QUEUES;


	g_mailbox_signal_mask = g_wanqos_en ? CPU_TO_WAN_SWAP_SIG : 0;
	if (g_eth_wan_mode == 0)
		g_mailbox_signal_mask |= WAN_RX_SIG(0);

	for (i = 0; i < NUM_ENTITY(g_queue_gamma_map); i++) {
		g_queue_gamma_map[i] = queue_gamma_map[i] & ((1 << g_wanqos_en) - 1);
		for (j = 0; j < i; j++)
			g_queue_gamma_map[i] &= ~g_queue_gamma_map[j];
	}

	err("g_wan_itf=%#x, g_wanqos_en=%d\n", g_wan_itf, g_wanqos_en);
	{
		unsigned int max_packet_priority = NUM_ENTITY(g_eth_prio_queue_map); //  assume only 8 levels are used in Linux
		int tx_num_q;
		int q_step, q_accum, p_step;

        tx_num_q = (g_eth_wan_mode == 3 /* ETH WAN */ && g_wanqos_en) ? __ETH_WAN_TX_QUEUE_NUM : __ETH_VIRTUAL_TX_QUEUE_NUM;

        q_step = tx_num_q - 1;
		p_step = max_packet_priority - 1;
		for (j = 0, q_accum = 0; j < max_packet_priority; j++, q_accum += q_step){
            g_eth_prio_queue_map[j] = (q_accum + (p_step >> 1)) / p_step;
        	dbg( "g_eth_prio_queue_map[%d]=%d\n", j, g_eth_prio_queue_map[j]);
		}
	}


	if (g_eth_wan_mode == 0 /* DSL WAN */) {
		unsigned int max_packet_priority = NUM_ENTITY(g_ptm_prio_queue_map);
		int tx_num_q;
		int q_step, q_accum, p_step;

		tx_num_q = __ETH_WAN_TX_QUEUE_NUM;
		q_step = tx_num_q - 1;
		p_step = max_packet_priority - 1;
		for (j = 0, q_accum = 0; j < max_packet_priority; j++, q_accum += q_step){
			// g_ptm_prio_queue_map[j] = q_step - (q_accum + (p_step >> 1)) / p_step;
			g_ptm_prio_queue_map[j] = (q_accum + (p_step >> 1)) / p_step;
			dbg("g_ptm_prio_queue_map[%d] = %d", j,g_ptm_prio_queue_map[j] );
		}
	}

    for ( i = 0; i < NUM_ENTITY(g_wan_queue_class_map); i++ )
        g_wan_queue_class_map[i] = (NUM_ENTITY(g_wan_queue_class_map) - 1 - i) % SWITCH_CLASS_NUM;

	return 0;
}

static INLINE void clear_local_variables(void) {
}

#if defined(ENABLE_FW_MODULE_TEST) && ENABLE_FW_MODULE_TEST

#include "cfg_arrays_d5.h"
#include "cfg_arrays_d5_ipv6.h"

unsigned int ppe_fw_mode = 0; // 0: normal mode, 1: mix mode
unsigned int ppe_wan_hash_algo = 0;// 0: using algo 0, 1: using algo 1
unsigned int acc_proto = 0;// 0: UDP, 1:TCP
unsigned int ipv6_en = 0;// 0: IPv6 disabled, 1: IPv6 enabled

void setup_acc_action_tbl(void)
{
	unsigned int idx;

	if (acc_proto == 0) {
		// clear bit16 of dw3 of each action entry
		unsigned long udp_mask = ~ (1 << 16);

		// setup for Hash entry
		idx = 3;
		if (ipv6_en == 0) {
			while (idx < sizeof(lan_uc_act_tbl_normal_mode_cfg)/sizeof(unsigned long)) {
				lan_uc_act_tbl_normal_mode_cfg[idx] &= udp_mask;
				lan_uc_act_tbl_mixed_mode_cfg[idx] &= udp_mask;

				wan_uc_act_tbl_alo_0_cfg[idx] &= udp_mask;
				wan_uc_act_tbl_alo_1_cfg[idx] &= udp_mask;
				idx += 6;
			}
		} else {
			while (idx < sizeof(lan_uc_act_tbl_normal_mode_cfg_ipv6)/sizeof(unsigned long)) {
				lan_uc_act_tbl_normal_mode_cfg_ipv6[idx] &= udp_mask;
				lan_uc_act_tbl_mixed_mode_cfg_ipv6[idx] &= udp_mask;

				wan_uc_act_tbl_alo_0_cfg_ipv6[idx] &= udp_mask;
				wan_uc_act_tbl_alo_1_cfg_ipv6[idx] &= udp_mask;
				idx += 6;
			}
		}

		// setup for Collsion entry
		idx = 3;
		if (ipv6_en == 0) {
			while (idx < sizeof(lan_uc_col_act_tbl_normal_mode_cfg)/sizeof(unsigned long)) {
				lan_uc_col_act_tbl_normal_mode_cfg[idx] &= udp_mask;
				lan_uc_col_act_tbl_mixed_mode_cfg[idx] &= udp_mask;
				wan_uc_col_act_tbl_cfg[idx] &= udp_mask;
				idx += 6;
			}
		} else {
			while (idx < sizeof(lan_uc_col_act_tbl_normal_mode_cfg_ipv6)/sizeof(unsigned long)) {
				lan_uc_col_act_tbl_normal_mode_cfg_ipv6[idx] &= udp_mask;
				lan_uc_col_act_tbl_mixed_mode_cfg_ipv6[idx] &= udp_mask;
				wan_uc_col_act_tbl_cfg_ipv6[idx] &= udp_mask;
				idx += 6;
			}
		}
	}
	else {
		// set bit16 of dw3 of each action entry
		unsigned long tcp_mask = (1 << 16);

		// setup for Hash entry
		idx = 3;
		if (ipv6_en == 0) {
			while (idx < sizeof(lan_uc_act_tbl_normal_mode_cfg)/sizeof(unsigned long)) {
				lan_uc_act_tbl_normal_mode_cfg[idx] |= tcp_mask;
				lan_uc_act_tbl_mixed_mode_cfg[idx] |= tcp_mask;

				wan_uc_act_tbl_alo_0_cfg[idx] |= tcp_mask;
				wan_uc_act_tbl_alo_1_cfg[idx] |= tcp_mask;
				idx += 6;
			}
		} else {
			while (idx < sizeof(lan_uc_act_tbl_normal_mode_cfg_ipv6)/sizeof(unsigned long)) {
				lan_uc_act_tbl_normal_mode_cfg_ipv6[idx] |= tcp_mask;
				lan_uc_act_tbl_mixed_mode_cfg_ipv6[idx] |= tcp_mask;

				wan_uc_act_tbl_alo_0_cfg_ipv6[idx] |= tcp_mask;
				wan_uc_act_tbl_alo_1_cfg_ipv6[idx] |= tcp_mask;
				idx += 6;
			}
		}

		// setup for Collsion entry
		idx = 3;
		if (ipv6_en == 0) {
			while (idx < sizeof(lan_uc_col_act_tbl_normal_mode_cfg)/sizeof(unsigned long)) {
				lan_uc_col_act_tbl_normal_mode_cfg[idx] |= tcp_mask;
				lan_uc_col_act_tbl_mixed_mode_cfg[idx] |= tcp_mask;
				wan_uc_col_act_tbl_cfg[idx] |= tcp_mask;
				idx += 6;
			}
		} else {
			while (idx < sizeof(lan_uc_col_act_tbl_normal_mode_cfg_ipv6)/sizeof(unsigned long)) {
				lan_uc_col_act_tbl_normal_mode_cfg_ipv6[idx] |= tcp_mask;
				lan_uc_col_act_tbl_mixed_mode_cfg_ipv6[idx] |= tcp_mask;
				wan_uc_col_act_tbl_cfg_ipv6[idx] |= tcp_mask;
				idx += 6;
			}
		}
	}
}

void init_acc_tables(void)
{
	setup_acc_action_tbl();

	// init MAC table
	memcpy((void *)ROUT_MAC_CFG_TBL(0), rout_mac_cfg, sizeof(rout_mac_cfg));

	// PPPoE table
	memcpy((void *)PPPOE_CFG_TBL(0), pppoe_cfg, sizeof(pppoe_cfg));

	// Outer VLAN Config
	memcpy((void *)OUTER_VLAN_TBL(0), outer_vlan_cfg, sizeof(outer_vlan_cfg));

	if (ipv6_en == 0) {
		//Use Original Data Structures

		// lan uc_cmp_tbl_cfg (Hash) and collision
		memcpy((void *)ROUT_LAN_HASH_CMP_TBL(0), lan_uc_cmp_tbl_cfg, sizeof(lan_uc_cmp_tbl_cfg));
		memcpy((void *)ROUT_LAN_COLL_CMP_TBL(0), lan_uc_col_cmp_tbl_cfg, sizeof(lan_uc_col_cmp_tbl_cfg));

		// lan action
		if(ppe_fw_mode == 0) {
			// normal mode
			memcpy((void *)ROUT_LAN_HASH_ACT_TBL(0), lan_uc_act_tbl_normal_mode_cfg, sizeof(lan_uc_act_tbl_normal_mode_cfg));
			memcpy((void *)ROUT_LAN_COLL_ACT_TBL(0), lan_uc_col_act_tbl_normal_mode_cfg, sizeof(lan_uc_col_act_tbl_normal_mode_cfg));
		} else {
			// mix mode
			memcpy((void *)ROUT_LAN_HASH_ACT_TBL(0), lan_uc_act_tbl_mixed_mode_cfg, sizeof(lan_uc_act_tbl_mixed_mode_cfg));
			memcpy((void *)ROUT_LAN_COLL_ACT_TBL(0), lan_uc_col_act_tbl_mixed_mode_cfg, sizeof(lan_uc_col_act_tbl_mixed_mode_cfg));
		}

		// wan hash cmp anc act table
		if ( ppe_wan_hash_algo == 0) {
			// WAN algo 0
			memcpy((void *)ROUT_WAN_HASH_CMP_TBL(0), wan_uc_cmp_tbl_alo_0_cfg, sizeof(wan_uc_cmp_tbl_alo_0_cfg));
			memcpy((void *)ROUT_WAN_HASH_ACT_TBL(0), wan_uc_act_tbl_alo_0_cfg, sizeof(wan_uc_act_tbl_alo_0_cfg));
		} else {
			// WAN algo 1
			memcpy((void *)ROUT_WAN_HASH_CMP_TBL(0), wan_uc_cmp_tbl_alo_1_cfg, sizeof(wan_uc_cmp_tbl_alo_1_cfg));
			memcpy((void *)ROUT_WAN_HASH_ACT_TBL(0), wan_uc_act_tbl_alo_1_cfg, sizeof(wan_uc_act_tbl_alo_1_cfg));
		}

		// wan collision cmp and act table
		memcpy((void *)ROUT_WAN_COLL_CMP_TBL(0), wan_uc_col_cmp_tbl_cfg, sizeof(wan_uc_col_cmp_tbl_cfg));
		memcpy((void *)ROUT_WAN_COLL_ACT_TBL(0), wan_uc_col_act_tbl_cfg, sizeof(wan_uc_col_act_tbl_cfg));

		// wan multicast cmp and act table
		memcpy((void *)ROUT_WAN_MC_CMP_TBL(0), wan_mc_cmp_tbl_cfg, sizeof(wan_mc_cmp_tbl_cfg));
		memcpy((void *)ROUT_WAN_MC_ACT_TBL(0), wan_mc_act_tbl_cfg, sizeof(wan_mc_act_tbl_cfg));
	} else {
		//Use IPv6 Data Structures

		// lan uc_cmp_tbl_cfg (Hash) and collision
		memcpy((void *)ROUT_LAN_HASH_CMP_TBL(0), lan_uc_cmp_tbl_cfg_ipv6, sizeof(lan_uc_cmp_tbl_cfg_ipv6));
		memcpy((void *)ROUT_LAN_COLL_CMP_TBL(0), lan_uc_col_cmp_tbl_cfg_ipv6, sizeof(lan_uc_col_cmp_tbl_cfg_ipv6));

		// lan action
		if(ppe_fw_mode == 0) {
			// normal mode
			memcpy((void *)ROUT_LAN_HASH_ACT_TBL(0), lan_uc_act_tbl_normal_mode_cfg_ipv6, sizeof(lan_uc_act_tbl_normal_mode_cfg_ipv6));
			memcpy((void *)ROUT_LAN_COLL_ACT_TBL(0), lan_uc_col_act_tbl_normal_mode_cfg_ipv6, sizeof(lan_uc_col_act_tbl_normal_mode_cfg_ipv6));
		} else {
			// mix mode
			memcpy((void *)ROUT_LAN_HASH_ACT_TBL(0), lan_uc_act_tbl_mixed_mode_cfg_ipv6, sizeof(lan_uc_act_tbl_mixed_mode_cfg_ipv6));
			memcpy((void *)ROUT_LAN_COLL_ACT_TBL(0), lan_uc_col_act_tbl_mixed_mode_cfg_ipv6, sizeof(lan_uc_col_act_tbl_mixed_mode_cfg_ipv6));
		}

		// wan hash cmp anc act table
		if ( ppe_wan_hash_algo == 0) {
			// WAN algo 0
			memcpy((void *)ROUT_WAN_HASH_CMP_TBL(0), wan_uc_cmp_tbl_alo_0_cfg_ipv6, sizeof(wan_uc_cmp_tbl_alo_0_cfg_ipv6));
			memcpy((void *)ROUT_WAN_HASH_ACT_TBL(0), wan_uc_act_tbl_alo_0_cfg_ipv6, sizeof(wan_uc_act_tbl_alo_0_cfg_ipv6));
		} else {
			// WAN algo 1
			memcpy((void *)ROUT_WAN_HASH_CMP_TBL(0), wan_uc_cmp_tbl_alo_1_cfg_ipv6, sizeof(wan_uc_cmp_tbl_alo_1_cfg_ipv6));
			memcpy((void *)ROUT_WAN_HASH_ACT_TBL(0), wan_uc_act_tbl_alo_1_cfg_ipv6, sizeof(wan_uc_act_tbl_alo_1_cfg_ipv6));
		}

		// wan collision cmp and act table
		memcpy((void *)ROUT_WAN_COLL_CMP_TBL(0), wan_uc_col_cmp_tbl_cfg_ipv6, sizeof(wan_uc_col_cmp_tbl_cfg_ipv6));
		memcpy((void *)ROUT_WAN_COLL_ACT_TBL(0), wan_uc_col_act_tbl_cfg_ipv6, sizeof(wan_uc_col_act_tbl_cfg_ipv6));

		// wan multicast cmp and act table
		memcpy((void *)ROUT_WAN_MC_CMP_TBL(0), wan_mc_cmp_tbl_cfg_ipv6, sizeof(wan_mc_cmp_tbl_cfg_ipv6));
		memcpy((void *)ROUT_WAN_MC_ACT_TBL(0), wan_mc_act_tbl_cfg_ipv6, sizeof(wan_mc_act_tbl_cfg_ipv6));
	}
}

#endif  //  #if defined(ENABLE_FW_MODULE_TEST) && ENABLE_FW_MODULE_TEST


static INLINE void init_ptm_tables(void)
{
    struct cfg_std_data_len cfg_std_data_len = {0};
    struct tx_qos_cfg tx_qos_cfg = {0};
    struct psave_cfg psave_cfg = {0};
    struct eg_bwctrl_cfg eg_bwctrl_cfg = {0};
    struct test_mode test_mode = {0};
    struct rx_bc_cfg rx_bc_cfg = {0};
    struct tx_bc_cfg tx_bc_cfg = {0};
    struct gpio_mode gpio_mode = {0};
    struct gpio_wm_cfg gpio_wm_cfg = {0};
    struct rx_gamma_itf_cfg rx_gamma_itf_cfg = {0};
    struct tx_gamma_itf_cfg tx_gamma_itf_cfg = {0};
        unsigned int i;

    cfg_std_data_len.byte_off = g_dsl_bonding ? 2 : 0;  //  this field replaces byte_off in rx descriptor of VDSL ingress
    cfg_std_data_len.data_len = DMA_PACKET_SIZE;
    *CFG_STD_DATA_LEN = cfg_std_data_len;

    tx_qos_cfg.time_tick    = cgu_get_pp32_clock() / 62500; //  16 * (cgu_get_pp32_clock() / 1000000)
    tx_qos_cfg.overhd_bytes = 24;   //  8-byte preamble, 12-byte inter-frame gap, E5 specific: 2-byte TC CRC, 1-byte SoF, 1 byte EOF (Cj)
    tx_qos_cfg.eth1_eg_qnum = __ETH_WAN_TX_QUEUE_NUM;
    tx_qos_cfg.eth1_burst_chk = 1;
    tx_qos_cfg.eth1_qss     = 0;
    tx_qos_cfg.shape_en     = 0;    //  disable
    tx_qos_cfg.wfq_en       = 0;    //  strict priority
    *TX_QOS_CFG_DYNAMIC = tx_qos_cfg;

    psave_cfg.start_state   = 0;
    psave_cfg.sleep_en      = 1;    //  enable sleep mode
    *PSAVE_CFG = psave_cfg;

    eg_bwctrl_cfg.fdesc_wm  = 16;
    eg_bwctrl_cfg.class_len = 128;
    *EG_BWCTRL_CFG = eg_bwctrl_cfg;

    test_mode.mib_clear_mode    = 0;
    test_mode.test_mode         = 0;
    *TEST_MODE = test_mode;

    //*GPIO_ADDR = (unsigned int)IFX_GPIO_P0_OUT;
    *GPIO_ADDR = (unsigned int)0x00000000;  //  disabled by default

    gpio_mode.gpio_bit_bc1 = 2;
    gpio_mode.gpio_bit_bc0 = 1;
    gpio_mode.gpio_bc1_en  = 0;
    gpio_mode.gpio_bc0_en  = 0;
    *GPIO_MODE = gpio_mode;

    gpio_wm_cfg.stop_wm_bc1  = 2;
    gpio_wm_cfg.start_wm_bc1 = 4;
    gpio_wm_cfg.stop_wm_bc0  = 2;
    gpio_wm_cfg.start_wm_bc0 = 4;
    *GPIO_WM_CFG = gpio_wm_cfg;

    rx_bc_cfg.local_state   = 0;
    rx_bc_cfg.remote_state  = 0;
    rx_bc_cfg.to_false_th   = 7;
    rx_bc_cfg.to_looking_th = 3;
    *RX_BC_CFG(0) = rx_bc_cfg;
    *RX_BC_CFG(1) = rx_bc_cfg;

    tx_bc_cfg.fill_wm   = 2;
    tx_bc_cfg.uflw_wm   = 2;
    *TX_BC_CFG(0) = tx_bc_cfg;
    *TX_BC_CFG(1) = tx_bc_cfg;

    rx_gamma_itf_cfg.receive_state      = 0;
    rx_gamma_itf_cfg.rx_min_len         = 64;
    rx_gamma_itf_cfg.rx_pad_en          = 1;
    rx_gamma_itf_cfg.rx_eth_fcs_ver_dis = g_dsl_bonding ? 1 : 0;    //  disable Ethernet FCS verification during bonding
    rx_gamma_itf_cfg.rx_rm_eth_fcs      = g_dsl_bonding ? 0 : 1;    //  disable Ethernet FCS verification during bonding
    rx_gamma_itf_cfg.rx_tc_crc_ver_dis  = 0;
    rx_gamma_itf_cfg.rx_tc_crc_size     = 1;
    rx_gamma_itf_cfg.rx_eth_fcs_result  = 0xC704DD7B;
    rx_gamma_itf_cfg.rx_tc_crc_result   = 0x1D0F1D0F;
    rx_gamma_itf_cfg.rx_crc_cfg         = 0x2500;
    rx_gamma_itf_cfg.rx_eth_fcs_init_value  = 0xFFFFFFFF;
    rx_gamma_itf_cfg.rx_tc_crc_init_value   = 0x0000FFFF;
    rx_gamma_itf_cfg.rx_max_len_sel     = 0;
    rx_gamma_itf_cfg.rx_edit_num2       = 0;
    rx_gamma_itf_cfg.rx_edit_pos2       = 0;
    rx_gamma_itf_cfg.rx_edit_type2      = 0;
    rx_gamma_itf_cfg.rx_edit_en2        = 0;
    rx_gamma_itf_cfg.rx_edit_num1       = g_dsl_bonding ? 0 : 4;    //  no PMAC header insertion during bonding
    rx_gamma_itf_cfg.rx_edit_pos1       = 0;
    rx_gamma_itf_cfg.rx_edit_type1      = g_dsl_bonding ? 0 : 1;    //  no PMAC header insertion during bonding
    rx_gamma_itf_cfg.rx_edit_en1        = g_dsl_bonding ? 0 : 1;    //  no PMAC header insertion during bonding
    rx_gamma_itf_cfg.rx_inserted_bytes_1l   = 0x00000007;   //  E5: byte swap of value 0x07000000
    rx_gamma_itf_cfg.rx_inserted_bytes_1h   = 0;
    rx_gamma_itf_cfg.rx_inserted_bytes_2l   = 0;
    rx_gamma_itf_cfg.rx_inserted_bytes_2h   = 0;
    rx_gamma_itf_cfg.rx_len_adj         = -2;               //  E5: remove TC CRC
    for ( i = 0; i < 4; i++ )
        *RX_GAMMA_ITF_CFG(i) = rx_gamma_itf_cfg;

    tx_gamma_itf_cfg.tx_len_adj         = g_dsl_bonding ? 2 : 6;
    tx_gamma_itf_cfg.tx_crc_off_adj     = 6;
    tx_gamma_itf_cfg.tx_min_len         = 0;
    tx_gamma_itf_cfg.tx_eth_fcs_gen_dis = g_dsl_bonding ? 1 : 0;
    tx_gamma_itf_cfg.tx_tc_crc_size     = 1;
    tx_gamma_itf_cfg.tx_crc_cfg         = g_dsl_bonding ? 0x2F02 : 0x2F00;
    tx_gamma_itf_cfg.tx_eth_fcs_init_value  = 0xFFFFFFFF;
    tx_gamma_itf_cfg.tx_tc_crc_init_value   = 0x0000FFFF;
    for ( i = 0; i < NUM_ENTITY(g_queue_gamma_map); i++ ) {
        tx_gamma_itf_cfg.queue_mapping = g_queue_gamma_map[i];
        *TX_GAMMA_ITF_CFG(i) = tx_gamma_itf_cfg;
    }

    init_qos_queue();

    //  default TX queue QoS config is all ZERO

    //  TX Ctrl K Table
    IFX_REG_W32(0x90111293, TX_CTRL_K_TABLE(0));
    IFX_REG_W32(0x14959617, TX_CTRL_K_TABLE(1));
    IFX_REG_W32(0x18999A1B, TX_CTRL_K_TABLE(2));
    IFX_REG_W32(0x9C1D1E9F, TX_CTRL_K_TABLE(3));
    IFX_REG_W32(0xA02122A3, TX_CTRL_K_TABLE(4));
    IFX_REG_W32(0x24A5A627, TX_CTRL_K_TABLE(5));
    IFX_REG_W32(0x28A9AA2B, TX_CTRL_K_TABLE(6));
    IFX_REG_W32(0xAC2D2EAF, TX_CTRL_K_TABLE(7));
    IFX_REG_W32(0x30B1B233, TX_CTRL_K_TABLE(8));
    IFX_REG_W32(0xB43536B7, TX_CTRL_K_TABLE(9));
    IFX_REG_W32(0xB8393ABB, TX_CTRL_K_TABLE(10));
    IFX_REG_W32(0x3CBDBE3F, TX_CTRL_K_TABLE(11));
    IFX_REG_W32(0xC04142C3, TX_CTRL_K_TABLE(12));
    IFX_REG_W32(0x44C5C647, TX_CTRL_K_TABLE(13));
    IFX_REG_W32(0x48C9CA4B, TX_CTRL_K_TABLE(14));
    IFX_REG_W32(0xCC4D4ECF, TX_CTRL_K_TABLE(15));

    //  Bug workaround: US packet go to wrong queue issue for offchip bonding.
    //  For offchip bonding,
    //    SM needs write access to __DPLUS_RX_DSLWAN_FP_QID_BASE (0x5080) on BM.
    //    FPI address of base address of __DPLUS_RX_DSLWAN_FP_QID_BASE (0x5000) will be programmed in __FP_QID_BASE_PDMA_BAR on both SM and BM.
    //    Offset of __DPLUS_RX_DSLWAN_FP_QID_BASE (0x0080) is hardcoded in firmware.
    //    The value on SM must be well calculated as below, the value on BM is a flag to indicate workaround activated.
    //  Calculation:
    //    PCI host outbound memory
    //    PCI Host      PCI Device  Size
    //    0xB8000000    0x1E000000  16MB
    //    0x1E200000 is the location of SB, in order to cross-pci access SB, it is 0xB8200000,
    //    since 0x5000 is in SB_RAM_BLOCK3, FPI address=0xB000, 0xB000 * 4 = 0x2C000,
    //    address = 0xB8200000 + 0x2C000 = 0xB822C000
    if ( g_dsl_bonding == 2 )
        *FP_QID_BASE_PDMA_BAR = (unsigned int)SB_BUFFER_BOND(1, 0x5000);

    *__DPLUS_QID_CONF_PTR  = 2; //Qid configured by FW
}

static INLINE void init_ptm_bonding_tables(void)
{
    struct bond_conf bond_conf = {0};
    struct bg_map bg_map = {0};
    struct b1_ds_ll_ctxt b1_ds_ll_ctxt = {0};
    int i;

    if ( g_dsl_bonding == 1 )   //  on chip bonding
    {
        *BM_BS_MBOX_IGU6_ISRS = 0x7220;
        *BM_BM_MBOX_IGU6_ISRS = 0x7220;

        // IGU4: linu up/down status (i.e., in showtime, not in showtime)
        // IGU5: gif on_off status
        *LINK_UP_NOTIFY_ADDR   = 0x7218;    // __MBOX_IGU4_ISRS
        *LINK_DOWN_NOTIFY_ADDR = 0x7219;    //__MBOX_IGU4_ISRC
        // Although only IGU4 is configed, FW will infer gif_on_of_notify_addr
        // based on LINK_UP_NOTIFY_ADDR
        // *GIF_ON_NOTIFY_ADDR = 0x721C     // __MBOX_IGU5_ISRS
        // *GIF_OFF_NOTIFY_ADDR = 0x721D    //__MBOX_IGU5_ISRC


        bond_conf.max_frag_size = 0x200;
        bond_conf.polling_ctrl_cnt = 2;
        bond_conf.dplus_fp_fcs_en  = 0x1;
        bond_conf.bg_num        = 0x2;
        bond_conf.bond_mode     = L1_PTM_TC_BOND_MODE;
        bond_conf.e1_bond_en    = 0x1;
        bond_conf.d5_acc_dis    = 0x0;
        bond_conf.d5_b1_en      = 0x1;
        *BOND_CONF = bond_conf;

        bg_map.map3 = 0x00;
        bg_map.map2 = 0x00;
        bg_map.map1 = 0xFF;
        bg_map.map0 = 0x00;
        *US_BG_QMAP = bg_map;   //  upstream bonding group to queue map

        bg_map.map3 = 0x00;
        bg_map.map2 = 0x00;
        bg_map.map1 = 0x05;
        bg_map.map0 = 0x0A;
        *US_BG_GMAP = bg_map;   //  upstream bonding group to gamma interface map
        *DS_BG_GMAP = bg_map;   //  downstream bonding group to gamma interface map

        b1_ds_ll_ctxt.max_des_num   = MAX_DESC_PER_GAMMA_ITF;
        b1_ds_ll_ctxt.TO_buff_thres = 16; // when free_des_num <= 16, timeout any frag
        b1_ds_ll_ctxt.max_delay = 1000;  //  TODO: fix the number

        *__READ_MIN_CYCLES  = 0xFFFFFFFF;
        *__WRITE_MIN_CYCLES = 0xFFFFFFFF;

        for ( i = 0; i < 9; i++ )
            *B1_DS_LL_CTXT(i) = b1_ds_ll_ctxt;
        B1_DS_LL_CTXT(0)->head_ptr = B1_RX_LINK_LIST_DESC_PPE;
        B1_DS_LL_CTXT(0)->tail_ptr = B1_RX_LINK_LIST_DESC_PPE + (B1_RX_LINK_LIST_DESC_NUM - 1) * 2;
        B1_DS_LL_CTXT(0)->des_num  = B1_RX_LINK_LIST_DESC_NUM;

        for ( i = 0; i < 4; i ++) {
            DS_BG_CTXT_BASE(i)->last_eop = 1;
        }
    }
#ifdef CONFIG_IFX_PPE_E5_OFFCHIP_BONDING
    else
    {
        unsigned int up_addr, down_addr;

        /*
         *  off chip bonding (system master)
         */

        bond_conf.polling_ctrl_cnt  = 2;
        bond_conf.e1_bond_en        = 0x1;
        bond_conf.dplus_fp_fcs_en   = 0x1;
        *BOND_CONF = bond_conf;

        bg_map.map3 = 0x00;
        bg_map.map2 = 0x00;
        bg_map.map1 = 0xFF;
        bg_map.map0 = 0x00;
        *US_BG_QMAP = bg_map;

        bg_map.map3 = 0x00;   //  TODO: 0x44 if app1 two bearer channels
        bg_map.map2 = 0x00;   //  TODO: 0x88 if app1 two bearer channels
        bg_map.map1 = 0x11;   //  TODO: 0x55 if app2
        bg_map.map0 = 0x22;   //  TODO: 0xAA if app2
        *US_BG_GMAP = bg_map;
        *DS_BG_GMAP = bg_map;

        // config E1 to use polling mode due to hardware issue
        // echo device > /proc/eth/dev; echo w 2004 2002 > /proc/eth/mem;
        // echo host > /proc/eth/dev; echo w 2022 2002 > /proc/eth/mem; echo w 2002 1 > /proc/eth/mem;
        *US_FRAG_READY_NOTICE_ADDR = FRAG_READY_NOTICE;
        *SB_BUFFER_BOND(0, FRAG_READY_NOTICE) = 1;

        // IGU4: linu up/down status (i.e., in showtime, not in showtime)
        // IGU5: gif on_off status
        up_addr   = (unsigned int)SB_BUFFER_BOND(1, 0x7218);
        down_addr = (unsigned int)SB_BUFFER_BOND(1, 0x7219);
        *LINK_UP_NOTIFY_ADDR   = up_addr;   // __MBOX_IGU4_ISRS
        *LINK_DOWN_NOTIFY_ADDR = down_addr; // __MBOX_IGU4_ISRC

        // Although only IGU4 is configed, FW will infer gif_on_of_notify_addr
        // based on LINK_UP_NOTIFY_ADDR
        // *GIF_ON_NOTIFY_ADDR  = 0x721C    // __MBOX_IGU5_ISRS
        // *GIF_OFF_NOTIFY_ADDR = 0x721D    // __MBOX_IGU5_ISRC

        /*
         *  off chip bonding (bonding master)
         */

        g_ppe_base_addr = IFX_PPE_ORG(1);

        // config E1 to use notificaion mode
        *BM_BM_MBOX_IGU6_ISRS = 0x7220;

        // config E1 to use polling mode due to hardware issue
        // echo device > /proc/eth/dev; echo w 2004 0x2002 > /proc/eth/mem;
        *BM_BS_MBOX_IGU6_ISRS = FRAG_READY_NOTICE;
        *SB_BUFFER_BOND(1, FRAG_READY_NOTICE) = 1;
        // echo host > /proc/eth/dev; echo w 2022 2002 > /proc/eth/mem; echo w 2002 1 > /proc/eth/mem;

        // IGU4: linu up/down status (i.e., in showtime, not in showtime)
        // IGU5: gif on_off status
        *LINK_UP_NOTIFY_ADDR   = 0x7218;    // __MBOX_IGU4_ISRS
        *LINK_DOWN_NOTIFY_ADDR = 0x7219;    // __MBOX_IGU4_ISRC
        // Although only IGU4 is configed, FW will infer gif_on_of_notify_addr
        // based on LINK_UP_NOTIFY_ADDR
        // *GIF_ON_NOTIFY_ADDR  = 0x721C    // __MBOX_IGU5_ISRS
        // *GIF_OFF_NOTIFY_ADDR = 0x721D    // __MBOX_IGU5_ISRC

        // set BM TTHA.pnu = 80 to avoid under flow issue
        // echo device > /proc/eth/dev; echo w 50a 10050 > /proc/eth/mem
        *FFSM_CFG0 = 0x00010050;
        // set US.under_flow_threshold
        // echo device > /proc/eth/dev; echo w 3ec0 360036 > /proc/eth/mem
        *(volatile unsigned int *)TX_BC_CFG(0) = 0x00360036;    //  overwrite setting done in init_ptm_tables

        bond_conf.max_frag_size = 0x200;
        bond_conf.polling_ctrl_cnt = 2;
        bond_conf.dplus_fp_fcs_en  = 0x1;
        bond_conf.bg_num        = 0x2;  //  TODO: 0x4 if two bearer channels are enabled
        bond_conf.bond_mode     = L1_PTM_TC_BOND_MODE;
        bond_conf.e1_bond_en    = 0x1;
        bond_conf.d5_acc_dis    = 0x1;
        bond_conf.d5_b1_en      = 0x1;
        *BOND_CONF = bond_conf;

        bg_map.map3 = 0x00;
        bg_map.map2 = 0x00;
        bg_map.map1 = 0xFF;
        bg_map.map0 = 0x00;
        *US_BG_QMAP = bg_map;

        bg_map.map3 = 0x00;   //  TODO: 0x44 if app1 two bearer channels
        bg_map.map2 = 0x00;   //  TODO: 0x88 if app1 two bearer channels
        bg_map.map1 = 0x11;   //  TODO: 0x55 if app2
        bg_map.map0 = 0x22;   //  TODO: 0xAA if app2
        *US_BG_GMAP = bg_map;
        *DS_BG_GMAP = bg_map;

        b1_ds_ll_ctxt.max_des_num   = MAX_DESC_PER_GAMMA_ITF;
        b1_ds_ll_ctxt.TO_buff_thres = 16; // when free_des_num <= 16, timeout any frag
        b1_ds_ll_ctxt.max_delay = 1000;  //  TODO: fix the number

        *__READ_MIN_CYCLES  = 0xFFFFFFFF;
        *__WRITE_MIN_CYCLES = 0xFFFFFFFF;

        for ( i = 0; i < 9; i++ )
            *B1_DS_LL_CTXT(i) = b1_ds_ll_ctxt;
        B1_DS_LL_CTXT(0)->head_ptr = B1_RX_LINK_LIST_DESC_PPE;
        B1_DS_LL_CTXT(0)->tail_ptr = B1_RX_LINK_LIST_DESC_PPE + (B1_RX_LINK_LIST_DESC_NUM - 1) * 2;
        B1_DS_LL_CTXT(0)->des_num  = B1_RX_LINK_LIST_DESC_NUM;

        for ( i = 0; i < 4; i ++) {
            DS_BG_CTXT_BASE(i)->last_eop = 1;
        }

        g_ppe_base_addr = IFX_PPE_ORG(0);
    }
#endif
}
static INLINE void init_qos_queue(void)
{
    struct wtx_qos_q_desc_cfg wtx_qos_q_desc_cfg = {0};
    int i;
    int des_num = WAN_TX_DESC_NUM_TOTAL;
    int q_len = WAN_TX_DESC_NUM_TOTAL;
    int equal_div = 0;
    
    for( i = 0; i < min(__ETH_WAN_TX_QUEUE_NUM, MAX_WAN_QOS_QUEUES); i ++){
        if(q_len < qos_queue_len[i] || (q_len - qos_queue_len[i]) < (__ETH_WAN_TX_QUEUE_NUM - i - 1)){
            q_len = -1;
            err("total qos queue descriptor number is large than max number(%d)", des_num);
            err("q_len: %d, queue len: %d, qid: %d", q_len, qos_queue_len[i], i);
            break;
        }
        q_len -= qos_queue_len[i];
    }
    
    if(g_dsl_bonding || q_len == -1){
        for( i = 0; i < min(__ETH_WAN_TX_QUEUE_NUM, MAX_WAN_QOS_QUEUES); i ++){
            wtx_qos_q_desc_cfg.length = WAN_TX_DESC_NUM(i);
            wtx_qos_q_desc_cfg.addr   = __ETH_WAN_TX_DESC_BASE(i);
            *WTX_QOS_Q_DESC_CFG(i) = wtx_qos_q_desc_cfg;
        }
    }else{      
        q_len = 0;
        for( i = 0; i < min(__ETH_WAN_TX_QUEUE_NUM, MAX_WAN_QOS_QUEUES); i ++){
            if(!equal_div && qos_queue_len[i] == 0){
                equal_div = (des_num - q_len)/(__ETH_WAN_TX_QUEUE_NUM - i) < 256 ? (des_num - q_len)/(__ETH_WAN_TX_QUEUE_NUM - i) : 255; 
            }

            if(!equal_div){
                wtx_qos_q_desc_cfg.length = qos_queue_len[i] < 256 ? qos_queue_len[i] : 255;
            }else{
                wtx_qos_q_desc_cfg.length = equal_div;
            }
            wtx_qos_q_desc_cfg.addr   = WAN_QOS_DESC_BASE + (q_len << 1);
            q_len += wtx_qos_q_desc_cfg.length;
            *WTX_QOS_Q_DESC_CFG(i) = wtx_qos_q_desc_cfg;
        }
    }

    return;
}

static INLINE void init_qos(void)
{
    struct tx_qos_cfg tx_qos_cfg = {0};
    struct cfg_std_data_len cfg_std_data_len = {0};
    
    init_qos_queue();

    tx_qos_cfg.time_tick    = cgu_get_pp32_clock() / 62500; //  16 * (cgu_get_pp32_clock() / 1000000)
    tx_qos_cfg.eth1_eg_qnum = __ETH_WAN_TX_QUEUE_NUM;
    tx_qos_cfg.eth1_qss     = 1;
    tx_qos_cfg.eth1_burst_chk = 1;
    *TX_QOS_CFG_DYNAMIC = tx_qos_cfg;

    cfg_std_data_len.byte_off = 0;
    cfg_std_data_len.data_len = DMA_PACKET_SIZE;

    *CFG_STD_DATA_LEN = cfg_std_data_len;
}
static INLINE void init_communication_data_structures(
		int fwcode __attribute__ ((unused))) {
	struct eth_ports_cfg eth_ports_cfg = { 0 };
	struct rout_tbl_cfg lan_rout_tbl_cfg = { 0 };
	struct rout_tbl_cfg wan_rout_tbl_cfg = { 0 };
	struct gen_mode_cfg1 gen_mode_cfg1 = { 0 };    
	struct ps_mc_cfg ps_mc_gencfg3={0};
    unsigned int i, j;

	*CDM_CFG = CDM_CFG_RAM1_SET(0x00) | CDM_CFG_RAM0_SET(0x00);

    udelay(10);

	*PSEUDO_IPv4_BASE_ADDR = 0xFFFFFF00;

	eth_ports_cfg.wan_vlanid_hi = 0;
	eth_ports_cfg.wan_vlanid_lo = 0;
	eth_ports_cfg.eth0_type = 0; //  lan
	*ETH_PORTS_CFG = eth_ports_cfg;

	lan_rout_tbl_cfg.rout_num = MAX_COLLISION_ROUTING_ENTRIES;
	lan_rout_tbl_cfg.ttl_en = 1;
	lan_rout_tbl_cfg.rout_drop = 0;
	*LAN_ROUT_TBL_CFG = lan_rout_tbl_cfg;

	wan_rout_tbl_cfg.rout_num = MAX_COLLISION_ROUTING_ENTRIES;
	wan_rout_tbl_cfg.wan_rout_mc_num = 64;
	wan_rout_tbl_cfg.ttl_en = 1;
	wan_rout_tbl_cfg.wan_rout_mc_drop = 0;
	wan_rout_tbl_cfg.rout_drop = 0;
	*WAN_ROUT_TBL_CFG = wan_rout_tbl_cfg;

	gen_mode_cfg1.app2_indirect = 1; //  0: means DMA RX ch 3 is dedicated for CPU1 process
	gen_mode_cfg1.us_indirect = 0;
	gen_mode_cfg1.us_early_discard_en = 0;
#if defined(CONFIG_IFX_PPA_MFE)
	gen_mode_cfg1.classification_num = MAX_CLASSIFICATION_ENTRIES;
#else
	gen_mode_cfg1.classification_num = 0;
#endif
	gen_mode_cfg1.ipv6_rout_num = g_ipv6_acc_en ? 2 : 0;
	gen_mode_cfg1.session_ctl_en = 0;
	gen_mode_cfg1.wan_hash_alg = 0;
#if defined(CONFIG_IFX_PPA_MFE)
	gen_mode_cfg1.brg_class_en = 1;
#else
	gen_mode_cfg1.brg_class_en = 0;
#endif
	gen_mode_cfg1.ipv4_mc_acc_mode = 0;
	gen_mode_cfg1.ipv6_acc_en = g_ipv6_acc_en;
	gen_mode_cfg1.wan_acc_en = 1;
	gen_mode_cfg1.lan_acc_en = 1;
	gen_mode_cfg1.sw_iso_mode = 1;
	gen_mode_cfg1.sys_cfg = g_eth_wan_mode; //  3 - ETH1 WAN, 2 - ETH0 mixed, 0 - DSL WAN
											// default - ETH WAN Separate, Ethernet WAN Mode.
											// Ethernet 0 carry LAN traffic, Ethernet 1 carry WAN traffic.
	*GEN_MODE_CFG1 = gen_mode_cfg1;
	GEN_MODE_CFG2->itf_outer_vlan_vld = 0xFF; //  Turn on outer VLAN for all interfaces;

    ps_mc_gencfg3.time_tick = cgu_get_pp32_clock() / 250000;    //  4 * (cgu_get_pp32_clock() / 1000000), 4 microsecond, 1/4 of QoS time tick
    ps_mc_gencfg3.ssc_mode = 1;
    ps_mc_gencfg3.asc_mode = 0;
    ps_mc_gencfg3.class_en = 0;
    ps_mc_gencfg3.psave_en = 1;
    *PS_MC_GENCFG3= ps_mc_gencfg3;

    *__DPLUS_QID_CONF_PTR  = 2; //Qid configured by FW
    *CFG_CLASS2QID_MAP(0) = 0;
    *CFG_CLASS2QID_MAP(1) = 0;
    *CFG_QID2CLASS_MAP(0) = 0;
    *CFG_QID2CLASS_MAP(1) = 0;
    if ( (g_wan_itf & 0x80) )
    {
        for ( j = 0; j < NUM_ENTITY(g_ptm_prio_queue_map); j++ ) {
            *CFG_CLASS2QID_MAP(j >> 3) |= g_ptm_prio_queue_map[j] << ((j & 0x07) * 4);
            *CFG_CLASS2QID_MAP((j + 8) >> 3) |= (g_ptm_prio_queue_map[j] | 8) << ((j & 0x07) * 4);
        }
    }
    else
    {
        for ( j = 0; j < NUM_ENTITY(g_eth_prio_queue_map); j++ ) {
            *CFG_CLASS2QID_MAP(j >> 3) |= g_eth_prio_queue_map[j] << ((j & 0x07) * 4);
            *CFG_CLASS2QID_MAP((j + 8) >> 3) |= (g_eth_prio_queue_map[j] | 8) << ((j & 0x07) * 4);
        }
    }
    for ( j = 0; j < NUM_ENTITY(g_wan_queue_class_map); j++ )
        *CFG_QID2CLASS_MAP(j >> 3) |= g_wan_queue_class_map[j] << ((j & 0x07) * 4);

    *MTU_CFG_TBL(0) = ETH_MAX_DATA_LENGTH;
    for ( i = 1; i < 8; i++ )
        *MTU_CFG_TBL(i) = ETH_MAX_DATA_LENGTH;  //  for ETH

    if ( g_eth_wan_mode == 3 /* ETH WAN */ && g_wanqos_en )
        init_qos();

	*CFG_WAN_PORTMAP = g_wan_itf;
	*CFG_MIXED_PORTMAP = g_eth_wan_mode == 2 /* Mixed Mode */? g_wan_itf : 0;
	*TUNNEL_MAX_ID = -1;

#if defined(ENABLE_FW_MODULE_TEST) && ENABLE_FW_MODULE_TEST
	init_acc_tables();
#endif
}

static INLINE int alloc_dma(void) {
	int ret;
	unsigned int pre_alloc_desc_total_num;
	struct sk_buff **skb_pool;
	struct sk_buff **pskb;
	volatile u32 *p;
	unsigned int i;

    dbg("g_wanqos_en %d\n",  g_wanqos_en);
#if defined(ENABLE_P2P_COPY_WITHOUT_DESC) && ENABLE_P2P_COPY_WITHOUT_DESC
	pre_alloc_desc_total_num = 0;
#else
	pre_alloc_desc_total_num = DMA_RX_CH2_DESC_NUM;
#endif
    if ( g_eth_wan_mode == 0 && g_dsl_bonding )     //  on chip bonding does not support SWAP queue
    {
        if ( g_dsl_bonding == 1 )
            pre_alloc_desc_total_num += E1_FRAG_RX_DESC_NUM + B1_RX_LINK_LIST_DESC_NUM + WAN_TX_DESC_NUM_TOTAL + DMA_RX_CH1_DESC_NUM + DMA_TX_CH1_DESC_NUM;

    }else if (g_eth_wan_mode == 0 || g_wanqos_en) //  DSL WAN or ETH WAN with FW QoS

        pre_alloc_desc_total_num += CPU_TO_WAN_TX_DESC_NUM
				+ WAN_TX_DESC_NUM_TOTAL + CPU_TO_WAN_SWAP_DESC_NUM
				+ DMA_RX_CH1_DESC_NUM + DMA_TX_CH1_DESC_NUM;

	skb_pool = (struct sk_buff **) kmalloc( pre_alloc_desc_total_num * sizeof(*skb_pool), GFP_KERNEL);
	if (skb_pool == NULL) {
		ret = -ENOMEM;
		printk("[%s] err %d\n", __func__, __LINE__);
		goto ALLOC_SKB_POOL_FAIL;
	}

	for (i = 0; (unsigned int) i < pre_alloc_desc_total_num; i++) {
		skb_pool[i] = alloc_skb_rx();
		if (!skb_pool[i]) {
			ret = -ENOMEM;
			printk("[%s] err %d\n", __func__, __LINE__);
			goto ALLOC_SKB_FAIL;
		}
#ifndef CONFIG_MIPS_UNCACHED
		/*  invalidate cache    */
		dma_cache_inv((unsigned long)skb_pool[i]->data, DMA_PACKET_SIZE);
#endif
#if defined(DEBUG_SKB_SWAP) && DEBUG_SKB_SWAP
		g_dbg_skb_swap_pool[i] = skb_pool[i];
#endif
	}

	g_dma_device_ppe = dma_device_reserve("PPE");
	if (!g_dma_device_ppe) {
		ret = -EIO;
		printk("[%s] err %d\n", __func__, __LINE__);
		goto RESERVE_DMA_FAIL;
	}

	if ( g_dbg_datapath & DBG_ENABLE_MASK_DUMP_INIT )
        dump_dma_device(g_dma_device_ppe, "just reserved");

	g_dma_device_ppe->buffer_alloc = dma_buffer_alloc;
	g_dma_device_ppe->buffer_free = dma_buffer_free;
#if defined(ENABLE_NAPI) && ENABLE_NAPI
	g_dma_device_ppe->activate_poll = g_napi_enable ? dma_activate_poll : NULL;
	g_dma_device_ppe->inactivate_poll = dma_inactivate_poll;
#endif
	g_dma_device_ppe->intr_handler = dma_int_handler;
	//g_dma_device_ppe->max_rx_chan_num = 8;
	//g_dma_device_ppe->max_tx_chan_num = 4;
	g_dma_device_ppe->tx_burst_len = 8;
	g_dma_device_ppe->rx_burst_len = 8;

    g_dma_device_ppe->num_tx_chan  = 4;
    g_dma_device_ppe->num_rx_chan  = 6;

	for (i = 0; i < g_dma_device_ppe->max_rx_chan_num; i++) {
		g_dma_device_ppe->rx_chan[i]->packet_size = DMA_PACKET_SIZE;
		g_dma_device_ppe->rx_chan[i]->control = IFX_DMA_CH_ON;
		//g_dma_device_ppe->rx_chan[i]->channel_packet_drop_enable    = 1;
	}
#if !defined(DEBUG_REDIRECT_FASTPATH_TO_CPU) || !DEBUG_REDIRECT_FASTPATH_TO_CPU
#ifdef CONFIG_IFX_PPE_E5_OFFCHIP_BONDING
	if ( g_dsl_bonding == 2 )
	    g_ppe_base_addr = IFX_PPE_ORG(1);
#endif
	g_dma_device_ppe->rx_chan[1]->desc_base = (int)DMA_RX_CH1_DESC_BASE;
#ifdef CONFIG_IFX_PPE_E5_OFFCHIP_BONDING
	if ( g_dsl_bonding == 2 )
	    g_ppe_base_addr = IFX_PPE_ORG(0);
#endif


	// PPA -> DSL
	g_dma_device_ppe->rx_chan[1]->desc_base = (int) DMA_RX_CH1_DESC_BASE;
	g_dma_device_ppe->rx_chan[1]->desc_len = DMA_RX_CH1_DESC_NUM;
	g_dma_device_ppe->rx_chan[1]->req_irq_to_free = g_dma_device_ppe->rx_chan[1]->irq;
	g_dma_device_ppe->rx_chan[1]->no_cpu_data = 1;

	/* RX2 -> TX0: (PPA -> Switch(port6))*/
	g_dma_device_ppe->rx_chan[2]->desc_base = (int) DMA_RX_CH2_DESC_BASE;
	g_dma_device_ppe->rx_chan[2]->desc_len = DMA_RX_CH2_DESC_NUM;
	g_dma_device_ppe->rx_chan[2]->req_irq_to_free = g_dma_device_ppe->rx_chan[2]->irq;
	g_dma_device_ppe->rx_chan[2]->no_cpu_data = 1;
#if defined(ENABLE_P2P_COPY_WITHOUT_DESC) && ENABLE_P2P_COPY_WITHOUT_DESC
	if (g_eth_wan_mode != 0 /* ETH WAN */&& !g_wanqos_en /* QoS Disabled */) {
	    printk(KERN_ERR "Warning: no WAN QOS in FW!\n");
		g_dma_device_ppe->rx_chan[1]->loopback_enable = 1;
		g_dma_device_ppe->rx_chan[1]->loopback_channel_number = 1 * 2 + 1; // absolute nummer - entsprict TX1
	}
	/* RX2 -> TX0: (PPA -> Switch(port6))*/
	g_dma_device_ppe->rx_chan[2]->loopback_enable = 1;
	g_dma_device_ppe->rx_chan[2]->loopback_channel_number = 0 * 2 + 1; // absolute nummer - entspricht TX0
#endif
#endif

	for (i = 0; i < g_dma_device_ppe->max_tx_chan_num; i++) {
		g_dma_device_ppe->tx_chan[i]->control = IFX_DMA_CH_ON;
#if defined (CONFIG_AVM_SCATTER_GATHER)
        g_dma_device_ppe->tx_chan[i]->scatter_gather_channel = 1;
#endif
	}
#if !defined(DEBUG_REDIRECT_FASTPATH_TO_CPU) || !DEBUG_REDIRECT_FASTPATH_TO_CPU
	g_dma_device_ppe->tx_chan[0]->desc_base = (int) DMA_TX_CH0_DESC_BASE;
	g_dma_device_ppe->tx_chan[0]->desc_len = DMA_TX_CH0_DESC_NUM;
	g_dma_device_ppe->tx_chan[1]->desc_base = (int) DMA_TX_CH1_DESC_BASE;
	g_dma_device_ppe->tx_chan[1]->desc_len = DMA_TX_CH1_DESC_NUM;
#if !defined(ENABLE_P2P_COPY_WITHOUT_DESC) || !ENABLE_P2P_COPY_WITHOUT_DESC
	g_dma_device_ppe->tx_chan[0]->global_buffer_len = DMA_PACKET_SIZE;
	g_dma_device_ppe->tx_chan[0]->peri_to_peri = 1;
	g_dma_device_ppe->tx_chan[1]->global_buffer_len = DMA_PACKET_SIZE;
	g_dma_device_ppe->tx_chan[1]->peri_to_peri = 1;
#else
	g_dma_device_ppe->tx_chan[0]->loopback_enable = 1;
	if (g_eth_wan_mode == 0 /* DSL WAN */|| g_wanqos_en /* QoS Enabled */) {
		// DSL -> Switch(port11)
		g_dma_device_ppe->tx_chan[1]->global_buffer_len = DMA_PACKET_SIZE;
		g_dma_device_ppe->tx_chan[1]->peri_to_peri = 1;
		g_dma_device_ppe->tx_chan[1]->req_irq_to_free = g_dma_device_ppe->tx_chan[1]->irq;
		g_dma_device_ppe->tx_chan[1]->no_cpu_data = 1;

		// PPA -> Switch(port 6)
		g_dma_device_ppe->tx_chan[0]->req_irq_to_free = g_dma_device_ppe->tx_chan[0]->irq;
		g_dma_device_ppe->tx_chan[0]->no_cpu_data = 1;
	} else
		g_dma_device_ppe->tx_chan[1]->loopback_enable = 1;
#endif
#endif

	g_dma_device_ppe->port_packet_drop_enable = 1;

	if (dma_device_register(g_dma_device_ppe) != IFX_SUCCESS) {
		err("failed in \"dma_device_register\"");
		ret = -EIO;
		printk("[%s] err %d\n", __func__, __LINE__);
		goto DMA_DEVICE_REGISTER_FAIL;
	}

	if ( g_dbg_datapath & DBG_ENABLE_MASK_DUMP_INIT )
        dump_dma_device(g_dma_device_ppe, "registered");

	pskb = skb_pool;

	if (g_eth_wan_mode == 0 || g_wanqos_en) {
	    if ( g_eth_wan_mode == 0 && g_dsl_bonding ) {
	        if ( g_dsl_bonding == 1 ) {
	            p = (volatile u32 *)E1_FRAG_TX_DESC_NB_BASE(0);
	            for ( i = 0; i < E1_FRAG_TX_DESC_NUM_TOTAL; i++ )
	            {
	                *p++ = 0x30000000;
	                *p++ = 0x00000000;
	            }
	            E1_FRAG_TX_DESBA->desba    = E1_FRAG_TX_DESC_NB_PPE(0);
	            E1_FRAG_TX_DESBA->bp_desba = E1_FRAG_TX_DESC_B_PPE(0);

	            p = (volatile u32 *)E1_FRAG_RX_DESC_BASE(0);
	            for ( i = 0; i < E1_FRAG_RX_DESC_NUM; i++ )
	            {
	                ASSERT(((u32)(*pskb)->data & (DMA_ALIGNMENT - 1)) == 0, "E1 Frag RX data pointer (0x%08x) must be 8 DWORDS aligned", (u32)(*pskb)->data);
	                // byte_offset should be set to 2
	                *p++ = 0xB1000000 | DMA_PACKET_SIZE /* 544 */;
	                *p++ = (u32)(*pskb++)->data & 0x1FFFFFE0;
	            }
	            E1_FRAG_RX_DESBA->desba    = E1_FRAG_RX_DESC_PPE(0);
	            E1_FRAG_RX_DESBA->bp_desba = WAN_RX_DESC_PPE;

	            p = (volatile u32 *)E1_FRAG_RX_DESC_BASE(1);
	            for ( i = 0; i < E1_FRAG_RX_DESC_NUM; i++ )
	            {
	                *p++ = 0xB1000000;
	                *p++ = 0x00000000;
	            }

	            p = (volatile u32 *)B1_RX_LINK_LIST_DESC_BASE;
	            for ( i = 0; i < B1_RX_LINK_LIST_DESC_NUM; i++ )
	            {
	                ASSERT(((u32)(*pskb)->data & (DMA_ALIGNMENT - 1)) == 0, "B1 Packet TX data pointer (0x%08x) must be 8 DWORDS aligned", (u32)(*pskb)->data);
	                *p++ = ((B1_RX_LINK_LIST_DESC_PPE + 2 * ((i + 1) & 0xFF)) << 16) | 1664 /* 544 */;
	                *p++ = (u32)(*pskb++)->data & 0x1FFFFFE0;
	            }
	        }
	    }
	    else {

	        p = (volatile u32 *) CPU_TO_WAN_TX_DESC_BASE;
	        for (i = 0; i < CPU_TO_WAN_TX_DESC_NUM; i++) {
	            ASSERT(
	                    ((u32)(*pskb)->data & (DMA_TX_ALIGNMENT - 1)) == 0,
	                    "CPU to WAN TX data pointer (0x%08x) must be 8 DWORDS aligned",
	                    (u32)(*pskb)->data);
	            *p++ = 0x30000000;
	            *p++ = (u32) (*pskb++)->data & 0x1FFFFFE0;
	        }
	    }
	    if ( g_dsl_bonding != 2 ) {
	        p = (volatile u32 *) WAN_TX_DESC_BASE(0);
	        for (i = 0; i < WAN_TX_DESC_NUM_TOTAL; i++) {
	            ASSERT(
	                    ((u32)(*pskb)->data & (DMA_TX_ALIGNMENT - 1)) == 0,
	                    "WAN (PTM/ETH) QoS TX data pointer (0x%08x) must be 8 DWORDS aligned",
	                    (u32)(*pskb)->data);
	            *p++ = 0x30000000;
	            *p++ = (u32) (*pskb++)->data & 0x1FFFFFE0;
	        }

	    }
	    if ( g_dsl_bonding == 0 ) {
	        p = (volatile u32 *) CPU_TO_WAN_SWAP_DESC_BASE;
	        for (i = 0; i < CPU_TO_WAN_SWAP_DESC_NUM; i++) {
	            ASSERT(
	                    ((u32)(*pskb)->data & (DMA_TX_ALIGNMENT - 1)) == 0,
	                    "WAN (PTM) Swap data pointer (0x%08x) must be 8 DWORDS aligned",
	                    (u32)(*pskb)->data);
	            *p++ = 0xB0000000;
	            *p++ = (u32) (*pskb++)->data & 0x1FFFFFE0;
	        }
	    }
	}
#if !defined(DEBUG_REDIRECT_FASTPATH_TO_CPU) || !DEBUG_REDIRECT_FASTPATH_TO_CPU
    if ( g_dsl_bonding != 2 )
    {
        p = (volatile u32 *)DMA_RX_CH1_DESC_BASE;
        for ( i = 0; i < DMA_RX_CH1_DESC_NUM; i++ )
        {
  #if !defined(ENABLE_P2P_COPY_WITHOUT_DESC) || !ENABLE_P2P_COPY_WITHOUT_DESC
            ASSERT(((u32)(*pskb)->data & (DMA_ALIGNMENT - 1)) == 0, "DMA RX channel 1 data pointer (0x%08x) must be 8 DWORDS aligned", (u32)(*pskb)->data);
  #else
            if ( g_eth_wan_mode == 0 || g_wanqos_en ) {
                ASSERT(((u32)(*pskb)->data & (DMA_ALIGNMENT - 1)) == 0, "DMA RX channel 1 data pointer (0x%#x) must be 8 DWORDS aligned", (u32)(*pskb)->data);
            }
  #endif
            *p++ = 0xB0000000 | DMA_PACKET_SIZE;
  #if !defined(ENABLE_P2P_COPY_WITHOUT_DESC) || !ENABLE_P2P_COPY_WITHOUT_DESC
            *p++ = (u32)(*pskb++)->data & 0x1FFFFFE0;
  #else
            if ( g_eth_wan_mode == 0 || g_wanqos_en )
                *p++ = (u32)(*pskb++)->data & 0x1FFFFFE0;
            else
                *p++ = 0x00000000;
  #endif
        }
    }

  #if !defined(ENABLE_P2P_COPY_WITHOUT_DESC) || !ENABLE_P2P_COPY_WITHOUT_DESC
    p = (volatile u32 *)DMA_RX_CH2_DESC_BASE;
    for ( i = 0; i < DMA_RX_CH2_DESC_NUM; i++ )
    {
        ASSERT(((u32)(*pskb)->data & (DMA_ALIGNMENT - 1)) == 0, "DMA RX channel 2 data pointer (0x%08x) must be 8 DWORDS aligned", (u32)(*pskb)->data);
        *p++ = 0xB0000000 | DMA_PACKET_SIZE;
        *p++ = (u32)(*pskb++)->data & 0x1FFFFFE0;
    }
  #endif
#endif

    if ( (g_eth_wan_mode == 0 || g_wanqos_en) && g_dsl_bonding != 2 )
    {
        p = (volatile u32 *)DMA_TX_CH1_DESC_BASE;
        for ( i = 0; i < DMA_TX_CH1_DESC_NUM; i++ )
        {
            ASSERT(((u32)(*pskb)->data & (DMA_ALIGNMENT - 1)) == 0, "WAN (PTM) RX or (ETH) TX data pointer (0x%08x) must be 8 DWORDS aligned", (u32)(*pskb)->data);
            *p++ = 0xB0000000;
            *p++ = (u32)(*pskb++)->data & 0x1FFFFFE0;
        }
    }

    g_f_dma_opened = 0;

    kfree(skb_pool);

    return 0;

DMA_DEVICE_REGISTER_FAIL:
    dma_device_release(g_dma_device_ppe);
    g_dma_device_ppe = NULL;
RESERVE_DMA_FAIL:
    i = pre_alloc_desc_total_num;
ALLOC_SKB_FAIL:
    while ( i-- )
        dev_kfree_skb_any(skb_pool[i]);
    kfree(skb_pool);
ALLOC_SKB_POOL_FAIL:
    return ret;

}

#ifdef CONFIG_IFX_PPE_E5_OFFCHIP_BONDING
static INLINE int alloc_bonding_dma(void)
{
    volatile u32 *p;
    unsigned int pdata = DMA_ALIGNED(CPHYSADDR(g_bm_addr) + DMA_TX_CH1_DESC_NUM * 2 * sizeof(unsigned int));
//    unsigned int pdata = DMA_ALIGNED(CPHYSADDR(g_bm_addr) + (DMA_TX_CH1_DESC_NUM + DMA_RX_CH1_DESC_NUM) * 2 * sizeof(unsigned int));
    unsigned int e1_bar;
    int i;

    e1_bar = (unsigned int)SB_BUFFER_BOND(1, 0x3800);

    E1_FRAG_TX_DESBA->desba    = 0x6000;
    E1_FRAG_TX_DESBA->bp_desba = 0x6760;
    E1_FRAG_RX_DESBA->desba    = 0x6500;
    E1_FRAG_RX_DESBA->bp_desba = 0;         //  offset is ZERO
    *E1_DES_PDMA_BAR = e1_bar;
    *B1_DES_PDMA_BAR = 0;
    *DATAPTR_PDMA_BAR = 0;

    p = (volatile u32 *)DMA_TX_CH1_DESC_BASE;
    for ( i = 0; i < DMA_TX_CH1_DESC_NUM; i++ )
    {
        *p++ = 0xB0000000;
        *p++ = pdata;
        pdata = DMA_ALIGNED(pdata + 1664) ;
    }

    g_ppe_base_addr = IFX_PPE_ORG(1);

    E1_FRAG_TX_DESBA->desba    = E1_FRAG_TX_DESC_NB_PPE(4);
    E1_FRAG_TX_DESBA->bp_desba = E1_FRAG_TX_DESC_B_PPE(4);
    E1_FRAG_RX_DESBA->desba    = E1_FRAG_RX_DESC_PPE(1);
    E1_FRAG_RX_DESBA->bp_desba = 0x6800;    //  offset is ZERO
    *E1_DES_PDMA_BAR = 0;
    *B1_DES_PDMA_BAR = g_pci_host_ddr_addr;
    *DATAPTR_PDMA_BAR = (g_pci_host_ddr_addr & 0xFF000000) | 0x0000FF00;    //  31-16: prefix, 15-0: mask

    p = (volatile u32 *)E1_FRAG_TX_DESC_NB_BASE(0);
    for ( i = 0; i < E1_FRAG_TX_DESC_NUM_TOTAL; i++ )
    {
        *p++ = 0x30000000;
        *p++ = 0x00000000;
    }

    p = (volatile u32 *)E1_FRAG_RX_DESC_BASE(0);
    for ( i = 0; i < E1_FRAG_RX_DESC_NUM * 2; i++ )
    {
        *p++ = 0xB1000000 | 1664 /* 544 */;
        *p++ = pdata;
        pdata = DMA_ALIGNED(pdata + 1664 /* 544 */);
    }

    p = (volatile u32 *)B1_RX_LINK_LIST_DESC_BASE;
    for ( i = 0; i < B1_RX_LINK_LIST_DESC_NUM; i++ )
    {
        *p++ = ((B1_RX_LINK_LIST_DESC_PPE + 2 * ((i + 1) & 0xFF)) << 16) | 1664 /* 544 */;
        *p++ = pdata;
        pdata = DMA_ALIGNED(pdata + 1664 /* 544 */);
    }

    p = (volatile u32 *)WAN_TX_DESC_BASE(0);
    for ( i = 0; i < WAN_TX_DESC_NUM_TOTAL; i++ )
    {
        *p++ = 0x30000000;
        *p++ = pdata;
        pdata = DMA_ALIGNED(pdata + 1664);
    }

    p = (volatile u32 *)DMA_RX_CH1_DESC_BASE;
    for ( i = 0; i < DMA_RX_CH1_DESC_NUM; i++ )
    {
        *p++ = 0xB0000000 | DMA_PACKET_SIZE;
        *p++ = pdata;
        pdata = DMA_ALIGNED(pdata + 1664);
    }
#if defined(ENABLE_MPS_MEMORY) && ENABLE_MPS_MEMORY
    *US_CDMA_RX_DES_PDMA_BAR = DMA_RX_CH1_DESC_MPS_ADDR;
    *US_CDMA_RX_DES_BASE     = 0x6800;
//    *US_CDMA_RX_DES_PDMA_BAR = g_pci_host_ddr_addr;
//    *US_CDMA_RX_DES_BASE     = 0x6800 + DMA_TX_CH1_DESC_NUM * 2;
#else
    *US_CDMA_RX_DES_PDMA_BAR = 0;
    *US_CDMA_RX_DES_BASE     = 0x2580;
#endif

    g_ppe_base_addr = IFX_PPE_ORG(0);

    return 0;
}
#endif


static INLINE void free_dma(void)
{
    volatile struct tx_descriptor *p;
    int i;

    dma_device_unregister(g_dma_device_ppe);
	dma_device_release(g_dma_device_ppe);
	g_dma_device_ppe = NULL;

    if ( g_eth_wan_mode == 0 || g_wanqos_en )
    {
        if ( g_eth_wan_mode == 0 && g_dsl_bonding )
        {
            if ( g_dsl_bonding == 1 )
            {
                p = (volatile struct tx_descriptor *)E1_FRAG_RX_DESC_BASE(0);
                for ( i = 0; i < E1_FRAG_RX_DESC_NUM; i++ )
                {
                    free_skb_clear_dataptr(p);
                    p++;
                }

                p = (volatile struct tx_descriptor *)B1_RX_LINK_LIST_DESC_BASE;
                for ( i = 0; i < B1_RX_LINK_LIST_DESC_NUM; i++ )
                {
                    free_skb_clear_dataptr(p);
                    p++;
                }
            }
        }
        else
        {
            p = (volatile struct tx_descriptor *)CPU_TO_WAN_TX_DESC_BASE;
            for ( i = 0; i < CPU_TO_WAN_TX_DESC_NUM; i++ )
            {
                free_skb_clear_dataptr(p);
                p++;
            }
        }

        if ( g_dsl_bonding != 2 )
        {
            p = (volatile struct tx_descriptor *)WAN_TX_DESC_BASE(0);
            for ( i = 0; i < WAN_TX_DESC_NUM_TOTAL; i++ )
            {
                free_skb_clear_dataptr(p);
                p++;
            }
        }

        if ( g_dsl_bonding == 0 )
        {
            p = (volatile struct tx_descriptor *)CPU_TO_WAN_SWAP_DESC_BASE;
            for ( i = 0; i < CPU_TO_WAN_SWAP_DESC_NUM; i++ )
            {
                free_skb_clear_dataptr(p);
                p++;
            }
        }
    }

#if !defined(DEBUG_REDIRECT_FASTPATH_TO_CPU) || !DEBUG_REDIRECT_FASTPATH_TO_CPU
    if ( g_dsl_bonding != 2 )
    {
        p = (volatile struct tx_descriptor *)DMA_RX_CH1_DESC_BASE;
        for ( i = 0; i < DMA_RX_CH1_DESC_NUM; i++ )
        {
  #if defined(ENABLE_P2P_COPY_WITHOUT_DESC) && ENABLE_P2P_COPY_WITHOUT_DESC
            if ( g_eth_wan_mode != 0 && !g_wanqos_en )
                break;
  #endif
            free_skb_clear_dataptr(p);
            p++;
        }
    }
#endif

#if !defined(ENABLE_P2P_COPY_WITHOUT_DESC) || !ENABLE_P2P_COPY_WITHOUT_DESC
    p = (volatile struct tx_descriptor *)DMA_RX_CH2_DESC_BASE;
    for ( i = 0; i < DMA_RX_CH2_DESC_NUM; i++ )
    {
        free_skb_clear_dataptr(p);
        p++;
    }
#endif

    if ( (g_eth_wan_mode == 0 || g_wanqos_en) && g_dsl_bonding != 2 )
    {
        p = (volatile struct tx_descriptor *)DMA_TX_CH1_DESC_BASE;
        for ( i = 0; i < DMA_TX_CH1_DESC_NUM; i++ )
        {
            free_skb_clear_dataptr(p);
            p++;
        }
    }


#if defined(DEBUG_SKB_SWAP) && DEBUG_SKB_SWAP
    for ( i = 0; i < NUM_ENTITY(g_dbg_skb_swap_pool); i++ )
        if ( g_dbg_skb_swap_pool[i] != NULL )
        {
            err("skb swap pool is not clean: %d - %08x\n", i, (unsigned int)g_dbg_skb_swap_pool[i]);
        }
#endif
}



#if defined(ENABLE_MY_MEMCPY) && ENABLE_MY_MEMCPY
static INLINE void *my_memcpy(unsigned char *dest, const unsigned char *src, unsigned int count)
{
	char *d = (char *)dest, *s = (char *)src;

	if (count >= 32) {
		int i = 8 - (((unsigned long) d) & 0x7);

		if (i != 8)
		while (i-- && count--) {
			*d++ = *s++;
		}

		if (((((unsigned long) d) & 0x7) == 0) &&
				((((unsigned long) s) & 0x7) == 0)) {
			while (count >= 32) {
				unsigned long long t1, t2, t3, t4;
				t1 = *(unsigned long long *) (s);
				t2 = *(unsigned long long *) (s + 8);
				t3 = *(unsigned long long *) (s + 16);
				t4 = *(unsigned long long *) (s + 24);
				*(unsigned long long *) (d) = t1;
				*(unsigned long long *) (d + 8) = t2;
				*(unsigned long long *) (d + 16) = t3;
				*(unsigned long long *) (d + 24) = t4;
				d += 32;
				s += 32;
				count -= 32;
			}
			while (count >= 8) {
				*(unsigned long long *) d =
				*(unsigned long long *) s;
				d += 8;
				s += 8;
				count -= 8;
			}
		}

		if (((((unsigned long) d) & 0x3) == 0) &&
				((((unsigned long) s) & 0x3) == 0)) {
			while (count >= 4) {
				*(unsigned long *) d = *(unsigned long *) s;
				d += 4;
				s += 4;
				count -= 4;
			}
		}

		if (((((unsigned long) d) & 0x1) == 0) &&
				((((unsigned long) s) & 0x1) == 0)) {
			while (count >= 2) {
				*(unsigned short *) d = *(unsigned short *) s;
				d += 2;
				s += 2;
				count -= 2;
			}
		}
	}

	while (count--) {
		*d++ = *s++;
	}

	return d;
}
#endif




#ifdef CONFIG_IFX_PPE_E5_OFFCHIP_BONDING

static void proc_dev_output(struct seq_file *m, void *private_data __maybe_unused){

    seq_printf(m,  "%s mode\n", g_ppe_base_addr == IFX_PPE_ORG(0) ? "host" : "device");
    seq_printf(m,  "  g_slave_ppe_base_addr  = 0x%08x\n", g_slave_ppe_base_addr);
    seq_printf(m,  "  g_pci_dev_ppe_addr_off = 0x%08x\n", g_pci_dev_ppe_addr_off);
    seq_printf(m,  "  g_ppe_base_addr        = 0x%08x\n", g_ppe_base_addr);
    seq_printf(m,  "  IFX_PMU                = 0x%08x\n", IFX_PMU);
    seq_printf(m,  "  g_bm_addr              = 0x%08x\n", g_bm_addr);
    seq_printf(m,  "  g_pci_host_ddr_addr    = 0x%08x\n", g_pci_host_ddr_addr);

}

static int proc_dev_input(char *line, void *data __maybe_unused) {

    char *p = line;

    if ( ifx_stricmp(p, "host") == 0 )
        g_ppe_base_addr = IFX_PPE_ORG(0);
    else if ( ifx_stricmp(p, "device") == 0 )
        g_ppe_base_addr = IFX_PPE_ORG(1);
    else
        printk("echo <host|device> > /proc/eth/dev");

    return 0;
}

#endif

#if defined(DEBUG_BONDING_PROC) && DEBUG_BONDING_PROC


static void check_ds_des_array(void)
{

    int i, j;
    int res;
    int data_ptr_num[DMA_TX_CH1_DESC_NUM + E1_FRAG_RX_DESC_NUM * 2 + B1_RX_LINK_LIST_DESC_NUM];

    if (g_dsl_bonding != 2)
        return;

    memset(data_ptr_num, 0, sizeof(data_ptr_num));

    {
        SAVE_AND_SWITCH_PPE_BASE_TO_BM (saved_ppe_base_addr);
        memcpy(des, (void *)DMA_TX_CH1_DESC_BASE, sizeof(struct rx_descriptor)* DMA_TX_CH1_DESC_NUM);
        RESTORE_PPE_BASE(saved_ppe_base_addr);
    }
    {
        SAVE_AND_SWITCH_PPE_BASE_TO_BM (saved_ppe_base_addr);
        memcpy(&des[DMA_TX_CH1_DESC_NUM], (void *)E1_FRAG_RX_DESC_BASE(0), sizeof(struct rx_descriptor)* E1_FRAG_RX_DESC_NUM * 2);
        memcpy(&des[DMA_TX_CH1_DESC_NUM + E1_FRAG_RX_DESC_NUM * 2], (void *)B1_RX_LINK_LIST_DESC_BASE, sizeof(struct rx_descriptor)* B1_RX_LINK_LIST_DESC_NUM );
        RESTORE_PPE_BASE(saved_ppe_base_addr);
    }

    for (i = 0; i < DMA_TX_CH1_DESC_NUM + E1_FRAG_RX_DESC_NUM * 2 + B1_RX_LINK_LIST_DESC_NUM; ++ i) {
        unsigned int data_ptr = ((unsigned int *) des)[2*i + 1];
        for(j = 0; j < DMA_TX_CH1_DESC_NUM + E1_FRAG_RX_DESC_NUM * 2 + B1_RX_LINK_LIST_DESC_NUM; ++ j) {
            unsigned int save_data_ptr = ((unsigned int *) saved_ds_des_array)[2*j + 1];
            if (save_data_ptr == data_ptr) {
                data_ptr_num[j] ++;
                break;
            }
        }
    }

    res = 0;
    for (i = 0; i < DMA_TX_CH1_DESC_NUM + E1_FRAG_RX_DESC_NUM * 2 + B1_RX_LINK_LIST_DESC_NUM; ++ i) {
        if (data_ptr_num[i] == 0) {
            printk("DS des lost: %08X %08X\n",
               ((unsigned int * ) &saved_ds_des_array[i])[0], ((unsigned int * ) &saved_ds_des_array[i])[1]);
            res ++;
        }
        else if (data_ptr_num[i] > 1) {
            printk("DS des dup : %08X %08X ==> %d\n",
               ((unsigned int * ) &saved_ds_des_array[i])[0], ((unsigned int * ) &saved_ds_des_array[i])[1],
                data_ptr_num[i]);
            res ++;
        }
    }

    if (res == 0) {
        printk("DS descriptor data_ptrs are consistent!\n");
    }
}

static void check_us_des_array(void)
{

    int i, j;
    int res;
    int data_ptr_num[512 + 64];

    if (g_dsl_bonding != 2)
        return;

    memset(data_ptr_num, 0, sizeof(data_ptr_num));

    {
        SAVE_AND_SWITCH_PPE_BASE_TO_BM (saved_ppe_base_addr);
        memcpy(&us_des, (void *)US_BOND_PKT_DES_BASE(0), sizeof(struct rx_descriptor)* 512 );
        memcpy(&us_des[512], (void *)DMA_RX_CH1_DESC_BASE, sizeof(struct rx_descriptor)* 64 );
        RESTORE_PPE_BASE(saved_ppe_base_addr);
    }

    for (i = 0; i < 512 + 64; ++ i) {
        unsigned int data_ptr = ((unsigned int *) us_des)[2*i + 1];
        for(j = 0; j < 512 + 64; ++ j) {
            unsigned int save_data_ptr = ((unsigned int *) saved_us_des_array)[2*j + 1];
            if (save_data_ptr == data_ptr) {
                data_ptr_num[j] ++;
                break;
            }
        }
    }

    res = 0;
    for (i = 0; i < 512 + 64; ++ i) {
        if (data_ptr_num[i] == 0) {
            printk("US des lost: %08X %08X\n",
               ((unsigned int * ) &saved_us_des_array[i])[0], ((unsigned int * ) &saved_us_des_array[i])[1]);
            res ++;
        }
        else if (data_ptr_num[i] > 1) {
            printk("US des dup : %08X %08X ==> %d\n",
               ((unsigned int * ) &saved_us_des_array[i])[0], ((unsigned int * ) &saved_us_des_array[i])[1],
                data_ptr_num[i]);
            res ++;
        }
    }

    if (res == 0) {
        printk("US descriptor data_ptrs are consistent!\n");
    }
}

static void proc_bonding_output(struct seq_file *, void *data) {
    unsigned int dw;

    seq_printf(m,  "Bonding Config:\n");

    seq_printf(m,  "  BOND_CONF [0x2008]       = 0x%08x, max_frag_size=%u, pctrl_cnt= %u, dplus_fp_fcs_en=%u\n"
                                     "                                     bg_num=%u, bond_mode=%u (%s)\n",
        *(volatile u32 *)BOND_CONF, (u32)BOND_CONF->max_frag_size, (u32)BOND_CONF->polling_ctrl_cnt, (u32)BOND_CONF->dplus_fp_fcs_en,
        (u32)BOND_CONF->bg_num, (u32)BOND_CONF->bond_mode, (u32)BOND_CONF->bond_mode ? "L2 Trunking" : "L1 TC");
    seq_printf(m,  "                           =             e1_bond_en=%u, d5_acc_dis=%u, d5_b1_en=%u\n",
        (u32)BOND_CONF->e1_bond_en, (u32)BOND_CONF->d5_acc_dis, (u32)BOND_CONF->d5_b1_en);

    dw = *(volatile u32 *)US_BG_QMAP;
    seq_printf(m,  "  US_BG_QMAP[0x2009]       = 0x%08x, qmap0=0x%02x, qmap1=0x%02x, qmap2=0x%02x, qmap3=0x%02x\n",
        dw, dw & 0xff, (dw >> 8) & 0xff, (dw >> 16) & 0xff, (dw >> 24) & 0xff );

    dw = *(volatile u32 *)US_BG_GMAP;
    seq_printf(m,  "  US_BG_GMAP[0x200A]       = 0x%08x, gmap0=0x%02x, gmap1=0x%02x, gmap2=0x%02x, qmap3=0x%02x\n",
        dw, dw & 0xff, (dw >> 8) & 0xff, (dw >> 16) & 0xff, (dw >> 24) & 0xff );

    dw = *(volatile u32 *)DS_BG_GMAP;
    seq_printf(m,  "  DS_BG_GMAP[0x200B]       = 0x%08x, gmap0=0x%02x, gmap1=0x%02x, gmap2=0x%02x, qmap3=0x%02x\n",
        dw, dw & 0xff, (dw >> 8) & 0xff, (dw >> 16) & 0xff, (dw >> 24) & 0xff );

    seq_printf(m,  "  CURR_TIME [0x200C]       = 0x%08x\n\n", *(volatile u32 *)TIME_STAMP);

#ifdef CONFIG_IFX_PPE_E5_OFFCHIP_BONDING
    seq_printf(m,  "Cross Pci Debug Info --- Read                  Write \n");
    seq_printf(m,  "    min clock cycle:     %-20u, %u\n", *__READ_MIN_CYCLES, *__WRITE_MIN_CYCLES);
    seq_printf(m,  "    max clock cycle:     %-20u, %u\n", *__READ_MAX_CYCLES, *__WRITE_MAX_CYCLES);
    seq_printf(m,  "   total access num:     %-20u, %u\n", *__READ_NUM, *__WRITE_NUM);
    seq_printf(m,  "     total_cycle_lo:     %-20u, %u\n",
        *__TOTAL_READ_CYCLES_LO,  *__TOTAL_WRITE_CYCLES_LO);
    seq_printf(m,  " *total_lo/acc_num*:     %-20u, %u\n",
        *__READ_NUM ?  *__TOTAL_READ_CYCLES_LO / *__READ_NUM   : 0 ,
        *__WRITE_NUM ? *__TOTAL_WRITE_CYCLES_LO / *__WRITE_NUM : 0);
    seq_printf(m,  "  total cycle_hi_lo:     0x%08x%08x,   0x%08x%08x\n",
        *__TOTAL_READ_CYCLES_HI, *__TOTAL_READ_CYCLES_LO, * __TOTAL_WRITE_CYCLES_HI, *__TOTAL_WRITE_CYCLES_LO);
#endif

}

#ifdef CONFIG_IFX_PPE_E5_OFFCHIP_BONDING


typedef struct  {
    unsigned int start_test     : 1;
    unsigned int test_type      : 2;
    unsigned int test_cnt       : 29;

} PDMA_PF_TEST_CMD;

typedef struct {

    unsigned int  byte_cnt          : 10;
    unsigned int  int_mem_addr_off  : 11;
    unsigned int  res0              : 6;
    unsigned int  eop               : 1;
    unsigned int  pdma              : 1;
    unsigned int  sar               : 1;
    unsigned int  bc                : 2;

    unsigned int  res1              : 1;
    unsigned int  ext_mem_addr      : 29;
    unsigned int  release           : 1;
    unsigned int  pdma_cmd_type     : 1;

}PDMA_DIRA_CMD;

#define __PF_TEST_CMD                      ((volatile PDMA_PF_TEST_CMD *) SB_BUFFER_BOND(2, 0x51AC))
#define __RW_CYCLES                         SB_BUFFER_BOND(2, 0x51AD)

#define __RW_PERF_TEST_PDMA_CMD0            SB_BUFFER_BOND(2, 0x51AE)
#define __RW_PERF_TEST_PDMA_CMD1            SB_BUFFER_BOND(2, 0x51AF)

#define __RW_PERF_TEST_PDMA_CMD             ((volatile PDMA_DIRA_CMD *) __RW_PERF_TEST_PDMA_CMD0)

#define PFT_READ_ONLY                       2
#define PFT_WRITE_ONLY                      1
#define PFT_READ_WRITE                      (PFT_READ_ONLY | PFT_WRITE_ONLY)


void init_pdma_pftest_pdma_cmd(PDMA_DIRA_CMD * p_cmd, unsigned int offset, unsigned int byte_cnt)
{
    // pdma_cmd0.bc        = 3
    // pdma_cmd0.sar       = 0
    // pdma_cmd0.pdma      = 1
    // pdma_cmd0.eop       = 1
    // pdma_cmd0.reserved  = 0

    p_cmd->byte_cnt          = byte_cnt;
    p_cmd->int_mem_addr_off  = offset;
    p_cmd->res0              = 0;
    p_cmd->eop               = 1;
    p_cmd->pdma              = 1;
    p_cmd->sar               = 0;
    p_cmd->bc                = 3;

    // pdma_cmd1.pdma_cmd_type = 1
    // pdma_cmd1.release       = 0
    // pdma_cmd1.reserve       = 0

    p_cmd->res1              = 0;
    p_cmd->ext_mem_addr      = *B1_DES_PDMA_BAR + offset;
    p_cmd->release           = 0;
    p_cmd->pdma_cmd_type     = 1;
}

void issue_test_cmd(unsigned int test_type, unsigned int offset, unsigned int byte_cnt, unsigned int test_cnt)
{
    int i = 0;
    PDMA_PF_TEST_CMD test_cmd;
    PDMA_DIRA_CMD pdma_cmd;

    // clear idlekeep
    *SFSM_CFG0 = ( *SFSM_CFG0 ) & ( ~ ( 1 << 15) );
    *SFSM_CFG1 = ( *SFSM_CFG1 ) & ( ~ ( 1 << 15) );

    // wait for enough time to release idle cw
    for(i = 0; i < 100000; ++i);

    test_cmd = *__PF_TEST_CMD;
    if (test_cmd.start_test == 1) {
        printk("Error! A test is on-going, please wait \n");
        return;
    }
    if (offset > 63 || byte_cnt > 384 || byte_cnt == 0) {
        printk("Error parameters, offset in [0..63], and byte_cnt in [1..384]\n");
        return;
    }

    init_pdma_pftest_pdma_cmd(&pdma_cmd, offset, byte_cnt);
    * __RW_PERF_TEST_PDMA_CMD = pdma_cmd;

    test_cmd.test_type  = test_type;
    test_cmd.test_cnt   = test_cnt;
    test_cmd.start_test = 1;
    *__PF_TEST_CMD      = test_cmd;
}

void check_pf_test_result(void)
{
    PDMA_PF_TEST_CMD test_cmd;
    PDMA_DIRA_CMD pdma_cmd;
    unsigned int cycles;
    unsigned int rate;

    test_cmd = *__PF_TEST_CMD;
    if (test_cmd.start_test == 1 ) {
        printk("Error! A test is on-going, please wait \n");
        return;
    }

    if(test_cmd.test_cnt == 0 || test_cmd.test_type == 0 ) {
        printk("No test performed\n");
        return;
    }

    pdma_cmd = * __RW_PERF_TEST_PDMA_CMD;
    cycles = *__RW_CYCLES;

    rate = 0;
    if (cycles) {
        // we assume PPE running at 432Mhz
        unsigned int ratio = 1;

        if (test_cmd.test_type == PFT_READ_WRITE)
            ratio = 2;

        rate = test_cmd.test_cnt * pdma_cmd.byte_cnt * ratio * 8 * 432 / cycles ;
    }

    printk("Test Parameters: test_type = %5s, offset = %3d, byte_cnt = %3d, test_cnt = %d\n"
           "Test Results:  ppe_cycle = %10d, Throughput = %10d Mbps\n",
           test_cmd.test_type == PFT_READ_ONLY ? "read" : (test_cmd.test_type == PFT_WRITE_ONLY ?"write" : "R/W"),
           pdma_cmd.int_mem_addr_off, pdma_cmd.byte_cnt, test_cmd.test_cnt,
           cycles, rate);

}

#endif // #ifdef CONFIG_IFX_PPE_E5_OFFCHIP_BONDING

void check_ds_ll_list(volatile struct DS_BOND_GIF_LL_CTXT * p_ll_ctxt, unsigned char * leading)
{
    unsigned int sb_curr_des_ptr;
    char c_check_result[16];
    unsigned int des_num;

    des_num = p_ll_ctxt->des_num;
    sb_curr_des_ptr = p_ll_ctxt->head_ptr;

    while(des_num > 1) {
        sb_curr_des_ptr = ((volatile struct DS_BOND_GIF_LL_DES *)SB_BUFFER_BOND(2, sb_curr_des_ptr))->next_des_ptr;
        des_num --;
    }

    if ( (des_num == 1) && (sb_curr_des_ptr != p_ll_ctxt->tail_ptr) ) {
        sprintf( c_check_result, "!=0x%04x", sb_curr_des_ptr);
    }
    else {
        sprintf( c_check_result, "==0x%04x", sb_curr_des_ptr);
    }

    printk("%s", leading);
    printk("head_ptr: 0x%04x  tail_ptr: 0x%04x [%s]  des_num: %-3d  fh_valid: %d  sid:%05d, sop: %d eop: %d\n",
           p_ll_ctxt->head_ptr, p_ll_ctxt->tail_ptr, c_check_result,
           p_ll_ctxt->des_num, p_ll_ctxt->fh_valid, p_ll_ctxt->sid, p_ll_ctxt->sop,  p_ll_ctxt->eop);
}

static int proc_bonding_input(char *line, void *data __maybe_unused){
    char *p = line;
    char * tokens[5];
    int finished, token_num;


    const int cw_pg_size = 17;

    int len = strlen( line );

    token_num = tokenize(p, " \t", tokens, 5, &finished);
    if (token_num <= 0) {
        // MOD_DEC_USE_COUNT;
        return 0;
    }


    if ( strincmp(tokens[0], "help", 4) == 0) {
        printk("echo help > /proc/eth/bonding ==> \n\tprint this help message\n");

        printk("echo clear_rx_cb [start_pg] [clear_pg_num] > /proc/eth/bonding \n");
        printk("   :clear rx cell/codeword buffer for bc0 and bc1  \n");

        printk("echo clear_rx_cb[0|1] [start_pg] [clear_pg_num] > /proc/eth/bonding \n");
        printk("   :clear rx cell/codeword buffer for bc0 or bc1  \n");

        printk("echo clear_tx_cb [start_pg] [clear_pg_num] > /proc/eth/bonding \n");
        printk("   :clear tx cell/codeword buffer for bc0 and bc1  \n");

        printk("echo clear_tx_cb[0|1] [start_pg] [clear_pg_num] > /proc/eth/bonding \n");
        printk("   :clear tx cell/codeword buffer for bc0 or bc1  \n");

        printk("echo clear_cb[0|1] [start_pg] [clear_pg_num] > /proc/eth/bonding \n");
        printk("   :clear rx and tx cell/codeword buffer for bc0 or bc1  \n");

        printk("echo clear_eth_dbuf [start_pg] [clear_pg_num] > /proc/eth/bonding \n");
        printk("   :clear eth rx data buffer  \n");

        printk("echo clear_eth_cbuf [start_pg] [clear_pg_num] > /proc/eth/bonding \n");
        printk("   :clear eth rx control buffer  \n");

        printk("echo clear_ethbuf [start_pg] [clear_pg_num] > /proc/eth/bonding \n");
        printk("   :clear eth rx data and control buffer  \n");

        printk("echo read_ethbuf [start_pg] [print_pg_num] > /proc/eth/bonding \n");
        printk("   :clear eth rx data and control buffer  \n");

        printk("echo read_rx_cb[0|1] [start_pg] [print_pg_num] > /proc/eth/bonding \n");
        printk("   :read rx cell/codeword buffer for bc0 or bc1  \n");

        printk("echo read_tx_cb[0|1] [start_pg] [print_pg_num] > /proc/eth/bonding \n");
        printk("   :read tx cell/codeword buffer for bc0 or bc1  \n");

        printk("echo us_bg_ctxt > /proc/eth/bonding \n");
        printk("   : US - display us_bg_ctxt\n");

        printk("echo ds_bg_ctxt > /proc/eth/bonding \n");
        printk("   : DS - display ds_bg_ctxt\n");

        printk("echo us_bond_gif_mib > /proc/eth/bonding \n");
        printk("   : US - display us_bond_gif_mib\n");

        printk("echo clear_us_bond_gif_mib > /proc/eth/bonding \n");
        printk("   : US - clear us_bond_gif_mib\n");

        printk("echo ds_bond_gif_mib > /proc/eth/bonding \n");
        printk("   : DS - display ds_bond_gif_mib\n");

        printk("echo clear_ds_bond_gif_mib > /proc/eth/bonding \n");
        printk("   : DS - clear ds_bond_gif_mib\n");

        printk("echo ds_bg_mib > /proc/eth/bonding \n");
        printk("   : DS - display ds_bg_mib\n");

        printk("echo clear_ds_bg_mib > /proc/eth/bonding \n");
        printk("   : DS - clear ds_bg_mib\n");

        printk("echo ds_link_list > /proc/eth/bonding \n");
        printk("   : DS - check link list\n");

        printk("echo rx_bc_cfg > /proc/eth/bonding \n");
        printk("   : read RX bear channel config\n");

        printk("echo tx_bc_cfg > /proc/eth/bonding \n");
        printk("   : read TX bear channel config\n");

        printk("echo rx_gif_cfg [0|1|2|3] > /proc/eth/bonding \n");
        printk("   : read RX gamma interface config\n");

        printk("echo tx_gif_cfg [0|1|2|3] > /proc/eth/bonding \n");
        printk("   : read TX gamma interface config\n");

        printk("echo idlekeep > /proc/eth/bonding \n");
        printk("   : read idle keep of BC0 and BC1\n");

        printk("echo set idlekeep[0|1] > /proc/eth/bonding \n");
        printk("   : set idle keep of BC0 or BC1\n");

        printk("echo clear idlekeep[0|1] > /proc/eth/bonding \n");
        printk("   : clear idle keep of BC0 or BC1\n");

        printk("echo clear_dbg_info > /proc/eth/bonding \n");
        printk("   : clear cross pci debug info\n");

        printk("echo switch_mode <L1|L2> > /proc/eth/bonding \n");
        printk("   : switch to L1 mode (PTM-TC bonding) or L2 (ethernet thunk mode)\n");

        printk("echo check_ds_des > /proc/eth/bonding \n");
        printk("   : check ds descriptor consistency\n");

        printk("echo check_us_des > /proc/eth/bonding \n");
        printk("   : check us descriptor consistency\n");

  #ifdef CONFIG_IFX_PPE_E5_OFFCHIP_BONDING
        printk("echo pdma_perf_test <test_type> <offset> <byte_num> [read_times=10000] > /proc/eth/bonding \n");
        printk("   : pdma cross PCI read performance test\n");
        printk("     test_type = [r|w|rw], read, write, or read write \n");
        printk("     offset in range [0..63]; byte_num [1..384], read_times >= 10 \n");

        printk("echo check_pf_result \n");
        printk("   : check pdma cross PCI R/W performance test result\n");
  #endif
    }
  #ifdef CONFIG_IFX_PPE_E5_OFFCHIP_BONDING
    else if ( strincmp(tokens[0], "pdma_perf_test", 14) == 0 ) {
        //SAVE_AND_SWITCH_PPE_BASE_TO_BM (saved_ppe_base_addr);
        if (token_num < 4) {
            printk("echo pdma_perf_test <test_type> <offset> <byte_num> [read_times=10000] > /proc/eth/bonding \n");
            printk("   : pdma cross PCI read performance test\n");
            printk("     test_type = [r|w|rw], read, write, or read write \n");
            printk("     offset in range [0..63]; byte_num [1..384], read_times >= 10 \n");
        }
        else {
            unsigned int offset, byte_num, test_cnt;
            unsigned int test_type;
            test_cnt = 10000;

            if(strincmp(tokens[1], "rw", 2) == 0)
                test_type = PFT_READ_WRITE;
            else if(strincmp(tokens[1], "r", 1) == 0)
                test_type = PFT_READ_ONLY;
            else if(strincmp(tokens[1], "w", 1) == 0)
                test_type = PFT_WRITE_ONLY;
            else
                test_type = PFT_READ_WRITE;

            offset =  simple_strtoul(tokens[2], NULL, 0);
            byte_num =  simple_strtoul(tokens[3], NULL, 0);
            if(token_num >= 5) {
                test_cnt =  simple_strtoul(tokens[4], NULL, 0);
            }
            if ( test_cnt == 0)
                test_cnt = 10000;
            issue_test_cmd(test_type, offset, byte_num, test_cnt);
        }
        //RESTORE_PPE_BASE(saved_ppe_base_addr);
    }
    else if ( strincmp(tokens[0], "check_pf_result", 15) == 0 ) {
        //SAVE_AND_SWITCH_PPE_BASE_TO_BM (saved_ppe_base_addr);
        check_pf_test_result();
        //RESTORE_PPE_BASE(saved_ppe_base_addr);
    }
  #endif
    else if ( strincmp(tokens[0], "check_ds_des", 12) == 0 ) {
        check_ds_des_array();
    }
    else if ( strincmp(tokens[0], "check_us_des", 12) == 0 ) {
        check_us_des_array();
    }
    else if ( strincmp(tokens[0], "switch_mode", 11) == 0 ) {
        if ( (token_num > 1) &&  strincmp(tokens[1], "L1", 2) == 0 ) {
            if (BOND_CONF->e1_bond_en && BOND_CONF->bond_mode == L2_ETH_TRUNK_MODE) {
                // current we are in L1 ptmtc bonding bonding mode
                switch_to_ptmtc_bonding();
            }
        }
        else if ( (token_num > 1) &&  strincmp(tokens[1], "L2", 2) == 0 ) {
            if (BOND_CONF->e1_bond_en && BOND_CONF->bond_mode == L1_PTM_TC_BOND_MODE) {
                // current we are in L1 ptmtc bonding bonding mode
                switch_to_eth_trunk();
            }
        }
        else {
            printk("usage: 'switch_mode <L1|L2> > /proc/eth/bonding'\n");
            printk("        :switch to L1 mode (PTM-TC bonding) or L2 (ethernet thunk mode)\n");
        }

    }
    else if ( strincmp(tokens[0], "clear_dbg_info", 13) == 0 ) {

        SAVE_AND_SWITCH_PPE_BASE_TO_BM (saved_ppe_base_addr);

        *__READ_MAX_CYCLES  = 0;
        *__WRITE_MAX_CYCLES = 0;
        *__READ_MIN_CYCLES  = 0xFFFFFFFF;
        *__WRITE_MIN_CYCLES = 0xFFFFFFFF;
        *__READ_NUM         = 0;
        *__WRITE_NUM        = 0;
        *__TOTAL_READ_CYCLES_HI  = 0;
        *__TOTAL_READ_CYCLES_LO  = 0;
        *__TOTAL_WRITE_CYCLES_HI = 0;
        *__TOTAL_WRITE_CYCLES_LO = 0;

        RESTORE_PPE_BASE(saved_ppe_base_addr);
    }
    else if ( strincmp(tokens[0], "read_ethbuf", 10) == 0 ) {


        unsigned int start_pg;
        unsigned int print_pg_num;
        unsigned int num;
        unsigned int pg_size;
        volatile unsigned int *dbase, *cbase;
        unsigned int pnum, i;
        volatile unsigned int * cw;
        SAVE_AND_SWITCH_PPE_BASE_TO_SM (saved_ppe_base_addr);

        dbase = (volatile unsigned int *)SB_BUFFER_BOND(2, ( ( struct dmrx_dba * )DM_RXDB)->dbase + 0x2000 );
        cbase = (volatile unsigned int *)SB_BUFFER_BOND(2, ( ( struct dmrx_cba * )DM_RXCB)->cbase + 0x2000);

        pnum = ( ( struct dmrx_cfg * )DM_RXCFG)->pnum;
        pg_size =  ( ( ( struct  dmrx_cfg * )DM_RXCFG)->psize + 1) * 32 / 4;

        start_pg = 0;
        print_pg_num = pnum;

        if (token_num >= 2)
            start_pg =  simple_strtoul(tokens[1], NULL, 0);
        if (token_num >= 3)
            print_pg_num =  simple_strtoul(tokens[2], NULL, 0);

        start_pg = start_pg % pnum;
        if(print_pg_num > pnum)
            print_pg_num = pnum;

        printk("PTM TX BC 0 CELL data/ctrl buffer:\n\n");
        for(i = start_pg, num = 0 ; num < print_pg_num ; num ++, i = (i + 1) % pnum ) {

            struct ctrl_dmrx_2_fw * p_ctrl_dmrx;
            struct ctrl_fw_2_dsrx * p_ctrl_dsrx;
            int j = 0;

            cw = dbase + i * pg_size;

            p_ctrl_dmrx = (struct ctrl_dmrx_2_fw *) ( &cbase[i]);
            p_ctrl_dsrx = (struct ctrl_fw_2_dsrx *) ( &cbase[i]);


            for(j =0; j < pg_size; j=j+4) {

                if(j==0) {

                    printk("Pg_id %2d: %08x %08x %08x %08x ", i, cw[0], cw[1], cw[2], cw[3]);
                    printk("pg_val: %2d, byte_off: %-3d, cos: %x \n", p_ctrl_dmrx->pg_val, p_ctrl_dmrx->byte_off, p_ctrl_dmrx->cos);
                }


                else if(j==4) {
                    printk("          %08x %08x %08x %08x ",  cw[j], cw[j+1], cw[j+2], cw[j+3]);
                    printk("bytes_cnt: %-3d, eof: %x \n", p_ctrl_dmrx->bytes_cnt, p_ctrl_dmrx->eof);
                }

                else if(j==12) {
                    printk("          %08x %08x %08x %08x ",  cw[j], cw[j+1], cw[j+2], cw[j+3]);
                    printk("pkt_status: %x, flag_len: %x, byte_off: %-3d, acc_sel: %x\n", p_ctrl_dsrx->pkt_status, p_ctrl_dsrx->flag_len, p_ctrl_dsrx->byte_off,p_ctrl_dsrx->acc_sel );
                }

                else if(j==16) {
                    printk("          %08x %08x %08x %08x ",  cw[j], cw[j+1], cw[j+2], cw[j+3]);
                    printk("fcs_en: %x, cos: %x, bytes_cnt: %-3d, eof: %x\n", p_ctrl_dsrx->fcs_en, p_ctrl_dsrx->cos, p_ctrl_dsrx->bytes_cnt,p_ctrl_dsrx->eof );
                }

                else {
                    printk("          %08x %08x %08x %08x \n",  cw[j], cw[j+1], cw[j+2], cw[j+3]);
                }

                }

                printk("\n");

           }

        RESTORE_PPE_BASE(saved_ppe_base_addr);
    }

    else if ( strincmp(tokens[0], "read_tx_cb", 10) == 0 ) {

        unsigned int start_pg;
        unsigned int print_pg_num;
        unsigned int num;

        if(tokens[0][10] == '0' || tokens[0][10] == '\0') {

            volatile unsigned int *dbase0;
            unsigned int pnum0, i;
            volatile unsigned int * cw;

            dbase0 = (volatile unsigned int *)SB_BUFFER_BOND(2, FFSM_DBA(0)->dbase + 0x2000);
            pnum0 = FFSM_CFG(0)->pnum;

            start_pg = 0;
            print_pg_num = pnum0;

            if (token_num >= 2)
                start_pg =  simple_strtoul(tokens[1], NULL, 0);
            if (token_num >= 3)
                print_pg_num =  simple_strtoul(tokens[2], NULL, 0);

            start_pg = start_pg % pnum0;
            if(print_pg_num > pnum0)
                print_pg_num = pnum0;

            printk("PTM TX BC 0 CELL data/ctrl buffer:\n\n");
            for(i = start_pg, num = 0 ; num < print_pg_num ; num ++, i = (i + 1) % pnum0 ) {

                cw = dbase0 + i * cw_pg_size;
                printk("CW %2d: %08x \n"          , i, cw[0]);
                printk("       %08x %08x %08x %08x\n",  cw[1], cw[2], cw[3], cw[4]);
                printk("       %08x %08x %08x %08x\n",  cw[5], cw[6], cw[7], cw[8]);
                printk("       %08x %08x %08x %08x\n",  cw[9], cw[10], cw[11], cw[12]);
                printk("       %08x %08x %08x %08x\n",  cw[13], cw[14], cw[15], cw[16]);
            }

        }

        if (tokens[0][10] == '1' || tokens[0][10] == '\0'){

            volatile unsigned int *dbase1;
            unsigned int pnum1, i;
            volatile unsigned int * cw;

            dbase1 = (volatile unsigned int *)SB_BUFFER_BOND(2, FFSM_DBA(1)->dbase + 0x2000);
            pnum1 = FFSM_CFG(1)->pnum;

            start_pg = 0;
            print_pg_num = pnum1;

            if (token_num >= 2)
                start_pg =  simple_strtoul(tokens[1], NULL, 0);
            if (token_num >= 3)
                print_pg_num =  simple_strtoul(tokens[2], NULL, 0);

            start_pg = start_pg % pnum1;
            if(print_pg_num > pnum1)
                print_pg_num = pnum1;

            printk("PTM TX BC 1 CELL data/ctrl buffer:\n\n");
            for(i = start_pg, num = 0 ; num < print_pg_num ; num ++, i = (i + 1) % pnum1 ) {

                cw = dbase1 + i * cw_pg_size;
                printk("CW %2d: %08x \n"          , i, cw[0]);
                printk("       %08x %08x %08x %08x\n",  cw[1], cw[2], cw[3], cw[4]);
                printk("       %08x %08x %08x %08x\n",  cw[5], cw[6], cw[7], cw[8]);
                printk("       %08x %08x %08x %08x\n",  cw[9], cw[10], cw[11], cw[12]);
                printk("       %08x %08x %08x %08x\n\n",  cw[13], cw[14], cw[15], cw[16]);
            }

        }

    }

    else if ( strincmp(tokens[0], "read_rx_cb", 10) == 0 ) {

        unsigned int start_pg;
        unsigned int print_pg_num;
        unsigned int num;

        unsigned char * cwid_txt[8] = {
            "All_data",
            "End_of_frame",
            "Start_while_tx",
            "All_idle",
            "Start_while_idle",
            "All_idle_nosync",
            "Error",
            "Res"
         };

        if(tokens[0][10] == '0' || tokens[0][10] == '\0') {

            volatile unsigned int *dbase0, *cbase0;
            unsigned int pnum0, i;
            volatile unsigned int * cw;

            dbase0 = (volatile unsigned int *)SB_BUFFER_BOND(2, SFSM_DBA(0)->dbase + 0x2000);
            cbase0 = (volatile unsigned int *)SB_BUFFER_BOND(2, SFSM_CBA(0)->cbase + 0x2000);

            pnum0 = SFSM_CFG(0)->pnum;

            start_pg = 0;
            print_pg_num = pnum0;

            if (token_num >= 2)
                start_pg =  simple_strtoul(tokens[1], NULL, 0);
            if (token_num >= 3)
                print_pg_num =  simple_strtoul(tokens[2], NULL, 0);

            start_pg = start_pg % pnum0;
            if(print_pg_num > pnum0)
                print_pg_num = pnum0;

            printk("PTM RX BC 0 CELL data/ctrl buffer (pnum0 = %d):\n\n", pnum0);
            for(i = start_pg, num = 0 ; num < print_pg_num ; num ++, i = (i + 1) % pnum0 ) {

                struct PTM_CW_CTRL * p_ctrl;

                cw = dbase0 + i * cw_pg_size;
                p_ctrl = (struct PTM_CW_CTRL *) ( &cbase0[i]);

                printk("CW %2d: %08x                            ", i, cw[0]);
                printk("cwid=%d [%-16s], cwer=%d, preempt=%d, short=%d\n",
                        p_ctrl->cwid, cwid_txt[p_ctrl->cwid], p_ctrl->cwer, p_ctrl->preempt, p_ctrl->shrt);

                printk("       %08x %08x %08x %08x ", cw[1], cw[2], cw[3], cw[4]);
                printk("state=%d, bad=%d, ber=%-3d, spos=%-3d, ffbn=%-3d\n",
                        p_ctrl->state, p_ctrl->bad, p_ctrl->ber, p_ctrl->spos, p_ctrl->ffbn);

                printk("       %08x %08x %08x %08x\n",  cw[5], cw[6], cw[7], cw[8]);
                printk("       %08x %08x %08x %08x\n",  cw[9], cw[10], cw[11], cw[12]);
                printk("       %08x %08x %08x %08x\n",  cw[13], cw[14], cw[15], cw[16]);

            }

        }

        if (tokens[0][10] == '1' || tokens[0][10] == '\0'){

            volatile unsigned int *dbase1, *cbase1;
            unsigned int pnum1, i;
            volatile unsigned int * cw;

            dbase1 = (volatile unsigned int *)SB_BUFFER_BOND(2, SFSM_DBA(1)->dbase + 0x2000);
            cbase1 = (volatile unsigned int *)SB_BUFFER_BOND(2, SFSM_CBA(1)->cbase + 0x2000);

            pnum1 = SFSM_CFG(1)->pnum;

            start_pg = 0;
            print_pg_num = pnum1;


            if (token_num >= 2)
                start_pg =  simple_strtoul(tokens[1], NULL, 0);
            if (token_num >= 3)
                print_pg_num =  simple_strtoul(tokens[2], NULL, 0);

            start_pg = start_pg % pnum1;
            if(print_pg_num > pnum1)
                print_pg_num = pnum1;

            printk("PTM RX BC 1 CELL data/ctrl buffer:\n\n");
            for(i = start_pg, num = 0 ; num < print_pg_num ; num ++, i = (i + 1) % pnum1 ) {

                struct PTM_CW_CTRL * p_ctrl;

                cw = dbase1 + i * cw_pg_size;
                p_ctrl = (struct PTM_CW_CTRL *) ( &cbase1[i]);

                printk("CW %2d: %08x                            ", i, cw[0]);
                printk("cwid=%d [%16s], cwer=%d, preempt=%d, short=%d\n",
                        p_ctrl->cwid, cwid_txt[p_ctrl->cwid], p_ctrl->cwer, p_ctrl->preempt, p_ctrl->shrt);

                printk("       %08x %08x %08x %08x ", cw[1], cw[2], cw[3], cw[4]);
                printk("state=%d, bad=%d, ber=%-3d, spos=%-3d, ffbn=%-3d\n",
                        p_ctrl->state, p_ctrl->bad, p_ctrl->ber, p_ctrl->spos, p_ctrl->ffbn);

                printk("       %08x %08x %08x %08x\n",  cw[5], cw[6], cw[7], cw[8]);
                printk("       %08x %08x %08x %08x\n",  cw[9], cw[10], cw[11], cw[12]);
                printk("       %08x %08x %08x %08x\n",  cw[13], cw[14], cw[15], cw[16]);

            }

        }
    }
     else if ( strincmp(tokens[0], "clear_tx_cb", 11) == 0 ) {

        unsigned int start_pg;
        unsigned int clear_pg_num;

        if(tokens[0][11] == '0' || tokens[0][11] == '\0') {
            volatile unsigned int *dbase0;
            unsigned int pnum0;

            dbase0 = (volatile unsigned int *)SB_BUFFER_BOND(2, FFSM_DBA(0)->dbase + 0x2000);
            pnum0 = FFSM_CFG(0)->pnum;

            start_pg = 0;
            clear_pg_num = pnum0;

            if (token_num >= 2)
                start_pg =  simple_strtoul(tokens[1], NULL, 0);
            if (token_num >= 3)
                clear_pg_num =  simple_strtoul(tokens[2], NULL, 0);

            start_pg = start_pg % pnum0;
            if(clear_pg_num > pnum0)
                clear_pg_num = pnum0;

            dbase0 = dbase0 + start_pg * cw_pg_size;

            memset((void *)dbase0, 0,  cw_pg_size * sizeof(unsigned int ) * clear_pg_num);

        }

        if (tokens[0][11] == '1' || tokens[0][11] == '\0'){

            volatile unsigned int *dbase1;
            unsigned int pnum1;

            dbase1 = (volatile unsigned int *)SB_BUFFER_BOND(2, FFSM_DBA(1)->dbase + 0x2000);
            pnum1 = FFSM_CFG(1)->pnum;

            start_pg = 0;
            clear_pg_num = pnum1;

            if (token_num >= 2)
                start_pg =  simple_strtoul(tokens[1], NULL, 0);
            if (token_num >= 3)
                clear_pg_num =  simple_strtoul(tokens[2], NULL, 0);

            start_pg = start_pg % pnum1;
            if(clear_pg_num > pnum1)
                clear_pg_num = pnum1;

            dbase1 = dbase1 + start_pg * cw_pg_size;

            memset((void *)dbase1, 0,  cw_pg_size * sizeof(unsigned int ) * clear_pg_num);

        }
    }

    else if ( strincmp(tokens[0], "clear_rx_cb", 11) == 0 ) {

        unsigned int start_pg;
        unsigned int clear_pg_num;

        if(tokens[0][11] == '0' || tokens[0][11] == '\0'){

            volatile unsigned int *dbase0, *cbase0;
            unsigned int pnum0;

            dbase0 = (volatile unsigned int *)SB_BUFFER_BOND(2, SFSM_DBA(0)->dbase + 0x2000);
            cbase0 = (volatile unsigned int *)SB_BUFFER_BOND(2, SFSM_CBA(0)->cbase + 0x2000);

            pnum0 = SFSM_CFG(0)->pnum;

            start_pg = 0;
            clear_pg_num = pnum0;

            if (token_num >= 2)
                start_pg =  simple_strtoul(tokens[1], NULL, 0);
            if (token_num >= 3)
                clear_pg_num =  simple_strtoul(tokens[2], NULL, 0);

            start_pg = start_pg % pnum0;
            if(clear_pg_num > pnum0)
                clear_pg_num = pnum0;

            dbase0 = dbase0 + start_pg * cw_pg_size;
            cbase0 = &cbase0[start_pg];

            memset((void *)dbase0, 0,  cw_pg_size * sizeof(unsigned int ) * clear_pg_num);
            memset((void *)cbase0, 0,  sizeof(unsigned int ) * clear_pg_num);

        }

        if (tokens[0][11] == '1' || tokens[0][11] == '\0'){

            volatile unsigned int *dbase1, *cbase1;
            unsigned int pnum1;

            dbase1 = (volatile unsigned int *)SB_BUFFER_BOND(2, SFSM_DBA(1)->dbase + 0x2000);
            cbase1 = (volatile unsigned int *)SB_BUFFER_BOND(2, SFSM_CBA(1)->cbase + 0x2000);
            pnum1 = SFSM_CFG(1)->pnum;

            start_pg = 0;
            clear_pg_num = pnum1;

            if (token_num >= 2)
                start_pg =  simple_strtoul(tokens[1], NULL, 0);
            if (token_num >= 3)
                clear_pg_num =  simple_strtoul(tokens[2], NULL, 0);

            start_pg = start_pg % pnum1;
            if(clear_pg_num > pnum1)
                clear_pg_num = pnum1;

            dbase1 = dbase1 + start_pg * cw_pg_size;
            cbase1 = &cbase1[start_pg];

            memset((void *)dbase1, 0,  cw_pg_size * sizeof(unsigned int )* clear_pg_num);
            memset((void *)cbase1, 0,  sizeof(unsigned int ) * clear_pg_num);
        }
    }

    else if ( strincmp(tokens[0], "clear_cb", 8) == 0 ) {

        unsigned int start_pg;
        unsigned int clear_pg_num;

        if(tokens[0][8] == '0' || tokens[0][8] == '\0'){

            //Clear tx_cb0
            volatile unsigned int *tx_dbase0;
            unsigned int tx_pnum0;
            volatile unsigned int *rx_dbase0, *rx_cbase0;
            unsigned int rx_pnum0;


            tx_dbase0 = (volatile unsigned int *)SB_BUFFER_BOND(2, FFSM_DBA(0)->dbase + 0x2000);
            tx_pnum0 = FFSM_CFG(0)->pnum;

            start_pg = 0;
            clear_pg_num = tx_pnum0;

            if (token_num >= 2)
                start_pg =  simple_strtoul(tokens[1], NULL, 0);
            if (token_num >= 3)
                clear_pg_num =  simple_strtoul(tokens[2], NULL, 0);

            start_pg = start_pg % tx_pnum0;

            if(clear_pg_num > tx_pnum0)
                clear_pg_num = tx_pnum0;

            tx_dbase0 = tx_dbase0 + start_pg * cw_pg_size;

            memset((void *)tx_dbase0, 0,  cw_pg_size * sizeof(unsigned int ) * clear_pg_num);

            //Clear rx_cb0

            rx_dbase0 = (volatile unsigned int *)SB_BUFFER_BOND(2, SFSM_DBA(0)->dbase + 0x2000);
            rx_cbase0 = (volatile unsigned int *)SB_BUFFER_BOND(2, SFSM_CBA(0)->cbase + 0x2000);

            rx_pnum0 = SFSM_CFG(0)->pnum;

            start_pg = 0;
            clear_pg_num = rx_pnum0;

            if (token_num >= 2)
                start_pg =  simple_strtoul(tokens[1], NULL, 0);
            if (token_num >= 3)
                clear_pg_num =  simple_strtoul(tokens[2], NULL, 0);

            start_pg = start_pg % rx_pnum0;
            if(clear_pg_num > rx_pnum0)
                clear_pg_num = rx_pnum0;

            rx_dbase0 = rx_dbase0 + start_pg * cw_pg_size;
            rx_cbase0 = &rx_cbase0[start_pg];

            memset((void *)rx_dbase0, 0,  cw_pg_size * sizeof(unsigned int ) * clear_pg_num);
            memset((void *)rx_cbase0, 0,  sizeof(unsigned int ) * clear_pg_num);

        }

        if(tokens[0][8] == '1' || tokens[0][8] == '\0'){

            //Clear tx_cb1
            volatile unsigned int *tx_dbase1;
            unsigned int tx_pnum1;
            volatile unsigned int *rx_dbase1, *rx_cbase1;
            unsigned int rx_pnum1;


            tx_dbase1 = (volatile unsigned int *)SB_BUFFER_BOND(2, FFSM_DBA(1)->dbase + 0x2000);
            tx_pnum1 = FFSM_CFG(1)->pnum;

            start_pg = 0;
            clear_pg_num = tx_pnum1;

            if (token_num >= 2)
                start_pg =  simple_strtoul(tokens[1], NULL, 0);
            if (token_num >= 3)
                clear_pg_num =  simple_strtoul(tokens[2], NULL, 0);

            start_pg = start_pg % tx_pnum1;

            if(clear_pg_num > tx_pnum1)
                clear_pg_num = tx_pnum1;

            tx_dbase1 = tx_dbase1 + start_pg * cw_pg_size;

            memset((void *)tx_dbase1, 0,  cw_pg_size * sizeof(unsigned int ) * clear_pg_num);

            rx_dbase1 = (volatile unsigned int *)SB_BUFFER_BOND(2, SFSM_DBA(1)->dbase + 0x2000);
            rx_cbase1 = (volatile unsigned int *)SB_BUFFER_BOND(2, SFSM_CBA(1)->cbase + 0x2000);
            rx_pnum1 = SFSM_CFG(1)->pnum;

            start_pg = 0;
            clear_pg_num = rx_pnum1;

            if (token_num >= 2)
                start_pg =  simple_strtoul(tokens[1], NULL, 0);
            if (token_num >= 3)
                clear_pg_num =  simple_strtoul(tokens[2], NULL, 0);

            start_pg = start_pg % rx_pnum1;
            if(clear_pg_num > rx_pnum1)
                clear_pg_num = rx_pnum1;

            rx_dbase1 = rx_dbase1 + start_pg * cw_pg_size;
            rx_cbase1 = &rx_cbase1[start_pg];

            memset((void *)rx_dbase1, 0,  cw_pg_size * sizeof(unsigned int ) * clear_pg_num);
            memset((void *)rx_cbase1, 0,  sizeof(unsigned int )* clear_pg_num);

        }

    }

    else if ( strincmp(tokens[0], "clear_eth_dbuf", 14) == 0) {
        volatile u32 *p_dbase;
        unsigned int pg_size = 0;
        unsigned int pg_num  = 0;
        unsigned int start_pg;
        unsigned int clear_pg_num;
        SAVE_AND_SWITCH_PPE_BASE_TO_SM(saved_ppe_base_addr);

        pg_num  =  ( ( struct dmrx_cfg * )DM_RXCFG)->pnum;
        pg_size =  ( ( ( struct  dmrx_cfg * )DM_RXCFG)->psize + 1) * 32 / 4;

        p_dbase =  (volatile unsigned int *)SB_BUFFER_BOND(2, ( ( struct dmrx_dba * )DM_RXDB)->dbase + 0x2000 );

        start_pg = 0;
        clear_pg_num = pg_num;

        if (token_num >= 2)
            start_pg =  simple_strtoul(tokens[1], NULL, 0);
        if (token_num >= 3)
            clear_pg_num =  simple_strtoul(tokens[2], NULL, 0);

        start_pg = start_pg % pg_num;
        if(clear_pg_num > pg_num)
            clear_pg_num = pg_num;

        p_dbase = p_dbase + start_pg * pg_size;

        memset((void *)p_dbase, 0, pg_size * sizeof(unsigned int) * clear_pg_num);
        RESTORE_PPE_BASE(saved_ppe_base_addr);
    }
    else if ( strincmp(tokens[0], "clear_eth_cbuf", 14) == 0) {
        volatile u32 *p_cbase;
        unsigned int pg_num  = 0;
        unsigned int start_pg;
        unsigned int clear_pg_num;
        SAVE_AND_SWITCH_PPE_BASE_TO_SM(saved_ppe_base_addr);

        pg_num  =  ( ( struct  dmrx_cfg * )DM_RXCFG )->pnum;
        p_cbase =  (volatile unsigned int *)SB_BUFFER_BOND(2, ( ( struct  dmrx_cba * )DM_RXCB)->cbase + 0x2000);

        start_pg = 0;
        clear_pg_num = pg_num;

        if (token_num >= 2)
            start_pg =  simple_strtoul(tokens[1], NULL, 0);
        if (token_num >= 3)
            clear_pg_num =  simple_strtoul(tokens[2], NULL, 0);

        start_pg = start_pg % pg_num;
        if(clear_pg_num > pg_num)
            clear_pg_num = pg_num;

        p_cbase = &p_cbase[start_pg];


        memset((void *)p_cbase, 0, sizeof(unsigned int) * clear_pg_num );
        RESTORE_PPE_BASE(saved_ppe_base_addr);
    }

    else if ( strincmp(tokens[0], "clear_ethbuf", 12) == 0)
    {
        volatile u32 *p_dbase, *p_cbase;
        unsigned int pg_size = 0;
        unsigned int pg_num  = 0;
        unsigned int start_pg;
        unsigned int clear_pg_num;
        SAVE_AND_SWITCH_PPE_BASE_TO_SM(saved_ppe_base_addr);

        //Clear ethernet data buffer
        pg_num  =  ( ( struct dmrx_cfg * )DM_RXCFG)->pnum;
        pg_size =  ( ( ( struct  dmrx_cfg * )DM_RXCFG)->psize + 1) * 32 / 4;

        p_dbase =  (volatile unsigned int *)SB_BUFFER_BOND(2, ( ( struct dmrx_dba * )DM_RXDB)->dbase +0x2000);

        start_pg = 0;
        clear_pg_num = pg_num;

        if (token_num >= 2)
            start_pg =  simple_strtoul(tokens[1], NULL, 0);
        if (token_num >= 3)
            clear_pg_num =  simple_strtoul(tokens[2], NULL, 0);

        start_pg = start_pg % pg_num;
        if(clear_pg_num > pg_num)
            clear_pg_num = pg_num;

        p_dbase = p_dbase + start_pg * pg_size;

        memset((void *)p_dbase, 0, pg_size * sizeof(unsigned int) * clear_pg_num);

        //Clear ethernet control buffer
        pg_num  =  ( ( struct  dmrx_cfg * )DM_RXCFG )->pnum;
        p_cbase =  (volatile unsigned int *)SB_BUFFER_BOND(2, ( ( struct  dmrx_cba * )DM_RXCB)->cbase + 0x2000 );

        start_pg = 0;
        clear_pg_num = pg_num;

        if (token_num >= 2)
            start_pg =  simple_strtoul(tokens[1], NULL, 0);
        if (token_num >= 3)
            clear_pg_num =  simple_strtoul(tokens[2], NULL, 0);

        start_pg = start_pg % pg_num;
        if(clear_pg_num > pg_num)
            clear_pg_num = pg_num;

        p_cbase = &p_cbase[start_pg];

        memset((void *)p_cbase, 0, sizeof(unsigned int) * clear_pg_num );
        RESTORE_PPE_BASE(saved_ppe_base_addr);
    }

    else if ( strincmp(tokens[0], "us_bg_ctxt", 10) == 0 )  {
        int k = 0;
        unsigned int pkt_status = 0;
        SAVE_AND_SWITCH_PPE_BASE_TO_BM(saved_ppe_base_addr);

        for(; k<4; k++) {
            printk( "US_BG_CTXT[%d]\n", k);
            printk( "         sid: 0x%04x     sop: %x      eop: %x  pkt_status: %x    des_addr: 0x%04x \n",
                US_BG_CTXT_BASE(k)->sid, US_BG_CTXT_BASE(k)->sop, US_BG_CTXT_BASE(k)->eop,
                US_BG_CTXT_BASE(k)->pkt_status, US_BG_CTXT_BASE(k)->des_addr);
            printk( "    data_ptr: 0x%08x qid: %x   gif_id: %x     rem_len: %-4u \n\n",
                US_BG_CTXT_BASE(k)->data_ptr, US_BG_CTXT_BASE(k)->qid, US_BG_CTXT_BASE(k)->gif_id,
                US_BG_CTXT_BASE(k)->rem_len);

            pkt_status = US_BG_CTXT_BASE(k)->pkt_status;

            if(pkt_status == 1) {
                volatile struct US_BOND_PKT_DES * p_des = (volatile struct US_BOND_PKT_DES *) SB_BUFFER_BOND(2, US_BG_CTXT_BASE(k)->des_addr);
                int q_no = (US_BG_CTXT_BASE(k)->des_addr - 0x5c00)/0x80;
                printk("    ==>US_BOND_PKT_DES_BASE[%d]\n", q_no);
                printk("              own: %x         c: %x        sop: %x           eop: %x           byte_off: %d \n",
                    p_des->own,p_des->c,
                    p_des->sop,p_des->eop,
                    p_des->byte_off);
                printk("        last_frag: %x  frag_num: %-2d  data_len: %-4d   data_ptr: 0x%08x\n\n",
                    p_des->last_frag,p_des->frag_num,
                    p_des->data_len,p_des->data_ptr);

           }
        }
        RESTORE_PPE_BASE(saved_ppe_base_addr);
    }
    else if ( strincmp(tokens[0], "ds_bg_ctxt", 10) == 0 )   {
        int k = 0;
        SAVE_AND_SWITCH_PPE_BASE_TO_BM(saved_ppe_base_addr);
        for(; k<4; k++) {
            printk( "DS_BG_CTXT[%d]\n", k);
            printk( "       link_state_chg: %u        expected_sid: 0x%04x     last_sop: %u      last_eop: %u\t  bg_pkt_state: %u\n",
                DS_BG_CTXT_BASE(k)->link_state_chg, DS_BG_CTXT_BASE(k)->expected_sid, DS_BG_CTXT_BASE(k)->last_sop,
                DS_BG_CTXT_BASE(k)->last_eop, DS_BG_CTXT_BASE(k)->bg_pkt_state);
            printk( "        noncosec_flag: %u       oversize_flag: %u       no_eop_flag: %u   no_sop_flag: %u\t   no_err_flag: %u \n",
                DS_BG_CTXT_BASE(k)->noncosec_flag, DS_BG_CTXT_BASE(k)->oversize_flag, DS_BG_CTXT_BASE(k)->no_eop_flag,
                DS_BG_CTXT_BASE(k)->no_sop_flag, DS_BG_CTXT_BASE(k)->no_err_flag);
            printk( "    curr_pkt_frag_cnt: %-3d curr_pkt_byte_cnt: %-4u \n\n",
                DS_BG_CTXT_BASE(k)->curr_pkt_frag_cnt, DS_BG_CTXT_BASE(k)->curr_pkt_byte_cnt);
        }
        RESTORE_PPE_BASE(saved_ppe_base_addr);
    }
    else if ( strincmp(tokens[0], "us_bond_gif_mib", 15) == 0 ) {
        int k = 0;
        SAVE_AND_SWITCH_PPE_BASE_TO_BM(saved_ppe_base_addr);
        printk("US bonding gif frag mib: \n");
        for(k = 0; k < 8; k ++ ) {
            printk("    gif%d: %-u\n", k, * US_BOND_GIF_MIB(k));
        }
        RESTORE_PPE_BASE(saved_ppe_base_addr);
    }
    else if ( strincmp(tokens[0], "clear_us_bond_gif_mib", 21) == 0 ) {
        int k = 0;
        SAVE_AND_SWITCH_PPE_BASE_TO_BM(saved_ppe_base_addr);
        for(k = 0; k < 8; k ++ ) {
            void *p = (void *)US_BOND_GIF_MIB(k);
            memset(p, 0, sizeof(unsigned int));
        }
        RESTORE_PPE_BASE(saved_ppe_base_addr);
    }
    else if ( strincmp(tokens[0], "ds_bond_gif_mib", 15) == 0 ) {
        int k = 0;
        SAVE_AND_SWITCH_PPE_BASE_TO_BM(saved_ppe_base_addr);
        for(; k<8; k++) {
            printk( "DS_BOND_GIF_MIB[%d]\n", k);
            printk( "       total_rx_frag_cnt: %-10u total_rx_byte_cnt: %-10u overflow_frag_cnt: %-10u overflow_byte_cnt: %-10u\n",
                DS_BOND_GIF_MIB_BASE(k)->total_rx_frag_cnt, DS_BOND_GIF_MIB_BASE(k)->total_rx_byte_cnt,
                DS_BOND_GIF_MIB_BASE(k)->overflow_frag_cnt, DS_BOND_GIF_MIB_BASE(k)->overflow_byte_cnt);
            printk( "   out_of_range_frag_cnt: %-10u  missing_frag_cnt: %-10u  timeout_frag_cnt: %-10u\n\n",
                DS_BOND_GIF_MIB_BASE(k)->out_of_range_frag_cnt, DS_BOND_GIF_MIB_BASE(k)->missing_frag_cnt,
                DS_BOND_GIF_MIB_BASE(k)->timeout_frag_cnt);
        }
        RESTORE_PPE_BASE(saved_ppe_base_addr);
    }
    else if ( strincmp(tokens[0], "clear_ds_bond_gif_mib", 21) == 0 ) {
        int k = 0;
        SAVE_AND_SWITCH_PPE_BASE_TO_BM(saved_ppe_base_addr);
        for(; k<8; k++) {
            void *p = (void *)DS_BOND_GIF_MIB_BASE(k);
            memset(p, 0, sizeof(struct DS_BOND_GIF_MIB));
        }
        RESTORE_PPE_BASE(saved_ppe_base_addr);
    }

    else if ( strincmp(tokens[0], "ds_bg_mib", 9) == 0 ) {
        int k = 0;
        SAVE_AND_SWITCH_PPE_BASE_TO_BM(saved_ppe_base_addr);
        for(; k<4; k++) {
            printk( "DS_BG_MIB[%d]\n", k);
            printk( "     conform_pkt_cnt: %-10u  conform_frag_cnt: %-10u conform_byte_cnt: %-10u    no_sop_pkt_cnt: %-10u   no_sop_frag_cnt: %-10u\n",
                DS_BG_MIB_BASE(k)->conform_pkt_cnt, DS_BG_MIB_BASE(k)->conform_frag_cnt,
                DS_BG_MIB_BASE(k)->conform_byte_cnt, DS_BG_MIB_BASE(k)->no_sop_pkt_cnt, DS_BG_MIB_BASE(k)->no_sop_frag_cnt);
            printk( "     no_sop_byte_cnt: %-10u    no_eop_pkt_cnt: %-10u  no_eop_frag_cnt: %-10u   no_eop_byte_cnt: %-10u  oversize_pkt_cnt: %-10u\n",
                DS_BG_MIB_BASE(k)->no_sop_byte_cnt, DS_BG_MIB_BASE(k)->no_eop_pkt_cnt, DS_BG_MIB_BASE(k)->no_eop_frag_cnt,
                DS_BG_MIB_BASE(k)->no_eop_byte_cnt, DS_BG_MIB_BASE(k)->oversize_pkt_cnt);
            printk( "   oversize_frag_cnt: %-10u oversize_byte_cnt: %-10u noncosec_pkt_cnt: %-10u noncosec_frag_cnt: %-10u noncosec_byte_cnt: %-10u\n\n",
                DS_BG_MIB_BASE(k)->oversize_frag_cnt, DS_BG_MIB_BASE(k)->oversize_byte_cnt, DS_BG_MIB_BASE(k)->noncosec_pkt_cnt,
                DS_BG_MIB_BASE(k)-> noncosec_frag_cnt, DS_BG_MIB_BASE(k)->noncosec_byte_cnt);
        }
        RESTORE_PPE_BASE(saved_ppe_base_addr);
    }
    else if ( strincmp(tokens[0], "clear_ds_bg_mib", 15) == 0 ) {
        int k = 0;
        SAVE_AND_SWITCH_PPE_BASE_TO_BM(saved_ppe_base_addr);
        for(; k<4; k++) {
            void *p = (void *)DS_BG_MIB_BASE(k);
            memset(p, 0, sizeof(struct DS_BG_MIB));
        }
        RESTORE_PPE_BASE(saved_ppe_base_addr);
    }
    else if ( strincmp(tokens[0], "ds_link_list", 12) == 0 ) {
        unsigned int i;
        unsigned char buf[8];
        unsigned int total_des_num = 0;
        SAVE_AND_SWITCH_PPE_BASE_TO_BM(saved_ppe_base_addr);

        check_ds_ll_list(DS_BOND_FREE_LL_CTXT_BASE,    "Free: ");
        for( i = 0; i < 8; i ++ ) {
            sprintf(buf, "Gif%d: ", i);
            check_ds_ll_list(DS_BOND_GIF_LL_CTXT_BASE(i), buf);
        }

        for(i = 0; i < 4; i ++) {
            sprintf(buf, "BgR%d: ", i);
            check_ds_ll_list( (volatile struct DS_BOND_GIF_LL_CTXT *)
                                    ( (unsigned int *) (DS_BG_CTXT_BASE(i))  + 4 ), buf);
        }

        total_des_num = DS_BOND_FREE_LL_CTXT_BASE->des_num;
        for( i = 0; i < 8; i ++ ) {
            total_des_num += DS_BOND_GIF_LL_CTXT_BASE(i)->des_num;
        }
        for(i = 0; i < 4; i ++) {
            total_des_num +=   ( (volatile struct DS_BOND_GIF_LL_CTXT *)
                                 ( (unsigned int *) (DS_BG_CTXT_BASE(i))  + 4 )) ->des_num;
        }

        if (total_des_num != 256) {
            printk("[X] ");
        }
        else {
            printk("[Y] ");
        }
        printk("total_des_num = %d\n\n", total_des_num);
        RESTORE_PPE_BASE(saved_ppe_base_addr);
    }
    else if ( strincmp(tokens[0], "rx_bc_cfg", 9) == 0 )  {

        unsigned int bc_s = 0;
        unsigned int bc_e = 1;
        int i = 0;
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

        if(tokens[0][10] == '\0' && token_num == 1) {
            bc_s = 0;
            bc_e = 1;
        }
        else if ( tokens[0][10] <= '1' && tokens[0][10] >= '0'  ) {
            bc_s = bc_e = tokens[0][10] - '0';
        }
        else if ( token_num >= 2 && tokens[1][0] <= '1' && tokens[1][0] >= '0'  ) {
            bc_s = bc_e = tokens[1][0] - '0';
        }

        for ( i = bc_s; i <= bc_e; i ++ ) {
            printk("TX_GIF_CFG[%d]\n", i);
            printk("    local_state: %d [ %-20s ] remote_state: %d [ %-12s ]  rx_cw_rdptr:%d \n\n",
                   RX_BC_CFG(i)->local_state, ls_txt[RX_BC_CFG(i)->local_state],
                   RX_BC_CFG(i)->remote_state, rs_txt[RX_BC_CFG(i)->remote_state],
                   RX_BC_CFG(i)->rx_cw_rdptr);
        }

    }
    else if ( strincmp(tokens[0], "tx_bc_cfg", 9) == 0 )  {
    }
    else if ( strincmp(tokens[0], "rx_gif_cfg", 10) == 0 )  {
    }
    else if ( strincmp(tokens[0], "tx_gif_cfg", 10) == 0 )  {
        unsigned int gif_s = 0;
        unsigned int gif_e = 0;
        int i = 0;
        if(tokens[0][10] == '\0' && token_num == 1) {
            gif_s = 0;
            gif_e = 3;
        }
        else if ( tokens[0][10] <= '3' && tokens[0][10] >= '0'  ) {
            gif_s = gif_e = tokens[0][10] - '0';
        }
        else if ( token_num >= 2 && tokens[1][0] <= '3' && tokens[1][0] >= '0'  ) {
            gif_s = gif_e = tokens[1][0] - '0';
        }

        for ( i = gif_s; i <= gif_e; i ++ ) {
            printk("TX_GIF_CFG[%d]\n", i);
            printk("    curr_qid: %d     fill_pkt_state: %d      post_pkt_state: %d  curr_pdma_context_ptr: 0x%04x\n"
                   "     des_qid: %d           des_addr: 0x%04x                     curr_sar_context_ptr: 0x%04x\n"
                   "    rem_data: %-4d         rem_crc: %d          rem_fh_len: %d\n"
                   "     des_dw0: 0x%08x   des_dw1: 0x%08x  des_bp_dw: 0x%08x\n\n",
                   TX_GAMMA_ITF_CFG(i)->curr_qid, TX_GAMMA_ITF_CFG(i)->fill_pkt_state, TX_GAMMA_ITF_CFG(i)->post_pkt_state, TX_GAMMA_ITF_CFG(i)->curr_pdma_context_ptr,
                   TX_GAMMA_ITF_CFG(i)->des_qid,  TX_GAMMA_ITF_CFG(i)->des_addr, TX_GAMMA_ITF_CFG(i)->curr_sar_context_ptr,
                   TX_GAMMA_ITF_CFG(i)->rem_data, TX_GAMMA_ITF_CFG(i)->rem_crc, TX_GAMMA_ITF_CFG(i)->rem_fh_len,
                   TX_GAMMA_ITF_CFG(i)->des_dw0, TX_GAMMA_ITF_CFG(i)->des_dw1, TX_GAMMA_ITF_CFG(i)->des_bp_dw);

        }

    }
    else if ( strincmp(tokens[0], "idlekeep", 8) == 0 ) {
        printk("BC0.idlekp = %d, BC1.idlekp = %d\n",
            (*SFSM_CFG0 >> 15) & 1, (*SFSM_CFG1 >> 15) & 1);
    }
    else if ( strincmp(tokens[0], "clear", 5) == 0 ) {
        if(token_num >= 2) {
            if(strincmp(tokens[1], "idlekeep", 8) == 0) {
                if(tokens[1][8] == '\0' || tokens[1][8] == '0')
                    *SFSM_CFG0 = ( *SFSM_CFG0 ) & ( ~ ( 1 << 15) );
                if(tokens[1][8] == '\0' || tokens[1][8] == '1')
                    *SFSM_CFG1 = ( *SFSM_CFG1 ) & ( ~ ( 1 << 15) );
            }
        }
    }
    else if ( strincmp(tokens[0], "set", 3) == 0 ) {
        if(token_num >= 2) {
            if(strincmp(tokens[1], "idlekeep", 8) == 0) {
                if(tokens[1][8] == '\0' || tokens[1][8] == '0')
                    *SFSM_CFG0 =  *SFSM_CFG0 | ( 1 << 15) ;
                if(tokens[1][8] == '\0' || tokens[1][8] == '1')
                    *SFSM_CFG1 =  *SFSM_CFG1 | ( 1 << 15);
            }
        }

    }


    // MOD_DEC_USE_COUNT;

    return count;
}

#endif //#if defined(DEBUG_BONDING_PROC) && DEBUG_BONDING_PROC



#if defined(DEBUG_DUMP_INIT) && DEBUG_DUMP_INIT
static INLINE void dump_init(void)
{
	int i;

	if ( !(g_dbg_datapath & DBG_ENABLE_MASK_DUMP_INIT) )
	return;

	printk("Share Buffer Conf:\n");
	printk("  SB_MST_SEL(%08X) = 0x%08X\n", (u32)SB_MST_SEL, *SB_MST_SEL);

	printk("ETOP:\n");
	printk("  ETOP_MDIO_CFG        = 0x%08X\n", *ETOP_MDIO_CFG);
	printk("  ETOP_MDIO_ACC        = 0x%08X\n", *ETOP_MDIO_ACC);
	printk("  ETOP_CFG             = 0x%08X\n", *ETOP_CFG);
	printk("  ETOP_IG_VLAN_COS     = 0x%08X\n", *ETOP_IG_VLAN_COS);
	printk("  ETOP_IG_DSCP_COSx(0) = 0x%08X\n", *ETOP_IG_DSCP_COSx(0));
	printk("  ETOP_IG_DSCP_COSx(1) = 0x%08X\n", *ETOP_IG_DSCP_COSx(1));
	printk("  ETOP_IG_DSCP_COSx(2) = 0x%08X\n", *ETOP_IG_DSCP_COSx(2));
	printk("  ETOP_IG_DSCP_COSx(3) = 0x%08X\n", *ETOP_IG_DSCP_COSx(3));
	printk("  ETOP_IG_PLEN_CTRL0   = 0x%08X\n", *ETOP_IG_PLEN_CTRL0);
	printk("  ETOP_ISR             = 0x%08X\n", *ETOP_ISR);
	printk("  ETOP_IER             = 0x%08X\n", *ETOP_IER);
	printk("  ETOP_VPID            = 0x%08X\n", *ETOP_VPID);

	for ( i = 0; i < 2; i++ )
	{
		printk("ENET%d:\n", i);
		printk("  ENET_MAC_CFG(%d)      = 0x%08X\n", i, *ENET_MAC_CFG(i));
		printk("  ENETS_DBA(%d)         = 0x%08X\n", i, *ENETS_DBA(i));
		printk("  ENETS_CBA(%d)         = 0x%08X\n", i, *ENETS_CBA(i));
		printk("  ENETS_CFG(%d)         = 0x%08X\n", i, *ENETS_CFG(i));
		printk("  ENETS_PGCNT(%d)       = 0x%08X\n", i, *ENETS_PGCNT(i));
		printk("  ENETS_PKTCNT(%d)      = 0x%08X\n", i, *ENETS_PKTCNT(i));
		printk("  ENETS_BUF_CTRL(%d)    = 0x%08X\n", i, *ENETS_BUF_CTRL(i));
		printk("  ENETS_COS_CFG(%d)     = 0x%08X\n", i, *ENETS_COS_CFG(i));
		printk("  ENETS_IGDROP(%d)      = 0x%08X\n", i, *ENETS_IGDROP(i));
		printk("  ENETS_IGERR(%d)       = 0x%08X\n", i, *ENETS_IGERR(i));
		printk("  ENETS_MAC_DA0(%d)     = 0x%08X\n", i, *ENETS_MAC_DA0(i));
		printk("  ENETS_MAC_DA1(%d)     = 0x%08X\n", i, *ENETS_MAC_DA1(i));
		printk("  ENETF_DBA(%d)         = 0x%08X\n", i, *ENETF_DBA(i));
		printk("  ENETF_CBA(%d)         = 0x%08X\n", i, *ENETF_CBA(i));
		printk("  ENETF_CFG(%d)         = 0x%08X\n", i, *ENETF_CFG(i));
		printk("  ENETF_PGCNT(%d)       = 0x%08X\n", i, *ENETF_PGCNT(i));
		printk("  ENETF_PKTCNT(%d)      = 0x%08X\n", i, *ENETF_PKTCNT(i));
		printk("  ENETF_HFCTRL(%d)      = 0x%08X\n", i, *ENETF_HFCTRL(i));
		printk("  ENETF_TXCTRL(%d)      = 0x%08X\n", i, *ENETF_TXCTRL(i));
		printk("  ENETF_VLCOS0(%d)      = 0x%08X\n", i, *ENETF_VLCOS0(i));
		printk("  ENETF_VLCOS1(%d)      = 0x%08X\n", i, *ENETF_VLCOS1(i));
		printk("  ENETF_VLCOS2(%d)      = 0x%08X\n", i, *ENETF_VLCOS2(i));
		printk("  ENETF_VLCOS3(%d)      = 0x%08X\n", i, *ENETF_VLCOS3(i));
		printk("  ENETF_EGCOL(%d)       = 0x%08X\n", i, *ENETF_EGCOL(i));
		printk("  ENETF_EGDROP(%d)      = 0x%08X\n", i, *ENETF_EGDROP(i));
	}

	printk("DPLUS:\n");
	printk("  DPLUS_TXDB           = 0x%08X\n", *DPLUS_TXDB);
	printk("  DPLUS_TXCB           = 0x%08X\n", *DPLUS_TXCB);
	printk("  DPLUS_TXCFG          = 0x%08X\n", *DPLUS_TXCFG);
	printk("  DPLUS_TXPGCNT        = 0x%08X\n", *DPLUS_TXPGCNT);
	printk("  DPLUS_RXDB           = 0x%08X\n", *DPLUS_RXDB);
	printk("  DPLUS_RXCB           = 0x%08X\n", *DPLUS_RXCB);
	printk("  DPLUS_RXCFG          = 0x%08X\n", *DPLUS_RXCFG);
	printk("  DPLUS_RXPGCNT        = 0x%08X\n", *DPLUS_RXPGCNT);

	printk("Communication:\n")
	printk("  FW_VER_ID(%08X)  = 0x%08X\n", (u32)FW_VER_ID, *(u32*)FW_VER_ID);
}
#endif


static int swap_mac(unsigned char *data __attribute__((unused)))
{
    int ret = 0;

#if defined(ENABLE_DBG_PROC) && ENABLE_DBG_PROC
    if ( (g_dbg_datapath & DBG_ENABLE_MASK_MAC_SWAP) )
    {
        unsigned char tmp[8];
        unsigned char *p = data;

        if ( p[12] == 0x08 && p[13] == 0x06 ) { //  arp
            if ( p[14] == 0x00 && p[15] == 0x01 && p[16] == 0x08 && p[17] == 0x00 && p[20] == 0x00 && p[21] == 0x01 ) {
                //  dest mac
                memcpy(p, p + 6, 6);
                //  src mac
                p[6] = p[7] = 0;
                memcpy(p + 8, p + 38, 4);
                //  arp reply
                p[21] = 0x02;
                //  sender mac
                memcpy(p + 22, p + 6, 6);
                //  sender IP
                memcpy(tmp, p + 28, 4);
                memcpy(p + 28, p + 38, 4);
                //  target mac
                memcpy(p + 32, p, 6);
                //  target IP
                memcpy(p + 38, tmp, 4);

                ret = 42;
            }
        }
        else if ( !(p[0] & 0x01) ) { //  bypass broadcast/multicast
            //  swap MAC
            memcpy(tmp, p, 6);
            memcpy(p, p + 6, 6);
            memcpy(p + 6, tmp, 6);
            p += 12;

            //  bypass VLAN
            while ( p[0] == 0x81 && p[1] == 0x00 )
                p += 4;

            //  IP
            if ( p[0] == 0x08 && p[1] == 0x00 ) {
                p += 14;
                memcpy(tmp, p, 4);
                memcpy(p, p + 4, 4);
                memcpy(p + 4, tmp, 4);
                p += 8;
            }

            ret = (int)((unsigned long)p - (unsigned long)data);
        }
    }
#endif

    return ret;
}


static INLINE void register_netdev_event_handler(void) {
	g_netdev_event_handler_nb.notifier_call = netdev_event_handler;
	register_netdevice_notifier(&g_netdev_event_handler_nb);
}

static INLINE void unregister_netdev_event_handler(void) {
	unregister_netdevice_notifier(&g_netdev_event_handler_nb);
}

#ifdef AVM_DOESNT_USE_IFX_ETHSW_API
static int netdev_event_handler(struct notifier_block *nb, unsigned long event, void *netdev)
{
	struct net_device *netif = (struct net_device *)netdev;
	IFX_ETHSW_HANDLE handle;
	union ifx_sw_param x;

	if ( netif->type != ARPHRD_ETHER )
	return NOTIFY_DONE;

	if ( (netif->flags & IFF_POINTOPOINT) ) //  ppp interface
	return NOTIFY_DONE;

	switch ( event )
	{
		case NETDEV_UP:
		handle = ifx_ethsw_kopen("/dev/switch/0");
		memset(&x.MAC_tableAdd, 0x00, sizeof(x.MAC_tableAdd));
		x.MAC_tableAdd.nFId = 0;
		x.MAC_tableAdd.nPortId = 6; //  CPU port
		//if ( netif->name[0] == 'p' && netif->name[1] == 't' && netif->name[2] == 'm' )  //  ptm
		//    x.MAC_tableAdd.nPortId = 11;
		//else if ( netif->name[0] == 'n' && netif->name[1] == 'a' && netif->name[2] == 's' ) //  atm
		//    x.MAC_tableAdd.nPortId = 11;
		//else if ( netif->name[0] == 'e' && netif->name[1] == 't' && netif->name[2] == 'h' && netif->name[3] == '1' )    //  eth1
		//    x.MAC_tableAdd.nPortId = ;
		//else
		//{
		//    int i;
		//
		//    for ( i = 0; i < NUM_ENTITY(ppa_pushthrough_data); i++ )
		//    {
		//        if ( netif == ppa_pushthrough_data[i].netif )
		//            x.MAC_tableAdd.nPortId = 7 + i;
		//    }
		//}
		x.MAC_tableAdd.nAgeTimer = 3;
		x.MAC_tableAdd.bStaticEntry = 1;
		x.MAC_tableAdd.nMAC[0] = netif->dev_addr[0];
		x.MAC_tableAdd.nMAC[1] = netif->dev_addr[1];
		x.MAC_tableAdd.nMAC[2] = netif->dev_addr[2];
		x.MAC_tableAdd.nMAC[3] = netif->dev_addr[3];
		x.MAC_tableAdd.nMAC[4] = netif->dev_addr[4];
		x.MAC_tableAdd.nMAC[5] = netif->dev_addr[5];
		printk( KERN_ERR "[%s] setup netif->dev_addr in Switch\n", __FUNCTION__);
		ifx_ethsw_kioctl(handle, IFX_ETHSW_MAC_TABLE_ENTRY_ADD, (unsigned int)&x.MAC_tableAdd);
		ifx_ethsw_kclose(handle);
		return NOTIFY_OK;
		case NETDEV_DOWN:
		handle = ifx_ethsw_kopen("/dev/switch/0");
		memset(&x.MAC_tableRemove, 0x00, sizeof(x.MAC_tableRemove));
		x.MAC_tableRemove.nFId = 0;
		x.MAC_tableRemove.nMAC[0] = netif->dev_addr[0];
		x.MAC_tableRemove.nMAC[1] = netif->dev_addr[1];
		x.MAC_tableRemove.nMAC[2] = netif->dev_addr[2];
		x.MAC_tableRemove.nMAC[3] = netif->dev_addr[3];
		x.MAC_tableRemove.nMAC[4] = netif->dev_addr[4];
		x.MAC_tableRemove.nMAC[5] = netif->dev_addr[5];
		ifx_ethsw_kioctl(handle, IFX_ETHSW_MAC_TABLE_ENTRY_REMOVE, (unsigned int)&x.MAC_tableRemove);
		ifx_ethsw_kclose(handle);
		printk( KERN_ERR "[%s] setup remove addr in switch\n", __FUNCTION__);
		return NOTIFY_OK;
	}

	return NOTIFY_DONE;
}
#else
static int netdev_event_handler(
		struct notifier_block *nb __attribute__ ((unused)),
		unsigned long event __attribute__ ((unused)),
		void *netdev __attribute__ ((unused))) {
	return NOTIFY_DONE;
}
#endif

/*
 * ####################################
 *           Global Function
 * ####################################
 */

static int32_t ppa_datapath_generic_hook(PPA_GENERIC_HOOK_CMD cmd,
		void *buffer, uint32_t flag __attribute__((unused))) {
	dbg("ppa_datapath_generic_hook cmd 0x%x\n", cmd);
	switch (cmd) {
	case PPA_GENERIC_DATAPATH_GET_PPE_VERION: {
		PPA_VERSION *version = (PPA_VERSION *) buffer;
		if (version->index >= NUM_ENTITY(g_fw_ver))
			return IFX_FAILURE;
		version->family = g_fw_ver[version->index].family;
            version->type   = g_fw_ver[version->index].package;
		version->major = g_fw_ver[version->index].major;
            version->mid    = g_fw_ver[version->index].middle;
		version->minor = g_fw_ver[version->index].minor;

		return IFX_SUCCESS;
	}
	case PPA_GENERIC_DATAPATH_ADDR_TO_FPI_ADDR: {
		PPA_FPI_ADDR *addr = (PPA_FPI_ADDR *) buffer;
		addr->addr_fpi = sb_addr_to_fpi_addr_convert(addr->addr_orig);
		return IFX_SUCCESS;
	}
    case PPA_GENERIC_DATAPATH_CLEAR_MIB:
        {
        }
    case PPA_GENERIC_DATAPATH_SET_LAN_SEPARATE_FLAG:
        {
            lan_port_seperate_enabled = flag;
        }
    case PPA_GENERIC_DATAPATH_GET_LAN_SEPARATE_FLAG:
        {
            return lan_port_seperate_enabled;
        }
    case PPA_GENERIC_DATAPATH_SET_WAN_SEPARATE_FLAG:
        {
            wan_port_seperate_enabled = flag;
        }
    case PPA_GENERIC_DATAPATH_GET_WAN_SEPARATE_FLAG:
        {
            return wan_port_seperate_enabled;
        }
	default:
		dbg("ppa_datapath_generic_hook not support cmd 0x%x\n", cmd);
		return IFX_FAILURE;
	}

	return IFX_FAILURE;
}

static int dsl_showtime_enter(struct port_cell_info *port_cell __attribute__((unused)), void *xdata_addr)
{
	ASSERT(port_cell != NULL, "port_cell is NULL");
	ASSERT(xdata_addr != NULL, "xdata_addr is NULL");

	//  TODO: ReTX set xdata_addr
	g_xdata_addr = xdata_addr;

	g_showtime = 1;
	wmb();

//	*UTP_CFG = 0x0F;

//#ifdef CONFIG_VR9
//    SW_WRITE_REG32_MASK(1 << 17, 0, FFSM_CFG0);
//#endif

	dbg("enter showtime");

	return IFX_SUCCESS;
}

static int dsl_showtime_exit(void)
{
	if (!g_showtime)
		return IFX_ERROR;

//#ifdef CONFIG_VR9
//    SW_WRITE_REG32_MASK(0, 1 << 17, FFSM_CFG0);
//#endif

	//*UTP_CFG = 0x00;

	g_showtime = 0;
	wmb();

	//  TODO: ReTX clean state
	g_xdata_addr = NULL;

	dbg("leave showtime");

	return IFX_SUCCESS;
}

#ifdef AVM_DOESNT_USE_IFX_ETHSW_API
int mac_entry_setting(unsigned char *mac, uint32_t fid, uint32_t portid, uint32_t agetime, uint32_t st_entry , uint32_t action)
{
	IFX_ETHSW_HANDLE handle;
	union ifx_sw_param x;
	int ret = IFX_SUCCESS;

	if(!mac)
	return IFX_ERROR;

	dump_stack();
	switch(action)
	{
		case IFX_ETHSW_MAC_TABLE_ENTRY_ADD:
		handle = ifx_ethsw_kopen("/dev/switch/0");
		memset(&x.MAC_tableAdd, 0x00, sizeof(x.MAC_tableAdd));
		x.MAC_tableAdd.nFId = fid;
		x.MAC_tableAdd.nPortId = portid;
		x.MAC_tableAdd.nAgeTimer = agetime;
		x.MAC_tableAdd.bStaticEntry = st_entry;
		x.MAC_tableAdd.nMAC[0] = mac[0];
		x.MAC_tableAdd.nMAC[1] = mac[1];
		x.MAC_tableAdd.nMAC[2] = mac[2];
		x.MAC_tableAdd.nMAC[3] = mac[3];
		x.MAC_tableAdd.nMAC[4] = mac[4];
		x.MAC_tableAdd.nMAC[5] = mac[5];
		ret = ifx_ethsw_kioctl(handle, IFX_ETHSW_MAC_TABLE_ENTRY_ADD, (unsigned int)&x.MAC_tableAdd);
		ifx_ethsw_kclose(handle);
		break;

		case IFX_ETHSW_MAC_TABLE_ENTRY_REMOVE:
		handle = ifx_ethsw_kopen("/dev/switch/0");
		memset(&x.MAC_tableRemove, 0x00, sizeof(x.MAC_tableRemove));
		x.MAC_tableRemove.nFId = fid;
		x.MAC_tableRemove.nMAC[0] = mac[0];
		x.MAC_tableRemove.nMAC[1] = mac[1];
		x.MAC_tableRemove.nMAC[2] = mac[2];
		x.MAC_tableRemove.nMAC[3] = mac[3];
		x.MAC_tableRemove.nMAC[4] = mac[4];
		x.MAC_tableRemove.nMAC[5] = mac[5];
		printk("[%s] REMOVE: nFID =%d\n", __FUNCTION__, fid);
		ifx_ethsw_kioctl(handle, IFX_ETHSW_MAC_TABLE_ENTRY_REMOVE, (unsigned int)&x.MAC_tableRemove);
		ifx_ethsw_kclose(handle);
		break;

		default:
		break;
	}

	return ret;
}
#endif


static int avm_get_ptm_qid(unsigned int skb_priority){

    unsigned int priority = (skb_priority & TC_H_MIN_MASK);
    return g_ptm_prio_queue_map[priority >= NUM_ENTITY(g_ptm_prio_queue_map) ? NUM_ENTITY(g_ptm_prio_queue_map) - 1 : priority];
}

static int ppe_clk_change(unsigned int arg, unsigned int flags ) {
	unsigned long sys_flag;
	unsigned int ppe_cfg_bit = ( arg == IFX_PPA_PWM_LEVEL_D0 ) ? (1 << 16) : (2 << 16);

	if (flags &= FLAG_PPE_CLK_CHANGE_ARG_AVM_DIRCT){
		switch(arg){
			case 500:
				ppe_cfg_bit = 0;
			break;
			case 432:
				ppe_cfg_bit = (1 << 16);
				break;
			case 288:
			default:
				ppe_cfg_bit = (2 << 16);
				break;
		}
	}

	local_irq_save(sys_flag);
	*VR9_CGU_CLKFSR = (*VR9_CGU_CLKFSR & ~(0x07 << 16)) | ppe_cfg_bit;
	local_irq_restore(sys_flag);

	udelay(1000);

    TX_QOS_CFG_DYNAMIC->time_tick = cgu_get_pp32_clock() / 62500;   //  16 * (cgu_get_pp32_clock() / 1000000)
#ifdef CONFIG_IFX_PPE_E5_OFFCHIP_BONDING
    if ( g_dsl_bonding == 2 )
        TX_QOS_CFG->time_tick = cgu_get_pp32_clock() / 62500;       //  16 * (cgu_get_pp32_clock() / 1000000)
#endif

	return 0;
}

int ppe_pwm_change(unsigned int arg, unsigned int flags)
{
    if ( flags == 1 )   //  clock gating
    {
        arg = arg ? 1 : 0;
        PSAVE_CFG->sleep_en = arg;
        PS_MC_GENCFG3->psave_en = arg;
    }

    return 0;
}


void ifx_ppa_setup_ptm_net_dev(struct net_device* dev) {
	static int setup_dev_nr = 0;
	dbg( "setup_dev_nr = %d", setup_dev_nr);
	g_ptm_net_dev[setup_dev_nr++] = dev;
	dev->watchdog_timeo = ETH_WATCHDOG_TIMEOUT;
}


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void avmnet_swi_vr9_e5_register_xmit(void) {
	struct net_device *dev;
	unsigned int i;
	dbg("setup new xmit function");
	for ( i = 0; i < num_registered_eth_dev; i++ ) {
		struct net_device_ops * ops;
		dev = g_eth_net_dev[i];
		ops = dev->netdev_ops;
		if (avm_ata_dev && (strncmp(dev->name, avm_ata_dev, NETDEV_NAME_LEN) == 0)){
    		ops->ndo_start_xmit = &ifx_ppa_eth_qos_hard_start_xmit_e5;
    		dev->features &= ~(NETIF_F_SG | NETIF_F_IP_CSUM);
		} else {
    		ops->ndo_start_xmit = &ifx_ppa_eth_hard_start_xmit_e5;
		}
       	dbg("xmit %s = %pF",dev->name, ops->ndo_start_xmit );
		ops->ndo_tx_timeout = &ifx_ppa_eth_tx_timeout_e5;
		ops->ndo_do_ioctl = &ifx_ppa_eth_ioctl_e5;
	}
}

/*------------------------------------------------------------------------------------------*\
 * Modul Init: Init/Cleanup API
 \*------------------------------------------------------------------------------------------*/
static char ppe_e5_init_buff[512];

static int ppe_e5_init(void) {
	int ret, i;
	struct port_cell_info port_cell = { 0 };
	int showtime = 0;
	void *xdata_addr = NULL;
    avmnet_device_t *avm_ptm_dev = NULL;

	if (g_datapath_mod_count){
		printk("E5 ( MII0/1 ) driver already loaded\n");
		return -EBUSY;
	}

	// we might have to wait for offload ioctrls to finish
	rtnl_offload_write_lock();

	g_datapath_mod_count++;

	printk("Loading E5 (MII0/1) driver ...... ");
	g_stop_datapath = 1;
	wmb();

	// (1) block all incoming switch ports
    // tell driver to suspend packet flow on all devices
    avmnet_swi_7port_drop_everything();
    if(avmnet_hw_config_entry->config->suspend){
        avmnet_hw_config_entry->config->suspend(avmnet_hw_config_entry->config, NULL);
    }else{
        printk(KERN_ERR "[%s] Config error, no suspend function found!\n", __func__);
        BUG();
    }


	if (avm_ata_dev)
	    printk("avm_ata_dev: '%s' \n" , avm_ata_dev);

    // stop NAPI, empty skb backlog queues
    for_each_possible_cpu(i){
        struct softnet_data *queue;
        struct sk_buff *skb, *tmp;

        queue = &per_cpu(g_7port_eth_queue, i);

        napi_disable(&queue->backlog);
        queue->backlog.poll = NULL;

        skb_queue_walk_safe(&(queue->input_pkt_queue), skb, tmp){
            __skb_unlink(skb, &(queue->input_pkt_queue));
            dev_kfree_skb_any(skb);
        }
    }

	stop_7port_dma_gracefully();
  	dma_device_unregister(g_dma_device_ppe);
	dma_device_release(g_dma_device_ppe);
	g_dma_device_ppe = NULL;

#if IFX_PPA_DP_DBG_PARAM_ENABLE
	if (ifx_ppa_drv_dp_dbg_param_enable == 1) {
		ethwan = ifx_ppa_drv_dp_dbg_param_ethwan;
		wanitf = ifx_ppa_drv_dp_dbg_param_wanitf;
		ipv6_acc_en = ifx_ppa_drv_dp_dbg_param_ipv6_acc_en;
		wanqos_en = ifx_ppa_drv_dp_dbg_param_wanqos_en;
	}
#endif  //IFX_PPA_DP_DBG_PARAM_ENABLE

	reset_ppe();

	init_local_variables();

	init_hw();
#ifdef CONFIG_IFX_PPE_E5_OFFCHIP_BONDING
    if ( g_dsl_bonding == 2 )   //  off chip bonding
        init_bonding_hw();
#endif

	if (g_eth_wan_mode == 0 /* DSL WAN */){
		init_ptm_tables();
#ifdef CONFIG_IFX_PPE_E5_OFFCHIP_BONDING
        if ( g_dsl_bonding == 2 )
        {
            g_ppe_base_addr = IFX_PPE_ORG(1);
            PS_MC_GENCFG3->time_tick = cgu_get_pp32_clock() / 250000;
            PS_MC_GENCFG3->psave_en = 1;
            init_ptm_tables();
            g_ppe_base_addr = IFX_PPE_ORG(0);
        }
#endif
        if ( g_dsl_bonding )
            init_ptm_bonding_tables();
	}

	init_communication_data_structures(g_fwcode);

	ret = alloc_dma();
	if (ret)
		goto ALLOC_DMA_FAIL;

#ifdef CONFIG_IFX_PPE_E5_OFFCHIP_BONDING
    if ( g_dsl_bonding == 2 )   //  off chip bonding
        alloc_bonding_dma();
#endif

	dbg(KERN_ERR "[%s] line %d\n", __FUNCTION__, __LINE__);

	ret = request_irq(PPE_MAILBOX_IGU0_INT, mailbox0_irq_handler, IRQF_DISABLED,
			"e5_mailbox0_isr", NULL);
	if (ret) {
		if (ret == -EBUSY)
			err( "IRQ may be occupied by other PPE driver, please reconfig to disable it.\n");
		goto REQUEST_IGU0_IRQ_FAIL;
	}
	ret = request_irq(PPE_MAILBOX_IGU1_INT, mailbox1_irq_handler, IRQF_DISABLED,
			"e5_mailbox_isr", NULL);
	if (ret) {
		if (ret == -EBUSY)
			err(
					"IRQ may be occupied by other PPE driver, please reconfig to disable it.\n");
		goto REQUEST_IRQ_FAIL;
	}

	//don't clear pushthtrough data in Datapath-Init in AVM Setup -> we use common pushthrough_data for all datapath modules
	// memset(ppa_pushthrough_data, 0, sizeof(ppa_pushthrough_data));

	dump_init();

	if (g_eth_wan_mode == 0)
	{
		ret = pp32_start(0, 1);
		if (ret)
			goto PP32_START_E1_FAIL;
	}

	ret = pp32_start(1, 0);
	if (ret)
		goto PP32_START_D5_FAIL;

#ifdef CONFIG_IFX_PPE_E5_OFFCHIP_BONDING
    if ( g_eth_wan_mode == 0 && g_dsl_bonding == 2 )
    {
        g_mailbox_signal_mask &= ~WAN_RX_SIG(0);
        g_dma_device->tx_chan[1]->open(g_dma_device->tx_chan[1]);
    }
#endif

	*MBOX_IGU0_IER = g_eth_wan_mode ? DMA_TX_CH0_SIG | DMA_TX_CH1_SIG : DMA_TX_CH0_SIG;
	*MBOX_IGU1_IER = g_mailbox_signal_mask;

#ifdef CONFIG_IFX_PPE_E5_OFFCHIP_BONDING
    if ( g_dsl_bonding == 2 )
    {
        g_ppe_base_addr = IFX_PPE_ORG(1);
        pp32_start(0, 1);
        pp32_start(1, 0);
        *MBOX_IGU1_IER = 0;
        g_ppe_base_addr = IFX_PPE_ORG(0);
    }
#endif
	start_cpu_port();

	//  turn on DMA TX channels (0, 1) for fast path
	g_dma_device_ppe->tx_chan[0]->open(g_dma_device_ppe->tx_chan[0]);  // ppa -> switch
	g_dma_device_ppe->rx_chan[2]->open(g_dma_device_ppe->rx_chan[2]);

	// g_dma_device_ppe->tx_chan[1]->open(g_dma_device_ppe->tx_chan[1]);  // dsl -> switch(port6)

	// shut the fuck up :-)
	// g_dma_device_ppe->tx_chan[1]->close(g_dma_device_ppe->tx_chan[1]);
	// g_dma_device_ppe->rx_chan[1]->close(g_dma_device_ppe->rx_chan[1]);

	if (g_eth_wan_mode == 0 /* DSL WAN */) {

		avm_wan_mode = AVM_WAN_MODE_PTM;
        avmnet_swi_7port_set_ethwan_dev_by_name( NULL );

		if (!IS_ERR(&ifx_mei_atm_showtime_check) && &ifx_mei_atm_showtime_check)
			ifx_mei_atm_showtime_check(&showtime, &port_cell, &xdata_addr);
		else
			printk("Weak-Symbol nicht gefunden: ifx_mei_atm_showtime_check\n");
		if (showtime)
			dsl_showtime_enter(&port_cell, xdata_addr);

#if defined(ENABLE_LED_FRAMEWORK) && ENABLE_LED_FRAMEWORK
		ifx_led_trigger_register("dsl_data", &g_data_led_trigger);
#endif
	}

	if ( g_eth_wan_mode == 3 /* ETH WAN */ ) {
    	avm_wan_mode = AVM_WAN_MODE_ETH;
    }

	register_netdev_event_handler();

	/*  create proc file    */
	proc_file_create();

	if (g_eth_wan_mode == 0 /* DSL WAN */) {
		if (!IS_ERR(&ifx_mei_atm_showtime_enter) && &ifx_mei_atm_showtime_enter)
			ifx_mei_atm_showtime_enter = dsl_showtime_enter;
		else
			err("Weak-Symbol nicht gefunden: ifx_mei_atm_showtime_enter\n");

		if (!IS_ERR(&ifx_mei_atm_showtime_exit) && &ifx_mei_atm_showtime_exit)
			ifx_mei_atm_showtime_exit = dsl_showtime_exit;
		else
			err("Weak-Symbol nicht gefunden: ifx_mei_atm_showtime_exit\n");

	}
	ifx_ppa_drv_avm_get_atm_qid_hook = avm_get_atm_qid;
	ifx_ppa_drv_avm_get_ptm_qid_hook = avm_get_ptm_qid;
	ifx_ppa_drv_avm_get_eth_qid_hook = avm_get_eth_qid;

	ifx_ppa_drv_ppe_clk_change_hook = ppe_clk_change;
    ifx_ppa_drv_ppe_pwm_change_hook = ppe_pwm_change;

	in_avm_ata_mode_hook = e5_in_avm_ata_mode;

	ppa_pushthrough_hook = ppe_directpath_send;

	ifx_ppa_drv_datapath_generic_hook = ppa_datapath_generic_hook;
#ifdef AVM_DOESNT_USE_IFX_ETHSW_API
	ifx_ppa_drv_datapath_mac_entry_setting = mac_entry_setting;
#else
	ifx_ppa_drv_datapath_mac_entry_setting = NULL;
#endif
	//Fix warning message ---end

	/* ptm device funktionen setzen und das device vom avmnet-Treiber alloziieren lassen */
	avm_ptm_dev = get_avmdev_by_name("ptm_vr9");
	if (avm_ptm_dev) {
		dbg("got a fancy ptm_device: do function setup\n");
		avm_ptm_dev->device_ops.ndo_init = ifx_ppa_ptm_init;
		avm_ptm_dev->device_ops.ndo_open = ifx_ppa_ptm_open;
		avm_ptm_dev->device_ops.ndo_stop = ifx_ppa_ptm_stop;
		avm_ptm_dev->device_ops.ndo_start_xmit = g_dsl_bonding ? ifx_ppa_ptm_hard_start_xmit_b : ifx_ppa_ptm_hard_start_xmit;
		avm_ptm_dev->device_ops.ndo_do_ioctl = ifx_ppa_ptm_ioctl;
		avm_ptm_dev->device_ops.ndo_tx_timeout = ifx_ppa_ptm_tx_timeout;

		avm_ptm_dev->device_setup = ifx_ppa_setup_ptm_net_dev;
		avm_ptm_dev->flags &= ~AVMNET_DEVICE_FLAG_WAIT_FOR_MODULE_FUNCTIONS;
		avmnet_create_netdevices();
	}

	// replace common NAPI poll function with our one
    for_each_possible_cpu(i) {
        struct softnet_data *queue;

        queue = &per_cpu(g_7port_eth_queue, i);

        skb_queue_head_init(&queue->input_pkt_queue);
        queue->completion_queue = NULL;
        INIT_LIST_HEAD(&queue->poll_list);

        queue->backlog.poll = process_backlog_e5;
        napi_enable(&queue->backlog);
    }

#if 0 //DMA lauft immer
	/* check if some interfaces have been opened by common driver before */
	for (i = 0; i < NUM_ETH_INF; i++)
	if (g_eth_net_dev[i] && netif_running(g_eth_net_dev[i])) {
		printk(KERN_ERR "[%s] dev=%d is already up and running -> reenable dma\n", __FUNCTION__, i);
		turn_on_dma_rx(1);
		break;
	}
#else
	turn_on_dma_rx(1);
#endif

	/* (re-)aktivieren der DMA-Ports */
	show_fdma_sdma();
	enable_fdma_sdma();
	show_fdma_sdma();

	/* (re-)aktivieren der MACs */
    // TKL: wird jetzt am Schluss durch resume() gemacht
	//avmnet_swi_7port_reinit_macs(avmnet_hw_config_entry->config);

	/* neue (e5 spezifische) xmit-Funktionen registrieren */
	avmnet_swi_vr9_e5_register_xmit();

	/* disable learning, flush mac table*/
	avmnet_swi_7port_disable_learning(avmnet_hw_config_entry->config);

	if (( g_eth_wan_mode == 3 ) && avm_ata_dev) /* ETH WAN */
        avmnet_swi_7port_set_ethwan_dev_by_name( avm_ata_dev );

    // tell driver to resume packet flow on all devices
    if(avmnet_hw_config_entry->config->resume){
        avmnet_hw_config_entry->config->resume(avmnet_hw_config_entry->config, NULL);
    }else{
        printk(KERN_ERR "[%s] Config error, no resume function found!\n", __func__);
        BUG();
    }

    /* disable drop xmit packets*/
	wmb();
	g_stop_datapath = 0;

	printk("[%s] Succeeded!\n", __func__);

	dbg(KERN_ERR "[%s] showtime_exit = %pF, showtime_enter =%pF\n",
			__FUNCTION__, dsl_showtime_exit, dsl_showtime_enter);
	print_driver_ver(ppe_e5_init_buff, sizeof(ppe_e5_init_buff), "PPE datapath driver info", VER_FAMILY,
			VER_DRTYPE, VER_INTERFACE, VER_ACCMODE, VER_MAJOR, VER_MID,
			VER_MINOR);
	printk(ppe_e5_init_buff);
	print_fw_ver(ppe_e5_init_buff, sizeof(ppe_e5_init_buff));
	printk(ppe_e5_init_buff);

#if defined( AVM_NET_TRACE_DUMP ) && AVM_NET_TRACE_DUMP && defined(CONFIG_AVM_NET_TRACE)
	register_avm_net_trace_device ( &ifx_ant_e5 );
	printk(KERN_ERR "[%s] minor:%d", ifx_ant_e5.minor);
#endif

	// dump_dplus("e5 initialized");

    g_dsl_ig_fast_brg_bytes     = ITF_MIB_TBL(7)->ig_fast_brg_bytes;
    g_dsl_ig_fast_rt_ipv4_bytes = ITF_MIB_TBL(7)->ig_fast_rt_ipv4_bytes;
    g_dsl_ig_fast_rt_ipv6_bytes = ITF_MIB_TBL(7)->ig_fast_rt_ipv6_bytes;
    g_dsl_ig_cpu_bytes          = ITF_MIB_TBL(7)->ig_cpu_bytes;
    g_dsl_eg_fast_pkts          = ITF_MIB_TBL(7)->eg_fast_pkts;

	rtnl_offload_write_unlock();
	return 0;

PP32_START_D5_FAIL:
	printk(KERN_ERR "[%s] start d5 failed\n", __FUNCTION__);

	if (g_eth_wan_mode == 0 /* DSL WAN */)
		pp32_stop(0);
	PP32_START_E1_FAIL: printk(KERN_ERR "[%s] start e1 failed\n", __FUNCTION__);
	free_irq(PPE_MAILBOX_IGU1_INT, NULL);
	REQUEST_IRQ_FAIL: printk(KERN_ERR "[%s] irq req failed\n", __FUNCTION__);
	free_irq(PPE_MAILBOX_IGU0_INT, NULL);
	REQUEST_IGU0_IRQ_FAIL: printk(KERN_ERR "[%s] igu0 irq fail\n",
			__FUNCTION__);
	// ifx_eth_fw_unregister_netdev(g_ptm_net_dev[0], 1); //TODO uregister_netdev realisieren....
	// ifx_eth_fw_free_netdev(g_ptm_net_dev[0], 1); //TODO free_netdev realisieren....
	g_ptm_net_dev[0] = NULL;

	stop_7port_dma_gracefully();

	free_dma();

	ALLOC_DMA_FAIL: printk(KERN_ERR "[%s] dma alloc failed\n", __FUNCTION__);
	clear_local_variables();

	g_datapath_mod_count--;
   	avm_wan_mode = AVM_WAN_MODE_NOT_CONFIGURED;

	rtnl_offload_write_unlock();
	return ret;
}

static void __exit ppe_e5_exit(void) {
	int i;
    avmnet_device_t *avm_ptm_dev = NULL;

	// we might have to wait for offload ioctrls to finish
	rtnl_offload_write_lock();
    g_stop_datapath = 1;
    wmb();

    avmnet_swi_7port_drop_everything();
    // (1) block all incoming switch ports
    // tell driver to suspend packet flow on all devices
    if(avmnet_hw_config_entry->config->suspend){
        avmnet_hw_config_entry->config->suspend(avmnet_hw_config_entry->config, NULL);
    }else{
        printk(KERN_ERR "[%s] Config error, no suspend function found!\n", __func__);
        BUG();
    }

	// stop NAPI, empty skb backlog queues
    for_each_possible_cpu(i){
        struct softnet_data *queue;
        struct sk_buff *skb, *tmp;

        queue = &per_cpu(g_7port_eth_queue, i);

        napi_disable(&queue->backlog);
        queue->backlog.poll = NULL;

        skb_queue_walk_safe(&(queue->input_pkt_queue), skb, tmp){
            __skb_unlink(skb, &(queue->input_pkt_queue));
            dev_kfree_skb_any(skb);
        }
    }

	if (g_eth_wan_mode == 0 /* DSL WAN */) {
		if (!IS_ERR(&ifx_mei_atm_showtime_enter) && &ifx_mei_atm_showtime_enter)
			ifx_mei_atm_showtime_enter = NULL;
		else
			printk("Weak-Symbol nicht gefunden: ifx_mei_atm_showtime_enter\n");

		if (!IS_ERR(&ifx_mei_atm_showtime_exit) && &ifx_mei_atm_showtime_exit)
			ifx_mei_atm_showtime_exit = NULL;
		else
			printk("Weak-Symbol nicht gefunden: ifx_mei_atm_showtime_exit\n");
	}

	proc_file_delete();

	unregister_netdev_event_handler();

	in_avm_ata_mode_hook = NULL;

	ppa_pushthrough_hook = NULL;

	ifx_ppa_drv_avm_get_atm_qid_hook = NULL;
	ifx_ppa_drv_avm_get_ptm_qid_hook = NULL;
	ifx_ppa_drv_avm_get_eth_qid_hook = NULL;

	ifx_ppa_drv_ppe_clk_change_hook = NULL;
    ifx_ppa_drv_ppe_pwm_change_hook = NULL;

#ifdef CONFIG_IFX_PPA_API_DIRECTPATH
    ifx_ppa_drv_g_ppe_directpath_data = NULL;
    ifx_ppa_drv_directpath_send_hook = NULL;
    ifx_ppa_drv_directpath_rx_start_hook = NULL;
    ifx_ppa_drv_directpath_rx_stop_hook = NULL;
#endif //CONFIG_IFX_PPA_API_DIRECTPATH
    ifx_ppa_drv_datapath_generic_hook = NULL;

#if defined(ENABLE_LED_FRAMEWORK) && ENABLE_LED_FRAMEWORK
	if ( g_eth_wan_mode == 0 /* DSL WAN */)
	{
		ifx_led_trigger_deregister(g_data_led_trigger);
		g_data_led_trigger = NULL;
	}
#endif

	// stop_datapath();
#if 1

	stop_7port_dma_gracefully();
#else
	dbg(  "[%s] unregister g_dma_device_ppe\n", __FUNCTION__);
	dma_device_unregister(g_dma_device_ppe);
	dbg(  "[%s] unregister g_dma_device_ppe\n", __FUNCTION__);
	dma_device_release(g_dma_device_ppe);
#endif

	if (g_eth_wan_mode == 0 /* DSL WAN */)
		pp32_stop(0);
	pp32_stop(1);

	free_irq(PPE_MAILBOX_IGU0_INT, NULL);

	free_irq(PPE_MAILBOX_IGU1_INT, NULL);

	avm_ptm_dev = get_avmdev_by_name("ptm_vr9");
	if (avm_ptm_dev) {
	    avmnet_destroy_netdevice(avm_ptm_dev);
		g_ptm_net_dev[0] = NULL;
	}

#if 0
	for (i = 0; i < NUM_ENTITY(g_eth_net_dev); i++) {
		unregister_netdev(g_eth_net_dev[i]);
		free_netdev(g_eth_net_dev[i]);
		g_eth_net_dev[i] = NULL;
	}
#endif

	free_dma();

	clear_share_buffer();

	clear_local_variables();

	reinit_7port_common_eth();

    // tell driver to resume packet flow on all devices
    if(avmnet_hw_config_entry->config->resume){
        avmnet_hw_config_entry->config->resume(avmnet_hw_config_entry->config, NULL);
    }else{
        printk(KERN_ERR "[%s] Config error, no resume function found!\n", __func__);
        BUG();
    }

	wmb();
    g_stop_datapath = 0;

	g_datapath_mod_count--;
   	avm_wan_mode = AVM_WAN_MODE_NOT_CONFIGURED;

	rtnl_offload_write_unlock();

}


#if defined(CONFIG_IFX_PPA_DATAPATH)

static int __init wan_mode_setup(char *line)
{
	if ( strcmp(line, "1") == 0 )
	ethwan = 1;
	else if ( strcmp(line, "2") == 0 )
	ethwan = 2;

	dbg(KERN_ERR "[%s] ethwan=%#x\n", __FUNCTION__, ethwan);
	dbg(KERN_ERR "[%s] wanitf=%#x\n", __FUNCTION__, wanitf);
	return 0;
}

static int __init wanitf_setup(char *line)
{
	wanitf = simple_strtoul(line, NULL, 0);

	if ( wanitf > 0xFF )
	wanitf = ~0;
	dbg("[%s] wanitf=%#x\n", __FUNCTION__, wanitf);

	return 0;
}
static int __init ipv6_acc_en_setup(char *line)
{
	if ( strcmp(line, "0") == 0 )
	ipv6_acc_en = 0;
	else
	ipv6_acc_en = 1;

	return 0;
}

static int __init wanqos_en_setup(char *line)
{
	wanqos_en = simple_strtoul(line, NULL, 0);

	if ( wanqos_en < 0 || wanqos_en > 8 )
		wanqos_en = 0;

	return 0;
}

static int __init dsl_bonding_setup(char *line) {
    int tmp_wanqos_en = simple_strtoul(line, NULL, 0);

    if ( isdigit(*line) && dsl_bonding >= 0 && dsl_bonding <= 2 )
        dsl_bonding = tmp_dsl_bonding;

    return 0;
}

#ifdef CONFIG_IFX_PPE_E5_OFFCHIP_BONDING

static int __init pci_ppe_addr_setup(char *line)
{
    pci_ppe_addr = simple_strtoul(line, NULL, 0);

    return 0;
}

static int __init bm_addr_setup(char *line)
{
    bm_addr = simple_strtoul(line, NULL, 0);

    return 0;
}

static int __init pci_ddr_addr_setup(char *line)
{
    pci_ppe_addr = simple_strtoul(line, NULL, 0);

    return 0;
}

static int __init pci_bm_ppe_addr_setup(char *line)
{
    pci_bm_ppe_addr = simple_strtoul(line, NULL, 0);

    return 0;
}

#endif

static int __init queue_gamma_map_setup(char *line)
{
	char *p;
	int i;

	for ( i = 0, p = line; i < NUM_ENTITY(queue_gamma_map) && isxdigit(*p); i++ )
	{
		queue_gamma_map[i] = simple_strtoul(p, &p, 0);
        if ( *p == ',' || *p == ';' || *p == ':' )
            p++;
    }

    return 0;
}

static int __init qos_queue_len_setup(char *line)
{
    char *p;
    int i;

    for ( i = 0, p = line; i < NUM_ENTITY(qos_queue_len) && isxdigit(*p); i++ )
    {
        qos_queue_len[i] = simple_strtoul(p, &p, 0);
        if ( *p == ',' || *p == ';' || *p == ':' )
            p++;
    }

    return 0;
}


#endif

module_init(ppe_e5_init);
module_exit(ppe_e5_exit);
#ifdef CONFIG_IFX_PPA_DATAPATH
__setup("ethaddr=", eth0addr_setup);
__setup("eth1addr=", eth1addr_setup);
__setup("ethwan=", wan_mode_setup);
__setup("wanitf=", wanitf_setup);
__setup("ipv6_acc_en=", ipv6_acc_en_setup);
__setup("wanqos_en=", wanqos_en_setup);
__setup("dsl_bonding=", dsl_bonding_setup);
#ifdef CONFIG_IFX_PPE_E5_OFFCHIP_BONDING
__setup("pci_ppe_addr=", pci_ppe_addr_setup);
__setup("bm_addr=", bm_addr_setup);
__setup("pci_ddr_addr=", pci_ddr_addr_setup);
__setup("pci_bm_ppe_addr=", pci_bm_ppe_addr_setup);
#endif
__setup("queue_gamma_map=", queue_gamma_map_setup);
  __setup("qos_queue_len=", qos_queue_len_setup);
#endif

MODULE_LICENSE("GPL");

