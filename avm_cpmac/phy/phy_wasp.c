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
#include <linux/avm_hw_config.h>
#include <ifx_gpio.h>
#include <avmnet_common.h>
#include <avmnet_debug.h>
#include <avmnet_module.h>
#include <avmnet_config.h>
#include <mdio_reg.h>
#include "phy_wasp.h"

static avmnet_module_t *wasp_module;
volatile unsigned int wasp_in_reset;

void do_wasp_heartbeat(struct work_struct *work);
void cpmac_magpie_reset(enum avm_cpmac_magpie_reset value);
unsigned int cpmac_magpie_mdio_read(unsigned short regadr, unsigned short phyadr);
void cpmac_magpie_mdio_write(unsigned short regadr, unsigned short phyadr, unsigned short data);

static int phy_reg_open(struct inode *inode, struct file *file);
static int phy_reg_show(struct seq_file *seq, void *data __attribute__ ((unused)) );

static const struct file_operations phy_reg_fops =
{
   .open    = phy_reg_open,
   .read    = seq_read,
   .llseek  = seq_lseek,
   .release = single_release,
};

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_phy_wasp_init(avmnet_module_t *this)
{
    struct avmnet_phy_wasp_context *my_ctx;
    int i, result;

    AVMNET_INFO("[%s] Init on module %s called.\n", __func__, this->name);

    if(wasp_module != NULL){
        AVMNET_ERR("[%s] Multiple instances not supported, aborting\n", __func__);
        return -EFAULT;
    }

    my_ctx = kzalloc(sizeof(struct avmnet_phy_wasp_context), GFP_KERNEL);
    if(my_ctx == NULL){
        AVMNET_ERR("[%s] Init of avmnet-module %s failed.\n", __func__, this->name);
        return -ENOMEM;
    }

    this->priv = my_ctx;

    // store context information in global variables to make them accessible to the wasp boot functions
    wasp_module = this;

    my_ctx->heartbeat_wq = create_singlethread_workqueue("PhyWaspHeartbeat");
    if(my_ctx->heartbeat_wq == NULL){
        AVMNET_ERR("[%s] Creating heartbeat workqueue failed!\n", __FUNCTION__);
        this->priv = NULL;
        kfree(my_ctx);

        return -EFAULT;
    }

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
int avmnet_phy_wasp_setup(avmnet_module_t *this)
{
    int i, result;
    struct avmnet_phy_wasp_context *ctx;

    AVMNET_INFO("[%s] Setup on module %s called.\n", __func__, this->name);

    if(this != wasp_module){
        AVMNET_ERR("[%s] Configuration error detected, multiple instances not supported. Aborting\n", __func__);
        return -EFAULT;
    }

    /*
     * do cunning setup-stuff
     */

    ctx = (struct avmnet_phy_wasp_context *) this->priv;

    sema_init(&(ctx->mutex), 1);

    // reset wasp
    cpmac_magpie_reset(CPMAC_MAGPIE_RESET_PULSE);

    for(i = 0; i < this->num_children; ++i){
        result = this->children[i]->setup(this->children[i]);
        if(result != 0){
            // handle error
        }
    }

    avmnet_cfg_register_module(this);

    /* WASP might not expect MDIO queries in normal operation. Therefore they
     * might be harmful. Additionally, the results of a register dump are
     * meaningless at the moment, because the contain only the last data from
     * the pre boot phase.
     * => Do no register dump for the moment. */
    /*--- avmnet_cfg_add_seq_procentry(this, "mdio_regs", &phy_reg_fops); // cannot be done before avmnet_cfg_register_module ---*/

    ctx = (struct avmnet_phy_wasp_context *) this->priv;
    ctx->heartbeat.this = this;

    INIT_DELAYED_WORK(&(ctx->heartbeat.work), do_wasp_heartbeat);

    queue_delayed_work(ctx->heartbeat_wq, &(ctx->heartbeat.work), CONFIG_HZ);

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_phy_wasp_exit(avmnet_module_t *this)
{
    int i, result;
    struct avmnet_phy_wasp_context *ctx;

    AVMNET_INFO("[%s] Init on module %s called.\n", __func__, this->name);

    for(i = 0; i < this->num_children; ++i){
        result = this->children[i]->exit(this->children[i]);
        if(result != 0){
            // handle error
        }
    }

    /*
     * clean up our mess
     */

    ctx = (struct avmnet_phy_wasp_context *) this->priv;

    cancel_delayed_work_sync(&(ctx->heartbeat.work));
    destroy_workqueue(ctx->heartbeat_wq);

    this->priv = NULL;
    kfree(ctx);

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_phy_wasp_setup_irq(avmnet_module_t *this __attribute__ ((unused)),
                                unsigned int on __attribute__ ((unused)))
{
    int result = 0;

    return result;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
unsigned int avmnet_phy_wasp_reg_read(avmnet_module_t *this __attribute__ ((unused)),
                                        unsigned int addr __attribute__ ((unused)),
                                        unsigned int reg __attribute__ ((unused)))
{
    return 0xffff;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_phy_wasp_reg_write(avmnet_module_t *this __attribute__ ((unused)),
                                unsigned int addr __attribute__ ((unused)),
                                unsigned int reg __attribute__ ((unused)),
                                unsigned int val __attribute__ ((unused)))
{
    return -EINVAL;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_phy_wasp_lock(avmnet_module_t *this) {
    return this->parent->lock(this->parent);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void avmnet_phy_wasp_unlock(avmnet_module_t *this) {
    this->parent->unlock(this->parent);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_phy_wasp_trylock(avmnet_module_t *this) {
    return this->parent->trylock(this->parent);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void avmnet_phy_wasp_status_changed(avmnet_module_t *this, avmnet_module_t *caller)
{
    int i;
    // struct avmnet_phy_wasp_context *my_ctx = (struct avmnet_phy_wasp_context *) this->priv;
    avmnet_module_t *child = NULL;

    /*
     * find out which of our progeny demands our attention
     */
    for(i = 0; i < this->num_children; ++i){
        if(this->children[i] == caller){
            child = this->children[i];
            break;
        }
    }

    if(child == NULL){
        AVMNET_ERR("[%s] module %s: received status_changed from unknown module.\n", __func__, this->name);
        return;
    }

    /*
     * handle status change
     */

    return;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_phy_wasp_poll(avmnet_module_t *this)
{
    int result;
    avmnet_linkstatus_t status = { .Status = 0 };

    // wasp not in reset anymore
    // TODO: Query real state (waiting for firmware, booted, ready, etc.)
//    if(!wasp_in_reset){
        status.Details.link = 1;
        status.Details.speed = 2; // GBit
        status.Details.fullduplex = 1;
        status.Details.powerup = 1;
        status.Details.flowcontrol = 3; // RXTX
//    }

    AVMNET_DEBUG("{%s} power%s, %slink, Speed %d, %sduplex\n", __FUNCTION__, status.Details.powerup ? "up":"down", status.Details.link ? "":"no", status.Details.speed, status.Details.fullduplex ? "full":"half");

    result = this->parent->set_status(this->parent, this->device_id, status);
    if(result < 0){
        AVMNET_ERR("[%s] setting status for device %s failed.\n", __func__, (this->device_id != NULL) ? this->device_id->device_name : "NULL");
    }

    return result;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_phy_wasp_set_status(avmnet_module_t *this, avmnet_device_t *device_id,
                                avmnet_linkstatus_t status)
{
    return this->parent->set_status(this->parent, device_id, status);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void do_wasp_heartbeat(struct work_struct *work)
{
    phy_wasp_heartbeat_work_t *heartbeat;
    avmnet_module_t *this;
    struct avmnet_phy_wasp_context *ctx;

    heartbeat = container_of(to_delayed_work(work), phy_wasp_heartbeat_work_t, work);
    this = heartbeat->this;

    ctx = (struct avmnet_phy_wasp_context *) this->priv;

    queue_delayed_work(ctx->heartbeat_wq, &(ctx->heartbeat.work), CONFIG_HZ);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_phy_wasp_powerdown(avmnet_module_t *this __attribute__ ((unused)))
{

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_phy_wasp_powerup(avmnet_module_t *this __attribute__ ((unused)))
{

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void cpmac_magpie_mdio_write(unsigned short regadr,
                             unsigned short phyadr __attribute__ ((unused)),
                             unsigned short data)
{
    avmnet_module_t *this = wasp_module;

    if (this->lock(this)) {
        return;
    }
    avmnet_mdio_write(this, regadr, data);
    this->unlock(this);
}
EXPORT_SYMBOL(cpmac_magpie_mdio_write);

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
unsigned int cpmac_magpie_mdio_read(unsigned short regadr,
                                    unsigned short phyadr __attribute__ ((unused)))
{
    avmnet_module_t *this = wasp_module;
    unsigned int value;

    if (this->lock(this))
        return -EINTR;

    value = avmnet_mdio_read(this, regadr);
    this->unlock(this);
    return value;
}
EXPORT_SYMBOL(cpmac_magpie_mdio_read);

/*--------------------------------------------------------------------------------*\
\*--------------------------------------------------------------------------------*/
void (*cpmac_magpie_reset_cb_hook)(void);
EXPORT_SYMBOL(cpmac_magpie_reset_cb_hook);
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void cpmac_magpie_reset(enum avm_cpmac_magpie_reset value)
{
    avmnet_module_t *this = wasp_module;
    static int gpio_reserved = 0;
    static int reset_pin = 0xffffffff;
    static enum _avm_hw_param pin_param = avm_hw_param_no_param;
    int result;

    if (this->lock(this)) {
        AVMNET_ERR("[%s] Interrupted while waiting for lock.\n", __func__);
        return;
    }

    if(!gpio_reserved){
        result = avm_get_hw_config(AVM_HW_CONFIG_VERSION,
                                   "gpio_avm_offload_wasp_reset",
                                   &reset_pin,
                                   &pin_param);

        if(result < 0){
            AVMNET_ERR("[%s]: Could not read WASP reset pin data from avm_hw_config!\n", __func__);
            this->unlock(this);
            return;
        }

        if((pin_param != avm_hw_param_gpio_out_active_low) && (pin_param != avm_hw_param_gpio_out_active_high)) {
            AVMNET_ERR("[%s]: Received bogus GPIO pin parameter from avm_hw_config: %x\n", __func__, pin_param);
            this->unlock(this);
            return;
        }

        if(ifx_gpio_register(IFX_GPIO_MODULE_WLAN_OFFLOAD_WASP_RESET)){
            AVMNET_ERR("[%s]: Could not register GPIO WLAN_OFFLOAD_WASP_RESET\n", __func__);
            this->unlock(this);
            return;
        }

        gpio_reserved = 1;
    }

    if((value == CPMAC_MAGPIE_RESET_ON) || (value == CPMAC_MAGPIE_RESET_PULSE)){
        AVMNET_INFO("[%s] Putting Magpie into reset\n", __func__);

        if(pin_param == avm_hw_param_gpio_out_active_low){
            ifx_gpio_output_clear(reset_pin, IFX_GPIO_MODULE_WLAN_OFFLOAD_WASP_RESET);
        }else{
            ifx_gpio_output_set(reset_pin, IFX_GPIO_MODULE_WLAN_OFFLOAD_WASP_RESET);
        }

        wasp_in_reset = 1;
        msleep(100);
    }

    if((value == CPMAC_MAGPIE_RESET_OFF) || (value == CPMAC_MAGPIE_RESET_PULSE)){

        AVMNET_INFO("[%s] Getting Magpie out of reset\n", __func__);
        if(pin_param == avm_hw_param_gpio_out_active_low){
            ifx_gpio_output_set(reset_pin, IFX_GPIO_MODULE_WLAN_OFFLOAD_WASP_RESET);
        }else{
            ifx_gpio_output_clear(reset_pin, IFX_GPIO_MODULE_WLAN_OFFLOAD_WASP_RESET);
        }
		if(cpmac_magpie_reset_cb_hook) {
			msleep(10);
			cpmac_magpie_reset_cb_hook();
		} else {
			msleep(500);
		}
        wasp_in_reset = 0;
    }

    this->unlock(this);

    this->poll(this);
}
EXPORT_SYMBOL(cpmac_magpie_reset);

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static int phy_reg_show(struct seq_file *seq, void *data __attribute__ ((unused)) )
{
    int mdio_reg, mdio_addr;
    avmnet_module_t *this;

    this = (avmnet_module_t *) seq->private;

    mdio_addr = this->initdata.phy.mdio_addr;
    seq_printf(seq, "----------------------------------------\n");
    seq_printf(seq, "dumping regs for phy %s [mdio:%#x]\n", this->name, mdio_addr);

    this->lock(this);

    for(mdio_reg = 0; mdio_reg < 0x20; mdio_reg++){
        seq_printf(seq, "    reg[%#x]: %#x\n", mdio_reg, avmnet_mdio_read(this, mdio_reg));
    }
    seq_printf(seq, "----------------------------------------\n");

    this->unlock(this);
    return 0;
}

static int phy_reg_open(struct inode *inode, struct file *file)
{
    avmnet_module_t *this;

    this = (avmnet_module_t *) PDE_DATA(inode);

    return single_open(file, phy_reg_show, this);
}

