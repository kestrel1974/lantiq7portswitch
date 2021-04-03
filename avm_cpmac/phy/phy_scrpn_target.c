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

#include <avmnet_debug.h>
#include <avmnet_module.h>
#include <avmnet_config.h>
#include "phy_scrpn_target.h"

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_phy_scrpn_tgt_init(avmnet_module_t *this) {
    AVMNET_INFO("[%s] Init on module %s called.\n", __func__, this->name);
    /* We do not need to store additional data */
    this->priv = NULL;

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_phy_scrpn_tgt_setup(avmnet_module_t *this) {
    int result;
    AVMNET_INFO("[%s] Setup on module %s called.\n", __func__, this->name);

    /*
     * do cunning setup-stuff
     */
    result = avmnet_cfg_register_module(this);
    if(result < 0){
        AVMNET_ERR("[%s] avmnet_cfg_register_module failed for module %s\n", __func__, this->name);
        return result;
    }
    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_phy_scrpn_tgt_exit(avmnet_module_t *this) {
    AVMNET_INFO("[%s] Init on module %s called.\n", __func__, this->name);

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_phy_scrpn_tgt_setup_irq(avmnet_module_t *this __attribute__ ((unused)),
                                unsigned int on __attribute__ ((unused))) {
    /* No IRQ available */
    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
unsigned int avmnet_phy_scrpn_tgt_reg_read(avmnet_module_t *this, unsigned int addr, unsigned int reg) {
    return this->parent->reg_read(this->parent, addr, reg);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_phy_scrpn_tgt_reg_write(avmnet_module_t *this, unsigned int addr, unsigned int reg, unsigned int val) {
    return this->parent->reg_write(this->parent, addr, reg, val);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_phy_scrpn_tgt_lock(avmnet_module_t *this) {
    return this->parent->lock(this->parent);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void avmnet_phy_scrpn_tgt_unlock(avmnet_module_t *this) {
    this->parent->unlock(this->parent);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_phy_scrpn_tgt_trylock(avmnet_module_t *this) {
    return this->parent->trylock(this->parent);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void avmnet_phy_scrpn_tgt_status_changed(avmnet_module_t *this, avmnet_module_t *caller) {
    /* We have no childs */
    return;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_phy_scrpn_tgt_poll(avmnet_module_t *this) {
    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_phy_scrpn_tgt_set_status(avmnet_module_t *this, avmnet_device_t *device_id,
                                avmnet_linkstatus_t status) {
    return this->parent->set_status(this->parent, device_id, status);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_phy_scrpn_tgt_powerdown(avmnet_module_t *this) {
    avmnet_linkstatus_t status;

    AVMNET_INFO("[%s] Powerdown on module %s called.\n", __func__, this->name);

    status.Status = 0;
    this->parent->set_status(this->parent, this->device_id, status);

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_phy_scrpn_tgt_powerup(avmnet_module_t *this) {
    avmnet_linkstatus_t status;

    AVMNET_INFO("[%s] Powerup on module %s called.\n", __func__, this->name);

    status.Details.powerup = 1;
    status.Details.link = 1;
    status.Details.fullduplex = 1;
    status.Details.speed = AVMNET_LINKSTATUS_SPD_1000;

    this->parent->set_status(this->parent, this->device_id, status);

    return 0;
}