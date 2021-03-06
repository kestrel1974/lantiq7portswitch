/*
 * swi_reg.h
 *
 *  Created on: 02.08.2012
 *      Author: tklaassen
 */

#ifndef SWI_REG_H_
#define SWI_REG_H_

/** ==========================  */
/* Include files                */
/** =========================== */
#include <ifx_regs.h>
#include <avmnet_debug.h>


#if defined(CONFIG_VR9)
#include "../vr9/swi_vr9_reg.h"
#elif defined(CONFIG_AR10)
#include "../ar10/swi_ar10_reg.h"
#endif

/* ============================= */
/* Local Macros & Definitions    */
/* ============================= */

#if defined(AVMNET_DEBUG_LEVEL) && (AVMNET_DEBUG_LEVEL & AVMNET_DEBUG_LEVEL_TRACE)
inline static void sw_write_reg32_debug(const char *func __attribute__((unused)), unsigned int address, unsigned int value) {
    AVMNET_DEBUG("[%s] sw_write_reg32 addr %#x = %#x\n", func, address, value);
    IFX_REG_W32(value, (volatile unsigned int *) (address));
}

inline static unsigned int sw_read_reg32_debug(const char *func __attribute__((unused)), unsigned int address) {
    unsigned int result = IFX_REG_R32((volatile unsigned int *) address);
    AVMNET_DEBUG("[%s] sw_read_reg32 addr %#x = %#x\n", func, address, result);
    return result;
}
#define SW_WRITE_REG32(data,addr)         sw_write_reg32_debug(__FUNCTION__, (unsigned int) (addr), (data))
#define SW_READ_REG32(addr)               sw_read_reg32_debug(__FUNCTION__, (unsigned int) (addr))
#else /*--- #if defined(AVMNET_DEBUG_LEVEL) && (AVMNET_DEBUG_LEVEL & AVMNET_DEBUG_LEVEL_DEBUG) ---*/
#define SW_WRITE_REG32(data,addr)         IFX_REG_W32((data), (volatile unsigned int *) (addr))
#define SW_READ_REG32(addr)               IFX_REG_R32((volatile unsigned int *) (addr))
#endif /*--- #else ---*/ /*--- #if defined(AVMNET_DEBUG_LEVEL) && (AVMNET_DEBUG_LEVEL & AVMNET_DEBUG_LEVEL_DEBUG) ---*/

#define SW_WRITE_REG32_MASK(_clr, _set, _r)   SW_WRITE_REG32((SW_READ_REG32((_r)) & ~(_clr)) | (_set), (_r))

/*
 *  PPE Reset Registers
 */
#ifdef CONFIG_IFX_PPE_E5_OFFCHIP_BONDING
  // ARC Cross PCI
  #define PCI_ADDR_BASE                         (g_ppe_base_addr == IFX_PPE_ORG(0) ? 0 : g_pci_dev_ppe_addr_off)
#else
  #define PCI_ADDR_BASE                         0
#endif

/*
 *  Reset Registers
 */
#define IFX_RCU_ORG                             0x1F203000
#define RCU_BASE_ADDR                           KSEG1ADDR(PCI_ADDR_BASE + IFX_RCU_ORG)
#define RCU_RST_REQ                             ((volatile u32*)(RCU_BASE_ADDR + 0x0010))
#define RCU_RST_STAT                            ((volatile u32*)(RCU_BASE_ADDR + 0x0014))
#define RCU_PPE_CONF                            ((volatile u32*)(RCU_BASE_ADDR + 0x002C))

/** Registers Description */

#define VR9_SWIP_BASE_ADDR                  (KSEG1 | 0x1E108000)
#define VR9_SWIP_TOP_BASE_ADDR              (VR9_SWIP_BASE_ADDR + (0x0C40 * 4))
#define VR9_SWIP_MACRO_REG(off)             ((volatile u32*)(VR9_SWIP_BASE_ADDR + (off) * 4))
#define VR9_SWIP_TOP                        (VR9_SWIP_BASE_ADDR | (0x0C40 * 4))
#define VR9_SWIP_TOP_REG(off)               ((volatile u32*)(VR9_SWIP_TOP + (off) * 4))
/** Switch Reset Control register */
#define ETHSW_SWRES_REG                     VR9_SWIP_MACRO_REG(0x00)
/** Register Configuration Resets all registers to their default state (such as after a hardware reset).
 * 0B RUN reset is off, 1B STOP reset is active */
#define SWRES_R0                            0x0001
/** Hardware Reset Reset all hardware modules except for the register settings.
 * 0B RUN reset is off, 1B STOP reset is active */
#define SWRES_R1                            0x0002

/** Ethernet Switch Clock Control Register */
#define ETHSW_CLK_REG                       VR9_SWIP_MACRO_REG(0x01)

#define ETHSW_BM_RAM_VAL_3_REG              VR9_SWIP_MACRO_REG(0x40)
#define ETHSW_BM_RAM_VAL_2_REG              VR9_SWIP_MACRO_REG(0x41)
#define ETHSW_BM_RAM_VAL_1_REG              VR9_SWIP_MACRO_REG(0x42)
#define ETHSW_BM_RAM_VAL_0_REG              VR9_SWIP_MACRO_REG(0x43)
#define ETHSW_BM_RAM_ADDR_REG               VR9_SWIP_MACRO_REG(0x44)
#define ETHSW_BM_RAM_CTRL_REG               VR9_SWIP_MACRO_REG(0x45)
#define BM_RAM_CTRL_BAS             ( 0x01   << 15 )
#define BM_RAM_CTRL_OPMOD_WR        ( 0x01   << 5 )
#define BM_RAM_CTRL_ADDR_MASK       ( 0x1f   << 0 )
#define BM_RAM_CTRL_ADDR_RMONP0     ( 0x00 )
#define BM_RAM_CTRL_ADDR_RMONP1     ( 0x01 )
#define BM_RAM_CTRL_ADDR_RMONP2     ( 0x02 )
#define BM_RAM_CTRL_ADDR_RMONP3     ( 0x03 )
#define BM_RAM_CTRL_ADDR_RMONP4     ( 0x04 )
#define BM_RAM_CTRL_ADDR_RMONP5     ( 0x05 )
#define BM_RAM_CTRL_ADDR_RMONP6     ( 0x06 )
#define BM_RAM_CTRL_ADDR_RMONP7     ( 0x07 )
#define BM_RAM_CTRL_ADDR_WFQ        ( 0x08 )
#define BM_RAM_CTRL_ADDR_PQMCTXT    ( 0x09 )
#define BM_RAM_CTRL_ADDR_PQMPP      ( 0x0a )
#define BM_RAM_CTRL_ADDR_SLLNP      ( 0x0b )
#define BM_RAM_CTRL_ADDR_SLLHD1     ( 0x0c )
#define BM_RAM_CTRL_ADDR_SLLHD2     ( 0x0d )
#define BM_RAM_CTRL_ADDR_QMAP       ( 0x0e )

/*Buffer manager per port registrs*/
//#define ETHSW_BM_PCFG_REG(port)       (VR9_SWIP_BASE_ADDR + (0x80 * 4) )
//#define ETHSW_BM_RMON_CTRL_REG  (VR9_SWIP_BASE_ADDR + (0x81 * 4))

#define ETHSW_BM_PCFG_REG(port)                 VR9_SWIP_MACRO_REG(0x80 + (port) * 2)   //  port < 7
#define ETHSW_BM_RMON_CTRL_REG(port)            VR9_SWIP_MACRO_REG(0x81 + (port) * 2)   //  port < 7

#define BM_PCFG_CNTEN(val)      (((val & 0x1) << 0 ) )

/** MAC Frame Length Register */
#define MAC_FLEN_REG                        VR9_SWIP_MACRO_REG(0x8C5)
#define MAC_FLEN(arg)                       ( (arg & 0x3FFF))
#define MAC_FLEN_MAX_SIZE                   (1590)

/** MAC Port Status Register */
#define MAC_0_PSTAT_REG                     (VR9_SWIP_BASE_ADDR + (0x900 * 4) )
#define MAC_1_PSTAT_REG                     (VR9_SWIP_BASE_ADDR + (0x90C * 4) )
#define MAC_2_PSTAT_REG                     (VR9_SWIP_BASE_ADDR + (0x918 * 4) )
#define MAC_3_PSTAT_REG                     (VR9_SWIP_BASE_ADDR + (0x924 * 4) )
#define MAC_4_PSTAT_REG                     (VR9_SWIP_BASE_ADDR + (0x930 * 4) )
#define MAC_5_PSTAT_REG                     (VR9_SWIP_BASE_ADDR + (0x93C * 4) )
#define MAC_6_PSTAT_REG                     (VR9_SWIP_BASE_ADDR + (0x948 * 4) )
#define MAC_PSTAT_REG(port)                 VR9_SWIP_MACRO_REG(0x900 + (port) * 0xC)//  port < 7

/** MAC Control Register 0 */
#define MAC_0_CTRL_0                        (VR9_SWIP_BASE_ADDR + (0x903 * 4) )
#define MAC_1_CTRL_0                        (VR9_SWIP_BASE_ADDR + (0x90F * 4) )
#define MAC_2_CTRL_0                        (VR9_SWIP_BASE_ADDR + (0x91B * 4) )
#define MAC_3_CTRL_0                        (VR9_SWIP_BASE_ADDR + (0x927 * 4) )
#define MAC_4_CTRL_0                        (VR9_SWIP_BASE_ADDR + (0x933 * 4) )
#define MAC_5_CTRL_0                        (VR9_SWIP_BASE_ADDR + (0x93F * 4) )
#define MAC_6_CTRL_0                        (VR9_SWIP_BASE_ADDR + (0x94B * 4) )
#define MAC_CTRL(port, reg)                 VR9_SWIP_MACRO_REG(0x903 + (port) * 0xC + (reg))    //  port < 7, reg < 7
#define MAC_CTRL_REG(port, reg)             MAC_CTRL((port), (reg))

#define MAC_CTRL_0_FCS_EN                   0x0080
#define MAC_CTRL_0_FCON_MASK                0x0070
#define MAC_CTRL_0_FCON_AUTO                0x0000
#define MAC_CTRL_0_FCON_RX                  0x0010
#define MAC_CTRL_0_FCON_TX                  0x0020
#define MAC_CTRL_0_FCON_RXTX                0x0030
#define MAC_CTRL_0_FCON_NONE                0x0040

#define MAC_CTRL_0_FDUP_MASK                0x000C
#define MAC_CTRL_0_FDUP_AUTO                0x0000
#define MAC_CTRL_0_FDUP_EN                  0x0004
#define MAC_CTRL_0_FDUP_DIS                 0x000C

#define MAC_CTRL_0_GMII_MASK                0x0003
#define MAC_CTRL_0_GMII_AUTO                0x0000
#define MAC_CTRL_0_GMII_MII                 0x0001
#define MAC_CTRL_0_GMII_RGMII               0x0002

//  Buffer manager per port registrs
#define ETHSW_BM_PCFG_REG(port)             VR9_SWIP_MACRO_REG(0x80 + (port) * 2)   //  port < 7
#define ETHSW_BM_RMON_CTRL_REG(port)        VR9_SWIP_MACRO_REG(0x81 + (port) * 2)   //  port < 7
#define PCE_PMAP_REG(reg)                   VR9_SWIP_MACRO_REG(0x453 + (reg) - 1)   //  1 <= reg <= 3
//  Parser & Classification Engine
#define PCE_TBL_KEY(n)                      VR9_SWIP_MACRO_REG(0x440 + 7 - (n))                 //  n < 7
#define PCE_TBL_MASK                        VR9_SWIP_MACRO_REG(0x448)
#define PCE_TBL_VAL(n)                      VR9_SWIP_MACRO_REG(0x449 + 5 - (n))                 //  n < 5;
#define PCE_TBL_ADDR                        VR9_SWIP_MACRO_REG(0x44E)
#define PCE_TBL_CTRL                        VR9_SWIP_MACRO_REG(0x44F)
#define PCE_TBL_STAT                        VR9_SWIP_MACRO_REG(0x450)
#define PCE_GCTRL_REG(reg)                  VR9_SWIP_MACRO_REG(0x456 + (reg))
#define PCE_PCTRL_REG(port, reg)            VR9_SWIP_MACRO_REG(0x480 + (port) * 0xA + (reg))    //  port < 12, reg < 4
#define PCE_TCM_STAT(port)                  VR9_SWIP_MACRO_REG(0x581 + (port) * 7)

//  MAC Port Status Register
//  Ethernet Switch Fetch DMA Port Control, Controls per-port functions of the Fetch DMA
#define FDMA_PCTRL_REG(port)                VR9_SWIP_MACRO_REG(0xA80 + (port) * 6)  //  port < 7
//  Ethernet Switch Store DMA Port Control, Controls per-ingress-port functions of the Store DMA
#define SDMA_PCTRL_REG(port)                VR9_SWIP_MACRO_REG(0xBC0 + (port) * 6)  //  port < 7

/** Ethernet Switch Fetch DMA Port Control
 Controls per-port functions of the Fetch DMA */
#define FDMA_PCTRL_PORT6                    (VR9_SWIP_BASE_ADDR + (0xAA4 * 4))
/** Special Tag Insertion Enable(to egress frames )*/
#define FDMA_PCTRL_STEN                     (1 << 1)

/** Ethernet Switch Store DMA Port Control
 Controls per-port functions of the Store DMA */
#define SDMA_PCTRL_PORT(port)               (VR9_SWIP_BASE_ADDR + 4 * (0xBC0 + 6 * (port)))
#define SDMA_PCTRL_PORT_ENABLE              (1 << 0)

/** Global Software Reset Reset all hardware modules excluding the register settings.
 * 0B OFF reset is off, 1B ON reset is active */
#define GLOB_CTRL_SWRES                     0x0001
/** Global Hardware Reset Reset all hardware modules including the register settings.
 * 0B OFF reset is off, 1B ON reset is active */
#define GLOB_CTRL_HWRES                     0x0002
/** Global Switch Macro Enable If set to OFF, the switch macro is inactive and frame forwarding is disabled.
 * 0B OFF switch macro is not active, 1B ON switch macro is active */
#define GLOB_CTRL_SE                        0x8000

/** MDIO Control Register */
#define MDIO_CTRL_REG                       VR9_SWIP_TOP_REG(0x08)
/** MDIO Busy*/
#define MDIO_CTRL_MBUSY                     0x1000
#define MDIO_CTRL_OP_MASK                   0x0C00
#define MDIO_CTRL_OP_WR                     0x0400
#define MDIO_CTRL_OP_RD                     0x0800
#define MDIO_CTRL_PHYAD_SET(arg)            ((arg & 0x1F) << 5)
#define MDIO_CTRL_PHYAD_GET(arg)            ( (arg >> 5 ) & 0x1F)
#define MDIO_CTRL_REGAD(arg)                ( arg & 0x1F)

/** MDIO Read Data Register */
#define MDIO_READ_REG                       VR9_SWIP_TOP_REG(0x09)
#define MDIO_READ_RDATA(arg)                (arg & 0xFFFF)

/** MDIO Write Data Register */
#define MDIO_WRITE_REG                      VR9_SWIP_TOP_REG(0x0A)
#define MDIO_READ_WDATA(arg)                (arg & 0xFFFF)

/** MDC Clock Configuration Register 0 */
#define MDC_CFG_0_REG                       VR9_SWIP_TOP_REG(0x0B)
#define MDC_CFG_0_PEN_SET(port)             (0x1 << port)
#define MDC_CFG_0_PEN_GET(port, reg_data)   ((reg_data >> port ) & 0x1 )
/** MDC Clock Configuration Register 1 */
#define MDC_CFG_1_REG                       VR9_SWIP_TOP_REG(0x0C)
#  define MDC_CFG_1_RES                     (1 << 15)
#  define MDC_CFG_1_MCEN                    (1 << 8)
#  define MDC_CFG_1_25MHZ                   0x00
#  define MDC_CFG_1_12_5MHZ                 0x01
#  define MDC_CFG_1_6_25MHZ                 0x02
#  define MDC_CFG_1_4_167MHZ                0x03
#  define MDC_CFG_1_3_125MHZ                0x04
#  define MDC_CFG_1_2_5MHZ                  0x05
#  define MDC_CFG_1_2_083MHZ                0x06
/* ... */

//  Global Control Register 0
#define GLOB_CTRL_REG                       VR9_SWIP_TOP_REG(0x00)

/** PHY Address Register PORT 5~0 */
#define PHY_ADDR_5                          (VR9_SWIP_TOP_BASE_ADDR + (0x10 * 4))
#define PHY_ADDR_4                          (VR9_SWIP_TOP_BASE_ADDR + (0x11 * 4))
#define PHY_ADDR_3                          (VR9_SWIP_TOP_BASE_ADDR + (0x12 * 4))
#define PHY_ADDR_2                          (VR9_SWIP_TOP_BASE_ADDR + (0x13 * 4))
#define PHY_ADDR_1                          (VR9_SWIP_TOP_BASE_ADDR + (0x14 * 4))
#define PHY_ADDR_0                          (VR9_SWIP_TOP_BASE_ADDR + (0x15 * 4))
#define PHY_ADDR(port)                      VR9_SWIP_TOP_REG(0x15 - (port))     //  port < 6
#define PHY_ADDR_REG(port)                  PHY_ADDR(port)

/** Link Status Control */
#define PHY_ADDR_LINKST_SHIFT               13
#define PHY_ADDR_LINKST_SIZE                2
#define PHY_ADDR_LINKST_MASK                0x6000
#define PHY_ADDR_LINKST_AUTO                0x0000
#define PHY_ADDR_LINKST_UP                  0x2000
#define PHY_ADDR_LINKST_DOWN                0x4000
/** Speed Control */
#define PHY_ADDR_SPEED_MASK                 0x1800
#define PHY_ADDR_SPEED_10                   0x0000
#define PHY_ADDR_SPEED_100                  0x0800
#define PHY_ADDR_SPEED_1000                 0x1000
#define PHY_ADDR_SPEED_AUTO                 0x1800
/** Full Duplex Control */
#define PHY_ADDR_FDUP_MASK                  0x0600
#define PHY_ADDR_FDUP_AUTO                  0x0000
#define PHY_ADDR_FDUP_EN                    0x0200
#define PHY_ADDR_FDUP_DIS                   0x0600
/** Flow Control Mode TX */
#define PHY_ADDR_FCONTX_MASK                0x0180
#define PHY_ADDR_FCONTX_AUTO                0x0000
#define PHY_ADDR_FCONTX_EN                  0x0080
#define PHY_ADDR_FCONTX_DIS                 0x0180
/** Flow Control Mode RX */
#define PHY_ADDR_FCONRX_MASK                0x0060
#define PHY_ADDR_FCONRX_AUTO                0x0000
#define PHY_ADDR_FCONRX_EN                  0x0020
#define PHY_ADDR_FCONRX_DIS                 0x0060
/** PHY Address */
#define PHY_ADDR_ADDR_MASK                  0x001F
#define PHY_ADDR_ADDR(arg)                  (arg & 0x001F)

/** PHY MDIO Polling Status per PORT */
#define MDIO_STAT_0_REG                     (VR9_SWIP_TOP_BASE_ADDR + (0x16 * 4))
#define MDIO_STAT_1_REG                     (VR9_SWIP_TOP_BASE_ADDR + (0x17 * 4))
#define MDIO_STAT_2_REG                     (VR9_SWIP_TOP_BASE_ADDR + (0x18 * 4))
#define MDIO_STAT_3_REG                     (VR9_SWIP_TOP_BASE_ADDR + (0x19 * 4))
#define MDIO_STAT_4_REG                     (VR9_SWIP_TOP_BASE_ADDR + (0x1A * 4))
#define MDIO_STAT_5_REG                     (VR9_SWIP_TOP_BASE_ADDR + (0x1B * 4))
#define MDIO_STAT_REG(port)                 VR9_SWIP_TOP_REG(0x16 + (port))     //  port < 6

/** PHY Active Status */
#define MDIO_STAT_PACT                      0x0040
#define MDIO_STAT_LSTAT                     0x0020
#define MDIO_STAT_SPEED(arg)                ( (arg >> 0x3) & 0x03)
#define MDIO_STAT_FDUP                      0x0004
#define MDIO_STAT_RXPAUEN                   0x0002
#define MDIO_STAT_TXPAUEN                   0x0001

/** xMII Control Registers */
/** xMII Port 0 Configuration register */
#define MII_CFG_0_REG                       (VR9_SWIP_TOP_BASE_ADDR + (0x36 * 4))
#define MII_CFG_1_REG                       (VR9_SWIP_TOP_BASE_ADDR + (0x38 * 4))
#define MII_CFG_2_REG                       (VR9_SWIP_TOP_BASE_ADDR + (0x3A * 4))
#define MII_CFG_3_REG                       (VR9_SWIP_TOP_BASE_ADDR + (0x3C * 4))
#define MII_CFG_4_REG                       (VR9_SWIP_TOP_BASE_ADDR + (0x3E * 4))
#define MII_CFG_5_REG                       (VR9_SWIP_TOP_BASE_ADDR + (0x40 * 4))
#define MII_CFG_REG(port)                   VR9_SWIP_TOP_REG(0x36 + (port) * 2) //  port < 6

#define MII_CFG_RES                         0x8000
#define MII_CFG_EN                          0x4000
/** Bits are only valid in PHY Mode */
#define MII_CFG_CRS_SET(arg)                ( (arg & 0x03) << 0x9)
#define MII_CFG_CRS_GET(arg)                ( (arg >> 0x9) & 0x03)
/** RGMII In Band Status */
#define MII_CFG_RGMII_IBS                   0x0100
/** RMII Reference Clock Direction of the Port */
#define MII_CFG_RMII_OUT                    0x0080
/** xMII Port Interface Clock Rate */
#define MII_CFG_MIIRATE_MASK                0x0070
#define MII_CFG_MIIRATE_2_5MHZ              0x0000
#define MII_CFG_MIIRATE_25MHZ               0x0010
#define MII_CFG_MIIRATE_125MHZ              0x0020
#define MII_CFG_MIIRATE_50MHZ               0x0030
#define MII_CFG_MIIRATE_AUTO                0x0040

#define MII_CFG_ISOLATE                     0x2000
#define MII_CFG_LNK_DWN_CLK_DIS             0x1000

/** xMII Interface Mode */
#define MII_CFG_MIIMODE_MASK                0x000F
#define MII_CFG_MIIMODE_MIIP                0x0000
#define MII_CFG_MIIMODE_MIIM                0x0001
#define MII_CFG_MIIMODE_RMIIP               0x0002
#define MII_CFG_MIIMODE_RMIIM               0x0003
#define MII_CFG_MIIMODE_RGMII               0x0004

/** Configuration of Clock Delay for Port 0 (used for RGMII mode only)*/
#define MII_PCDU_0_REG                      (VR9_SWIP_TOP_BASE_ADDR + (0x37 * 4))
#define MII_PCDU_1_REG                      (VR9_SWIP_TOP_BASE_ADDR + (0x39 * 4))
#define MII_PCDU_5_REG                      (VR9_SWIP_TOP_BASE_ADDR + (0x41 * 4))
#define MII_PCDU_REG(port)                  VR9_SWIP_TOP_REG(0x37 + (port) * 2) //  port < 6

#define MII_PCDU_RXLOCK                     0x8000
#define MII_PCDU_TXLOCK                     0x4000
#define MII_PCDU_RXSEL_CLK_MASK             0x3000
#define MII_PCDU_RXSEL_CLK_AUTO             0x0000
#define MII_PCDU_RXSEL_CLK_RXCLK            0x1000
#define MII_PCDU_RXSEL_CLK_CLKREF           0x2000
#define MII_PCDU_RXINIT                     0x0800
#define MII_PCDU_RXPD                       0x0400
#define MII_PCDU_RXDLY_MASK                 0x0380

#define MII_PCDU_TXSEL_CLK_MASK             0x0060
#define MII_PCDU_TXSEL_CLK_AUTO             0x0000
#define MII_PCDU_TXSEL_CLK_TXCLK            0x0020
#define MII_PCDU_TXSEL_CLK_CLKREF           0x0040
#define MII_PCDU_TXINIT                     0x0010
#define MII_PCDU_TXPD                       0x0008
#define MII_PCDU_TXDLY_MASK                 0x0007

/** PMAC Header Control Register */
#define PMAC_HD_CTL_REG                     VR9_SWIP_TOP_REG(0x82)
#define PMAC_HD_CTL_FC                      0x0400
#define PMAC_HD_CTL_CCRC                    0x0200
#define PMAC_HD_CTL_RST                     0x0100
#define PMAC_HD_CTL_AST                     0x0080
#define PMAC_HD_CTL_RXSH                    0x0040
#define PMAC_HD_CTL_RL2                     0x0020
#define PMAC_HD_CTL_RC                      0x0010
#define PMAC_HD_CTL_AS                      0x0008
#define PMAC_HD_CTL_AC                      0x0004
#define PMAC_HD_CTL_TAG                     0x0002
#define PMAC_HD_CTL_ADD                     0x0001

/** PMAC Type/Length register */
#define PMAC_TL_REG                         VR9_SWIP_TOP_REG(0x83)
/** PMAC Source Address Register */
#define PMAC_SA1_REG                        VR9_SWIP_TOP_REG(0x84)
#define PMAC_SA2_REG                        VR9_SWIP_TOP_REG(0x85)
#define PMAC_SA3_REG                        VR9_SWIP_TOP_REG(0x86)
/** PMAC Destination Address Register */
#define PMAC_DA1_REG                        VR9_SWIP_TOP_REG(0x87)
#define PMAC_DA2_REG                        VR9_SWIP_TOP_REG(0x88)
#define PMAC_DA3_REG                        VR9_SWIP_TOP_REG(0x89)
/** PMAC VLAN register */
#define PMAC_VLAN_REG                       VR9_SWIP_TOP_REG(0x8A)
/** PMAC Inter Packet Gap in RX Direction */
#define PMAC_RX_IPG_REG                     VR9_SWIP_TOP_REG(0x8B)
/** PMAC Special Tag Ethertype */
#define PMAC_ST_ETYPE_REG                   VR9_SWIP_TOP_REG(0x8C)
/** PMAC Ethernet WAN Group */
#define PMAC_EWAN_REG                       VR9_SWIP_TOP_REG(0x8D)

#define VR9_RCU_BASE_ADDR                   (0xBF203000)

/* Reset Request Register                        */
#define RST_REQ                             (VR9_RCU_BASE_ADDR + 0x10)
/* GPHY0 Firmware Base Address Register          */
#define GFS_ADD0                            (VR9_RCU_BASE_ADDR + 0x20)
/* GPHY1 Firmware Base Address Register          */
#define GFS_ADD1                            (VR9_RCU_BASE_ADDR + 0x68)
/* GPHY0/1 MDIO address register                 */
#define GFMDIO_ADD                          (VR9_RCU_BASE_ADDR + 0x44)


#endif /* SWI_REG_H_ */
