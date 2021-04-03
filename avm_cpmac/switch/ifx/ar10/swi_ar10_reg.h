/*
 * swi_ar10_reg.h
 *
 *  Created on: 02.08.2012
 *      Author: tklaassen
 */

#if !defined(SWI_REG_H_)
#error Do not include machine dependent register definitions directly!
#endif

#ifndef SWI_AR10_REG_H_
#define SWI_AR10_REG_H_

#define PP32_NUM_CORES  1

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
 *  FPI Configuration Bus Register and Memory Address Mapping
 */
#define IFX_PPE                                 KSEG1ADDR(0x1E180000)
#define AMAZON_S_PPE                            KSEG1ADDR(0x1E180000)
#define PP32_DEBUG_REG_ADDR(x)                  ((volatile u32*)(AMAZON_S_PPE + (((x) + 0x0000) << 2)))

#define PPM_INT_REG_ADDR(x)                     ((volatile unsigned int *)(IFX_PPE + (((x) + 0x0030) << 2)))
#define PP32_INTERNAL_RES_ADDR(x)               ((volatile unsigned int *)(IFX_PPE + (((x) + 0x0040) << 2)))
#define PPE_CLOCK_CONTROL_ADDR(x)               ((volatile unsigned int *)(IFX_PPE + (((x) + 0x0100) << 2)))
#define CDM_CODE_MEMORY_RAM0_ADDR(x)            ((volatile unsigned int *)(IFX_PPE + (((x) + 0x1000) << 2)))
#define CDM_CODE_MEMORY_RAM1_ADDR(x)            ((volatile unsigned int *)(IFX_PPE + (((x) + 0x2000) << 2)))
#define CDM_CODE_MEMORY(i, x)                   CDM_CODE_MEMORY_RAM0_ADDR(x)
#define PPE_REG_ADDR(x)                         ((volatile unsigned int *)(IFX_PPE + (((x) + 0x4000) << 2)))
#define PP32_DATA_MEMORY_RAM1_ADDR(x)           ((volatile unsigned int *)(IFX_PPE + (((x) + 0x5000) << 2)))
//#define CDM_DATA_MEMORY(i, x)                   ((volatile unsigned int*)(IFX_PPE + (((x) + 0x004000 + (i) * 0x00010000) << 2)))
#define PPM_INT_UNIT_ADDR(x)                    ((volatile unsigned int *)(IFX_PPE + (((x) + 0x6000) << 2)))
#define PPM_TIMER0_ADDR(x)                      ((volatile unsigned int *)(IFX_PPE + (((x) + 0x6100) << 2)))
#define PPM_TASK_IND_REG_ADDR(x)                ((volatile unsigned int *)(IFX_PPE + (((x) + 0x6200) << 2)))
#define PPS_BRK_ADDR(x)                         ((volatile unsigned int *)(IFX_PPE + (((x) + 0x6300) << 2)))
#define PPM_TIMER1_ADDR(x)                      ((volatile unsigned int *)(IFX_PPE + (((x) + 0x6400) << 2)))
#define SB_RAM0_ADDR(x)                         ((volatile unsigned int *)(IFX_PPE + (((x) + 0x8000) << 2)))
#define SB_RAM1_ADDR(x)                         ((volatile unsigned int *)(IFX_PPE + (((x) + 0x8800) << 2)))
#define SB_RAM2_ADDR(x)                         ((volatile unsigned int *)(IFX_PPE + (((x) + 0x9000) << 2)))
#define SB_RAM3_ADDR(x)                         ((volatile unsigned int *)(IFX_PPE + (((x) + 0x9800) << 2)))
#define SB_RAM4_ADDR(x)                         ((volatile unsigned int *)(IFX_PPE + (((x) + 0xA000) << 2)))
#define QSB_CONF_REG(x)                         ((volatile unsigned int *)(IFX_PPE + (((x) + 0xC000) << 2)))


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
 *  Share Buffer Registers
 */
#define SB_MST_PRI0                             PPE_REG_ADDR(0x0300)
#define SB_MST_PRI1                             PPE_REG_ADDR(0x0301)

/*
 *  DPlus Registers
 */
#define DM_RXDB                                 PPE_REG_ADDR(0x0612)
#define DM_RXCB                                 PPE_REG_ADDR(0x0613)
#define DM_RXCFG                                PPE_REG_ADDR(0x0614)
#define DM_RXPGCNT                              PPE_REG_ADDR(0x0615)
#define DM_RXPKTCNT                             PPE_REG_ADDR(0x0616)
#define DS_RXDB                                 PPE_REG_ADDR(0x0710)
#define DS_RXCB                                 PPE_REG_ADDR(0x0711)
#define DS_RXCFG                                PPE_REG_ADDR(0x0712)
#define DS_RXPGCNT                              PPE_REG_ADDR(0x0713)

/*
 *  EMA Registers
 */
#define EMA_CMDCFG                              PPE_REG_ADDR(0x0A00)
#define EMA_DATACFG                             PPE_REG_ADDR(0x0A01)
#define EMA_CMDCNT                              PPE_REG_ADDR(0x0A02)
#define EMA_DATACNT                             PPE_REG_ADDR(0x0A03)
#define EMA_ISR                                 PPE_REG_ADDR(0x0A04)
#define EMA_IER                                 PPE_REG_ADDR(0x0A05)
#define EMA_CFG                                 PPE_REG_ADDR(0x0A06)
#define EMA_SUBID                               PPE_REG_ADDR(0x0A07)

/*
 *  PP32 Debug Control Register
 */
#define PP32_DBG_CTRL                           PP32_DEBUG_REG_ADDR(0x0000)

#define DBG_CTRL_RESTART                        0
#define DBG_CTRL_STOP                           1

/*
 *  PP32 Debug Control Register
 */
#define PP32_FREEZE                             PPE_REG_ADDR(0x0000)
#define PP32_SRST                               PPE_REG_ADDR(0x0020)

/*
 *  PP32 Registers
 */

// Amazon-S

#define PP32_CTRL_CMD                           PP32_DEBUG_REG_ADDR(0x0B00)
  #define PP32_CTRL_CMD_RESTART                 (1 << 0)
  #define PP32_CTRL_CMD_STOP                    (1 << 1)
  #define PP32_CTRL_CMD_STEP                    (1 << 2)
  #define PP32_CTRL_CMD_BREAKOUT                (1 << 3)

#define PP32_CTRL_OPT                           PP32_DEBUG_REG_ADDR(0x0C00)
  #define PP32_CTRL_OPT_BREAKOUT_ON_STOP_ON     (3 << 0)
  #define PP32_CTRL_OPT_BREAKOUT_ON_STOP_OFF    (2 << 0)
  #define PP32_CTRL_OPT_BREAKOUT_ON_BREAKIN_ON  (3 << 2)
  #define PP32_CTRL_OPT_BREAKOUT_ON_BREAKIN_OFF (2 << 2)
  #define PP32_CTRL_OPT_STOP_ON_BREAKIN_ON      (3 << 4)
  #define PP32_CTRL_OPT_STOP_ON_BREAKIN_OFF     (2 << 4)
  #define PP32_CTRL_OPT_STOP_ON_BREAKPOINT_ON   (3 << 6)
  #define PP32_CTRL_OPT_STOP_ON_BREAKPOINT_OFF  (2 << 6)
  #define PP32_CTRL_OPT_BREAKOUT_ON_STOP        (*PP32_CTRL_OPT & (1 << 0))
  #define PP32_CTRL_OPT_BREAKOUT_ON_BREAKIN     (*PP32_CTRL_OPT & (1 << 2))
  #define PP32_CTRL_OPT_STOP_ON_BREAKIN         (*PP32_CTRL_OPT & (1 << 4))
  #define PP32_CTRL_OPT_STOP_ON_BREAKPOINT      (*PP32_CTRL_OPT & (1 << 6))

#define PP32_BRK_PC(i)                          PP32_DEBUG_REG_ADDR(0x0900 + (i) * 2)
#define PP32_BRK_PC_MASK(i)                     PP32_DEBUG_REG_ADDR(0x0901 + (i) * 2)
#define PP32_BRK_DATA_ADDR(i)                   PP32_DEBUG_REG_ADDR(0x0904 + (i) * 2)
#define PP32_BRK_DATA_ADDR_MASK(i)              PP32_DEBUG_REG_ADDR(0x0905 + (i) * 2)
#define PP32_BRK_DATA_VALUE_RD(i)               PP32_DEBUG_REG_ADDR(0x0908 + (i) * 2)
#define PP32_BRK_DATA_VALUE_RD_MASK(i)          PP32_DEBUG_REG_ADDR(0x0909 + (i) * 2)
#define PP32_BRK_DATA_VALUE_WR(i)               PP32_DEBUG_REG_ADDR(0x090C + (i) * 2)
#define PP32_BRK_DATA_VALUE_WR_MASK(i)          PP32_DEBUG_REG_ADDR(0x090D + (i) * 2)
  #define PP32_BRK_CONTEXT_MASK(i)              (1 << (i))
  #define PP32_BRK_CONTEXT_MASK_EN              (1 << 4)
  #define PP32_BRK_COMPARE_GREATER_EQUAL        (1 << 5)    //  valid for break data value rd/wr only
  #define PP32_BRK_COMPARE_LOWER_EQUAL          (1 << 6)
  #define PP32_BRK_COMPARE_EN                   (1 << 7)

#define PP32_BRK_TRIG                           PP32_DEBUG_REG_ADDR(0x0F00)
  #define PP32_BRK_GRPi_PCn_ON(i, n)            ((3 << ((n) * 2)) << ((i) * 16))
  #define PP32_BRK_GRPi_PCn_OFF(i, n)           ((2 << ((n) * 2)) << ((i) * 16))
  #define PP32_BRK_GRPi_DATA_ADDRn_ON(i, n)     ((3 << ((n) * 2 + 4)) << ((i) * 16))
  #define PP32_BRK_GRPi_DATA_ADDRn_OFF(i, n)    ((2 << ((n) * 2 + 4)) << ((i) * 16))
  #define PP32_BRK_GRPi_DATA_VALUE_RDn_ON(i, n) ((3 << ((n) * 2 + 8)) << ((i) * 16))
  #define PP32_BRK_GRPi_DATA_VALUE_RDn_OFF(i, n)((2 << ((n) * 2 + 8)) << ((i) * 16))
  #define PP32_BRK_GRPi_DATA_VALUE_WRn_ON(i, n) ((3 << ((n) * 2 + 12)) << ((i) * 16))
  #define PP32_BRK_GRPi_DATA_VALUE_WRn_OFF(i, n)((2 << ((n) * 2 + 12)) << ((i) * 16))
  #define PP32_BRK_GRPi_PCn(i, n)               (*PP32_BRK_TRIG & ((1 << ((n))) << ((i) * 8)))
  #define PP32_BRK_GRPi_DATA_ADDRn(i, n)        (*PP32_BRK_TRIG & ((1 << ((n) + 2)) << ((i) * 8)))
  #define PP32_BRK_GRPi_DATA_VALUE_RDn(i, n)    (*PP32_BRK_TRIG & ((1 << ((n) + 4)) << ((i) * 8)))
  #define PP32_BRK_GRPi_DATA_VALUE_WRn(i, n)    (*PP32_BRK_TRIG & ((1 << ((n) + 6)) << ((i) * 8)))


#define PP32_CPU_STATUS                         PP32_DEBUG_REG_ADDR(0x0D00)
#define PP32_HALT_STAT                          PP32_CPU_STATUS
#define PP32_DBG_CUR_PC                         PP32_CPU_STATUS
  #define PP32_CPU_USER_STOPPED                 (*PP32_CPU_STATUS & (1 << 0))
  #define PP32_CPU_USER_BREAKIN_RCV             (*PP32_CPU_STATUS & (1 << 1))
  #define PP32_CPU_USER_BREAKPOINT_MET          (*PP32_CPU_STATUS & (1 << 2))
  #define PP32_CPU_CUR_PC                       (*PP32_CPU_STATUS >> 16)

#define PP32_BREAKPOINT_REASONS                 PP32_DEBUG_REG_ADDR(0x0A00)
  #define PP32_BRK_PC_MET(i)                    (*PP32_BREAKPOINT_REASONS & (1 << (i)))
  #define PP32_BRK_DATA_ADDR_MET(i)             (*PP32_BREAKPOINT_REASONS & (1 << ((i) + 2)))
  #define PP32_BRK_DATA_VALUE_RD_MET(i)         (*PP32_BREAKPOINT_REASONS & (1 << ((i) + 4)))
  #define PP32_BRK_DATA_VALUE_WR_MET(i)         (*PP32_BREAKPOINT_REASONS & (1 << ((i) + 6)))
  #define PP32_BRK_DATA_VALUE_RD_LO_EQ(i)       (*PP32_BREAKPOINT_REASONS & (1 << ((i) * 2 + 8)))
  #define PP32_BRK_DATA_VALUE_RD_GT_EQ(i)       (*PP32_BREAKPOINT_REASONS & (1 << ((i) * 2 + 9)))
  #define PP32_BRK_DATA_VALUE_WR_LO_EQ(i)       (*PP32_BREAKPOINT_REASONS & (1 << ((i) * 2 + 12)))
  #define PP32_BRK_DATA_VALUE_WR_GT_EQ(i)       (*PP32_BREAKPOINT_REASONS & (1 << ((i) * 2 + 13)))
  #define PP32_BRK_CUR_CONTEXT                  ((*PP32_BREAKPOINT_REASONS >> 16) & 0x03)


#define PP32_GP_REG_BASE                        PP32_DEBUG_REG_ADDR(0x0E00)
#define PP32_GP_CONTEXTi_REGn(i, n)             PP32_DEBUG_REG_ADDR(0x0E00 + (i) * 16 + (n))

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
#define SFSM_CFG(x)                             PPE_REG_ADDR((0x416 + x)) // x between 0 -1
#define SFSM_DBA(x)                             PPE_REG_ADDR((0x412 + x)) // x between 0 -1
#define SFSM_CBA(x)                             PPE_REG_ADDR((0x414 + x)) // x between 0 -1
#define FFSM_DBA(x)                             PPE_REG_ADDR((0x508 + x)) // x between 0 -1
#define FFSM_CFG(x)                             PPE_REG_ADDR((0x50A + x)) // x between 0 -1


/*
 *  Reset Registers
 */
#define AMAZON_S_RCU_BASE_ADDR                  (KSEG1 | 0x1F203000)
#define AMAZON_S_RCU_RST_REQ                    ((volatile u32*)(AMAZON_S_RCU_BASE_ADDR + 0x0010))
#define AMAZON_S_RCU_RST_STAT                   ((volatile u32*)(AMAZON_S_RCU_BASE_ADDR + 0x0014))
#define AMAZON_S_USB_CFG                        ((volatile u32*)(AMAZON_S_RCU_BASE_ADDR + 0x0018))
#define AMAZON_S_RCU_PPE_CONF                   ((volatile u32*)(AMAZON_S_RCU_BASE_ADDR + 0x002C))

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

  /*
  *  SLL Registers
  */
#define SLL_CMD1                                PPE_REG_ADDR(0x900)
#define SLL_CMD0                                PPE_REG_ADDR(0x901)
#define SLL_KEY(x)                              PPE_REG_ADDR(0x910+x)
#define SLL_RESULT                              PPE_REG_ADDR(0x920)

/*
  * Chip ID Registers
*/
#define AR10_MIPS_CHIPID_BASE                   (KSEG1 | 0x1F107340)
#define AR10_MIPS_CHIPID                        (volatile u32*)(AR10_MIPS_CHIPID_BASE + 0x4)

#endif /* SWI_AR10_REG_H_ */
