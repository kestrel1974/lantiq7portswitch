/**
** FILE NAME    : ifxmips_3port_sw_reg.h
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

#ifndef _IFXMIPS_3PORT_REG_H_
#define _IFXMIPS_3PORT_REG_H_ 
/*!
  \file ifxmips_3port_sw_reg.h
  \ingroup IFX_ETH_DRV
  \brief IFX Eth module register definition for ARx platforms
*/

#include <ifx_regs.h>

/************************************************************************/
/*   Module       :  AR9  3-port Ethernet Switch register address and bits*/
/************************************************************************/
#define IFX_AR9_ETH_REG_BASE        (0xBE108000)
/*Port status */
#define IFX_AR9_ETH_PS			(volatile unsigned int*)(IFX_AR9_ETH_REG_BASE + 0x000)
#define PS_P0LS                 0x00000001	/*Port 0 link status */ 
#define PS_P0SS                 0x00000002	/*Port 0 speed status (0=10Mbps, 1=100Mbps*/ 
#define PS_P0SHS                0x00000004	/*Port 0 speed high status (1=1000Mbps)*/ 
#define PS_P0DS                 0x00000008	/*Port 0 duplex status (0= Half Duplex, 1= Full Duplex)*/ 
#define PS_P0FCS                0x00000010	/*Port 0 flow control(0= disable, 1= enable)*/ 
#define PS_P1LS                 0x00000100	/*Port 1 link status */ 
#define PS_P1SS                 0x00000200	/*Port 1 speed status (0=10Mbps, 1=100Mbps*/ 
#define PS_P1SHS                0x00000400	/*Port 1 speed high status (1=1000Mbps)*/ 
#define PS_P1DS                 0x00000800	/*Port 1 duplex status (0= Half Duplex, 1= Full Duplex)*/ 
#define PS_P1FCS                0x00001000	/*Port 1 flow control(0= disable, 1= enable)*/ 

#define PS_LS(a)                (0x01 << (8 * (a)))
#define PS_SS(a)                (0x02 << (8 * (a)))
#define PS_SHS(a)               (0x04 << (8 * (a)))
#define PS_DS(a)                (0x08 << (8 * (a)))
#define PS_FCS(a)               (0x10 << (8 * (a)))

#define IFX_AR9_ETH_P0_CTL      (volatile unsigned int*)(IFX_AR9_ETH_REG_BASE + 0x004)
#define IFX_AR9_ETH_P1_CTL      (volatile unsigned int*)(IFX_AR9_ETH_REG_BASE + 0x008)
#define IFX_AR9_ETH_P2_CTL      (volatile unsigned int*)(IFX_AR9_ETH_REG_BASE + 0x00C)
#define IFX_AR9_ETH_CTL(a)      (volatile unsigned int*)(IFX_AR9_ETH_REG_BASE + 0x004 + ((a) * 0x4))
#define PX_CTL_BYPASS           0x00000001	/*Bypass mode (0=rule of VLAN tag member, 1=same format as receive ) */
#define PX_CTL_DSV8021X         0x00000002	/*Drop scheme for violation 802.1x (0=drop the packet, 1=don't care) */ 
#define PX_CTL_REDIR            0x00002000	/*Port redirect (0=disable, 1= enable)) */
#define PX_CTL_LD               0x00004000	/*Learning disable (1=disable, 0= enable)) */
#define PX_CTL_AD               0x00008000	/*Aging disable (1=disable, 0= enable)) */
#define PX_CTL_FLD              0x00020000	/*force mac link down (0=disable, 1= enable)) */
#define PX_CTL_FLP              0x00040000	/*force mac link up (0=disable, 1= enable)) */
#define PX_CTL_DFWD             0x00080000	/* Port ingress direct forwarding (0=disable, 1=enable) */
#define PX_CTL_DMDIO            0x00400000	/* Disable MDIO auto polling (0=disable, 1=enable) */
#define PX_CTL_TPE              0x01000000	/* Ether type Priority enable(0=disable, 1=enable) */
#define PX_CTL_IPVLAN           0x02000000	/* IP over VLAN Priority enable(0=disable, 1=enable) */
#define PX_CTL_SPE              0x04000000	/* Service Priority enable(0=disable, 1=enable) */
#define PX_CTL_VPE              0x08000000	/* VLAN Priority enable(0=disable, 1=enable) */
#define PX_CTL_IPOVTU           0x10000000	/* IP over TCP/UDP Priority enable(0=disable, 1=enable) */
#define PX_CTL_TCPE             0x20000000	/* TCP/UDP Priority enable(0=disable, 1=enable) */

#define IFX_AR9_ETH_P0_VLAN     (volatile unsigned int*) (IFX_AR9_ETH_REG_BASE + 0x010)
#if defined(BIG_ENDIAN) || defined(__BIG_ENDIAN)
typedef union {
    struct {
        unsigned int DFID     :  2;
        unsigned int TBVE     :  1;
        unsigned int IFNTE    :  1;
        unsigned int VC       :  1;
        unsigned int VSD      :  1;
        unsigned int AOVTP    :  1;
        unsigned int VMCE     :  1;
        unsigned int DVPM     :  8;
        unsigned int PP       :  2;
        unsigned int PPE      :  1;
        unsigned int PVTAGMP  :  1;
        unsigned int PVID     : 12;
    } Bits;
    unsigned int Register;
} ifx_ar9_eth_px_vlan_t;
#endif /*--- #if defined(BIG_ENDIAN) || defined(__BIG_ENDIAN) ---*/
#define IFX_AR9_ETH_PORT_VLAN(a)     (volatile unsigned int*) (IFX_AR9_ETH_REG_BASE + 0x010 + ((a) * 0x4))
#define PORT_VLAN_DFID_MASK     (0x003          << 30)
#define PORT_VLAN_DFID(a)       (((a) & 0x3)    << 30)
#define PORT_VLAN_TBVE          (0x001          << 29)
#define PORT_VLAN_IFNTE         (0x001          << 28)
#define PORT_VLAN_VC            (0x001          << 27)
#define PORT_VLAN_VSD           (0x001          << 26)
#define PORT_VLAN_AOVTP         (0x001          << 25)
#define PORT_VLAN_VMCE          (0x001          << 24)
#define PORT_VLAN_DVPM_MASK     (0x0ff          << 16)
#define PORT_VLAN_DVPM(a)       (((a) & 0xff)   << 16)
#define PORT_VLAN_PP_MASK       (0x003          << 14)
#define PORT_VLAN_PP(a)         (((a) & 0x3)    << 14)
#define PORT_VLAN_PPE           (0x1            << 13)
#define PORT_VLAN_PVTAGMP       (0x1            << 12)
#define PORT_VLAN_PVID_MASK     (0xfff          << 0 )
#define PORT_VLAN_PVID(a)       (((a) & 0xfff)  << 0 )

#define IFX_AR9_ETH_GCTL0       (volatile unsigned int*) (IFX_AR9_ETH_REG_BASE + 0x068) 
#define GCTL0_MPL_MASK          0x00000300   
#define GCTL0_MPL_1522          0x00000000   /*Max packet length 1522 Bytes */
#define GCTL0_MPL_1518          0x00000100   /*Max packet length 1518 Bytes */
#define GCTL0_MPL_1536          0x00000200   /*Max packet length 1536 Bytes */
#define GCTL0_LPE               0x20000000   /*Virtual ports over cpu physical port enable (0=disable, 1=enable) */
#define GCTL0_ICRCCD            0x40000000   /*Crc check enabled (0=disable, 1=enable) */
#define GCTL0_SE                0x80000000   /*Switch Enable (0=disable, 1=enable) */
#define GCTL0_MCA               0x00020000   // Mirror CRC
#define GCTL0_MRA               0x00010000   // Mirror RXER
#define GCTL0_MPA               0x00020000   // Mirror PAUSE
#define GCTL0_MLA               0x00020000   // Mirror Long
#define GCTL0_MSA               0x00020000   // Mirror Short
#define GCTL0_SNIFFPN_MASK      (0x7 << 10)  // sniffer port number
#define GCTL0_ATS_MASK          (0x7 << 18)
#define GCTL0_ATS_1S            (0x0)
#define GCTL0_ATS_10S           (0x1 << 18)
#define GCTL0_ATS_300S          (0x2 << 18)
#define GCTL0_ATS_1H            (0x3 << 18)
#define GCTL0_ATS_1D            (0x4 << 18)

#define IFX_AR9_ETH_GCTL1       (volatile unsigned int*) (IFX_AR9_ETH_REG_BASE + 0x06C)
#define GCTL1_BISTDN            (0x1 << 27)
#define GCTL1_EDSTX             (0x1 << 26)
#define GCTL1_CTTX_MASK         (0x3 << 24)
#define GCTL1_IJT_MASK          (0x7 << 21)
#define GCTL1_DIVS              (0x1 << 20)
#define GCTL1_DII6P             (0x1 << 19)
#define GCTL1_DIIPS             (0x1 << 18)
#define GCTL1_DIE               (0x1 << 17)
#define GCTL1_DIIP              (0x1 << 16)
#define GCTL1_DIS               (0x1 << 15)

#define IFX_AR9_ETH_P0_INCTL_REG      (volatile unsigned int*) (IFX_AR9_ETH_REG_BASE + 0x20)
#define IFX_AR9_ETH_P1_INCTL_REG      (volatile unsigned int*) (IFX_AR9_ETH_REG_BASE + 0x24)
#define IFX_AR9_ETH_P2_INCTL_REG      (volatile unsigned int*) (IFX_AR9_ETH_REG_BASE + 0x28)

/** Default Portmap register */
#define IFX_AR9_ETH_DF_PORTMAP_REG      (volatile unsigned int*) (IFX_AR9_ETH_REG_BASE + 0x02C)
/** Unicast Portmap */
#define DF_PORTMAP_UP_MASK              (0xff << 24)
#define DF_PORTMAP_UP(port)             (1 << (24 + (port)))
/** Broadcast Portmap */
#define DF_PORTMAP_BP_MASK              (0xff << 16)
#define DF_PORTMAP_BP(port)             (1 << (16 + (port)))
/** Multicast Portmap */
#define DF_PORTMAP_MP_MASK              (0xff << 24)
#define DF_PORTMAP_MP(port)             (1 << (8 + (port)))
/** Reserve Portmap */
#define DF_PORTMAP_RP_MASK              (0xff << 24)
#define DF_PORTMAP_RP(port)             (1 << (0 + (port)))

/** Port Egress Control for Strict Q32/Q10 */
#define IFX_AR9_ETH_P0_ECS_Q32_REG      (volatile unsigned int*) (IFX_AR9_ETH_REG_BASE + 0x30)
#define IFX_AR9_ETH_P0_ECS_Q10_REG      (volatile unsigned int*) (IFX_AR9_ETH_REG_BASE + 0x34)
#define IFX_AR9_ETH_P0_ECW_Q32_REG      (volatile unsigned int*) (IFX_AR9_ETH_REG_BASE + 0x38)
#define IFX_AR9_ETH_P0_ECW_Q10_REG      (volatile unsigned int*) (IFX_AR9_ETH_REG_BASE + 0x3C)

#define IFX_AR9_ETH_P1_ECS_Q32_REG      (volatile unsigned int*) (IFX_AR9_ETH_REG_BASE + 0x40)
#define IFX_AR9_ETH_P1_ECS_Q10_REG      (volatile unsigned int*) (IFX_AR9_ETH_REG_BASE + 0x44)
#define IFX_AR9_ETH_P1_ECW_Q32_REG      (volatile unsigned int*) (IFX_AR9_ETH_REG_BASE + 0x48)
#define IFX_AR9_ETH_P1_ECW_Q10_REG      (volatile unsigned int*) (IFX_AR9_ETH_REG_BASE + 0x4C)

#define IFX_AR9_ETH_P2_ECS_Q32_REG      (volatile unsigned int*) (IFX_AR9_ETH_REG_BASE + 0x50)
#define IFX_AR9_ETH_P2_ECS_Q10_REG      (volatile unsigned int*) (IFX_AR9_ETH_REG_BASE + 0x54)
#define IFX_AR9_ETH_P2_ECW_Q32_REG      (volatile unsigned int*) (IFX_AR9_ETH_REG_BASE + 0x58)
#define IFX_AR9_ETH_P2_ECW_Q10_REG      (volatile unsigned int*) (IFX_AR9_ETH_REG_BASE + 0x5C)


#define IFX_AR9_ETH_RGMII_CTL           (volatile unsigned int*) (IFX_AR9_ETH_REG_BASE + 0x078)
#define RGMII_CTL_FCE_SET(port, arg)    ( (arg & 0x1) << (port * 10))
#define RGMII_CTL_FCE_GET(port, arg)    ( (arg  >> (port * 10) ) & 0x1)

#define RGMII_CTL_DUP_MASK(port)        ( 0x1 << ((port * 10)+1))
#define RGMII_CTL_DUP_SET(port, arg)    ( (arg & 0x1) << ((port * 10)+1))
#define RGMII_CTL_DUP_GET(port, arg)    ( (arg  >> ((port * 10)+1) ) & 0x1)

#define RGMII_CTL_SPD_MASK(port)        ( (0xC) << ((port * 10)))
#define RGMII_CTL_SPD_100_SET(port)     ( (0x1) << ((port * 10)+2))
#define RGMII_CTL_SPD_1000_SET(port)    ( (0x2) << ((port * 10)+2))

#define RGMII_CTL_P0FCE         0x00000001	/*Port 0 flow control enable (0=disable, 1=enable) */
#define RGMII_CTL_P0DUP         0x00000002	/*Port 0 duplex mode (0=half duplex, 1=full duplex) */
#define RGMII_CTL_P0SPD_MASK    0x0000000C
#define RGMII_CTL_P0SPD_10      0x00000000	/*Port 0 speed 10Mbps */
#define RGMII_CTL_P0SPD_100     0x00000004	/*Port 0 speed 100Mbps */
#define RGMII_CTL_P0SPD_1000    0x00000008	/*Port 0 speed 1000Mbps */
#define RGMII_CTL_P0TDLY_MASK   0x00000030	/*Port 0 RGMII Tx clock delay */
#define RGMII_CTL_P0TDLY_0      0x00000000	/*Port 0 RGMII Tx clock delay 0ns */
#define RGMII_CTL_P0TDLY_1_5    0x00000010	/*Port 0 RGMII Tx clock delay 1.5ns */
#define RGMII_CTL_P0TDLY_1_75   0x00000020	/*Port 0 RGMII Tx clock delay 1.75ns */
#define RGMII_CTL_P0TDLY_2      0x00000030	/*Port 0 RGMII Tx clock delay 2ns */
#define RGMII_CTL_P0RDLY_MASK   0x000000C0	/*Port 0 RGMII Rx clock delay */
#define RGMII_CTL_P0RDLY_0      0x00000000	/*Port 0 RGMII Rx clock delay 0ns */
#define RGMII_CTL_P0RDLY_1_5    0x00000040	/*Port 0 RGMII Rx clock delay 1.5ns */
#define RGMII_CTL_P0RDLY_1_75   0x00000080	/*Port 0 RGMII Rx clock delay 1.75ns */
#define RGMII_CTL_P0RDLY_2      0x000000C0	/*Port 0 RGMII Rx clock delay 2ns */
#define RGMII_CTL_P0IS_MASK     0x00000300	/*Port 0 interface selection*/
#define RGMII_CTL_P0IS_RGMII    0x00000000	/*Port 0 interface selection  RGMII mode*/
#define RGMII_CTL_P0IS_MII      0x00000100	/*Port 0 interface selection  MII mode*/
#define RGMII_CTL_P0IS_REVMII   0x00000200	/*Port 0 interface selection  Rev MII mode*/
#define RGMII_CTL_P0IS_REDMII   0x00000300	/*Port 0 interface selection  Reduced MII mode*/

#define RGMII_CTL_P1FCE         0x00000400	/*Port 1 flow control enable (0=disable, 1=enable) */
#define RGMII_CTL_P1DUP         0x00000800	/*Port 1 duplex mode (0=half duplex, 1=full duplex) */
#define RGMII_CTL_P1SPD_MASK    0x00003000
#define RGMII_CTL_P1SPD_10      0x00000000	/*Port 1 speed 10Mbps */
#define RGMII_CTL_P1SPD_100     0x00001000	/*Port 1 speed 100Mbps */
#define RGMII_CTL_P1SPD_1000    0x00002000	/*Port 1 speed 1000Mbps */
#define RGMII_CTL_P1TDLY_MASK   0x0000C000	/*Port 1 RGMII Tx clock delay */
#define RGMII_CTL_P1TDLY_0      0x00000000	/*Port 1 RGMII Tx clock delay 0ns */
#define RGMII_CTL_P1TDLY_1_5    0x00004000	/*Port 1 RGMII Tx clock delay 1.5ns */
#define RGMII_CTL_P1TDLY_1_75   0x00008000	/*Port 1 RGMII Tx clock delay 1.75ns */
#define RGMII_CTL_P1TDLY_2      0x0000C000	/*Port 1 RGMII Tx clock delay 2ns */
#define RGMII_CTL_P1RDLY_MASK   0x00030000	/*Port 1 RGMII Rx clock delay */
#define RGMII_CTL_P1RDLY_0      0x00000000	/*Port 1 RGMII Rx clock delay 0ns */
#define RGMII_CTL_P1RDLY_1_5    0x00010000	/*Port 1 RGMII Rx clock delay 1.5ns */
#define RGMII_CTL_P1RDLY_1_75   0x00020000	/*Port 1 RGMII Rx clock delay 1.75ns */
#define RGMII_CTL_P1RDLY_2      0x00030000	/*Port 1 RGMII Rx clock delay 2ns */
#define RGMII_CTL_P1IS_MASK     0x000C0000	/*Port 1 interface selection*/
#define RGMII_CTL_P1IS_RGMII    0x00000000	/*Port 1 interface selection  RGMII mode*/
#define RGMII_CTL_P1IS_MII      0x00040000	/*Port 1 interface selection  MII mode*/
#define RGMII_CTL_P1IS_REVMII   0x00080000	/*Port 1 interface selection  Rev MII mode*/
#define RGMII_CTL_P1IS_REDMII   0x000C0000	/*Port 1 interface selection  Reduced MII mode*/

#define RGMII_CTL_P0FRQ         0x00100000	/*Port 0 interface Rev MII clock frquency ( 0=25MHz, 1=50MHz)*/
#define RGMII_CTL_P0CKIO        0x00200000	/*Port 0 interface Clock PAD I/O select ( 0=Input , 1=Output)*/
#define RGMII_CTL_P1FRQ         0x00400000	/*Port 1 interface Rev MII clock frquency ( 0=25MHz, 1=50MHz)*/
#define RGMII_CTL_P1CKIO        0x00800000	/*Port 1 interface Clock PAD I/O select ( 0=Input , 1=Output)*/
#define RGMII_CTL_MCS_MASK      0xFF000000	/* MDC clcok select, 25MHz/((MCS+1)*2)*/

#define RGMII_CTL_FCE(port)             (0x001 << ((port) * 10))
#define RGMII_CTL_DUP(port)             (0x002 << ((port) * 10))
#define RGMII_CTL_SPD_100(port)         (0x004 << ((port) * 10))
#define RGMII_CTL_SPD_1000(port)        (0x008 << ((port) * 10))
#define RGMII_CTL_TDLY_MASK(port)       (0x030 << ((port) * 10))
#define RGMII_CTL_TDLY_1_5(port)        (0x010 << ((port) * 10))
#define RGMII_CTL_TDLY_1_75(port)       (0x020 << ((port) * 10))
#define RGMII_CTL_TDLY_2_0(port)        (0x030 << ((port) * 10))
#define RGMII_CTL_RDLY_MASK(port)       (0x0c0 << ((port) * 10))
#define RGMII_CTL_RDLY_1_5(port)        (0x040 << ((port) * 10))
#define RGMII_CTL_RDLY_1_75(port)       (0x080 << ((port) * 10))
#define RGMII_CTL_RDLY_2_0(port)        (0x0c0 << ((port) * 10))
#define RGMII_CTL_IS_MASK(port)         (0x300 << ((port) * 10))
#define RGMII_CTL_IS_RGMII(port)        (0x0) // default
#define RGMII_CTL_IS_MII(port)          (0x100 << ((port) * 10))
#define RGMII_CTL_IS_REVMII(port)       (0x200 << ((port) * 10))
#define RGMII_CTL_IS_RMII(port)         (0x300 << ((port) * 10))
#define RGMII_CTL_FEQ(port)             (0x100000 << ((port) * 2))
#define RGMII_CTL_CKIO(port)            (0x200000 << ((port) * 2))

#define IFX_AR9_ETH_1P_PRT      (volatile unsigned int*)(IFX_AR9_ETH_REG_BASE + 0x07C)
#define P_PRT_1PPQ0_MASK        0x00000003	/*Priority Queue 0 */
#define P_PRT_1PPQ0_Q0			0x00000000	/*Switch Queue 0 --> Priority Queue 0 */
#define P_PRT_1PPQ0_Q1			0x00000001	/*Swithc Queue 1 --> Priority Queue 0 */
#define P_PRT_1PPQ0_Q2			0x00000002	/*Switch Queue 2 --> Priority Queue 0 */
#define P_PRT_1PPQ0_Q3			0x00000003	/*Switch Queue 3 --> Priority Queue 0 */

#define P_PRT_1PPQ1_MASK		0x0000000C	/*Priority Queue 1 */
#define P_PRT_1PPQ1_Q0			0x00000000	/*Switch Queue 0 --> Priority Queue 1 */
#define P_PRT_1PPQ1_Q1			0x00000004	/*Switch Queue 1 --> Priority Queue 1 */
#define P_PRT_1PPQ1_Q2			0x00000008	/*Switch Queue 2 --> Priority Queue 1 */
#define P_PRT_1PPQ1_Q3			0x0000000C	/*Switch Queue 3 --> Priority Queue 1 */

#define P_PRT_1PPQ2_MASK		0x00000030	/*Priority Queue 2 */
#define P_PRT_1PPQ2_Q0			0x00000000	/*Switch Queue 0 --> Priority Queue 2 */
#define P_PRT_1PPQ2_Q1			0x00000010	/*Switch Queue 1 --> Priority Queue 2 */
#define P_PRT_1PPQ2_Q2			0x00000020	/*Switch Queue 2 --> Priority Queue 2 */
#define P_PRT_1PPQ2_Q3			0x00000030	/*Switch Queue 3 --> Priority Queue 2 */

#define P_PRT_1PPQ3_MASK		0x000000C0	/*Priority Queue 3 */
#define P_PRT_1PPQ3_Q0			0x00000000	/*Switch Queue 0 --> Priority Queue 3 */
#define P_PRT_1PPQ3_Q1			0x00000040	/*Switch Queue 1 --> Priority Queue 3 */
#define P_PRT_1PPQ3_Q2			0x00000080	/*Switch Queue 2 --> Priority Queue 3 */
#define P_PRT_1PPQ3_Q3			0x000000C0	/*Switch Queue 3 --> Priority Queue 3 */

#define P_PRT_1PPQ4_MASK		0x00000300	/*Priority Queue 4 */
#define P_PRT_1PPQ4_Q0			0x00000000	/*Switch Queue 0 --> Priority Queue 4 */
#define P_PRT_1PPQ4_Q1			0x00000100	/*Switch Queue 1 --> Priority Queue 4 */
#define P_PRT_1PPQ4_Q2			0x00000200	/*Switch Queue 2 --> Priority Queue 4 */
#define P_PRT_1PPQ4_Q3			0x00000300	/*Switch Queue 3 --> Priority Queue 4 */

#define P_PRT_1PPQ5_MASK		0x00000C00	/*Priority Queue 5 */
#define P_PRT_1PPQ5_Q0			0x00000000	/*Switch Queue 0 --> Priority Queue 5 */
#define P_PRT_1PPQ5_Q1			0x00000400	/*Switch Queue 1 --> Priority Queue 5 */
#define P_PRT_1PPQ5_Q2			0x00000800	/*Switch Queue 2 --> Priority Queue 5 */
#define P_PRT_1PPQ5_Q3			0x00000C00	/*Switch Queue 3 --> Priority Queue 5 */

#define P_PRT_1PPQ6_MASK		0x00003000	/*Priority Queue 6 */
#define P_PRT_1PPQ6_Q0			0x00000000	/*Switch Queue 0 --> Priority Queue 6 */
#define P_PRT_1PPQ6_Q1			0x00000100	/*Switch Queue 1 --> Priority Queue 6 */
#define P_PRT_1PPQ6_Q2			0x00000200	/*Switch Queue 2 --> Priority Queue 6 */
#define P_PRT_1PPQ6_Q3			0x00000300	/*Switch Queue 3 --> Priority Queue 6 */

#define P_PRT_1PPQ7_MASK		0x0000C000	/*Priority Queue 7 */
#define P_PRT_1PPQ7_Q0			0x00000000	/*Switch Queue 0 --> Priority Queue 7 */
#define P_PRT_1PPQ7_Q1			0x00004000	/*Switch Queue 1 --> Priority Queue 7 */
#define P_PRT_1PPQ7_Q2			0x00008000	/*Switch Queue 2 --> Priority Queue 7 */
#define P_PRT_1PPQ7_Q3			0x0000C000	/*Switch Queue 3 --> Priority Queue 7 */

#define IFX_AR9_ETH_PAUSE_ON_WM  (volatile unsigned int*) (IFX_AR9_ETH_REG_BASE + 0x080)
#define IFX_AR9_ETH_PAUSE_OFF_WM (volatile unsigned int*) (IFX_AR9_ETH_REG_BASE + 0x084)
#define IFX_AR9_ETH_BF_TH_REG    (volatile unsigned int*) (IFX_AR9_ETH_REG_BASE + 0x088)

#define IFX_AR9_ETH_PMAC_HD_CTL (volatile unsigned int*) (IFX_AR9_ETH_REG_BASE + 0x08C)   
#define PMAC_HD_CTL_ADD         0x00000001	/* Add ethernet header to packets from DMA to PMAC */
#define PMAC_HD_CTL_TAG         0x00000002	/* Add VLAN Tag to packets from DMA to PMAC */
#define PMAC_HD_CTL_TYPE_LEN_MASK   0x0003FFFC	/* Contains the Length/Type value added to packest from DMA to PMAC */
#define PMAC_HD_CTL_AC          0x00040000	/* Add CRC for packest from DMA to PMAC */
#define PMAC_HD_CTL_AS          0x00080000	/* Add status header for packest from PMAC to DMA */
#define PMAC_HD_CTL_RC          0x00100000	/* Remove CRC from packest going from PMAC to DMA */
#define PMAC_HD_CTL_RL2         0x00200000	/* Remove Layer-2 Header from packest going from PMAC to DMA */
#define PMAC_HD_CTL_RXSH        0x00400000	/* Add status header for packest from DMA to PMAC */

#define IFX_AR9_ETH_PMAC_SA1    (volatile unsigned int*) (IFX_AR9_ETH_REG_BASE + 0x090)
#define IFX_AR9_ETH_PMAC_SA2    (volatile unsigned int*) (IFX_AR9_ETH_REG_BASE + 0x094)
#define IFX_AR9_ETH_PMAC_DA1    (volatile unsigned int*) (IFX_AR9_ETH_REG_BASE + 0x098)
#define IFX_AR9_ETH_PMAC_DA2    (volatile unsigned int*) (IFX_AR9_ETH_REG_BASE + 0x09C)

#define IFX_AR9_ETH_PMAC_TX_IPG (volatile unsigned int*) (IFX_AR9_ETH_REG_BASE + 0x0A4)
#define IFX_AR9_ETH_PMAC_RX_IPG (volatile unsigned int*) (IFX_AR9_ETH_REG_BASE + 0x0A8)
#define PMAC_RX_IPG_IPG_CNT_MASK    0x0000000F 	/*IPG counter value */
#define PMAC_RX_IPG_IREQ_WM_MASK    0x000000F0 	/* Rx FIFO request watermark value */
#define PMAC_RX_IPG_IDIS_RWQ_WM     0x00000100 	/* Disable RX FIFO request watermark */

#define IFX_AR9_ETH_RMON_CTL    (volatile unsigned int*) (IFX_AR9_ETH_REG_BASE + 0x0C4)
#define RMON_CTL_OFFSET_MASK    0x0000003F	/*Counter offset */
#define RMON_CTL_PORTC_MASK     0x000001C0	/*Port number */
#define RMON_CTL_CAC_MASK       0x00000600	/*Command for access counters*/
#define RMON_CTL_CAC_INDIR      0x00000000	/*Indirect Read counters */
#define RMON_CTL_CAC_PORT_CNT   0x00000200	/*Get port counters (offset 0~237)*/
#define RMON_CTL_CAC_PORT_RST   0x00000400	/*Reset port counters*/
#define RMON_CTL_CAC_RST        0x00000600	/*Reset all counters */
#define RMON_CTL_BUSY           0x00000800	/*Busy/Access state*/

#define IFX_AR9_ETH_RMON_ST     (volatile unsigned int*) (IFX_AR9_ETH_REG_BASE + 0x0C8)
#define IFX_AR9_ETH_MDIO_CTL    (volatile unsigned int*) (IFX_AR9_ETH_REG_BASE + 0x0CC)   
#define MDIO_CTL_REGAD_MASK     0x0000001F	/*Register address */
#define MDIO_CTL_REGAD_SET(arg) ((arg & 0x1F))
#define MDIO_CTL_PHYAD_MASK     0x000003E0	/*PHY address */
#define MDIO_CTL_PHYAD_SET(arg) ((arg & 0x1FF) << 5)
#define MDIO_CTL_OP_MASK        0x00000C00	/* Operation code*/
#define MDIO_CTL_OP_WRITE       0x00000400	/* Write command*/
#define MDIO_CTL_OP_READ        0x00000800	/* Read command*/
#define MDIO_CTL_MBUSY          0x00008000	/* Busy state */
#define MDIO_CTL_WD_MASK        0xFFFF0000	
#define MDIO_CTL_WD_OFFSET      16	
#define MDIO_CTL_WD(val)        ( ( val << MDIO_CTL_WD_OFFSET) & MDIO_CTL_WD_MASK)

#define IFX_AR9_ETH_MDIO_DATA   (volatile unsigned int*) (IFX_AR9_ETH_REG_BASE + 0x0D0)
#define MDIO_CTL_RD_MASK        0x0000FFFF	

#define IFX_AR9_ETH_VLAN_FLT0   (volatile unsigned int*) (IFX_AR9_ETH_REG_BASE + 0x100)
#if defined(BIG_ENDIAN) || defined(__BIG_ENDIAN)
typedef union {
    struct {
        unsigned int M        :  8;
        unsigned int FID      :  2;
        unsigned int TM       :  3;
        unsigned int reserved :  3;
        unsigned int VV       :  1;
        unsigned int VP       :  3;
        unsigned int VID      : 12;
    } Bits;
    unsigned int Register;
} ifx_ar9_eth_vlan_fltx_t;
#endif /*--- #if defined(BIG_ENDIAN) || defined(__BIG_ENDIAN) ---*/

#define IFX_AR9_ETH_DFSRV_MAP0  (volatile unsigned int*) (IFX_AR9_ETH_REG_BASE + 0x150)
#define IFX_AR9_ETH_DFSRV_MAP1  (volatile unsigned int*) (IFX_AR9_ETH_REG_BASE + 0x154)
#define IFX_AR9_ETH_DFSRV_MAP2  (volatile unsigned int*) (IFX_AR9_ETH_REG_BASE + 0x158)
#define IFX_AR9_ETH_DFSRV_MAP3  (volatile unsigned int*) (IFX_AR9_ETH_REG_BASE + 0x15C)

#define IFX_AR9_ETH_ADRTB_CTL0  (volatile unsigned int*) (IFX_AR9_ETH_REG_BASE + 0x0AC)
#define IFX_AR9_ETH_ADRTB_ST0   (volatile unsigned int*) (IFX_AR9_ETH_REG_BASE + 0x0B8)

#define IFX_AR9_ETH_ADRTB_CTL1  (volatile unsigned int*) (IFX_AR9_ETH_REG_BASE + 0x0B0)
#define IFX_AR9_ETH_ADRTB_ST1   (volatile unsigned int*) (IFX_AR9_ETH_REG_BASE + 0x0BC)
#define ADRTB_REG1_ADDR47_32_MASK       (0xffff)
#define ADRTB_REG1_FID_MASK             (0x03         << 16)
#define ADRTB_REG1_FID(a)               (((a) & 0x3)  << 16)
#define ADRTB_REG1_PMAP_MASK            (0xff         << 20)
#define ADRTB_REG1_PMAP(a)              (((a) & 0xff) << 20)
#define ADRTB_REG1_PMAP_PORT(a)         (0x01         << (20 + (a)))

#define IFX_AR9_ETH_ADRTB_CTL2  (volatile unsigned int*) (IFX_AR9_ETH_REG_BASE + 0x0B4)
#define IFX_AR9_ETH_ADRTB_ST2   (volatile unsigned int*) (IFX_AR9_ETH_REG_BASE + 0x0C0)
#define ADRTB_REG2_ITAT_MASK            (0x7ff)
#define ADRTB_REG2_INFOT                (0x001         << 12)
#define ADRTB_REG2_OCP                  (0x001         << 13)
#define ADRTB_REG2_BAD                  (0x001         << 14)
#define ADRTB_REG2_AC_MASK              (0x00f         << 16)
#define ADRTB_REG2_AC(a)                (((a) & 0xf)   << 16)
#define ADRTB_REG2_CMD_MASK             (0x007         << 20)
#define ADRTB_REG2_CMD(a)               (((a) & 0x007) << 20)
#define ADRTB_REG2_IFCE                 (0x001         << 23)
#define ADRTB_REG2_RSLT_MASK            (0x007         << 28)
#define ADRTB_REG2_RSLT_OK              (0x0                )
#define ADRTB_REG2_RSLT_ALL_USED        (0x001         << 28)
#define ADRTB_REG2_RSLT_ERROR           (0x005         << 28)
#define ADRTB_REG2_BUSY                 (0x001         << 31)

// commands are a combination of Command and Access Control bits
#define ADRTB_CMD_CREATE                (0x07          << 16)
#define ADRTB_CMD_REPLACE               (0x0f          << 16)
#define ADRTB_CMD_ERASE                 (0x1f          << 16)
#define ADRTB_CMD_SRCH_MAC              (0x2a          << 16)
#define ADRTB_CMD_SRCH_MAC_FID          (0x2c          << 16)
#define ADRTB_CMD_SRCH_MAC_PORT         (0x29          << 16)
#define ADRTB_CMD_SRCH_ALL              (0x2f          << 16)
#define ADRTB_CMD_INIT_ADDR             (0x34          << 16)
#define ADRTB_CMD_INIT_START            (0x30          << 16)

typedef struct
{
    u32 res31_27  :5; /*!< reserved bits */
    u32 SPID      :3; /*!< Source port ID*/
    u32 res23_18  :6; /*!< reserved bits */
    u32 DPID      :2; /*!< Destination Physical Port ID valid only when DIRECT is set*/
    u32 res15_10  :6; /*!< reserved bits */
    u32 QID       :2; /*!< Queue ID of destination Physical port, valid only when Direct is set*/
    u32 res7_1    :7; /*!< reserved bits */
    u32 DIRECT    :1; /*!< Direct Forwarding Enable*/
}cpu_ingress_pkt_header_t;


typedef struct ar9_pmac_header
{
    u32 ipOffset         :8; /*!< IP offset*/
    u32 destPortMap      :8; /*!< destination port map*/
    u32 srcPortID        :3; /*!< Source port id*/
    u32 inputTag         :1; /*!< input tag flag, packet is received with VALN tag or not*/
    u32 managementPacket :1; /*!< management flag, packet is classifield as managemnt packet*/
    u32 outputTag        :1; /*!< packet is transmitted with VLAN tag to CPU */
    u32 queueID          :2; /*!< Queue ID*/
    u32 crcGenerated     :1; /*!< packet include CRC*/
    u32 crcErr           :1; /*!< crc error packet*/
    u32 arpRarpPacket    :1; /*!< ARP/RARP packet*/
    u32 PPPoEPacket      :1; /*!< PPPoE packet*/
    u32 iPv6Packet       :1; /*!< IPv6 packet*/
    u32 res              :1; /*!< reserved */
    u32 iPv4Packet       :1; /*!< IPv4 packet*/
    u32 mirrored         :1; /*!< Packet was mirrored*/
    u32 res_31_16        :16;    /*!< reserved */
    u32 PKTLEN_8         :8; /*!< packet length Lower 8 bits*/
    u32 res_7_3          :5; /*!< reserved bits 7~3*/
    u32 PKTLEN_3         :3; /*!< packet length higher 3 bits*/
}cpu_egress_pkt_header_t;

#define IFX_RCU_PPE_CONF        ((volatile u32*)(IFX_RCU + 0x002C))
#define DLL_BP_MASK(a)          (0x7 << (9 + (((a) - 1) * 3)))
#define DLL_NO_BYPASS(a)        (0x0)
#define DLL_DELAY_0(a)          (0x1 << (9 + (((a) - 1) * 3)))
#define DLL_DELAY_0_5(a)        (0x2 << (9 + (((a) - 1) * 3)))
#define DLL_DELAY_1(a)          (0x3 << (9 + (((a) - 1) * 3)))
#define DLL_DELAY_1_5(a)        (0x4 << (9 + (((a) - 1) * 3)))
#define DLL_DELAY_2(a)          (0x5 << (9 + (((a) - 1) * 3)))
#define DLL_DELAY_2_5(a)        (0x6 << (9 + (((a) - 1) * 3)))
#define DLL_DELAY_3(a)          (0x7 << (9 + (((a) - 1) * 3)))


#define AMAZON_S_PPE                            (KSEG1 | 0x1E180000)
#define PPE_REG_ADDR(x)                         ((volatile u32*)(AMAZON_S_PPE + (((x) + 0x4000) << 2)))
#define PP32_DEBUG_REG_ADDR(x)                  ((volatile u32*)(AMAZON_S_PPE + (((x) + 0x0000) << 2)))

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
  *  SLL Registers
  */
#define SLL_CMD1                                PPE_REG_ADDR(0x900)
#define SLL_CMD0                                PPE_REG_ADDR(0x901)
#define SLL_KEY(x)                              PPE_REG_ADDR(0x910+x)
#define SLL_RESULT                              PPE_REG_ADDR(0x920)

/*
 *  PP32 Debug Control Register
 */
#define PP32_SRST                               PPE_REG_ADDR(0x0020)

#define PP32_DBG_CTRL                           PP32_DEBUG_REG_ADDR(0x0000)

#define DBG_CTRL_RESTART                        0
#define DBG_CTRL_STOP                           1

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


#endif /*_IFXMIPS_3PORT_REG_H_  */
