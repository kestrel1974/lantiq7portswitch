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

#ifndef DRIVERS_NET_AVM_CPMAC_SWITCH_VLAN_MASTER_VLAN_MASTER_H_
#define DRIVERS_NET_AVM_CPMAC_SWITCH_VLAN_MASTER_VLAN_MASTER_H_

#include <avmnet_module.h>
#include <avm/net/net.h>

struct avmnet_vlan_master_context
{
    struct avmnet_master_dev *master;
    unsigned int setup_done;
    struct semaphore lock;
    int num_avm_devices;
    avmnet_device_t **avm_devices;
    char oldname[IFNAMSIZ];
};

extern int avmnet_vlan_master_init(avmnet_module_t *this);
extern int avmnet_vlan_master_setup(avmnet_module_t *this);
extern int avmnet_vlan_master_exit(avmnet_module_t *this);
extern int avmnet_vlan_master_setup_irq(avmnet_module_t *this , unsigned int on);
extern unsigned int avmnet_vlan_master_reg_read(avmnet_module_t *this, unsigned int addr, unsigned int reg);
extern int avmnet_vlan_master_reg_write(avmnet_module_t *this, unsigned int addr, unsigned int reg, unsigned int data);
extern int avmnet_vlan_master_lock(avmnet_module_t *this);
extern void avmnet_vlan_master_unlock(avmnet_module_t *this);
extern int avmnet_vlan_master_trylock(avmnet_module_t *this);
extern void avmnet_vlan_master_status_changed(avmnet_module_t *this, avmnet_module_t *caller);
extern int avmnet_vlan_master_set_status(avmnet_module_t *this, avmnet_device_t *device_id, avmnet_linkstatus_t status);
extern int avmnet_vlan_master_poll(avmnet_module_t *this);
extern int avmnet_vlan_master_powerdown(avmnet_module_t *this);
extern int avmnet_vlan_master_powerup(avmnet_module_t *this);
extern int avmnet_vlan_master_suspend(avmnet_module_t *this, avmnet_module_t *caller);
extern int avmnet_vlan_master_resume(avmnet_module_t *this, avmnet_module_t *caller);
extern int avmnet_vlan_master_reinit(avmnet_module_t *this);

#endif /* DRIVERS_NET_AVM_CPMAC_SWITCH_VLAN_MASTER_VLAN_MASTER_H_ */
