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

#undef INFINEON_CPU
#if defined(CONFIG_VR9) || defined(CONFIG_AR9) || defined(CONFIG_AR10)
#define INFINEON_CPU
#include <ifx_gpio.h>
#include <ifx_types.h>
#endif

#if defined(CONFIG_MACH_ATHEROS) || defined(CONFIG_ATH79)
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,4,46)
#include <atheros.h>
#include <atheros_gpio.h>
#else
#include <avm_atheros.h>
#include <avm_atheros_gpio.h>
#endif
#endif

#include <avmnet_debug.h>
#include <avmnet_module.h>
#include <avmnet_config.h>
#include <mdio_reg.h>
#include "phy_plc.h"

static void reset_plc(avmnet_module_t *this, enum rst_type type);


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_phy_plc_init(avmnet_module_t *this)
{
    struct avmnet_phy_plc_context *plc;
    int i, result;

    AVMNET_INFO("[%s] Init on module %s called.\n", __func__, this->name);

    plc = kzalloc(sizeof(struct avmnet_phy_plc_context), GFP_KERNEL);
    if(plc == NULL){
        AVMNET_ERR("[%s] Init of avmnet-module %s failed.\n", __func__, this->name);
        return -ENOMEM;
    }

    this->priv = plc;

    if(this->initdata.phy.flags & AVMNET_CONFIG_FLAG_RESET){

#if defined(CONFIG_MACH_ATHEROS) || defined(CONFIG_ATH79)
        if(plc->reset < 100){
            /*--- Hängt nicht am Shift-Register => GPIO Resourcen anmelden ---*/
            plc->gpio.name = "plc_reset",
            plc->gpio.flags = IORESOURCE_IO;
            plc->gpio.start = this->initdata.phy.reset;
            plc->gpio.end = this->initdata.phy.reset;
            if(request_resource(&gpio_resource, &(plc->gpio))){
                AVMNET_ERR(KERN_ERR "[%s] ERROR request resource gpio %d\n", __func__, this->initdata.phy.reset);
                kfree(plc);
                return -EIO;
            }
        }
#endif
    }

    sema_init(&(plc->mutex), 1);

    for(i = 0; i < this->num_children; ++i){
        result = this->children[i]->init(this->children[i]);
        if(result < 0){
            AVMNET_ERR("[%s] init failed for module %s\n", __func__, this->name);
            return result;     // handle error
        }
    }
    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_phy_plc_setup(avmnet_module_t *this)
{
    int i, result;
    struct avmnet_phy_plc_context *plc = (struct avmnet_phy_plc_context *) this->priv;

    AVMNET_INFO("[%s] Setup on module %s called.\n", __func__, this->name);

    if (this->lock(this)) {
        return -EINTR;
    }

    /*
     * do cunning setup-stuff
     */
    if(this->initdata.phy.flags & AVMNET_CONFIG_FLAG_RESET){
        reset_plc(this, rst_on);
        plc->powerdown = 1;
    }

    this->unlock(this);

    for(i = 0; i < this->num_children; ++i){
        result = this->children[i]->setup(this->children[i]);
        if(result != 0){
            AVMNET_ERR("[%s] setup failed for module %s\n", __func__, this->name);
            return result;     // handle error
        }
    }

    result = avmnet_cfg_register_module(this);
    if(result < 0){
        AVMNET_ERR("[%s] avmnet_cfg_register_module failed for module %s\n", __func__, this->name);
        return result;
    }
    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_phy_plc_exit(avmnet_module_t *this)
{
    int i, result;
    struct avmnet_phy_plc_context *plc = (struct avmnet_phy_plc_context *) this->priv;

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
#if defined(CONFIG_MACH_ATHEROS) || defined(CONFIG_ATH79)
        if(plc->reset < 100){
            release_resource(&(plc->gpio));
        }
#endif

    this->priv = NULL;
    kfree(plc);

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_phy_plc_setup_irq(avmnet_module_t *this __attribute__ ((unused)),
                                unsigned int on __attribute__ ((unused)))
{
    int result = 0;

    return result;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
unsigned int avmnet_phy_plc_reg_read(avmnet_module_t *this, unsigned int addr, unsigned int reg)
{
    return this->parent->reg_read(this->parent, addr, reg);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_phy_plc_reg_write(avmnet_module_t *this, unsigned int addr, unsigned int reg, unsigned int val)
{
    return this->parent->reg_write(this->parent, addr, reg, val);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_phy_plc_lock(avmnet_module_t *this) {
    return this->parent->lock(this->parent);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void avmnet_phy_plc_unlock(avmnet_module_t *this) {
    this->parent->unlock(this->parent);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_phy_plc_trylock(avmnet_module_t *this) {
    return this->parent->trylock(this->parent);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void avmnet_phy_plc_status_changed(avmnet_module_t *this, avmnet_module_t *caller)
{
    int i;
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
int avmnet_phy_plc_poll(avmnet_module_t *this)
{
    int i, result;

    for(i = 0; i < this->num_children; ++i){
        result = this->children[i]->poll(this->children[i]);
        if(result < 0){
            AVMNET_WARN("Module %s: poll() failed on child %s\n", this->name, this->children[i]->name);
        }
    }

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_phy_plc_set_status(avmnet_module_t *this, avmnet_device_t *device_id,
                                avmnet_linkstatus_t status)
{
    return this->parent->set_status(this->parent, device_id, status);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_phy_plc_powerdown(avmnet_module_t *this) {

    struct avmnet_phy_plc_context *plc = (struct avmnet_phy_plc_context *) this->priv;

    AVMNET_INFO("[%s] Powerdown on module %s called.\n", __func__, this->name);
 
    if(!(this->initdata.phy.flags & AVMNET_CONFIG_FLAG_RESET)){
        AVMNET_WARN("[%s] Module %s: no reset configured.\n", __func__, this->name);
        return 0;
    }

#if defined(CONFIG_MACH_QCA953x) || defined(CONFIG_SOC_QCA953x)
    {
        avmnet_linkstatus_t status;
        status.Status = 0;
        this->parent->set_status(this->parent, this->device_id, status);
    }
#endif

    if (this->lock(this)) {
        return -EINTR;
    }

    reset_plc(this, rst_on);
    plc->powerdown = 1;

    this->unlock(this);
    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_phy_plc_powerup(avmnet_module_t *this) {

    struct avmnet_phy_plc_context *plc = (struct avmnet_phy_plc_context *) this->priv;
#if defined(CONFIG_MACH_QCA953x) || defined(CONFIG_SOC_QCA953x)
    avmnet_linkstatus_t status;
#endif

    AVMNET_INFO("[%s] Powerup on module %s called.\n", __func__, this->name);

    if(!(this->initdata.phy.flags & AVMNET_CONFIG_FLAG_RESET)){
        AVMNET_WARN("[%s] Module %s: no reset configured.\n", __func__, this->name);
        return 0;
    }

    if (this->lock(this)) {
        return -EINTR;
    }

    reset_plc(this, rst_off);
    plc->powerdown = 0;

#if defined(CONFIG_MACH_QCA953x) || defined(CONFIG_SOC_QCA953x)
    {
        int i;

        /*--- reset PHY ---*/
        avmnet_mdio_write(this, MDIO_CONTROL, MDIO_CONTROL_AUTONEG_ENABLE | MDIO_CONTROL_RESET);
        mdelay(100);

        i = 0;
        while((avmnet_mdio_read(this, MDIO_PHY_ID1) == 0xFFFF) && (i < 10)){
            ++i;
            AVMNET_WARN("[%s] Waiting for PHY %s\n", __func__, this->name);
            mdelay(1000);
        }

        if(i >= 10){
            AVMNET_ERR("[%s] Giving up on hanging PHY %s\n", __func__, this->name);
            return -EFAULT;
        }
        AVMNET_DEBUG("{%s} PHY_ID 0x%x 0x%x\n", __func__, 
                avmnet_mdio_read(this, MDIO_PHY_ID1), avmnet_mdio_read(this, MDIO_PHY_ID2));

        status.Details.powerup = 1;
        status.Details.link = 1;
        status.Details.fullduplex = 1;
        status.Details.speed = AVMNET_LINKSTATUS_SPD_1000;

    }
    this->unlock(this);
    this->parent->set_status(this->parent, this->device_id, status);
#else
    this->unlock(this);
#endif
    return 0;
}

#ifdef INFINEON_CPU
#error FIXME
static void reset_plc(avmnet_module_t *this, enum rst_type type)
{
    if((this->initdata.phy.flags & AVMNET_CONFIG_FLAG_RESET) == 0){
        AVMNET_ERR("[%s] Reset not configured for module %s\n", __func__, this->name);
        return;
    }

    if( ! this->trylock(this)) {
        AVMNET_ERR("[%s] Called without MUTEX held!\n", __func__);
        dump_stack();
        this->unlock(this);
    }

    if(type == rst_on || type == rst_pulse){
        AVMNET_INFOTRC("[%s] PHY %s reset on\n", __func__, this->name);
        ifx_gpio_output_clear(this->initdata.phy.reset, IFX_GPIO_MODULE_EXTPHY_RESET);
        mdelay(10);
    }

    if(type == rst_off || type == rst_pulse){
        AVMNET_INFOTRC("[%s] PHY %s reset off\n", __func__, this->name);
        ifx_gpio_output_set(this->initdata.phy.reset, IFX_GPIO_MODULE_EXTPHY_RESET);
        mdelay(10);
    }
}
#elif defined(CONFIG_MACH_ATHEROS) || defined(CONFIG_ATH79)
static void reset_plc(avmnet_module_t *this, enum rst_type type)
{
    if((this->initdata.phy.flags & AVMNET_CONFIG_FLAG_RESET) == 0){
        AVMNET_ERR("[%s] Reset not configured for module %s\n", __func__, this->name);
        return;
    }

    if( ! this->trylock(this)) {
        AVMNET_ERR("[%s] Called without MUTEX held!\n", __func__);
        dump_stack();
        this->unlock(this);
    }

    if(type == rst_on || type == rst_pulse){
        ath_avm_gpio_out_bit(this->initdata.phy.reset, 0);
        mdelay(100);
    }

    if(type == rst_off || type == rst_pulse){
        ath_avm_gpio_out_bit(this->initdata.phy.reset, 1);
        mdelay(100);
    }
}
#endif // INFINEON_CPU


