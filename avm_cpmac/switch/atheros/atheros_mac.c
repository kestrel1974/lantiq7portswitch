/*------------------------------------------------------------------------------------------*\
 *   Copyright (C) 2011,2012 AVM GmbH <fritzbox_info@avm.de>
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

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/types.h>
#include <asm/byteorder.h>
#include <linux/init.h>
#include <linux/errno.h>

#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/mm.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/list.h>

#include <asm/irq.h>
#include <asm/io.h>
#include <asm/prom.h>
#include <net/sch_generic.h>

#include "avmnet_debug.h"
#include "avmnet_module.h"
#include "avmnet_config.h"
#include "avmnet_common.h"
#include "../management/avmnet_links.h"

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,4,46)
#include <atheros.h>
#else
#include <avm_atheros.h>
#endif
#include "ag934x.h"
#include "atheros_mac.h"

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
#if defined(CONFIG_MACH_QCA955x) || defined(CONFIG_SOC_QCA955X)
void athrs_sgmii_res_cal(void) {

	unsigned int read_data, read_data_otp, otp_value, otp_per_val, rbias_per;
	unsigned int rbias_pos_or_neg, res_cal_val;
	unsigned int sgmii_pos, sgmii_res_cal_value;
	unsigned int reversed_sgmii_value, use_value;

	ath_reg_wr(OTP_INTF2_ADDRESS, 0x7d);
	ath_reg_wr(OTP_LDO_CONTROL_ADDRESS, 0x0);

	while (ath_reg_rd(OTP_LDO_STATUS_ADDRESS) & OTP_LDO_STATUS_POWER_ON_MASK);

	read_data = ath_reg_rd(OTP_MEM_0_ADDRESS + 4);

	while (!(ath_reg_rd(OTP_STATUS0_ADDRESS) & OTP_STATUS0_EFUSE_READ_DATA_VALID_MASK));

	read_data_otp = ath_reg_rd(OTP_STATUS1_ADDRESS);

	if (read_data_otp & 0x1fff) {
		read_data = read_data_otp;
	}

	if (read_data & 0x00001000) {
		otp_value = (read_data & 0xfc0) >> 6;
	} else {
		otp_value = read_data & 0x3f;
	}

	if (otp_value > 31) {
		otp_per_val = 63 - otp_value;
		rbias_pos_or_neg = 1;
	} else {
		otp_per_val = otp_value;
		rbias_pos_or_neg = 0;
	}

	rbias_per = otp_per_val * 15;

	if (rbias_pos_or_neg == 1) {
		res_cal_val = (rbias_per + 34) / 21;
		sgmii_pos = 1;
	} else {
		if (rbias_per > 34) {
			res_cal_val = (rbias_per - 34) / 21;
			sgmii_pos = 0;
		} else {
			res_cal_val = (34 - rbias_per) / 21;
			sgmii_pos = 1;
		}
	}

	if (sgmii_pos == 1) {
		sgmii_res_cal_value = 8 + res_cal_val;
	} else {
		sgmii_res_cal_value = 8 - res_cal_val;
	}

	reversed_sgmii_value = 0;
	use_value = 0x8;
	reversed_sgmii_value = reversed_sgmii_value | ((sgmii_res_cal_value & use_value) >> 3);
	use_value = 0x4;
	reversed_sgmii_value = reversed_sgmii_value | ((sgmii_res_cal_value & use_value) >> 1);
	use_value = 0x2;
	reversed_sgmii_value = reversed_sgmii_value | ((sgmii_res_cal_value & use_value) << 1);
	use_value = 0x1;
	reversed_sgmii_value = reversed_sgmii_value | ((sgmii_res_cal_value & use_value) << 3);

	reversed_sgmii_value &= 0xf;

	// To Check the locking of the SGMII PLL
	read_data = (ath_reg_rd(SGMII_SERDES_ADDRESS) & ~SGMII_SERDES_RES_CALIBRATION_MASK) | 
        SGMII_SERDES_RES_CALIBRATION_SET(reversed_sgmii_value);
	ath_reg_wr(SGMII_SERDES_ADDRESS, read_data);
	ath_reg_wr(ETH_SGMII_SERDES_ADDRESS, 
            ETH_SGMII_SERDES_EN_LOCK_DETECT_MASK | 
            ETH_SGMII_SERDES_PLL_REFCLK_SEL_MASK | 
            ETH_SGMII_SERDES_EN_PLL_MASK);

    AVMNET_DEBUG("{%s} SGMII_SERDES_CTRL 0x%x\n", __func__,  ath_reg_rd(SGMII_SERDES_ADDRESS));

    {
        unsigned int sgmii_serdes_ctl = ath_reg_rd(SGMII_SERDES_ADDRESS);

        sgmii_serdes_ctl &= ~(  SGMII_SERDES_CDR_BW_MASK |
			                    SGMII_SERDES_TX_DR_CTRL_MASK |
			                    SGMII_SERDES_PLL_BW_MASK |
			                    SGMII_SERDES_EN_SIGNAL_DETECT_MASK |
			                    SGMII_SERDES_FIBER_SDO_MASK |
			                    SGMII_SERDES_VCO_REG_MASK
                             );

        sgmii_serdes_ctl |= ( SGMII_SERDES_CDR_BW_SET(3) |
			                  SGMII_SERDES_TX_DR_CTRL_SET(1) |        /*--- 600mV Amplitude ---*/
			                  SGMII_SERDES_PLL_BW_SET(1) |
                              SGMII_SERDES_EN_SIGNAL_DETECT_SET(1) |
                              SGMII_SERDES_FIBER_SDO_SET(1) |
                              SGMII_SERDES_VCO_REG_SET(3)
                            );


	    ath_reg_wr(SGMII_SERDES_ADDRESS, sgmii_serdes_ctl);
    }

    AVMNET_DEBUG("{%s} SGMII_SERDES_CTRL 0x%x\n", __func__,  ath_reg_rd(SGMII_SERDES_ADDRESS));

	ath_reg_rmw_clear(ATH_RST_RESET, ATH_RESET_GE1_PHY);
	udelay(500);
	ath_reg_rmw_clear(ATH_RST_RESET, ATH_RESET_GE0_PHY);

	while (!(ath_reg_rd(SGMII_SERDES_ADDRESS) & SGMII_SERDES_LOCK_DETECT_STATUS_MASK));
}
#endif /*--- #if defined(CONFIG_MACH_QCA955x) || defined(CONFIG_SOC_QCA955X) ---*/

#if defined(CONFIG_MACH_QCA956x) || defined(CONFIG_SOC_QCA956X) || defined(CONFIG_SOC_QCN550X)
void athrs_sgmii_res_cal(void) {

	unsigned int v;
	unsigned int reversed_sgmii_value;
	unsigned int i=0;
	unsigned int vco_fast,vco_slow;
	unsigned int startValue=0, endValue=0;

	ath_reg_wr (ATH_PLL_ETH_SGMII_SERDES, 
            ATH_PLL_ETH_SGMII_SERDES_EN_LOCK_DETECT_MASK | ATH_PLL_ETH_SGMII_SERDES_PLL_EN_MASK);

	v = ath_reg_rd (ATH_GMAC_SGMII_SERDES);

    // [GJu] Diese Bits sind laut Datenblatt reserviert. Im Zweifel kommt immer 0 raus ..
	vco_fast = ATH_GMAC_SGMII_SERDES_VCO_FAST_GET (v);
	vco_slow = ATH_GMAC_SGMII_SERDES_VCO_SLOW_GET (v);

	/* set resistor Calibration from 0000 -> 1111 */
	for (i = 0; i < 0x10; i++) {
		v = (ath_reg_rd ( ATH_GMAC_SGMII_SERDES) & ~ATH_GMAC_SGMII_SERDES_RES_CALIBRATION_MASK) | 
            ATH_GMAC_SGMII_SERDES_RES_CALIBRATION_SET (i); 
		ath_reg_wr (ATH_GMAC_SGMII_SERDES, v);
		udelay (50);
		v = ath_reg_rd (ATH_GMAC_SGMII_SERDES);

		if ( (ATH_GMAC_SGMII_SERDES_VCO_FAST_GET (v) != vco_fast) || 
                 (ATH_GMAC_SGMII_SERDES_VCO_SLOW_GET (v) != vco_slow) ) {
			if (startValue == 0) {
				startValue = endValue = i;
			} else {
				endValue = i;
			}
		}

		vco_fast = ATH_GMAC_SGMII_SERDES_VCO_FAST_GET (v);
		vco_slow = ATH_GMAC_SGMII_SERDES_VCO_SLOW_GET (v);		
	}
	
	if (0 == startValue) {
		/* No boundary found, use middle value for resistor calibration value */
		reversed_sgmii_value = 0x7;
	} else { 
		/* get resistor calibration from the middle of boundary */
		reversed_sgmii_value = (startValue + endValue) / 2; 
	}

	v = (ath_reg_rd ( ATH_GMAC_SGMII_SERDES) & ~ATH_GMAC_SGMII_SERDES_RES_CALIBRATION_MASK) | 
        ATH_GMAC_SGMII_SERDES_RES_CALIBRATION_SET (reversed_sgmii_value);

	ath_reg_wr (ATH_GMAC_SGMII_SERDES, v);

	ath_reg_wr ( ATH_PLL_ETH_SGMII_SERDES,  
            ATH_PLL_ETH_SGMII_SERDES_EN_LOCK_DETECT_MASK | 
            ATH_PLL_ETH_SGMII_SERDES_PLL_EN_MASK );

	ath_reg_rmw_set ( ATH_GMAC_SGMII_SERDES,
			ATH_GMAC_SGMII_SERDES_CDR_BW_SET           (3) |
			ATH_GMAC_SGMII_SERDES_TX_DR_CTRL_SET       (1) |          /*--- 600mV ---*/
			ATH_GMAC_SGMII_SERDES_PLL_BW_SET           (1) |
			ATH_GMAC_SGMII_SERDES_EN_SIGNAL_DETECT_SET (1) |
			ATH_GMAC_SGMII_SERDES_FIBER_SDO_SET        (1) |
			ATH_GMAC_SGMII_SERDES_VCO_REG_SET          (3) );

	ath_reg_rmw_clear ( ATH_RST_RESET, ATH_RESET_GE1_PHY);      /*--- SGMII-Reset ---*/
	udelay (500);
	ath_reg_rmw_clear ( ATH_RST_RESET, ATH_RESET_GE0_PHY);      /*--- SGMII-Reset ---*/

	while (!(ath_reg_rd (ATH_GMAC_SGMII_SERDES) & ATH_GMAC_SGMII_SERDES_LOCK_DETECT_STATUS_MASK)) ;
}
#endif /*--- #if defined(CONFIG_MACH_QCA956x) || defined(CONFIG_SOC_QCA956X) ---*/

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
#if 0       /*--- not used ---*/
static int athr_gmac_do_ioctl(struct net_device *dev __attribute__((unused)),struct ifreq *ifr __attribute__((unused)), int cmd) {
    int ret = -EPERM;
    switch (cmd){
#if 0
        case ATHR_GMAC_CTRL_IOC:
            ret = athr_gmac_ctrl(dev,ifr,cmd);
            break;
        case ATHR_PHY_CTRL_IOC:
            ret = athr_phy_ctrl(dev,ifr,cmd);
            break;
#ifdef CONFIG_ATHRS_QOS
        case ATHR_GMAC_QOS_CTRL_IOC:
            ret = athr_config_qos(ifr->ifr_data,cmd);
            break;
#endif
#ifdef CONFIG_ATHR_VLAN_IGMP
        case ATHR_VLAN_IGMP_IOC:
           ret = athr_vlan_ctrls(dev,ifr,cmd);
           break;
#endif
#ifdef CONFIG_ATHRS_HW_ACL
        case ATHR_HW_ACL_IOC:
           ret = athr_hw_acl_config(ifr->ifr_data, cmd, dev);
           break;
#endif
#       if defined(CONFIG_AVM_CPMAC) && CONFIG_AVM_CPMAC
        case AVM_CPMAC_IOCTL_GENERIC:
           /* If the ioctl is not defined, fall through to the error message */
           if(!IS_ERR(&cpmac_virian_ioctl) && &cpmac_virian_ioctl) {
               ret = cpmac_virian_ioctl(dev, ifr, cmd);
           }
#       endif /*--- #if defined(CONFIG_AVM_CPMAC) && CONFIG_AVM_CPMAC ---*/
#endif
        default:
           break;
    }
  
  return ret;
}
#endif

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int athmac_setup(avmnet_module_t *this) {

    int i, result;
    
    AVMNET_INFO("[%s] Setup on module %s called.\n", __func__, this->name);

    /*--- reset Hardware ---*/
#if defined(CONFIG_MACH_QCA955x) || defined(CONFIG_SOC_QCA955X) || \
    defined(CONFIG_MACH_QCA956x) || defined(CONFIG_SOC_QCA956X) || \
    defined(CONFIG_SOC_QCN550X)
    ath_reg_rmw_set(ATH_RST_RESET, ATH_RESET_GE0_PHY | ATH_RESET_GE1_PHY);
#else /*--- #if defined(CONFIG_MACH_QCA955x) ---*/
    ath_reg_rmw_set(ATH_RST_RESET, ATH_RESET_GE0_PHY | ATH_RESET_GE1_PHY);
    mdelay(50);
    ath_reg_rmw_clear(ATH_RST_RESET, ATH_RESET_GE0_PHY | ATH_RESET_GE1_PHY);
    mdelay(200);
#endif /*--- #else ---*/ /*--- #if defined(CONFIG_MACH_QCA95xx) ---*/
    ath_reg_rmw_set(ATH_RST_RESET, 
            ATH_RESET_GE0_MAC | 
            ATH_RESET_GE1_MAC | 
            ATH_RESET_GE0_MDIO | 
            ATH_RESET_GE1_MDIO);
    mdelay(100);
    ath_reg_rmw_clear(ATH_RST_RESET, 
            ATH_RESET_GE0_MAC | 
            ATH_RESET_GE1_MAC | 
            ATH_RESET_GE0_MDIO | 
            ATH_RESET_GE1_MDIO);

#if defined(CONFIG_MACH_QCA955x) || defined(CONFIG_SOC_QCA955X) || \
    defined(CONFIG_MACH_QCA956x) || defined(CONFIG_SOC_QCA956X) || \
    defined(CONFIG_SOC_QCN550X)
    ath_reg_wr ( ATH_PLL_ETH_SGMII_SERDES, 
            ATH_PLL_ETH_SGMII_SERDES_PLL_REFCLK_SEL_SET(1) | 
            ATH_PLL_ETH_SGMII_SERDES_EN_LOCK_DETECT_SET(1) );

    {
        unsigned int usb_clock, switch_source = ath_reg_rd (ATH_PLL_SWITCH_CLOCK_CONTROL);

        usb_clock = switch_source & ATH_PLL_SWITCH_CLOCK_CONTROL_USB_REFCLK_FREQ_SEL_MASK;

        switch_source = usb_clock | ATH_PLL_SWITCH_CLOCK_CONTROL_MDIO_CLK_SEL1_1_SET (1);
#if defined(CONFIG_MACH_QCA956x) || defined(CONFIG_SOC_QCA956X)
        switch_source |= 
            ATH_PLL_SWITCH_CLOCK_CONTROL_OEN_CLK125M_SEL_SET (1) | 
            ATH_PLL_SWITCH_CLOCK_CONTROL_EN_PLL_TOP_SET      (1) ;

        if (0x200 == usb_clock) {
            switch_source |= ATH_PLL_SWITCH_CLOCK_CONTROL_SWITCHCLK_SEL_SET (1);
        }
#endif

        ath_reg_wr (ATH_PLL_SWITCH_CLOCK_CONTROL, switch_source);
    }
    athrs_sgmii_res_cal();
#endif /*--- #if defined(CONFIG_MACH_QCA955x) || defined(CONFIG_SOC_QCA955X) || defined(CONFIG_MACH_QCA956x) || defined(CONFIG_SOC_QCA955X) || defined(CONFIG_SOC_QCN550X) ---*/

    for(i = 0; i < this->num_children; ++i){
        result = this->children[i]->setup(this->children[i]);
        if(result != 0){
            // handle error
        }
    }

    avmnet_cfg_register_module(this);

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
#if defined(CONFIG_MACH_QCA956x) || defined(CONFIG_SOC_QCA956X) || defined(CONFIG_SOC_QCN550X)
int athmac_setup_hw219(avmnet_module_t *this) {

    int i, result;
    
    AVMNET_INFO("[%s] Setup on module %s called.\n", __func__, this->name);

    /*--- reset Hardware ---*/
    ath_reg_rmw_set( ATH_RST_RESET, ATH_RESET_GE0_PHY | ATH_RESET_GE1_PHY );    /*--- SGMII-Reset / SGMII-Analog-Reset ---*/
    ath_reg_rmw_set( ATH_RST_RESET, 
                     ATH_RESET_GE1_MDIO |
                     ATH_RESET_GE0_MDIO | 
                     ATH_RESET_GE1_MAC  | 
                     ATH_RESET_GE0_MAC  | 
                     ATH_RESET_ETH_SWITCH_ANALOG_RESET );
    mdelay(10);
    ath_reg_rmw_set ( ATH_RST_RESET, ATH_RESET_ETH_SWITCH_RESET );

    {
        unsigned int v, usb_clock;

        v = ath_reg_rd (ATH_PLL_SWITCH_CLOCK_CONTROL);

        usb_clock =  v & ATH_PLL_SWITCH_CLOCK_CONTROL_USB_REFCLK_FREQ_SEL_MASK;

        v |= ATH_PLL_SWITCH_CLOCK_CONTROL_OEN_CLK125M_SEL_SET (1) |
            ATH_PLL_SWITCH_CLOCK_CONTROL_EN_PLL_TOP_SET       (1) |
            ATH_PLL_SWITCH_CLOCK_CONTROL_MDIO_CLK_SEL1_1_SET  (1);

        if (0x200 == usb_clock) {
            v |= ATH_PLL_SWITCH_CLOCK_CONTROL_SWITCHCLK_SEL_SET (1);
        }

        ath_reg_wr (ATH_PLL_SWITCH_CLOCK_CONTROL, v);
    }

    mdelay (100);
    ath_reg_rmw_clear( ATH_RST_RESET, 
                       ATH_RESET_GE1_MDIO | 
                       ATH_RESET_GE0_MDIO | 
                       ATH_RESET_GE1_MAC  | 
                       ATH_RESET_GE0_MAC  | 
                       ATH_RESET_GE0_PHY  |    
                       ATH_RESET_GE1_PHY  |    
                       ATH_RESET_ETH_SWITCH_RESET );
    mdelay (10);
    ath_reg_rmw_clear( ATH_RST_RESET, ATH_RESET_ETH_SWITCH_ANALOG_RESET );
    mdelay (10);

    for(i = 0; i < this->num_children; ++i){
        result = this->children[i]->setup(this->children[i]);
        if(result != 0){
            // handle error
        }
    }

    avmnet_cfg_register_module (this);

    return 0;
}
#endif

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int athmac_lock(avmnet_module_t *this) {
    struct athmac_context *athmac = (struct athmac_context *)this->priv;

    return down_interruptible(&athmac->lock);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void athmac_unlock(avmnet_module_t *this) {
    struct athmac_context *athmac = (struct athmac_context *)this->priv;

    up(&athmac->lock);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int athmac_trylock(avmnet_module_t *this) {
    struct athmac_context *athmac = (struct athmac_context *)this->priv;

    return down_trylock(&athmac->lock);
}

/*------------------------------------------------------------------------------------------*\
 * die Hardware für MDIO-Zugriffe muss hier gelockt werden, da u.U. GMAC0 und GMAC1 auf die
 * MDIO-Hardware des GMAC1 zugreifen
\*------------------------------------------------------------------------------------------*/
int athmac_init(avmnet_module_t *this) {

    struct athmac_context *athmac;
    int i, result;

    AVMNET_DEBUG("{%s} Init on module %s called.\n", __func__, this->name);

    this->priv = kmalloc(sizeof(struct athmac_context), GFP_KERNEL);
    if ( ! this->priv) {
        AVMNET_ERR("{%s} Could not allocate Memory\n", __func__);
        return -ENOMEM;
    }

    athmac = (struct athmac_context *)this->priv;
    sema_init(&athmac->lock, 1);

    for(i = 0; i < this->num_children; ++i){
        result = this->children[i]->init(this->children[i]);
        if(result < 0){
            // handle error
        }
    }

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int athmac_exit(avmnet_module_t *this) {

    int i, result;
   
    AVMNET_DEBUG("{%s} Init on module %s called.\n", __func__, this->name);
    
    for(i = 0; i < this->num_children; ++i){
        result = this->children[i]->exit(this->children[i]);
        if(result != 0){
            // handle error
        }
    }

    /*
     * clean up our mess
     */
    kfree(this->priv);

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int athmac_set_status(avmnet_module_t *this, avmnet_device_t *device_id,
                      avmnet_linkstatus_t status)
{

    if(this->parent != NULL){
        return this->parent->set_status(this->parent, device_id, status);
    }

    return 0;
}


