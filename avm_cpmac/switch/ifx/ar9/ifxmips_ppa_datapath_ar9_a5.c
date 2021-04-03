/******************************************************************************
**
** FILE NAME    : ifxmips_ppa_datapath_ar9_a5.c
** PROJECT      : UEIP
** MODULES      : ATM + MII0/1 Acceleration Package (AR9 PPA A5)
**
** DATE         : 23 MAR 2008
** AUTHOR       : Xu Liang
** DESCRIPTION  : ATM + MII0/1 Driver with Acceleration Firmware (A5)
** COPYRIGHT    :   Copyright (c) 2006
**          Infineon Technologies AG
**          Am Campeon 1-12, 85579 Neubiberg, Germany
**
**    This program is free software; you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation; either version 2 of the License, or
**    (at your option) any later version.
**
** HISTORY
** $Date        $Author         $Comment
** 23 MAR 2008  Xu Liang        Initiate Version
** 10 DEC 2012  Manamohan Shetty       Added the support for RTP and MIB mode Features 
*******************************************************************************/



/*
 * ####################################
 *              Version No.
 * ####################################
 */
#define VER_FAMILY      0x20    //  bit 0: res
                                //      1: Danube
                                //      2: Twinpass
                                //      3: Amazon-SE
                                //      4: res
                                //      5: AR9
                                //      6: VR9
                                //      7: AR10
#define VER_DRTYPE      0x03    //  bit 0: Normal Data Path driver
                                //      1: Indirect-Fast Path driver
                                //      2: HAL driver
                                //      3: Hook driver
                                //      4: Stack/System Adaption Layer driver
                                //      5: PPA API driver
#define VER_INTERFACE   0x07    //      bit 0: MII 0
                                //      1: MII 1
                                //      2: ATM WAN
                                //      3: PTM WAN
#define VER_ACCMODE     0x00    //  bit 0: Routing
                                //      1: Bridging
#define VER_MAJOR       0
#define VER_MID         1
#define VER_MINOR       1

#define DMA_CHAN_NR_DIRECTPATH_CPU_TO_PPE  3
#define DMA_CHAN_NR_DEFAULT_CPU_TX         2
#define ATM_DATAPATH 1


/*
 * ####################################
 *              Head File
 * ####################################
 */

/*
 *  Common Head File
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/ctype.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/miscdevice.h>
#include <linux/atm.h>
#include <linux/atmdev.h>
#include <linux/init.h>
#include <linux/etherdevice.h>  /*  eth_type_trans  */
#include <linux/ethtool.h>      /*  ethtool_cmd     */
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <asm/uaccess.h>
#include <asm/unistd.h>
#include <asm/irq.h>
#include <asm/delay.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <net/xfrm.h>
#if __has_include(<avm/pa/ifx_multiplexer.h>)
#include <avm/pa/ifx_multiplexer.h>
#else
#include <linux/avm_pa_ifx_multiplexer.h>
#endif
#include <avm/net/net.h>
#include <net/pkt_sched.h>


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
#include <avm/net/ifx_ppa_api.h>
#include <avm/net/ifx_ppa_stack_al.h>
#include <avm/net/ifx_ppa_api_directpath.h>
#include <avm/net/ifx_ppa_ppe_hal.h>

#include "ifxmips_ppa_datapath_fw_ar9_a5.h"

#include <avmnet_config.h>
#include "avmnet_debug.h"

#include "../common/swi_ifx_common.h"
#include "../common/ifx_ppa_datapath.h"
#include "swi_ar9.h"
#include "swi_ar9_reg.h"

#include "ifxmips_ppa_hal_ar9_a5.h"


/*
 * ####################################
 *   Parameters to Configure PPE
 * ####################################
 */
static int port_cell_rate_up[2] = {3200, 3200}; /*  Maximum TX cell rate for ports                  */

static int qsb_tau   = 1;                       /*  QSB cell delay variation due to concurrency     */
static int qsb_srvm  = 0x0F;                    /*  QSB scheduler burst length                      */
static int qsb_tstep = 4 ;                      /*  QSB time step, all legal values are 1, 2, 4     */

static int write_descriptor_delay  = 0x20;      /*  Write descriptor delay                          */

static int aal5r_max_packet_size   = 0x0630;    /*  Max frame size for RX                           */
static int aal5r_min_packet_size   = 0x0000;    /*  Min frame size for RX                           */
static int aal5s_max_packet_size   = 0x0630;    /*  Max frame size for TX                           */

static int ethwan = 0;
static int wanitf = ~0;

static int ipv6_acc_en = 1; //TODO UGW 5.1.1: 1 --- vorischt auch g_ipv6_acc_en anpassen
static unsigned int oam_cell_drop_count = 0;

static char *avm_ata_dev = NULL;

static uint32_t lan_port_seperate_enabled=0, wan_port_seperate_enabled=0;

static int wanqos_en = 8;

static int unknown_session_dump_len = DUMP_SKB_LEN;
module_param(unknown_session_dump_len, int, S_IRUSR | S_IWUSR);

#if defined(CONFIG_IFX_PPA_DIRECTPATH_TX_QUEUE_SIZE)
  static int directpath_tx_queue_size = 1524 * 50;
#endif
#if defined(CONFIG_IFX_PPA_DIRECTPATH_TX_QUEUE_PKTS)
  static int directpath_tx_queue_pkts = 50;
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,11)
  #define MODULE_PARM_ARRAY(a, b)   module_param_array(a, int, NULL, 0)
  #define MODULE_PARM(a, b)         module_param(a, int, 0)
#else
  #define MODULE_PARM_ARRAY(a, b)   MODULE_PARM(a, b)
#endif

MODULE_PARM_ARRAY(port_cell_rate_up, "2-2i");
MODULE_PARM_DESC(port_cell_rate_up, "ATM port upstream rate in cells/s");

MODULE_PARM(qsb_tau,"i");
MODULE_PARM_DESC(qsb_tau, "Cell delay variation. Value must be > 0");
MODULE_PARM(qsb_srvm, "i");
MODULE_PARM_DESC(qsb_srvm, "Maximum burst size");
MODULE_PARM(qsb_tstep, "i");
MODULE_PARM_DESC(qsb_tstep, "n*32 cycles per sbs cycles n=1,2,4");

MODULE_PARM(write_descriptor_delay, "i");
MODULE_PARM_DESC(write_descriptor_delay, "PPE core clock cycles between descriptor write and effectiveness in external RAM");

MODULE_PARM(aal5r_max_packet_size, "i");
MODULE_PARM_DESC(aal5r_max_packet_size, "Max packet size in byte for downstream AAL5 frames");
MODULE_PARM(aal5r_min_packet_size, "i");
MODULE_PARM_DESC(aal5r_min_packet_size, "Min packet size in byte for downstream AAL5 frames");
MODULE_PARM(aal5s_max_packet_size, "i");
MODULE_PARM_DESC(aal5s_max_packet_size, "Max packet size in byte for upstream AAL5 frames");

MODULE_PARM(ethwan, "i");
MODULE_PARM_DESC(ethwan, "WAN mode, 2 - ETH WAN, 1 - ETH mixed, 0 - DSL WAN.");

#if !defined(CONFIG_IFX_PPA_AVM_USAGE)
MODULE_PARM(wanitf, "i");
MODULE_PARM_DESC(wanitf, "WAN interfaces, bit 0 - ETH0, 1 - ETH1, 2 - reserved for CPU0, 3/4/5 - DirectPath, 6/7 - DSL");
#endif

module_param(avm_ata_dev, charp, 0);
MODULE_PARM_DESC(avm_ata_dev, "eth0: ethernet_wan with NAT, ath0: WLAN_WAN with NAT");

MODULE_PARM(ipv6_acc_en, "i");
MODULE_PARM_DESC(ipv6_acc_en, "IPv6 support, 1 - enabled, 0 - disabled.");

MODULE_PARM(wanqos_en, "i");
MODULE_PARM_DESC(wanqos_en, "WAN QoS support, 1 - enabled, 0 - disabled.");

#if defined(CONFIG_IFX_PPA_DIRECTPATH_TX_QUEUE_SIZE)
  MODULE_PARM(directpath_tx_queue_size, "i");
  MODULE_PARM_DESC(directpath_tx_queue_size, "directpath TX queue size in total buffer size");
#endif
#if defined(CONFIG_IFX_PPA_DIRECTPATH_TX_QUEUE_PKTS)
  MODULE_PARM(directpath_tx_queue_pkts, "i");
  MODULE_PARM_DESC(directpath_tx_queue_pkts, "directpath TX queue size in total packet number");
#endif



/*
 * ####################################
 *              Board Type
 * ####################################
 */

#define BOARD_AR9_REFERENCE                     0x01

/*
 * ####################################
 *              Definition
 * ####################################
 */

#define BOARD_CONFIG                            BOARD_AR9_REFERENCE

#define DEBUG_SKB_SWAP                          0

#define ENABLE_RX_PROFILE                       0

#define ENABLE_LED_FRAMEWORK                    0

#define ENABLE_CONFIGURABLE_DSL_VLAN            1

#if defined(CONFIG_IFX_PPA_DIRECTPATH_TX_QUEUE_SIZE)
  #define ENABLE_DIRECTPATH_TX_QUEUE            1
  #define ENABLE_DIRECTPATH_TX_QUEUE_SIZE       directpath_tx_queue_size
#elif defined(CONFIG_IFX_PPA_DIRECTPATH_TX_QUEUE_PKTS)
  #define ENABLE_DIRECTPATH_TX_QUEUE            1
  #define ENABLE_DIRECTPATH_TX_QUEUE_PKTS       directpath_tx_queue_pkts
#endif

#define ENABLE_SWITCH_PORT_AUTHORIZED_MODE      1

#define ENABLE_MII0_BYPASS_DELAY_FIX            1

#define ENABLE_MII1_BYPASS_DELAY_FIX            1

//TODO UGW 5.1.1: ENABLE_SWITCH_FLOW_CONTROL 1
#define ENABLE_SWITCH_FLOW_CONTROL              0

#ifdef CONFIG_IFX_PPA_NAPI_ENABLE
#define ENABLE_NAPI 1
#else
#error driver just working with napi support so far
#endif

#define ENABLE_STATS_ON_VCC_BASIS               1

#define ENABLE_DSL_LOOPBACK_TEST                0   //  driver level loopback

#define ENABLE_DFE_LOOPBACK_TEST                0   //  DSL firmware level loopback

#define DISABLE_VBR                             0

#define ENABLE_MY_MEMCPY                        0

#define ENABLE_FW_MODULE_TEST                   0

#if defined(CONFIG_AVMNET_DEBUG) 
#define ENABLE_ASSERT                           1

#define DEBUG_QOS                               1

#define DEBUG_DUMP_SKB                          1

#define DEBUG_DUMP_FLAG_HEADER                  1

#define DEBUG_DUMP_INIT                         0

#define DEBUG_FIRMWARE_TABLES_PROC              1

#define DEBUG_HTU_PROC                          1

#define DEBUG_MEM_PROC                          1

#define DEBUG_PP32_PROC                         1

#define DEBUG_FW_PROC                           1

#define DEBUG_SEND_PROC                         0

#else // defined(CONFIG_AVMNET_DEBUG) 

#define CONFIG_AVMNET_DEBUG                            0

#define ENABLE_ASSERT                           0

#define DEBUG_QOS                               0

#define DEBUG_DUMP_SKB                          0

#define DEBUG_DUMP_FLAG_HEADER                  0

#define DEBUG_DUMP_INIT                         0

#define DEBUG_FIRMWARE_TABLES_PROC              1

#define DEBUG_HTU_PROC                          0

#define DEBUG_MEM_PROC                          0

#define DEBUG_PP32_PROC                         0

#define DEBUG_FW_PROC                           0

#define DEBUG_SEND_PROC                         0
#endif // defined(CONFIG_AVMNET_DEBUG) 

#define PPE_MAILBOX_IGU0_INT                    INT_NUM_IM2_IRL23

#define PPE_MAILBOX_IGU1_INT                    INT_NUM_IM2_IRL24

#define MY_ETH0_ADDR                            g_my_ethaddr

#if defined(CONFIG_DSL_MEI_CPE_DRV) && !defined(CONFIG_IFXMIPS_DSL_CPE_MEI)
  #define CONFIG_IFXMIPS_DSL_CPE_MEI            1
#endif

//  default board related configuration
#ifdef CONFIG_AR9_CUSTOM_BOARD
  #define MII0_MODE_SETUP                       REV_MII_MODE
#else
  #define MII0_MODE_SETUP                       RGMII_MODE
#endif
#if defined(CONFIG_MII1_RGMII_MODE_100MB)
  #define MII1_MODE_SETUP                       RGMII_MODE_100MB
#elif defined(CONFIG_MII1_RGMII_MODE)
  #define MII1_MODE_SETUP                       RGMII_MODE
#elif defined(CONFIG_MII1_AUTONEG)
  #define MII1_MODE_SETUP                       RGMII_MODE_100MB  //  default setting before auto-negotiate
 #ifdef CONFIG_LTQ_VB300
  #define MII1_GPHY_INT_EXT_INT_NO              3
  #define PHY11G_ADDR                           5
 #else
  #define MII1_GPHY_INT_EXT_INT_NO              5
 #endif

  #define GPHY_VSC                              0
  #define GPHY_LANTIQ_11G                       1

  #if MII1_GPHY_INT_EXT_INT_NO == 0
    #define MII1_GPHY_INT                       INT_NUM_IM4_IRL30
  #elif MII1_GPHY_INT_EXT_INT_NO == 1
    #define MII1_GPHY_INT                       INT_NUM_IM3_IRL31
  #elif MII1_GPHY_INT_EXT_INT_NO == 2
    #define MII1_GPHY_INT                       INT_NUM_IM1_IRL26
  #elif MII1_GPHY_INT_EXT_INT_NO == 3
    #define MII1_GPHY_INT                       INT_NUM_IM1_IRL0
  #elif MII1_GPHY_INT_EXT_INT_NO == 4
    #define MII1_GPHY_INT                       INT_NUM_IM1_IRL1
  #elif MII1_GPHY_INT_EXT_INT_NO == 5
    #define MII1_GPHY_INT                       INT_NUM_IM1_IRL2
  #endif
#else
  #warning AVMNET-Mode - IFX says: you must specify MII1 link state in current stage
#endif

//  specific board related configuration
#if defined (BOARD_CONFIG)
  #if BOARD_CONFIG == BOARD_AR9_REFERENCE
  #endif
#endif

#if defined(ENABLE_STATS_ON_VCC_BASIS) && ENABLE_STATS_ON_VCC_BASIS
  #define UPDATE_VCC_STAT(conn, field, num)     do { g_atm_priv_data.connection[conn].field += (num); } while (0)
#else
  #define UPDATE_VCC_STAT(conn, field, num)
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
#define CPU_TO_ETH_WAN_TX_SIG                   (1 << 17)   //  IGU1
#define CPU_TO_DSL_WAN_TX_SIG                   (1 << 16)   //  IGU1
#define CPU_TO_WAN_SWAP_SIG                     (1 << 16)   //  IGU1, ETH WAN QoS Mode only
#define WAN_RX_SIG(i)                           (1 << (i))  //  IGU1, 0: AAL5, 1: OAM

/*
 *  Eth Mode
 */
#define RGMII_MODE                              0
#define MII_MODE                                1
#define REV_MII_MODE                            2
#define RED_MII_MODE_IC                         3   //  Input clock
#define RGMII_MODE_100MB                        4
#define TURBO_REV_MII_MODE                      6   //  Turbo Rev Mii mode
#define RED_MII_MODE_OC                         7   //  Output clock
#define RGMII_MODE_10MB                         8

/*
 *  GPIO Related
 */
#define IFX_PPA_GPIO_OP(op, pin)                op((pin), IFX_GPIO_MODULE_PPA)
#define IFX_SWITCH_PIN_RESERVE(pin)             IFX_PPA_GPIO_OP(ifx_gpio_pin_reserve, pin)
#define IFX_SWITCH_PIN_FREE(pin)                IFX_PPA_GPIO_OP(ifx_gpio_pin_free, pin)
#define IFX_SWITCH_DIR_OUT(pin)                 IFX_PPA_GPIO_OP(ifx_gpio_dir_out_set, pin)
#define IFX_SWITCH_DIR_IN(pin)                  IFX_PPA_GPIO_OP(ifx_gpio_dir_in_set, pin)
#define IFX_SWITCH_OUTPUT_SET(pin)              IFX_PPA_GPIO_OP(ifx_gpio_output_set, pin)
#define IFX_SWITCH_OUTPUT_CLR(pin)              IFX_PPA_GPIO_OP(ifx_gpio_output_clear, pin)
#define IFX_SWITCH_ALTSEL0_SET(pin)             IFX_PPA_GPIO_OP(ifx_gpio_altsel0_set, pin)
#define IFX_SWITCH_ALTSEL0_CLR(pin)             IFX_PPA_GPIO_OP(ifx_gpio_altsel0_clear, pin)
#define IFX_SWITCH_OD_SET(pin)                  IFX_PPA_GPIO_OP(ifx_gpio_open_drain_set, pin)
#define IFX_SWITCH_OD_CLR(pin)                  IFX_PPA_GPIO_OP(ifx_gpio_open_drain_clear, pin)
#define IFX_SWITCH_ALTSEL1_SET(pin)             IFX_PPA_GPIO_OP(ifx_gpio_altsel1_set, pin)
#define IFX_SWITCH_ALTSEL1_CLR(pin)             IFX_PPA_GPIO_OP(ifx_gpio_altsel1_clear, pin)
#define IFX_SWITCH_PUDSEL_SET(pin)              IFX_PPA_GPIO_OP(ifx_gpio_pudsel_set, pin)
#define IFX_SWITCH_PUDEN_SET(pin)               IFX_PPA_GPIO_OP(ifx_gpio_puden_set, pin)
#define SWITCH_MDIO                             42  //  P2.10
#define SWITCH_MDC                              43  //  P2.11
#define MII0_COL                                32  //  P2.0
#define MII0_CRS                                33  //  P2.1
#define MII0_TXERR                              40  //  P2.8
#define MII0_RXERR                              41  //  P2.9
#define MII1_COL                                44  //  P2.12
#define MII1_CRS                                47  //  P2.15
#define MII1_TXERR                              45  //  P2.13
#define MII1_RXERR                              46  //  P2.14
#define CLOCK_OUT2                              3   //  P0.3
#define RESET_GPHY                              32  //  P2.0

/*
 *  Constant Definition
 */
#define ETH_WATCHDOG_TIMEOUT                    (10 * HZ)
#define ETOP_MDIO_DELAY                         1
#define IDLE_CYCLE_NUMBER                       30000

#define DMA_PACKET_SIZE                         1600    //  ((1518 + 62 + 8) + 31) & ~31
#define DMA_RX_ALIGNMENT                        128
#define DMA_TX_ALIGNMENT                        32

#define FWCODE_ROUTING_ACC_D2                   0x02
#define FWCODE_BRIDGING_ACC_D3                  0x03
#define FWCODE_ROUTING_BRIDGING_ACC_D4          0x04
#define FWCODE_ROUTING_BRIDGING_ACC_A4          0x14
#define FWCODE_ROUTING_ACC_D5                   0x05
#define FWCODE_ROUTING_ACC_A5                   0x15

/*
 *  ATM Cell
 */
#define CELL_SIZE                               ATM_AAL0_SDU

/*
 *  Ethernet Frame Definitions
 */
#define ETH_CRC_LENGTH                          4
#define ETH_MAX_DATA_LENGTH                     ETH_DATA_LEN
#define ETH_MIN_TX_PACKET_LENGTH                ETH_ZLEN

/*
 *  ATM Port, QSB Queue, DMA RX/TX Channel Parameters
 */
#define ATM_PORT_NUMBER                         2
#define ATM_QUEUE_NUMBER                        15
#define ATM_SW_TX_QUEUE_NUMBER                  16
#define QSB_QUEUE_NUMBER_BASE                   1

#define DEFAULT_RX_HUNT_BITTH                   4

/*
 *  QSB Queue Scheduling and Shaping Definitions
 */
#define QSB_WFQ_NONUBR_MAX                      0x3f00
#define QSB_WFQ_UBR_BYPASS                      0x3fff
#define QSB_TP_TS_MAX                           65472
#define QSB_TAUS_MAX                            64512
#define QSB_GCR_MIN                             18

/*
 *  QSB Command Set
 */
#define QSB_RAMAC_RW_READ                       0
#define QSB_RAMAC_RW_WRITE                      1

#define QSB_RAMAC_TSEL_QPT                      0x01
#define QSB_RAMAC_TSEL_SCT                      0x02
#define QSB_RAMAC_TSEL_SPT                      0x03
#define QSB_RAMAC_TSEL_VBR                      0x08

#define QSB_RAMAC_LH_LOW                        0
#define QSB_RAMAC_LH_HIGH                       1

#define QSB_QPT_SET_MASK                        0x0
#define QSB_QVPT_SET_MASK                       0x0
#define QSB_SET_SCT_MASK                        0x0
#define QSB_SET_SPT_MASK                        0x0
#define QSB_SET_SPT_SBVALID_MASK                0x7FFFFFFF

#define QSB_SPT_SBV_VALID                       (1 << 31)
#define QSB_SPT_PN_SET(value)                   (((value) & 0x01) ? (1 << 16) : 0)
#define QSB_SPT_INTRATE_SET(value)              SET_BITS(0, 13, 0, value)

/*
 *  OAM Definitions
 */
#define OAM_HTU_ENTRY_NUMBER                    3
#define OAM_F4_SEG_HTU_ENTRY                    0
#define OAM_F4_TOT_HTU_ENTRY                    1
#define OAM_F5_HTU_ENTRY                        2
#define OAM_F4_CELL_ID                          0
#define OAM_F5_CELL_ID                          15

/*
 *  EMA Settings
 */
#define EMA_CMD_BUF_LEN                         0x0010
#define EMA_CMD_BASE_ADDR                       (0x1710 << 2)
#define EMA_DATA_BUF_LEN                        0x0040
#define EMA_DATA_BASE_ADDR                      (0x16d0 << 2)
#define EMA_WRITE_BURST                         0x02
#define EMA_READ_BURST                          0x02

/*
 *  Firmware Settings
 */

#define CPU_TO_WAN_TX_DESC_NUM                  32  //  WAN CPU TX
#define DSL_WAN_TX_DESC_NUM(i)                  32  //  WAN QoS TX, i < 15
#define ETH_WAN_TX_DESC_NUM(i)                  __ETH_WAN_TX_QUEUE_LEN  //  ETH WAN QoS TX, i < __ETH_WAN_TX_QUEUE_NUM
#define CPU_TO_WAN_SWAP_DESC_NUM                32
#define WAN_TX_DESC_NUM_TOTAL                   (DSL_WAN_TX_DESC_NUM(0) * 15)   //  DSL WAN TX desc overlap ETH WAN TX desc + ETH WAN Swap desc
#define WAN_RX_DESC_NUM(i)                      32  //  DSL RX PKT/OAM, i < 2, 0 PKT, 1 OAM
#define DMA_TX_CH1_DESC_NUM                     (g_eth_wan_mode ? (g_eth_wan_mode == 3 /* ETH WAN */ && g_wanqos_en ? 32 : DMA_RX_CH1_DESC_NUM) : WAN_RX_DESC_NUM(0))
#define DMA_RX_CH1_DESC_NUM                     32  //  DSL/MII1 WAN mode fast path
#define DMA_RX_CH2_DESC_NUM                     32  //  Switch/MII0/MII1 LAN mode fast path
#define DMA_TX_CH0_DESC_NUM                     DMA_RX_CH2_DESC_NUM //  DMA_TX_CH0_DESC_NUM is used to replace DMA_TX_CH2_DESC_NUM

#define IFX_PPA_PORT_NUM                        8

/*
 *  Bits Operation
 */
//#define GET_BITS(x, msb, lsb)                   (((x) & ((1 << ((msb) + 1)) - 1)) >> (lsb))
//#define SET_BITS(x, msb, lsb, value)            (((x) & ~(((1 << ((msb) + 1)) - 1) ^ ((1 << (lsb)) - 1))) | (((value) & ((1 << (1 + (msb) - (lsb))) - 1)) << (lsb)))

/*
 *  External Tantos Switch Related
 */
#define MDIO_OP_READ                            (2 << 10)
#define MDIO_OP_WRITE                           (1 << 10)
#define TANTOS_CHIP_ID                          0x2599


/*
 *  DWORD-Length of Memory Blocks
 */
#define PP32_DEBUG_REG_DWLEN                    0x0030
#define PPM_INT_REG_DWLEN                       0x0010
#define PP32_INTERNAL_RES_DWLEN                 0x00C0
#define PPE_CLOCK_CONTROL_DWLEN                 0x0F00
#define CDM_CODE_MEMORY_RAM0_DWLEN              0x1000
#define CDM_CODE_MEMORY_RAM1_DWLEN              0x1000
#define PPE_REG_DWLEN                           0x1000
#define PP32_DATA_MEMORY_RAM1_DWLEN             CDM_CODE_MEMORY_RAM1_DWLEN
#define PPM_INT_UNIT_DWLEN                      0x0100
#define PPM_TIMER0_DWLEN                        0x0100
#define PPM_TASK_IND_REG_DWLEN                  0x0100
#define PPS_BRK_DWLEN                           0x0100
#define PPM_TIMER1_DWLEN                        0x0100
#define SB_RAM0_DWLEN                           0x0800
#define SB_RAM1_DWLEN                           0x0800
#define SB_RAM2_DWLEN                           0x0800
#define SB_RAM3_DWLEN                           0x0800
#define SB_RAM4_DWLEN                           0x0C00
#define QSB_CONF_REG_DWLEN                      0x0100


/*
 *  DMA/EMA Descriptor Base Address
 */
#define CPU_TO_WAN_TX_DESC_BASE                 ((volatile struct tx_descriptor *)SB_BUFFER(0x3C00))                /*         32 each queue    */
#define DSL_WAN_TX_DESC_BASE(i)                 ((volatile struct tx_descriptor *)SB_BUFFER(0x3C40 + (i) * 2 * 32)) /*  i < 15,32 each queue    */
#define __ETH_WAN_TX_QUEUE_NUM                  g_wanqos_en
#define __ETH_VIRTUAL_TX_QUEUE_NUM              8   //  LAN interface, there is no real firmware queue, but only virtual queue maintained by driver, so that skb->priority => firmware QId => switch class can be mapped.
#define SWITCH_CLASS_NUM                        4   //  AR9 switch has 4 classes.
#define __ETH_WAN_TX_QUEUE_LEN                  (448 / __ETH_WAN_TX_QUEUE_NUM < 256 ? (448 / __ETH_WAN_TX_QUEUE_NUM) : 255)
#define __ETH_WAN_TX_DESC_BASE(i)               (0x3C80 + (i) * 2 * __ETH_WAN_TX_QUEUE_LEN)
#define ETH_WAN_TX_DESC_BASE(i)                 ((volatile struct tx_descriptor *)SB_BUFFER(__ETH_WAN_TX_DESC_BASE(i))) /*  i < __ETH_WAN_TX_QUEUE_NUM, __ETH_WAN_TX_QUEUE_LEN each queue    */
#define CPU_TO_WAN_SWAP_DESC_BASE               ((volatile struct tx_descriptor *)SB_BUFFER(0x3C40))                /*         32 each queue    */
#define WAN_RX_DESC_BASE(i)                     ((volatile struct rx_descriptor *)SB_BUFFER(0x3B80 + (i) * 2 * 32)) /*  i < 2, 32 each queue    */
#define DMA_TX_CH1_DESC_BASE                    (g_eth_wan_mode ? (g_eth_wan_mode == 3 /* ETH WAN */ && g_wanqos_en ? ((volatile struct rx_descriptor *)SB_BUFFER(0x3AC0)): DMA_RX_CH1_DESC_BASE) : WAN_RX_DESC_BASE(0))
#define DMA_RX_CH1_DESC_BASE                    ((volatile struct rx_descriptor *)SB_BUFFER(0x3B40))                /*         32 each queue    */
#define DMA_RX_CH2_DESC_BASE                    ((volatile struct rx_descriptor *)SB_BUFFER(0x3B00))                /*         32 each queue    */
#define DMA_TX_CH0_DESC_BASE                    DMA_RX_CH2_DESC_BASE

#if defined(DEBUG_FW_PROC) && DEBUG_FW_PROC
/*
 *  Firmware Proc
 */
//  need check with hejun
  #define TIR(i)                                ((volatile u32 *)(0xBE198800) + (i))

  #define __CPU_TX_SWAPPER_DROP_PKTS            SB_BUFFER(0x32C0)
  #define __CPU_TX_SWAPPER_DROP_BYTES           SB_BUFFER(0x32C1)
  #define __DSLWAN_FP_SWAPPER_DROP_PKTS         SB_BUFFER(0x32C2)
  #define __DSLWAN_FP_SWAPPER_DROP_BYTES        SB_BUFFER(0x32C3)

  #define __CPU_TXDES_SWAP_RDPTR                SB_BUFFER(0x32C4)
  #define __DSLWAN_FP_RXDES_SWAP_RDPTR          SB_BUFFER(0x32C5)
  #define __DSLWAN_FP_RXDES_DPLUS_WRPTR         SB_BUFFER(0x32C6)

  #define __DSLWAN_TX_PKT_CNT(i)                SB_BUFFER(0x32D0+(i))   //  i < 8

  #define __DSLWAN_TXDES_SWAP_PTR(i)            SB_BUFFER(0x32E0+(i))   //  i < 8

#if 0
  // SB_BUFFER(0x29C0) - SB_BUFFER(0x29C7)
  #define __VLAN_PRI_TO_QID_MAPPING             SB_BUFFER(0x32E0)
#endif

  //
  // etx MIB: SB_BUFFER(0x32F0) - SB_BUFFER(0x32FF)
  #define __DSLWAN_TX_THRES_DROP_PKT_CNT(i)     SB_BUFFER(0x2D80+(i))   //  i < 8

  #define __CPU_TO_DSLWAN_TX_PKTS               SB_BUFFER(0x32C8)
  #define __CPU_TO_DSLWAN_TX_BYTES              SB_BUFFER(0x32C9)

  #define ACC_ERX_PID                           SB_BUFFER(0x2B00)
  #define ACC_ERX_PORT_TIMES                    SB_BUFFER(0x2B01)
  #define SLL_ISSUED                            SB_BUFFER(0x2B02)
  #define BMC_ISSUED                            SB_BUFFER(0x2B03)
  #define DPLUS_RX_ON                           SB_BUFFER(0x3283)
  #define ISR_IS                                SB_BUFFER(0x3284)

  #define __ENETS_PGNUM                         20
  #define __ERX_CBUF_BASE                       SB_BUFFER(0x343C)
  #define __ERX_DBUF_BASE                       SB_BUFFER(0x3450)

  #define PRESEARCH_RDPTR                       SB_BUFFER(0x2B06)

  #define SLL_ERX_PID                           SB_BUFFER(0x2B04)
  #define SLL_PKT_CNT                           SB_BUFFER(0x2B08)
  #define SLL_RDPTR                             SB_BUFFER(0x2B0A)

  #define EDIT_PKT_CNT                          SB_BUFFER(0x2B0C)
  #define EDIT_RDPTR                            SB_BUFFER(0x2B0E)

  #define DPLUSRX_PKT_CNT                       SB_BUFFER(0x2B10)
  #define DPLUS_RDPTR                           SB_BUFFER(0x2B12)

  #define SLL_STATE_NULL                        0
  #define SLL_STATE_DA                          1
  #define SLL_STATE_SA                          2
  #define SLL_STATE_DA_BC                       3
  #define SLL_STATE_ROUTER                      4

  #define PRE_DPLUS_PTR                         SB_BUFFER(0x3281)
  #define DPLUS_PTR                             SB_BUFFER(0x3282)
  #define DPLUS_CNT                             SB_BUFFER(0x3280)
#endif

/*
 *  Share Buffer Registers
 */
#define SB_MST_PRI0                             PPE_REG_ADDR(0x0300)
#define SB_MST_PRI1                             PPE_REG_ADDR(0x0301)
//#define SB_MST_SEL                              PPE_REG_ADDR(0x0304)

/*
 *    ETOP MDIO Registers
 */
//#define ETOP_MDIO_CFG                           PPE_REG_ADDR(0x0600)
//#define ETOP_MDIO_ACC                           PPE_REG_ADDR(0x0601)
//#define ETOP_CFG                                PPE_REG_ADDR(0x0602)
//#define ETOP_IG_VLAN_COS                        PPE_REG_ADDR(0x0603)
//#define ETOP_IG_DSCP_COSx(x)                    PPE_REG_ADDR(0x0607 - ((x) & 0x03))
//#define ETOP_IG_PLEN_CTRL0                      PPE_REG_ADDR(0x0608)
//#define ETOP_ISR                                PPE_REG_ADDR(0x060A)
//#define ETOP_IER                                PPE_REG_ADDR(0x060B)
//#define ETOP_VPID                               PPE_REG_ADDR(0x060C)
//#define ENET_MAC_CFG(i)                         PPE_REG_ADDR(0x0610 + ((i) ? 0x40 : 0x00))
//#define ENETS_DBA(i)                            PPE_REG_ADDR(0x0612 + ((i) ? 0x40 : 0x00))
//#define ENETS_CBA(i)                            PPE_REG_ADDR(0x0613 + ((i) ? 0x40 : 0x00))
//#define ENETS_CFG(i)                            PPE_REG_ADDR(0x0614 + ((i) ? 0x40 : 0x00))
//#define ENETS_PGCNT(i)                          PPE_REG_ADDR(0x0615 + ((i) ? 0x40 : 0x00))
//#define ENETS_PKTCNT(i)                         PPE_REG_ADDR(0x0616 + ((i) ? 0x40 : 0x00))
//#define ENETS_BUF_CTRL(i)                       PPE_REG_ADDR(0x0617 + ((i) ? 0x40 : 0x00))
//#define ENETS_COS_CFG(i)                        PPE_REG_ADDR(0x0618 + ((i) ? 0x40 : 0x00))
//#define ENETS_IGDROP(i)                         PPE_REG_ADDR(0x0619 + ((i) ? 0x40 : 0x00))
//#define ENETS_IGERR(i)                          PPE_REG_ADDR(0x061A + ((i) ? 0x40 : 0x00))
//#define ENETS_MAC_DA0(i)                        PPE_REG_ADDR(0x061B + ((i) ? 0x40 : 0x00))
//#define ENETS_MAC_DA1(i)                        PPE_REG_ADDR(0x061C + ((i) ? 0x40 : 0x00))
//#define ENETF_DBA(i)                            PPE_REG_ADDR(0x0630 + ((i) ? 0x40 : 0x00))
//#define ENETF_CBA(i)                            PPE_REG_ADDR(0x0631 + ((i) ? 0x40 : 0x00))
//#define ENETF_CFG(i)                            PPE_REG_ADDR(0x0632 + ((i) ? 0x40 : 0x00))
//#define ENETF_PGCNT(i)                          PPE_REG_ADDR(0x0633 + ((i) ? 0x40 : 0x00))
//#define ENETF_PKTCNT(i)                         PPE_REG_ADDR(0x0634 + ((i) ? 0x40 : 0x00))
//#define ENETF_HFCTRL(i)                         PPE_REG_ADDR(0x0635 + ((i) ? 0x40 : 0x00))
//#define ENETF_TXCTRL(i)                         PPE_REG_ADDR(0x0636 + ((i) ? 0x40 : 0x00))
//#define ENETF_VLCOS0(i)                         PPE_REG_ADDR(0x0638 + ((i) ? 0x40 : 0x00))
//#define ENETF_VLCOS1(i)                         PPE_REG_ADDR(0x0639 + ((i) ? 0x40 : 0x00))
//#define ENETF_VLCOS2(i)                         PPE_REG_ADDR(0x063A + ((i) ? 0x40 : 0x00))
//#define ENETF_VLCOS3(i)                         PPE_REG_ADDR(0x063B + ((i) ? 0x40 : 0x00))
//#define ENETF_EGCOL(i)                          PPE_REG_ADDR(0x063C + ((i) ? 0x40 : 0x00))
//#define ENETF_EGDROP(i)                         PPE_REG_ADDR(0x063D + ((i) ? 0x40 : 0x00))

/*
 *  DPlus Registers
 */

/*
 *    Code/Data Memory (CDM) Interface Configuration Register
 */
#define CDM_CFG                                 PPE_REG_ADDR(0x0100)

#define CDM_CFG_RAM1_SET(value)                 SET_BITS(0, 3, 2, value)
#define CDM_CFG_RAM0_SET(value)                 ((value) ? (1 << 1) : 0)

/*
 *  QSB Internal Cell Delay Variation Register
 */
#define QSB_ICDV                                QSB_CONF_REG(0x0007)

#define QSB_ICDV_TAU                            GET_BITS(*QSB_ICDV, 5, 0)

#define QSB_ICDV_TAU_SET(value)                 SET_BITS(0, 5, 0, value)

/*
 *  QSB Scheduler Burst Limit Register
 */
#define QSB_SBL                                 QSB_CONF_REG(0x0009)

#define QSB_SBL_SBL                             GET_BITS(*QSB_SBL, 3, 0)

#define QSB_SBL_SBL_SET(value)                  SET_BITS(0, 3, 0, value)

/*
 *  QSB Configuration Register
 */
#define QSB_CFG                                 QSB_CONF_REG(0x000A)

#define QSB_CFG_TSTEPC                          GET_BITS(*QSB_CFG, 1, 0)

#define QSB_CFG_TSTEPC_SET(value)               SET_BITS(0, 1, 0, value)

/*
 *  QSB RAM Transfer Table Register
 */
#define QSB_RTM                                 QSB_CONF_REG(0x000B)

#define QSB_RTM_DM                              (*QSB_RTM)

#define QSB_RTM_DM_SET(value)                   ((value) & 0xFFFFFFFF)

/*
 *  QSB RAM Transfer Data Register
 */
#define QSB_RTD                                 QSB_CONF_REG(0x000C)

#define QSB_RTD_TTV                             (*QSB_RTD)

#define QSB_RTD_TTV_SET(value)                  ((value) & 0xFFFFFFFF)

/*
 *  QSB RAM Access Register
 */
#define QSB_RAMAC                               QSB_CONF_REG(0x000D)

#define QSB_RAMAC_RW                            (*QSB_RAMAC & (1 << 31))
#define QSB_RAMAC_TSEL                          GET_BITS(*QSB_RAMAC, 27, 24)
#define QSB_RAMAC_LH                            (*QSB_RAMAC & (1 << 16))
#define QSB_RAMAC_TESEL                         GET_BITS(*QSB_RAMAC, 9, 0)

#define QSB_RAMAC_RW_SET(value)                 ((value) ? (1 << 31) : 0)
#define QSB_RAMAC_TSEL_SET(value)               SET_BITS(0, 27, 24, value)
#define QSB_RAMAC_LH_SET(value)                 ((value) ? (1 << 16) : 0)
#define QSB_RAMAC_TESEL_SET(value)              SET_BITS(0, 9, 0, value)

/*
 *  Mailbox IGU0 Registers
 */
#define MBOX_IGU0_ISRS                          PPE_REG_ADDR(0x0200)
#define MBOX_IGU0_ISRC                          PPE_REG_ADDR(0x0201)
#define MBOX_IGU0_ISR                           PPE_REG_ADDR(0x0202)
#define MBOX_IGU0_IER                           PPE_REG_ADDR(0x0203)

/*
 *  Mailbox IGU1 Registers
 */
#define MBOX_IGU1_ISRS                          PPE_REG_ADDR(0x0204)
#define MBOX_IGU1_ISRC                          PPE_REG_ADDR(0x0205)
#define MBOX_IGU1_ISR                           PPE_REG_ADDR(0x0206)
#define MBOX_IGU1_IER                           PPE_REG_ADDR(0x0207)

/*
 *  DSL registers
 */
#define AT_CTRL                                 PPE_REG_ADDR(0xD02)
#define AR_CTRL                                 PPE_REG_ADDR(0xD08)
#define AT_IDLE0                                PPE_REG_ADDR(0xD28)
#define AT_IDLE1                                PPE_REG_ADDR(0xD29)
#define AR_IDLE0                                PPE_REG_ADDR(0xD74)
#define AR_IDLE1                                PPE_REG_ADDR(0xD75)
#define RFBI_CFG                                PPE_REG_ADDR(0x400)
#define SFSM_DBA0                               PPE_REG_ADDR(0x412)
#define SFSM_DBA1                               PPE_REG_ADDR(0x413)
#define SFSM_CBA0                               PPE_REG_ADDR(0x414)
#define SFSM_CBA1                               PPE_REG_ADDR(0x415)
#define SFSM_CFG0                               PPE_REG_ADDR(0x416)
#define SFSM_CFG1                               PPE_REG_ADDR(0x417)
#define FFSM_DBA0                               PPE_REG_ADDR(0x508)
#define FFSM_DBA1                               PPE_REG_ADDR(0x509)
#define FFSM_CFG0                               PPE_REG_ADDR(0x50A)
#define FFSM_CFG1                               PPE_REG_ADDR(0x50B)
#define FFSM_IDLE_HEAD_BC0                      PPE_REG_ADDR(0x50E)
#define FFSM_IDLE_HEAD_BC1                      PPE_REG_ADDR(0x50F)

/*
 *  Reset Registers
 */
#define AMAZON_S_RCU_BASE_ADDR                  (KSEG1 | 0x1F203000)
#define AMAZON_S_RCU_RST_REQ                    ((volatile u32*)(AMAZON_S_RCU_BASE_ADDR + 0x0010))
#define AMAZON_S_RCU_RST_STAT                   ((volatile u32*)(AMAZON_S_RCU_BASE_ADDR + 0x0014))
#define AMAZON_S_USB_CFG                        ((volatile u32*)(AMAZON_S_RCU_BASE_ADDR + 0x0018))
#define AMAZON_S_RCU_PPE_CONF                   ((volatile u32*)(AMAZON_S_RCU_BASE_ADDR + 0x002C))

/*
 *  Power Management Unit Registers
 */
//#define AMAZON_S_PMU                            (KSEG1 + 0x1F102000)
//#define PMU_PWDCR                               ((volatile u32 *)(AMAZON_S_PMU + 0x001C))
//#define PMU_SR                                  ((volatile u32 *)(AMAZON_S_PMU + 0x0020))

/*
 *  GPIO Registers
 */
//#define AMAZON_S_GPIO                           (KSEG1 + 0x1E100B00)
//#define GPIO_Pi_OUT(i)                          ((volatile u32 *)(AMAZON_S_GPIO + (i) * 0x0030 + 0x0010))
//#define GPIO_Pi_IN(i)                           ((volatile u32 *)(AMAZON_S_GPIO + (i) * 0x0030 + 0x0014))
//#define GPIO_Pi_DIR(i)                          ((volatile u32 *)(AMAZON_S_GPIO + (i) * 0x0030 + 0x0018))
//#define GPIO_Pi_ALTSEL0(i)                      ((volatile u32 *)(AMAZON_S_GPIO + (i) * 0x0030 + 0x001C))
//#define GPIO_Pi_ALTSEL1(i)                      ((volatile u32 *)(AMAZON_S_GPIO + (i) * 0x0030 + 0x0020))
//#define GPIO_Pi_OD(i)                           ((volatile u32 *)(AMAZON_S_GPIO + (i) * 0x0030 + 0x0024))
//#define GPIO_Pi_STOFF(i)                        ((volatile u32 *)(AMAZON_S_GPIO + (i) * 0x0030 + 0x0028))
//#define GPIO_Pi_PUDSEL(i)                       ((volatile u32 *)(AMAZON_S_GPIO + (i) * 0x0030 + 0x002C))
//#define GPIO_Pi_PUDEN(i)                        ((volatile u32 *)(AMAZON_S_GPIO + (i) * 0x0030 + 0x0030))

/*
 *  DMA Registers
 */
#define AMAZON_S_DMA                            (KSEG1 | 0x1E104100)
#define AMAZON_S_DMA_BASE                       AMAZON_S_DMA
#define AMAZON_S_DMA_CLC                        (volatile u32*)(AMAZON_S_DMA_BASE + 0x00)
#define AMAZON_S_DMA_ID                         (volatile u32*)(AMAZON_S_DMA_BASE + 0x08)
#define AMAZON_S_DMA_CTRL                       (volatile u32*)(AMAZON_S_DMA_BASE + 0x10)
#define AMAZON_S_DMA_CPOLL                      (volatile u32*)(AMAZON_S_DMA_BASE + 0x14)
#define AMAZON_S_DMA_CS(i)                      (volatile u32*)(AMAZON_S_DMA_BASE + 0x18 + 0x38 * (i))
#define AMAZON_S_DMA_CCTRL(i)                   (volatile u32*)(AMAZON_S_DMA_BASE + 0x1C + 0x38 * (i))
#define AMAZON_S_DMA_CDBA(i)                    (volatile u32*)(AMAZON_S_DMA_BASE + 0x20 + 0x38 * (i))
#define AMAZON_S_DMA_CDLEN(i)                   (volatile u32*)(AMAZON_S_DMA_BASE + 0x24 + 0x38 * (i))
#define AMAZON_S_DMA_CIS(i)                     (volatile u32*)(AMAZON_S_DMA_BASE + 0x28 + 0x38 * (i))
#define AMAZON_S_DMA_CIE(i)                     (volatile u32*)(AMAZON_S_DMA_BASE + 0x2C + 0x38 * (i))
#define AMAZON_S_CGBL                           (volatile u32*)(AMAZON_S_DMA_BASE + 0x30)
#define AMAZON_S_DMA_PS(i)                      (volatile u32*)(AMAZON_S_DMA_BASE + 0x40 + 0x30 * (i))
#define AMAZON_S_DMA_PCTRL(i)                   (volatile u32*)(AMAZON_S_DMA_BASE + 0x44 + 0x30 * (i))
#define AMAZON_S_DMA_IRNEN                      (volatile u32*)(AMAZON_S_DMA_BASE + 0xf4)
#define AMAZON_S_DMA_IRNCR                      (volatile u32*)(AMAZON_S_DMA_BASE + 0xf8)
#define AMAZON_S_DMA_IRNICR                     (volatile u32*)(AMAZON_S_DMA_BASE + 0xfc)

/*
 *  External Interrupt Registers
 */
#define AMAZON_S_EIU_BASE                       (KSEG1 | 0x1F101000)
#define AMAZON_S_EIU_EXIN_C                     (volatile u32*)(AMAZON_S_EIU_BASE + 0x00)
#define AMAZON_S_EIU_INIC                       (volatile u32*)(AMAZON_S_EIU_BASE + 0x04)
#define AMAZON_S_EIU_INC                        (volatile u32*)(AMAZON_S_EIU_BASE + 0x08)
#define AMAZON_S_EIU_INEN                       (volatile u32*)(AMAZON_S_EIU_BASE + 0x0C)
#define AMAZON_S_EIU_YIELDEN0                   (volatile u32*)(AMAZON_S_EIU_BASE + 0x10)
#define AMAZON_S_EIU_YIELDEN1                   (volatile u32*)(AMAZON_S_EIU_BASE + 0x14)
#define AMAZON_S_EIU_YIELDEN2                   (volatile u32*)(AMAZON_S_EIU_BASE + 0x18)
#define AMAZON_S_EIU_NMI_CR                     (volatile u32*)(AMAZON_S_EIU_BASE + 0xF0)
#define AMAZON_S_EIU_NMI_SR                     (volatile u32*)(AMAZON_S_EIU_BASE + 0xF4)

#define PPA_VDEV_ID(hw_dev_id) (hw_dev_id +   IFX_VIRT_DEV_OFFSET)
#define HW_DEV_ID(ppa_vdev_id) (ppa_vdev_id - IFX_VIRT_DEV_OFFSET)

/*
 * ####################################
 *              Data Type
 * ####################################
 */

/*------------------------------------------------------------------------------------------*\
 * header wird benutzt auf dem Weg zum PPE ( wird eingefuegt vom Switch )
 \*------------------------------------------------------------------------------------------*/

void dump_ar9_pmac_header(struct ar9_pmac_header *header, const char *title)
{
#if defined(CONFIG_AVMNET_DEBUG) 
	if (!(g_dbg_datapath & DBG_ENABLE_MASK_DUMP_PMAC_HEADER))
		return;

	printk("%s\n", title);
	printk(" ip offset      = %#x\n", (u32) header->ipOffset);
	printk(" dest portmap   = %#x\n", (u32) header->destPortMap);
	printk(" is tagged      = %#x\n", (u32) header->inputTag);
	printk(" PPPoES         = %#x\n", (u32) header->PPPoEPacket);
	printk(" ipv4           = %#x\n", (u32) header->iPv4Packet);
	printk(" ipv6           = %#x\n", (u32) header->iPv6Packet);
	printk(" mirrored       = %#x\n", (u32) header->mirrored);
	printk(" PKTLEN HI      = %#x\n", (u32) header->PKTLEN_8);
	printk(" PKTLEN LO      = %#x\n", (u32) header->PKTLEN_3);
	printk(" SPPID          = %#x\n", (u32) header->srcPortID);
	printk(" QueueId        = %#x\n", (u32) header->queueID);
#endif
}

/*
 *  64-bit Data Type
 */
typedef struct {
    unsigned int    h   :32;
    unsigned int    l   :32;
} ppe_u64_t;

/*
 *  PPE ATM Cell Header
 */
#if defined(__BIG_ENDIAN)
  struct uni_cell_header {
    unsigned int    gfc :4;
    unsigned int    vpi :8;
    unsigned int    vci :16;
    unsigned int    pti :3;
    unsigned int    clp :1;
  };
#else
  struct uni_cell_header {
    unsigned int    clp :1;
    unsigned int    pti :3;
    unsigned int    vci :16;
    unsigned int    vpi :8;
    unsigned int    gfc :4;
  };
#endif

/*
 *  Switch Header, Flag Header & Descriptor
 */
#if defined(__BIG_ENDIAN)
  struct sw_eg_pkt_header {
    unsigned int    spid                :8; //  bit 26-24
    unsigned int    dpid                :8; //  bit 17-16
    unsigned int    qid                 :8; //  bit 9-8
    unsigned int    direct              :8; //  bit 0
  };

  struct flag_header {
    //  0 - 3h
    unsigned int    ipv4_mc_pkt         :1; //  IPv4 multicast packet?
    unsigned int    res0                :1;
    unsigned int    proc_type           :1; //  0: routing, 1: bridging
    unsigned int    res1                :1;
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
    unsigned int    qid                 :4; //  7h
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
#define STD_BYTE_OFF 8
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

  struct rx_descriptor {
    //  0 - 3h
    unsigned int    own                 :1; //  0: PPE, 1: MIPS, this value set is for DSL PKT RX and DSL OAM RX, it's special case.
    unsigned int    c                   :1;
    unsigned int    sop                 :1;
    unsigned int    eop                 :1;
    unsigned int    res1                :3;
    unsigned int    byteoff             :2;
    unsigned int    res2                :2;
    unsigned int    qid                 :4;
    unsigned int    err                 :1; //  0: no error, 1: error (RA, IL, CRC, USZ, OVZ, MFL, CPI, CPCS-UU)
    unsigned int    datalen             :16;
    //  4 - 7h
    unsigned int    res3                :3;
    unsigned int    dataptr             :29;
  };

  struct tx_descriptor {
    //  0 - 3h
    unsigned int    own                 :1; //  0: MIPS, 1: PPE, for CPU to WAN TX, it's inverse, 0: PPE, 1: MIPS
    unsigned int    c                   :1;
    unsigned int    sop                 :1;
    unsigned int    eop                 :1;
    unsigned int    byteoff             :5;
    unsigned int    mpoa_pt             :1; //  not for WLAN TX, 0: MPoA is dterminated in FW, 1: MPoA is transparent to FW.
    unsigned int    mpoa_type           :2; //  not for WLAN TX, 0: EoA without FCS, 1: reserved, 2: PPPoA, 3: IPoA
    unsigned int    pdu_type            :1; //  not for WLAN TX, 0: AAL5, 1: Non-AAL5 cell
    unsigned int    qid                 :4; //  not for WLAN TX, careful, in DSL it's 4 bits, in ETH it's 3 bits
    unsigned int    datalen             :15;//  careful, in DSL it's 15 bits, in ETH it's 16 bits
    //  4 - 7h
    unsigned int    small               :1;
    unsigned int    res1                :2;
    unsigned int    dataptr             :29;
  };

  struct eth_tx_descriptor {
    //  0 - 3h
    unsigned int    own                 :1; //  0: MIPS, 1: PPE
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
#else
#endif

/*
 *  QSB Queue Parameter Table Entry and Queue VBR Parameter Table Entry
 */
#if defined(__BIG_ENDIAN)
  union qsb_queue_parameter_table {
    struct {
      unsigned int  res1    :1;
      unsigned int  vbr     :1;
      unsigned int  wfqf    :14;
      unsigned int  tp      :16;
    }             bit;
    u32           dword;
  };

  union qsb_queue_vbr_parameter_table {
    struct {
      unsigned int  taus    :16;
      unsigned int  ts      :16;
    }             bit;
    u32           dword;
  };
#else
  union qsb_queue_parameter_table {
    struct {
      unsigned int  tp      :16;
      unsigned int  wfqf    :14;
      unsigned int  vbr     :1;
      unsigned int  res1    :1;
    }             bit;
    u32           dword;
  };

  union qsb_queue_vbr_parameter_table {
    struct {
      unsigned int  ts      :16;
      unsigned int  taus    :16;
    }             bit;
    u32           dword;
  };
#endif  //  defined(__BIG_ENDIAN)

/*
 *  Internal Structure of Devices (ETH/ATM)
 */
struct eth_priv_data {
    struct  net_device_stats        stats;

    unsigned int                    rx_packets;
    unsigned int                    rx_bytes;
    unsigned int                    rx_dropped;
    unsigned int                    tx_packets;
    unsigned int                    tx_bytes;
    unsigned int                    tx_errors;
    unsigned int                    tx_dropped;

    unsigned int                    dev_id;
};

struct atm_port {
    struct atm_dev                 *dev;

    u32                             tx_max_cell_rate;       /*  maximum cell rate                       */
    u32                             tx_used_cell_rate;      /*  currently used cell rate                */
};

struct atm_connection {
    struct atm_vcc                 *vcc;                    /*  opened VCC                              */
    struct timespec                 access_time;            /*  time when last F4/F5 user cell arrives  */

    int                             prio_queue_map[8];
    u32                             prio_tx_packets[8];

    u32                             rx_packets;
    u32                             rx_bytes;
    u32                             rx_errors;
    u32                             rx_dropped;
    u32                             tx_packets;
    u32                             tx_bytes;
    u32                             tx_errors;
    u32                             tx_dropped;

    u32                             port;                   /*  to which port the connection belongs    */
    u32                             sw_tx_queue_table;      /*  software TX queues used for this        */
                                                            /*  connection                              */
};

struct atm_priv_data {
    struct atm_port                 port[ATM_PORT_NUMBER];
    volatile struct atm_connection           connection[ATM_QUEUE_NUMBER];
    u32                             max_connections;        /*  total connections available             */
    u32                             connection_table;       /*  connection opened status, every bit     */
                                                            /*  stands for one connection as well as    */
                                                            /*  the QSB queue used by this connection   */
    u32                             max_sw_tx_queues;       /*  total software TX queues available      */
    u32                             sw_tx_queue_table;      /*  software TX queue occupations status    */

    ppe_u64_t                       wrx_total_byte;         /*  bit-64 extention of MIB table member    */
    ppe_u64_t                       wtx_total_byte;         /*  bit-64 extention of MIB talbe member    */

    u32                             wrx_pdu;                /*  successfully received AAL5 packet       */
    u32                             wrx_drop_pdu;           /*  AAL5 packet dropped by driver on RX     */
    u32                             wtx_pdu;
    u32                             wtx_err_pdu;            /*  error AAL5 packet                       */
    u32                             wtx_drop_pdu;           /*  AAL5 packet dropped by driver on TX     */

    struct dsl_wan_mib_table        prev_mib;
};

typedef struct fw_dbg {
    char *cmd;
    void (*pfunc)(char **tokens, int token_num);
}fw_dbg_t;



/*
 * ####################################
 *             Declaration
 * ####################################
 */

/*
 *  ATM Operations
 */
static int ppe_ioctl(struct atm_dev *, unsigned int, void *);
static int ppe_open(struct atm_vcc *);
/*
 *  Description:
 *    Handle all ioctl command. This is the only way, which user level could
 *    use to access PPE driver.
 *  Input:
 *    inode --- struct inode *, file descriptor on drive
 *    file  --- struct file *, file descriptor of virtual file system
 *    cmd   --- unsigned int, device specific commands.
 *    arg   --- unsigned long, pointer to a structure to pass in arguments or
 *              pass out return value.
 *  Output:
 *    int   --- 0:    Success
 *              else: Error Code
 */


/*
 *  Description:
 *    Handle all ioctl command. This is the only way, which user level could
 *    use to access PPE driver.
 *  Input:
 *    inode --- struct inode *, file descriptor on drive
 *    file  --- struct file *, file descriptor of virtual file system
 *    cmd   --- unsigned int, device specific commands.
 *    arg   --- unsigned long, pointer to a structure to pass in arguments or
 *              pass out return value.
 *  Output:
 *    int   --- 0:    Success
 *              else: Error Code
 */


/*
 *  Description:
 *    Handle all ioctl command. This is the only way, which user level could
 *    use to access PPE driver.
 *  Input:
 *    inode --- struct inode *, file descriptor on drive
 *    file  --- struct file *, file descriptor of virtual file system
 *    cmd   --- unsigned int, device specific commands.
 *    arg   --- unsigned long, pointer to a structure to pass in arguments or
 *              pass out return value.
 *  Output:
 *    int   --- 0:    Success
 *              else: Error Code
 */


/*
 *  Description:
 *    Handle all ioctl command. This is the only way, which user level could
 *    use to access PPE driver.
 *  Input:
 *    inode --- struct inode *, file descriptor on drive
 *    file  --- struct file *, file descriptor of virtual file system
 *    cmd   --- unsigned int, device specific commands.
 *    arg   --- unsigned long, pointer to a structure to pass in arguments or
 *              pass out return value.
 *  Output:
 *    int   --- 0:    Success
 *              else: Error Code
 */


/*
 *  Description:
 *    Handle all ioctl command. This is the only way, which user level could
 *    use to access PPE driver.
 *  Input:
 *    inode --- struct inode *, file descriptor on drive
 *    file  --- struct file *, file descriptor of virtual file system
 *    cmd   --- unsigned int, device specific commands.
 *    arg   --- unsigned long, pointer to a structure to pass in arguments or
 *              pass out return value.
 *  Output:
 *    int   --- 0:    Success
 *              else: Error Code
 */


/*
 *  Description:
 *    Handle all ioctl command. This is the only way, which user level could
 *    use to access PPE driver.
 *  Input:
 *    inode --- struct inode *, file descriptor on drive
 *    file  --- struct file *, file descriptor of virtual file system
 *    cmd   --- unsigned int, device specific commands.
 *    arg   --- unsigned long, pointer to a structure to pass in arguments or
 *              pass out return value.
 *  Output:
 *    int   --- 0:    Success
 *              else: Error Code
 */


/*
 *  Description:
 *    Handle all ioctl command. This is the only way, which user level could
 *    use to access PPE driver.
 *  Input:
 *    inode --- struct inode *, file descriptor on drive
 *    file  --- struct file *, file descriptor of virtual file system
 *    cmd   --- unsigned int, device specific commands.
 *    arg   --- unsigned long, pointer to a structure to pass in arguments or
 *              pass out return value.
 *  Output:
 *    int   --- 0:    Success
 *              else: Error Code
 */


static void ppe_close(struct atm_vcc *);
static int ppe_send(struct atm_vcc *, struct sk_buff *);
static int ppe_send_oam(struct atm_vcc *, void *, int);
static int ppe_change_qos(struct atm_vcc *, struct atm_qos *, int);
static void dump_atm_tx_desc( struct tx_descriptor *d );

static int ppe_clk_change(unsigned int arg, unsigned int flags);

/*
 *  ATM Upper Layer Hook Function
 */
static void mpoa_setup(struct atm_vcc *, int, int);
static void mpoa_setup_conn(int conn, int mpoa_type, int f_llc);

/*
 *  Network Operations
 */

int ifx_ppa_eth_hard_start_xmit_a5(struct sk_buff *, struct net_device *);
int ifx_ppa_eth_qos_hard_start_xmit_a5(struct sk_buff *, struct net_device *);
int ifx_ppa_eth_ioctl_a5(struct net_device *, struct ifreq *, int);
void ifx_ppa_eth_tx_timeout_a5(struct net_device *);

/*
 *  Network operations help functions
 */
static INLINE int eth_xmit(struct sk_buff *, unsigned int, int, int, int);
static int process_backlog_a5(struct napi_struct *napi, int quota);
static INLINE int handle_skb_a5(struct sk_buff *skb);

/*
 *  ioctl help functions
 */
static INLINE int ethtool_ioctl(struct net_device *, struct ifreq *);
#if 0   //  TODO
  static INLINE void set_vlan_cos(struct vlan_cos_req *);
  static INLINE void set_dscp_cos(struct dscp_cos_req *);
#endif

/*
 *  64-bit operation used by MIB calculation
 */
static INLINE void u64_add_u32(ppe_u64_t opt1, u32 opt2,ppe_u64_t *ret);

/*
 *  DSL led flash function
 */
static INLINE void adsl_led_flash(void);

/*
 *  DSL PVC Multiple Queue/Priority Operation
 */
static int sw_tx_queue_add(int);
static int sw_tx_queue_del(int);
static int sw_tx_prio_to_queue(int, int, int, int *);

/*
 *  QSB & HTU setting functions
 */
static void set_qsb(struct atm_vcc *, struct atm_qos *, unsigned int);
static INLINE void set_htu_entry(unsigned int, unsigned int, unsigned int, int);
static INLINE void clear_htu_entry(unsigned int);

/*
 *  QSB & HTU init functions
 */
static INLINE void qsb_global_set(void);
static INLINE void setup_oam_htu_entry(void);
static INLINE void validate_oam_htu_entry(void);
static INLINE void invalidate_oam_htu_entry(void);

/*
 *  look up for connection ID
 */
static INLINE int find_vpi(unsigned int);
static INLINE int find_vpivci(unsigned int, unsigned int);
static INLINE int find_vcc(struct atm_vcc *);

/*
 *  Buffer manage functions
 */
static INLINE struct sk_buff *alloc_skb_rx(void);
static struct sk_buff *alloc_skb_tx_aligned(struct sk_buff *, int);
static INLINE struct sk_buff *alloc_skb_tx(int);
static INLINE void __get_skb_from_dbg_pool(struct sk_buff *, const char *, unsigned int);
#define get_skb_from_dbg_pool(skb)  __get_skb_from_dbg_pool(skb, __FUNCTION__, __LINE__)
static INLINE void __put_skb_to_dbg_pool(struct sk_buff *, const char *, unsigned int);
#define put_skb_to_dbg_pool(skb)    __put_skb_to_dbg_pool(skb, __FUNCTION__, __LINE__)
static struct sk_buff* atm_alloc_tx(struct atm_vcc *, unsigned int);
static INLINE void atm_free_tx_skb_vcc(struct sk_buff *skb);

/*
 *  Mailbox handler
 */
static irqreturn_t mailbox0_irq_handler(int, void *);
static irqreturn_t mailbox1_irq_handler(int, void *);

/*
 *  Turn On/Off Dma
 */
static INLINE void turn_on_dma_rx(int);
static INLINE void turn_off_dma_rx(int);

/*
 *  DMA interface functions
 */
static u8 *dma_buffer_alloc(int, int *, void **);
static int dma_buffer_free(u8 *, void *);
static int dma_int_handler(struct dma_device_info *, int, int);
static INLINE int dma_rx_int_handler(struct dma_device_info *, int dma_ch_no);

/*
 *  MII/Switch help functions
 */
static INLINE u16 smi_reg_read(u16 reg);
static INLINE void smi_reg_write(u16 reg, u16 data);
static INLINE void init_external_tantos(void);

/*
 *  GPHY Interrupt Function (MII1 Auto-negotiate)
 */
#ifdef CONFIG_MII1_AUTONEG
  static INLINE void mii1_gphy_config_gpio(void);
  static INLINE void mii1_gphy_detect(void);
  static INLINE void mii1_gphy_config_gphy(void);
  static INLINE void mii1_gphy_config_ext_int(void);
  static irqreturn_t mii1_gphy_11G_int_handler(int, void *);
  static irqreturn_t mii1_gphy_VSC_int_handler(int, void *);
#endif

/*
 *  Hardware init functions
 */
static INLINE void reset_ppe(void);
static INLINE void init_pmu(void);
static INLINE void init_mii0(int);
static INLINE void init_mii1(int);
static INLINE void init_mii2(void);
static INLINE void start_mii2(void);
static INLINE void stop_datapath(void);
static INLINE void init_internal_tantos_qos_setting(void);
static INLINE void init_internal_tantos(void);
static INLINE void init_dplus(void);
static INLINE void init_ema(void);
static INLINE void init_mailbox(void);
static INLINE void clear_share_buffer(void);
static INLINE void clear_cdm(void);
static INLINE void board_init(void);
static INLINE void init_dsl_hw(void);
static INLINE void setup_ppe_conf(void);
static INLINE void init_hw(void);

/*
 *  PP32 specific functions
 */
static INLINE int pp32_download_code(const u32 *, unsigned int, const u32 *, unsigned int);
static INLINE int pp32_specific_init(int, void *);
static INLINE int pp32_start(int);
static INLINE void pp32_stop(void);

/*
 *  Init & clean-up functions
 */
static INLINE int init_local_variables(void);
static INLINE void clear_local_variables(void);
static INLINE void init_communication_data_structures(int);
static INLINE int alloc_dma(void);
static INLINE void free_dma(void);
#if defined(CONFIG_IFX_PPA_DATAPATH)
  static INLINE void ethaddr_setup(unsigned int, char *);
#endif


/*
 *  local implement memcpy
 */
#if defined(ENABLE_MY_MEMCPY) && ENABLE_MY_MEMCPY
  static INLINE void *my_memcpy(unsigned char *, const unsigned char *, unsigned int);
#else
  #define my_memcpy             memcpy
#endif

/*
 *  Print Firmware/Driver Version ID
 */
static int print_fw_ver(char *, int);
static int print_driver_ver(char *, int, char *, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int);

static int dma_alignment_atm_good_count = 0;
static int dma_alignment_atm_bad_count = 0;

static int dma_alignment_eth_good_count = 0;
static int dma_alignment_eth_bad_count = 0;


/*
 *  Debug functions
 */
#if defined(DEBUG_DUMP_FLAG_HEADER) && DEBUG_DUMP_FLAG_HEADER
  static INLINE void dump_flag_header(int, struct flag_header *, const char *, int, int);
#else
  #define dump_flag_header(a, b, c, d, e)
#endif
#if defined(DEBUG_DUMP_INIT) && DEBUG_DUMP_INIT
  static INLINE void dump_init(void);
#else
  #define dump_init()
#endif

#ifdef CONFIG_IFX_PORT_MIRROR
  extern struct net_device * (*ifx_get_mirror_netdev)(void);
  extern uint32_t (*ifx_is_device_type_wireless)(void);
  static void mirror_pkt_eth(struct sk_buff *, int);
  static void mirror_pkt_atm(struct sk_buff *, int);
#endif

/*
 *  Directpath Help Functions
 */
#if defined(ENABLE_DIRECTPATH_TX_QUEUE) && ENABLE_DIRECTPATH_TX_QUEUE
  static int send_directpath_tx_queue(void);
#endif

/*
 *  External Functions
 */
extern void (*ppa_hook_mpoa_setup)(struct atm_vcc *, int, int);
#if defined(CONFIG_LTQ_OAM) || defined(CONFIG_LTQ_OAM_MODULE)
  extern void ifx_push_oam(unsigned char *);
#else
  static inline void ifx_push_oam(unsigned char *dummy) {}
#endif
#if defined(CONFIG_IFXMIPS_DSL_CPE_MEI) || defined(CONFIG_IFXMIPS_DSL_CPE_MEI_MODULE)
 #if !defined(ENABLE_LED_FRAMEWORK) || !ENABLE_LED_FRAMEWORK
  extern int ifx_mei_atm_led_blink(void) __attribute__ ((weak));
 #endif
  extern int ifx_mei_atm_showtime_check(int *is_showtime,
		struct port_cell_info *port_cell, void **xdata_addr) __attribute__ ((weak));
#else
 #if !defined(ENABLE_LED_FRAMEWORK) || !ENABLE_LED_FRAMEWORK
  static inline int ifx_mei_atm_led_blink(void) { return IFX_SUCCESS; }
 #endif
  static inline int ifx_mei_atm_showtime_check(int *is_showtime, struct port_cell_info *port_cell, void **xdata_addr)
  {
    if ( is_showtime != NULL )
        *is_showtime = 0;
    return IFX_SUCCESS;
  }
#endif

/*
 *  External Variables
 */
extern struct sk_buff* (*ifx_atm_alloc_tx)(struct atm_vcc *, unsigned int);
#if defined(CONFIG_IFXMIPS_DSL_CPE_MEI) || defined(CONFIG_IFXMIPS_DSL_CPE_MEI_MODULE)
extern int (*ifx_mei_atm_showtime_enter)(struct port_cell_info *, void *) __attribute__ ((weak));
extern int (*ifx_mei_atm_showtime_exit)(void) __attribute__ ((weak));
#else
int (*ifx_mei_atm_showtime_enter)(struct port_cell_info *, void *) = NULL;
EXPORT_SYMBOL(ifx_mei_atm_showtime_enter);
int (*ifx_mei_atm_showtime_exit)(void) = NULL;
EXPORT_SYMBOL(ifx_mei_atm_showtime_exit);
#error
#endif



/*
 * ####################################
 *            Local Variable
 * ####################################
 */

static int g_fwcode = FWCODE_ROUTING_ACC_A5;

static struct semaphore g_sem;  //  lock used by open/close function

static int g_eth_wan_mode = 0;
static unsigned int g_wan_itf = 3 << 6;

static int g_ipv6_acc_en = 1; //TODO UGW 5.1.1: 1 --- vorischt auch ipv6_acc_en anpassen
static unsigned int g_ipv4_lan_collision_rout_fwdc_table_base;
static unsigned int g_ipv4_wan_collision_rout_fwda_table_base;

static int g_wanqos_en = 0;

static int g_cpu_to_wan_tx_desc_pos = 0;
static DEFINE_SPINLOCK(g_cpu_to_wan_tx_spinlock);
static int g_cpu_to_wan_swap_desc_pos = 0;
static int g_wan_rx1_desc_pos = 0;
struct sk_buff *g_wan_rx1_desc_skb[WAN_RX_DESC_NUM(1)] = {0};

static u32 g_mailbox_signal_mask;

static int                      g_f_dma_opened = 0;

static struct atm_priv_data g_atm_priv_data;

static struct atmdev_ops g_ppe_atm_ops = {
    owner:      THIS_MODULE,
    open:       ppe_open,
    close:      ppe_close,
    ioctl:      ppe_ioctl,
    send:       ppe_send,
    send_oam:   ppe_send_oam,
    change_qos: ppe_change_qos,
};

#ifdef CONFIG_IFXMIPS_MEI_FW_LOOPBACK
  static int g_showtime = 1;
#else
  static int g_showtime = 0;
#endif
static void *g_xdata_addr = NULL;

static u32 g_dsl_wrx_correct_pdu = 0;
static u32 g_dsl_wtx_total_pdu = 0;
#if defined(ENABLE_LED_FRAMEWORK) && ENABLE_LED_FRAMEWORK && defined(CONFIG_IFX_LED)
  static void *g_data_led_trigger = NULL;
#endif

static int g_wan_queue_class_map[8];    //  So far, this table is shared by all interfaces.

static DEFINE_SPINLOCK(g_eth_tx_spinlock);

//  1:switch forward packets between port0/1 and cpu port directly w/o processing
//  0:pkts go through normal path and are processed by switch central function
static int g_ingress_direct_forward_enable = 0;
static int g_egress_direct_forward_enable = 1;

#ifdef CONFIG_MII1_AUTONEG
  static int g_id;
  static int g_link_up = 0;         //  link status:    0: down, 1: up
  static int g_link_speed = 1000;   //  link speed:     10/100/1000 Mbps
  static int g_link_duplex = 1;     //  link is duplex: 0: simplex, 1: duplex
#endif

/*
 *  variable for directpath
 */
static int g_directpath_dma_full = 0;
static DEFINE_SPINLOCK(g_directpath_tx_spinlock);

#if defined(ENABLE_CONFIGURABLE_DSL_VLAN) && ENABLE_CONFIGURABLE_DSL_VLAN
  static unsigned short g_dsl_vlan_qid_map[ATM_QUEUE_NUMBER * 2] = {0};
#endif

/*
 *  variable for test
 */
#if defined(DEBUG_SKB_SWAP) && DEBUG_SKB_SWAP
  static struct sk_buff *g_dbg_skb_swap_pool[1024] = {0};
#endif



/*
 * ####################################
 *           Global Variable
 * ####################################
 */

/*
 *  variable for directpath
 */

#if defined(ENABLE_DIRECTPATH_TX_QUEUE)
// replaced by common ppa_pushthrough_data, ppa_virt_rx_devs, ppa_virt_tx_devs
// struct ppe_directpath_data g_ppe_directpath_data[3];    //  port 0 - eth0, 1 - eth1, 2 - cpu, 6 - dsl (PPPoA/IPoA), 7 - dsl (EoA)
#error AVM_CONCEPT doesnet support DIRECTPATH_TX_QUEUE so far
#endif



/*
 * ####################################
 *            Local Function
 * ####################################
*/

#define PP32_REG_ADDR_BEGIN     0x0
#define PP32_REG_ADDR_END       0x1FFF
#define PP32_SB_ADDR_BEGIN      0x2000
#define PP32_SB_ADDR_END        0x4BFF
static inline unsigned long sb_addr_to_fpi_addr_convert(unsigned long sb_addr) {
   if ( sb_addr <= PP32_SB_ADDR_END) {
      return (unsigned long ) SB_BUFFER(sb_addr);
   }
   else {
      return sb_addr;
   }
}
/*
 * ####################################
 *        Common Datapath Functions
 * ####################################
 */
#include "../common/ifxmips_ppa_datapath_common.h"


/*
 *  Description:
 *    Handle all ioctl command. This is the only way, which user level could
 *    use to access PPE driver.
 *  Input:
 *    inode --- struct inode *, file descriptor on drive
 *    file  --- struct file *, file descriptor of virtual file system
 *    cmd   --- unsigned int, device specific commands.
 *    arg   --- unsigned long, pointer to a structure to pass in arguments or
 *              pass out return value.
 *  Output:
 *    int   --- 0:    Success
 *              else: Error Code
 */

static void dump_eg_pkt_header(struct sw_eg_pkt_header *header) {
#if defined(CONFIG_AVMNET_DEBUG) 
	if ( !(g_dbg_datapath & DBG_ENABLE_MASK_DUMP_EG_HEADER))
        return;

	printk(KERN_ERR "[%s] called by %pF\n", __FUNCTION__, __builtin_return_address(0));
	printk(KERN_ERR "[%s] spid=%d\n", __FUNCTION__, header->spid );
	printk(KERN_ERR "[%s] dpid=%d\n", __FUNCTION__, header->dpid );
	printk(KERN_ERR "[%s] qid=%d\n", __FUNCTION__, header->qid );
	printk(KERN_ERR "[%s] direct=%d\n", __FUNCTION__, header->direct );
#endif
};


static int ppe_ioctl(struct atm_dev *dev __maybe_unused, unsigned int cmd, void *arg)
{
    int ret = 0;
    atm_cell_ifEntry_t mib_cell;
    atm_aal5_ifEntry_t mib_aal5;
    atm_aal5_vcc_x_t mib_vcc;
    u32 value;
    int conn;

    if ( _IOC_TYPE(cmd) != PPE_ATM_IOC_MAGIC
        || _IOC_NR(cmd) >= PPE_ATM_IOC_MAXNR )
        return -ENOTTY;

    if ( _IOC_DIR(cmd) & _IOC_READ )
        ret = !access_ok(VERIFY_WRITE, arg, _IOC_SIZE(cmd));
    else if ( _IOC_DIR(cmd) & _IOC_WRITE )
        ret = !access_ok(VERIFY_READ, arg, _IOC_SIZE(cmd));
    if ( ret )
        return -EFAULT;

    switch ( cmd )
    {
    case PPE_ATM_MIB_CELL:  /*  cell level  MIB */
        /*  These MIB should be read at ARC side, now put zero only.    */
        mib_cell.ifHCInOctets_h = 0;
        mib_cell.ifHCInOctets_l = 0;
        mib_cell.ifHCOutOctets_h = 0;
        mib_cell.ifHCOutOctets_l = 0;
        mib_cell.ifInErrors = 0;
        mib_cell.ifInUnknownProtos = DSL_WAN_MIB_TABLE->wrx_drophtu_cell;
        mib_cell.ifOutErrors = 0;

        ret = sizeof(mib_cell) - copy_to_user(arg, &mib_cell, sizeof(mib_cell));
        break;
    case PPE_ATM_MIB_AAL5:  /*  AAL5 MIB    */
        value = DSL_WAN_MIB_TABLE->wrx_total_byte;
        u64_add_u32(g_atm_priv_data.wrx_total_byte, value - g_atm_priv_data.prev_mib.wrx_total_byte, &g_atm_priv_data.wrx_total_byte);
        g_atm_priv_data.prev_mib.wrx_total_byte = value;
        mib_aal5.ifHCInOctets_h = g_atm_priv_data.wrx_total_byte.h;
        mib_aal5.ifHCInOctets_l = g_atm_priv_data.wrx_total_byte.l;

        value = DSL_WAN_MIB_TABLE->wtx_total_byte;
        u64_add_u32(g_atm_priv_data.wtx_total_byte, value - g_atm_priv_data.prev_mib.wtx_total_byte, &g_atm_priv_data.wtx_total_byte);
        g_atm_priv_data.prev_mib.wtx_total_byte = value;
        mib_aal5.ifHCOutOctets_h = g_atm_priv_data.wtx_total_byte.h;
        mib_aal5.ifHCOutOctets_l = g_atm_priv_data.wtx_total_byte.l;

        mib_aal5.ifInUcastPkts  = g_atm_priv_data.wrx_pdu;
        mib_aal5.ifOutUcastPkts = DSL_WAN_MIB_TABLE->wtx_total_pdu;
        mib_aal5.ifInErrors     = DSL_WAN_MIB_TABLE->wrx_err_pdu;
        mib_aal5.ifInDiscards   = DSL_WAN_MIB_TABLE->wrx_dropdes_pdu + g_atm_priv_data.wrx_drop_pdu;
        mib_aal5.ifOutErros     = g_atm_priv_data.wtx_err_pdu;
        mib_aal5.ifOutDiscards  = g_atm_priv_data.wtx_drop_pdu;

        ret = sizeof(mib_aal5) - copy_to_user(arg, &mib_aal5, sizeof(mib_aal5));
        break;
    case PPE_ATM_MIB_VCC:   /*  VCC related MIB */
        copy_from_user(&mib_vcc, arg, sizeof(mib_vcc));
        conn = find_vpivci(mib_vcc.vpi, mib_vcc.vci);
        if ( conn >= 0 )
        {
            u32 sw_tx_queue_table;
            int sw_tx_queue;

            memset(&mib_vcc.mib_vcc, 0, sizeof(mib_vcc.mib_vcc));
            //mib_vcc.mib_vcc.aal5VccCrcErrors     = 0;   //  no support any more, g_atm_priv_data.connection[conn].aal5_vcc_crc_err;
            //mib_vcc.mib_vcc.aal5VccOverSizedSDUs = 0;   //  no support any more, g_atm_priv_data.connection[conn].aal5_vcc_oversize_sdu;
            //mib_vcc.mib_vcc.aal5VccSarTimeOuts   = 0;   //  no timer support
            mib_vcc.mib_vcc.aal5VccRxPDU    = DSL_QUEUE_RX_MIB_TABLE(conn)->pdu;
            mib_vcc.mib_vcc.aal5VccRxBytes  = DSL_QUEUE_RX_MIB_TABLE(conn)->bytes;
            for ( sw_tx_queue_table = g_atm_priv_data.connection[conn].sw_tx_queue_table, sw_tx_queue = 0;
                  sw_tx_queue_table != 0;
                  sw_tx_queue_table >>= 1, sw_tx_queue++ )
                if ( (sw_tx_queue_table & 0x01) )
                {
                    mib_vcc.mib_vcc.aal5VccTxPDU        += DSL_QUEUE_TX_MIB_TABLE(sw_tx_queue)->pdu;
                    mib_vcc.mib_vcc.aal5VccTxBytes      += DSL_QUEUE_TX_MIB_TABLE(sw_tx_queue)->bytes;
                    mib_vcc.mib_vcc.aal5VccTxDroppedPDU += DSL_QUEUE_TX_DROP_MIB_TABLE(sw_tx_queue)->pdu;
                }            
			ret = sizeof(mib_vcc) - copy_to_user(arg, &mib_vcc, sizeof(mib_vcc));
        }
        else
            ret = -EINVAL;
        break;
    case PPE_ATM_MAP_PKT_PRIO_TO_Q:
        {
            struct ppe_prio_q_map cmd;

            if ( copy_from_user(&cmd, arg, sizeof(cmd)) )
                return -EFAULT;

            if ( cmd.qid < 0 || cmd.pkt_prio < 0 || cmd.pkt_prio >= 8 )
                return -EINVAL;

            conn = find_vpivci(cmd.vpi, cmd.vci);
            if ( conn < 0 )
                return -EINVAL;

            return sw_tx_prio_to_queue(conn, cmd.pkt_prio, cmd.qid, NULL);
        }
        break;
    case PPE_ATM_TX_Q_OP:
        {
            struct tx_q_op cmd;

            if ( copy_from_user(&cmd, arg, sizeof(cmd)) )
                return -EFAULT;

            if ( !(cmd.flags & PPE_ATM_TX_Q_OP_CHG_MASK) )
                return -EFAULT;

            conn = find_vpivci(cmd.vpi, cmd.vci);
            if ( conn < 0 )
                return -EINVAL;

            if ( (cmd.flags & PPE_ATM_TX_Q_OP_ADD) )
                return sw_tx_queue_add(conn);
            else
                return sw_tx_queue_del(conn);
        }
        break;
    case PPE_ATM_GET_MAP_PKT_PRIO_TO_Q:
        {
            struct ppe_prio_q_map_all cmd;
            u32 sw_tx_queue_table;
            int sw_tx_queue_to_virt_queue_map[ATM_SW_TX_QUEUE_NUMBER] = {0};
            int sw_tx_queue;
            int virt_queue;
            int i;

            if ( copy_from_user(&cmd, arg, sizeof(cmd)) )
                return -EFAULT;

            conn = find_vpivci(cmd.vpi, cmd.vci);
            if ( conn >= 0 )
            {
                for ( sw_tx_queue_table = g_atm_priv_data.connection[conn].sw_tx_queue_table, sw_tx_queue = virt_queue = 0;
                      sw_tx_queue_table != 0;
                      sw_tx_queue_table >>= 1, sw_tx_queue++ )
                    if ( (sw_tx_queue_table & 0x01) )
                    {
                        sw_tx_queue_to_virt_queue_map[sw_tx_queue] = virt_queue++;
                    }

                cmd.total_queue_num = virt_queue;
                for ( i = 0; i < 8; i++ )
                {
                    cmd.pkt_prio[i] = i;
                    cmd.qid[i] = sw_tx_queue_to_virt_queue_map[g_atm_priv_data.connection[conn].prio_queue_map[i]];
                }

                ret = sizeof(cmd) - copy_to_user(arg, &cmd, sizeof(cmd));
            }
            else
                ret = -EINVAL;
        }
        break;
    default:
        ret = -ENOIOCTLCMD;
    }

    return ret;
}

static int ppe_open(struct atm_vcc *vcc)
{
    int ret;
    short vpi = vcc->vpi;
    int   vci = vcc->vci;
    struct atm_port *port = &g_atm_priv_data.port[(int)vcc->dev->dev_data];
    int sw_tx_queue;
    int conn;
    int f_enable_irq = 0;
    struct wrx_queue_config wrx_queue_config = {0};
    struct wtx_queue_config wtx_queue_config = {0};
    struct uni_cell_header *cell_header;
    int i;

    if ( g_eth_wan_mode )
    {
        err("ETH WAN mode: g_eth_wan_mode = %d", g_eth_wan_mode);
        return -EIO;
    }

    if ( vcc->qos.aal != ATM_AAL5 && vcc->qos.aal != ATM_AAL0 )
        return -EPROTONOSUPPORT;

    down(&g_sem);

    /*  check bandwidth */
    if ( (vcc->qos.txtp.traffic_class == ATM_CBR && vcc->qos.txtp.max_pcr > (port->tx_max_cell_rate - port->tx_used_cell_rate))
      || (vcc->qos.txtp.traffic_class == ATM_VBR_RT && vcc->qos.txtp.max_pcr > (port->tx_max_cell_rate - port->tx_used_cell_rate))
#if defined(DISABLE_VBR) && DISABLE_VBR
      || (vcc->qos.txtp.traffic_class == ATM_VBR_NRT && vcc->qos.txtp.pcr > (port->tx_max_cell_rate - port->tx_used_cell_rate))
#else
      || (vcc->qos.txtp.traffic_class == ATM_VBR_NRT && vcc->qos.txtp.scr > (port->tx_max_cell_rate - port->tx_used_cell_rate))
#endif
      || (vcc->qos.txtp.traffic_class == ATM_UBR_PLUS && vcc->qos.txtp.min_pcr > (port->tx_max_cell_rate - port->tx_used_cell_rate)) )
    {
        err("exceed TX line rate");
        ret = -EINVAL;
        goto PPE_OPEN_EXIT;
    }

    /*  check existing vpi,vci  */
    conn = find_vpivci(vpi, vci);
    if ( conn >= 0 )
    {
        err("existing PVC (%d.%d)", (int)vpi, vci);
        ret = -EADDRINUSE;
        goto PPE_OPEN_EXIT;
    }

    /*  check whether it need to enable irq */
    if ( g_atm_priv_data.connection_table == 0 )
        f_enable_irq = 1;

    /*  allocate software TX queue  */
    for ( sw_tx_queue = 0; sw_tx_queue < g_atm_priv_data.max_sw_tx_queues; sw_tx_queue++ )
        if ( !(g_atm_priv_data.sw_tx_queue_table & (1 << sw_tx_queue)) )
            break;
    if ( sw_tx_queue == g_atm_priv_data.max_sw_tx_queues )
    {
        err("no free TX queue");
        ret = -EINVAL;
        goto PPE_OPEN_EXIT;
    }

    /*  allocate connection */
    for ( conn = 0; conn < g_atm_priv_data.max_connections; conn++ )
        if ( !(g_atm_priv_data.connection_table & (1 << conn)) )
        {
            g_atm_priv_data.connection_table |= 1 << conn;
            g_atm_priv_data.sw_tx_queue_table |= 1 << sw_tx_queue;
            g_atm_priv_data.connection[conn].vcc = vcc;
            g_atm_priv_data.connection[conn].port = (u32)vcc->dev->dev_data;
            g_atm_priv_data.connection[conn].sw_tx_queue_table = 1 << sw_tx_queue;
            for ( i = 0; i < 8; i++ )
                g_atm_priv_data.connection[conn].prio_queue_map[i] = sw_tx_queue;
            break;
        }
    if ( conn == g_atm_priv_data.max_connections )
    {
        err("exceed PVC limit");
        ret = -EINVAL;
        goto PPE_OPEN_EXIT;
    }

    /*  reserve bandwidth   */
    switch ( vcc->qos.txtp.traffic_class )
    {
    case ATM_CBR:
    case ATM_VBR_RT:
        port->tx_used_cell_rate += vcc->qos.txtp.max_pcr;
        break;
    case ATM_VBR_NRT:
#if defined(DISABLE_VBR) && DISABLE_VBR
        port->tx_used_cell_rate += vcc->qos.txtp.pcr;
#else
        port->tx_used_cell_rate += vcc->qos.txtp.scr;
#endif
        break;
    case ATM_UBR_PLUS:
        port->tx_used_cell_rate += vcc->qos.txtp.min_pcr;
        break;
    }

    /*  setup RX queue cfg and TX queue cfg */
    wrx_queue_config.new_vlan   = 0x0FF0 | conn;    //  use vlan to differentiate PVCs
    wrx_queue_config.vlan_ins   = 1;    //  use vlan to differentiate PVCs, please use outer VLAN in routing table, and do remember to configure switch to handle bridging case
    wrx_queue_config.mpoa_type  = 3;    //  set IPoA as default
    wrx_queue_config.ip_ver     = 0;    //  set IPv4 as default
    wrx_queue_config.mpoa_mode  = 0;    //  set VC-mux as default
    wrx_queue_config.oversize   = aal5r_max_packet_size;
    wrx_queue_config.undersize  = aal5r_min_packet_size;
    wrx_queue_config.mfs        = aal5r_max_packet_size;

    wtx_queue_config.same_vc_qmap   = 0x00;     //  only one TX queue is assigned now, use ioctl to add other TX queues
    wtx_queue_config.sbid           = g_atm_priv_data.connection[conn].port;
    wtx_queue_config.qsb_vcid       = conn + QSB_QUEUE_NUMBER_BASE; //  qsb qid = firmware qid + 1
    wtx_queue_config.mpoa_mode      = 0;        //  set VC-mux as default
    wtx_queue_config.qsben          = 1;        //  reserved in A4, however put 1 for backward compatible

    cell_header = (struct uni_cell_header *)((u32 *)&wtx_queue_config + 2);
    cell_header->clp    = (vcc->atm_options & ATM_ATMOPT_CLP) ? 1 : 0;
    cell_header->pti    = ATM_PTI_US0;
    cell_header->vci    = vci;
    cell_header->vpi    = vpi;
    cell_header->gfc    = 0;

    *WRX_QUEUE_CONFIG(conn) = wrx_queue_config;
    *WTX_QUEUE_CONFIG(sw_tx_queue) = wtx_queue_config;

    /*  set qsb */
    set_qsb(vcc, &vcc->qos, conn);

    /*  update atm_vcc structure    */
    vcc->itf = (int)vcc->dev->dev_data;
    set_bit(ATM_VF_ADDR, &vcc->flags);
    set_bit(ATM_VF_READY, &vcc->flags);

    if ( f_enable_irq )
    {
        ifx_atm_alloc_tx = atm_alloc_tx;
        // turn_on_dma_rx(31);

        g_dsl_wrx_correct_pdu = DSL_WAN_MIB_TABLE->wrx_correct_pdu;
        g_dsl_wtx_total_pdu = DSL_WAN_MIB_TABLE->wtx_total_pdu;
    }

    /*  set htu entry   */
    set_htu_entry(vpi, vci, conn, vcc->qos.aal == ATM_AAL5 ? 1 : 0);

    dbg("ppe_open(%d.%d): conn = %d", vcc->vpi, vcc->vci, conn);
    up(&g_sem);
    return 0;

PPE_OPEN_EXIT:
    up(&g_sem);
    return ret;
}

static void ppe_close(struct atm_vcc *vcc)
{
    int conn;
    struct atm_port *port;
    struct atm_connection *connection;
    int i;

    if ( vcc == NULL )
        return;

    down(&g_sem);

    /*  get connection id   */
    conn = find_vcc(vcc);
    if ( conn < 0 )
    {
        err("can't find vcc");
        goto PPE_CLOSE_EXIT;
    }
    connection = &g_atm_priv_data.connection[conn];
    port = &g_atm_priv_data.port[connection->port];

    clear_bit(ATM_VF_READY, &vcc->flags);
    clear_bit(ATM_VF_ADDR, &vcc->flags);

    /*  clear htu   */
    clear_htu_entry(conn);

    /*  release connection  */
    g_atm_priv_data.connection_table &= ~(1 << conn);
    g_atm_priv_data.sw_tx_queue_table &= ~(connection->sw_tx_queue_table);
    memset(connection, 0, sizeof(*connection));

    /*  disable irq */
    if ( g_atm_priv_data.connection_table == 0 )
    {

        turn_off_dma_rx(31);
        ifx_atm_alloc_tx = NULL;
    }

    /*  release bandwidth   */
    switch ( vcc->qos.txtp.traffic_class )
    {
    case ATM_CBR:
    case ATM_VBR_RT:
        port->tx_used_cell_rate -= vcc->qos.txtp.max_pcr;
        break;
    case ATM_VBR_NRT:
#if defined(DISABLE_VBR) && DISABLE_VBR
        port->tx_used_cell_rate -= vcc->qos.txtp.pcr;
#else
        port->tx_used_cell_rate -= vcc->qos.txtp.scr;
#endif
        break;
    case ATM_UBR_PLUS:
        port->tx_used_cell_rate -= vcc->qos.txtp.min_pcr;
        break;
    }

    /*  idle for a while to let parallel operation finish   */
    for ( i = 0; i < IDLE_CYCLE_NUMBER; i++ );

#if defined(ENABLE_CONFIGURABLE_DSL_VLAN) && ENABLE_CONFIGURABLE_DSL_VLAN
    for ( i = 0; i < NUM_ENTITY(g_dsl_vlan_qid_map); i += 2 )
        if ( g_dsl_vlan_qid_map[i] == WRX_QUEUE_CONFIG(conn)->new_vlan )
        {
            g_dsl_vlan_qid_map[i] = 0;
            break;
        }
#endif

PPE_CLOSE_EXIT:
    up(&g_sem);
}

static int ppe_send(struct atm_vcc *vcc, struct sk_buff *skb)
{
    int ret;
    int conn;
    unsigned long sys_flag;
    volatile struct tx_descriptor *desc;
    struct tx_descriptor reg_desc;
    struct sk_buff *skb_to_free;
    int byteoff;
    u32 priority;

    if ( vcc == NULL || skb == NULL )
        return -EINVAL;

    if ( skb->len > aal5s_max_packet_size )
        return -EOVERFLOW;

    ATM_SKB(skb)->vcc = vcc;

    if ( g_stop_datapath != 0 )
    {
        atm_free_tx_skb_vcc(skb);
        return -EBUSY;
    }

    skb_get(skb);
    atm_free_tx_skb_vcc(skb);
    ATM_SKB(skb)->vcc = NULL;

    conn = find_vcc(vcc);
    if ( conn < 0 )
    {
        err("not find vcc");
        ret = -EINVAL;
        goto FIND_VCC_FAIL;
    }

    if ( !g_showtime )
    {
        err("not in showtime");
        ret = -EIO;
        goto CHECK_SHOWTIME_FAIL;
    }

#if (defined(ENABLE_DSL_LOOPBACK_TEST) && ENABLE_DSL_LOOPBACK_TEST) || (defined(ENABLE_DFE_LOOPBACK_TEST) && ENABLE_DFE_LOOPBACK_TEST)
    {
        unsigned char mac[6];
        unsigned char mac_broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        unsigned char ip[4];
        int off = 0;

        dump_skb(skb, DUMP_SKB_LEN, "ppe_send - before swap", 7, 0, 1, 0);

        switch ( WRX_QUEUE_CONFIG(conn)->mpoa_type )
        {
        case 0: //  EoA w/o FCS
        case 1: //  EoA w FCS
            if ( memcmp(skb->data + 10, mac_broadcast, 6) == 0 )
                break;
            memcpy(mac, skb->data + 10, 6);
            memcpy(skb->data + 10, skb->data + 16, 6);
            memcpy(skb->data + 16, mac, 6);
            off = 22;
            while ( skb->data[off] == 0x81 && skb->data[off + 1] == 0x00 )  //  VLAN
                off += 4;
            off -= 8;
        case 3: //  IPoA
            off += 8;
            if ( skb->data[off] == 0x08 && skb->data[off + 1] == 0x00 ) //  IP
            {
                off += 14;
                memcpy(ip, skb->data + off, 4);
                memcpy(skb->data + off, skb->data + off + 4, 4);
                memcpy(skb->data + off + 4, ip, 4);
            }
            break;
        }
  #if (defined(ENABLE_DSL_LOOPBACK_TEST) && ENABLE_DSL_LOOPBACK_TEST)
        dump_skb(skb, DUMP_SKB_LEN, "DSL Loopback", (WRX_QUEUE_CONFIG(conn)->mpoa_type & 0x02) ? 7 : 6, 0, 0, 0);

        g_atm_priv_data.connection[conn].access_time = current_kernel_time();

        adsl_led_flash();

        vcc->push(vcc, skb);

        if ( vcc->stats )
            atomic_inc(&vcc->stats->rx);

        g_atm_priv_data.wrx_pdu++;

        UPDATE_VCC_STAT(conn, rx_packets, 1);
        UPDATE_VCC_STAT(conn, rx_bytes, skb->len);

        return 0;
  #endif
    }
#endif

    priority = (skb->priority & TC_H_MIN_MASK);
    if ( priority >= 8 )
        priority = 7;

    /*  reserve space to put pointer in skb */
    byteoff = (u32)skb->data & (DMA_TX_ALIGNMENT - 1);
    if ( skb_headroom(skb) < sizeof(struct sk_buff *) + byteoff || skb->end - skb->data < 1564 /* 1518 (max ETH frame) + 22 (reserved for firmware) + 10 (AAL5) + 6 (PPPoE) + 4 x 2 (2 VLAN) */ || skb_shared(skb) || skb_cloned(skb) )
    {
        //  this should be few case

        struct sk_buff *new_skb;

		dma_alignment_atm_bad_count++;

        //dbg("skb_headroom(skb) < sizeof(struct sk_buff *) + byteoff");
        ASSERT(skb_headroom(skb) >= sizeof(struct sk_buff *) + byteoff, "skb_headroom(skb) < sizeof(struct sk_buff *) + byteoff");
        ASSERT(skb->end - skb->data >= 1564, "skb->end - skb->data < 1518 + 22 + 10 + 6 + 4 x 2");

#if defined(CONFIG_AVMNET_DEBUG) 
		if ( g_dbg_datapath & DBG_ENABLE_MASK_DEBUG_ALIGNMENT )
			printk( KERN_ERR
				"\n"
				"   skb_headroom :   %d \n"
				"   skb_tailroom :   %d \n"
				"   skb_len      :   %d \n"
				"   skb_total    :   %d \n"
				"   sizeof(struct sk_buff *): %d \n"
				"   byteoff:         %d \n"
				"   skb->head:       %p\n"
				"   skb->data:       %p\n"
				"   in_dev:          %s\n",
					skb_headroom(skb),
					skb_tailroom(skb),
					skb->len,
					skb->end - skb->head,
					sizeof(struct sk_buff *),
					byteoff,
					skb->head,
					skb->data,
					skb->input_dev?skb->input_dev->name:"no_input_device");
#endif

        new_skb = alloc_skb_tx(skb->len < DMA_PACKET_SIZE ? DMA_PACKET_SIZE : skb->len);    //  may be used by RX fastpath later
        if ( !new_skb )
        {
            err("no memory");
            ret = -ENOMEM;
            goto ALLOC_SKB_TX_FAIL;
        }
        skb_put(new_skb, skb->len);
        my_memcpy(new_skb->data, skb->data, skb->len);
        atm_free_tx_skb_vcc(skb);
		/* AVMGRM: priority wegen QoS auch kopieren */
		new_skb->priority = skb->priority;
        skb = new_skb;
        byteoff = (u32)skb->data & (DMA_TX_ALIGNMENT - 1);
#ifndef CONFIG_MIPS_UNCACHED
        /*  write back to physical memory   */
        dma_cache_wback((u32)skb->data, skb->len);
#endif
    }
    else
    {
	    skb_orphan(skb);
        *(struct sk_buff **)((u32)skb->data - byteoff - sizeof(struct sk_buff *)) = skb;
#ifndef CONFIG_MIPS_UNCACHED
        /*  write back to physical memory   */
        dma_cache_wback((u32)skb->data - byteoff - sizeof(struct sk_buff *), skb->len + byteoff + sizeof(struct sk_buff *));
#endif
		dma_alignment_atm_good_count++;
#if defined(CONFIG_AVMNET_DEBUG) 
		if ( g_dbg_datapath & DBG_ENABLE_MASK_DEBUG_ALIGNMENT )
			printk( KERN_ERR "len ok: total_skb=%d\n", skb->end - skb->head );
#endif
    }

    /*  allocate descriptor */
    spin_lock_irqsave(&g_cpu_to_wan_tx_spinlock, sys_flag);
    desc = CPU_TO_WAN_TX_DESC_BASE + g_cpu_to_wan_tx_desc_pos;
    if ( !desc->own )    //  PPE hold
    {
        spin_unlock_irqrestore(&g_cpu_to_wan_tx_spinlock, sys_flag);
        ret = -EIO;
        err("PPE hold");
        goto NO_FREE_DESC;
    }
    if ( ++g_cpu_to_wan_tx_desc_pos == CPU_TO_WAN_TX_DESC_NUM )
        g_cpu_to_wan_tx_desc_pos = 0;
    spin_unlock_irqrestore(&g_cpu_to_wan_tx_spinlock, sys_flag);

    /*  load descriptor from memory */
    reg_desc = *desc;

    /*  free previous skb   */
    ASSERT((reg_desc.dataptr & (DMA_TX_ALIGNMENT - 1)) == 0, "reg_desc.dataptr (0x%#x) must be 8 DWORDS aligned", reg_desc.dataptr);
	free_skb_clear_dataptr(&reg_desc);

    /*  detach from protocol    */
    skb_to_free = skb;

    if( (skb = skb_break_away_from_protocol_avm(skb)) == NULL) {
        skb = skb_to_free;
		goto ALLOC_SKB_TX_FAIL;
    }

    put_skb_to_dbg_pool(skb);

#ifdef CONFIG_IFX_PORT_MIRROR
    if ( ifx_get_mirror_netdev && ifx_get_mirror_netdev() != NULL )
        mirror_pkt_atm(skb, conn);
#endif

    /*  update descriptor   */
    reg_desc.dataptr    = (u32)skb->data & (0x1FFFFFFF ^ (DMA_TX_ALIGNMENT - 1));  //  byte address
    reg_desc.byteoff    = byteoff;
    reg_desc.datalen    = skb->len;
    reg_desc.mpoa_pt    = 1;
    reg_desc.mpoa_type  = 0;    //  this flag should be ignored by firmware
    reg_desc.pdu_type   = vcc->qos.aal == ATM_AAL5 ? 0 : 1;
    reg_desc.qid        = g_atm_priv_data.connection[conn].prio_queue_map[priority];
    reg_desc.own        = 0;
    reg_desc.c          = 0;
    reg_desc.sop = reg_desc.eop = 1;

    /*  update MIB  */
    UPDATE_VCC_STAT(conn, tx_packets, 1);
    UPDATE_VCC_STAT(conn, tx_bytes, skb->len);

    if ( vcc->stats )
        atomic_inc(&vcc->stats->tx);
    if ( vcc->qos.aal == ATM_AAL5 )
        g_atm_priv_data.wtx_pdu++;

    g_atm_priv_data.connection[conn].prio_tx_packets[priority]++;

    dump_atm_tx_desc( &reg_desc );
#if defined(CONFIG_AVMNET_DEBUG) 
	if (g_dbg_datapath & DBG_ENABLE_MASK_DUMP_ATM_TX){
		dump_skb(skb, DUMP_SKB_LEN, "ppe_send", 7, reg_desc.qid, 1, 1);
	}
#endif

    /*  write discriptor to memory and write back cache */
    *((volatile u32 *)desc + 1) = *((u32 *)&reg_desc + 1);
    *(volatile u32 *)desc = *(u32 *)&reg_desc;

    adsl_led_flash();

    return 0;

FIND_VCC_FAIL:
    if ( vcc->stats )
    {
        atomic_inc(&vcc->stats->tx_err);
    }
    if ( vcc->qos.aal == ATM_AAL5 )
    {
        g_atm_priv_data.wtx_err_pdu++;
        g_atm_priv_data.wtx_drop_pdu++;
    }
    atm_free_tx_skb_vcc(skb);
    return ret;

CHECK_SHOWTIME_FAIL:
ALLOC_SKB_TX_FAIL:
    if ( vcc->stats )
        atomic_inc(&vcc->stats->tx_err);
    if ( vcc->qos.aal == ATM_AAL5 )
        g_atm_priv_data.wtx_drop_pdu++;
    UPDATE_VCC_STAT(conn, tx_dropped, 1);
    atm_free_tx_skb_vcc(skb);
    return ret;

NO_FREE_DESC:
    if ( vcc->stats )
        atomic_inc(&vcc->stats->tx_err);
    if ( vcc->qos.aal == ATM_AAL5 )
        g_atm_priv_data.wtx_drop_pdu++;
    UPDATE_VCC_STAT(conn, tx_dropped, 1);
    atm_free_tx_skb_vcc(skb);
    return ret;
}

static int ppe_send_oam(struct atm_vcc *vcc, void *cell, int flags)
{
    int conn;
    struct uni_cell_header *uni_cell_header = (struct uni_cell_header *)cell;
    struct sk_buff *skb;
    unsigned long sys_flag;
    volatile struct tx_descriptor *desc;
    struct tx_descriptor reg_desc;
    struct sk_buff *skb_to_free;

    if ( ((uni_cell_header->pti == ATM_PTI_SEGF5 || uni_cell_header->pti == ATM_PTI_E2EF5)
        && find_vpivci(uni_cell_header->vpi, uni_cell_header->vci) < 0)
        || ((uni_cell_header->vci == 0x03 || uni_cell_header->vci == 0x04)
        && find_vpi(uni_cell_header->vpi) < 0) )
        return -EINVAL;

    if ( !g_showtime )
    {
        err("not in showtime");
        return -EIO;
    }

    /*  find queue ID   */
    conn = find_vcc(vcc);
    if ( conn < 0 )
    {
        err("OAM not find queue");
        return -EINVAL;
    }

    /*  allocate sk_buff    */
    skb = alloc_skb_tx(DMA_PACKET_SIZE);    //  may be used by RX fastpath later
    if ( skb == NULL )
        return -ENOMEM;

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
    if ( !desc->own )    //  PPE hold
    {
        spin_unlock_irqrestore(&g_cpu_to_wan_tx_spinlock, sys_flag);
        err("OAM not alloc tx connection");
        return -EIO;
    }
    if ( ++g_cpu_to_wan_tx_desc_pos == CPU_TO_WAN_TX_DESC_NUM )
        g_cpu_to_wan_tx_desc_pos = 0;
    spin_unlock_irqrestore(&g_cpu_to_wan_tx_spinlock, sys_flag);

    /*  load descriptor from memory */
    reg_desc = *desc;

    /*  free previous skb   */
    ASSERT((reg_desc.dataptr & (DMA_TX_ALIGNMENT - 1)) == 0, "reg_desc.dataptr (0x%#x) must be 8 DWORDS aligned", reg_desc.dataptr);

	free_skb_clear_dataptr(&reg_desc);
    put_skb_to_dbg_pool(skb);

    /*  setup descriptor    */
    reg_desc.dataptr    = (u32)skb->data & (0x1FFFFFFF ^ (DMA_TX_ALIGNMENT - 1));  //  byte address
    reg_desc.byteoff    = (u32)skb->data & (DMA_TX_ALIGNMENT - 1);
    reg_desc.datalen    = skb->len;
    reg_desc.mpoa_pt    = 1;
    reg_desc.mpoa_type  = 0;    //  this flag should be ignored by firmware
    reg_desc.pdu_type   = 1;    //  cell
    reg_desc.qid        = g_atm_priv_data.connection[conn].prio_queue_map[7];   //  expect priority '7' is highest priority
    reg_desc.own        = 0;
    reg_desc.c          = 0;
    reg_desc.sop = reg_desc.eop = 1;

    dump_skb(skb, DUMP_SKB_LEN, "ppe_send_oam", 7, reg_desc.qid, 1, 0);

    /*  write discriptor to memory and write back cache */
    *((volatile u32 *)desc + 1) = *((u32 *)&reg_desc + 1);
    *(volatile u32 *)desc = *(u32 *)&reg_desc;

    adsl_led_flash();

    return 0;
}

static int ppe_change_qos(struct atm_vcc *vcc, struct atm_qos *qos, int flags)
{
    int conn;

    if ( vcc == NULL || qos == NULL )
        return -EINVAL;

    conn = find_vcc(vcc);
    if ( conn < 0 )
        return -EINVAL;

    set_qsb(vcc, qos, conn);

    return 0;
}

#ifdef AVMNET_VCC_QOS

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

/*
* returns res:
*   res == 0: queue setup successful
*   res <  0: setup qos queue failure
*/
static int vcc_set_nr_qos_queues_datapath(struct atm_vcc *vcc, unsigned int nr_queues){
	int conn;
	unsigned int new_queue_count = 1;
	int res;

	if (vcc == NULL )
		return -EINVAL;

	conn = find_vcc(vcc);
	if (conn < 0)
		return -EINVAL;
    while( new_queue_count < nr_queues ){

	    // add lower priority queue
        res = sw_tx_queue_add(conn);
	    if (res != 0) {
	        // something went wrong: delete all the queues, we already added
            while( --new_queue_count ){
                // del lowest priority queue
                sw_tx_queue_del(conn);

            }
            return -EINVAL;
	    }
	    ++new_queue_count;
    }

    return 0;
}


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
/*
* returns ret:
*   ret == 0: success
*   ret != 0: failure
*/
int vcc_map_skb_prio_qos_queue_datapath(struct atm_vcc *vcc, unsigned int skb_prio, unsigned int qid){
	int conn;

	if (vcc == NULL )
		return -EINVAL;
	conn = find_vcc(vcc);
	if (conn < 0)
		return -EINVAL;

    return sw_tx_prio_to_queue(conn, skb_prio, qid, NULL);
}

#endif

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/


static void mpoa_setup_conn(int conn, int mpoa_type, int f_llc) {

	u32 sw_tx_queue_table;
	int sw_tx_queue;

	printk(KERN_ERR "%s conn=%d, mpoa_type=%d, mpoa_mode=%d\n ", __func__, conn, mpoa_type, f_llc);

	WRX_QUEUE_CONFIG(conn)->mpoa_type = mpoa_type; //mpoa_type = 0: funktionierts im AVM Netz
	WRX_QUEUE_CONFIG(conn)->mpoa_mode = f_llc;

	for (sw_tx_queue_table = g_atm_priv_data.connection[conn].sw_tx_queue_table, sw_tx_queue =
			0; sw_tx_queue_table != 0; sw_tx_queue_table >>= 1, sw_tx_queue++)
		if ((sw_tx_queue_table & 0x01))
			WTX_QUEUE_CONFIG(sw_tx_queue)->mpoa_mode = f_llc;
}

static void mpoa_setup(struct atm_vcc *vcc, int mpoa_type, int f_llc) { //todo geeignete Aufrufparameter im kdsld waeheln (bei uns funktioniert (vcc, 0, 1)
	int conn;

	printk(KERN_ERR "%s vcc=%p, mpoa_type=%d, mpoa_mode=%d\n ", __func__, vcc, mpoa_type, f_llc);
	conn = find_vcc(vcc);
	if (conn < 0)
		return;

	mpoa_setup_conn(conn, mpoa_type, f_llc);
}


int ifx_ppa_eth_hard_start_xmit_a5(struct sk_buff *skb, struct net_device *dev)
{
    int port;
    int qid;

    port = get_eth_port(dev);
    if ( port < 0 )
        return -ENODEV;

    if ( skb->priority >= NUM_ENTITY(g_eth_prio_queue_map) )
        qid = g_eth_prio_queue_map[NUM_ENTITY(g_eth_prio_queue_map) - 1];
    else
        qid = g_eth_prio_queue_map[skb->priority];

    eth_xmit(skb, port, DMA_CHAN_NR_DEFAULT_CPU_TX, 2, g_wan_queue_class_map[qid]); //  spid - CPU, class - 2 layer map, 1st: taken from prio_queue_map, 2nd: taken from wan_queue_class_map

    return 0;
}

int ifx_ppa_eth_qos_hard_start_xmit_a5(struct sk_buff *skb, struct net_device *dev)
{
    int qid;
    unsigned long sys_flag;
    volatile struct eth_tx_descriptor *desc;
    struct eth_tx_descriptor reg_desc;
    struct sk_buff *skb_to_free;
    int byteoff;
    int len;
    int if_id;
    struct sw_eg_pkt_header pkth = {0};

	avmnet_netdev_priv_t *priv = NULL;
	avmnet_device_t *avm_dev;

	if_id = get_eth_port(dev);
	if (if_id < 0 || if_id >= num_ar9_eth_devs ) {
		printk("%s: Improper ethernet device\n", __FUNCTION__);
		return -ENODEV;
	}

	priv = netdev_priv(dev);
	avm_dev = priv->avm_dev;

    if ( g_stop_datapath != 0 )
        goto ETH_XMIT_DROP;

    /*
     * AVM/CBU: JZ-34572 Zero-padding for small packets
     */
    len = skb->len;
    if (skb->len < ETH_MIN_TX_PACKET_LENGTH ){
	    if ( skb_pad(skb, ETH_MIN_TX_PACKET_LENGTH - skb->len )){
		    skb = NULL;
		    goto ETH_PAD_DROP;
	    }
	    len = ETH_MIN_TX_PACKET_LENGTH;
	    skb->len = len;
    }

    if ( skb->cb[13] == 0x5A )  //  magic number indicating directpath
	    qid = skb->cb[15];
    else
    {
		if ( skb->priority >= NUM_ENTITY(g_eth_prio_queue_map) )
		    qid = g_eth_prio_queue_map[NUM_ENTITY(g_eth_prio_queue_map) - 1];
		else
		    qid = g_eth_prio_queue_map[skb->priority];
	}

    pkth.spid   = 2;    //  CPU
    pkth.dpid   = if_id; //  eth0/eth1
    pkth.qid    = g_wan_queue_class_map[qid];       //  0 is lowest priority, 3 is highest priority
    pkth.direct = g_egress_direct_forward_enable;   //  enable/disable direct forward according to flag, need re-consideration

    BUG_ON(len < ETH_MIN_TX_PACKET_LENGTH); 

    dump_skb(skb, DUMP_SKB_LEN, "eth_qos_hard_start_xmit", if_id, qid, 1, 0);

    /*  reserve space to put pointer in skb */
    byteoff = (((unsigned int)skb->data - sizeof(pkth)) & (DMA_TX_ALIGNMENT - 1)) + sizeof(pkth);
    //if ( skb_headroom(skb) < sizeof(struct sk_buff *) + byteoff || skb->end - skb->data < 1564 /* 1518 (max ETH frame) + 22 (reserved for firmware) + 10 (AAL5) + 6 (PPPoE) + 4 x 2 (2 VLAN) */ || skb_shared(skb) || skb_cloned(skb) )
    if ( skb_headroom(skb) < sizeof(struct sk_buff *) + byteoff || skb_shared(skb) || skb_cloned(skb) ||
            (unsigned int)skb->end - (unsigned int)skb->data < DMA_PACKET_SIZE  )
    {
        struct sk_buff *new_skb;

        ASSERT(skb_headroom(skb) >= sizeof(struct sk_buff *) + byteoff, "skb_headroom(skb) < sizeof(struct sk_buff *) + byteoff");
		dma_alignment_eth_bad_count++;
        new_skb = alloc_skb_tx(sizeof(pkth) + skb->len < DMA_PACKET_SIZE ? DMA_PACKET_SIZE : sizeof(pkth) + skb->len);  //  may be used by RX fastpath later
        if ( !new_skb )
        {
            err("no memory");
            goto ALLOC_SKB_TX_FAIL;
        }
        skb_put(new_skb, sizeof(pkth) + skb->len);
        my_memcpy(new_skb->data, &pkth, sizeof(pkth));
        my_memcpy(new_skb->data + sizeof(pkth), skb->data, skb->len);
        dev_kfree_skb_any(skb);
        skb = new_skb;
        byteoff = (u32)skb->data & (DMA_TX_ALIGNMENT - 1);
#ifndef CONFIG_MIPS_UNCACHED
        /*  write back to physical memory   */
        dma_cache_wback((u32)skb->data, skb->len);
#endif
    }
    else
    {
	dma_alignment_eth_good_count++;

        skb_push(skb, sizeof(pkth));
        byteoff -= sizeof(pkth);
        my_memcpy(skb->data, &pkth, sizeof(pkth));
        *(struct sk_buff **)((u32)skb->data - byteoff - sizeof(struct sk_buff *)) = skb;
#ifndef CONFIG_MIPS_UNCACHED
        /*  write back to physical memory   */
        dma_cache_wback((u32)skb->data - byteoff - sizeof(struct sk_buff *), skb->len + byteoff + sizeof(struct sk_buff *));
#endif
    }

    /*  allocate descriptor */
    spin_lock_irqsave(&g_cpu_to_wan_tx_spinlock, sys_flag);
    desc = (volatile struct eth_tx_descriptor *)(CPU_TO_WAN_TX_DESC_BASE + g_cpu_to_wan_tx_desc_pos);
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
    ASSERT((reg_desc.dataptr & (DMA_TX_ALIGNMENT - 1)) == 0, "reg_desc.dataptr (0x%#x) must be 8 DWORDS aligned", reg_desc.dataptr);

    free_skb_clear_dataptr(desc);

    /*  detach from protocol    */
    skb_to_free = skb;
    if( (skb = skb_break_away_from_protocol_avm(skb)) == NULL) {
        skb = skb_to_free;
		goto ALLOC_SKB_TX_FAIL;
    }

    put_skb_to_dbg_pool(skb);

#ifdef CONFIG_IFX_PORT_MIRROR
    if ( ifx_get_mirror_netdev && ifx_get_mirror_netdev() != NULL )
        mirror_pkt_eth(skb, conn);
#endif
    dump_eg_pkt_header(&pkth);

    /*  update descriptor   */
    reg_desc.small      = (unsigned int)skb->end - (unsigned int)skb->data < DMA_PACKET_SIZE ? 1 : 0;
    reg_desc.dataptr    = (u32)skb->data & (0x1FFFFFFF ^ (DMA_TX_ALIGNMENT - 1));  //  byte address
    reg_desc.byteoff    = byteoff;
    reg_desc.datalen    = len + sizeof(pkth);
    reg_desc.qid        = qid;
    reg_desc.own        = 1;
    reg_desc.c          = 0;
    reg_desc.sop = reg_desc.eop = 1;

    /*  update MIB  */
    dev->trans_start = jiffies;

	avm_dev->device_stats.tx_packets++;
	avm_dev->device_stats.tx_bytes += len;

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

int ifx_ppa_eth_ioctl_a5(struct net_device *dev, struct ifreq *ifr, int cmd)
{
    int port;

    port = get_eth_port(dev);
    if ( port < 0 )
        return -ENODEV;

    switch ( cmd )
    {
    case SIOCETHTOOL:
        return ethtool_ioctl(dev, ifr);
    case SET_VLAN_COS:
        {
            struct vlan_cos_req vlan_cos_req;

            if ( copy_from_user(&vlan_cos_req, ifr->ifr_data, sizeof(struct vlan_cos_req)) )
                return -EFAULT;
#if 0   //  TODO
            set_vlan_cos(&vlan_cos_req);
#endif
        }
        break;
    case SET_DSCP_COS:
        {
            struct dscp_cos_req dscp_cos_req;

            if ( copy_from_user(&dscp_cos_req, ifr->ifr_data, sizeof(struct dscp_cos_req)) )
                return -EFAULT;
#if 0   //  TODO
            set_dscp_cos(&dscp_cos_req);
#endif
        }
        break;
#if 0   //  TODO
    case ENABLE_VLAN_CLASSIFICATION:
        *ENETS_COS_CFG(port) |= ENETS_COS_CFG_VLAN_SET;     break;
    case DISABLE_VLAN_CLASSIFICATION:
        *ENETS_COS_CFG(port) &= ENETS_COS_CFG_VLAN_CLEAR;   break;
    case ENABLE_DSCP_CLASSIFICATION:
        *ENETS_COS_CFG(port) |= ENETS_COS_CFG_DSCP_SET;     break;
    case DISABLE_DSCP_CLASSIFICATION:
        *ENETS_COS_CFG(port) &= ENETS_COS_CFG_DSCP_CLEAR;   break;
    case VLAN_CLASS_FIRST:
        *ENETS_CFG(port) &= ENETS_CFG_FTUC_CLEAR;           break;
    case VLAN_CLASS_SECOND:
        *ENETS_CFG(port) |= ENETS_CFG_VL2_SET;              break;
    case PASS_UNICAST_PACKETS:
        *ENETS_CFG(port) &= ENETS_CFG_FTUC_CLEAR;           break;
    case FILTER_UNICAST_PACKETS:
        *ENETS_CFG(port) |= ENETS_CFG_FTUC_SET;             break;
    case KEEP_BROADCAST_PACKETS:
        *ENETS_CFG(port) &= ENETS_CFG_DPBC_CLEAR;           break;
    case DROP_BROADCAST_PACKETS:
        *ENETS_CFG(port) |= ENETS_CFG_DPBC_SET;             break;
    case KEEP_MULTICAST_PACKETS:
        *ENETS_CFG(port) &= ENETS_CFG_DPMC_CLEAR;           break;
    case DROP_MULTICAST_PACKETS:
        *ENETS_CFG(port) |= ENETS_CFG_DPMC_SET;             break;
#endif
    case ETH_MAP_PKT_PRIO_TO_Q:
        {
            struct ppe_prio_q_map cmd;

            if ( copy_from_user(&cmd, ifr->ifr_data, sizeof(cmd)) )
                return -EFAULT;

            if ( cmd.pkt_prio < 0 || cmd.pkt_prio >= NUM_ENTITY(g_eth_prio_queue_map) )
                return -EINVAL;

            if ( cmd.qid < 0 )
                return -EINVAL;
            if ( cmd.qid >= ((g_wan_itf & (1 << port)) && (g_eth_wan_mode == 3) && g_wanqos_en ? __ETH_WAN_TX_QUEUE_NUM : __ETH_VIRTUAL_TX_QUEUE_NUM) )
                return -EINVAL;

            g_eth_prio_queue_map[cmd.pkt_prio] = cmd.qid;
        }
        break;
    default:
        return -EOPNOTSUPP;
    }

    return 0;
}


void ifx_ppa_eth_tx_timeout_a5(struct net_device *dev) {
    unsigned long sys_flag;

	avmnet_netdev_priv_t *priv = netdev_priv(dev);
	avmnet_device_t *avm_dev = NULL;

    if (priv)
    	avm_dev = priv->avm_dev;

    if (avm_dev)
        avm_dev->device_stats.tx_errors++;

    spin_lock_irqsave(&g_eth_tx_spinlock, sys_flag);
    g_dma_device_ppe->tx_chan[2]->disable_irq(g_dma_device_ppe->tx_chan[2]);
    netif_wake_queue(g_eth_net_dev[0]);
    if ( !g_wanqos_en )
        netif_wake_queue(g_eth_net_dev[1]);
    spin_unlock_irqrestore(&g_eth_tx_spinlock, sys_flag);

    return;
}

static INLINE int eth_xmit(struct sk_buff *skb, unsigned int dpid, int ch,
		int spid __attribute__((unused)), int class __attribute__((unused)) ) {
    struct sw_eg_pkt_header pkth = {spid:   spid,   //  CPU/app0/app1/app2
                                    dpid:   dpid,   //  eth0/eth1/app0/app1/app2
                                    qid:    class,   //  0 is lowest priority, 3 is highest priority
                                    direct: dpid < 2 && spid <= 2 ? g_egress_direct_forward_enable : 0};
    								//  enable/disable direct forward according to flag, need re-consideration
    int len;
	avmnet_netdev_priv_t *priv = NULL;
	avmnet_device_t *avm_dev = NULL;

    if ( g_stop_datapath != 0 )
        goto ETH_XMIT_DROP;

    len = skb->len;
    if (skb->len < ETH_MIN_TX_PACKET_LENGTH ){
	    if ( skb_pad(skb, ETH_MIN_TX_PACKET_LENGTH - skb->len)){
		    skb = NULL;
		    goto ETH_PAD_DROP;
	    }
	    len = ETH_MIN_TX_PACKET_LENGTH;
	    skb->len = len;
    }

#ifdef CONFIG_IFX_PORT_MIRROR
    if ( (g_wan_itf & (1 << port)) && ifx_get_mirror_netdev && ifx_get_mirror_netdev() != NULL )
        mirror_pkt_eth(skb, 0);
#endif

//    g_dma_device_ppe->current_tx_chan = ch;

    dump_skb(skb, DUMP_SKB_LEN, "eth_xmit - raw data", dpid, ch, 1, 0);

    if ( skb_headroom(skb) < sizeof(struct sw_eg_pkt_header) )
    {
        struct sk_buff *new_skb;

		dma_alignment_eth_bad_count++;
        new_skb = alloc_skb_tx(skb->len + sizeof(struct sw_eg_pkt_header));
        if ( !new_skb )
            goto ETH_XMIT_DROP;
        else
        {
            skb_put(new_skb, skb->len + sizeof(struct sw_eg_pkt_header));
            memcpy(new_skb->data + sizeof(struct sw_eg_pkt_header), skb->data, skb->len);
            dev_kfree_skb_any(skb);
            skb = new_skb;
        }
    }
    else{
		dma_alignment_eth_good_count++;
        skb_push(skb, sizeof(struct sw_eg_pkt_header));
		skb_orphan( skb );
    }

    len += sizeof(struct sw_eg_pkt_header);
//    PPA_PORT_SEPARATION_TX(skb, pkth, lan_port_seperate_enabled, wan_port_seperate_enabled, port );
    //TODO hier ggf anpassen an AVM-Konzept
    memcpy(skb->data, &pkth, sizeof(struct sw_eg_pkt_header));

    dump_eg_pkt_header(&pkth);

    if ( dpid < num_ar9_eth_devs)
    {
        g_eth_net_dev[dpid]->trans_start = jiffies;
		priv = netdev_priv(g_eth_net_dev[dpid]);

		avm_dev = priv->avm_dev;
		avm_dev->device_stats.tx_packets++;
		avm_dev->device_stats.tx_bytes += len;
    }

    put_skb_to_dbg_pool(skb);
    if(unlikely(dma_device_write(g_dma_device_ppe, skb->data, len, skb, ch) != len)){
        goto ETH_XMIT_DROP;
    }

    return 0;

ETH_XMIT_DROP:
    dbg("ETH_XMIT_DROP");
    dev_kfree_skb_any(skb);

ETH_PAD_DROP:
    if (priv && priv->avm_dev) {
	    avm_dev->device_stats.tx_dropped++;
    }
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
static INLINE int ethtool_ioctl(struct net_device *dev, struct ifreq *ifr)
{
    int port;
    struct ethtool_cmd cmd;

    if ( copy_from_user(&cmd, ifr->ifr_data, sizeof(cmd)) )
        return -EFAULT;

    port = get_eth_port(dev);

    switch ( cmd.cmd )
    {
    case ETHTOOL_GSET:      /*  get hardware information        */
        {
            memset(&cmd, 0, sizeof(cmd));

#if 0   //  TODO
            cmd.supported   = SUPPORTED_Autoneg | SUPPORTED_TP | SUPPORTED_MII |
                              SUPPORTED_10baseT_Half | SUPPORTED_10baseT_Full |
                              SUPPORTED_100baseT_Half | SUPPORTED_100baseT_Full;
            cmd.port        = PORT_MII;
            cmd.transceiver = XCVR_EXTERNAL;
            cmd.phy_address = port;
            cmd.speed       = ENET_MAC_CFG_SPEED(port) ? SPEED_100 : SPEED_10;
            cmd.duplex      = ENET_MAC_CFG_DUPLEX(port) ? DUPLEX_FULL : DUPLEX_HALF;

            if ( (*ETOP_MDIO_CFG & ETOP_MDIO_CFG_UMM(port, 1)) )
            {
                /*  auto negotiate  */
                cmd.autoneg = AUTONEG_ENABLE;
                cmd.advertising |= ADVERTISED_10baseT_Half | ADVERTISED_10baseT_Full |
                                   ADVERTISED_100baseT_Half | ADVERTISED_100baseT_Full;
            }
            else
            {
                cmd.autoneg = AUTONEG_DISABLE;
                cmd.advertising &= ~(ADVERTISED_10baseT_Half | ADVERTISED_10baseT_Full |
                                     ADVERTISED_100baseT_Half | ADVERTISED_100baseT_Full);
            }
#endif

            if ( copy_to_user(ifr->ifr_data, &cmd, sizeof(cmd)) )
                return -EFAULT;
        }
        break;
    case ETHTOOL_SSET:      /*  force the speed and duplex mode */
        {
            if ( !capable(CAP_NET_ADMIN) )
                return -EPERM;

            if ( cmd.autoneg == AUTONEG_ENABLE )
            {
                /*  set property and start autonegotiation                                  */
                /*  have to set mdio advertisement register and restart autonegotiation     */
                /*  which is a very rare case, put it to future development if necessary.   */
            }
            else
            {
#if 0   //  TODO
                /*  set property without autonegotiation    */
                *ETOP_MDIO_CFG &= ~ETOP_MDIO_CFG_UMM(port, 1);

                /*  set speed   */
                if ( cmd.speed == SPEED_10 )
                    ENET_MAC_CFG_SPEED_10M(port);
                else if ( cmd.speed == SPEED_100 )
                    ENET_MAC_CFG_SPEED_100M(port);

                /*  set duplex  */
                if ( cmd.duplex == DUPLEX_HALF )
                    ENET_MAC_CFG_DUPLEX_HALF(port);
                else if ( cmd.duplex == DUPLEX_FULL )
                    ENET_MAC_CFG_DUPLEX_FULL(port);

                ENET_MAC_CFG_LINK_OK(port);
#endif
            }
        }
        break;
    case ETHTOOL_GDRVINFO:  /*  get driver information          */
        {
            struct ethtool_drvinfo info;
            char str[32];

            memset(&info, 0, sizeof(info));
            strncpy(info.driver, "Danube Eth Driver (A4)", sizeof(info.driver) - 1);
            sprintf(str, "%d.%d.%d.%d.%d", (int)FW_VER_ID->family, (int)FW_VER_ID->package, (int)FW_VER_ID->major, (int)FW_VER_ID->middle, (int)FW_VER_ID->minor);
            strncpy(info.fw_version, str, sizeof(info.fw_version) - 1);
            strncpy(info.bus_info, "N/A", sizeof(info.bus_info) - 1);
            info.regdump_len = 0;
            info.eedump_len = 0;
            info.testinfo_len = 0;
            if ( copy_to_user(ifr->ifr_data, &info, sizeof(info)) )
                return -EFAULT;
        }
        break;
    case ETHTOOL_NWAY_RST:  /*  restart auto negotiation        */
#if 0   //  TODO
        *ETOP_MDIO_CFG |= ETOP_MDIO_CFG_SMRST(port) | ETOP_MDIO_CFG_UMM(port, 1);
#endif
        break;
    default:
        return -EOPNOTSUPP;
    }

    return 0;
}

#if 0   //  TODO
/*
 *  Description:
 *    Specify ETOP ingress VLAN priority's class of service mapping.
 *  Input:
 *    req --- struct vlan_cos_req *, pass parameters such as priority and class
 *            of service mapping.
 *  Output:
 *    none
 */
static INLINE void set_vlan_cos(struct vlan_cos_req *req)
{
    *ETOP_IG_VLAN_COS = SET_BITS(*ETOP_IG_VLAN_COS, (req->pri << 1) + 1, req->pri << 1, req->cos_value);
}

/*
 *  Description:
 *    Specify ETOP ingress VLAN differential service control protocol's class of
 *    service mapping.
 *  Input:
 *    req --- struct dscp_cos_req *, pass parameters such as differential
 *            service control protocol and class of service mapping.
 *  Output:
 *    none
 */
static INLINE void set_dscp_cos(struct dscp_cos_req *req)
{
    *ETOP_IG_DSCP_COSx(req->dscp >> 4) = SET_BITS(*ETOP_IG_DSCP_COSx(req->dscp >> 4), ((req->dscp & 0x0F) << 1) + 1, (req->dscp & 0x0F) << 1, req->cos_value);
}
#endif

/*
 *  Description:
 *    Add a 32-bit value to 64-bit value, and put result in a 64-bit variable.
 *  Input:
 *    opt1 --- ppe_u64_t, first operand, a 64-bit unsigned integer value
 *    opt2 --- u32, second operand, a 32-bit unsigned integer value
 *    ret  --- ppe_u64_t, pointer to a variable to hold result
 *  Output:
 *    none
 */
static INLINE void u64_add_u32(ppe_u64_t opt1, u32 opt2,ppe_u64_t *ret)
{
    ret->l = opt1.l + opt2;
    if ( ret->l < opt1.l || ret->l < opt2 )
        ret->h++;
}

static INLINE void adsl_led_flash(void)
{
#if defined(ENABLE_LED_FRAMEWORK) && ENABLE_LED_FRAMEWORK
  #ifdef CONFIG_IFX_LED
    if ( g_data_led_trigger != NULL )
        ifx_led_trigger_activate(g_data_led_trigger);
  #endif
#else
	if (!IS_ERR(&ifx_mei_atm_led_blink) && &ifx_mei_atm_led_blink)
		ifx_mei_atm_led_blink();
#endif
}

//  add one lower priority queue
static int sw_tx_queue_add(int conn)
{
    int ret;
    int new_sw_tx_queue;
    u32 new_sw_tx_queue_table;
    int sw_tx_queue_to_virt_queue_map[ATM_SW_TX_QUEUE_NUMBER] = {0};
    int virt_queue_to_sw_tx_queue_map[8] = {0};
    int num_of_virt_queue;
    u32 sw_tx_queue_table;
    int sw_tx_queue;
    int virt_queue;
    struct wtx_queue_config wtx_queue_config;
    int f_got_wtx_queue_config = 0;
    int i;

    if ( !in_interrupt() )
        down(&g_sem);

    for ( new_sw_tx_queue = 0; new_sw_tx_queue < g_atm_priv_data.max_sw_tx_queues; new_sw_tx_queue++ )
        if ( !(g_atm_priv_data.sw_tx_queue_table & (1 << new_sw_tx_queue)) )
            break;
    if ( new_sw_tx_queue == g_atm_priv_data.max_sw_tx_queues )
    {
        ret = -EINVAL;
        goto SW_TX_QUEUE_ADD_EXIT;
    }

    new_sw_tx_queue_table = g_atm_priv_data.connection[conn].sw_tx_queue_table | (1 << new_sw_tx_queue);

    for ( sw_tx_queue_table = g_atm_priv_data.connection[conn].sw_tx_queue_table, sw_tx_queue = virt_queue = 0;
          sw_tx_queue_table != 0;
          sw_tx_queue_table >>= 1, sw_tx_queue++ )
        if ( (sw_tx_queue_table & 0x01) )
        {
            if ( !f_got_wtx_queue_config )
            {
                wtx_queue_config = *WTX_QUEUE_CONFIG(sw_tx_queue);
                f_got_wtx_queue_config = 1;
            }

            sw_tx_queue_to_virt_queue_map[sw_tx_queue] = virt_queue++;
            WTX_QUEUE_CONFIG(sw_tx_queue)->same_vc_qmap = new_sw_tx_queue_table & ~(1 << sw_tx_queue);
        }

    wtx_queue_config.same_vc_qmap = new_sw_tx_queue_table & ~(1 << new_sw_tx_queue);
    *WTX_QUEUE_CONFIG(new_sw_tx_queue) = wtx_queue_config;

    for ( num_of_virt_queue = 0, sw_tx_queue_table = new_sw_tx_queue_table, sw_tx_queue = 0;
          num_of_virt_queue < NUM_ENTITY(virt_queue_to_sw_tx_queue_map) && sw_tx_queue_table != 0;
          sw_tx_queue_table >>= 1, sw_tx_queue++ )
        if ( (sw_tx_queue_table & 0x01) )
            virt_queue_to_sw_tx_queue_map[num_of_virt_queue++] = sw_tx_queue;

    g_atm_priv_data.sw_tx_queue_table |= 1 << new_sw_tx_queue;
    g_atm_priv_data.connection[conn].sw_tx_queue_table = new_sw_tx_queue_table;

    for ( i = 0; i < 8; i++ )
    {
        sw_tx_queue = g_atm_priv_data.connection[conn].prio_queue_map[i];
        virt_queue = sw_tx_queue_to_virt_queue_map[sw_tx_queue];
        sw_tx_queue = virt_queue_to_sw_tx_queue_map[virt_queue];
        g_atm_priv_data.connection[conn].prio_queue_map[i] = sw_tx_queue;
    }

    ret = 0;

SW_TX_QUEUE_ADD_EXIT:

    if ( !in_interrupt() )
        up(&g_sem);

    return ret;
}

//  delete lowest priority queue
static int sw_tx_queue_del(int conn)
{
    int ret;
    int rem_sw_tx_queue;
    u32 new_sw_tx_queue_table;
    u32 sw_tx_queue_table;
    int sw_tx_queue;
    int i;

    if ( !in_interrupt() )
        down(&g_sem);

    for ( sw_tx_queue_table = g_atm_priv_data.connection[conn].sw_tx_queue_table, rem_sw_tx_queue = g_atm_priv_data.max_sw_tx_queues - 1;
          (sw_tx_queue_table & (1 << (g_atm_priv_data.max_sw_tx_queues - 1))) == 0;
          sw_tx_queue_table <<= 1, rem_sw_tx_queue-- );

    if ( g_atm_priv_data.connection[conn].sw_tx_queue_table == (1 << rem_sw_tx_queue) )
    {
        ret = -EIO;
        goto SW_TX_QUEUE_DEL_EXIT;
    }

    new_sw_tx_queue_table = g_atm_priv_data.connection[conn].sw_tx_queue_table & ~(1 << rem_sw_tx_queue);

    for ( sw_tx_queue_table = new_sw_tx_queue_table, sw_tx_queue = g_atm_priv_data.max_sw_tx_queues - 1;
          (sw_tx_queue_table & (1 << (g_atm_priv_data.max_sw_tx_queues - 1))) == 0;
          sw_tx_queue_table <<= 1, sw_tx_queue-- );

    for ( i = 0; i < 8; i++ )
        if ( g_atm_priv_data.connection[conn].prio_queue_map[i] == rem_sw_tx_queue )
            g_atm_priv_data.connection[conn].prio_queue_map[i] = sw_tx_queue;

    g_atm_priv_data.sw_tx_queue_table &= ~(1 << rem_sw_tx_queue);
    g_atm_priv_data.connection[conn].sw_tx_queue_table = new_sw_tx_queue_table;

    for ( sw_tx_queue_table = new_sw_tx_queue_table, sw_tx_queue = 0;
          sw_tx_queue_table != 0;
          sw_tx_queue_table >>= 1, sw_tx_queue++ )
        if ( (sw_tx_queue_table & 0x01) )
            WTX_QUEUE_CONFIG(sw_tx_queue)->same_vc_qmap = new_sw_tx_queue_table & ~(1 << sw_tx_queue);

    ret = 0;

SW_TX_QUEUE_DEL_EXIT:

    if ( !in_interrupt() )
        up(&g_sem);

    return ret;
}

static int sw_tx_prio_to_queue(int conn, int prio, int queue, int *p_num_of_virt_queue)
{
    int virt_queue_to_sw_tx_queue_map[8] = {0};
    int num_of_virt_queue;
    u32 sw_tx_queue_table;
    int sw_tx_queue;

    if ( conn < 0 )
        return -EINVAL;

    if ( !in_interrupt() )
        down(&g_sem);

    for ( num_of_virt_queue = 0, sw_tx_queue_table = g_atm_priv_data.connection[conn].sw_tx_queue_table, sw_tx_queue = 0;
          num_of_virt_queue < NUM_ENTITY(virt_queue_to_sw_tx_queue_map) && sw_tx_queue_table != 0;
          sw_tx_queue_table >>= 1, sw_tx_queue++ )
        if ( (sw_tx_queue_table & 0x01) )
            virt_queue_to_sw_tx_queue_map[num_of_virt_queue++] = sw_tx_queue;

    if ( p_num_of_virt_queue != NULL )
        *p_num_of_virt_queue = num_of_virt_queue;

    if ( prio < 0 || queue < 0 || queue >= num_of_virt_queue )
    {
        if ( !in_interrupt() )
            up(&g_sem);
        return -EINVAL;
    }

    g_atm_priv_data.connection[conn].prio_queue_map[prio] = virt_queue_to_sw_tx_queue_map[queue];

    if ( !in_interrupt() )
        up(&g_sem);

    return 0;
}

/*
 *  Description:
 *    Setup QSB queue.
 *  Input:
 *    vcc        --- struct atm_vcc *, structure of an opened connection
 *    qos        --- struct atm_qos *, QoS parameter of the connection
 *    connection --- unsigned int, QSB queue ID, which is same as connection ID
 *  Output:
 *    none
 */
static void set_qsb(struct atm_vcc *vcc, struct atm_qos *qos, unsigned int connection)
{
    u32 qsb_clk = ifx_get_fpi_hz();
    union qsb_queue_parameter_table qsb_queue_parameter_table = {{0}};
    union qsb_queue_vbr_parameter_table qsb_queue_vbr_parameter_table = {{0}};
    u32 tmp;

    connection += QSB_QUEUE_NUMBER_BASE;    //  qsb qid = firmware qid + 1

#if defined(DEBUG_QOS) && DEBUG_QOS
    if ( (g_dbg_datapath & DBG_ENABLE_MASK_DUMP_QOS) )
    {
        static char *str_traffic_class[9] = {
            "ATM_NONE",
            "ATM_UBR",
            "ATM_CBR",
            "ATM_VBR",
            "ATM_ABR",
            "ATM_ANYCLASS",
            "ATM_VBR_RT",
            "ATM_UBR_PLUS",
            "ATM_MAX_PCR"
        };

        unsigned char traffic_class = qos->txtp.traffic_class;
        int max_pcr = qos->txtp.max_pcr;
        int pcr = qos->txtp.pcr;
        int min_pcr = qos->txtp.min_pcr;
  #if !defined(DISABLE_VBR) || !DISABLE_VBR
        int scr = qos->txtp.scr;
        int mbs = qos->txtp.mbs;
        int cdv = qos->txtp.cdv;
  #endif

        printk("TX QoS\n");

        printk("  traffic class : ");
        if ( traffic_class == (unsigned char)ATM_MAX_PCR )
            printk("ATM_MAX_PCR\n");
        else if ( traffic_class > ATM_UBR_PLUS )
            printk("Unknown Class\n");
        else
            printk("%s\n", str_traffic_class[traffic_class]);

        printk("  max pcr       : %d\n", max_pcr);
        printk("  desired pcr   : %d\n", pcr);
        printk("  min pcr       : %d\n", min_pcr);

  #if !defined(DISABLE_VBR) || !DISABLE_VBR
        printk("  sustained rate: %d\n", scr);
        printk("  max burst size: %d\n", mbs);
        printk("  cell delay var: %d\n", cdv);
  #endif
    }
#endif    //  defined(DEBUG_QOS) && DEBUG_QOS

    /*
     *  Peak Cell Rate (PCR) Limiter
     */
    if ( qos->txtp.max_pcr == 0 )
        qsb_queue_parameter_table.bit.tp = 0;   /*  disable PCR limiter */
    else
    {
        /*  peak cell rate would be slightly lower than requested [maximum_rate / pcr = (qsb_clock / 8) * (time_step / 4) / pcr] */
        tmp = ((qsb_clk * qsb_tstep) >> 5) / qos->txtp.max_pcr + 1;
        /*  check if overflow takes place   */
        qsb_queue_parameter_table.bit.tp = tmp > QSB_TP_TS_MAX ? QSB_TP_TS_MAX : tmp;
    }

    //  A funny issue. Create two PVCs, one UBR and one UBR with max_pcr.
    //  Send packets to these two PVCs at same time, it trigger strange behavior.
    //  In A1, RAM from 0x80000000 to 0x0x8007FFFF was corrupted with fixed pattern 0x00000000 0x40000000.
    //  In A4, PPE firmware keep emiting unknown cell and do not respond to driver.
    //  To work around, create UBR always with max_pcr.
    //  If user want to create UBR without max_pcr, we give a default one larger than line-rate.
    if ( qos->txtp.traffic_class == ATM_UBR && qsb_queue_parameter_table.bit.tp == 0 )
    {
        int port = g_atm_priv_data.connection[connection - QSB_QUEUE_NUMBER_BASE].port;
        unsigned int max_pcr = g_atm_priv_data.port[port].tx_max_cell_rate + 1000;

        tmp = ((qsb_clk * qsb_tstep) >> 5) / max_pcr + 1;
        if ( tmp > QSB_TP_TS_MAX )
            tmp = QSB_TP_TS_MAX;
        else if ( tmp < 1 )
            tmp = 1;
        qsb_queue_parameter_table.bit.tp = tmp;
    }

    /*
     *  Weighted Fair Queueing Factor (WFQF)
     */
    switch ( qos->txtp.traffic_class )
    {
    case ATM_CBR:
    case ATM_VBR_RT:
        /*  real time queue gets weighted fair queueing bypass  */
        qsb_queue_parameter_table.bit.wfqf = 0;
        break;
    case ATM_VBR_NRT:
    case ATM_UBR_PLUS:
        /*  WFQF calculation here is based on virtual cell rates, to reduce granularity for high rates  */
        /*  WFQF is maximum cell rate / garenteed cell rate                                             */
        /*  wfqf = qsb_minimum_cell_rate * QSB_WFQ_NONUBR_MAX / requested_minimum_peak_cell_rate        */
        if ( qos->txtp.min_pcr == 0 )
            qsb_queue_parameter_table.bit.wfqf = QSB_WFQ_NONUBR_MAX;
        else
        {
            tmp = QSB_GCR_MIN * QSB_WFQ_NONUBR_MAX / qos->txtp.min_pcr;
            if ( tmp == 0 )
                qsb_queue_parameter_table.bit.wfqf = 1;
            else if ( tmp > QSB_WFQ_NONUBR_MAX )
                qsb_queue_parameter_table.bit.wfqf = QSB_WFQ_NONUBR_MAX;
            else
                qsb_queue_parameter_table.bit.wfqf = tmp;
        }
        break;
    default:
    case ATM_UBR:
        qsb_queue_parameter_table.bit.wfqf = QSB_WFQ_UBR_BYPASS;
    }

    /*
     *  Sustained Cell Rate (SCR) Leaky Bucket Shaper VBR.0/VBR.1
     */
    if ( qos->txtp.traffic_class == ATM_VBR_RT || qos->txtp.traffic_class == ATM_VBR_NRT )
    {
#if defined(DISABLE_VBR) && DISABLE_VBR
        /*  disable shaper  */
        qsb_queue_vbr_parameter_table.bit.taus = 0;
        qsb_queue_vbr_parameter_table.bit.ts = 0;
#else
        if ( qos->txtp.scr == 0 )
        {
            /*  disable shaper  */
            qsb_queue_vbr_parameter_table.bit.taus = 0;
            qsb_queue_vbr_parameter_table.bit.ts = 0;
        }
        else
        {
            /*  Cell Loss Priority  (CLP)   */
            if ( (vcc->atm_options & ATM_ATMOPT_CLP) )
                /*  CLP1    */
                qsb_queue_parameter_table.bit.vbr = 1;
            else
                /*  CLP0    */
                qsb_queue_parameter_table.bit.vbr = 0;
            /*  Rate Shaper Parameter (TS) and Burst Tolerance Parameter for SCR (tauS) */
            tmp = ((qsb_clk * qsb_tstep) >> 5) / qos->txtp.scr + 1;
            qsb_queue_vbr_parameter_table.bit.ts = tmp > QSB_TP_TS_MAX ? QSB_TP_TS_MAX : tmp;
            tmp = (qos->txtp.mbs - 1) * (qsb_queue_vbr_parameter_table.bit.ts - qsb_queue_parameter_table.bit.tp) / 64;
            if ( tmp == 0 )
                qsb_queue_vbr_parameter_table.bit.taus = 1;
            else if ( tmp > QSB_TAUS_MAX )
                qsb_queue_vbr_parameter_table.bit.taus = QSB_TAUS_MAX;
            else
                qsb_queue_vbr_parameter_table.bit.taus = tmp;
        }
#endif
    }
    else
    {
        qsb_queue_vbr_parameter_table.bit.taus = 0;
        qsb_queue_vbr_parameter_table.bit.ts = 0;
    }

    /*  Queue Parameter Table (QPT) */
    *QSB_RTM   = QSB_RTM_DM_SET(QSB_QPT_SET_MASK);
    *QSB_RTD   = QSB_RTD_TTV_SET(qsb_queue_parameter_table.dword);
    *QSB_RAMAC = QSB_RAMAC_RW_SET(QSB_RAMAC_RW_WRITE) | QSB_RAMAC_TSEL_SET(QSB_RAMAC_TSEL_QPT) | QSB_RAMAC_LH_SET(QSB_RAMAC_LH_LOW) | QSB_RAMAC_TESEL_SET(connection);
#if defined(DEBUG_QOS) && DEBUG_QOS
    if ( (g_dbg_datapath & DBG_ENABLE_MASK_DUMP_QOS) )
        printk("QPT: QSB_RTM (%08X) = 0x%08X, QSB_RTD (%08X) = 0x%08X, QSB_RAMAC (%08X) = 0x%08X\n", (u32)QSB_RTM, *QSB_RTM, (u32)QSB_RTD, *QSB_RTD, (u32)QSB_RAMAC, *QSB_RAMAC);
#endif
    /*  Queue VBR Paramter Table (QVPT) */
    *QSB_RTM   = QSB_RTM_DM_SET(QSB_QVPT_SET_MASK);
    *QSB_RTD   = QSB_RTD_TTV_SET(qsb_queue_vbr_parameter_table.dword);
    *QSB_RAMAC = QSB_RAMAC_RW_SET(QSB_RAMAC_RW_WRITE) | QSB_RAMAC_TSEL_SET(QSB_RAMAC_TSEL_VBR) | QSB_RAMAC_LH_SET(QSB_RAMAC_LH_LOW) | QSB_RAMAC_TESEL_SET(connection);
#if defined(DEBUG_QOS) && DEBUG_QOS
    if ( (g_dbg_datapath & DBG_ENABLE_MASK_DUMP_QOS) )
        printk("QVPT: QSB_RTM (%08X) = 0x%08X, QSB_RTD (%08X) = 0x%08X, QSB_RAMAC (%08X) = 0x%08X\n", (u32)QSB_RTM, *QSB_RTM, (u32)QSB_RTD, *QSB_RTD, (u32)QSB_RAMAC, *QSB_RAMAC);
#endif

#if defined(DEBUG_QOS) && DEBUG_QOS
    if ( (g_dbg_datapath & DBG_ENABLE_MASK_DUMP_QOS) )
    {
        printk("set_qsb\n");
        printk("  qsb_clk = %lu\n", (unsigned long)qsb_clk);
        printk("  qsb_queue_parameter_table.bit.tp       = %d\n", (int)qsb_queue_parameter_table.bit.tp);
        printk("  qsb_queue_parameter_table.bit.wfqf     = %d (0x%08X)\n", (int)qsb_queue_parameter_table.bit.wfqf, (int)qsb_queue_parameter_table.bit.wfqf);
        printk("  qsb_queue_parameter_table.bit.vbr      = %d\n", (int)qsb_queue_parameter_table.bit.vbr);
        printk("  qsb_queue_parameter_table.dword        = 0x%08X\n", (int)qsb_queue_parameter_table.dword);
        printk("  qsb_queue_vbr_parameter_table.bit.ts   = %d\n", (int)qsb_queue_vbr_parameter_table.bit.ts);
        printk("  qsb_queue_vbr_parameter_table.bit.taus = %d\n", (int)qsb_queue_vbr_parameter_table.bit.taus);
        printk("  qsb_queue_vbr_parameter_table.dword    = 0x%08X\n", (int)qsb_queue_vbr_parameter_table.dword);
    }
#endif
}

/*
 *  Description:
 *    Add one entry to HTU table.
 *  Input:
 *    vpi        --- unsigned int, virtual path ID
 *    vci        --- unsigned int, virtual channel ID
 *    connection --- unsigned int, connection ID
 *    aal5       --- int, 0 means AAL0, else means AAL5
 *  Output:
 *    none
 */
static INLINE void set_htu_entry(unsigned int vpi, unsigned int vci, unsigned int conn, int aal5)
{
    struct htu_entry htu_entry = {  res1:       0x00,
                                    pid:        g_atm_priv_data.connection[conn].port & 0x01,
                                    vpi:        vpi,
                                    vci:        vci,
                                    pti:        0x00,
                                    vld:        0x01};

    struct htu_mask htu_mask = {    set:        0x03,
                                    pid_mask:   0x02,
                                    vpi_mask:   0x00,
                                    vci_mask:   0x0000,
                                    pti_mask:   0x03,   //  0xx, user data
                                    clear:      0x00};

    struct htu_result htu_result = {res1:       0x00,
                                    cellid:     conn,
                                    res2:       0x00,
                                    type:       aal5 ? 0x00 : 0x01,
                                    ven:        0x01,
                                    res3:       0x00,
                                    qid:        conn};

#if 0
    htu_entry.vld     = 1;
    htu_mask.pid_mask = 0x03;
    htu_mask.vpi_mask = 0xFF;
    htu_mask.vci_mask = 0xFFFF;
    htu_mask.pti_mask = 0x7;
    htu_result.cellid = 1;
    htu_result.type   = 0;
    htu_result.ven    = 0;
    htu_result.qid    = conn;
#endif
    *HTU_RESULT(conn + OAM_HTU_ENTRY_NUMBER) = htu_result;
    *HTU_MASK(conn + OAM_HTU_ENTRY_NUMBER)   = htu_mask;
    *HTU_ENTRY(conn + OAM_HTU_ENTRY_NUMBER)  = htu_entry;

//    printk("Config HTU (%d) for QID %d\n", conn + OAM_HTU_ENTRY_NUMBER, conn);
//    printk("  HTU_RESULT = 0x%08X\n", *(u32*)HTU_RESULT(conn + OAM_HTU_ENTRY_NUMBER));
//    printk("  HTU_MASK   = 0x%08X\n", *(u32*)HTU_MASK(conn + OAM_HTU_ENTRY_NUMBER));
//    printk("  HTU_ENTRY  = 0x%08X\n", *(u32*)HTU_ENTRY(conn + OAM_HTU_ENTRY_NUMBER));
}

/*
 *  Description:
 *    Remove one entry from HTU table.
 *  Input:
 *    conn --- unsigned int, connection ID
 *  Output:
 *    none
 */
static INLINE void clear_htu_entry(unsigned int conn)
{
    HTU_ENTRY(conn + OAM_HTU_ENTRY_NUMBER)->vld = 0;
}

/*
 *  Description:
 *    Setup QSB.
 *  Input:
 *    none
 *  Output:
 *    none
 */
static INLINE void qsb_global_set(void)
{
    u32 qsb_clk = ifx_get_fpi_hz();
    int i;
    u32 tmp1, tmp2, tmp3;

    *QSB_ICDV = QSB_ICDV_TAU_SET(qsb_tau);
    *QSB_SBL  = QSB_SBL_SBL_SET(qsb_srvm);
    *QSB_CFG  = QSB_CFG_TSTEPC_SET(qsb_tstep >> 1);
#if defined(DEBUG_QOS) && DEBUG_QOS
    if ( (g_dbg_datapath & DBG_ENABLE_MASK_DUMP_QOS) )
    {
        printk("qsb_clk = %u\n", qsb_clk);
        printk("QSB_ICDV (%08X) = %d (%d), QSB_SBL (%08X) = %d (%d), QSB_CFG (%08X) = %d (%d)\n", (u32)QSB_ICDV, *QSB_ICDV, QSB_ICDV_TAU_SET(qsb_tau), (u32)QSB_SBL, *QSB_SBL, QSB_SBL_SBL_SET(qsb_srvm), (u32)QSB_CFG, *QSB_CFG, QSB_CFG_TSTEPC_SET(qsb_tstep >> 1));
    }
#endif

    /*
     *  set SCT and SPT per port
     */
    for ( i = 0; i < ATM_PORT_NUMBER; i++ )
        if ( g_atm_priv_data.port[i].tx_max_cell_rate != 0 )
        {
            tmp1 = ((qsb_clk * qsb_tstep) >> 1) / g_atm_priv_data.port[i].tx_max_cell_rate;
            tmp2 = tmp1 >> 6;                   /*  integer value of Tsb    */
            tmp3 = (tmp1 & ((1 << 6) - 1)) + 1; /*  fractional part of Tsb  */
            /*  carry over to integer part (?)  */
            if ( tmp3 == (1 << 6) )
            {
                tmp3 = 0;
                tmp2++;
            }
            if ( tmp2 == 0 )
                tmp2 = tmp3 = 1;
            /*  1. set mask                                 */
            /*  2. write value to data transfer register    */
            /*  3. start the tranfer                        */
            /*  SCT (FracRate)  */
            *QSB_RTM   = QSB_RTM_DM_SET(QSB_SET_SCT_MASK);
            *QSB_RTD   = QSB_RTD_TTV_SET(tmp3);
            *QSB_RAMAC = QSB_RAMAC_RW_SET(QSB_RAMAC_RW_WRITE) | QSB_RAMAC_TSEL_SET(QSB_RAMAC_TSEL_SCT) | QSB_RAMAC_LH_SET(QSB_RAMAC_LH_LOW) | QSB_RAMAC_TESEL_SET(i & 0x01);
#if defined(DEBUG_QOS) && DEBUG_QOS
            if ( (g_dbg_datapath & DBG_ENABLE_MASK_DUMP_QOS) )
                printk("SCT: QSB_RTM (%08X) = 0x%08X, QSB_RTD (%08X) = 0x%08X, QSB_RAMAC (%08X) = 0x%08X\n", (u32)QSB_RTM, *QSB_RTM, (u32)QSB_RTD, *QSB_RTD, (u32)QSB_RAMAC, *QSB_RAMAC);
#endif
            /*  SPT (SBV + PN + IntRage)    */
            *QSB_RTM   = QSB_RTM_DM_SET(QSB_SET_SPT_MASK);
            *QSB_RTD   = QSB_RTD_TTV_SET(QSB_SPT_SBV_VALID | QSB_SPT_PN_SET(i & 0x01) | QSB_SPT_INTRATE_SET(tmp2));
            *QSB_RAMAC = QSB_RAMAC_RW_SET(QSB_RAMAC_RW_WRITE) | QSB_RAMAC_TSEL_SET(QSB_RAMAC_TSEL_SPT) | QSB_RAMAC_LH_SET(QSB_RAMAC_LH_LOW) | QSB_RAMAC_TESEL_SET(i & 0x01);
#if defined(DEBUG_QOS) && DEBUG_QOS
            if ( (g_dbg_datapath & DBG_ENABLE_MASK_DUMP_QOS) )
                printk("SPT: QSB_RTM (%08X) = 0x%08X, QSB_RTD (%08X) = 0x%08X, QSB_RAMAC (%08X) = 0x%08X\n", (u32)QSB_RTM, *QSB_RTM, (u32)QSB_RTD, *QSB_RTD, (u32)QSB_RAMAC, *QSB_RAMAC);
#endif
        }
}

static INLINE void setup_oam_htu_entry(void)
{
    struct htu_entry htu_entry = {0};
    struct htu_result htu_result = {0};
    struct htu_mask htu_mask = {    set:        0x03,
                                    pid_mask:   0x00,
                                    vpi_mask:   0x00,
                                    vci_mask:   0x00,
                                    pti_mask:   0x00,
                                    clear:      0x00};
    int i;

    /*
     *  HTU Tables
     */
    for ( i = 0; i < g_atm_priv_data.max_connections; i++ )
    {
        htu_result.qid = (unsigned int)i;

        *HTU_ENTRY(i + OAM_HTU_ENTRY_NUMBER)  = htu_entry;
        *HTU_MASK(i + OAM_HTU_ENTRY_NUMBER)   = htu_mask;
        *HTU_RESULT(i + OAM_HTU_ENTRY_NUMBER) = htu_result;
    }
    /*  OAM HTU Entry   */
    htu_entry.vci     = 0x03;
    htu_mask.pid_mask = 0x03;
    htu_mask.vpi_mask = 0xFF;
    htu_mask.vci_mask = 0x0000;
    htu_mask.pti_mask = 0x07;
    htu_result.cellid = 0;  //  need confirm
    htu_result.type   = 1;
    htu_result.ven    = 1;
    htu_result.qid    = 0;  //  need confirm
    *HTU_RESULT(OAM_F4_SEG_HTU_ENTRY) = htu_result;
    *HTU_MASK(OAM_F4_SEG_HTU_ENTRY)   = htu_mask;
    *HTU_ENTRY(OAM_F4_SEG_HTU_ENTRY)  = htu_entry;
    htu_entry.vci     = 0x04;
    htu_result.cellid = 0;  //  need confirm
    htu_result.type   = 1;
    htu_result.ven    = 1;
    htu_result.qid    = 0;  //  need confirm
    *HTU_RESULT(OAM_F4_TOT_HTU_ENTRY) = htu_result;
    *HTU_MASK(OAM_F4_TOT_HTU_ENTRY)   = htu_mask;
    *HTU_ENTRY(OAM_F4_TOT_HTU_ENTRY)  = htu_entry;
    htu_entry.vci     = 0x00;
    htu_entry.pti     = 0x04;
    htu_mask.vci_mask = 0xFFFF;
    htu_mask.pti_mask = 0x01;
    htu_result.cellid = 0;  //  need confirm
    htu_result.type   = 1;
    htu_result.ven    = 1;
    htu_result.qid    = 0;  //  need confirm
    *HTU_RESULT(OAM_F5_HTU_ENTRY) = htu_result;
    *HTU_MASK(OAM_F5_HTU_ENTRY)   = htu_mask;
    *HTU_ENTRY(OAM_F5_HTU_ENTRY)  = htu_entry;
}

/*
 *  Description:
 *    Add HTU entries to capture OAM cell.
 *  Input:
 *    none
 *  Output:
 *    none
 */
static INLINE void validate_oam_htu_entry(void)
{
    HTU_ENTRY(OAM_F4_SEG_HTU_ENTRY)->vld = 1;
    HTU_ENTRY(OAM_F4_TOT_HTU_ENTRY)->vld = 1;
    HTU_ENTRY(OAM_F5_HTU_ENTRY)->vld = 1;
}

/*
 *  Description:
 *    Remove HTU entries which are used to capture OAM cell.
 *  Input:
 *    none
 *  Output:
 *    none
 */
static INLINE void invalidate_oam_htu_entry(void)
{
    register int i;

    HTU_ENTRY(OAM_F4_SEG_HTU_ENTRY)->vld = 0;
    HTU_ENTRY(OAM_F4_TOT_HTU_ENTRY)->vld = 0;
    HTU_ENTRY(OAM_F5_HTU_ENTRY)->vld = 0;
    /*  idle for a while to finish running HTU search   */
    for ( i = 0; i < IDLE_CYCLE_NUMBER; i++ );
}

/*
 *  Description:
 *    Loop up for connection ID with virtual path ID.
 *  Input:
 *    vpi --- unsigned int, virtual path ID
 *  Output:
 *    int --- negative value: failed
 *            else          : connection ID
 */
static INLINE int find_vpi(unsigned int vpi)
{
    int i;
    struct atm_connection *connection = g_atm_priv_data.connection;

    for ( i = 0; i < g_atm_priv_data.max_connections; i++ )
        if ( (g_atm_priv_data.connection_table & (1 << i))
            && connection[i].vcc != NULL
            && vpi == connection[i].vcc->vpi )
            return i;
    return -1;
}

/*
 *  Description:
 *    Loop up for connection ID with virtual path ID and virtual channel ID.
 *  Input:
 *    vpi --- unsigned int, virtual path ID
 *    vci --- unsigned int, virtual channel ID
 *  Output:
 *    int --- negative value: failed
 *            else          : connection ID
 */
static INLINE int find_vpivci(unsigned int vpi, unsigned int vci)
{
    int i;
    struct atm_connection *connection = g_atm_priv_data.connection;

    for ( i = 0; i < g_atm_priv_data.max_connections; i++ )
        if ( (g_atm_priv_data.connection_table & (1 << i))
            && connection[i].vcc != NULL
            && vpi == connection[i].vcc->vpi
            && vci == connection[i].vcc->vci )
            return i;
    return -1;
}

/*
 *  Description:
 *    Loop up for connection ID with atm_vcc structure.
 *  Input:
 *    vcc --- struct atm_vcc *, atm_vcc structure of opened connection
 *  Output:
 *    int --- negative value: failed
 *            else          : connection ID
 */
static INLINE int find_vcc(struct atm_vcc *vcc)
{
    int i;
    struct atm_connection *connection = g_atm_priv_data.connection;

    for ( i = 0; i < g_atm_priv_data.max_connections; i++ )
        if ( (g_atm_priv_data.connection_table & (1 << i))
            && connection[i].vcc == vcc )
            return i;
    return -1;
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


static struct sk_buff* atm_alloc_tx(struct atm_vcc *vcc, unsigned int size)
{
    struct sk_buff *skb;

    /*  send buffer overflow    */
    if ( atomic_read(&sk_atm(vcc)->sk_wmem_alloc) && !atm_may_send(vcc, size) )
    {
        err("send buffer overflow");
        return NULL;
    }

    skb = alloc_skb_tx(size < DMA_PACKET_SIZE ? DMA_PACKET_SIZE : size);
    if ( skb == NULL )
    {
        err("sk buffer is used up");
        return NULL;
    }

    atomic_add(skb->truesize, &sk_atm(vcc)->sk_wmem_alloc);

    return skb;
}

static INLINE void atm_free_tx_skb_vcc(struct sk_buff *skb)
{
    struct atm_vcc* vcc;

    ASSERT((u32)skb > 0x80000000, "atm_free_tx_skb_vcc: skb = %08X", (u32)skb);

    vcc = ATM_SKB(skb)->vcc;

    if ( vcc != NULL && vcc->pop != NULL )
    {
        ASSERT(atomic_read(&skb->users) != 0, "atm_free_tx_skb_vcc(vcc->pop): skb->users == 0, skb = %08X", (u32)skb);
        vcc->pop(vcc, skb);
    }
    else
    {
        ASSERT(atomic_read(&skb->users) != 0, "atm_free_tx_skb_vcc(dev_kfree_skb_any): skb->users == 0, skb = %08X", (u32)skb);
        dev_kfree_skb_any(skb);
    }
}


static irqreturn_t mailbox0_irq_handler(int irq, void *dev_id)
{
    u32 mailbox_signal = 0;

    mailbox_signal = *MBOX_IGU0_ISR & *MBOX_IGU0_IER;
    if ( !mailbox_signal )
        return IRQ_HANDLED;
    *MBOX_IGU0_ISRC = mailbox_signal;

    if ( (mailbox_signal & DMA_TX_CH0_SIG) )
    {
        g_dma_device_ppe->tx_chan[0]->open(g_dma_device_ppe->tx_chan[0]);
        *MBOX_IGU0_IER &= DMA_TX_CH1_SIG;
        dbg("DMA_TX_CH0_SIG");
    }

    if ( (mailbox_signal & DMA_TX_CH1_SIG) )
    {
        g_dma_device_ppe->tx_chan[1]->open(g_dma_device_ppe->tx_chan[1]);
        *MBOX_IGU0_IER &= DMA_TX_CH0_SIG;
        dbg("DMA_TX_CH1_SIG");
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
static irqreturn_t mailbox1_irq_handler(int irq, void *dev_id)
{
    u32 mailbox_signal = 0;

    mailbox_signal = *MBOX_IGU1_ISR & g_mailbox_signal_mask;
    if ( !mailbox_signal )
        return IRQ_HANDLED;
    *MBOX_IGU1_ISRC = mailbox_signal;

    if ( (mailbox_signal & WAN_RX_SIG(0)) ) //  AAL5
    {
        g_mailbox_signal_mask &= ~WAN_RX_SIG(0);
        g_dma_device_ppe->tx_chan[1]->open(g_dma_device_ppe->tx_chan[1]);
        *MBOX_IGU1_IER = g_mailbox_signal_mask;
        dbg("WAN_RX_SIG(0)");
    }

    if ( (mailbox_signal & WAN_RX_SIG(1)) ) //  OAM
    {
        volatile struct rx_descriptor *desc;
        struct sk_buff *skb;
        struct uni_cell_header *header;
        int conn;
        struct atm_vcc *vcc;

        do
        {
            while ( 1 )
            {
                desc = WAN_RX_DESC_BASE(1) + g_wan_rx1_desc_pos;
                if ( desc->own )    //  PPE hold
                    break;
                skb = g_wan_rx1_desc_skb[g_wan_rx1_desc_pos];
                if ( ++g_wan_rx1_desc_pos == WAN_RX_DESC_NUM(1) )
                    g_wan_rx1_desc_pos = 0;

#if defined(DEBUG_DUMP_SKB) && DEBUG_DUMP_SKB
                skb->tail = skb->data + CELL_SIZE;
                skb->len = CELL_SIZE;
                dump_skb(skb, DUMP_SKB_LEN, "mailbox1_irq_handler - OAM", 7, 0, 0, 0);
#endif

                header = (struct uni_cell_header *)skb->data;
                if ( header->pti == ATM_PTI_SEGF5 || header->pti == ATM_PTI_E2EF5 )
                    conn = find_vpivci(header->vpi, header->vci);
                else if ( header->vci == 0x03 || header->vci == 0x04 )
                    conn = find_vpi(header->vpi);
                else
                    conn = -1;

                if ( conn >= 0 && (vcc = g_atm_priv_data.connection[conn].vcc) != NULL )
                {
                    g_atm_priv_data.connection[conn].access_time = current_kernel_time();

                    if ( vcc->push_oam != NULL )
                        vcc->push_oam(vcc, skb->data);
                    else
                        ifx_push_oam(skb->data);
                }
                else {
                    oam_cell_drop_count++;
                    dbg("OAM cell dropped due to wrong qid/conn");
                }

                desc->c = 0;
                desc->own = 1;
            }
            *MBOX_IGU1_ISRC = WAN_RX_SIG(1);
        } while ( !desc->own );   //  check after clean, if MIPS hold, repeat
    }

    if ( (mailbox_signal & CPU_TO_WAN_SWAP_SIG) )
    {
        struct sk_buff *new_skb;
        volatile struct tx_descriptor *desc = &CPU_TO_WAN_SWAP_DESC_BASE[g_cpu_to_wan_swap_desc_pos];
        struct tx_descriptor reg_desc = {0};
        struct sk_buff *skb_to_free;

        while ( desc->own == 0 )
        {
            new_skb = alloc_skb_rx();
            if ( new_skb == NULL )
                break;

#ifndef CONFIG_MIPS_UNCACHED
            /*  invalidate cache    */
            dma_cache_inv((unsigned long)new_skb->data, DMA_PACKET_SIZE);
#endif

            if ( ++g_cpu_to_wan_swap_desc_pos == CPU_TO_WAN_SWAP_DESC_NUM )
                g_cpu_to_wan_swap_desc_pos = 0;

            /*  free previous skb   */
            free_skb_clear_dataptr(desc);
            put_skb_to_dbg_pool(new_skb);

            /*  update descriptor   */
            reg_desc.dataptr = (unsigned int)new_skb->data & (0x1FFFFFFF ^ (DMA_TX_ALIGNMENT - 1));
            reg_desc.own     = 1;

            /*  write discriptor to memory  */
            *((volatile unsigned int *)desc + 1) = *((unsigned int *)&reg_desc + 1);
            wmb();
            *(volatile unsigned int *)desc = *(unsigned int *)&reg_desc;

            desc = &CPU_TO_WAN_SWAP_DESC_BASE[g_cpu_to_wan_swap_desc_pos];
        }
    }

    return IRQ_HANDLED;
}

static INLINE void turn_on_dma_rx(int mask)
{
    int i;

    if ( !g_f_dma_opened )
    {
        ASSERT((u32)g_dma_device_ppe >= 0x80000000, "g_dma_device_ppe = 0x%08X", (u32)g_dma_device_ppe);

        for ( i = 0; i < g_dma_device_ppe->max_rx_chan_num; i++ )
        {
            ASSERT((u32)g_dma_device_ppe->rx_chan[i] >= 0x80000000, "g_dma_device_ppe->rx_chan[%d] = 0x%08X", i, (u32)g_dma_device_ppe->rx_chan[i]);
            ASSERT(g_dma_device_ppe->rx_chan[i]->control == IFX_DMA_CH_ON, "g_dma_device_ppe->rx_chan[i]->control = %d", g_dma_device_ppe->rx_chan[i]->control);

            if ( g_dma_device_ppe->rx_chan[i]->control == IFX_DMA_CH_ON )
            {
                ASSERT((u32)g_dma_device_ppe->rx_chan[i]->open >= 0x80000000, "g_dma_device_ppe->rx_chan[%d]->open = 0x%08X", i, (u32)g_dma_device_ppe->rx_chan[i]->open);

#if 0
                //  channel 1, 2 is for fast path
                if ( i == 1 || i == 2 )
                    g_dma_device_ppe->rx_chan[i]->dir = 1;
#endif
                g_dma_device_ppe->rx_chan[i]->open(g_dma_device_ppe->rx_chan[i]);
#if 0
                if ( i == 1 || i == 2 )
                    g_dma_device->rx_chan[i]->dir = 0;
#endif
            }
        }
    }
    g_f_dma_opened |= 1 << mask;
}

static INLINE void turn_off_dma_rx(int mask)
{
    int i;

    g_f_dma_opened &= ~(1 << mask);
    if ( !g_f_dma_opened )
        for ( i = 0; i < g_dma_device_ppe->max_rx_chan_num; i++ )
            g_dma_device_ppe->rx_chan[i]->close(g_dma_device_ppe->rx_chan[i]);
}

static u8 *dma_buffer_alloc(int len, int *byte_offset, void **opt)
{
    u8 *buf;
    struct sk_buff *skb;

    skb = alloc_skb_rx();
    if ( !skb )
        return NULL;

    put_skb_to_dbg_pool(skb);

    buf = (u8 *)skb->data;
    *(u32 *)opt = (u32)skb;
    *byte_offset = 0;
    return buf;
}

static int dma_buffer_free(u8 *dataptr, void *opt)
{
    //  ! be careful
    //    this method makes kernel crash when free this DMA device
    //    since the pointers used by fastpast which is supposed not visible to CPU
    //    mix up the pointers so that "opt" is not align with "dataptr".

//    ASSERT(*(void **)((((u32)dataptr | KSEG1) & ~0x0F) - 4) == opt, "address is not match: dataptr = %08X, opt = %08X, *(void **)((((u32)dataptr | KSEG1) & ~0x0F) - 4) = %08X", (u32)dataptr, (u32)opt, *(u32 *)((((u32)dataptr | KSEG1) & ~0x0F) - 4));

    if ( opt )
    {
        get_skb_from_dbg_pool((struct sk_buff *)opt);
        dev_kfree_skb_any((struct sk_buff *)opt);
    }

    return 0;
}

#if defined(ENABLE_RX_PROFILE) && ENABLE_RX_PROFILE
  static unsigned int count_buf_a[512];
  static unsigned int count_buf_a_pos = 0;
  unsigned int count_rx_a_start, count_rx_a_end;
  static unsigned int count_buf_b[512];
  static unsigned int count_buf_b_pos = 0;
  unsigned int count_rx_b_start, count_rx_b_end;
#endif

static int dma_int_handler(struct dma_device_info *dma_dev, int status, int dma_ch_no)
{
    int ret = 0;
    unsigned long sys_flag;
#if !defined(ENABLE_DIRECTPATH_TX_QUEUE) || !ENABLE_DIRECTPATH_TX_QUEUE
    int i;
#endif
#if defined(ENABLE_RX_PROFILE) && ENABLE_RX_PROFILE
    unsigned int count_rx_start = 0, count_rx_end = 0;
    static unsigned int count_buf[512];
    static unsigned int count_buf_pos = 0;
#endif

    switch ( status )
    {
    case RCV_INT:
#if defined(ENABLE_RX_PROFILE) && ENABLE_RX_PROFILE
        count_rx_a_start = count_rx_a_end = 0;
        count_rx_b_start = count_rx_b_end = 0;
        local_irq_save(sys_flag);
        count_rx_start = read_c0_count();
#endif
        ret = dma_rx_int_handler(dma_dev, dma_ch_no);
#if defined(ENABLE_RX_PROFILE) && ENABLE_RX_PROFILE
        count_rx_end = read_c0_count();
        local_irq_restore(sys_flag);
        //printk("%s: rx %d cycles\n", __func__, count_rx_end - count_rx_start);
        count_buf[count_buf_pos++] = count_rx_end - count_rx_start;
        if ( count_buf_pos >= NUM_ENTITY(count_buf) )
        {
            for ( count_buf_pos = 0, count_rx_end = 0; count_buf_pos < NUM_ENTITY(count_buf); count_buf_pos++ )
                count_rx_end += count_buf[count_buf_pos];
            count_rx_end = (count_rx_end + NUM_ENTITY(count_buf) / 2) / NUM_ENTITY(count_buf);
            printk("%s: average rx (over %d packets) %d cycles\n", __func__, NUM_ENTITY(count_buf), count_rx_end);

            count_buf_pos = 0;
        }

        if ( count_rx_a_start != count_rx_a_end )
        {
            count_buf_a[count_buf_a_pos++] = count_rx_a_end - count_rx_a_start;
            if ( count_buf_a_pos >= NUM_ENTITY(count_buf_a) )
            {
                for ( count_buf_a_pos = 0, count_rx_a_end = 0; count_buf_a_pos < NUM_ENTITY(count_buf_a); count_buf_a_pos++ )
                    count_rx_a_end += count_buf_a[count_buf_a_pos];
                count_rx_a_end = (count_rx_a_end + NUM_ENTITY(count_buf_a) / 2) / NUM_ENTITY(count_buf_a);
                printk("%s: average rx decap (over %d packets) %d cycles\n", __func__, NUM_ENTITY(count_buf_a), count_rx_a_end);

                count_buf_a_pos = 0;
            }
        }
        if ( count_rx_b_start != count_rx_b_end )
        {
            count_buf_b[count_buf_b_pos++] = count_rx_b_end - count_rx_b_start;
            if ( count_buf_b_pos >= NUM_ENTITY(count_buf_b) )
            {
                for ( count_buf_b_pos = 0, count_rx_b_end = 0; count_buf_b_pos < NUM_ENTITY(count_buf_b); count_buf_b_pos++ )
                    count_rx_b_end += count_buf_b[count_buf_b_pos];
                count_rx_b_end = (count_rx_b_end + NUM_ENTITY(count_buf_b) / 2) / NUM_ENTITY(count_buf_b);
                printk("%s: average rx push (over %d packets) %d cycles\n", __func__, NUM_ENTITY(count_buf_b), count_rx_b_end);

                count_buf_b_pos = 0;
            }
        }
#endif
        break;
    case TX_BUF_FULL_INT:
        if (dma_ch_no == 2)
        {
        	int dev_nr;

            dbg("eth0/1 TX buffer full!");
            spin_lock_irqsave(&g_eth_tx_spinlock, sys_flag);

            /*
             * TODO vgl. UGW - hier wurden trans_start Zeiten geswapt - warum? -
             * bei mehr als 2 devices ist das nicht mehr trivial -
             */

#if 0
            for (dev_nr = 0; dev_nr < num_ar9_eth_devs; dev_nr++)
            	netif_stop_queue(g_eth_net_dev[dev_nr]);

#endif
            if ( g_dma_device_ppe->tx_chan[2]->control == IFX_DMA_CH_ON )
                g_dma_device_ppe->tx_chan[2]->enable_irq(g_dma_device_ppe->tx_chan[2]);
            spin_unlock_irqrestore(&g_eth_tx_spinlock, sys_flag);
        }
        else if (dma_ch_no == 3)
        {
            dbg("directpath TX (CPU->PPE) buffer full!");

            if ( !g_directpath_dma_full )
            {
                g_directpath_dma_full = 1;
                if ( g_dma_device_ppe->tx_chan[3]->control == IFX_DMA_CH_ON )
                    g_dma_device_ppe->tx_chan[3]->enable_irq(g_dma_device_ppe->tx_chan[3]);
#if !defined(ENABLE_DIRECTPATH_TX_QUEUE) || !ENABLE_DIRECTPATH_TX_QUEUE
                for ( i = 0; i <  MAX_PPA_PUSHTHROUGH_DEVICES; i++ )
                    if ( ppa_virt_rx_devs[i] )
                        avm_pa_rx_channel_suspend( ppa_virt_rx_devs[i]->pid_handle );
#endif
            }

        }
        else
        {
            err("incorrect DMA TX channel: %d (0 - 1 is reserved for fast path)", dma_ch_no);
        }
        break;
    case TRANSMIT_CPT_INT:
        if (dma_ch_no == 2)
        {
        	int dev_nr;
            dbg("eth0/1 TX buffer released!");
            local_irq_save(sys_flag);
            g_dma_device_ppe->tx_chan[2]->disable_irq(g_dma_device_ppe->tx_chan[2]);

#if 0
            for (dev_nr = 0; dev_nr < num_ar9_eth_devs; dev_nr++)
            	netif_wake_queue(g_eth_net_dev[dev_nr]);
#endif
            local_irq_restore(sys_flag);
        }
        else if (dma_ch_no ==  DMA_CHAN_NR_DIRECTPATH_CPU_TO_PPE )
        {
            dbg("directpath TX (CPU->PPE) buffer released");
            spin_lock_irqsave(&g_directpath_tx_spinlock, sys_flag);
            if ( g_directpath_dma_full )
            {
                g_directpath_dma_full = 0;
                g_dma_device_ppe->tx_chan[3]->disable_irq(g_dma_device_ppe->tx_chan[3]);
#if !defined(ENABLE_DIRECTPATH_TX_QUEUE) || !ENABLE_DIRECTPATH_TX_QUEUE
                for (i = 0; i < MAX_PPA_PUSHTHROUGH_DEVICES; i++)
                    if ( ppa_virt_rx_devs[i] )
                        avm_pa_rx_channel_resume( ppa_virt_rx_devs[i]->pid_handle );
#endif
            }
            local_irq_restore(sys_flag);
#if defined(ENABLE_DIRECTPATH_TX_QUEUE) && ENABLE_DIRECTPATH_TX_QUEUE
            send_directpath_tx_queue();
#endif
            spin_unlock_irqrestore(&g_directpath_tx_spinlock, sys_flag);
        }
        else
        {
            err("incorrect DMA TX channel: %d (0 - 1 is reserved for fast path)", dma_ch_no);
        }
        break;
    default:
        err("unkown DMA interrupt event - %d", status);
    }

    return ret;
}
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

static void dump_atm_tx_desc( struct tx_descriptor *d ){
#if defined(CONFIG_AVMNET_DEBUG) 
	if (!(g_dbg_datapath & DBG_ENABLE_MASK_DUMP_ATM_TX))
		return;

	printk(KERN_ERR "[%s] called by %pF", __FUNCTION__, __builtin_return_address(0));
	printk(KERN_ERR "   own:     %d\n", d->own);
	printk(KERN_ERR "   c:       %d\n", d->c);
	printk(KERN_ERR "   sop:     %d\n", d->sop);
	printk(KERN_ERR "   eop:     %d\n", d->eop);
	printk(KERN_ERR "   byteoff: %d\n", d->byteoff);
	printk(KERN_ERR "   mpoa_pt: %d\n", d->mpoa_pt);
	printk(KERN_ERR "   mpoa_ty: %d\n", d->mpoa_type);
	printk(KERN_ERR "   pdu_ty:  %d\n", d->pdu_type);
	printk(KERN_ERR "   qid:     %d\n", d->qid);
	printk(KERN_ERR "   datalen: %d\n", d->datalen);
#endif
}

/*------------------------------------------------------------------------------------------*\
 * wird auschliesslich von der RX-Routine verwendet
\*------------------------------------------------------------------------------------------*/
void ifx_ppa_atm_hard_start_xmit(struct sk_buff *skb,
		struct flag_header *fheader) {
	unsigned long sys_flag;
	volatile struct tx_descriptor *desc;
	struct tx_descriptor reg_desc;
	struct sk_buff *skb_to_free;
	int byteoff;
	int conn;

//	conn = (header_val >> 16) & 0x07;
	conn = fheader->qid & 0x07;
	dbg_trace_ppa_data("atm start xmit");

#if defined(ENABLE_DFE_LOOPBACK_TEST_INDIRECT) && ENABLE_DFE_LOOPBACK_TEST_INDIRECT
	{
		unsigned char *p = (unsigned char *)skb->data;

		if ( WRX_QUEUE_CONFIG(conn)->mpoa_type < 2 ) //  EoA
		{
			unsigned char mac[6];

			//  mac
			memcpy(mac, p, 6);
			memcpy(p, p + 6, 6);
			memcpy(p + 6, mac, 6);
#ifndef CONFIG_MIPS_UNCACHED
			dma_cache_wback((unsigned long)p, 12);
#endif
		}
		if ( fheader->mpoa_type != 2 ) //  EoA or IPoA
		{
			unsigned char ip[4];
			unsigned char port[2];

			//  IP
			p += fheader->ip_offset + 8; //hier nicht ip_inner_offset
			memcpy(ip, p + 4, 4);
			memcpy(p + 4, p + 8, 4);
			memcpy(p + 8, ip, 4);

			//  port
			if ( p[1] == 0x06 || p[1] == 0x11 )//  TCP or UDP
			{
				memcpy(port, p + 12, 2);
				memcpy(p + 12, p + 14, 2);
				memcpy(p + 14, port, 2);
			}
#ifndef CONFIG_MIPS_UNCACHED
			dma_cache_wback((unsigned long)p, 22);
#endif
		}
	}
#endif

	byteoff = (u32) skb->data & (DMA_TX_ALIGNMENT - 1);
	ASSERT(skb_headroom(skb) >= sizeof(struct sk_buff *) + byteoff,
				"   skb_headroom < sizeof(struct sk_buff *) +  byteoff \n");

#if defined(CONFIG_AVMNET_DEBUG) 
	if ( g_dbg_datapath & DBG_ENABLE_MASK_DEBUG_ALIGNMENT )
		printk( KERN_ERR
			"\n"
			"   skb_headroom :   %d \n"
			"   sizeof(struct sk_buff *): %d \n"
			"   byteoff:         %d \n"
			"   skb->head:       %p\n"
			"   skb->data:       %p\n"
			"   in_dev:          %s\n",
				skb_headroom(skb),
				sizeof(struct sk_buff *),
				byteoff,
				skb->head,
				skb->data,
				skb->input_dev?skb->input_dev->name:"no_input_device");
#endif

	*(struct sk_buff **) ((u32) skb->data - byteoff - sizeof(struct sk_buff *)) =
			skb;
#ifndef CONFIG_MIPS_UNCACHED
	/*  write back to physical memory   */
	dma_cache_wback((u32)skb->data - byteoff - sizeof(struct sk_buff *),
			sizeof(struct sk_buff *));
#endif

	/*  allocate descriptor */
    spin_lock_irqsave(&g_cpu_to_wan_tx_spinlock, sys_flag);
	desc = CPU_TO_WAN_TX_DESC_BASE + g_cpu_to_wan_tx_desc_pos;
	if (!desc->own) //  PPE hold
	{
        spin_unlock_irqrestore(&g_cpu_to_wan_tx_spinlock, sys_flag);
		err("PPE hold");
		dev_kfree_skb_any(skb);
		return;
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

	if ((reg_desc.dataptr & (DMA_TX_ALIGNMENT - 1)) == 0)
		dma_alignment_atm_good_count++;
	else
		dma_alignment_atm_bad_count++;

	free_skb_clear_dataptr(&reg_desc);
	put_skb_to_dbg_pool(skb);

	/*  update descriptor   */
	reg_desc.dataptr = (u32) skb->data & (0x1FFFFFFF ^ (DMA_TX_ALIGNMENT - 1)); //  byte address
	reg_desc.byteoff = byteoff;
	reg_desc.datalen = skb->len;
	reg_desc.mpoa_pt = 0; //  firmware apply MPoA
	reg_desc.mpoa_type = WRX_QUEUE_CONFIG(conn)->mpoa_type;
	reg_desc.pdu_type = 0; //  AAL5
	reg_desc.qid = conn;
	reg_desc.own = 0;
	reg_desc.c = 0;
	reg_desc.sop = reg_desc.eop = 1;

	dump_atm_tx_desc (&reg_desc) ;
#if 0
	/*  update MIB  */
	UPDATE_VCC_STAT(conn, tx_packets, 1);
	UPDATE_VCC_STAT(conn, tx_bytes, skb->len);
#endif

#if defined(CONFIG_AVMNET_DEBUG) 
	if (g_dbg_datapath & DBG_ENABLE_MASK_DUMP_ATM_TX)
		dump_skb(skb, DUMP_SKB_LEN, "ppe_send", 7, 0, 1, 1);
#endif

	/*  write discriptor to memory and write back cache */
	*((volatile u32 *) desc + 1) = *((u32 *) &reg_desc + 1);
	*(volatile u32 *) desc = *(u32 *) &reg_desc;

	adsl_led_flash();
	return;

}

/*------------------------------------------------------------------------------------------*\
 * merged handler
\*------------------------------------------------------------------------------------------*/
static INLINE int dma_rx_int_handler(struct dma_device_info *dma_dev, int dma_ch_no) {
	struct sk_buff *skb = NULL;
	u8 *buf;
	int len;
	struct flag_header *fheader __attribute__((unused));
    struct softnet_data *queue;
    unsigned long flags;

#if defined(TRACE_RX_PATH)
	static int once = 5;
	if (once) {
		dump_stack();
		once--;
	}
#endif
	dbg("[%s] s\n", __FUNCTION__);

	len = dma_device_read(dma_dev, &buf, (void **) &skb, dma_ch_no);
    if (!skb)
		return 0;

    if(unlikely(g_stop_datapath)){
        dbg("[%s]: datapath to be stopped, dropping skb.\n", __func__);
        dev_kfree_skb_any(skb);
        return 0;
    }

    get_skb_from_dbg_pool(skb);

    skb_put(skb, len);
//	skb->tail = skb->data + len;
//	skb->len = len;

	dump_skb(skb, DUMP_SKB_LEN, "dma_rx_int_handler raw data", 0, dma_ch_no, 0, 0);
	dump_flag_header(g_fwcode, (struct flag_header *) skb->data, __FUNCTION__, dma_ch_no, 0);

    local_irq_save(flags);
    queue = &__get_cpu_var(g_ar9_eth_queue);

    if(likely(queue->input_pkt_queue.qlen <= 1000)){
        if(unlikely(queue->input_pkt_queue.qlen == 0)){
            napi_schedule(&queue->backlog);
        }

        __skb_queue_tail(&queue->input_pkt_queue, skb);
    } else {
        dbg("[%s] Backlog queue full, dropping skb.\n", __func__);
        dev_kfree_skb_any(skb);
    }

    local_irq_restore(flags);

    return 0;
}

static int process_backlog_a5(struct napi_struct *napi, int quota)
{
    struct sk_buff *skb = NULL;
    int work = 0;
    struct softnet_data *queue = &__get_cpu_var(g_ar9_eth_queue);
    unsigned long start_time = jiffies;
    unsigned long flags;

    napi->weight = 64;
    do{
        local_irq_save(flags);
        skb = __skb_dequeue(&queue->input_pkt_queue);
        if (!skb) {
            __napi_complete(napi);
            local_irq_restore(flags);
            break;
        }
        local_irq_restore(flags);

        handle_skb_a5(skb);
    }while (++work < quota && jiffies == start_time);

    return work;
}

static INLINE int handle_skb_a5(struct sk_buff *skb)
{
    struct flag_header *fheader;
    struct ar9_pmac_header *pheader;
    avmnet_device_t *avm_dev = NULL;
    int conn;
    int hw_dev_id;
    int dump_enforce __attribute__((unused));

	fheader = (struct flag_header *) skb->data;

	ASSERT(skb->len >= 60 + STD_BYTE_OFF + fheader->pl_byteoff, "packet is too small: %d", skb->len);

	skb_pull(skb, STD_BYTE_OFF + fheader->pl_byteoff);
//	skb->data += STD_BYTE_OFF + fheader->pl_byteoff;
//	skb->tail = skb->data + len;
//	skb->len = len;

	pheader = (struct ar9_pmac_header *) (skb->data - sizeof(struct ar9_pmac_header));
    dump_ar9_pmac_header(pheader, __FUNCTION__);

    //PPA_PORT_SEPARATION_RX(header,skb,lan_port_seperate_enabled,wan_port_seperate_enabled);
    
    skb->priority = fheader->tc;

#if defined(CONFIG_AVMNET_DEBUG) 
    dump_enforce = 0;
    if ((g_dbg_datapath & DBG_ENABLE_MASK_DUMP_ATM_RX) && (( fheader->src_itf == 6 ) || ( fheader->src_itf == 7 ) ))
		dump_enforce = 1;
#endif

//	dump_skb(skb, DUMP_SKB_LEN, "dma_rx_int_handler packet data", fheader->src_itf, dma_ch_no, 0, dump_enforce);

	//  implement indirect fast path
	if (fheader->acc_done && fheader->dest_list) //  acc_done == 1 && dest_list != 0
	{
		//  Current implementation send packet to highest priority dest only (0 -> 7, 0 is highest)
		//  2 is CPU, so we ignore it here
		if ((fheader->dest_list & (3 << 0))) //  0 - eth0; 1 - eth1
		{
#if 0
		    int itf = (fheader->dest_list & (1 << 1)) ? 1 : 0;
		    if ( (fheader->dest_list & g_wan_itf) ) {
		        skb->cb[13] = 0x5A; //  magic number indicating directpath
		        skb->cb[15] = fheader->qid & 0x07;
		        ifx_ppa_eth_qos_hard_start_xmit_a5(skb, g_eth_net_dev[itf]);
		    } else
			    eth_xmit(skb, itf, DMA_CHAN_NR_DEFAULT_CPU_TX, 2, g_wan_queue_class_map[fheader->qid & 0x07]);
#else
           	dev_kfree_skb_any(skb);
		    err("This Path is not adopted to AVM Datapath, yet");

#endif
			return 0;		
		} else if ((fheader->dest_list & (1 << 7))) //  7 - DSL
		{
			ifx_ppa_atm_hard_start_xmit(skb, fheader);
			return 0;
		} else {
			//  fill MAC info into skb, in case the driver hooked to direct path need such info

			int dst_id;

			for (dst_id = 3; dst_id < 3 + MAX_PPA_PUSHTHROUGH_DEVICES; dst_id++) {
				if ((fheader->dest_list & (1 << dst_id))) //  3...5 (einschliesslich 3 u. 5)
				{
				    int if_id = HW_DEV_ID(dst_id);
	                if ( ifx_lookup_routing_entry && ppa_virt_tx_devs[ if_id ] ){
        				int rout_index;
	                    avm_session_handle avm_pa_session;

						//TODO ab hier ist mit adsl dazugekommen
						//  detect ATM port and remove the tag having PVC info
						if (fheader->src_itf == 7 && fheader->is_udp == 0
								&& fheader->is_tcp == 0 && skb->data[12] == 0x81
								&& skb->data[13] == 0x00
								&& (skb->data[14] & 0x0F) == 0x0F
								&& (skb->data[15] & 0xF0) == 0xF0) {
							int i;
							dbg("remove outer VLAN tag");
							//  remove outer VLAN tag
							for (i = 11; i >= 0; i--)
								skb->data[i + 4] = skb->data[i];
							skb_pull(skb, 4);
						}
						//TODO bis hier ist mit adsl dazugekommen

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
    					   // printk(KERN_ERR "colission session %d", rout_index);

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
	switch (fheader->src_itf) {
	case 0: //  incomming LAN IF ( vom Switch klassifizerit )
	case 1: //  incomming WAN IF ( vom Switch klassifizerit )
        avm_dev = mac_nr_to_avm_dev(pheader->srcPortID);
		if (avm_dev && avm_dev->device && netif_running(avm_dev->device)) {
			//  do not need to remove CRC
			//skb->tail -= ETH_CRC_LENGTH;
			//skb->len  -= ETH_CRC_LENGTH;
			unsigned int keep_len = 0;
            avmnet_netdev_priv_t *priv = netdev_priv(avm_dev->device);

			dbg("got receive if: avm_name:%s ifname:%s\n",
					avm_dev->device_name, avm_dev->device->name);
			skb->dev = avm_dev->device;
#           if defined(CONFIG_IP_MULTICAST_FASTFORWARD)
			if(likely(priv->mcfw_used)){
                mcfw_portset set;
                mcfw_portset_reset(&set);
                mcfw_portset_port_add(&set, 0);
                (void) mcfw_snoop_recv(&priv->mcfw_netdrv, set, skb);
            }
#           endif /*--- #if defined(CONFIG_IP_MULTICAST_FASTFORWARD) ---*/
			skb->protocol = eth_type_trans(skb, avm_dev->device);

			if (skb->len > 0)
				keep_len = skb->len;
			else
				dbg(KERN_ERR "[%s] skb->len <= 0: %d", __FUNCTION__, skb->len);

			avm_dev->device_stats.rx_packets++;
			avm_dev->device_stats.rx_bytes += keep_len;

			if(likely(avm_pa_dev_receive(AVM_PA_DEVINFO(avm_dev->device), skb) == AVM_PA_RX_ACCELERATED)) {
			    avm_dev->device->last_rx = jiffies;
			    return 0;
			}

            netif_receive_skb(skb);
			return 0;
		} else {
			dbg(KERN_ERR "[%s] netif %d down drop packet\n", __FUNCTION__, fheader->src_itf);
		}
		break;
	case 6: // ATM (IPoA, PPPoA)
	case 7: // ATM (EoA)

		if (fheader->src_itf == 6) {
			conn = skb->data[11] & 0x3F; //  QID: 6 bits
			dbg("src_itf = 6; conn = %d", conn);
		} else {
			int i;

			ASSERT(skb->data[12] == 0x81 && skb->data[13] == 0x00, "EoA traffic: no outer VLAN for PVCs differentiation");
			//  outer VLAN
#if defined(ENABLE_CONFIGURABLE_DSL_VLAN) && ENABLE_CONFIGURABLE_DSL_VLAN

			if (skb->data[14] != 0x0F || (skb->data[15] & 0xF0) != 0xF0) {
				unsigned short vlan_id = ((unsigned short) skb->data[14] << 8) | (unsigned short) skb->data[15];
				for (i = 0; i < NUM_ENTITY(g_dsl_vlan_qid_map); i += 2)
					if (g_dsl_vlan_qid_map[i] == vlan_id) {
						skb->data[15] = (unsigned char) g_dsl_vlan_qid_map[i + 1];
						dbg("setup vlan id");
						break;
					}
				ASSERT(i < NUM_ENTITY(g_dsl_vlan_qid_map), "search DSL VLAN table fail");
			}
#endif
			conn = skb->data[15] & 0x0F;
			//  remove outer VLAN tag (die ersten 12 Byte werden um 4 byte nach hinten geschoben (ueber skb_data[12;15])
			dbg("remove outer VLAN tag II");
			for (i = 11; i >= 0; i--)
				skb->data[i + 4] = skb->data[i];
			skb_pull(skb, 4);
		}
		if (conn < 0 || conn >= ATM_QUEUE_NUMBER) {
			dbg("DMA channel %d, ATM conn (%d) is out of range 0 - %d", fheader->src_itf, conn, ATM_QUEUE_NUMBER - 1);
			break;
		}
		if ( g_atm_priv_data.connection[conn].vcc != NULL ) {
			dbg_trace_ppa_data("conn = %d, vcc = %p", conn, g_atm_priv_data.connection[conn].vcc );
			if (g_atm_priv_data.connection[conn].vcc && atm_charge( g_atm_priv_data.connection[conn].vcc, skb->truesize)) {
                //err("reminder: how to handle raw packet without flag aal5_raw?");
				//ASSERT(vcc->qos.aal == ATM_AAL5 || (header_val & (1 << 23)), "ETH packet comes in from ATM port non-AAL5 VC");

				ATM_SKB(skb)->vcc = g_atm_priv_data.connection[conn].vcc;

				//if ( (header_val & (1 << 23)) )
				//{
				//    /*  AAL 5 raw packet    */
				//    //skb->tail -= ETH_CRC_LENGTH;
				//    //skb->len  -= ETH_CRC_LENGTH;
				//}
				//else
				{
					/*  ETH packet, need recover ATM encapsulation  */
					if (WRX_QUEUE_CONFIG(conn)->mpoa_mode) {
						unsigned int proto_type;
						dbg("mpoa_mode, type=%d", WRX_QUEUE_CONFIG(conn)->mpoa_type);

						/*  LLC */
						switch (WRX_QUEUE_CONFIG(conn)->mpoa_type) {

						case 0: //  EoA w/o FCS
							ASSERT(
									skb_headroom(skb) >= 10,
									"not enough skb headroom (LLC EoA w/o FCS)");
							ASSERT(
									((u32)skb->data & 0x03) == 2,
									"not aligned data pointer (LLC EoA w/o FCS)");
							skb->data -= 10;
							skb->len += 10;
							//skb->tail -= ETH_CRC_LENGTH;
							//skb->len  += 10 - ETH_CRC_LENGTH;
							((u32 *) skb->data)[0] = 0xAAAA0300;
							((u32 *) skb->data)[1] = 0x80C20007;
							((u16 *) skb->data)[4] = 0x0000;
							break;
						case 1: //  EoA w FCS
							ASSERT(skb_headroom(skb) >= 10,
									"not enough skb headroom (LLC EoA w FCS)");
							ASSERT(((u32)skb->data & 0x03) == 2,
									"not aligned data pointer (LLC EoA w FCS)");
							skb->data -= 10;
							skb->len += 10;
							((u32 *) skb->data)[0] = 0xAAAA0300;
							((u32 *) skb->data)[1] = 0x80C20001;
							((u16 *) skb->data)[4] = 0x0000;
							break;
						case 2: //  PPPoA
							switch ((proto_type = ntohs(
									*(u16 *) ( skb->data + 12 )))) {
							case 0x0800: // EtherType=IPv4-Tag
								dbg("proto_type %#x", proto_type);
								proto_type = 0x0021;
								break;
							case 0x86DD: // EtherType=IPv6-Tag
								dbg("proto_type %#x", proto_type);
								proto_type = 0x0057;
								break;
							}
							dbg("final proto_type %#x", proto_type);
							ASSERT(
									fheader->ip_inner_offset - 6 >= 0 || skb_headroom(skb) >= 6 - fheader->ip_inner_offset,
									"not enough skb headroom (LLC PPPoA)");
							skb->data += 0x0E - 6;
							skb->len -= 0x0E - 6;
							//skb->tail -= ETH_CRC_LENGTH;
							//skb->len  -= 0x0E - 6 + ETH_CRC_LENGTH;
							ASSERT(((u32)skb->data & 0x03) == 2,
									"not aligned data pointer (LLC PPPoA)");
							((u32 *) skb->data)[0] = 0xFEFE03CF;
							((u16 *) skb->data)[2] = (u16) proto_type;
							break;
						case 3: //  IPoA
							ASSERT(
									fheader->ip_inner_offset - 8 >= 0 || skb_headroom(skb) >= 8 - fheader->ip_inner_offset,
									"not enough skb headroom (LLC IPoA)");
							skb->data += fheader->ip_inner_offset - 8;
							skb->len -= fheader->ip_inner_offset - 8;
							//skb->tail -= ETH_CRC_LENGTH;
							//skb->len  -= header->ip_inner_offset - 8 + ETH_CRC_LENGTH;
							ASSERT(((u32)skb->data & 0x03) == 0,
									"not aligned data pointer (LLC IPoA)");
							((u32 *) skb->data)[0] = 0xAAAA0300;
							//((u32 *)skb->data)[1] = header->is_ipv4 ? 0x00000800 : 0x000086DD;
							((u16 *) skb->data)[2] = 0x0000;
							break;
						}
					} else {
						unsigned int proto_type;

						dbg("no mpoa_mode, type=%d, ip_inner_offset=%d",
								WRX_QUEUE_CONFIG(conn)->mpoa_type, fheader->ip_inner_offset);
						switch (WRX_QUEUE_CONFIG(conn)->mpoa_type) {
						case 0: //  EoA w/o FCS
							ASSERT(
									skb_headroom(skb) >= 2,
									"not enough skb headroom (VC-mux EoA w/o FCS)");
							ASSERT(
									((u32)skb->data & 0x03) == 2,
									"not aligned data pointer (VC-mux EoA w/o FCS)");
							skb->data -= 2;
							skb->len += 2;
							//skb->tail -= ETH_CRC_LENGTH;
							//skb->len  += 2 - ETH_CRC_LENGTH;
							*(u16 *) skb->data = 0x0000;
							break;
						case 1: //  EoA w FCS
							ASSERT(
									skb_headroom(skb) >= 2,
									"not enough skb headroom (VC-mux EoA w FCS)");
							ASSERT(
									((u32)skb->data & 0x03) == 2,
									"not aligned data pointer (VC-mux EoA w FCS)");
							skb->data -= 2;
							skb->len += 2;
							*(u16 *) skb->data = 0x0000;
							break;
						case 2: //  PPPoA
							dbg("got PPPoA");
							switch ((proto_type = ntohs(
									*(u16 *) (skb->data + 12)))) {
							case 0x0800: // EtherType=IPv4-Tag
								proto_type = 0x0021;
								break;
							case 0x86DD: // EtherType=IPv6-Tag
								proto_type = 0x0057;
								break;
							}
							dbg("got PPPoA: proto_type=%#x", proto_type);
							ASSERT(
									fheader->ip_inner_offset - 2 >= 0 || skb_headroom(skb) >= 2 - fheader->ip_inner_offset,
									"not enough skb headroom (VC-mux PPPoA)");
							skb->data += 0x0E - 2;
							skb->len -= 0x0E - 2;
							// skb->tail -= ETH_CRC_LENGTH;
							// skb->len  -= 0x0E - 2 + ETH_CRC_LENGTH;
							ASSERT(((u32)skb->data & 0x03) == 2,
									"not aligned data pointer (VC-mux PPPoA)");
							*(u16 *) skb->data = (u16) proto_type;
							break;
						case 3: //  IPoA oder RAW-Packet fuer kdsld-probe
							if ( fheader->ip_outer_offset ) {
								skb->data += fheader->ip_outer_offset ;
								skb->len -= fheader->ip_outer_offset ;
								//skb->tail -= ETH_CRC_LENGTH;
								//skb->len  -= header->ip_outer_offset + ETH_CRC_LENGTH;
							} else {
								int default_raw_offset = 14;
								skb->data += default_raw_offset;
								skb->len -= default_raw_offset;
								ASSERT( (skb->len == fheader->pkt_len - 4), "raw offset does not fit to packet length" );
								dbg("seems to be raw packet, default_raw_offset=%d (current ip_inner_offset = %d, ip_outer_offset=%d)", default_raw_offset, fheader->ip_inner_offset, fheader->ip_outer_offset);
							}
							break;
						}
					}
					dump_skb(skb, DUMP_SKB_LEN, "dma_rx_int_handler AAL5 encapsulated", fheader->src_itf, -1, 0, dump_enforce);
				}

				g_atm_priv_data.connection[conn].access_time =
						current_kernel_time();

				adsl_led_flash();

				if (g_atm_priv_data.connection[conn].vcc && g_atm_priv_data.connection[conn].vcc->stats)
					atomic_inc(&g_atm_priv_data.connection[conn].vcc->stats->rx);

				if (g_atm_priv_data.connection[conn].vcc && g_atm_priv_data.connection[conn].vcc->qos.aal == ATM_AAL5)
					g_atm_priv_data.wrx_pdu++;

				UPDATE_VCC_STAT(conn, rx_packets, 1);
				UPDATE_VCC_STAT(conn, rx_bytes, skb->len);

				if (g_atm_priv_data.connection[conn].vcc && g_atm_priv_data.connection[conn].vcc->push) {
                    dbg_trace_ppa_data("push, %pF", g_atm_priv_data.connection[conn].vcc->push);
    				g_atm_priv_data.connection[conn].vcc->push(g_atm_priv_data.connection[conn].vcc, skb);
                } else {
                    dbg_trace_ppa_data("dropping packet -> vcc seesm to be closed");
	                dev_kfree_skb_any(skb);
                }
				return 0;
			} else {
				dbg_trace_ppa_data("inactive qid %d", conn);

				if ( g_atm_priv_data.connection[conn].vcc && g_atm_priv_data.connection[conn].vcc->stats)
					atomic_inc( &g_atm_priv_data.connection[conn].vcc->stats->rx_drop );

				if ( g_atm_priv_data.connection[conn].vcc && g_atm_priv_data.connection[conn].vcc->qos.aal == ATM_AAL5)
					g_atm_priv_data.wrx_drop_pdu++;

				UPDATE_VCC_STAT(conn, rx_dropped, 1);
			}
		}

		break;
	case 3: //  WLAN
	default: //  other interface receive
		hw_dev_id = HW_DEV_ID(fheader->src_itf);

        if (hw_dev_id < ARRAY_SIZE(ppa_virt_rx_devs) && ppa_virt_rx_devs[hw_dev_id]){
			dbg_trace_ppa_data("PPE_DIRECTPATH_DATA_RX_ENABLE: src_itf=%d", fheader->src_itf );

            ppa_virt_rx_devs[hw_dev_id]->hw_pkt_slow_cnt ++;
            avm_pa_rx_channel_packet_not_accelerated(ppa_virt_rx_devs[hw_dev_id]->pid_handle, skb);
			return 0;
		}else {
			dbg_trace_ppa_data("!PPE_DIRECTPATH_DATA_RX_ENABLE: src_itf=%d", fheader->src_itf );
		}
	}

	if (avm_dev)
		avm_dev->device_stats.rx_dropped++;
	dev_kfree_skb_any(skb);
	return 0;
}


static INLINE u16 smi_reg_read(u16 reg)
//  reg is ((phy_addr & 0x1F) << 5) | (reg_addr & 0x1F)
{
    while ( (*AMAZON_S_SW_MDIO_CTL & 0x8000) )
        udelay(100);
#ifdef CONFIG_LTQ_VB300
    //  force phy address to be PHY11G_ADDR
    *AMAZON_S_SW_MDIO_CTL = MDIO_OP_READ | (reg & 0x001F) | 0x8000 | (PHY11G_ADDR << 5);
#else
    *AMAZON_S_SW_MDIO_CTL = MDIO_OP_READ | (reg & 0x03FF) | 0x8000;
#endif
    udelay(200);
    while ( (*AMAZON_S_SW_MDIO_CTL & 0x8000) )
        udelay(100);
    return (u16)(*AMAZON_S_SW_MDIO_DATA & 0x0000FFFF);
}

static INLINE void smi_reg_write(u16 reg, u16 data)
//  reg is ((phy_addr & 0x1F) << 5) | (reg_addr & 0x1F)
{
    while ( (*AMAZON_S_SW_MDIO_CTL & 0x8000) )
        udelay(100);
#ifdef CONFIG_LTQ_VB300
    //  force phy address to be PHY11G_ADDR
    *AMAZON_S_SW_MDIO_CTL = (data << 16) | MDIO_OP_WRITE | (reg & 0x001F) | 0x8000 | (PHY11G_ADDR << 5);
#else
    *AMAZON_S_SW_MDIO_CTL = (data << 16) | MDIO_OP_WRITE | (reg & 0x03FF) | 0x8000;
#endif
    udelay(200);
    while ( (*AMAZON_S_SW_MDIO_CTL & 0x8000) )
        udelay(100);
}

static INLINE void init_external_tantos(void)
{
    u16 chipid;
    u16 value;

    IFX_SWITCH_PIN_RESERVE(SWITCH_MDIO);
    IFX_SWITCH_ALTSEL0_SET(SWITCH_MDIO);
    IFX_SWITCH_ALTSEL1_CLR(SWITCH_MDIO);
    IFX_SWITCH_DIR_OUT(SWITCH_MDIO);
    IFX_SWITCH_OD_SET(SWITCH_MDIO);

    IFX_SWITCH_PIN_RESERVE(SWITCH_MDC);
    IFX_SWITCH_ALTSEL0_SET(SWITCH_MDC);
    IFX_SWITCH_ALTSEL1_CLR(SWITCH_MDC);
    IFX_SWITCH_DIR_OUT(SWITCH_MDC);
    IFX_SWITCH_OD_SET(SWITCH_MDC);

    chipid = (unsigned short)(smi_reg_read(0x0101));
    if (chipid == TANTOS_CHIP_ID)
    {
        smi_reg_write(0x00a1, 0x0004);  //  PORT 5
        udelay(500);    //  fix me
        value = smi_reg_read(0x00a1);
        if ( value != 0x0004 )
            err("error: port 5 force link up failed");

        smi_reg_write(0x00c1, 0x0004);  //  PORT 6
        udelay(500);    //  fix me
        value = smi_reg_read(0x00c1);
        if ( value != 0x0004 )
            err("error: port 6 force link up failed");

#if defined(ENABLE_SWITCH_FLOW_CONTROL) && ENABLE_SWITCH_FLOW_CONTROL
        smi_reg_write(0x00f5, 0x0bbb);  //  enable flow control
        udelay(500);    //  fix me
        value = smi_reg_read(0x00f5);
        if ( value != 0x0bbb )
            err("error: enable flow control failed");
#else
        smi_reg_write(0x00f5, 0x0aaa);  //  disable flow control
        udelay(500);
        value = smi_reg_read(0x00f5);
        if ( value != 0x0aaa )
            err("error: disable flow control failed");
#endif

        //  Set Bypass mode
        //    P0 portbase vlan map reg
        value = (unsigned short)(smi_reg_read(0x03));
        smi_reg_write(0x03, (value| 0x80));
        //    P1 portbase vlan map reg
        value = (unsigned short)(smi_reg_read(0x23));
        smi_reg_write(0x23, (value| 0x80));
        //    P2 portbase vlan map reg
        value = (unsigned short)(smi_reg_read(0x43));
        smi_reg_write(0x43, (value| 0x80));
        //    P3 portbase vlan map reg
        value = (unsigned short)(smi_reg_read(0x63));
        smi_reg_write(0x63, (value| 0x80));
        //    P4 portbase vlan map reg
        value = (unsigned short)(smi_reg_read(0x83));
        smi_reg_write(0x83, (value| 0x80));
        //    P5 portbase vlan map reg
        value = (unsigned short)(smi_reg_read(0xa3));
        smi_reg_write(0xa3, (value| 0x80));
        //    P6 portbase vlan map reg
        value = (unsigned short)(smi_reg_read(0xc3));
        smi_reg_write(0xc3, (value| 0x80));
    }
    else
    {
        //  TODO:
        //smi_reg_write(0x0001, 0x840f);
        //smi_reg_write(0x0003, 0x840f);
        //smi_reg_write(0x0005, 0x840f);
        //smi_reg_write(0x0006, 0x840f);
        //smi_reg_write(0x0008, 0x840f);
        //smi_reg_write(0x0012, 0x3602);
        //smi_reg_write(0x0033, 0x4000);  //  clockout2 @25MHz
    }
}

#ifdef CONFIG_MII1_AUTONEG

static INLINE void mii1_gphy_config_gpio(void)
{
  #if MII1_GPHY_INT_EXT_INT_NO == 0
    IFX_SWITCH_PIN_RESERVE(0);
    IFX_SWITCH_ALTSEL0_SET(0);
    IFX_SWITCH_ALTSEL1_CLR(0);
    IFX_SWITCH_DIR_IN(0);
  #elif MII1_GPHY_INT_EXT_INT_NO == 1
    IFX_SWITCH_PIN_RESERVE(1);
    IFX_SWITCH_ALTSEL0_SET(1);
    IFX_SWITCH_ALTSEL1_CLR(1);
    IFX_SWITCH_DIR_IN(1);
  #elif MII1_GPHY_INT_EXT_INT_NO == 2
    IFX_SWITCH_PIN_RESERVE(2);
    IFX_SWITCH_ALTSEL0_CLR(2);
    IFX_SWITCH_ALTSEL1_SET(2);
    IFX_SWITCH_DIR_IN(2);
  #elif MII1_GPHY_INT_EXT_INT_NO == 3
    IFX_SWITCH_PIN_RESERVE(39);
    IFX_SWITCH_ALTSEL0_CLR(39);
    IFX_SWITCH_ALTSEL1_SET(39);
    IFX_SWITCH_DIR_IN(39);
  #elif MII1_GPHY_INT_EXT_INT_NO == 4
    IFX_SWITCH_PIN_RESERVE(10);
    IFX_SWITCH_ALTSEL0_SET(10);
    IFX_SWITCH_ALTSEL1_SET(10);
    IFX_SWITCH_DIR_IN(10);
  #elif MII1_GPHY_INT_EXT_INT_NO == 5
    IFX_SWITCH_PIN_RESERVE(9);
    IFX_SWITCH_ALTSEL0_SET(9);
    IFX_SWITCH_ALTSEL1_SET(9);
    IFX_SWITCH_DIR_IN(9);
  #else
    #error ERROR EXT INT NO
  #endif
}

static INLINE void mii1_gphy_detect(void)
{
    u16 id;

    id = smi_reg_read((0x11 << 5) | 0x03);
    //printk("\n mii1_gphy_config_gphy id=%x id-decimal=%d",((id >> 4) & 0x000f ), ((id >> 4) & 0x000f ));
    switch ((id >> 4) & 0x1f)
    {
    case 2:
        g_id = GPHY_VSC;
        printk("\n GPHY_VSC \n");
        break;
    case 0: //  V1.4
    case 13: // V1.3
        g_id = GPHY_LANTIQ_11G;
        printk("\n GPHY_LANTIQ \n");
        break;
    default:
        printk("\n unknown ID! \n");
    }
}

static INLINE void mii1_gphy_config_gphy(void)
{
    if ( g_id == GPHY_VSC )
        smi_reg_write(0x0239, 0xF000);
    else
        smi_reg_write(0x0239, 0x0007);
}

static INLINE void mii1_gphy_config_ext_int(void)
{
    if ( g_id == GPHY_VSC )
        *AMAZON_S_EIU_EXIN_C |= 0x2 << (MII1_GPHY_INT_EXT_INT_NO * 4);  //  Falling Edge
    else
        *AMAZON_S_EIU_EXIN_C |= 0x6 << (MII1_GPHY_INT_EXT_INT_NO * 4);  //  Low Level
    *AMAZON_S_EIU_INEN   |= 1 << MII1_GPHY_INT_EXT_INT_NO;          //  Enable External interrupt
}

static irqreturn_t mii1_gphy_11G_int_handler(int irq, void *dev_id)
{
    u16 status, linkReg, speedReg;

    status = smi_reg_read((0x11 << 5) | 0x1A);  //  read data from phy register to clear phy interrupts
    if ( (status & 0x0007) )  //  interrupt due to link status, speed, full duplex
    {
        linkReg = smi_reg_read((0x11 << 5) | 0x01);
        if ( (linkReg & 0x04) == 0 )    //  link down
        {
            dbg("MII1 - link down");
            *AMAZON_S_SW_P1_CTL = (*AMAZON_S_SW_P1_CTL & ~(1 << 18)) | (1 << 17);   //  force link down
            g_link_up = 0;
        }
        else
        {
            speedReg = smi_reg_read((0x11 << 5) | 0x18);
            dbg("MII1 - link up, speedReg = %04x", (u32)speedReg);

            if ( (speedReg & 0x08) )
            {
                dbg("MII1 - full duplex");
                *AMAZON_S_SW_RGMII_CTL |= 1 << 11;
                g_link_duplex = 1;
            }
            else
            {
                dbg("MII1 - half duplex");
                *AMAZON_S_SW_RGMII_CTL &= ~(1 << 11);
                g_link_duplex = 0;
            }

            switch ( (speedReg  & 0x03) )
            {
            case 0:
                dbg("MII1 - 10BASE-T");
                *AMAZON_S_SW_RGMII_CTL &= 0xffffcfff;
                g_link_speed = 10;
                break;
            case 1:
                dbg("MII1 - 100BASE-TX");
                *AMAZON_S_SW_RGMII_CTL &= 0xffffcfff;
                *AMAZON_S_SW_RGMII_CTL |= 1 << 12;
                g_link_speed = 100;
                break;
            case 2:
                dbg("MII1 - 1000BASE-T");
                *AMAZON_S_SW_RGMII_CTL &= 0xffffcfff;
                *AMAZON_S_SW_RGMII_CTL |= 2 << 12;
                g_link_speed = 1000;
                break;
            case 3:
                err("MII1 - unknown link speed");
                g_link_speed = 0;
                break;
            }

            if ( g_link_speed )
            {
                *AMAZON_S_SW_P1_CTL &= ~(1 << 17);  //  disable force link down
                //*AMAZON_S_SW_P1_CTL |= 0x00040001;  //  the output packet keep the same format as receive, force link up
                *AMAZON_S_SW_P1_CTL &= ~1;          //  follow VLAN rules, and remove tag
                *AMAZON_S_SW_P1_CTL |= 0x00040000;  //  force link up
            }
        }
    }

    return IRQ_HANDLED;
}

static irqreturn_t mii1_gphy_VSC_int_handler(int irq, void *dev_id)
{
    u16 status, linkReg, speedReg;

    status = smi_reg_read((0x11 << 5) | 0x1A);  //  read data from phy register to clear phy interrupts
    if ( (status & 0x8000)      //  interrupt status bit
        && (status & 0x7000) )  //  interrupt due to link status, speed, full duplex
    {
        linkReg = smi_reg_read((0x11 << 5) | 0x01);
        if ( (linkReg & 0x04) == 0 )    //  link down
        {
            dbg("MII1 - link down");
            *AMAZON_S_SW_P1_CTL = (*AMAZON_S_SW_P1_CTL & ~(1 << 18)) | (1 << 17);   //  force link down
            g_link_up = 0;
        }
        else
        {
            speedReg = smi_reg_read((0x11 << 5) | 0x1C);
            dbg("MII1 - link up, speedReg = %04x", (u32)speedReg);

            if ( (speedReg & 0x20) )
            {
                dbg("MII1 - full duplex");
                *AMAZON_S_SW_RGMII_CTL |= 1 << 11;
                g_link_duplex = 1;
            }
            else
            {
                dbg("MII1 - half duplex");
                *AMAZON_S_SW_RGMII_CTL &= ~(1 << 11);
                g_link_duplex = 0;
            }

            switch ( (speedReg >> 3) & 0x03 )
            {
            case 0:
                dbg("MII1 - 10BASE-T");
                *AMAZON_S_SW_RGMII_CTL &= 0xffffcfff;
                g_link_speed = 10;
                break;
            case 1:
                dbg("MII1 - 100BASE-TX");
                *AMAZON_S_SW_RGMII_CTL &= 0xffffcfff;
                *AMAZON_S_SW_RGMII_CTL |= 1 << 12;
                g_link_speed = 100;
                break;
            case 2:
                dbg("MII1 - 1000BASE-T");
                *AMAZON_S_SW_RGMII_CTL &= 0xffffcfff;
                *AMAZON_S_SW_RGMII_CTL |= 2 << 12;
                g_link_speed = 1000;
                break;
            case 3:
                err("MII1 - unknown link speed");
                g_link_speed = 0;
                break;
            }

            if ( g_link_speed )
            {
                *AMAZON_S_SW_P1_CTL &= ~(1 << 17);  //  disable force link down
                //*AMAZON_S_SW_P1_CTL |= 0x00040001;  //  the output packet keep the same format as receive, force link up
                *AMAZON_S_SW_P1_CTL &= ~1;          //  follow VLAN rules, and remove tag
                *AMAZON_S_SW_P1_CTL |= 0x00040000;  //  force link up
            }
        }
    }

    return IRQ_HANDLED;
}

#endif

static INLINE void reset_ppe(void)
{
    if ( (*DM_RXCFG & 0xFF) == 0x30 ){
        printk(KERN_ERR "[%s] skipping reset, due to DM_RXCFG\n", __func__);
        return;
    }

    //  reset PPE
    ifx_rcu_rst(IFX_RCU_DOMAIN_DSLDFE, IFX_RCU_MODULE_PPA);
    udelay(1000);
    ifx_rcu_rst(IFX_RCU_DOMAIN_DSLTC, IFX_RCU_MODULE_PPA);
    udelay(1000);
    ifx_rcu_rst(IFX_RCU_DOMAIN_PPE, IFX_RCU_MODULE_PPA);
    udelay(1000);

}

static INLINE void init_pmu(void)
{
    //   5 - DMA, should be enabled in DMA driver
    //   9 - DSL DFE/AFE
    //  15 - AHB
    //  21 - PPE TC
    //  22 - PPE EMA
    //  23 - PPE DPLUS
    //  28 - SWITCH

    //  code as reference
    //*PMU_PWDCR &= ~((1 << 28) | (1 << 23) | (1 << 22) | (1 << 21) | (1 << 15) | (1 << 9) | (1 << 5));

    PPE_TOP_PMU_SETUP(IFX_PMU_ENABLE);
    PPE_SLL01_PMU_SETUP(IFX_PMU_ENABLE);
    PPE_TC_PMU_SETUP(IFX_PMU_ENABLE);
    PPE_EMA_PMU_SETUP(IFX_PMU_ENABLE);
    PPE_QSB_PMU_SETUP(IFX_PMU_ENABLE);
    PPE_TPE_PMU_SETUP(IFX_PMU_ENABLE);
    DSL_DFE_PMU_SETUP(IFX_PMU_ENABLE);
    PPE_DPLUS_PMU_SETUP(IFX_PMU_ENABLE);
    SWITCH_PMU_SETUP(IFX_PMU_ENABLE);
}

static INLINE void init_mii0(int mii0_mode)
{
    if ( mii0_mode == REV_MII_MODE || mii0_mode == TURBO_REV_MII_MODE || mii0_mode == MII_MODE )
    {
        IFX_SWITCH_PIN_RESERVE(MII0_COL);
        IFX_SWITCH_ALTSEL0_SET(MII0_COL);
        IFX_SWITCH_ALTSEL1_CLR(MII0_COL);
        if ( mii0_mode == MII_MODE )
            IFX_SWITCH_DIR_IN(MII0_COL);
        else
        {
            IFX_SWITCH_DIR_OUT(MII0_COL);
            IFX_SWITCH_OD_SET(MII0_COL);
        }

        IFX_SWITCH_PIN_RESERVE(MII0_CRS);
        IFX_SWITCH_ALTSEL0_SET(MII0_CRS);
        IFX_SWITCH_ALTSEL1_CLR(MII0_CRS);
        if ( mii0_mode == MII_MODE )
            IFX_SWITCH_DIR_IN(MII0_CRS);
        else
        {
            IFX_SWITCH_DIR_OUT(MII0_CRS);
            IFX_SWITCH_OD_SET(MII0_CRS);
        }

        IFX_SWITCH_PIN_RESERVE(MII0_TXERR);
        IFX_SWITCH_ALTSEL0_SET(MII0_TXERR);
        IFX_SWITCH_ALTSEL1_CLR(MII0_TXERR);
        if ( mii0_mode == MII_MODE )
        {
            IFX_SWITCH_DIR_OUT(MII0_TXERR);
            IFX_SWITCH_OD_SET(MII0_TXERR);
        }
        else
            IFX_SWITCH_DIR_IN(MII0_TXERR);

        IFX_SWITCH_PIN_RESERVE(MII0_RXERR);
        IFX_SWITCH_ALTSEL0_SET(MII0_RXERR);
        IFX_SWITCH_ALTSEL1_CLR(MII0_RXERR);
        if ( mii0_mode == MII_MODE )
            IFX_SWITCH_DIR_IN(MII0_RXERR);
        else
        {
            IFX_SWITCH_DIR_OUT(MII0_RXERR);
            IFX_SWITCH_OD_SET(MII0_RXERR);
        }
    }

    init_external_tantos();

    *AMAZON_S_SW_RGMII_CTL &= 0xffcffc0f;

#if defined(ENABLE_MII0_BYPASS_DELAY_FIX) && ENABLE_MII0_BYPASS_DELAY_FIX
    *AMAZON_S_SW_RGMII_CTL |= 2 << 6;   //  set RX delay, SWITCH Port 0 Rx DLL 1.75ns
    *AMAZON_S_RCU_PPE_CONF = (*AMAZON_S_RCU_PPE_CONF & 0xFFFF81FF) | (5 << 9);  //  Bypass of DLL of SWITCH Port 0 Tx 1.75ns delay
#endif

    *AMAZON_S_SW_RGMII_CTL |= (mii0_mode & 0x03) << 8;

    switch ( mii0_mode )
    {
    case RGMII_MODE:
        *AMAZON_S_SW_RGMII_CTL = (*AMAZON_S_SW_RGMII_CTL & 0xfffffff3) | 0x08; break;
    case RGMII_MODE_100MB:
        *AMAZON_S_SW_RGMII_CTL = (*AMAZON_S_SW_RGMII_CTL & 0xfffffff3) | 0x04; break;
    case RGMII_MODE_10MB:
        *AMAZON_S_SW_RGMII_CTL = *AMAZON_S_SW_RGMII_CTL & 0xfffffff3; break;
    case TURBO_REV_MII_MODE:
        *AMAZON_S_SW_RGMII_CTL |= 1 << 20; break;
    case RED_MII_MODE_OC:
        *AMAZON_S_SW_RGMII_CTL |= 1 << 21; break;
    }

#if defined(ENABLE_SWITCH_FLOW_CONTROL) && ENABLE_SWITCH_FLOW_CONTROL
    *AMAZON_S_SW_RGMII_CTL |= 1;
#else
    *AMAZON_S_SW_RGMII_CTL &= ~1;
#endif

    if ( g_ingress_direct_forward_enable )
        *AMAZON_S_SW_P0_CTL |= (1 << 19);   //  enable port ingress direct forwarding
    else
        *AMAZON_S_SW_P0_CTL &= ~(1 << 19);  //  disable port ingress direct forwarding
#if defined(ENABLE_SWITCH_PORT_AUTHORIZED_MODE) && ENABLE_SWITCH_PORT_AUTHORIZED_MODE
    *AMAZON_S_SW_P0_CTL |= 3 << 2;      //  enable 802.1x port authorized state
#else
    *AMAZON_S_SW_P0_CTL &= ~(3 << 2);   //  disable 802.1x port authorized state
#endif
    *AMAZON_S_SW_P0_CTL &= ~(1 << 13);  //  disable port redirect
    *AMAZON_S_SW_P0_CTL &= ~(1 << 17);  //  disable force link down
    //*AMAZON_S_SW_P0_CTL |= 0x00040001;  //  the output packet keep the same format as receive, force link up
    if ( g_eth_wan_mode == 0 )
        *AMAZON_S_SW_P0_CTL &= ~1;      //  follow VLAN rules, and remove tag
    *AMAZON_S_SW_P0_CTL |= 0x00040000;  //  force link up
}

static INLINE void init_mii1(int mii1_mode)
{
    *AMAZON_S_SW_P1_CTL = (*AMAZON_S_SW_P2_CTL & ~(1 << 18)) | (1 << 17);   //  force link down

    //  Select Clock_Out2 @25MHz
    *AMAZON_S_CGU_IFCCR &= ~(3 << 10);
#ifdef CONFIG_MII1_PHY_CLOCK_GPIO3
    //  Select GPIO P0.3 for CLOCK OUT 2
    IFX_SWITCH_PIN_RESERVE(CLOCK_OUT2);
    IFX_SWITCH_ALTSEL0_SET(CLOCK_OUT2);
    IFX_SWITCH_ALTSEL1_CLR(CLOCK_OUT2);
    IFX_SWITCH_DIR_OUT(CLOCK_OUT2);
    IFX_SWITCH_OD_SET(CLOCK_OUT2);
#endif

    if ( mii1_mode == REV_MII_MODE || mii1_mode ==TURBO_REV_MII_MODE || mii1_mode == MII_MODE )
    {
        IFX_SWITCH_PIN_RESERVE(MII1_COL);
        IFX_SWITCH_ALTSEL0_SET(MII1_COL);
        IFX_SWITCH_ALTSEL1_CLR(MII1_COL);
        if ( mii1_mode == MII_MODE )
            IFX_SWITCH_DIR_IN(MII1_COL);
        else
        {
            IFX_SWITCH_DIR_OUT(MII1_COL);
            IFX_SWITCH_OD_SET(MII1_COL);
        }

        IFX_SWITCH_PIN_RESERVE(MII1_CRS);
        IFX_SWITCH_ALTSEL0_SET(MII1_CRS);
        IFX_SWITCH_ALTSEL1_CLR(MII1_CRS);
        if ( mii1_mode == MII_MODE )
            IFX_SWITCH_DIR_IN(MII1_CRS);
        else
        {
            IFX_SWITCH_DIR_OUT(MII1_CRS);
            IFX_SWITCH_OD_SET(MII1_CRS);
        }

        IFX_SWITCH_PIN_RESERVE(MII1_TXERR);
        IFX_SWITCH_ALTSEL0_SET(MII1_TXERR);
        IFX_SWITCH_ALTSEL1_CLR(MII1_TXERR);
        if ( mii1_mode == MII_MODE )
        {
            IFX_SWITCH_DIR_OUT(MII1_TXERR);
            IFX_SWITCH_OD_SET(MII1_TXERR);
        }
        else
            IFX_SWITCH_DIR_IN(MII1_TXERR);

        IFX_SWITCH_PIN_RESERVE(MII1_RXERR);
        IFX_SWITCH_ALTSEL0_SET(MII1_RXERR);
        IFX_SWITCH_ALTSEL1_CLR(MII1_RXERR);
        if ( mii1_mode == MII_MODE )
            IFX_SWITCH_DIR_IN(MII1_RXERR);
        else
        {
            IFX_SWITCH_DIR_OUT(MII1_RXERR);
            IFX_SWITCH_OD_SET(MII1_RXERR);
        }
    }

#ifdef CONFIG_MII1_AUTONEG
    mii1_gphy_config_gpio();
#endif

    *AMAZON_S_SW_RGMII_CTL &= 0xff303fff;

#if defined(ENABLE_MII1_BYPASS_DELAY_FIX) && ENABLE_MII1_BYPASS_DELAY_FIX
    *AMAZON_S_RCU_PPE_CONF = (*AMAZON_S_RCU_PPE_CONF & 0xFFE07FFF) | (0x20 << 15); //  Bypass of DLL of SWITCH Port 1 Tx 1.5ns, SWITCH Port 1 Rx DLL 0ns
#endif

    *AMAZON_S_SW_RGMII_CTL |= (mii1_mode & 0x03) << 18;

    switch ( mii1_mode )
    {
    case RGMII_MODE:
        *AMAZON_S_SW_RGMII_CTL = (*AMAZON_S_SW_RGMII_CTL & 0xffffcfff) | 0x2000; break;
    case RGMII_MODE_100MB:
        *AMAZON_S_SW_RGMII_CTL = (*AMAZON_S_SW_RGMII_CTL & 0xffffcfff) | 0x1000; break;
    case RGMII_MODE_10MB:
        *AMAZON_S_SW_RGMII_CTL = *AMAZON_S_SW_RGMII_CTL & 0xffffcfff; break;
    case TURBO_REV_MII_MODE:
        *AMAZON_S_SW_RGMII_CTL |= 1 << 22; break;
    case RED_MII_MODE_OC:
        *AMAZON_S_SW_RGMII_CTL |= 1 << 23; break;
    }

#if defined(ENABLE_SWITCH_FLOW_CONTROL) && ENABLE_SWITCH_FLOW_CONTROL
    *AMAZON_S_SW_RGMII_CTL |= 1 << 10;
#else
    *AMAZON_S_SW_RGMII_CTL &= ~(1 << 10);
#endif

    if ( g_ingress_direct_forward_enable )
        *AMAZON_S_SW_P1_CTL |= (1 << 19);   //  enable port ingress direct forwarding
    else
        *AMAZON_S_SW_P1_CTL &= ~(1 << 19);  //  disable port ingress direct forwarding
#if defined(ENABLE_SWITCH_PORT_AUTHORIZED_MODE) && ENABLE_SWITCH_PORT_AUTHORIZED_MODE
    *AMAZON_S_SW_P1_CTL |= 3 << 2;      //  enable 802.1x port authorized state
#else
    *AMAZON_S_SW_P1_CTL &= ~(3 << 2);   //  disable 802.1x port authorized state
#endif
    *AMAZON_S_SW_P1_CTL &= ~(1 << 13);  //  disable port redirect
#ifndef CONFIG_MII1_AUTONEG
    *AMAZON_S_SW_P1_CTL &= ~(1 << 17);  //  disable force link down
    //*AMAZON_S_SW_P1_CTL |= 0x00040001;  //  the output packet keep the same format as receive, force link up
    if ( g_eth_wan_mode == 0 )
        *AMAZON_S_SW_P1_CTL &= ~1;      //  follow VLAN rules, and remove tag
    *AMAZON_S_SW_P1_CTL |= 0x00040000;  //  force link up
#endif
}

static INLINE void init_mii2(void)
{
#ifndef CONFIG_MII_SETUP_AVMNET
    *AMAZON_S_SW_P2_CTL = (*AMAZON_S_SW_P2_CTL & ~(1 << 18)) | (1 << 17);   //  force link down
#endif

    *AMAZON_S_SW_GCTL0 &= ~0x0300;          //  max packet length: 1152 bytes
//    *AMAZON_S_SW_PMAC_HD_CTL |= 0x00040000; //  enable CRC
    *AMAZON_S_SW_PMAC_HD_CTL |= (1 << 20);  //  remove CRC of packets going from PMAC to DMA
    *AMAZON_S_SW_P2_CTL |= 0x00000001;      //  the output packet keep the same format as receive

    *AMAZON_S_SW_GCTL0 |= 1 << 29;          //  enable logical port
    *AMAZON_S_SW_PMAC_HD_CTL |= 1 << 19;    //  enable status header (PMAC -> DMA)
    *AMAZON_S_SW_PMAC_HD_CTL |= 1 << 22;    //  enable status header (DMA -> PMAC)

    *AMAZON_S_SW_GCTL1 &= ~ ( (1 << 16) | (1 << 17) | (1 << 19));         //  identify IP in PPPoE and IPv6 in PPPoE

#if defined(ENABLE_SWITCH_PORT_AUTHORIZED_MODE) && ENABLE_SWITCH_PORT_AUTHORIZED_MODE
    *AMAZON_S_SW_P2_CTL |= 3 << 2;      //  enable 802.1x port authorized state
#else
    *AMAZON_S_SW_P2_CTL &= ~(3 << 2);   //  disable 802.1x port authorized state
#endif

#ifndef CONFIG_MII_SETUP_AVMNET
    *AMAZON_S_SW_P2_CTL = (*AMAZON_S_SW_P2_CTL & ~((3 << 30) | (1 << 17))) | (1 << 18) | (1 << 30); //  force link up, discard
    udelay(10);

    //  reset GPHY - P2.0
    IFX_SWITCH_PIN_RESERVE(RESET_GPHY);
    IFX_SWITCH_ALTSEL0_CLR(RESET_GPHY);
    IFX_SWITCH_ALTSEL1_CLR(RESET_GPHY);
    IFX_SWITCH_DIR_OUT(RESET_GPHY);
    IFX_SWITCH_OD_SET(RESET_GPHY);
    IFX_SWITCH_OUTPUT_SET(RESET_GPHY);
    mdelay(50);
    IFX_SWITCH_OUTPUT_CLR(RESET_GPHY);
    mdelay(50);
    IFX_SWITCH_OUTPUT_SET(RESET_GPHY);
    mdelay(300);

#ifdef CONFIG_MII1_AUTONEG
    mii1_gphy_detect();
#endif
#endif
}

static INLINE void start_mii2(void) // this is the cpu port!
{
    *DS_RXCFG |= 0x00008000;    //  enable DSRX
    *DM_RXCFG |= 0x80000000;    //  enable DMRX

    *AMAZON_S_SW_P2_CTL = (*AMAZON_S_SW_P2_CTL & ~((3 << 30) | (3 << 17))) | (1 << 18); //  force link up, normal forwarding
}

static INLINE void stop_datapath(void)
{
    unsigned long org_jiffies;
    int i;

    g_stop_datapath = 1;    //  going to be unloaded

    //  disable switch external port
#ifdef CONFIG_MII1_AUTONEG
    disable_irq(MII1_GPHY_INT);
#endif
    *AMAZON_S_SW_P0_CTL = (*AMAZON_S_SW_P0_CTL & ~(1 << 18)) | (1 << 17);   //  force MII0 link down
    *AMAZON_S_SW_P1_CTL = (*AMAZON_S_SW_P1_CTL & ~(1 << 18)) | (1 << 17);   //  force MII1 link down

    //  turn on DMA RX channels
    // turn_on_dma_rx(31);

    //  clean ingress datapath
    org_jiffies = jiffies;
    while ( ((IFX_REG_R32(DM_RXPGCNT) & 0x00FF) != 0 || (IFX_REG_R32(DM_RXPKTCNT) & 0x00FF) != 0) && jiffies - org_jiffies < HZ / 10 + 1 )
        schedule();
    if ( jiffies - org_jiffies > HZ / 10 )
        err("Can not clear DM_RXPGCNT/DM_RXPKTCNT");

    //  turn off DMA RX channels (including loopback RX channels)
    while ( (i = clz(g_f_dma_opened)) >= 0 )
        turn_off_dma_rx(i);

    //  turn off DMA Loopback TX channels
    g_dma_device_ppe->tx_chan[0]->close(g_dma_device_ppe->tx_chan[0]);
    g_dma_device_ppe->tx_chan[1]->close(g_dma_device_ppe->tx_chan[1]);

    //  free GPIO pins
    //    MII0
#if MII0_MODE_SETUP == REV_MII_MODE || MII0_MODE_SETUP == TURBO_REV_MII_MODE || MII0_MODE_SETUP == MII_MODE
    IFX_SWITCH_PIN_FREE(MII0_COL);
    IFX_SWITCH_PIN_FREE(MII0_CRS);
    IFX_SWITCH_PIN_FREE(MII0_TXERR);
    IFX_SWITCH_PIN_FREE(MII0_RXERR);
#endif
    //    External Switch
    IFX_SWITCH_PIN_FREE(SWITCH_MDIO);
    IFX_SWITCH_PIN_FREE(SWITCH_MDC);
    //    MII1
#ifdef CONFIG_MII1_PHY_CLOCK_GPIO3
    IFX_SWITCH_PIN_FREE(CLOCK_OUT2);
#endif
#if MII1_MODE_SETUP == REV_MII_MODE || MII1_MODE_SETUP == TURBO_REV_MII_MODE || MII1_MODE_SETUP == MII_MODE
    IFX_SWITCH_PIN_FREE(MII1_COL);
    IFX_SWITCH_PIN_FREE(MII1_CRS);
    IFX_SWITCH_PIN_FREE(MII1_TXERR);
    IFX_SWITCH_PIN_FREE(MII1_RXERR);
#endif
#ifdef CONFIG_MII1_AUTONEG
  #if MII1_GPHY_INT_EXT_INT_NO == 0
    IFX_SWITCH_PIN_FREE(0);
  #elif MII1_GPHY_INT_EXT_INT_NO == 1
    IFX_SWITCH_PIN_FREE(1);
  #elif MII1_GPHY_INT_EXT_INT_NO == 2
    IFX_SWITCH_PIN_FREE(2);
  #elif MII1_GPHY_INT_EXT_INT_NO == 3
    IFX_SWITCH_PIN_FREE(39);
  #elif MII1_GPHY_INT_EXT_INT_NO == 4
    IFX_SWITCH_PIN_FREE(10);
  #elif MII1_GPHY_INT_EXT_INT_NO == 5
    IFX_SWITCH_PIN_FREE(9);
  #else
    #error ERROR EXT INT NO
  #endif
#endif
    //    MII2
    IFX_SWITCH_PIN_FREE(RESET_GPHY);
}

static INLINE void init_internal_tantos_qos_setting(void)
{
    /*
     *  Set internal switch configurations is,
     *  Classification: 802.1q & DSCP
     *  Scheduling for all internal ports: Strict Priority and Rate limit is Disabled
     */

    /*
     *  classification scheme:
     *      if packet is IP packet, then classification is based on DSCP.
     *      if packet is inot IP packet, then classification is based on 802.1q.
     */

    //  Enable VLAN, DSCP & IP Over VLAN priority on all ports (0/1/2)
    *AMAZON_S_SW_P0_CTL |= 7 << 25;
    *AMAZON_S_SW_P1_CTL |= 7 << 25;
    *AMAZON_S_SW_P2_CTL |= 7 << 25;

    /*
     *  Queue Mapping
     */

    *AMAZON_S_SW_1P_PRT = 0xFA41;

    *AMAZON_S_SW_DFSRV_MAP0 = 0x11100000;
    *AMAZON_S_SW_DFSRV_MAP1 = 0x22211111;
    *AMAZON_S_SW_DFSRV_MAP2 = 0x30222222;
    *AMAZON_S_SW_DFSRV_MAP3 = 0x00030003;

    /*
     *  Scheduling Scheme
     *      Strict Priority scheduling is used for all ports and Rate limit is disabled
     */

    *AMAZON_S_SW_P0_ECS_Q32 = 0x03E803E8;
    *AMAZON_S_SW_P0_ECS_Q10 = 0x03E803E8;
    *AMAZON_S_SW_P0_ECW_Q32 = 0x00000000;
    *AMAZON_S_SW_P0_ECW_Q10 = 0x00000000;
    *AMAZON_S_SW_P1_ECS_Q32 = 0x03E803E8;
    *AMAZON_S_SW_P1_ECS_Q10 = 0x03E803E8;
    *AMAZON_S_SW_P1_ECW_Q32 = 0x00000000;
    *AMAZON_S_SW_P1_ECW_Q10 = 0x00000000;
    *AMAZON_S_SW_P2_ECS_Q32 = 0x03E803E8;
    *AMAZON_S_SW_P2_ECS_Q10 = 0x03E803E8;
    *AMAZON_S_SW_P2_ECW_Q32 = 0x00000000;
    *AMAZON_S_SW_P2_ECW_Q10 = 0x00000000;
}

static INLINE void init_internal_tantos(void)
{
    //  forward all unknown unicast, unknown multicast, broadcast, reserve packet to CPU port only
    *AMAZON_S_SW_DF_PORTMAP = 0x04040404;

    //  turn on all virtual port as bridge destination
    *AMAZON_S_SW_P0_VLAN |= 0xFF << 16;
    *AMAZON_S_SW_P1_VLAN |= 0xFF << 16;
    *AMAZON_S_SW_P2_VLAN |= 0xFF << 16;

    init_internal_tantos_qos_setting();
}

static INLINE void init_dplus(void)
{
    *DM_RXCFG &= ~0x80000000;   //  disable DMRX
    *DS_RXCFG &= ~0x00008000;   //  disable DSRX
    *DM_RXPGCNT = 0x00020000;   //  clear page pointer & counter
    *DS_RXPGCNT = 0x00040000;   //  clear page pointer & counter

    *DM_RXDB    = 0x1450;
    *DM_RXCB    = 0x143C;
    *DM_RXPGCNT = 0x00020000;
    *DM_RXPKTCNT= 0x00000200;
    *DM_RXCFG   = 0x00F87014;
}

static INLINE void init_ema(void)
{
    *EMA_CMDCFG  = (EMA_CMD_BUF_LEN << 16) | (EMA_CMD_BASE_ADDR >> 2);
    *EMA_DATACFG = (EMA_DATA_BUF_LEN << 16) | (EMA_DATA_BASE_ADDR >> 2);
    *EMA_IER     = 0x000000FF;
    *EMA_CFG     = EMA_READ_BURST | (EMA_WRITE_BURST << 2);
}

static INLINE void init_mailbox(void)
{
    *MBOX_IGU0_ISRC = 0xFFFFFFFF;
    *MBOX_IGU0_IER  = 0x00000000;
    *MBOX_IGU1_ISRC = 0xFFFFFFFF;
    *MBOX_IGU1_IER  = 0x00000000;   //  Don't need to enable RX interrupt, EMA driver handle RX path.
    dbg("MBOX_IGU1_IER = 0x%08X", *MBOX_IGU1_IER);
}

static INLINE void clear_share_buffer(void)
{
    volatile u32 *p = SB_RAM0_ADDR(0);
    unsigned int i;

    //  write all zeros only
    for ( i = 0; i < SB_RAM0_DWLEN + SB_RAM1_DWLEN + SB_RAM2_DWLEN + SB_RAM3_DWLEN + SB_RAM4_DWLEN; i++ )
        *p++ = 0;

    //  Configure share buffer master selection
    *SB_MST_PRI0 = 1;
    *SB_MST_PRI1 = 1;
}

static INLINE void clear_cdm(void)
{
    volatile u32 *dest;
    int i;

    //  Both blocks are set to code memory
    *CDM_CFG = CDM_CFG_RAM1_SET(0x01) | CDM_CFG_RAM0_SET(0x00);
    for ( i = 0; i < 1000; i++ );

    dest = CDM_CODE_MEMORY_RAM0_ADDR(0);
    for ( i = 0; i < CDM_CODE_MEMORY_RAM0_DWLEN + CDM_CODE_MEMORY_RAM1_DWLEN; i++ )
        *dest++ = 0;
}

static INLINE void board_init(void)
{
}

static INLINE void init_dsl_hw(void)
{
     int i;

    *SFSM_CFG0          = 0x00010014;
    for (i=0;i<10;i++);

    *SFSM_DBA0          = 0x1800;
    *SFSM_CBA0          = 0x1918;
    for (i=0;i<10;i++);

    *SFSM_CFG0          = 0x00014014;
    *FFSM_DBA0          = 0x17AC;
    *FFSM_CFG0          = 0x00010006;

    *SFSM_CFG1          = 0x00010014;
    for (i=0;i<10;i++);

    *SFSM_DBA1          = 0x192C;
    *SFSM_CBA1          = 0x1A44;
    *FFSM_DBA1          = 0x1758;
    *FFSM_CFG1          = 0x00010006;

    //*FFSM_IDLE_HEAD_BC0 = 0x00000001;
}

static INLINE void setup_ppe_conf(void)
{
    // disable PPE and MIPS access to DFE memory
    *AMAZON_S_RCU_PPE_CONF = *AMAZON_S_RCU_PPE_CONF & ~0xC0000000;
}

static INLINE void init_hw(void)
{
    setup_ppe_conf();

    init_pmu();

#ifndef CONFIG_MII_SETUP_AVMNET
    init_mii0(MII0_MODE_SETUP);

    init_mii1(MII1_MODE_SETUP);
	#error AVMNET does not support MII Setup by IFX
#endif

    init_mii2();

    init_internal_tantos();

    init_dplus();

    init_dsl_hw();

    init_ema();

    init_mailbox();

    clear_share_buffer();

    clear_cdm();

    board_init();
}

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
static INLINE int pp32_download_code(const u32 *code_src, unsigned int code_dword_len, const u32 *data_src, unsigned int data_dword_len)
{
    volatile u32 *dest;
    int i;

    if ( code_src == 0 || ((unsigned long)code_src & 0x03) != 0
        || data_src == 0 || ((unsigned long)data_src & 0x03) != 0
        || (code_dword_len > 0x1000 && data_dword_len > 0) )
        return -EINVAL;

    /*  set PPE code memory to FPI bus access mode  */
    if ( code_dword_len <= 0x1000 )
        *CDM_CFG = CDM_CFG_RAM1_SET(0x00) | CDM_CFG_RAM0_SET(0x00);
    else
        *CDM_CFG = CDM_CFG_RAM1_SET(0x01) | CDM_CFG_RAM0_SET(0x00);
    for ( i = 0; i < 1000; i++ );

    dbg("code_dword_len = 0x%X, data_dword_len = 0x%X", code_dword_len, data_dword_len);

    /*  copy code   */
    dest = CDM_CODE_MEMORY_RAM0_ADDR(0);
    while ( code_dword_len-- > 0 )
        *dest++ = *code_src++;

    /*  copy data   */
    dest = PP32_DATA_MEMORY_RAM1_ADDR(0);
    while ( data_dword_len-- > 0 )
        *dest++ = *data_src++;

    return 0;
}

/*
 *  Description:
 *    Do PP32 specific initialization.
 *  Input:
 *    data --- void *, specific parameter passed in.
 *  Output:
 *    int  --- 0:    Success
 *             else: Error Code
 */
static INLINE int pp32_specific_init(int fwcode, void *data)
{
    return 0;
}

/*
 *  Description:
 *    Initialize and start up PP32.
 *  Input:
 *    none
 *  Output:
 *    int  --- 0:    Success
 *             else: Error Code
 */
static INLINE int pp32_start(int fwcode)
{
    int ret;

    /*  download firmware   */
    ret = pp32_download_code(ar9_a5_binary_code, sizeof(ar9_a5_binary_code) / sizeof(*ar9_a5_binary_code), ar9_a5_binary_data, sizeof(ar9_a5_binary_data) / sizeof(*ar9_a5_binary_data));
    if ( ret )
        return ret;

    /*  firmware specific initialization    */
    ret = pp32_specific_init(fwcode, NULL);
    if ( ret )
        return ret;

    /*  run PP32    */
    *PP32_DBG_CTRL = DBG_CTRL_RESTART;
    /*  idle for a while to let PP32 init itself    */
    udelay(200);

    return 0;
}

/*
 *  Description:
 *    Halt PP32.
 *  Input:
 *    none
 *  Output:
 *    none
 */
static INLINE void pp32_stop(void)
{
    /*  halt PP32   */
    *PP32_DBG_CTRL = DBG_CTRL_STOP;
}

static INLINE int init_local_variables(void)
{
    int i, j;

    /*  initialize semaphore used by open and close */
    sema_init(&g_sem, 1);

    if ( ethwan == 1 )
        g_eth_wan_mode = 2;
    else if ( ethwan == 2 )
        g_eth_wan_mode = 3;

    if ( wanitf == ~0 || wanitf == 0) {
        switch ( g_eth_wan_mode ) {
        case 0: /*  DSL WAN     */
        	g_wan_itf = 3 << 6;
        	break;
        case 2:
        	/*  Mixed Mode  */
        	g_wan_itf = 1 << 0;
        	break;
        case 3:
        	/*  ETH WAN  - if no wanitf is set - use AVMNET ETHWAN settings*/
        	g_wan_itf =  avmnet_swi_ifx_common_set_ethwan_dev_by_name( avm_ata_dev ) ;
        	break;
        }
    }
    else
    {
        g_wan_itf = wanitf;
        switch ( g_eth_wan_mode )
        {
        case 0: /*  DSL WAN     */
                g_wan_itf &= 0xF8;
                g_wan_itf |= 3 << 6;
                break;   //  ETH0/1 can not be WAN
        case 2: /*  Mixed Mode  */
                g_wan_itf &= 0x03; //  only ETH0/1 support mixed mode
                if(g_wan_itf == 0x03 || g_wan_itf == 0){ //both ETH0/1 in mixed mode, reset to eth0 mixed mode
                    g_wan_itf = 1;
                }
                break;
        case 3: /*  ETH WAN     */
                g_wan_itf &= 0x3B; //  DSL disabled in ETH WAN mode
                if((g_wan_itf & 0x03) == 0x03){ //both ETH0/1 in WAN mode, remove eth0 wan setting
                    g_wan_itf &= ~0x01;
                }
                if((g_wan_itf & 0x03 ) == 0){//improper setting
                    g_wan_itf |= 1 << 1;
                }
                break;
        }
    }

    g_ipv6_acc_en = ipv6_acc_en ? 1 : 0;
    g_ipv4_lan_collision_rout_fwdc_table_base = __IPV4_LAN_COLLISION_ROUT_FWDC_TABLE_BASE;
    g_ipv4_wan_collision_rout_fwda_table_base = __IPV4_WAN_COLLISION_ROUT_FWDA_TABLE_BASE;

    g_wanqos_en = g_eth_wan_mode == 3 && (g_wan_itf & 0x03) != 0x03 && wanqos_en ? wanqos_en : 0;

    g_mailbox_signal_mask = g_wanqos_en ? CPU_TO_WAN_SWAP_SIG : 0;
    if ( g_eth_wan_mode == 0 )
        g_mailbox_signal_mask |= WAN_RX_SIG(0) | WAN_RX_SIG(1);

    memset(&g_atm_priv_data, 0, sizeof(g_atm_priv_data));
    for ( i = 0; i < ATM_PORT_NUMBER; i++ )
    {
        g_atm_priv_data.port[i].tx_max_cell_rate    = (u32)port_cell_rate_up[i];
    }
    g_atm_priv_data.max_connections = ATM_QUEUE_NUMBER;
    g_atm_priv_data.max_sw_tx_queues = ATM_SW_TX_QUEUE_NUMBER;

    for ( i = 0; i < 2; i++ )
    {
        int max_packet_priority = NUM_ENTITY(g_eth_prio_queue_map);  //  assume only 8 levels are used in Linux
        int tx_num_q;
        int q_step, q_accum, p_step;

        tx_num_q = (g_wan_itf & (1 << i)) && g_wanqos_en ? __ETH_WAN_TX_QUEUE_NUM : __ETH_VIRTUAL_TX_QUEUE_NUM;
        q_step = tx_num_q - 1;
        p_step = max_packet_priority - 1;
        for ( j = 0, q_accum = 0; j < max_packet_priority; j++, q_accum += q_step )
            g_eth_prio_queue_map[j] = q_step - (q_accum + (p_step >> 1)) / p_step;
    }

    for ( i = 0; i < NUM_ENTITY(g_wan_queue_class_map); i++ )
        g_wan_queue_class_map[i] = (NUM_ENTITY(g_wan_queue_class_map) - 1 - i) % SWITCH_CLASS_NUM;

    return 0;
}

static INLINE void clear_local_variables(void)
{
}

#if defined(ENABLE_FW_MODULE_TEST) && ENABLE_FW_MODULE_TEST

#include "cfg_arrays_a5.h"

unsigned int ppe_fw_mode        = 0; // 0: normal mode, 1: mix mode
unsigned int ppe_wan_hash_algo  = 0; // 0: using algo 0, 1: using algo 1
unsigned int acc_proto      = 0; // 0: UDP, 1:TCP

void setup_acc_action_tbl(void)
{
    unsigned int idx;

    if (acc_proto == 0) {
        // clear bit16 of dw3 of each action entry
        unsigned long udp_mask = ~ (1 << 16);

        // setup for Hash entry
        idx = 3;
        while (idx < sizeof(lan_uc_act_tbl_normal_mode_cfg)/sizeof(unsigned long)) {
            lan_uc_act_tbl_normal_mode_cfg[idx] &= udp_mask;
            lan_uc_act_tbl_mixed_mode_cfg[idx] &= udp_mask;

            wan_uc_act_tbl_alo_0_cfg[idx] &= udp_mask;
            wan_uc_act_tbl_alo_1_cfg[idx] &= udp_mask;
            idx += 6;
        }

        // setup for Collsion entry
        idx = 3;
        while (idx < sizeof(lan_uc_col_act_tbl_normal_mode_cfg)/sizeof(unsigned long)) {
            lan_uc_col_act_tbl_normal_mode_cfg[idx] &= udp_mask;
            lan_uc_col_act_tbl_mixed_mode_cfg[idx] &= udp_mask;
            wan_uc_col_act_tbl_cfg[idx] &= udp_mask;
            idx += 6;
        }
    }
    else {
        // set bit16 of dw3 of each action entry
        unsigned long tcp_mask =  (1 << 16);

        // setup for Hash entry
        idx = 3;
        while (idx < sizeof(lan_uc_act_tbl_normal_mode_cfg)/sizeof(unsigned long)) {
            lan_uc_act_tbl_normal_mode_cfg[idx] |= tcp_mask;
            lan_uc_act_tbl_mixed_mode_cfg[idx] |= tcp_mask;

            wan_uc_act_tbl_alo_0_cfg[idx] |= tcp_mask;
            wan_uc_act_tbl_alo_1_cfg[idx] |= tcp_mask;
            idx += 6;
        }

        // setup for Collsion entry
        idx = 3;
        while (idx < sizeof(lan_uc_col_act_tbl_normal_mode_cfg)/sizeof(unsigned long)) {
            lan_uc_col_act_tbl_normal_mode_cfg[idx] |= tcp_mask;
            lan_uc_col_act_tbl_mixed_mode_cfg[idx] |= tcp_mask;
            wan_uc_col_act_tbl_cfg[idx] |= tcp_mask;
            idx += 6;
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

    // lan uc_cmp_tbl_cfg (Hash) and collision
    memcpy((void *)ROUT_LAN_HASH_CMP_TBL(0), lan_uc_cmp_tbl_cfg, sizeof(lan_uc_cmp_tbl_cfg));
    memcpy((void *)ROUT_LAN_COLL_CMP_TBL(0), lan_uc_col_cmp_tbl_cfg, sizeof(lan_uc_col_cmp_tbl_cfg));

    // lan action
    if(ppe_fw_mode == 0) {
        // normal mode
        memcpy((void *)ROUT_LAN_HASH_ACT_TBL(0), lan_uc_act_tbl_normal_mode_cfg, sizeof(lan_uc_act_tbl_normal_mode_cfg));
        memcpy((void *)ROUT_LAN_COLL_ACT_TBL(0), lan_uc_col_act_tbl_normal_mode_cfg, sizeof(lan_uc_col_act_tbl_normal_mode_cfg));
    }
    else {
        // mix mode
        memcpy((void *)ROUT_LAN_HASH_ACT_TBL(0), lan_uc_act_tbl_mixed_mode_cfg, sizeof(lan_uc_act_tbl_mixed_mode_cfg));
        memcpy((void *)ROUT_LAN_COLL_ACT_TBL(0), lan_uc_col_act_tbl_mixed_mode_cfg, sizeof(lan_uc_col_act_tbl_mixed_mode_cfg));
    }

    // wan hash cmp anc act table
    if ( ppe_wan_hash_algo == 0) {
        // WAN algo 0
        memcpy((void *)ROUT_WAN_HASH_CMP_TBL(0), wan_uc_cmp_tbl_alo_0_cfg, sizeof(wan_uc_cmp_tbl_alo_0_cfg));
        memcpy((void *)ROUT_WAN_HASH_ACT_TBL(0), wan_uc_act_tbl_alo_0_cfg, sizeof(wan_uc_act_tbl_alo_0_cfg));
    }
    else {
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
}

#endif  //  #if defined(ENABLE_FW_MODULE_TEST) && ENABLE_FW_MODULE_TEST

static INLINE void init_communication_data_structures(int fwcode)
{
    struct eth_ports_cfg eth_ports_cfg = {0};
    struct rout_tbl_cfg lan_rout_tbl_cfg = {0};
    struct rout_tbl_cfg wan_rout_tbl_cfg = {0};
    struct gen_mode_cfg1 gen_mode_cfg1 = {0};
    struct wrx_queue_config wrx_queue_config = {0};
    struct wtx_port_config wtx_port_config = {0};
    struct wtx_queue_config wtx_queue_config = {0};
    struct ps_mc_cfg ps_mc_gencfg3              = {0};
    int i,j;

    *CDM_CFG = CDM_CFG_RAM1_SET(0x00) | CDM_CFG_RAM0_SET(0x00);

    for ( i = 0; i < 1000; i++ );

    *PSEUDO_IPv4_BASE_ADDR = 0xFFFFFF00;

    *CFG_WRX_HTUTS      = g_atm_priv_data.max_connections + OAM_HTU_ENTRY_NUMBER;
    *CFG_WRDES_DELAY    = write_descriptor_delay;
    *WRX_EMACH_ON       = 0x03;
    *WTX_EMACH_ON       = 0x7FFF;
    *WRX_HUNT_BITTH     = DEFAULT_RX_HUNT_BITTH;

    eth_ports_cfg.wan_vlanid_hi = 0;
    eth_ports_cfg.wan_vlanid_lo = 0;
    eth_ports_cfg.eth0_type     = 0;    //  lan
    *ETH_PORTS_CFG = eth_ports_cfg;

    lan_rout_tbl_cfg.rout_num   = MAX_COLLISION_ROUTING_ENTRIES;
    lan_rout_tbl_cfg.ttl_en     = 1;
    lan_rout_tbl_cfg.rout_drop  = 0;
    *LAN_ROUT_TBL_CFG = lan_rout_tbl_cfg;

    wan_rout_tbl_cfg.rout_num           = MAX_COLLISION_ROUTING_ENTRIES;
    wan_rout_tbl_cfg.wan_rout_mc_num    = MAX_WAN_MC_ENTRIES;
    wan_rout_tbl_cfg.ttl_en             = 1;
    wan_rout_tbl_cfg.wan_rout_mc_drop   = 0;
    wan_rout_tbl_cfg.rout_drop          = 0;
    *WAN_ROUT_TBL_CFG = wan_rout_tbl_cfg;

    gen_mode_cfg1.app2_indirect         = 1;    //  0: means DMA RX ch 3 is dedicated for CPU1 process
    gen_mode_cfg1.us_indirect           = 0;
    gen_mode_cfg1.us_early_discard_en   = 0;
#if defined(CONFIG_IFX_PPA_MFE) && CONFIG_IFX_PPA_MFE
    gen_mode_cfg1.classification_num    = MAX_CLASSIFICATION_ENTRIES;
#else
    gen_mode_cfg1.classification_num    = 0;
#endif
    gen_mode_cfg1.ipv6_rout_num         = g_ipv6_acc_en ? 2 : 2;
    gen_mode_cfg1.session_ctl_en        = 0;
    gen_mode_cfg1.wan_hash_alg          = 0;
#if defined(CONFIG_IFX_PPA_MFE) && CONFIG_IFX_PPA_MFE
    gen_mode_cfg1.brg_class_en          = 1;
#else
    gen_mode_cfg1.brg_class_en          = 0;
#endif
    gen_mode_cfg1.ipv4_mc_acc_mode      = 0;
    gen_mode_cfg1.ipv6_acc_en           = g_ipv6_acc_en;
    gen_mode_cfg1.wan_acc_en            = 1;
    gen_mode_cfg1.lan_acc_en            = 1;
    gen_mode_cfg1.sw_iso_mode           = 1;
    gen_mode_cfg1.sys_cfg               = g_eth_wan_mode;   //  3 - ETH1 WAN, 2 - ETH0 mixed, 0 - DSL WAN
    *GEN_MODE_CFG1 = gen_mode_cfg1;
    //GEN_MODE_CFG2->itf_outer_vlan_vld   = 1 << 7;   //  DSL EoA port use outer VLAN for PVCs differentiation
    GEN_MODE_CFG2->itf_outer_vlan_vld  |= 0xFF;     //  Turn on outer VLAN for all interfaces;

    ps_mc_gencfg3.ssc_mode = 1;
    ps_mc_gencfg3.asc_mode = 0;
    ps_mc_gencfg3.psave_en = 0; //default disable clock gating power save function
    *PS_MC_GENCFG3         = ps_mc_gencfg3;

    *CFG_CLASS2QID_MAP(0) = 0;
    *CFG_CLASS2QID_MAP(1) = 0;
    *CFG_QID2CLASS_MAP(0) = 0;
    *CFG_QID2CLASS_MAP(1) = 0;
    i = (g_wan_itf & 0x01) ? 0 : 1;
    for ( j = 0; j < NUM_ENTITY(g_eth_prio_queue_map); j++ )
	{
        *CFG_CLASS2QID_MAP(j >> 3) |= g_eth_prio_queue_map[j] << ((j & 0x07) * 4);
        *CFG_CLASS2QID_MAP((j + 8) >> 3) |= (g_eth_prio_queue_map[j] | 8) << ((j & 0x07) * 4);
    }
    for ( j = 0; j < NUM_ENTITY(g_wan_queue_class_map); j++ )
        *CFG_QID2CLASS_MAP(j >> 3) |= g_wan_queue_class_map[j] << ((j & 0x07) * 4);

    //  fake setting, when open ATM VC, it will be set with the real value
    wrx_queue_config.new_vlan   = 0;
    wrx_queue_config.vlan_ins   = 0;
    wrx_queue_config.mpoa_type  = 3;    //  IPoA
    wrx_queue_config.ip_ver     = 0;
    wrx_queue_config.mpoa_mode  = 0;    //  VC mux
    wrx_queue_config.oversize   = DMA_PACKET_SIZE;
    wrx_queue_config.undersize  = 0;
    wrx_queue_config.mfs        = DMA_PACKET_SIZE;
    wrx_queue_config.uumask     = 0xFF;
    wrx_queue_config.cpimask    = 0xFF;
    wrx_queue_config.uuexp      = 0;
    wrx_queue_config.cpiexp     = 0;
    for ( i = 0; i < 16; i++ )
        *WRX_QUEUE_CONFIG(i) = wrx_queue_config;

    wtx_port_config.qid     = 0;
    wtx_port_config.qsben   = 1;
    for ( i = 0; i < 2; i++ )
        *WTX_PORT_CONFIG(i) = wtx_port_config;

    //  fake setting, when open ATM VC, it will be set with the real value
    wtx_queue_config.uu             = 0;
    wtx_queue_config.cpi            = 0;
    wtx_queue_config.same_vc_qmap   = 0;
    wtx_queue_config.sbid           = 0;
    wtx_queue_config.mpoa_mode      = 0;    //  VC mux
    wtx_queue_config.qsben          = 1;
    wtx_queue_config.atm_header     = 0;
    for ( i = 0; i < 15; i++ )
    {
        wtx_queue_config.qsb_vcid   = i + QSB_QUEUE_NUMBER_BASE;
        *WTX_QUEUE_CONFIG(i) = wtx_queue_config;
    }

    //*MTU_CFG_TBL(0) = DMA_PACKET_SIZE;          //  for ATM
    *MTU_CFG_TBL(0) = ETH_MAX_DATA_LENGTH;
    for ( i = 1; i < 8; i++ )
        *MTU_CFG_TBL(i) = ETH_MAX_DATA_LENGTH;  //  for ETH

    if ( g_eth_wan_mode == 0 )
        setup_oam_htu_entry();

    if ( g_wanqos_en )
    {
        struct wtx_qos_q_desc_cfg wtx_qos_q_desc_cfg = {0};
        struct tx_qos_cfg tx_qos_cfg = {0};
        struct cfg_std_data_len cfg_std_data_len = {0};

        for ( i = 0; i < __ETH_WAN_TX_QUEUE_NUM; i++ )
        {
            wtx_qos_q_desc_cfg.length = ETH_WAN_TX_DESC_NUM(i);
            wtx_qos_q_desc_cfg.addr   = __ETH_WAN_TX_DESC_BASE(i);
            *WTX_QOS_Q_DESC_CFG(i) = wtx_qos_q_desc_cfg;
        }

        tx_qos_cfg.time_tick    = cgu_get_pp32_clock() / 62500; //  16 * (cgu_get_pp32_clock() / 1000000)
        tx_qos_cfg.eth1_eg_qnum = __ETH_WAN_TX_QUEUE_NUM;
        tx_qos_cfg.eth1_qss     = 1;
        tx_qos_cfg.eth1_burst_chk = 1;
        *TX_QOS_CFG = tx_qos_cfg;

        cfg_std_data_len.byte_off = 0;
        cfg_std_data_len.data_len = DMA_PACKET_SIZE;

        *CFG_STD_DATA_LEN = cfg_std_data_len;
    }

    printk(KERN_ERR "setup PPA CFG_WAN_PORTMAP: %#x", g_wan_itf );
    *CFG_WAN_PORTMAP    = g_wan_itf;
    *CFG_MIXED_PORTMAP  = g_eth_wan_mode == 2 /* Mixed Mode */ ? g_wan_itf : 0;
    *TUNNEL_MAX_ID = -1;

#if defined(ENABLE_FW_MODULE_TEST) && ENABLE_FW_MODULE_TEST
    init_acc_tables();
#endif
    printk(KERN_ERR "[%s] g_ethwan_mode = %d \n", __func__, g_eth_wan_mode);
}

static INLINE int alloc_dma(void)
{
    int ret;
    unsigned int pre_alloc_desc_total_num;
    struct sk_buff **skb_pool;
    struct sk_buff **pskb;
    volatile u32 *p;
    int i;

    pre_alloc_desc_total_num = DMA_RX_CH1_DESC_NUM + DMA_RX_CH2_DESC_NUM;
    if ( g_eth_wan_mode == 0 )  //  DSL WAN
        pre_alloc_desc_total_num += CPU_TO_WAN_TX_DESC_NUM + WAN_TX_DESC_NUM_TOTAL + WAN_RX_DESC_NUM(0) * 2;
    else if ( g_wanqos_en )
        pre_alloc_desc_total_num += CPU_TO_WAN_TX_DESC_NUM + WAN_TX_DESC_NUM_TOTAL + DMA_TX_CH1_DESC_NUM;

    skb_pool = (struct sk_buff **)kmalloc(pre_alloc_desc_total_num * sizeof(*skb_pool), GFP_KERNEL);
    if ( skb_pool == NULL )
    {
        ret = -ENOMEM;
        goto ALLOC_SKB_POOL_FAIL;
    }

    for ( i = 0; i < pre_alloc_desc_total_num; i++ )
    {
        skb_pool[i] = alloc_skb_rx();
        if ( !skb_pool[i] )
        {
            ret = -ENOMEM;
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
    if ( !g_dma_device_ppe )
    {
        ret = -EIO;
        goto RESERVE_DMA_FAIL;
    }

#if defined(ENABLE_NAPI) && ENABLE_NAPI
    g_dma_device_ppe->activate_poll = eth_activate_poll;
    g_dma_device_ppe->inactivate_poll = eth_inactivate_poll;
#endif

    g_dma_device_ppe->buffer_alloc    = dma_buffer_alloc;
    g_dma_device_ppe->buffer_free     = dma_buffer_free;
    g_dma_device_ppe->intr_handler    = dma_int_handler;
    g_dma_device_ppe->max_rx_chan_num = 4;
    g_dma_device_ppe->max_tx_chan_num = 4;
    g_dma_device_ppe->tx_burst_len    = 8;
    g_dma_device_ppe->rx_burst_len    = 8;
    *AMAZON_S_SW_PMAC_RX_IPG = (*AMAZON_S_SW_PMAC_RX_IPG & ~0xF0) | 0x0130; //  increase RX FIFO Request Watermark, 0x01?0 means maximum watermark

    for ( i = 0; i < g_dma_device_ppe->max_rx_chan_num; i++ )
    {
        g_dma_device_ppe->rx_chan[i]->packet_size = DMA_PACKET_SIZE;
        g_dma_device_ppe->rx_chan[i]->control     = IFX_DMA_CH_ON;
        //g_dma_device_ppe->rx_chan[i]->channel_packet_drop_enable    = 1;
    }

	// PPA -> DSL
    g_dma_device_ppe->rx_chan[1]->desc_base = (int)DMA_RX_CH1_DESC_BASE;
    g_dma_device_ppe->rx_chan[1]->desc_len  = DMA_RX_CH1_DESC_NUM;
    g_dma_device_ppe->rx_chan[1]->req_irq_to_free   = g_dma_device_ppe->rx_chan[1]->irq;
	g_dma_device_ppe->rx_chan[1]->no_cpu_data = 1;

	/* RX2 -> TX0: (PPA -> Switch(port6))*/
    g_dma_device_ppe->rx_chan[2]->desc_base = (int)DMA_RX_CH2_DESC_BASE;
    g_dma_device_ppe->rx_chan[2]->desc_len  = DMA_RX_CH2_DESC_NUM;
    g_dma_device_ppe->rx_chan[2]->req_irq_to_free   = g_dma_device_ppe->rx_chan[2]->irq;
	g_dma_device_ppe->rx_chan[2]->no_cpu_data = 1;

    for ( i = 0; i < g_dma_device_ppe->max_tx_chan_num; i++ ) {
        g_dma_device_ppe->tx_chan[i]->control     = IFX_DMA_CH_ON;
    }

	// PPA -> Switch(port 6)
    g_dma_device_ppe->tx_chan[0]->desc_base = (int)DMA_TX_CH0_DESC_BASE;
    g_dma_device_ppe->tx_chan[0]->desc_len  = DMA_TX_CH0_DESC_NUM;
    g_dma_device_ppe->tx_chan[0]->global_buffer_len = DMA_PACKET_SIZE;
    g_dma_device_ppe->tx_chan[0]->peri_to_peri      = 1;
    g_dma_device_ppe->tx_chan[0]->no_cpu_data = 1;
	g_dma_device_ppe->tx_chan[0]->req_irq_to_free = g_dma_device_ppe->tx_chan[0]->irq;

    // DSL -> Switch(port11)
    g_dma_device_ppe->tx_chan[1]->desc_base = (int)DMA_TX_CH1_DESC_BASE;
    g_dma_device_ppe->tx_chan[1]->desc_len  = DMA_TX_CH1_DESC_NUM;
    g_dma_device_ppe->tx_chan[1]->global_buffer_len = DMA_PACKET_SIZE;
    g_dma_device_ppe->tx_chan[1]->peri_to_peri      = 1;
    g_dma_device_ppe->tx_chan[1]->no_cpu_data = 1;
	g_dma_device_ppe->tx_chan[1]->req_irq_to_free = g_dma_device_ppe->tx_chan[1]->irq;

    g_dma_device_ppe->port_packet_drop_enable = 1;

    if ( dma_device_register(g_dma_device_ppe) != IFX_SUCCESS )
    {
        err("failed in \"dma_device_register\"");
        ret = -EIO;
        goto DMA_DEVICE_REGISTER_FAIL;
    }

    pskb = skb_pool;

    p = (volatile u32 *)CPU_TO_WAN_TX_DESC_BASE;
    for ( i = 0; i < CPU_TO_WAN_TX_DESC_NUM; i++ )
    {
        ASSERT(((u32)(*pskb)->data & (DMA_TX_ALIGNMENT - 1)) == 0, "CPU to WAN TX data pointer (0x%#x) must be 8 DWORDS aligned", (u32)(*pskb)->data);
        *p++ = g_wanqos_en ? 0x30000000 : 0xB0000000;
        if ( g_eth_wan_mode == 0 || g_wanqos_en )
            *p++ = (u32)(*pskb++)->data & 0x1FFFFFE0;
        else
            *p++ = 0;
    }

    p = (volatile u32 *)DSL_WAN_TX_DESC_BASE(0);
    for ( i = 0; i < WAN_TX_DESC_NUM_TOTAL; i++ )
    {
        ASSERT(((u32)(*pskb)->data & (DMA_TX_ALIGNMENT - 1)) == 0, "WAN (ATM/ETH) TX data pointer (0x%#x) must be 8 DWORDS aligned", (u32)(*pskb)->data);
        *p++ = 0x30000000;
        if ( g_eth_wan_mode == 0 )
            *p++ = ((u32)(*pskb++)->data & 0x1FFFFFE0) >> 2;
        else if ( g_wanqos_en )
            *p++ = (u32)(*pskb++)->data & 0x1FFFFFE0;
        else
            *p++ = 0;
    }

    if ( g_eth_wan_mode == 0 )
    {
        p = (volatile u32 *)WAN_RX_DESC_BASE(0);
        for ( i = 0; i < WAN_RX_DESC_NUM(0); i++ )
        {
            ASSERT(((u32)(*pskb)->data & (DMA_TX_ALIGNMENT - 1)) == 0, "WAN (ATM) RX channel 0 (AAL) data pointer (0x%#x) must be 8 DWORDS aligned", (u32)(*pskb)->data);
            *p++ = 0xB0000000 | DMA_PACKET_SIZE;
            *p++ = ((u32)(*pskb++)->data & 0x1FFFFFE0);
        }

        p = (volatile u32 *)WAN_RX_DESC_BASE(1);
        for ( i = 0; i < WAN_RX_DESC_NUM(1); i++ )
        {
            ASSERT(((u32)(*pskb)->data & 3) == 0, "WAN (ATM) RX channel 1 (OAM) data pointer (0x%#x) must be DWORD aligned", (u32)(*pskb)->data);
            g_wan_rx1_desc_skb[i] = *pskb;
            *p++ = 0xB0000000 | DMA_PACKET_SIZE;
            *p++ = ((u32)(*pskb++)->data & 0x1FFFFFFC) >> 2;
        }
    }

    p = (volatile u32 *)DMA_RX_CH1_DESC_BASE;
    for ( i = 0; i < DMA_RX_CH1_DESC_NUM; i++ )
    {
        ASSERT(((u32)(*pskb)->data & (DMA_RX_ALIGNMENT - 1)) == 0, "DMA RX channel 1 data pointer (0x%#x) must be 8 DWORDS aligned", (u32)(*pskb)->data);
        *p++ = 0x80000000 | DMA_PACKET_SIZE;
        *p++ = (u32)(*pskb++)->data & 0x1FFFFFE0;
    }

    p = (volatile u32 *)DMA_RX_CH2_DESC_BASE;
    for ( i = 0; i < DMA_RX_CH2_DESC_NUM; i++ )
    {
        ASSERT(((u32)(*pskb)->data & (DMA_TX_ALIGNMENT - 1)) == 0, "DMA RX channel 2 data pointer (0x%#x) must be 8 DWORDS aligned", (u32)(*pskb)->data);
        *p++ = 0xB0000000 | DMA_PACKET_SIZE;
        *p++ = (u32)(*pskb++)->data & 0x1FFFFFE0;
    }

    if ( g_wanqos_en )
    {
        p = (volatile u32 *)DMA_TX_CH1_DESC_BASE;
        for ( i = 0; i < DMA_TX_CH1_DESC_NUM; i++ )
        {
            ASSERT(((u32)(*pskb)->data & (DMA_TX_ALIGNMENT - 1)) == 0, "DMA TX channel 1 data pointer (0x%#x) must be 8 DWORDS aligned", (u32)(*pskb)->data);
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

static INLINE void free_dma(void)
{
    volatile struct tx_descriptor *p;
    struct sk_buff *skb_to_free;
    int i;

    dma_device_unregister(g_dma_device_ppe);
    dma_device_release(g_dma_device_ppe);
    g_dma_device_ppe = NULL;

    if ( g_eth_wan_mode == 0 || g_wanqos_en )
    {
        p = (volatile struct tx_descriptor *)CPU_TO_WAN_TX_DESC_BASE;
        for ( i = 0; i < CPU_TO_WAN_TX_DESC_NUM; i++ )
        {
            free_skb_clear_dataptr(p);
            p++;
        }

        p = (volatile struct tx_descriptor *)DSL_WAN_TX_DESC_BASE(0);
        for ( i = 0; i < WAN_TX_DESC_NUM_TOTAL; i++ )
        {
            free_skb_clear_dataptr_shift(p, (g_eth_wan_mode == 0)?2:0 );
            p++;
        }
    }

    if ( g_eth_wan_mode == 0 )
    {
        p = (volatile struct tx_descriptor *)WAN_RX_DESC_BASE(0);
        for ( i = 0; i < WAN_RX_DESC_NUM(0); i++ )
        {
            free_skb_clear_dataptr(p);
            p++;
        }

        p = (volatile struct tx_descriptor *)WAN_RX_DESC_BASE(1);
        for ( i = 0; i < WAN_RX_DESC_NUM(1); i++ )
        {
            free_skb_clear_dataptr_shift(p, 2);
            p++;
        }
    }

    p = (volatile struct tx_descriptor *)DMA_RX_CH1_DESC_BASE;
    for ( i = 0; i < DMA_RX_CH1_DESC_NUM; i++ )
    {
        free_skb_clear_dataptr(p);
        p++;
    }

    p = (volatile struct tx_descriptor *)DMA_RX_CH2_DESC_BASE;
    for ( i = 0; i < DMA_RX_CH2_DESC_NUM; i++ )
    {
        free_skb_clear_dataptr(p);
        p++;
    }

    if ( g_wanqos_en )
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

#if defined(CONFIG_IFX_PPA_DATAPATH)
static INLINE void ethaddr_setup(unsigned int port, char *line)
{
    u8 *p;
    char *ep;
    int i;

    p = MY_ETH0_ADDR + (port ? MAX_ADDR_LEN : 0);
    memset(p, 0, MAX_ADDR_LEN * sizeof(*p));
    for ( i = 0; i < 6; i++ )
    {
        p[i] = line ? simple_strtoul(line, &ep, 16) : 0;
        if ( line )
            line = *ep ? ep + 1 : ep;
    }
    dbg("eth%d mac address %02X-%02X-%02X-%02X-%02X-%02X",
        port ? 1 : 0,
        (u32)p[0] & 0xFF, (u32)p[1] & 0xFF, (u32)p[2] & 0xFF,
        (u32)p[3] & 0xFF, (u32)p[4] & 0xFF, (u32)p[5] & 0xFF);
}
#endif

#define RX_UNICAST_PCKNT                0x00
#define RX_BROADCAST_PCKNT              0x01
#define RX_MULTICAST_PCKNT              0x02
#define RX_CRC_ERR_PCKNT                0x03
#define RX_UNDERSIZE_GOOD_PCKNT         0x04
#define RX_OVER_SIZE_GOOD_PCKNT         0x05
#define RX_UNDERSIZE_ERR_PCKNT          0x06
#define RX_GOOD_PAUSE_PCKNT             0x07
#define RX_OVER_SIZE_ERR_PCKNT          0x08
#define RX_ALLIGN_ERR_PCKNT             0x09
#define RX_FILTERED_PCKNT               0x0A

#define RX_SIZE_64B_PCKNT               0x0B
#define RX_SIZE_65_127B_PCKNT           0x0C
#define RX_SIZE_128_255B_PCKNT          0x0D
#define RX_SIZE_256_511B_PCKNT          0x0E
#define RX_SIZE_512_1023B_PCKNT         0x0F
#define RX_SIZE_1024B_MAX_PCKNT         0x10

#define TX_UNICAST_PCKNT                0x11
#define TX_BROADCAST_PCKNT              0x12
#define TX_MULTICAST_PCKNT              0x13

#define TX_SINGLE_COLLISION_CNT         0x14
#define TX_MULTIPLE_COLLISION_CNT       0x15
#define TX_LATE_COLLISION_CNT           0x16
#define TX_EXCESSIVE_COLLISION_CNT      0x17
#define TX_COLLISION_CNT                0x18
#define TX_PAUSE_PCKNT                  0x19

#define TX_SIZE_64B_PCKNT               0x1A
#define TX_SIZE_65_127B_PCKNT           0x1B
#define TX_SIZE_128_255B_PCKNT          0x1C
#define TX_SIZE_256_511B_PCKNT          0x1D
#define TX_SIZE_512_1023B_PCKNT         0x1E
#define TX_SIZE_1024B_MAX_PCKNT         0x1F


#define TX_DROP_PCKNT                   0x20
#define RX_DROP_PCKNT                   0x21

#define RX_GOOD_BYTE_CNT_LOW            0x23
#define RX_GOOD_BYTE_CNT_HIGH           0x22

#define RX_BAD_BYTE_CNT_LOW             0x25
#define RX_BAD_BYTE_CNT_HIGH            0x24

#define TX_GOOD_BYTE_CNT_LOW            0x27
#define TX_GOOD_BYTE_CNT_HIGH           0x26

static INLINE unsigned long amazon_s_switch_get_rmon_counter_value(int port, int off)
{
    unsigned int val = 0;
    unsigned long counter;

    if ( port < 0 || port >=  3 )
    {
        err("Error !!!  Port:%d is not in limit\n", port);
        return -1;
    }
    while ( (*AMAZON_S_SW_RMON_CTL & 0x800) );
    /* read the specific port+offset counter from RMON counter */
    val = (port << 6) | 0x800 | off;
    *AMAZON_S_SW_RMON_CTL = val;
    while ( (*AMAZON_S_SW_RMON_CTL & 0x800) );
    /* get counters ! */
    counter = *AMAZON_S_SW_RMON_ST;

    return counter;
}

static INLINE void amazon_s_switch_clr_rmon_counter_value(int port)
{
    u32 val;

    while ( (*AMAZON_S_SW_RMON_CTL & 0x800) );
    /* renew the specific port RMON counter */
    if ( port >= 0 )
        val = (port << 6) | 0x800 | (0x2 << 9);
    else
        val = 0x800 | (0x03 << 9);
    *AMAZON_S_SW_RMON_CTL = val;
    while ( (*AMAZON_S_SW_RMON_CTL & 0x800) );
}

struct mac_table_entry
{
    int     fReset;
    int     nFid;
    int     fStatic;
    int     nAgeTimer;
    u32     uPortMap;
    u8      uMac[8];
};

static INLINE int amazon_s_switch_get_mac_table_entry (struct mac_table_entry *pEntry)
{
    unsigned int reg_value;

    ASSERT(pEntry != NULL, "pEntry is NULL");

    /* check if access engine is available */
    while ( (*AMAZON_S_SW_ADR_TB_ST2 & (1 << 31)) );

    if ( pEntry->fReset )
    {
        *AMAZON_S_SW_ADR_TB_CTL2 = (3 << 20) | (0 << 16);   //  Init search pointer to first address

        //  wait for command completion and check result
        do
        {
            udelay(1);
        } while ( ((reg_value = *AMAZON_S_SW_ADR_TB_ST2) & (1 << 31)) );

        if ( (reg_value & (7 << 28)) )
            return -1;
    }

    do
    {
        *AMAZON_S_SW_ADR_TB_CTL1 = (pEntry->nFid & 0x03) << 16;
        *AMAZON_S_SW_ADR_TB_CTL2 = (2 << 20) | (0x0A << 16);  //  search by forwarding group

        while ( ((reg_value = *AMAZON_S_SW_ADR_TB_ST2) & (1 << 31)) );

        if ( (reg_value & (7 << 28)) || (reg_value & (1 << 14)) )
            return -1;

    } while ( !(reg_value & (1 << 13)) );

    pEntry->fStatic     = (reg_value >> 12) & 0x01;
    pEntry->nAgeTimer   = !pEntry->fStatic ? (reg_value & 0x07FF) : 0;
    reg_value = *AMAZON_S_SW_ADR_TB_ST1;
    pEntry->nFid        = (reg_value >> 16) & 0x03;
    pEntry->uPortMap    = (reg_value >> 20) & 0xFF;
    pEntry->uMac[0]     = (reg_value >> 8) & 0xFF;
    pEntry->uMac[1]     = reg_value & 0xFF;
    reg_value = *AMAZON_S_SW_ADR_TB_ST0;
    pEntry->uMac[2]     = (reg_value >> 24) & 0xFF;
    pEntry->uMac[3]     = (reg_value >> 16) & 0xFF;
    pEntry->uMac[4]     = (reg_value >> 8) & 0xFF;
    pEntry->uMac[5]     = reg_value & 0xFF;

    return 0;
}


#if defined(ENABLE_DIRECTPATH_TX_QUEUE) && ENABLE_DIRECTPATH_TX_QUEUE
static int send_directpath_tx_queue(void)
{
    unsigned long sys_flag;
    struct sk_buff *skb;
    int len;
    int i;

    for ( i = 0; !g_directpath_dma_full && i < NUM_ENTITY(ppa_pushthrough_data); i++ )
    {
        while ( !g_directpath_dma_full && ppa_pushthrough_data[i].skb_list )
        {
            local_irq_save(sys_flag);

            skb = skb_get(ppa_pushthrough_data[i].skb_list);

	    len = skb->len;
	    if (unlikely(skb->len < ETH_MIN_TX_PACKET_LENGTH + sizeof(struct sw_eg_pkt_header))){
		    if (skb_pad(skb, ETH_MIN_TX_PACKET_LENGTH + sizeof(struct sw_eg_pkt_header) - skb->len )){
			    return -1;
		    }
		    len =  ETH_MIN_TX_PACKET_LENGTH + sizeof(struct sw_eg_pkt_header);
		    skb->len = len;
	    }

            put_skb_to_dbg_pool(skb);
            if ( dma_device_write(g_dma_device_ppe, skb->data, len, skb) == len )
            {
                //  remove skb from list
                if ( skb->prev == skb )
                    skb->prev = skb->next = ppa_pushthrough_data[i].skb_list = NULL;
                else
                {
                    ppa_pushthrough_data[i].skb_list = skb->next;
                    ppa_pushthrough_data[i].skb_list->prev = skb->prev;
                    skb->prev->next = skb->next;
                    skb->prev = skb->next = NULL;
                }
  #if defined(ENABLE_DIRECTPATH_TX_QUEUE_SIZE)
                ppa_pushthrough_data[i].skb_list_size -= skb->len;
  #else
                ppa_pushthrough_data[i].skb_list_len--;
  #endif
                dev_kfree_skb_any(skb);
            }
            else
            {
                local_irq_restore(sys_flag);
                dev_kfree_skb_any(skb);
                return -1;
            }

            local_irq_restore(sys_flag);

            dev_kfree_skb_any(skb);

            if ( ppa_pushthrough_data[i].callback.start_tx_fn
  #if defined(ENABLE_DIRECTPATH_TX_QUEUE_SIZE)
                 && ppa_pushthrough_data[i].skb_list_size <= (ENABLE_DIRECTPATH_TX_QUEUE_SIZE * 3 / 4)
  #else
                 && ppa_pushthrough_data[i].skb_list_len <= (ENABLE_DIRECTPATH_TX_QUEUE_PKTS * 3 / 4)
  #endif
               )
                ppa_pushthrough_data[i].callback.start_tx_fn(ppa_pushthrough_data[i].netif);
        }
    }

    return 0;
}
#endif



/*
 * ####################################
 *           Global Function
 * ####################################
 */

int32_t ppa_datapath_generic_hook(PPA_GENERIC_HOOK_CMD cmd, void *buffer, uint32_t flag)
{
    dbg("ppa_datapath_generic_hook cmd 0x%x\n", cmd );
    switch ( cmd )
    {
    case PPA_GENERIC_DATAPATH_TSET:
        return IFX_SUCCESS;

    case PPA_GENERIC_DATAPATH_ADDR_TO_FPI_ADDR:
        {
            PPA_FPI_ADDR *addr = (PPA_FPI_ADDR *) buffer;
            addr->addr_fpi = sb_addr_to_fpi_addr_convert(addr->addr_orig);
            return IFX_SUCCESS;
        }
    case PPA_GENERIC_DATAPATH_CLEAR_MIB:
        {
            clear_mib_counter(0x80000000);

            return IFX_SUCCESS;
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
        dbg("ppa_datapath_generic_hook not support cmd 0x%x\n", cmd );
        return IFX_FAILURE;
    }

    return IFX_FAILURE;

}

static int atm_showtime_enter(struct port_cell_info *port_cell, void *xdata_addr)
{
    int i, j;

    ASSERT(port_cell != NULL, "port_cell is NULL");
    ASSERT(xdata_addr != NULL, "xdata_addr is NULL");

    for ( j = 0; j < ATM_PORT_NUMBER && j < port_cell->port_num; j++ )
        if ( port_cell->tx_link_rate[j] > 0 )
            break;
    for ( i = 0; i < ATM_PORT_NUMBER && i < port_cell->port_num; i++ )
        g_atm_priv_data.port[i].tx_max_cell_rate = port_cell->tx_link_rate[i] > 0 ? port_cell->tx_link_rate[i] : port_cell->tx_link_rate[j];

    qsb_global_set();

    for ( i = 0; i < g_atm_priv_data.max_connections; i++ )
        if ( (g_atm_priv_data.connection_table & (1 << i)) && g_atm_priv_data.connection[i].vcc != NULL )
            set_qsb(g_atm_priv_data.connection[i].vcc, &g_atm_priv_data.connection[i].vcc->qos, i);

    //  TODO: ReTX set xdata_addr
    g_xdata_addr = xdata_addr;

    g_showtime = 1;

    dbg("enter showtime, cell rate: 0 - %d, 1 - %d, xdata addr: 0x%08x", g_atm_priv_data.port[0].tx_max_cell_rate, g_atm_priv_data.port[1].tx_max_cell_rate, (unsigned int)g_xdata_addr);

    return IFX_SUCCESS;
}

static int atm_showtime_exit(void)
{
    if ( !g_showtime )
        return IFX_ERROR;

    g_showtime = 0;

    //  TODO: ReTX clean state
    g_xdata_addr = NULL;

    dbg("leave showtime");

    return IFX_SUCCESS;
}

int get_dslwan_qid_with_vcc(struct atm_vcc *vcc, unsigned int prio) {
	int res = 0;
    int conn = find_vcc(vcc);

	if (conn >= 0 && conn < ATM_QUEUE_NUMBER){
	   res |= g_atm_priv_data.connection[conn].prio_queue_map[prio > 7 ? 7 : prio];
       res |= (conn << 8);
    }

	return res;
}


int get_netif_qid_with_pkt(struct sk_buff *skb, void *arg, int is_atm_vcc)
{
    if ( is_atm_vcc )
    {
        int conn = find_vcc((struct atm_vcc *)arg);

        if ( conn < 0 )
            return -1;

        return (conn << 8) | g_atm_priv_data.connection[conn].prio_queue_map[skb->priority > 7 ? 7 : skb->priority];
    }
    else
    {
        int port = get_eth_port((struct net_device *)arg);
        if ( port >= 0 )
            return g_eth_prio_queue_map[skb->priority >= NUM_ENTITY(g_eth_prio_queue_map) ? NUM_ENTITY(g_eth_prio_queue_map) - 1 : skb->priority];
    }

    return 0;
}

int ppe_directpath_send(uint32_t off, struct sk_buff *skb, int32_t len, uint32_t flags)
{
    int ret;
    unsigned long sys_flag;
    unsigned int dpid;
	uint32_t if_id = PPA_VDEV_ID(off);
    
    dpid = (g_wan_itf & (1 << if_id)) ? 2 : if_id;

    //  Careful, no any safety check here.
    //  Parameters must be correct.

    // ppa_pushthrough_data[off].tx_pkt++;

    if ( skb_headroom(skb) < sizeof(struct sw_eg_pkt_header) )
    {
        struct sk_buff *new_skb;

        new_skb = alloc_skb_tx(skb->len + sizeof(struct sw_eg_pkt_header));
        if ( !new_skb )
        {
            dev_kfree_skb_any(skb);
            // ppa_pushthrough_data[off].tx_pkt_dropped++;
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

    ret = 0;
#if defined(CONFIG_IFX_PPA_API_DIRECTPATH_TASKLET)
    return ppe_directpath_enqueue(if_id, skb);
#endif

    spin_lock_irqsave(&g_directpath_tx_spinlock, sys_flag);
#if !defined(ENABLE_DIRECTPATH_TX_QUEUE) || !ENABLE_DIRECTPATH_TX_QUEUE
    if ( eth_xmit(skb, dpid /* CPU or dummy */, DMA_CHAN_NR_DIRECTPATH_CPU_TO_PPE, if_id /* spid */, -1 /* SW classification */) )
    {
        // ppa_pushthrough_data[off].tx_pkt_dropped++;
        ret = -1;
    }
#else
    if ( !g_directpath_dma_full && !ppa_pushthrough_data[if_id - 3].skb_list )
    {
        if ( eth_xmit(skb, dpid /* CPU or dummy */, DMA_CHAN_NR_DIRECTPATH_CPU_TO_PPE, if_id /* spid */, -1 /* SW classification */) )
        {
            ppa_pushthrough_data[off].tx_pkt_dropped++;
            ret = -1;
        }
    }
    else
    {
#if defined(CONFIG_IFX_PPA_API_DIRECTPATH_TASKLET)    
        ret = ppe_directpath_enqueue(if_id, skb);
#endif
    }
#endif
    spin_unlock_irqrestore(&g_directpath_tx_spinlock, sys_flag);

    return ret;
}

int ppe_directpath_rx_stop(uint32_t if_id __attribute__ ((unused)), uint32_t flags __attribute__ ((unused))) {
   return 0;
}

int ppe_directpath_rx_start(uint32_t if_id __attribute__ ((unused)), uint32_t flags __attribute__ ((unused))) {
   return 0;
}


int ppe_clk_change(unsigned int arg, unsigned int flags) {
   int i;
   unsigned int ppe_cfg_bits = (1 << 16);
   unsigned long sys_flag;


   if (flags &= FLAG_PPE_CLK_CHANGE_ARG_AVM_DIRCT){

      local_irq_save(sys_flag);
      switch(arg){
      case 300:
         *AMAZON_S_CGU_SYS &= ~(1 << 7);
         *AMAZON_S_CGU_UPDATE = 1;
         break;
      case 250:
         *AMAZON_S_CGU_SYS |= 1 << 7;
         *AMAZON_S_CGU_UPDATE = 1;
         break;
      }
      local_irq_restore(sys_flag);
      udelay(1000);
      printk(KERN_ERR "clk=%d \n", arg );
   }
    qsb_global_set();

    for ( i = 0; i < g_atm_priv_data.max_connections; i++ )
        if ( (g_atm_priv_data.connection_table & (1 << i)) && g_atm_priv_data.connection[i].vcc != NULL )
            set_qsb(g_atm_priv_data.connection[i].vcc, &g_atm_priv_data.connection[i].vcc->qos, i);

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void avmnet_swi_ar9_a5_register_xmit (void) {
	struct net_device *dev;
	int i;
    AVMNET_DEBUG("[%s] setup new xmit function for %d devices \n", __FUNCTION__ , num_ar9_eth_devs);
	for ( i = 0; i < num_ar9_eth_devs; i++) {
		struct net_device_ops * ops;
		dev = g_eth_net_dev[i];

        AVMNET_DEBUG("[%s] dev=%p \n", __FUNCTION__, dev);
		ops = dev->netdev_ops;
        AVMNET_DEBUG("[%s] ops=%p \n", __FUNCTION__, ops);
		ops->ndo_start_xmit = &ifx_ppa_eth_hard_start_xmit_a5;
		ops->ndo_tx_timeout = &ifx_ppa_eth_tx_timeout_a5;
		ops->ndo_do_ioctl = &ifx_ppa_eth_ioctl_a5;
	}
    AVMNET_DEBUG("[%s] done \n", __FUNCTION__);
}


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

int a5_in_avm_ata_mode(void) {
	// printk(KERN_ERR "[%s]: %s\n", __FUNCTION__, (g_eth_wan_mode == 3 )?"ATA":"NO ATA");
	return (g_eth_wan_mode == 3);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static int __init ppe_a5_init(void)
{
    int ret;
    int i;
    char buf[512];
    struct port_cell_info port_cell = {0};
    int showtime = 0;
    void *xdata_addr = NULL;

    if ( g_datapath_mod_count ){
		printk("E5 ( MII0/1  + ATM ) driver already loaded\n");
		return -EBUSY;
	}

    printk("Loading A5 (MII0/1 + ATM) driver [%d]......\n ", g_datapath_mod_count);
    g_datapath_mod_count++;

	if (avm_ata_dev)
	    printk("avm_ata_dev: '%s' \n" , avm_ata_dev);


#if IFX_PPA_DP_DBG_PARAM_ENABLE
    if( ifx_ppa_drv_dp_dbg_param_enable == 1 )
    {
        ethwan = ifx_ppa_drv_dp_dbg_param_ethwan;
        wanitf = ifx_ppa_drv_dp_dbg_param_wanitf;
        ipv6_acc_en= ifx_ppa_drv_dp_dbg_param_ipv6_acc_en;
        wanqos_en= ifx_ppa_drv_dp_dbg_param_wanqos_en;
    }
#endif  //IFX_PPA_DP_DBG_PARAM_ENABLE

    g_stop_datapath = 1;

    // tell driver to suspend packet flow on all devices
    if(avmnet_hw_config_entry->config->suspend){
        avmnet_hw_config_entry->config->suspend(avmnet_hw_config_entry->config, NULL);
    }else{
        printk(KERN_ERR "[%s] Config error, no suspend function found!\n", __func__);
        BUG();
    }

    // (7) stop NAPI, empty skb backlog queues
    dbg(  "[%s] flushing skb backlog queues\n", __FUNCTION__);
    for_each_possible_cpu(i){
        struct sk_buff *skb, *tmp;
        struct softnet_data *queue;
        queue = &per_cpu(g_ar9_eth_queue, i);

        napi_disable(&queue->backlog);

        skb_queue_walk_safe(&(queue->input_pkt_queue), skb, tmp){
            __skb_unlink(skb, &(queue->input_pkt_queue));
            dev_kfree_skb_any(skb);
        }
    }

    dma_shutdown_gracefully_ar9();

    reset_ppe();

    init_local_variables();

    init_hw();

    init_communication_data_structures(g_fwcode);

    ret = alloc_dma();
    if ( ret )
        goto ALLOC_DMA_FAIL;

    printk("[%s] reached line=%d\n", __FUNCTION__, __LINE__);
    ppa_hook_mpoa_setup = mpoa_setup;
#ifdef AVMNET_VCC_QOS
    vcc_map_skb_prio_qos_queue = vcc_map_skb_prio_qos_queue_datapath;
    vcc_set_nr_qos_queues = vcc_set_nr_qos_queues_datapath;
#endif

    for ( i = 0; i < ATM_PORT_NUMBER; i++ )
    {
	g_atm_priv_data.port[i].dev =
		ifx_atm_dev_register("ifx_atm", NULL, &g_ppe_atm_ops, -1, 0UL);
        if ( !g_atm_priv_data.port[i].dev )
        {
            ret = -EIO;
            goto ATM_DEV_REGISTER_FAIL;
        }
        else
        {
            g_atm_priv_data.port[i].dev->ci_range.vpi_bits = 8;
            g_atm_priv_data.port[i].dev->ci_range.vci_bits = 16;
            g_atm_priv_data.port[i].dev->link_rate = g_atm_priv_data.port[i].tx_max_cell_rate;
            g_atm_priv_data.port[i].dev->dev_data = (void*)i;
        }
    }
    printk("[%s] reached line=%d\n", __FUNCTION__, __LINE__);

    ret = request_irq(PPE_MAILBOX_IGU0_INT, mailbox0_irq_handler, IRQF_DISABLED, "a5_mailbox0_isr", NULL);
    if ( ret )
    {
        if ( ret == -EBUSY )
            err("IRQ may be occupied by other PPE driver, please reconfig to disable it.\n");
        goto REQUEST_IGU0_IRQ_FAIL;
    }
    printk("[%s] reached line=%d\n", __FUNCTION__, __LINE__);

    ret = request_irq(PPE_MAILBOX_IGU1_INT, mailbox1_irq_handler, IRQF_DISABLED, "a5_mailbox_isr", NULL);
    if ( ret )
    {
        if ( ret == -EBUSY )
            err("IRQ may be occupied by other PPE driver, please reconfig to disable it.\n");
        goto REQUEST_IRQ_FAIL;
    }
    printk("[%s] reached line=%d\n", __FUNCTION__, __LINE__);

#ifdef CONFIG_MII1_AUTONEG
    if ( g_id == GPHY_VSC )
        ret = request_irq(MII1_GPHY_INT, mii1_gphy_VSC_int_handler, IRQF_DISABLED, "mii1_gphy_isr", NULL);
    else
        ret = request_irq(MII1_GPHY_INT, mii1_gphy_11G_int_handler, IRQF_DISABLED, "mii1_gphy_isr", NULL);
    if ( ret )
        goto MII1_GPHY_REQUEST_IRQ_FAIL;
    mii1_gphy_config_ext_int();
    mii1_gphy_config_gphy();
#endif

    /*
     *  init variable for directpath
     */

    //don't clear pushthtrough data in Datapath-Init in AVM Setup -> we use common pushthrough_data for all datapath modules
    // memset(ppa_pushthrough_data, 0, sizeof(ppa_pushthrough_data));

    dump_init();

    printk("[%s] reached line=%d\n", __FUNCTION__, __LINE__);
    ret = pp32_start(g_fwcode);
    if ( ret )
        goto PP32_START_FAIL;
    printk("[%s] reached line=%d\n", __FUNCTION__, __LINE__);

    *MBOX_IGU0_IER = g_eth_wan_mode ? DMA_TX_CH0_SIG | DMA_TX_CH1_SIG : DMA_TX_CH0_SIG;
    *MBOX_IGU1_IER = g_mailbox_signal_mask;

    qsb_global_set();
    if ( g_eth_wan_mode == 0 )
        validate_oam_htu_entry();


    if ( g_eth_wan_mode == 0 /* DSL WAN */ )
    {
    	avm_wan_mode = AVM_WAN_MODE_ATM;
    	//TODO hier wird beim VR0 tx_chan[0] u. rx_chan[2] konfiguriert

    	//  turn on DMA TX channels (0, 1) for fast path
        g_dma_device_ppe->tx_chan[0]->open(g_dma_device_ppe->tx_chan[0]); // ppa -> switch( port 6 )
        g_dma_device_ppe->rx_chan[2]->open(g_dma_device_ppe->rx_chan[2]); // ppa -> switch( port 6 )

        //TODO: Test tx_1 ch hier schon oeffnen, da offensichtlich das erste paket verloren geht?!
        g_dma_device_ppe->tx_chan[1]->open(g_dma_device_ppe->tx_chan[1]);
        //TODO: Test Ende

    	if (!IS_ERR(&ifx_mei_atm_showtime_check) && &ifx_mei_atm_showtime_check)
            ifx_mei_atm_showtime_check(&showtime, &port_cell, &xdata_addr);
    	else
            printk("Weak-Symbol nicht gefunden: ifx_mei_atm_showtime_check\n");
    	if (showtime)
            atm_showtime_enter(&port_cell, xdata_addr);

        //  init timer for Data Led
#if defined(ENABLE_LED_FRAMEWORK) && ENABLE_LED_FRAMEWORK && defined(CONFIG_IFX_LED)
        ifx_led_trigger_register("dsl_data", &g_data_led_trigger);
#endif
    }
    if ( g_eth_wan_mode == 3 /* ETH WAN */ ) {
    	avm_wan_mode = AVM_WAN_MODE_ETH;

    }

    /*  create proc file    */
    proc_file_create();

    if ( g_eth_wan_mode == 0 /* DSL WAN */ ) {
        if (!IS_ERR(&ifx_mei_atm_showtime_enter) && &ifx_mei_atm_showtime_enter)
            ifx_mei_atm_showtime_enter = atm_showtime_enter;
        else
            printk("Weak-Symbol nicht gefunden: ifx_mei_atm_showtime_enter\n");

        if (!IS_ERR(&ifx_mei_atm_showtime_exit) && &ifx_mei_atm_showtime_exit)
            ifx_mei_atm_showtime_exit = atm_showtime_exit;
        else
            printk("Weak-Symbol nicht gefunden: ifx_mei_atm_showtime_exit\n");

    } else {
        printk(KERN_ERR "[%s] warning: runnung A5 not in DSL mode!\n", __FUNCTION__);
    }

    for_each_possible_cpu(i) {
        struct softnet_data *queue;
        struct sk_buff *skb, *tmp;

        queue = &per_cpu(g_ar9_eth_queue, i);

        skb_queue_head_init(&queue->input_pkt_queue);
        queue->completion_queue = NULL;
        INIT_LIST_HEAD(&queue->poll_list);

        queue->backlog.poll = process_backlog_a5;
        napi_enable(&queue->backlog);
    }

    turn_on_dma_rx(1);

    /* (re-)aktivieren der MACs */
    // TKL: wird jetzt am Schluss durch resume() gemacht
    //avmnet_swi_ar9_reinit_macs( avmnet_hw_config_entry->config );

	/* reaktiviern des cpu-ports: mii2 */
    start_mii2();

    /* neue (a5 spezifische) xmit-Funktionen registrieren */
	avmnet_swi_ar9_a5_register_xmit ( );

    /* disable learning, flush mac table*/
	avmnet_swi_ar9_disable_learning( avmnet_hw_config_entry->config );
	g_ingress_direct_forward_enable = 0;
	g_egress_direct_forward_enable = 1;

    //Fix warning message ---start
	ifx_ppa_drv_avm_get_atm_qid_hook = avm_get_atm_qid;
	ifx_ppa_drv_avm_get_ptm_qid_hook = NULL;
	ifx_ppa_drv_avm_get_eth_qid_hook = avm_get_eth_qid;
    ifx_ppa_drv_ppe_clk_change_hook = ppe_clk_change;

    in_avm_ata_mode_hook = a5_in_avm_ata_mode;

    ppa_pushthrough_hook = ppe_directpath_send;

#ifdef CONFIG_IFX_PPA_API_DIRECTPATH
    ifx_ppa_drv_g_ppe_directpath_data = ppa_pushthrough_data;
    ifx_ppa_drv_directpath_send_hook = ppe_directpath_send;
    ifx_ppa_drv_directpath_rx_start_hook = ppe_directpath_rx_start;
    ifx_ppa_drv_directpath_rx_stop_hook = ppe_directpath_rx_stop;
#endif //CONFIG_IFX_PPA_API_DIRECTPATH
    ifx_ppa_drv_datapath_generic_hook = ppa_datapath_generic_hook;
    //Fix warning message ---end

    g_stop_datapath = 0;    // restart after init

    // tell driver to resume packet flow on all devices
    if(avmnet_hw_config_entry->config->resume){
        avmnet_hw_config_entry->config->resume(avmnet_hw_config_entry->config, NULL);
    }else{
        printk(KERN_ERR "[%s] Config error, no resume function found!\n", __func__);
        BUG();
    }

    printk("Succeeded!\n");
    print_driver_ver(buf, sizeof(buf), "PPE datapath driver info", VER_FAMILY, VER_DRTYPE, VER_INTERFACE, VER_ACCMODE, VER_MAJOR, VER_MID, VER_MINOR);
    printk(buf);
    print_fw_ver(buf, sizeof(buf));
    printk(buf);

    return 0;

PP32_START_FAIL:

    printk("[%s] Fail line=%d\n", __FUNCTION__, __LINE__);
#ifdef CONFIG_MII1_AUTONEG
    free_irq(MII1_GPHY_INT, NULL);
MII1_GPHY_REQUEST_IRQ_FAIL:
#endif
    free_irq(PPE_MAILBOX_IGU1_INT, NULL);
REQUEST_IRQ_FAIL:
    printk("[%s] Fail line=%d\n", __FUNCTION__, __LINE__);
    free_irq(PPE_MAILBOX_IGU0_INT, NULL);
REQUEST_IGU0_IRQ_FAIL:
    printk("[%s] Fail line=%d\n", __FUNCTION__, __LINE__);
ATM_DEV_REGISTER_FAIL:
    printk("[%s] Fail line=%d\n", __FUNCTION__, __LINE__);
    while ( i-- )
        atm_dev_deregister(g_atm_priv_data.port[i].dev);
    ppa_hook_mpoa_setup = NULL;
#ifdef AVMNET_VCC_QOS
    vcc_map_skb_prio_qos_queue = NULL;
    vcc_set_nr_qos_queues = NULL;
#endif

    free_dma();
ALLOC_DMA_FAIL:
    printk("[%s] Fail line=%d\n", __FUNCTION__, __LINE__);
    clear_local_variables();

    g_datapath_mod_count--;
   	avm_wan_mode = AVM_WAN_MODE_NOT_CONFIGURED;

    return ret;
}

static void __exit ppe_a5_exit(void)
{
    int i;

    g_stop_datapath = 1;

    // tell driver to suspend packet flow on all devices
    if(avmnet_hw_config_entry->config->suspend){
        avmnet_hw_config_entry->config->suspend(avmnet_hw_config_entry->config, NULL);
    }else{
        printk(KERN_ERR "[%s] Config error, no suspend function found!\n", __func__);
        BUG();
    }

    // stop NAPI, empty skb backlog queues
    dbg("[%s] flushing skb backlog queues\n", __FUNCTION__);
    for_each_possible_cpu(i){
        struct softnet_data *queue;
        struct sk_buff *skb, *tmp;

        queue = &per_cpu(g_ar9_eth_queue, i);

        napi_disable(&queue->backlog);
        queue->backlog.poll = NULL;

        skb_queue_walk_safe(&(queue->input_pkt_queue), skb, tmp){
            __skb_unlink(skb, &(queue->input_pkt_queue));
            dev_kfree_skb_any(skb);
        }
    }

    //Fix warning message ---start
	ifx_ppa_drv_avm_get_atm_qid_hook = NULL;
	ifx_ppa_drv_avm_get_ptm_qid_hook = NULL;
	ifx_ppa_drv_avm_get_eth_qid_hook = NULL;
    ifx_ppa_drv_ppe_clk_change_hook = NULL;

#ifdef CONFIG_IFX_PPA_API_DIRECTPATH
    ifx_ppa_drv_g_ppe_directpath_data = NULL;
    ifx_ppa_drv_directpath_send_hook = NULL;
    ifx_ppa_drv_directpath_rx_start_hook = NULL;
    ifx_ppa_drv_directpath_rx_stop_hook = NULL;
#endif //CONFIG_IFX_PPA_API_DIRECTPATH
   ifx_ppa_drv_datapath_generic_hook = NULL;
    //Fix warning message ---end

   in_avm_ata_mode_hook = NULL;

   ppa_pushthrough_hook = NULL;

    if ( g_eth_wan_mode == 0 /* DSL WAN */ ) {
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

    if ( g_eth_wan_mode == 0 /* DSL WAN */ )
    {
#if defined(ENABLE_LED_FRAMEWORK) && ENABLE_LED_FRAMEWORK && defined(CONFIG_IFX_LED)
        ifx_led_trigger_deregister(g_data_led_trigger);
        g_data_led_trigger = NULL;
#endif
    }

    stop_datapath();

    if ( g_eth_wan_mode == 0 )
        invalidate_oam_htu_entry();

    pp32_stop();

#ifdef CONFIG_MII1_AUTONEG
    free_irq(MII1_GPHY_INT, NULL);
#endif

    free_irq(PPE_MAILBOX_IGU1_INT, NULL);

    free_irq(PPE_MAILBOX_IGU0_INT, NULL);
#if defined(SETUP_OWN_NETDEVS)
    for ( i = 0; i < NUM_ENTITY(g_eth_net_dev); i++ )
	{
        ifx_eth_fw_unregister_netdev(g_eth_net_dev[i], 0);
        ifx_eth_fw_free_netdev(g_eth_net_dev[i], 0);
        g_eth_net_dev[i] = NULL;
    }
#endif

    for ( i = 0; i < ATM_PORT_NUMBER; i++ )
        atm_dev_deregister(g_atm_priv_data.port[i].dev);

    ppa_hook_mpoa_setup = NULL;
#ifdef AVMNET_VCC_QOS
    vcc_map_skb_prio_qos_queue = NULL;
    vcc_set_nr_qos_queues = NULL;
#endif


    free_dma();

    clear_local_variables();

    reinit_ar9_common_eth( avmnet_hw_config_entry->config );

    // tell driver to resume packet flow on all devices
    if(avmnet_hw_config_entry->config->resume){
        avmnet_hw_config_entry->config->resume(avmnet_hw_config_entry->config, NULL);
    }else{
        printk(KERN_ERR "[%s] Config error, no resume function found!\n", __func__);
        BUG();
    }

	g_stop_datapath = 0;

    printk("Unload A5 (MII0/1 + ATM) driver [%d]......\n ", g_datapath_mod_count);
    g_datapath_mod_count--;
   	avm_wan_mode = AVM_WAN_MODE_NOT_CONFIGURED;
}

#if defined(CONFIG_IFX_PPA_DATAPATH)
static int __init eth0addr_setup(char *line)
{
    ethaddr_setup(0, line);

    return 0;
}

static int __init eth1addr_setup(char *line)
{
    ethaddr_setup(1, line);

    return 0;
}

static int __init wan_mode_setup(char *line)
{
    if ( strcmp(line, "1") == 0 )
        ethwan = 1;
    else if ( strcmp(line, "2") == 0 )
        ethwan = 2;

    return 0;
}

static int __init wanitf_setup(char *line)
{
    wanitf = simple_strtoul(line, NULL, 0);

    if ( wanitf > 0xFF )
        wanitf = ~0;

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
    int tmp_wanqos_en = simple_strtoul(line, NULL, 0);

    if ( isdigit(*line) && tmp_wanqos_en >= 0 && tmp_wanqos_en <= 8 )
        wanqos_en = tmp_wanqos_en;

    return 0;
}

 #if defined(CONFIG_IFX_PPA_DIRECTPATH_TX_QUEUE_SIZE)
static int __init directpath_tx_queue_size_setup(char *line)
{
    unsigned long i;

    i = simple_strtoul(line, NULL, 0);
    directpath_tx_queue_size = (int)i;

    return 0;
}
 #endif

 #if defined(CONFIG_IFX_PPA_DIRECTPATH_TX_QUEUE_PKTS)
static int __init directpath_tx_queue_pkts_setup(char *line)
{
    unsigned long i;

    i = simple_strtoul(line, NULL, 0);
    directpath_tx_queue_pkts = (int)i;

    return 0;
}
 #endif
#endif



module_init(ppe_a5_init);
module_exit(ppe_a5_exit);
#if defined(CONFIG_IFX_PPA_DATAPATH)
  __setup("ethaddr=", eth0addr_setup);
  __setup("eth1addr=", eth1addr_setup);
  __setup("ethwan=", wan_mode_setup);
  __setup("wanitf=", wanitf_setup);
  __setup("ipv6_acc_en=", ipv6_acc_en_setup);
  __setup("wanqos_en=", wanqos_en_setup);
 #if defined(CONFIG_IFX_PPA_DIRECTPATH_TX_QUEUE_SIZE)
  __setup("directpath_tx_queue_size=", directpath_tx_queue_size_setup);
 #endif
 #if defined(CONFIG_IFX_PPA_DIRECTPATH_TX_QUEUE_PKTS)
  __setup("directpath_tx_queue_pkts=", directpath_tx_queue_pkts_setup);
 #endif
#endif



MODULE_LICENSE("GPL");
