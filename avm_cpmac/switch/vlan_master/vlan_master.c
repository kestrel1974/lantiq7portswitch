/*------------------------------------------------------------------------------------------*\
 *   Copyright (C) 2016 AVM GmbH <fritzbox_info@avm.de>
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

#include <avmnet_debug.h>
#include <avmnet_module.h>
#include <avmnet_config.h>
#include <avmnet_common.h>

#include "vlan_master.h"
#include "../../management/avmnet_links.h"

static int vlan_device_event(struct notifier_block *unused,
                             unsigned long event,
                             void *ptr);

static struct notifier_block vlan_notifier_block __read_mostly = {
    .notifier_call = vlan_device_event,
};

#define MAX_PORTS 10
unsigned int port_open[MAX_PORTS];
DEFINE_SPINLOCK(port_open_lock);

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_vlan_master_init(avmnet_module_t *this)
{
    struct avmnet_vlan_master_context *my_ctx;
    int i, result;

    AVMNET_TRC("[%s] Init on module %s called.\n", __func__, this->name);

    my_ctx = kzalloc(sizeof(struct avmnet_vlan_master_context), GFP_KERNEL);
    if(my_ctx == NULL){
        AVMNET_ERR("[%s] init of avmnet-module %s failed.\n", __func__, this->name);
        return -ENOMEM;
    }

    sema_init(&(my_ctx->lock), 1);

    this->priv = my_ctx;
    for(i = 0; i < ARRAY_SIZE(port_open); ++i){
        port_open[i] = 0;
    }

    for(i = 0; i < this->num_children; ++i){
        result = this->children[i]->init(this->children[i]);
        if(result != 0){
            AVMNET_WARN("Module %s: init() failed on child %s\n", this->name,
                        this->children[i]->name);
            return result;
        }
    }

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static int setup_children(avmnet_module_t *this)
{
    int i, result;
    struct avmnet_vlan_master_context *ctx;

    ctx = (struct avmnet_vlan_master_context *) this->priv;
    result = 0;

    ctx->master = avmnet_get_master();
    if(ctx->master == NULL){
        AVMNET_INFO("[%s] No VLAN master device registered, deferring setup\n", __func__);
        goto err_out;
    }

    AVMNET_INFO("[%s] Using device %s as master\n", __func__, ctx->master->master_dev->name);

    /* we can now call setup on our children. Set setup_done flag now,
     * so we will not try again if it fails
     */
    ctx->setup_done = 1;

    /* change name of master device */
    memcpy(ctx->oldname, ctx->master->master_dev->name, IFNAMSIZ);

    rtnl_lock();
    result = dev_change_name(ctx->master->master_dev, "vlan_master%d");
    rtnl_unlock();

    if(result != 0){
        AVMNET_WARN("[%s] Error changing master device's name: %d\n", __func__, result);
    }

    /* Call set-up in all children */
    for(i = 0; i < this->num_children; ++i){
        result = this->children[i]->setup(this->children[i]);
        if(result != 0){
            AVMNET_ERR("Module %s: setup() failed on child %s\n", this->name,
                       this->children[i]->name);
            goto err_out;
        }
    }

    result = avmnet_create_netdevices();
    if(result != 0){
        /* FIXME: BUG()? */
        goto err_out;
    }

    result = register_netdevice_notifier(&vlan_notifier_block);
    if(result != 0){
        AVMNET_ERR("[%s] register_netdevice_notifier() failed: %d\n", __func__, result);
        goto err_out;
    }

err_out:
    return result;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_vlan_master_setup(avmnet_module_t *this)
{
    int result;

    AVMNET_INFO("[%s] Setup on module %s called.\n", __func__, this->name);

    /*
     * do cunning setup-stuff
     */

    avmnet_cfg_register_module(this);

    result = setup_children(this);

    return result;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_vlan_master_exit(avmnet_module_t *this)
{
    int i, result;
    struct avmnet_vlan_master_context *ctx;

    AVMNET_INFO("[%s] Init on module %s called.\n", __func__, this->name);

    for(i = 0; i < this->num_children; ++i){
        result = this->children[i]->exit(this->children[i]);
        if(result != 0){
            AVMNET_WARN("Module %s: exit() failed on child %s\n", this->name,
                        this->children[i]->name);
        }
    }

    /*
     * clean up our mess
     */
    ctx = (struct avmnet_vlan_master_context *) this->priv;

    this->priv = NULL;
    kfree(ctx);

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_vlan_master_setup_irq(avmnet_module_t *this __maybe_unused,
                                 unsigned int on __maybe_unused)
{
    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
unsigned int avmnet_vlan_master_reg_read(avmnet_module_t *this,
                                         unsigned int addr,
                                         unsigned int reg)
{
    struct avmnet_vlan_master_context *ctx;
    uint16_t data = 0xFFFF;
    int result;

    ctx = (struct avmnet_vlan_master_context *) this->priv;
    if(unlikely(ctx->master == NULL)){
        AVMNET_ERR("[%s] No master set!\n", __func__);
        dump_stack();
        goto err_out;
    }

    if(in_interrupt()){
        AVMNET_ERR("[%s] in_interrupt()! Abort.\n", __func__);
        dump_stack();
        goto err_out;
    }
    
    result = ctx->master->mdio_read(ctx->master, addr, reg, &data);
    if(result != 0){
        AVMNET_TRC("[%s] mdio_read failed: %d\n", __func__, result);
        data = 0xFFFF;
        goto err_out;
    }

err_out:
    return data;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_vlan_master_reg_write(avmnet_module_t *this,
                                 unsigned int addr,
                                 unsigned int reg,
                                 unsigned int data)
{
    struct avmnet_vlan_master_context *ctx;
    int result;

    ctx = (struct avmnet_vlan_master_context *) this->priv;
    if(unlikely(ctx->master == NULL)){
        AVMNET_ERR("[%s] No master set!\n", __func__);
        dump_stack();
        result = -ENODEV;
        goto err_out;
    }

    if(in_interrupt()){
        AVMNET_ERR("[%s] in_interrupt()! Abort.\n", __func__);
        dump_stack();
        result = -EINVAL;
        goto err_out;
    }

    result = ctx->master->mdio_write(ctx->master, addr, reg, data);
    if(result != 0){
        AVMNET_TRC("[%s] mdio_write failed: %d\n", __func__, result);
        goto err_out;
    }

err_out:
    return result;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_vlan_master_lock(avmnet_module_t *this)
{
    struct avmnet_vlan_master_context *ctx;

    if(in_interrupt()){
        AVMNET_ERR("[%s] in_interrupt()! Abort.\n", __func__);
        dump_stack();
        return -EWOULDBLOCK;
    }

    ctx = (struct avmnet_vlan_master_context *) this->priv;

    return down_interruptible(&(ctx->lock));
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void avmnet_vlan_master_unlock(avmnet_module_t *this)
{
    struct avmnet_vlan_master_context *ctx;

    ctx = (struct avmnet_vlan_master_context *) this->priv;
    up(&(ctx->lock));
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_vlan_master_trylock(avmnet_module_t *this)
{
    struct avmnet_vlan_master_context *ctx;

    ctx = (struct avmnet_vlan_master_context *) this->priv;

    return down_trylock(&(ctx->lock));
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void avmnet_vlan_master_status_changed(avmnet_module_t *this, avmnet_module_t *caller)
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
        AVMNET_ERR("[%s] module %s: received status_changed from unknown module.\n", __func__,
                   this->name);
        return;
    }

    /*
     * handle status change
     */

    return;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static int __maybe_unused get_port_idx(avmnet_device_t *avm_dev)
{
    int idx;

    for(idx = 0; idx < avmnet_hw_config_entry->nr_avm_devices; ++idx){
        if(avm_dev == avmnet_hw_config_entry->avm_devices[idx]){
            break;
        }
    }

    if(idx >= avmnet_hw_config_entry->nr_avm_devices){
        idx = -1;
    }

    return idx;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static void check_port_states(avmnet_module_t *this __maybe_unused)
{
    unsigned int idx;
    avmnet_module_t *phy;
    avmnet_device_t *avm_dev;

    for(idx = 0; idx < avmnet_hw_config_entry->nr_avm_devices; ++idx){
        avm_dev = avmnet_hw_config_entry->avm_devices[idx];

        BUG_ON(avm_dev == NULL);

        if(avm_dev->device == NULL){
            AVMNET_TRC("[%s] avm_device %s has no device\n",
                       __func__, avm_dev->device_name);
            continue;
        }

        mb();
        if(avm_dev->status.Details.powerup == port_open[idx]){
            continue;
        }

        phy = get_phy_by_netdev(avm_dev->device);
        if(phy == NULL){
            AVMNET_ERR("[%s] No PHY module for avm_device %s(%s)\n",
                       __func__, avm_dev->device_name, avm_dev->device->name);
            continue;
        }

        if(port_open[idx] != 0){
            phy->powerup(phy);
        } else {
            phy->powerdown(phy);
        }
    }
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_vlan_master_set_status(avmnet_module_t *this,
                                  avmnet_device_t *device_id,
                                  avmnet_linkstatus_t status)
{
    avmnet_linkstatus_t old;
    struct avmnet_vlan_master_context *ctx;
    int result;

    ctx = (struct avmnet_vlan_master_context *) this->priv;
    result = 0;

    if(device_id == NULL){
        AVMNET_ERR("[%s] received status update for invalid device id\n", __func__);
        result = -EINVAL;
        goto err_out;
    }

    if(ctx->setup_done == 0){
        AVMNET_ERR("[%s] received status before children were set up\n", __func__);
        result = -EINVAL;
        goto err_out;
    }

    old = device_id->status;

    if(old.Status != status.Status){
        AVMNET_INFOTRC("[%s] status change for device %s\n",
                       __func__, device_id->device_name);

        AVMNET_INFOTRC(
                "[%s] old status: powerup: %x link %x flowctrl: %x duplex: %x speed: %x\n",
                __func__, old.Details.powerup, old.Details.link, old.Details.flowcontrol,
                old.Details.fullduplex, old.Details.speed);
        AVMNET_INFOTRC(
                "[%s] new status: powerup: %x link %x flowctrl: %x duplex: %x speed: %x\n",
                __func__, status.Details.powerup, status.Details.link,
                status.Details.flowcontrol, status.Details.fullduplex,
                status.Details.speed);

        device_id->status = status;
        avmnet_links_port_update(device_id);
    }

    if(device_id->device == NULL){
        AVMNET_DEBUG("[%s] No struct net_device for %s\n",
                     __func__, device_id->device_name);
        goto err_out;
    }

    if(status.Details.link){
        if(!netif_carrier_ok(device_id->device)){
            netif_carrier_on(device_id->device);
        }
    }else{
        /*
         * no link. Stop tx queues, update carrier flag and clear MAC table for this
         * port.
         */
        if(netif_carrier_ok(device_id->device)){
            netif_carrier_off(device_id->device);
        }
    }

err_out:
    return result;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_vlan_master_poll(avmnet_module_t *this)
{
    struct avmnet_vlan_master_context *ctx;
    int i, result;

    ctx = (struct avmnet_vlan_master_context *) this->priv;
    result = 0;

    if(ctx->setup_done == 0){
        result = setup_children(this);
        if(result != 0){
            goto err_out;
        }
    }

    /* check if LAN ports need to be powered up or down */
    check_port_states(this);

    for(i = 0; i < this->num_children; ++i){
        if(this->children[i]->poll){
            result = this->children[i]->poll(this->children[i]);
            if(result != 0){
                AVMNET_WARN("Module %s: poll() failed on child %s\n", this->name,
                            this->children[i]->name);
            }
        }
    }

err_out:
    return result;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_vlan_master_powerdown(avmnet_module_t *this __maybe_unused)
{
    AVMNET_DEBUG("[%s] Called.\n", __func__);

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_vlan_master_powerup(avmnet_module_t *this __maybe_unused)
{
    AVMNET_DEBUG("[%s] Called.\n", __func__);

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_vlan_master_suspend(avmnet_module_t *this __maybe_unused,
                               avmnet_module_t *caller __maybe_unused)
{
    AVMNET_DEBUG("[%s] Called.\n", __func__);

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_vlan_master_resume(avmnet_module_t *this __maybe_unused,
                               avmnet_module_t *caller __maybe_unused)
{
    AVMNET_DEBUG("[%s] Called.\n", __func__);

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_vlan_master_reinit(avmnet_module_t *this __maybe_unused)
{
    AVMNET_DEBUG("[%s] Called.\n", __func__);

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static int vlan_device_event(struct notifier_block *unused __maybe_unused,
                             unsigned long event,
                             void *ptr)
{
    struct net_device *dev = ptr;
    unsigned int idx;

    for(idx = 0; idx < avmnet_hw_config_entry->nr_avm_devices; ++idx){
        if(dev == avmnet_hw_config_entry->avm_devices[idx]->device){
            break;
        }
    }

    if(idx >= avmnet_hw_config_entry->nr_avm_devices){
        goto err_out;
    } else {
        AVMNET_TRC("[%s] Got notification for avmnet device %s\n",
                   __func__, avmnet_hw_config_entry->avm_devices[idx]->device->name);
    }

    switch(event){
    case NETDEV_PRE_UP:
        AVMNET_TRC("[%s] vlan port %d going up\n", __func__, idx);
        port_open[idx] = 1;
        mb();
        break;
    case NETDEV_DOWN:
        AVMNET_TRC("[%s] vlan port %d going down\n", __func__, idx);
        port_open[idx] = 0;
        mb();
        break;
    default:
        break;
    }

err_out:
    return NOTIFY_DONE;
}

