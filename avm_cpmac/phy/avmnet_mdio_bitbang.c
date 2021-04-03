/*------------------------------------------------------------------------------------------*\
 *   Copyright (C) 2011 AVM GmbH <fritzbox_info@avm.de>
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
#include <linux/delay.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/types.h>
#include <linux/ethtool.h>
#include <linux/avm_hw_config.h>

#include "avmnet_debug.h"
#include "avmnet_config.h"
#include "avmnet_module.h"
#include "avmnet_common.h"
#include "athrs_phy_ctrl.h"
#include "mdio_reg.h"
#include "avmnet_ar803x.h"

#if defined(CONFIG_MACH_ATHEROS) || defined(CONFIG_ATH79)
#include <asm/mach_avm.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,4,46)
#include <atheros_gpio.h>
#else
#include <avm_atheros_gpio.h>
#endif
#endif

/*------------------------------------------------------------------------------------------*\
 * configures data gpio as input
\*------------------------------------------------------------------------------------------*/
static void bb_mdio_configure_input(struct avmnet_bbmdio *bb_mdio) {

    ath_avm_gpio_ctrl(bb_mdio->data, GPIO_PIN, GPIO_INPUT_PIN);
}

/*------------------------------------------------------------------------------------------*\
 * configures data gpio as output
\*------------------------------------------------------------------------------------------*/
static void bb_mdio_configure_output(struct avmnet_bbmdio *bb_mdio) {

    ath_avm_gpio_out_bit(bb_mdio->data, 1);
    ath_avm_gpio_ctrl(bb_mdio->data, GPIO_PIN, GPIO_OUTPUT_PIN);
}

/*------------------------------------------------------------------------------------------*\
 * generate a clock pulse
\*------------------------------------------------------------------------------------------*/
static void bb_mdio_clk(struct avmnet_bbmdio *bb_mdio) {

   //generate clock
    ndelay(bb_mdio->delay);
    ath_avm_gpio_out_bit(bb_mdio->clk, 1);
    ndelay(bb_mdio->delay);
    ath_avm_gpio_out_bit(bb_mdio->clk, 0);
}

/*------------------------------------------------------------------------------------------*\
 * transfers one bit
\*------------------------------------------------------------------------------------------*/
static void bb_mdio_send_bit(struct avmnet_bbmdio *bb_mdio, uint32_t bit) {

    ndelay(bb_mdio->delay>>1);
    if(bit){
        ath_avm_gpio_out_bit(bb_mdio->data, 1);
    }
    else{
        ath_avm_gpio_out_bit(bb_mdio->data, 0);
    }
    //generate clock
    ndelay(bb_mdio->delay>>1);
    ath_avm_gpio_out_bit(bb_mdio->clk, 1);
    ndelay(bb_mdio->delay);
    ath_avm_gpio_out_bit(bb_mdio->clk, 0);
}

/*------------------------------------------------------------------------------------------*\
 * receives one bit
\*------------------------------------------------------------------------------------------*/
static uint32_t bb_mdio_get_bit(struct avmnet_bbmdio *bb_mdio) {

    //generate clock
    ndelay(bb_mdio->delay);
    ath_avm_gpio_out_bit(bb_mdio->clk, 1);
    ndelay(bb_mdio->delay);
    ath_avm_gpio_out_bit(bb_mdio->clk, 0);
    //return bit
    return ath_avm_gpio_in_bit(bb_mdio->data);
}

/*------------------------------------------------------------------------------------------*\
 * performs the mdio read 
\*------------------------------------------------------------------------------------------*/
uint32_t avmnet_bb_mdio_read(avmnet_module_t *this, uint32_t phy_addr, uint32_t reg) {

    struct athphy_context *phy = (struct athphy_context *) this->priv;
    int i;
    uint16_t ret = 0;

    /*--- AVMNET_ERR("{%s} phy_addr %d reg 0x%x\n", __func__, phy_addr, reg); ---*/

    if ( ! phy->bb_mdio.delay) {
        AVMNET_DEBUG("{%s} ERROR: context! phy_addr %d reg 0x%x\n", __func__, phy_addr, reg);
        return -EIO;
    }

    bb_mdio_configure_output(&phy->bb_mdio);
    //send 32 ones
    for(i = 0; i< 32; i++)
        bb_mdio_send_bit(&phy->bb_mdio, 1);
#ifdef USE_CLAUSE45
    //send start of frame 00 (Clause 45)
    bb_mdio_send_bit(&phy->bb_mdio, 0);
    bb_mdio_send_bit(&phy->bb_mdio, 0);
#else
    //send start of frame 01 (Clause 22)
    bb_mdio_send_bit(&phy->bb_mdio, 0);
    bb_mdio_send_bit(&phy->bb_mdio, 1);
#endif
    //send read frame 01
    bb_mdio_send_bit(&phy->bb_mdio, 1);
    bb_mdio_send_bit(&phy->bb_mdio, 0);
    //send phy_addr
    for(i = 4; i >= 0; i--){
        bb_mdio_send_bit(&phy->bb_mdio, (phy_addr >> i) & 0x01);
    }
    //send reg
    for(i = 4; i >= 0; i--){
        bb_mdio_send_bit(&phy->bb_mdio, (reg >> i) & 0x01);
    }
    bb_mdio_configure_input(&phy->bb_mdio);
    //send turn around and check if second bit is 0, if not read fails
    if(bb_mdio_get_bit(&phy->bb_mdio) != 0){
        for(i = 0; i< 32; i++)
            bb_mdio_clk(&phy->bb_mdio);
        AVMNET_ERR("MDIO read failed\n");
        return 0xFFFF;
    }
    //get data
    for(i = 15; i >= 0; i--){
        ret |= bb_mdio_get_bit(&phy->bb_mdio) << i;
    }
    bb_mdio_clk(&phy->bb_mdio);;
    bb_mdio_configure_output(&phy->bb_mdio);
    ath_avm_gpio_out_bit(phy->bb_mdio.data, 1);

    /*--- AVMNET_DEBUG("{%s} phy_addr %d reg 0x%x 0x%x\n", __func__, phy_addr, reg, ret); ---*/
    return ret;
}

/*------------------------------------------------------------------------------------------*\
 * performs the mdio write
\*------------------------------------------------------------------------------------------*/
int avmnet_bb_mdio_write(avmnet_module_t *this, uint32_t phy_addr, uint32_t reg, uint32_t data) {

    struct athphy_context *phy = (struct athphy_context *) this->priv;
    int i;

    /*--- AVMNET_ERR("{%s} phy_addr %d reg 0x%x data 0x%x\n", __func__, phy_addr, reg, data); ---*/

    if ( ! phy->bb_mdio.delay) {
        AVMNET_DEBUG("{%s} ERROR: context! phy_addr %d reg 0x%x\n", __func__, phy_addr, reg);
        return -EIO;
    }

    bb_mdio_configure_output(&phy->bb_mdio);
    //send 32 ones
    for(i = 0; i< 32; i++)
        bb_mdio_send_bit(&phy->bb_mdio, 1);
#ifdef USE_CLAUSE45
    //send start of frame 00 (Clause 45)
    bb_mdio_send_bit(&phy->bb_mdio, 0);
    bb_mdio_send_bit(&phy->bb_mdio, 0);
#else
    //send start of frame 01 (Clause 22)
    bb_mdio_send_bit(&phy->bb_mdio, 0);
    bb_mdio_send_bit(&phy->bb_mdio, 1);
#endif
    //send write frame 01
    bb_mdio_send_bit(&phy->bb_mdio, 0);
    bb_mdio_send_bit(&phy->bb_mdio, 1);
    //send phy_addr
    for(i = 4; i >= 0; i--){
        bb_mdio_send_bit(&phy->bb_mdio, (phy_addr >> i) & 0x01);
    }
    //send reg
    for(i = 4; i >= 0; i--){
        bb_mdio_send_bit(&phy->bb_mdio, (reg >> i) & 0x01);
    }
    //send turn around
    bb_mdio_send_bit(&phy->bb_mdio, 1);
    bb_mdio_send_bit(&phy->bb_mdio, 0);
    //send data
    for(i = 15; i >= 0; i--){
        bb_mdio_send_bit(&phy->bb_mdio, (data >> i) & 0x01);
    }

    bb_mdio_configure_output(&phy->bb_mdio);
    ath_avm_gpio_out_bit(phy->bb_mdio.data, 1);
    return 0;
}

/*------------------------------------------------------------------------------------------*\
 * initializes mdio gpios
\*------------------------------------------------------------------------------------------*/
int avmnet_bb_mdio_init(avmnet_module_t *this) {

    struct athphy_context *phy;
   
    phy = kzalloc(sizeof(struct athphy_context), GFP_KERNEL);
    if(phy == NULL){
        AVMNET_ERR("{%s} init of avmnet-module %s failed.\n", __func__, this->name);
        return -ENOMEM;
    }

    this->priv = phy;
    phy->this_module = this;
   
    phy->bb_mdio.delay = 500;

    if (avm_get_hw_config(AVM_HW_CONFIG_VERSION, "gpio_avm_bbmdio_clk", &phy->bb_mdio.clk, NULL))
        return -EIO;

    if (avm_get_hw_config(AVM_HW_CONFIG_VERSION, "gpio_avm_bbmdio_data", &phy->bb_mdio.data, NULL))
        return -EIO;

    AVMNET_DEBUG("{%s} clk %d data %d delay %d\n", __func__, phy->bb_mdio.clk, phy->bb_mdio.data, phy->bb_mdio.delay);

    return 0;

#if 0
            bbmdioclk.start = gpio_mdioclk;
            bbmdioclk.end = gpio_mdioclk;
            if(request_resource(&gpio_resource, &ar803x_gpio)){
                AVMNET_ERR(KERN_ERR "[%s] ERROR request resource gpio %d\n", __FUNCTION__, phy->reset);
                return -EIO;
            }
#endif


}
