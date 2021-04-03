/*------------------------------------------------------------------------------------------*\
 *   Copyright (C) 2006,2007, ... ,2011 AVM GmbH <fritzbox_info@avm.de>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation version 2 of the License.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
\*------------------------------------------------------------------------------------------*/


#ifndef _MDIO_H_
#define _MDIO_H_

/* avmnet convenience functions */

static inline unsigned int avmnet_mdio_read(avmnet_module_t *this,
                                            unsigned int reg) __maybe_unused;
static inline int avmnet_mdio_write(avmnet_module_t *this, unsigned int reg,
                                    unsigned int value) __maybe_unused;


static inline unsigned int avmnet_mdio_read(avmnet_module_t *this,
                                            unsigned int reg)
{
        BUG_ON(!this || !this->parent);
        BUG_ON(!this->parent->reg_read);

        return this->parent->reg_read(this->parent,
                                      this->initdata.phy.mdio_addr, reg);
}

static inline int avmnet_mdio_write(avmnet_module_t *this, unsigned int reg,
                                    unsigned int value)
{
        BUG_ON(!this || !this->parent);
        BUG_ON(!this->parent->reg_write);

        return this->parent->reg_write(this->parent,
                                       this->initdata.phy.mdio_addr, reg,
                                       value);
}


/* PHY control */
#define MDIO_CONTROL                     0x00u
#  define MDIO_CONTROL_RESET             (1u << 15)  
#  define MDIO_CONTROL_LOOPBACK          (1u << 14)  
#  define MDIO_CONTROL_SPEEDSEL_LSB      (1u << 13) 
#  define MDIO_CONTROL_AUTONEG_ENABLE    (1u << 12)
#  define MDIO_CONTROL_POWERDOWN         (1u << 11)
#  define MDIO_CONTROL_ISOLATE           (1u << 10)
#  define MDIO_CONTROL_AUTONEG_RESTART   (1u <<  9)
#  define MDIO_CONTROL_FULLDUPLEX        (1u <<  8)
#  define MDIO_CONTROL_COLLISION_TEST    (1u <<  7)
#  define MDIO_CONTROL_SPEEDSEL_MSB      (1u <<  6)

/* PHY status */
#define MDIO_STATUS                      0x01u
#  define MDIO_STATUS_100_T4             (1u << 15)
#  define MDIO_STATUS_100_X_FD           (1u << 14)
#  define MDIO_STATUS_100_X_HD           (1u << 13)
#  define MDIO_STATUS_10_FD              (1u << 12)
#  define MDIO_STATUS_10_HD              (1u << 11)
#  define MDIO_STATUS_100_T2_FD          (1u << 10)
#  define MDIO_STATUS_100_T2_HD          (1u <<  9)
#  define MDIO_STATUS_EXTENDED_STATUS    (1u <<  8)
#  define MDIO_STATUS_MF_PREAM_SUPPR     (1u <<  6)
#  define MDIO_STATUS_AUTONEG_COMPLETE   (1u <<  5)
#  define MDIO_STATUS_REMOTE_FAULT       (1u <<  4)
#  define MDIO_STATUS_AUTONEG_ABLE       (1u <<  3)
#  define MDIO_STATUS_LINK               (1u <<  2)
#  define MDIO_STATUS_JABBER             (1u <<  1)
#  define MDIO_STATUS_EXT_CAP            (1u <<  0)

/* PHY identifier */
#define MDIO_PHY_ID1                     0x02u
#define MDIO_PHY_ID2                     0x03u

/* PHY autonegotiation advertisement */
#define MDIO_AUTONEG_ADV                 0x04u
#  define MDIO_AUTONEG_ADV_NEXT_PAGE     (1u << 15)
#  define MDIO_AUTONEG_ADV_RESERVED2     (1u << 14)
#  define MDIO_AUTONEG_ADV_REMOTE_FAULT  (1u << 13)
#  define MDIO_AUTONEG_ADV_RESERVED      (1u << 12)
#  define MDIO_AUTONEG_ADV_PAUSE_ASYM    (1u << 11)
#  define MDIO_AUTONEG_ADV_PAUSE_SYM     (1u << 10)
#  define MDIO_AUTONEG_ADV_100BT4        (1u <<  9)
#  define MDIO_AUTONEG_ADV_100BT_FDX     (1u <<  8)
#  define MDIO_AUTONEG_ADV_100BT_HDX     (1u <<  7)
#  define MDIO_AUTONEG_ADV_10BT_FDX      (1u <<  6)
#  define MDIO_AUTONEG_ADV_10BT_HDX      (1u <<  5)
#  define MDIO_AUTONEG_ADV_SPEED_MASK	 (0x1F << 5)
#  define MDIO_AUTONEG_ADV_SF_MASK       ((1u << 5) - 1)
#  define MDIO_AUTONEG_ADV_SF(x)         ((x) & MDIO_AUTONEG_ADV_SF_MASK)

/* autoneg partner fields identical to autoneg adv */
#define MDIO_AUTONEG_PARTNER             0x05u
#define MDIO_AUTONEG_EXP                 0x06u
#define MDIO_AUTONEG_NEXT_PAGE           0x07u
#define MDIO_AUTONEG_PARTNER_NEXT_PAGE   0x08u

#define MDIO_MASTER_SLAVE_CTRL           0x09u 
/* master-slave bit definitions for both 100BaseT and GBit-ethernet (IEEE 802.3 chapters 32.5.3 and 40.5.1) */
#  define MDIO_MASTER_SLAVE_CTRL_MSEN    (1u << 12)
#  define MDIO_MASTER_SLAVE_CTRL_MS      (1u << 11)
#  define MDIO_MASTER_SLAVE_CTRL_MSPT    (1u << 10)
/* bit definitions for GBit-ethernet */
#  define MDIO_MASTER_SLAVE_CTRL_ADV_FD  (1u <<  9)
#  define MDIO_MASTER_SLAVE_CTRL_ADV_HD  (1u <<  8)

#define MDIO_MASTER_SLAVE_STATUS         0x0Au
#define MDIO_PSE_CTRL                    0x0Bu
#define MDIO_PSE_STATUS                  0x0Cu
#define MDIO_MMD_CTRL                    0x0Du
#define MDIO_MMD_ADDR                    0x0Eu
#define MDIO_EXT_STATUS		             0x0Fu
#  define MDIO_EXT_STATUS_1000_X_FD      (1u << 15)
#  define MDIO_EXT_STATUS_1000_X_HD      (1u << 14)
#  define MDIO_EXT_STATUS_1000_T2_FD     (1u << 13)
#  define MDIO_EXT_STATUS_1000_T2_HD     (1u << 12)

/*--- depend on manufacturer ---*/
#define MDIO_PHY_CONTROL                0x10u
#define     MDIO_PHY_CONTRL_MDI_MANUAL      (0<<5)
#define     MDIO_PHY_CONTRL_MDX_MANUAL      (1<<5)
#define     MDIO_PHY_CONTRL_MDX_AUTO        (3<<5)

#define MDIO_PHY_SPECIFIC               0x11u
#  define MDIO_PHY_SPECIFIC_FC_TX       (1u <<  3)
#  define MDIO_PHY_SPECIFIC_FC_RX       (1u <<  2)

#endif /*--- #define _MDIO_H_ ---*/

