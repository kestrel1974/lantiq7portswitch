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

#if !defined(__AVM_NET_CONFIG_)
#define __AVM_NET_CONFIG_

#include <linux/proc_fs.h>
#include <linux/list.h>
#include <linux/netdevice.h>
#include "avmnet_module.h"
#include <linux/ethtool.h>

#define AVMNET_PROC_PREFIX "driver/avmnet"

typedef struct _avmnet_cfg_procentry
{
    struct list_head        entry_list;
    char                    *name;
    struct file_operations  *fops;
    struct proc_dir_entry   *proc_entry;
} avmnet_cfg_procentry_t;


typedef struct _avmnet_cfg_entry
{
    struct list_head        module_list;
    avmnet_module_t         *module;
    int                     id;
    avmnet_device_t         *avm_dev;
    struct proc_dir_entry   *dev_procdir; 
    struct semaphore        proc_mutex;
    struct list_head        proc_entries;
} avmnet_cfg_entry_t;

struct avmnet_hw_config_entry_struct
{
    unsigned int    hw_id;
    unsigned int    hw_sub_id;
    unsigned int    profile_id;
    avmnet_module_t *config;
    int nr_avm_devices;
    avmnet_device_t **avm_devices; 
};

int avmnet_cfg_register_module(avmnet_module_t *module);
int avmnet_cfg_unregister_module(avmnet_module_t *module);
int avmnet_cfg_add_procentry(avmnet_module_t *module, const char *proc_name, const struct file_operations *fops);
int avmnet_cfg_add_seq_procentry(avmnet_module_t *module, const char *proc_name, const struct file_operations *fops);
int avmnet_cfg_add_simple_procentry(avmnet_module_t *module, const char *proc_name, int (*kernel_input)(char *, void *), void (*kernel_output)(struct seq_file *, void *));
int avmnet_cfg_remove_procentry(avmnet_module_t *module, const char *proc_name);
int avmnet_cfg_set_netdev(avmnet_module_t *module, avmnet_device_t *avm_dev);
int avmnet_cfg_unset_netdev(avmnet_module_t *module);
avmnet_device_t *avmnet_cfg_get_netdev(avmnet_module_t *module);
avmnet_module_t *avmnet_cfg_get_module(struct net_device *device);
avmnet_module_t *get_mac_by_netdev(const struct net_device *dev);
avmnet_module_t *get_phy_by_netdev(const struct net_device *dev);
avmnet_module_t *get_switch_by_netdev(const struct net_device *dev);
avmnet_device_t *get_avmdev_by_name(const char *dev_name);
int avmnet_netdev_open(struct net_device *dev);
int avmnet_netdev_stop(struct net_device *dev);
int avmnet_create_netdevices(void);
void avmnet_destroy_netdevice(avmnet_device_t *avm_dev);
struct workqueue_struct *avmnet_get_workqueue(void);

extern struct avmnet_hw_config_entry_struct *avmnet_hw_config_entry;

#endif // __AVM_NET_CONFIG_
