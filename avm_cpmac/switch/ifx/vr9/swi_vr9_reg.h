/**
 ** FILE NAME    : ifxmips_7port_sw_reg.h
 ** PROJECT      : IFX UEIP
 ** MODULES      : ETH
 ** DATE         : 24 July 2009
 ** AUTHOR       : Reddy Mallikarjuna
 ** DESCRIPTION  : IFX ETH driver header file
 ** COPYRIGHT    :       Copyright (c) 2009
 **                      Infineon Technologies AG
 **                      Am Campeon 1-12, 85579 Neubiberg, Germany
 **
 **    This program is free software; you can redistribute it and/or modify
 **    it under the terms of the GNU General Public License as published by
 **    the Free Software Foundation; either version 2 of the License, or
 **    (at your option) any later version.
 **
 ** HISTORY
 ** $Date            $Author         $Comment
 ** 24 July 2009     Reddy Mallikarjuna  Initial release
 *******************************************************************************/

#if !defined(SWI_REG_H_) && !defined(IFXMIPS_PPA_HAL_VR9_A5_H) && !defined(IFXMIPS_PPA_HAL_VR9_E5_H)
#error Do not include machine dependent register definitions directly!
#endif

#ifndef _SWI_VR9_REG_H
#define _SWI_VR9_REG_H

/*!
 \file ifxmips_7port_sw_reg.h
 \ingroup IFX_ETH_DRV
 \brief IFX Eth module register definition for VRx platforms
 */

#define PP32_NUM_CORES  2

/*
 *  FPI Configuration Bus Register and Memory Address Mapping
 */

#define IFX_PPE_ADDR                                  0x1E200000

#if defined(CONFIG_IFX_PPE_E5_OFFCHIP_BONDING)
  #define IFX_PPE_ADDR_BOND(idx)                      ((idx) == 0 ? 0x1E200000 : ((idx) == 1 ? g_slave_ppe_base_addr : g_ppe_base_addr))
  extern unsigned int g_slave_ppe_base_addr;
  extern unsigned int g_ppe_base_addr;
#else
#define IFX_PPE_ADDR_BOND(idx)                        IFX_PPE_ADDR
#endif
#define IFX_PPE                                       KSEG1ADDR(IFX_PPE_ADDR)
#define IFX_PPE_BOND(idx)                             KSEG1ADDR(IFX_PPE_ADDR_BOND(idx))
#define PP32_DEBUG_REG_ADDR(i, x)                    ((volatile unsigned int*)(IFX_PPE_BOND(2) + (((x) + 0x000000 + (i) * 0x00010000) << 2)))
#define CDM_CODE_MEMORY(i, x)                        ((volatile unsigned int*)(IFX_PPE_BOND(2) + (((x) + 0x001000 + (i) * 0x00010000) << 2)))
#define CDM_DATA_MEMORY(i, x)                        ((volatile unsigned int*)(IFX_PPE_BOND(2) + (((x) + 0x004000 + (i) * 0x00010000) << 2)))
#define QSB_CONF_REG_ADDR(x)                         ((volatile unsigned int*)(IFX_PPE_BOND(2) + (((x) + 0x00E000) << 2)))
#define QSB_CONF_REG(x)                               QSB_CONF_REG_ADDR(x)


#define SB_RAM0_ADDR_BOND(idx, x)                    ((volatile unsigned int*)(IFX_PPE_BOND(idx) + (((x) + 0x008000) << 2)))
#define SB_RAM1_ADDR_BOND(idx, x)                    ((volatile unsigned int*)(IFX_PPE_BOND(idx) + (((x) + 0x009000) << 2)))
#define SB_RAM2_ADDR_BOND(idx, x)                    ((volatile unsigned int*)(IFX_PPE_BOND(idx) + (((x) + 0x00A000) << 2)))
#define SB_RAM3_ADDR_BOND(idx, x)                    ((volatile unsigned int*)(IFX_PPE_BOND(idx) + (((x) + 0x00B000) << 2)))
#define PPE_REG_ADDR_BOND(idx, x)                    ((volatile unsigned int*)(IFX_PPE_BOND(idx) + (((x) + 0x00D000) << 2)))
#define SB_RAM6_ADDR_BOND(idx, x)                    ((volatile unsigned int*)(IFX_PPE_BOND(idx) + (((x) + 0x018000) << 2)))

#define SB_RAM0_ADDR(x)                    ((volatile unsigned int*)(IFX_PPE + (((x) + 0x008000) << 2)))
#define SB_RAM1_ADDR(x)                    ((volatile unsigned int*)(IFX_PPE + (((x) + 0x009000) << 2)))
#define SB_RAM2_ADDR(x)                    ((volatile unsigned int*)(IFX_PPE + (((x) + 0x00A000) << 2)))
#define SB_RAM3_ADDR(x)                    ((volatile unsigned int*)(IFX_PPE + (((x) + 0x00B000) << 2)))
#define PPE_REG_ADDR(x)                    ((volatile unsigned int*)(IFX_PPE + (((x) + 0x00D000) << 2)))
#define SB_RAM6_ADDR(x)                    ((volatile unsigned int*)(IFX_PPE + (((x) + 0x018000) << 2)))

/*
 *  DPlus Registers
 */
#define DM_RXDB                                 PPE_REG_ADDR_BOND(0, 0x0612)
#define DM_RXCB                                 PPE_REG_ADDR_BOND(0, 0x0613)
#define DM_RXCFG                                PPE_REG_ADDR_BOND(0, 0x0614)
#define DM_RXPGCNT                              PPE_REG_ADDR_BOND(0, 0x0615)
#define DM_RXPKTCNT                             PPE_REG_ADDR_BOND(0, 0x0616)
#define DS_RXDB                                 PPE_REG_ADDR_BOND(0, 0x0710)
#define DS_RXCB                                 PPE_REG_ADDR_BOND(0, 0x0711)
#define DS_RXCFG                                PPE_REG_ADDR_BOND(0, 0x0712)
#define DS_RXPGCNT                              PPE_REG_ADDR_BOND(0, 0x0713)

/*
 *  SAR Registers
 */
#define SAR_MODE_CFG                            PPE_REG_ADDR_BOND(2, 0x080A)
#define SAR_RX_CMD_CNT                          PPE_REG_ADDR_BOND(2, 0x080B)
#define SAR_TX_CMD_CNT                          PPE_REG_ADDR_BOND(2, 0x080C)
#define SAR_RX_CTX_CFG                          PPE_REG_ADDR_BOND(2, 0x080D)
#define SAR_TX_CTX_CFG                          PPE_REG_ADDR_BOND(2, 0x080E)
#define SAR_TX_CMD_DONE_CNT                     PPE_REG_ADDR_BOND(2, 0x080F)
#define SAR_POLY_CFG_SET0                       PPE_REG_ADDR_BOND(2, 0x0812)
#define SAR_POLY_CFG_SET1                       PPE_REG_ADDR_BOND(2, 0x0813)
#define SAR_POLY_CFG_SET2                       PPE_REG_ADDR_BOND(2, 0x0814)
#define SAR_POLY_CFG_SET3                       PPE_REG_ADDR_BOND(2, 0x0815)
#define SAR_CRC_SIZE_CFG                        PPE_REG_ADDR_BOND(2, 0x0816)

/*
 *  PDMA/EMA Registers
 */
#define PDMA_CFG                                PPE_REG_ADDR_BOND(2, 0x0A00)
#define PDMA_RX_CMDCNT                          PPE_REG_ADDR_BOND(2, 0x0A01)
#define PDMA_TX_CMDCNT                          PPE_REG_ADDR_BOND(2, 0x0A02)
#define PDMA_RX_FWDATACNT                       PPE_REG_ADDR_BOND(2, 0x0A03)
#define PDMA_TX_FWDATACNT                       PPE_REG_ADDR_BOND(2, 0x0A04)
#define PDMA_RX_CTX_CFG                         PPE_REG_ADDR_BOND(2, 0x0A05)
#define PDMA_TX_CTX_CFG                         PPE_REG_ADDR_BOND(2, 0x0A06)
#define PDMA_RX_MAX_LEN_REG                     PPE_REG_ADDR_BOND(2, 0x0A07)
#define PDMA_RX_DELAY_CFG                       PPE_REG_ADDR_BOND(2, 0x0A08)
#define PDMA_INT_FIFO_RD                        PPE_REG_ADDR_BOND(2, 0x0A09)
#define PDMA_ISR                                PPE_REG_ADDR_BOND(2, 0x0A0A)
#define PDMA_IER                                PPE_REG_ADDR_BOND(2, 0x0A0B)
#define PDMA_SUBID                              PPE_REG_ADDR_BOND(2, 0x0A0C)
#define PDMA_BAR0                               PPE_REG_ADDR_BOND(2, 0x0A0D)
#define PDMA_BAR1                               PPE_REG_ADDR_BOND(2, 0x0A0E)

#define SAR_PDMA_RX_CMDBUF_CFG                  PPE_REG_ADDR_BOND(2, 0x0F00)
#define SAR_PDMA_TX_CMDBUF_CFG                  PPE_REG_ADDR_BOND(2, 0x0F01)
#define SAR_PDMA_RX_FW_CMDBUF_CFG               PPE_REG_ADDR_BOND(2, 0x0F02)
#define SAR_PDMA_TX_FW_CMDBUF_CFG               PPE_REG_ADDR_BOND(2, 0x0F03)
#define SAR_PDMA_RX_CMDBUF_STATUS               PPE_REG_ADDR_BOND(2, 0x0F04)
#define SAR_PDMA_TX_CMDBUF_STATUS               PPE_REG_ADDR_BOND(2, 0x0F05)

/*
 *  PP32 Debug Control Register
 */
#define PP32_FREEZE                             PPE_REG_ADDR_BOND(2, 0x0000)
#define PP32_SRST                               PPE_REG_ADDR_BOND(2, 0x0020)

/*
 *  SLL Registers
 */
#define SLL_CMD1                                PPE_REG_ADDR_BOND(2, 0x900)
#define SLL_CMD0                                PPE_REG_ADDR_BOND(2, 0x901)
#define SLL_KEY(x)                              PPE_REG_ADDR_BOND(2, 0x910+x)
#define SLL_RESULT                              PPE_REG_ADDR_BOND(2, 0x920)

#define PP32_DBG_CTRL(n)                        PP32_DEBUG_REG_ADDR(n, 0x0000)

#define DBG_CTRL_RESTART                        0
#define DBG_CTRL_STOP                           1

#define PP32_CTRL_CMD(n)                        PP32_DEBUG_REG_ADDR(n, 0x0B00)
#define PP32_CTRL_CMD_RESTART                 (1 << 0)
#define PP32_CTRL_CMD_STOP                    (1 << 1)
#define PP32_CTRL_CMD_STEP                    (1 << 2)
#define PP32_CTRL_CMD_BREAKOUT                (1 << 3)

#define PP32_CTRL_OPT(n)                        PP32_DEBUG_REG_ADDR(n, 0x0C00)
#define PP32_CTRL_OPT_BREAKOUT_ON_STOP_ON     (3 << 0)
#define PP32_CTRL_OPT_BREAKOUT_ON_STOP_OFF    (2 << 0)
#define PP32_CTRL_OPT_BREAKOUT_ON_BREAKIN_ON  (3 << 2)
#define PP32_CTRL_OPT_BREAKOUT_ON_BREAKIN_OFF (2 << 2)
#define PP32_CTRL_OPT_STOP_ON_BREAKIN_ON      (3 << 4)
#define PP32_CTRL_OPT_STOP_ON_BREAKIN_OFF     (2 << 4)
#define PP32_CTRL_OPT_STOP_ON_BREAKPOINT_ON   (3 << 6)
#define PP32_CTRL_OPT_STOP_ON_BREAKPOINT_OFF  (2 << 6)
#define PP32_CTRL_OPT_BREAKOUT_ON_STOP(n)     (*PP32_CTRL_OPT(n) & (1 << 0))
#define PP32_CTRL_OPT_BREAKOUT_ON_BREAKIN(n)  (*PP32_CTRL_OPT(n) & (1 << 2))
#define PP32_CTRL_OPT_STOP_ON_BREAKIN(n)      (*PP32_CTRL_OPT(n) & (1 << 4))
#define PP32_CTRL_OPT_STOP_ON_BREAKPOINT(n)   (*PP32_CTRL_OPT(n) & (1 << 6))

#define PP32_BRK_PC(n, i)                       PP32_DEBUG_REG_ADDR(n, 0x0900 + (i) * 2)
#define PP32_BRK_PC_MASK(n, i)                  PP32_DEBUG_REG_ADDR(n, 0x0901 + (i) * 2)
#define PP32_BRK_DATA_ADDR(n, i)                PP32_DEBUG_REG_ADDR(n, 0x0904 + (i) * 2)
#define PP32_BRK_DATA_ADDR_MASK(n, i)           PP32_DEBUG_REG_ADDR(n, 0x0905 + (i) * 2)
#define PP32_BRK_DATA_VALUE_RD(n, i)            PP32_DEBUG_REG_ADDR(n, 0x0908 + (i) * 2)
#define PP32_BRK_DATA_VALUE_RD_MASK(n, i)       PP32_DEBUG_REG_ADDR(n, 0x0909 + (i) * 2)
#define PP32_BRK_DATA_VALUE_WR(n, i)            PP32_DEBUG_REG_ADDR(n, 0x090C + (i) * 2)
#define PP32_BRK_DATA_VALUE_WR_MASK(n, i)       PP32_DEBUG_REG_ADDR(n, 0x090D + (i) * 2)
#  define PP32_BRK_CONTEXT_MASK(i)              (1 << (i))
#  define PP32_BRK_CONTEXT_MASK_EN              (1 << 4)
#  define PP32_BRK_COMPARE_GREATER_EQUAL        (1 << 5)    //  valid for break data value rd/wr only
#  define PP32_BRK_COMPARE_LOWER_EQUAL          (1 << 6)
#  define PP32_BRK_COMPARE_EN                   (1 << 7)

#define PP32_BRK_TRIG(n)                        PP32_DEBUG_REG_ADDR(n, 0x0F00)
#  define PP32_BRK_GRPi_PCn_ON(i, n)            ((3 << ((n) * 2)) << ((i) * 16))
#  define PP32_BRK_GRPi_PCn_OFF(i, n)           ((2 << ((n) * 2)) << ((i) * 16))
#  define PP32_BRK_GRPi_DATA_ADDRn_ON(i, n)     ((3 << ((n) * 2 + 4)) << ((i) * 16))
#  define PP32_BRK_GRPi_DATA_ADDRn_OFF(i, n)    ((2 << ((n) * 2 + 4)) << ((i) * 16))
#  define PP32_BRK_GRPi_DATA_VALUE_RDn_ON(i, n) ((3 << ((n) * 2 + 8)) << ((i) * 16))
#  define PP32_BRK_GRPi_DATA_VALUE_RDn_OFF(i, n)((2 << ((n) * 2 + 8)) << ((i) * 16))
#  define PP32_BRK_GRPi_DATA_VALUE_WRn_ON(i, n) ((3 << ((n) * 2 + 12)) << ((i) * 16))
#  define PP32_BRK_GRPi_DATA_VALUE_WRn_OFF(i, n)((2 << ((n) * 2 + 12)) << ((i) * 16))
#  define PP32_BRK_GRPi_PCn(k, i, n)            (*PP32_BRK_TRIG(k) & ((1 << ((n))) << ((i) * 8)))
#  define PP32_BRK_GRPi_DATA_ADDRn(k, i, n)     (*PP32_BRK_TRIG(k) & ((1 << ((n) + 2)) << ((i) * 8)))
#  define PP32_BRK_GRPi_DATA_VALUE_RDn(k, i, n) (*PP32_BRK_TRIG(k) & ((1 << ((n) + 4)) << ((i) * 8)))
#  define PP32_BRK_GRPi_DATA_VALUE_WRn(k, i, n) (*PP32_BRK_TRIG(k) & ((1 << ((n) + 6)) << ((i) * 8)))

#define PP32_CPU_STATUS(n)                      PP32_DEBUG_REG_ADDR(n, 0x0D00)
#define PP32_HALT_STAT(n)                       PP32_CPU_STATUS(n)
#define PP32_DBG_CUR_PC(n)                      PP32_CPU_STATUS(n)
#  define PP32_CPU_USER_STOPPED(n)              (*PP32_CPU_STATUS(n) & (1 << 0))
#  define PP32_CPU_USER_BREAKIN_RCV(n)          (*PP32_CPU_STATUS(n) & (1 << 1))
#  define PP32_CPU_USER_BREAKPOINT_MET(n)       (*PP32_CPU_STATUS(n) & (1 << 2))
#  define PP32_CPU_CUR_PC(n)                    (*PP32_CPU_STATUS(n) >> 16)

#define PP32_BREAKPOINT_REASONS(n)              PP32_DEBUG_REG_ADDR(n, 0x0A00)
#  define PP32_BRK_PC_MET(n, i)                 (*PP32_BREAKPOINT_REASONS(n) & (1 << (i)))
#  define PP32_BRK_DATA_ADDR_MET(n, i)          (*PP32_BREAKPOINT_REASONS(n) & (1 << ((i) + 2)))
#  define PP32_BRK_DATA_VALUE_RD_MET(n, i)      (*PP32_BREAKPOINT_REASONS(n) & (1 << ((i) + 4)))
#  define PP32_BRK_DATA_VALUE_WR_MET(n, i)      (*PP32_BREAKPOINT_REASONS(n) & (1 << ((i) + 6)))
#  define PP32_BRK_DATA_VALUE_RD_LO_EQ(n, i)    (*PP32_BREAKPOINT_REASONS(n) & (1 << ((i) * 2 + 8)))
#  define PP32_BRK_DATA_VALUE_RD_GT_EQ(n, i)    (*PP32_BREAKPOINT_REASONS(n) & (1 << ((i) * 2 + 9)))
#  define PP32_BRK_DATA_VALUE_WR_LO_EQ(n, i)    (*PP32_BREAKPOINT_REASONS(n) & (1 << ((i) * 2 + 12)))
#  define PP32_BRK_DATA_VALUE_WR_GT_EQ(n, i)    (*PP32_BREAKPOINT_REASONS(n) & (1 << ((i) * 2 + 13)))
#  define PP32_BRK_CUR_CONTEXT(n)               ((*PP32_BREAKPOINT_REASONS(n) >> 16) & 0x03)

#define PP32_GP_REG_BASE(n)                     PP32_DEBUG_REG_ADDR(n, 0x0E00)
#define PP32_GP_CONTEXTi_REGn(n, i, j)          PP32_DEBUG_REG_ADDR(n, 0x0E00 + (i) * 16 + (j))

/*
 *    Code/Data Memory (CDM) Interface Configuration Register
 */
#define CDM_CFG                                 PPE_REG_ADDR_BOND(2, 0x0100)

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
#define MBOX_IGU0_ISRS                          PPE_REG_ADDR_BOND(0, 0x0200)
#define MBOX_IGU0_ISRC                          PPE_REG_ADDR_BOND(0, 0x0201)
#define MBOX_IGU0_ISR                           PPE_REG_ADDR_BOND(0, 0x0202)
#define MBOX_IGU0_IER                           PPE_REG_ADDR_BOND(0, 0x0203)

/*
 *  Mailbox IGU1 Registers
 */
#define MBOX_IGU1_ISRS                          PPE_REG_ADDR_BOND(0, 0x0204)
#define MBOX_IGU1_ISRC                          PPE_REG_ADDR_BOND(0, 0x0205)
#define MBOX_IGU1_ISR                           PPE_REG_ADDR_BOND(0, 0x0206)
#define MBOX_IGU1_IER                           PPE_REG_ADDR_BOND(0, 0x0207)

/*
 *  RTHA/TTHA Registers
 */
#define RFBI_CFG                                PPE_REG_ADDR_BOND(2, 0x0400)
#define RBA_CFG0                                PPE_REG_ADDR_BOND(2, 0x0404)
#define RBA_CFG1                                PPE_REG_ADDR_BOND(2, 0x0405)
#define RCA_CFG0                                PPE_REG_ADDR_BOND(2, 0x0408)
#define RCA_CFG1                                PPE_REG_ADDR_BOND(2, 0x0409)
#define RDES_CFG0                               PPE_REG_ADDR_BOND(2, 0x040C)
#define RDES_CFG1                               PPE_REG_ADDR_BOND(2, 0x040D)
#define SFSM_STATE0                             PPE_REG_ADDR_BOND(2, 0x0410)
#define SFSM_STATE1                             PPE_REG_ADDR_BOND(2, 0x0411)
#define SFSM_DBA0                               PPE_REG_ADDR_BOND(2, 0x0412)
#define SFSM_DBA1                               PPE_REG_ADDR_BOND(2, 0x0413)
#define SFSM_CBA0                               PPE_REG_ADDR_BOND(2, 0x0414)
#define SFSM_CBA1                               PPE_REG_ADDR_BOND(2, 0x0415)
#define SFSM_CFG0                               PPE_REG_ADDR_BOND(2, 0x0416)
#define SFSM_CFG1                               PPE_REG_ADDR_BOND(2, 0x0417)
#define SFSM_PGCNT0                             PPE_REG_ADDR_BOND(2, 0x041C)
#define SFSM_PGCNT1                             PPE_REG_ADDR_BOND(2, 0x041D)
#define FFSM_DBA0                               PPE_REG_ADDR_BOND(2, 0x0508)
#define FFSM_DBA1                               PPE_REG_ADDR_BOND(2, 0x0509)
#define FFSM_CFG0                               PPE_REG_ADDR_BOND(2, 0x050A)
#define FFSM_CFG1                               PPE_REG_ADDR_BOND(2, 0x050B)
#define FFSM_IDLE_HEAD_BC0                      PPE_REG_ADDR_BOND(2, 0x050E)
#define FFSM_IDLE_HEAD_BC1                      PPE_REG_ADDR_BOND(2, 0x050F)
#define FFSM_PGCNT0                             PPE_REG_ADDR_BOND(2, 0x0514)
#define FFSM_PGCNT1                             PPE_REG_ADDR_BOND(2, 0x0515)

#define SFSM_DBA(i)                             ((volatile struct SFSM_dba *)  PPE_REG_ADDR_BOND(2, 0x0412 + (i)))
#define SFSM_CBA(i)                             ((volatile struct SFSM_cba *)  PPE_REG_ADDR_BOND(2, 0x0414 + (i)))
#define SFSM_CFG(i)                             ((volatile struct SFSM_cfg *)  PPE_REG_ADDR_BOND(2, 0x0416 + (i)))
#define SFSM_PGCNT(i)                           ((volatile struct SFSM_pgcnt *)PPE_REG_ADDR_BOND(2, 0x041C + (i)))
#define FFSM_DBA(i)                             ((volatile struct FFSM_dba *)  PPE_REG_ADDR_BOND(2, 0x0508 + (i)))
#define FFSM_CFG(i)                             ((volatile struct FFSM_cfg *)  PPE_REG_ADDR_BOND(2, 0x050A + (i)))
#define FFSM_PGCNT(i)                           ((volatile struct FFSM_pgcnt *)PPE_REG_ADDR_BOND(2, 0x0514 + (i)))

/*
 *  PPE TC Logic Registers (partial)
 */
#define DREG_A_VERSION                          PPE_REG_ADDR_BOND(2, 0x0D00)
#define DREG_A_CFG                              PPE_REG_ADDR_BOND(2, 0x0D01)
#define DREG_AT_CTRL                            PPE_REG_ADDR_BOND(2, 0x0D02)
#define DREG_AT_CB_CFG0                         PPE_REG_ADDR_BOND(2, 0x0D03)
#define DREG_AT_CB_CFG1                         PPE_REG_ADDR_BOND(2, 0x0D04)
#define DREG_AR_CTRL                            PPE_REG_ADDR_BOND(2, 0x0D08)
#define DREG_AR_CB_CFG0                         PPE_REG_ADDR_BOND(2, 0x0D09)
#define DREG_AR_CB_CFG1                         PPE_REG_ADDR_BOND(2, 0x0D0A)
#define DREG_A_UTPCFG                           PPE_REG_ADDR_BOND(2, 0x0D0E)
#define DREG_A_STATUS                           PPE_REG_ADDR_BOND(2, 0x0D0F)
#define DREG_AT_CFG0                            PPE_REG_ADDR_BOND(2, 0x0D20)
#define DREG_AT_CFG1                            PPE_REG_ADDR_BOND(2, 0x0D21)
#define DREG_AT_FB_SIZE0                        PPE_REG_ADDR_BOND(2, 0x0D22)
#define DREG_AT_FB_SIZE1                        PPE_REG_ADDR_BOND(2, 0x0D23)
#define DREG_AT_CELL0                           PPE_REG_ADDR_BOND(2, 0x0D24)
#define DREG_AT_CELL1                           PPE_REG_ADDR_BOND(2, 0x0D25)
#define DREG_AT_IDLE_CNT0                       PPE_REG_ADDR_BOND(2, 0x0D26)
#define DREG_AT_IDLE_CNT1                       PPE_REG_ADDR_BOND(2, 0x0D27)
#define DREG_AT_IDLE0                           PPE_REG_ADDR_BOND(2, 0x0D28)
#define DREG_AT_IDLE1                           PPE_REG_ADDR_BOND(2, 0x0D29)
#define DREG_AR_CFG0                            PPE_REG_ADDR_BOND(2, 0x0D60)
#define DREG_AR_CFG1                            PPE_REG_ADDR_BOND(2, 0x0D61)
#define DREG_AR_CELL0                           PPE_REG_ADDR_BOND(2, 0x0D68)
#define DREG_AR_CELL1                           PPE_REG_ADDR_BOND(2, 0x0D69)
#define DREG_AR_IDLE_CNT0                       PPE_REG_ADDR_BOND(2, 0x0D6A)
#define DREG_AR_IDLE_CNT1                       PPE_REG_ADDR_BOND(2, 0x0D6B)
#define DREG_AR_AIIDLE_CNT0                     PPE_REG_ADDR_BOND(2, 0x0D6C)
#define DREG_AR_AIIDLE_CNT1                     PPE_REG_ADDR_BOND(2, 0x0D6D)
#define DREG_AR_BE_CNT0                         PPE_REG_ADDR_BOND(2, 0x0D6E)
#define DREG_AR_BE_CNT1                         PPE_REG_ADDR_BOND(2, 0x0D6F)
#define DREG_AR_HEC_CNT0                        PPE_REG_ADDR_BOND(2, 0x0D70)
#define DREG_AR_HEC_CNT1                        PPE_REG_ADDR_BOND(2, 0x0D71)
#define DREG_AR_IDLE0                           PPE_REG_ADDR_BOND(2, 0x0D74)
#define DREG_AR_IDLE1                           PPE_REG_ADDR_BOND(2, 0x0D75)

#define DREG_AR_OVDROP_CNT0                     PPE_REG_ADDR_BOND(2, 0x0D98)
#define DREG_AR_OVDROP_CNT1                     PPE_REG_ADDR_BOND(2, 0x0D99)
#define DREG_AR_CERRN_CNT0                      PPE_REG_ADDR_BOND(2, 0x0DA0)
#define DREG_AR_CERRN_CNT1                      PPE_REG_ADDR_BOND(2, 0x0DA1)
#define DREG_AR_CERRNP_CNT0                     PPE_REG_ADDR_BOND(2, 0x0DA2)
#define DREG_AR_CERRNP_CNT1                     PPE_REG_ADDR_BOND(2, 0x0DA3)
#define DREG_AR_CVN_CNT0                        PPE_REG_ADDR_BOND(2, 0x0DA4)


#define DREG_AR_CVN_CNT0                        PPE_REG_ADDR_BOND(2, 0x0DA4)
#define DREG_AR_CVN_CNT1                        PPE_REG_ADDR_BOND(2, 0x0DA5)
#define DREG_AR_CVNP_CNT0                       PPE_REG_ADDR_BOND(2, 0x0DA6)
#define DREG_AR_CVNP_CNT1                       PPE_REG_ADDR_BOND(2, 0x0DA7)
#define DREG_B0_LADR                            PPE_REG_ADDR_BOND(2, 0x0DA8)
#define DREG_B1_LADR                            PPE_REG_ADDR_BOND(2, 0x0DA9)

#define GIF0_RX_CRC_ERR_CNT                     DREG_AR_CERRN_CNT0
#define GIF1_RX_CRC_ERR_CNT                     DREG_AR_CERRNP_CNT0
#define GIF2_RX_CRC_ERR_CNT                     DREG_AR_CERRN_CNT1
#define GIF3_RX_CRC_ERR_CNT                     DREG_AR_CERRNP_CNT1
#define GIF0_RX_CV_CNT                          DREG_AR_CVN_CNT0
#define GIF1_RX_CV_CNT                          DREG_AR_CVNP_CNT0
#define GIF2_RX_CV_CNT                          DREG_AR_CVN_CNT1
#define GIF3_RX_CV_CNT                          DREG_AR_CVNP_CNT1
#define DREG_B0_OVERDROP_CNT                    DREG_AR_OVDROP_CNT0
#define DREG_B1_OVERDROP_CNT                    DREG_AR_OVDROP_CNT1

#define NUM_OF_PORTS_INCLUDE_CPU_PORT 			7

#endif /* _SWI_VR9_REG_H  */
