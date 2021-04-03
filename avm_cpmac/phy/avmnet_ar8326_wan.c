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

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/semaphore.h>
#include <linux/string.h>
#include <linux/sched.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,4,46)
#include <atheros.h>
#include <atheros_gpio.h>
#else
#include <avm_atheros.h>
#include <avm_atheros_gpio.h>
#endif
#include <irq.h>

#include "avmnet_debug.h"
#include "avmnet_module.h"
#include "avmnet_config.h"
#include "avmnet_common.h"
#include "athrs_phy_ctrl.h"
#include "avmnet_ar8326.h"
#include "mdio_reg.h"

unsigned int avmnet_athrs27_reg_read(avmnet_module_t *this, unsigned int s27_addr);
int avmnet_athrs27_reg_write(avmnet_module_t *this, unsigned int s27_addr, unsigned int s27_write_data);

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
#if defined(CONFIG_MACH_QCA956x) || defined(CONFIG_SOC_QCA956X)
int avmnet_ar8326_poll_intern_wan(avmnet_module_t *this) {

    avmnet_linkstatus_t linkstatus;
    int i;
    
    if (this->lock(this)) {
        AVMNET_WARN("[%s] Interrupted while waiting for lock!\n", __func__);
        return -EINTR;
    }

    for (i = 0; i < this->num_children; i++) {
        union avmnet_module_initdata *initdata = &this->children[i]->initdata;
        avmnet_device_t *device_id = this->children[i]->device_id;
        unsigned int port, status;

        switch (initdata->phy.mdio_addr) {
            case 0:
                port = 5;
                break;
            case 4:
                port = 1;
                break;
            default:
                port = initdata->phy.mdio_addr + 1;
                break;
        }

        status = avmnet_athrs27_reg_read(this, S27_PORT_STATUS_REG(port));

        linkstatus.Status = 0;
        AVMNET_DEBUG("{%s} port %d status 0x%x\n", __func__, port, status);
        if (status & S27_PS_LINK)
            linkstatus.Details.link = 1;
        if (status & S27_PS_DUPLEX)
            linkstatus.Details.fullduplex = 1;
        if (status & S27_PS_TX_FLOW_EN)
            linkstatus.Details.flowcontrol |= AVMNET_LINKSTATUS_FC_TX;
        if (status & S27_PS_RX_FLOW_EN)
            linkstatus.Details.flowcontrol |= AVMNET_LINKSTATUS_FC_RX;
        linkstatus.Details.speed = status & 0x3;

        this->parent->set_status(this->parent, device_id, linkstatus);
    }

    this->unlock(this);
    return 0;
}
#endif

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
#if defined(CONFIG_MACH_QCA953x) || defined(CONFIG_SOC_QCA953X)
int avmnet_ar8326_poll_hbee_wan(avmnet_module_t *this) {
    int i;

    for (i=0; i< this->num_children; i++) {
        this->children[i]->poll(this->children[i]);
    }

    return 0;
}
#endif

/*------------------------------------------------------------------------------------------*\
 * auf der 4020 ist der WAN-Port (eth0) auf GMAC0 geschaltet und muss konfiguriert werden
\*------------------------------------------------------------------------------------------*/
#if defined(CONFIG_MACH_QCA956x) || defined(CONFIG_SOC_QCA956X)
int avmnet_ar8326_setup_wan_HW219(avmnet_module_t *this) {
    int    i;

    AVMNET_DEBUG("[%s]\n", __func__);

    for (i=0; i< this->num_children; i++) {
        this->children[i]->setup(this->children[i]);    /*--- die internen PHY initialisieren ---*/
    }

    if (this->lock(this)) {
        AVMNET_WARN("[%s] Interrupted while waiting for lock!\n", __func__);
        goto lock_failed;
    }

    avmnet_athrs27_reg_write(this, S27_OPMODE_REG1, S27_PHY4_MII_EN);

    if (this->initdata.swi.flags & AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_SYM) {
        avmnet_athrs27_reg_write(this, S27_PORT_STATUS_REG(5), S27_PORT_STATUS_DEFAULT | 
                                                               S27_PS_RX_FLOW_EN | 
                                                               S27_PS_TX_FLOW_EN);
    } else {
        avmnet_athrs27_reg_write(this, S27_PORT_STATUS_REG(5), S27_PORT_STATUS_DEFAULT);
    }

    this->unlock(this);
    return 0;
 
lock_failed:

    for (i=0; i< this->num_children; i++) {
         this->children[i]->exit(this->children[i]);
    }

    return -EINTR;
}
#endif

#if defined(CONFIG_MACH_QCA953x) || defined(CONFIG_SOC_QCA953X)
int avmnet_ar8326_setup_wan_HW222(avmnet_module_t *this) {
    int    i;
    unsigned int value;

    AVMNET_DEBUG("[%s]\n", __func__);

    for (i=0; i< this->num_children; i++) {
        this->children[i]->setup(this->children[i]);    /*--- die internen PHY initialisieren ---*/
    }

    if (this->lock(this)) {
        AVMNET_WARN("[%s] Interrupted while waiting for lock!\n", __func__);
        goto lock_failed;
    }

    avmnet_athrs27_reg_write(this, S27_MASK_CTL_REG, S27_MASK_CTRL_RESET);

    i = 0x10;
    while(i--) {
        mdelay(1);
        if( ! (avmnet_athrs27_reg_read(this, S27_MASK_CTL_REG) & S27_MASK_CTRL_RESET))
            break;
    }
    if (i == 0) {
        AVMNET_ERR("[%s] Giving up on hanging Switch %s\n", __func__, this->name);
        return -EFAULT;
    }

    avmnet_athrs27_reg_write(this, S27_OPMODE_REG1, S27_PHY4_MII_EN);

    if (this->initdata.swi.flags & AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_SYM) {
        AVMNET_INFO("{%s} set FLOW_CONTROL\n", __func__);
        avmnet_athrs27_reg_write(this, S27_PORT_STATUS_REG(5), (1<<12) | S27_PORT_STATUS_DEFAULT | 
                                                               S27_PS_RX_FLOW_EN | 
                                                               S27_PS_TX_FLOW_EN);
    } else {
        avmnet_athrs27_reg_write(this, S27_PORT_STATUS_REG(5), S27_PORT_STATUS_DEFAULT);
    }

    /*--- den internen PHY im Honeybee zu einem Link überreden ---*/
    avmnet_s27_wr_phy(this, 0, MDIO_CONTROL, MDIO_CONTROL_RESET | MDIO_CONTROL_AUTONEG_ENABLE | MDIO_CONTROL_AUTONEG_RESTART );

    i = 0x10;
    while(i--) {
        mdelay(1);
        if( ! (avmnet_s27_rd_phy(this, 0, MDIO_CONTROL) & MDIO_CONTROL_RESET))
            break;
    }
    if (i == 0) {
        AVMNET_ERR("[%s] Giving up on hanging PHY 0 %s\n", __func__, this->name);
        return -EFAULT;
    }

    value = avmnet_s27_rd_phy(this, 0, MDIO_PHY_CONTROL);       /*--- disable AUTO_MDX ---*/
    value &= ~(MDIO_PHY_CONTRL_MDX_AUTO);
    avmnet_s27_wr_phy(this, 0, MDIO_PHY_CONTROL, value);

    this->unlock(this);

#if 0
    for (i = 0; i < 6; i++) {
        printk(KERN_ERR "{%s} S27_PORT_STATUS_REG(%d) 0x%x\n", __func__, i, avmnet_athrs27_reg_read(this, S27_PORT_STATUS_REG(i)));
    }
#endif
    return 0;
 
lock_failed:

    for (i=0; i< this->num_children; i++) {
         this->children[i]->exit(this->children[i]);
    }

    return -EINTR;
}
#endif /*--- #if defined(CONFIG_MACH_QCA953x) || defined(CONFIG_SOC_QCA953X) ---*/

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
#if defined(CONFIG_MACH_QCA953x) || defined(CONFIG_SOC_QCA953X)
static int avmnet_proc_switch_write(struct file *file __attribute__ ((unused)), const char __user *buf, 
                                    size_t count, 
                                    loff_t *offset __attribute__((unused))) {

    static char cmd[256];
    char *token[3];
    unsigned int number_of_tokens = 0;
    int len;
    unsigned int reg, value;
    avmnet_module_t *this = (avmnet_module_t *) file->private_data;

    len = (sizeof(cmd) - 1) < count ? (sizeof(cmd) - 1) : count;
    len = len - copy_from_user(cmd, buf, len);
    cmd[len] = 0;

    number_of_tokens = avmnet_tokenize(cmd, " ", 3, token);

    if(this->lock(this)) {
        AVMNET_WARN("[%s] Interrupted while waiting for lock!\n", __func__);
        return -EINTR;
    }

    switch (number_of_tokens) {
        case 2:
            if ( ! strncasecmp(token[0], "read", 4)) {
                sscanf(token[1], "%x", &reg);
                value = avmnet_athrs27_reg_read(this, reg);
                AVMNET_SUPPORT("register_read reg %#x = %#x\n", reg, value);
            }
            break;
        case 3:
            if ( ! strncasecmp(token[0], "write", 5)) {
                sscanf(token[1], "%x", &reg);
                sscanf(token[2], "%x", &value);
                avmnet_athrs27_reg_write(this, reg, value);
                AVMNET_SUPPORT("register_write reg %#x = %#x\n", reg, value);
            }
            break;
    }

    this->unlock(this);
    return count;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static int avmnet_proc_switch_open(struct inode *inode, struct file *file)
{
	file->private_data = PDE_DATA(inode);
	return 0;
}

static const struct file_operations avmnet_proc_switch_fops = {
	.open = avmnet_proc_switch_open,
	.write = avmnet_proc_switch_write,
};
#endif /*--- #if defined(CONFIG_MACH_QCA953x) || defined(CONFIG_SOC_QCA953X) ---*/

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_ar8326_init_wan(avmnet_module_t *this) {

    struct aths27_context *ctx;
    int i, result;

    AVMNET_DEBUG("[%s] Init on module %s called.\n", __func__, this->name);

    ctx = kzalloc(sizeof(struct aths27_context), GFP_KERNEL);
    if(ctx == NULL){
        AVMNET_ERR("[%s] init of avmnet-module %s failed.\n", __func__, this->name);
        return -ENOMEM;
    }

    this->priv = ctx;
    ctx->this_module = this;

    if (this->setup_special_hw)
        this->setup_special_hw(this);

    for(i = 0; i < this->num_children; ++i){
        result = this->children[i]->init(this->children[i]);
        if(result < 0){
            // handle error
        }
    }

    avmnet_cfg_register_module(this);
#if defined(CONFIG_MACH_QCA953x) || defined(CONFIG_SOC_QCA953X)
    avmnet_cfg_add_procentry(this, "switch_reg", &avmnet_proc_switch_fops);
#endif /*--- #if defined(CONFIG_MACH_QCA953x) || defined(CONFIG_SOC_QCA953X) ---*/

    return 0;
}
 
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_ar8326_exit_wan(avmnet_module_t *this) {

    int i, result;

    AVMNET_DEBUG("[%s] Exit on module %s called.\n", __func__, this->name);

    for(i = 0; i < this->num_children; ++i){
        result = this->children[i]->exit(this->children[i]);
        if(result != 0){
            // handle error
        }
    }

    kfree(this->priv);
    return 0;
}

