/*------------------------------------------------------------------------------------------*\
 *   Copyright (C) 2011,...,2014 AVM GmbH <fritzbox_info@avm.de>
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

#include <linux/list.h>
#include <linux/string.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/proc_fs.h>
#include <linux/avm_hw_config.h>
#include <linux/env.h>
#include <linux/ethtool.h>
#include <linux/moduleparam.h>
#include <linux/limits.h>
#if __has_include(<avm/sammel/simple_proc.h>)
#include <avm/sammel/simple_proc.h>
#else
#include <linux/simple_proc.h>
#endif
#include <linux/vmalloc.h>
#include <linux/rtnetlink.h>
#include <linux/if_vlan.h>
#include <linux/sched.h>

#ifdef CONFIG_AVM_PA
#if __has_include(<avm/pa/pa.h>)
#include <avm/pa/pa.h>
#else
#include <linux/avm_pa.h>
#endif
#if __has_include(<avm/pa/hw.h>)
#include <avm/pa/hw.h>
#else
#include <linux/avm_pa_hw.h>
#endif
#endif


#include <asm/prom.h>
#include <avmnet_debug.h>
#include <avmnet_module.h>
#include <avmnet_common.h>
#include <avmnet_config.h>
#include "../configs/avmnet_hw_config.h"
#include "../management/avmnet_mgmt.h"
#include <avmnet_multicast.h>


MODULE_AUTHOR("Maintainer: AVM GmbH");
MODULE_DESCRIPTION("Central configuration Module for AVM network infrastructure");
MODULE_LICENSE("GPL v2");

struct list_head module_list;
struct avmnet_master_dev *vlan_master = NULL;

DEFINE_SEMAPHORE(module_cfg_sema);

static void do_avmnet_heartbeat(struct work_struct *work);
DECLARE_DELAYED_WORK(avmnet_heartbeat, do_avmnet_heartbeat);

static avmnet_device_t *link_dev_to_offload_cpu = NULL;

static int no_avmnet = 0;
module_param(no_avmnet , int, S_IRUSR | S_IWUSR);

struct proc_dir_entry *avmnet_procdir;
struct avmnet_hw_config_entry_struct *avmnet_hw_config_entry = NULL;
EXPORT_SYMBOL( avmnet_hw_config_entry );

static avmnet_cfg_entry_t *get_entry_by_name(const char *);
static avmnet_cfg_entry_t *get_entry_by_id(int);
static avmnet_cfg_entry_t *get_entry_by_netdev(const struct net_device *); 
static avmnet_cfg_procentry_t *get_procentry(int, const char *);
static int create_cfg_procentry(const avmnet_cfg_entry_t *cfg_entry, avmnet_cfg_procentry_t *proc_entry, avmnet_module_t *module);
static int remove_cfg_procentry(const avmnet_cfg_entry_t *, avmnet_cfg_procentry_t *);
static int avmnet_create_netdevice(avmnet_device_t *avm_dev);
static void do_avmnet_heartbeat(struct work_struct *work);

/*------------------------------------------------------------------------------------------*\
 * Forward declarations for ethtool wrapper functions
\*------------------------------------------------------------------------------------------*/
static int assemble_ethops(avmnet_module_t *module, avmnet_ethtool_table_t *table, struct ethtool_ops *eth_ops);
static void avmnet_eth_get_drvinfo(struct net_device *netdev, struct ethtool_drvinfo *drvinfo);
static int avmnet_eth_get_regs_len(struct net_device *netdev);
static void avmnet_eth_get_regs(struct net_device *netdev, struct ethtool_regs *regs, void *data);
static void avmnet_eth_get_wol(struct net_device *netdev, struct ethtool_wolinfo *wolinfo);
static int avmnet_eth_set_wol(struct net_device *netdev, struct ethtool_wolinfo *wolinfo);
static u32 avmnet_eth_get_msglevel(struct net_device *netdev);
static void avmnet_eth_set_msglevel(struct net_device *netdev, u32 level);
static int avmnet_eth_nway_reset(struct net_device *netdev);
static u32 avmnet_eth_get_link(struct net_device *netdev);
static int avmnet_eth_get_eeprom_len(struct net_device *netdev);
static int avmnet_eth_get_eeprom(struct net_device *netdev, struct ethtool_eeprom *eeprom, u8 *data);
static int avmnet_eth_set_eeprom(struct net_device *netdev, struct ethtool_eeprom *eeprom, u8 *data);
static int avmnet_eth_get_coalesce(struct net_device *netdev, struct ethtool_coalesce *coalesce);
static int avmnet_eth_set_coalesce(struct net_device *netdev, struct ethtool_coalesce *coalesce);
static void avmnet_eth_get_ringparam(struct net_device *netdev, struct ethtool_ringparam *ringparam);
static int avmnet_eth_set_ringparam(struct net_device *netdev, struct ethtool_ringparam *ringparam);
static void avmnet_eth_get_pauseparam(struct net_device *netdev, struct ethtool_pauseparam *param);
static int avmnet_eth_set_pauseparam(struct net_device *netdev, struct ethtool_pauseparam *param);
static void avmnet_eth_self_test(struct net_device *netdev, struct ethtool_test *test, u64 *data);
static void avmnet_eth_get_strings(struct net_device *netdev, u32 stringset, u8 *data);
static void avmnet_eth_get_ethtool_stats(struct net_device *netdev, struct ethtool_stats *stats, u64 *data);
static int avmnet_eth_begin(struct net_device *netdev);
static void avmnet_eth_complete(struct net_device *netdev);
static u32 avmnet_eth_get_priv_flags(struct net_device *netdev);
static int avmnet_eth_set_priv_flags(struct net_device *netdev, u32 flags);
static int avmnet_eth_get_sset_count(struct net_device *netdev, int count);
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 2, 0)
static int avmnet_eth_get_rxnfc(struct net_device *, 
                                struct ethtool_rxnfc *, void *)
           __attribute__((alias("_avmnet_eth_get_rxnfc_compat")));
#else
static int avmnet_eth_get_rxnfc(struct net_device *, struct ethtool_rxnfc *,
                                u32 *)
           __attribute__((alias("_avmnet_eth_get_rxnfc_compat")));
#endif
static int avmnet_eth_set_rxnfc(struct net_device *netdev, struct ethtool_rxnfc *rxnfc);
static int avmnet_eth_flash_device(struct net_device *netdev, struct ethtool_flash *flash);
static int avmnet_eth_get_settings(struct net_device *netdev, struct ethtool_cmd *cmd);
static int avmnet_eth_set_settings(struct net_device *netdev, struct ethtool_cmd *cmd);
void avmnet_wait_for_link_to_offload_cpu(void);


// get profile ID from kernel command line
int avmnet_profile = 0;
int __init avmnet_profile_setup(char *str)
{
    if(get_option(&str, &avmnet_profile)){
        return 1;
    }

    return 0;
}
__setup("avmnet_profile=", avmnet_profile_setup);

static struct workqueue_struct *global_workqueue;

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static int avmnet_proc_mdio_open(struct inode *inode, struct file *file)
{
    file->private_data = PDE_DATA(inode);

    return 0;
}

static ssize_t avmnet_proc_mdio(struct file *file, const char __user *buf,
                                size_t count,
                                loff_t *offset __attribute__((unused)))
{
    static char cmd[256];
    char *token[4];
    unsigned int number_of_tokens = 0;
    int len;
    unsigned int phy, reg, value;
    avmnet_module_t *this = (avmnet_module_t *) file->private_data;

    len = (sizeof(cmd) - 1) < count ? (sizeof(cmd) - 1) : count;
    len = len - copy_from_user(cmd, buf, len);
    cmd[len] = 0;

    number_of_tokens = avmnet_tokenize(cmd, " ", 4, token);
    if(   (number_of_tokens == 3)
       && !strncasecmp(token[0], "read", 4)) {
        if(this->reg_read == NULL) {
            AVMNET_WARN("[%s] No mdio read function available!\n", __func__);
            return count;
        }
        sscanf(token[1], "%x", &phy);
        sscanf(token[2], "%x", &reg);
        value = this->reg_read(this, phy, reg);
        AVMNET_SUPPORT("mdio_read phy %u: reg %#x = %#x\n", phy, reg, value);
    } else if(   (number_of_tokens == 4)
              && !strncasecmp(token[0], "write", 5)) {
        if(this->reg_write == NULL) {
            AVMNET_WARN("[%s] No mdio write function available!\n", __func__);
            return count;
        }
        sscanf(token[1], "%x", &phy);
        sscanf(token[2], "%x", &reg);
        sscanf(token[3], "%x", &value);
        this->reg_write(this, phy, reg, value);
        AVMNET_SUPPORT("mdio_write phy %u: reg %#x = %#x\n", phy, reg, value);
    } else {
        AVMNET_SUPPORT("No command recognized. These are possible:\n");
        AVMNET_SUPPORT("   read <phynr> <reg>\n");
        AVMNET_SUPPORT("   write <phynr> <reg> <value>\n");
    }

    return count;
}

static const struct file_operations avmnet_proc_mdio_fops = {
    .open = avmnet_proc_mdio_open,
    .write = avmnet_proc_mdio,
};


/*------------------------------------------------------------------------------------------*\
 * create top level proc directory for avmnet 
\*------------------------------------------------------------------------------------------*/
static int init_avmnet_procfs(void)
{
    AVMNET_INFO("%s: creating proc dir\n", __func__);

    avmnet_procdir = proc_mkdir(AVMNET_PROC_PREFIX, NULL);
    if(avmnet_procdir == NULL){
        AVMNET_ERR("%s: failed to create avmnet directory in /proc\n", __func__);
        return -1;
    }

    return 0;
}

/*------------------------------------------------------------------------------------------*\
 * register an initialised module and create a directory in our proc dir
\*------------------------------------------------------------------------------------------*/
int avmnet_cfg_register_module(avmnet_module_t *module)
{
    static int id = 0;
    int result = -1;
    avmnet_cfg_entry_t *entry;

    if(module == NULL){
        return -EINVAL;
    }

    // allocate new entry struct and create proc dir entry 
    entry = kzalloc(sizeof(avmnet_cfg_entry_t), GFP_KERNEL);
    if(entry == NULL){
        return -ENOMEM;
    }

    AVMNET_INFO("%s: registering new module %s\n", __func__, module->name);

    entry->dev_procdir = proc_mkdir(module->name, avmnet_procdir);
    AVMNET_INFOTRC("[%s] proc_mkdir: %s parent = %p, res=%p\n", __func__, module->name, avmnet_procdir, entry->dev_procdir );

    if(entry->dev_procdir == NULL){
        AVMNET_ERR("%s: failed to create procfs-entry for device %s\n", __func__, module->name);
        kfree(entry);
        return -ENODEV;
    }

    // add entry to list of registered modules
    entry->id = id;
    entry->module = module;
    INIT_LIST_HEAD(&(entry->proc_entries));
    sema_init(&(entry->proc_mutex), 1);

    if (!down_interruptible(&module_cfg_sema)) {
        list_add_tail(&entry->module_list, &module_list);
        up(&module_cfg_sema);

        ++id;
        result = entry->id;
    }else{
        AVMNET_ERR("%s: interrupted while registering cfg module %s\n", __func__, entry->module->name);

        remove_proc_entry(module->name, avmnet_procdir);
        kfree(entry);

        result = -EINTR;
    }

    if((module->reg_read != NULL) || (module->reg_write != NULL)) {
        avmnet_cfg_add_procentry(module, "mdio", &avmnet_proc_mdio_fops);
    }

    return result;
}
EXPORT_SYMBOL(avmnet_cfg_register_module);


/*------------------------------------------------------------------------------------------*\
 * unregister module
\*------------------------------------------------------------------------------------------*/
int avmnet_cfg_unregister_module(avmnet_module_t *module)
{
    int result = -ENODEV;
    avmnet_cfg_entry_t *cfg_entry;
    avmnet_cfg_procentry_t *proc_entry, *proc_save;

    cfg_entry = get_entry_by_name(module->name);

    // remove module from module list
    if(cfg_entry != NULL){
        if(!down_interruptible(&module_cfg_sema)) {
            list_del(&(cfg_entry->module_list));
            up(&module_cfg_sema);

            // erase this module's proc files and directory
            if(!down_interruptible(&(cfg_entry->proc_mutex))){
                list_for_each_entry_safe(proc_entry, proc_save, &(cfg_entry->proc_entries), entry_list){
                    remove_proc_entry(proc_entry->name, cfg_entry->dev_procdir);
                    list_del(&(proc_entry->entry_list));
                    kfree(proc_entry->name);
                    kfree(proc_entry);
                }
                up(&(cfg_entry->proc_mutex));

                remove_proc_entry(cfg_entry->module->name, avmnet_procdir);

                kfree(cfg_entry);

                result = 0;
            }else{
                result = -EINTR;
            }
        }else{
            result = -EINTR;
        }
    }

    return result;
}
EXPORT_SYMBOL(avmnet_cfg_unregister_module);

/*------------------------------------------------------------------------------------------*\
 * add a new proc entry inside a module's directory
 *
\*------------------------------------------------------------------------------------------*/
static int add_procentry(avmnet_module_t *module, const char *proc_name,
                         const struct file_operations *fops)
{
    avmnet_cfg_entry_t *cfg_entry;
    avmnet_cfg_procentry_t *proc_entry;
    int result;

    cfg_entry = get_entry_by_name(module->name);
    if(cfg_entry == NULL){
        return -ENODEV;
    }

    if(get_procentry(cfg_entry->id, proc_name) != NULL){
        return -EEXIST;
    }

    // allocate and initialise new procentry struct
    proc_entry = kzalloc(sizeof(avmnet_cfg_procentry_t), GFP_KERNEL);
    if(proc_entry == NULL){
        return -ENOMEM;
    }

    proc_entry->name = kstrdup(proc_name, GFP_KERNEL);
    if(proc_entry->name == NULL){
        kfree(proc_entry);
        return -ENOMEM;
    }

    INIT_LIST_HEAD(&(proc_entry->entry_list));
    proc_entry->fops = (struct file_operations *) fops;

    // add entry to module's list of entries
    if(!down_interruptible(&(cfg_entry->proc_mutex))){
        list_add_tail(&(proc_entry->entry_list), &(cfg_entry->proc_entries));
        up(&(cfg_entry->proc_mutex));
    }else{
        kfree(proc_entry->name);
        kfree(proc_entry);
        return -1;
    }

    result = create_cfg_procentry(cfg_entry, proc_entry, module);

    // clean up if creating the proc entry failed
    if(result < 0){
        if(!down_interruptible(&(cfg_entry->proc_mutex))){
            list_del(&(proc_entry->entry_list));
            up(&(cfg_entry->proc_mutex));

            kfree(proc_entry->name);
            kfree(proc_entry);
        }else{
            AVMNET_ERR("%s: interrupted during clean-up\n", __func__);
        }
    }

    return result;
}

/*------------------------------------------------------------------------------------------*\
 * add a new proc entry inside a module's directory
\*------------------------------------------------------------------------------------------*/
int avmnet_cfg_add_procentry(avmnet_module_t *module, const char *proc_name,
                             const struct file_operations *fops)
{

    if(!proc_name || !fops)
        return -EINVAL;

    return add_procentry(module, proc_name, fops);
}
EXPORT_SYMBOL(avmnet_cfg_add_procentry);

/*------------------------------------------------------------------------------------------*\
 * add a new proc entry with sequential access interface inside a module's directory
\*------------------------------------------------------------------------------------------*/
int avmnet_cfg_add_simple_procentry(
      avmnet_module_t *module,
      const char *proc_name,
      int (*kernel_input)(char *, void *),
      void (*kernel_output)(struct seq_file *, void *) ){
   int res;
   char *path;
   path = vmalloc(PATH_MAX);
   if (!path)
      return -ENOMEM;

   if(proc_name == NULL || module == NULL){
      vfree(path);
      return -EINVAL;
   }
   snprintf(path, PATH_MAX, "%s/%s/%s", AVMNET_PROC_PREFIX, module->name, proc_name );

   res = add_simple_proc_file( path, kernel_input, kernel_output, module );
   vfree(path);
   return res;

}
EXPORT_SYMBOL(avmnet_cfg_add_simple_procentry);


/*------------------------------------------------------------------------------------------*\
 * add a new proc entry with sequential access interface inside a module's directory
\*------------------------------------------------------------------------------------------*/
int avmnet_cfg_add_seq_procentry(avmnet_module_t *module, const char *proc_name,
                                 const struct file_operations *fops)
{

    if(proc_name == NULL || fops == NULL){
        return -EINVAL;
    }

    return add_procentry(module, proc_name, fops);
}
EXPORT_SYMBOL(avmnet_cfg_add_seq_procentry);

/*------------------------------------------------------------------------------------------*\
 * remove a proc entry file
\*------------------------------------------------------------------------------------------*/
int avmnet_cfg_remove_procentry(avmnet_module_t *module, const char *proc_name)
{
    avmnet_cfg_entry_t *cfg_entry;
    avmnet_cfg_procentry_t *proc_entry;
    int result;

    if(proc_name == NULL){
        return -EINVAL;
    }

    cfg_entry = get_entry_by_name(module->name);
    if(cfg_entry == NULL){
        return -ENODEV;
    }

    proc_entry = get_procentry(cfg_entry->id, proc_name);
    if(proc_entry == NULL){
        return -ENOENT;
    }

    // remove the file from the procfs, remove the struct from the module's entry list
    result = remove_cfg_procentry(cfg_entry, proc_entry);

    if(!down_interruptible(&(cfg_entry->proc_mutex))){
        list_del(&(proc_entry->entry_list));
        up(&(cfg_entry->proc_mutex));

        kfree(proc_entry->name);
        kfree(proc_entry);
    }else{
        AVMNET_ERR("%s: interrupted during clean up\n", __func__);
        result = -EINTR;
    }


    return result;
}
EXPORT_SYMBOL(avmnet_cfg_remove_procentry);

/*------------------------------------------------------------------------------------------*\
 * Obsolete?
\*------------------------------------------------------------------------------------------*/
int avmnet_cfg_set_netdev(avmnet_module_t *module, avmnet_device_t *avm_dev)
{
    avmnet_cfg_entry_t *cfg_entry;

    if(module == NULL){
        return -EINVAL;
    }

    cfg_entry = get_entry_by_name(module->name);
    if(cfg_entry == NULL){
        return -ENODEV;
    }

    if(cfg_entry->avm_dev != NULL){
        return -EEXIST;
    }

    cfg_entry->avm_dev = avm_dev;

    return 0;
}
EXPORT_SYMBOL(avmnet_cfg_set_netdev);

/*------------------------------------------------------------------------------------------*\
 * Obsolete?
\*------------------------------------------------------------------------------------------*/
int avmnet_cfg_unset_netdev(avmnet_module_t *module)
{
    avmnet_cfg_entry_t *cfg_entry;

    if(module == NULL){
        return -EINVAL;
    }

    cfg_entry = get_entry_by_name(module->name);
    if(cfg_entry == NULL){
        return -ENODEV;
    }

    cfg_entry->avm_dev = NULL;

    return 0;
}
EXPORT_SYMBOL(avmnet_cfg_unset_netdev);

/*------------------------------------------------------------------------------------------*\
 * Obsolete?
\*------------------------------------------------------------------------------------------*/
avmnet_device_t * avmnet_cfg_get_netdev(avmnet_module_t *module)
{
    avmnet_cfg_entry_t *cfg_entry;

    if(module == NULL){
        AVMNET_ERR("%s: called with invalid module pointer.\n", __func__);
        return NULL;
    }

    cfg_entry = get_entry_by_name(module->name);
    if(cfg_entry == NULL){
        return NULL;
    }

    return cfg_entry->avm_dev;
}
EXPORT_SYMBOL(avmnet_cfg_get_netdev);

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
avmnet_module_t *avmnet_cfg_get_module(struct net_device *device) {
    avmnet_cfg_entry_t *cfg_entry = get_entry_by_netdev(device);

    if ( ! cfg_entry) {
        return NULL;
    }

    return cfg_entry->module;

}
EXPORT_SYMBOL(avmnet_cfg_get_module);


/*------------------------------------------------------------------------------------------*\
 * delete a procfs-entry. 
\*------------------------------------------------------------------------------------------*/
static int remove_cfg_procentry(const avmnet_cfg_entry_t *cfg_entry, avmnet_cfg_procentry_t *proc_entry)
{
    remove_proc_entry(proc_entry->name, cfg_entry->dev_procdir);
    proc_entry->proc_entry = NULL;

    return 0;
}

/*------------------------------------------------------------------------------------------*\
 * create a file entry in the procfs and register the read/write functions
\*------------------------------------------------------------------------------------------*/
static int create_cfg_procentry(const avmnet_cfg_entry_t *cfg_entry, 
                                avmnet_cfg_procentry_t *proc_entry,
                                avmnet_module_t *module)
{
    proc_entry->proc_entry = proc_create_data(proc_entry->name,
                                              S_IRUGO | S_IWUSR,
                                              cfg_entry->dev_procdir,
                                              proc_entry->fops, module);

    AVMNET_TRC("[%s] %s parent = %p, res=%p\n", __func__, proc_entry->name, cfg_entry, proc_entry->proc_entry);
    if(proc_entry->proc_entry == NULL){
        AVMNET_ERR("%s: failed to create proc entry %s/%s\n", __func__, cfg_entry->module->name,
                   proc_entry->name);
        return -1;
    }

    return 0;
}

/*------------------------------------------------------------------------------------------*\
 * find a modules procentry struct by name
\*------------------------------------------------------------------------------------------*/
static avmnet_cfg_procentry_t* get_procentry(int cfg_id, const char *name)
{
    avmnet_cfg_entry_t *cfg_entry;
    avmnet_cfg_procentry_t *proc_entry, *result = NULL;

    cfg_entry = get_entry_by_id(cfg_id);

    if(cfg_entry == NULL){
        return NULL;
    }

    if(!down_interruptible(&(cfg_entry->proc_mutex))){
        list_for_each_entry(proc_entry, &(cfg_entry->proc_entries), entry_list){
            if(!strcmp(name, proc_entry->name)){
                result = proc_entry;
                break;
            }
        }
        up(&(cfg_entry->proc_mutex));
    }

    return result;
}

/*------------------------------------------------------------------------------------------*\
 * search for a module by name
\*------------------------------------------------------------------------------------------*/
static avmnet_cfg_entry_t* get_entry_by_name(const char *name)
{
    avmnet_cfg_entry_t *entry, *cfg_entry = NULL;

    if(name != NULL){
        if (!down_interruptible(&module_cfg_sema)) {
            list_for_each_entry(entry, &module_list, module_list){
                if(!strcmp(name, entry->module->name)){
                    cfg_entry = entry;
                    break;
                }
            }
            up(&module_cfg_sema);
        }
    }

    return cfg_entry;
}

/*------------------------------------------------------------------------------------------*\
 * search for a module by ID
\*------------------------------------------------------------------------------------------*/
static avmnet_cfg_entry_t* get_entry_by_id(int id)
{
    avmnet_cfg_entry_t *entry, *cfg_entry = NULL;

    if (!down_interruptible(&module_cfg_sema)) {
        list_for_each_entry(entry, &module_list, module_list){
            if(entry->id == id){
                cfg_entry = entry;
                break;
            }
        }
        up(&module_cfg_sema);
    }

    return cfg_entry;
}

/*------------------------------------------------------------------------------------------*\
 * search for a module by net_device
\*------------------------------------------------------------------------------------------*/
static avmnet_cfg_entry_t* get_entry_by_netdev(const struct net_device *netdev) 
{
    avmnet_cfg_entry_t *entry, *cfg_entry = NULL;
    avmnet_device_t *avmnet_device;

    if ( ! netdev)
        return cfg_entry;

    if (!down_interruptible(&module_cfg_sema)) {
        list_for_each_entry(entry, &module_list, module_list){
            avmnet_device = entry->module->device_id;
            if (avmnet_device) {
                if( ! strcmp(avmnet_device->device->name, netdev->name)) {
                    cfg_entry = entry;
                    break;
                }
            }
        }
        up(&module_cfg_sema);
    }

    return cfg_entry;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static int get_config(void)
{
    unsigned int hwrev, hwsubrev, i;
    struct avmnet_hw_config_entry_struct *cfg_base, *cfg_sub_base;
    char *s;

    // stolen from avm_hw_config.c
    s = prom_getenv("HWRevision");
    if(s){
        hwrev = simple_strtoul(s, NULL, 10);
    }     

    if((s == NULL)) {
        AVMNET_ERR("[%s] error: %s HWRevision detected in environment variables: %s\n", __func__, (s ? "invalid" : "no"), s);
        BUG_ON(1);
        return -1;
    }

    s = prom_getenv("HWSubRevision");
    if(s){
        hwsubrev = simple_strtoul(s, NULL, 10);
    } else {
        hwsubrev = 0;
    }

    if(hwsubrev > 255) {
        AVMNET_ERR("[%s] error: %s HWSubRevision detected in environment variables: %s\n", __func__, (s ? "invalid" : "no"), s);
        BUG_ON(1);
        return -1;
    }

#if 0
    s = prom_getenv("AVMNetProfile");
    if (s) {
        profile = simple_strtoul(s, NULL, 10);
    } else {
        profile = 0;
    }

    if(profile > 255){
        AVMNET_ERR("[%s] Ignoring illegal profile number: %d\n", __func__, profile);
        profile = 0;
    }
#endif

#if (defined(CONFIG_MACH_ATHEROS) && defined(CONFIG_MACH_QCA953x)) || (defined(CONFIG_ATH79) && defined(CONFIG_SOC_QCA953X))
    /*-----------------------------------------------------------------------------------------------*\
     * eine Variante, um in der Produktion Ethernet auf der Prozessorplatine an den Start zu bekommen
    \*-----------------------------------------------------------------------------------------------*/
    {
        char *p = prom_getenv("ptest");
        if (p) {
            size_t len = strlen(p);
            char *q = strstr(p, "eth_sa=");
            if (q && (((q - p) + sizeof("eth_sa=")) <= len)) {
                if (q[7] == '1')
                    hwsubrev = WLAN_PROD_TEST;
                AVMNET_ERR("[%s] eth_sa=%c hwsubrev %d\n", __func__, q[7], hwsubrev);
            }
        }
    }
#endif /*--- #if (defined(CONFIG_MACH_ATHEROS) && defined(CONFIG_MACH_QCA953x)) || (defined(CONFIG_ATH79) && defined(CONFIG_SOC_QCA953X)) ---*/

    if(avmnet_profile < 0 || avmnet_profile > 255){
        AVMNET_ERR("[%s] Ignoring illegal profile number: %d\n", __func__, avmnet_profile);
        avmnet_profile = 0;
    }

    AVMNET_INFO("[%s] Looking for HWRev %d, HWSubRev %d, profile ID %d\n", __func__, hwrev, hwsubrev, avmnet_profile);
    AVMNET_INFO("[%s] Entries in avmnet_hw config_table: %d\n", __func__, AVMNET_HW_CONFIG_TABLE_SIZE);

    /*
     * find the best configuration match for this box.
     * Ideally, we will find a configuration that matches the hardware revision, sub-revision
     * and profile ID.
     */
    cfg_base = NULL;
    cfg_sub_base = NULL;
    avmnet_hw_config_entry = NULL;
    for(i = 0; i < AVMNET_HW_CONFIG_TABLE_SIZE; ++i){
        if(avmnet_hw_config_table[i].hw_id == hwrev){

            if(   avmnet_hw_config_table[i].hw_sub_id == 0
               && avmnet_hw_config_table[i].profile_id == 0)
            {
                // this configuration is the absolute base profile for the hardware revision
                cfg_base = &avmnet_hw_config_table[i];
            }

            if(   avmnet_hw_config_table[i].hw_sub_id == hwsubrev
               && avmnet_hw_config_table[i].profile_id == 0)
            {
                // this configuration is the base profile for a hardware sub revision
                cfg_sub_base = &avmnet_hw_config_table[i];
            }

            if(   avmnet_hw_config_table[i].hw_sub_id == hwsubrev
               && avmnet_hw_config_table[i].profile_id == (unsigned int) avmnet_profile)
            {
                // found an exact match, stop searching
                avmnet_hw_config_entry = &avmnet_hw_config_table[i];
                break;
            }
        }
    }

    if(avmnet_hw_config_entry == NULL){
        // no exact match found, try sub revision
        AVMNET_ERR("No config found for HWRev %d, HWSubRev %d, Profile-ID %d, trying base config for HWSubRev\n", hwrev, hwsubrev, avmnet_profile);
        avmnet_hw_config_entry = cfg_sub_base;
    }

    if(avmnet_hw_config_entry == NULL){
        // still no match, try base hw revision
        AVMNET_ERR("No config found for HWRev %d, HWSubRev %d, trying base config for HWRev\n", hwrev, hwsubrev);
        avmnet_hw_config_entry = cfg_base;
    }

    if(avmnet_hw_config_entry != NULL){
        // we have found a valid configuration
        return 0;
    }

    AVMNET_ERR("*****************************************************************************\n");
    AVMNET_ERR("*****************************************************************************\n");
    AVMNET_ERR("*****************************************************************************\n");
    AVMNET_ERR("***                                                                       ***\n");
    AVMNET_ERR("***                                                                       ***\n");
    AVMNET_ERR("***                                                                       ***\n");
    AVMNET_ERR("*** ATTENTION! There is no valid ethernet configuration for this product! ***\n");
    AVMNET_ERR("***                                                                       ***\n");
    AVMNET_ERR("***                                                                       ***\n");
    AVMNET_ERR("***                                                                       ***\n");
    AVMNET_ERR("*****************************************************************************\n");
    AVMNET_ERR("*****************************************************************************\n");
    AVMNET_ERR("*****************************************************************************\n");

    return -1;
}


/*------------------------------------------------------------------------------------------*\
 * initialise the config module
\*------------------------------------------------------------------------------------------*/
static int avmnet_cfg_init(void)
{

    if (no_avmnet){
        pr_err("Skipping AVMNET arch_initcall due to module variable\n");
        return 0;
    }

    AVMNET_SUPPORT("[%s] Driver version: %s\n", __func__, avmnet_version);

    INIT_LIST_HEAD(&module_list);

    if(init_avmnet_procfs() < 0){
        printk(KERN_ERR "{%s} exit init_avmnet_procfs\n", __func__);
        return -1;
    }

    AVMNET_INFO("[%s] done\n", __func__ );
    return 0;
}
arch_initcall(avmnet_cfg_init);

/*------------------------------------------------------------------------------------------*\
 * Funktion kann mehrmals aufgerufen werden, es werden Devices angelegt f�r die gilt:
 * !AVMNET_DEVICE_FLAG_WAIT_FOR_MODULE_FUNCTIONS && !AVMNET_DEVICE_FLAG_INITIALIZED
\*------------------------------------------------------------------------------------------*/
int avmnet_create_netdevices(void) {
    avmnet_device_t *device;
    int i;
    int ret = 0;

    AVMNET_INFO("[%s]\n", __func__);

    for (i = 0; i < avmnet_hw_config_entry->nr_avm_devices; i++){

        device = avmnet_hw_config_entry->avm_devices[i];

        if ( device->flags & AVMNET_DEVICE_FLAG_WAIT_FOR_MODULE_FUNCTIONS ) {
            AVMNET_INFO("[%s] FLAG_WAIT_FOR_MODULE_FUNCTIONS: skipping create_netdevice %s \n",
                        __func__, device->device_name );
            continue;
        }

        if ( device->flags & AVMNET_DEVICE_FLAG_INITIALIZED ) {
            AVMNET_INFO("[%s] skipping create_netdevice %s, device already initialized \n",
                        __func__, device->device_name );
            continue;
        }

        if ( (device->flags & AVMNET_CONFIG_FLAG_USE_VLAN) && vlan_master == NULL ) {
            AVMNET_INFO("[%s] %s has FLAG_USE_VLAN set but vlan_master not ready.\n",
                        __func__, device->device_name );
            continue;
        }

        ret = avmnet_create_netdevice(device);
        if(ret != 0) {
            AVMNET_ERR("[%s] avmnet_create_netdevice returned with error\n", __func__);
            return ret;
        }

        device->flags |= AVMNET_DEVICE_FLAG_INITIALIZED;
    }
    return ret;
}
EXPORT_SYMBOL(avmnet_create_netdevices);

/*------------------------------------------------------------------------------------------*\
 * initialise and set up the network modules
\*------------------------------------------------------------------------------------------*/
static int avmnet_cfg_netinit(void)
{
    avmnet_module_t *config;

    if (no_avmnet){
        pr_err("Skipping AVMNET late_initcall due to module variable\n");
        return 0;
    }

    printk(KERN_ERR "{%s}\n", __func__);
    if(get_config() < 0) {
        AVMNET_ERR("[%s] Error! No configuration available. Aborting!\n", __func__);
        BUG();
    }

    config = avmnet_hw_config_entry->config;

    global_workqueue = create_singlethread_workqueue("avmnet_workqueue");
    if(global_workqueue == NULL){
        AVMNET_ERR("[%s] Could not create global AVMNET workqueue\n", __FUNCTION__);
        BUG();
    }

    AVMNET_INFO("[%s] calling init on top network module\n", __func__);
    BUG_ON(!config->init);
    if(config->init(config) != 0){
        AVMNET_ERR("[%s] network module init returned with error\n", __func__);
        BUG();
    }

    AVMNET_INFO("[%s] calling setup on top network module\n", __func__);
    BUG_ON(!config->setup);
    if(config->setup(config) != 0){
        AVMNET_ERR("[%s] network module setup returned with error\n", __func__);
        BUG();
    }

    if (avmnet_create_netdevices() < 0) {
        BUG();
    }

    if(avmnet_mgmt_init()) {
        BUG();
    }

    queue_delayed_work(global_workqueue, &avmnet_heartbeat, CONFIG_HZ * 10);

    return 0;
}
late_initcall(avmnet_cfg_netinit);

int avmnet_register_master(struct avmnet_master_dev *master)
{
    int result;

    result = 0;
    if(vlan_master == NULL){
        vlan_master = master;
    } else {
        AVMNET_ERR("[%s] Error, vlan_master already registered\n", __func__);
        dump_stack();
        result = -EEXIST;
    }

    return result;
}
EXPORT_SYMBOL(avmnet_register_master);

int avmnet_unregister_master(struct avmnet_master_dev *master __attribute__((unused)))
{
    AVMNET_ERR("[%s] Not implemented\n", __func__);

    return 0;
}

struct avmnet_master_dev * avmnet_get_master(void)
{
    return vlan_master;
}
EXPORT_SYMBOL(avmnet_get_master);

struct workqueue_struct *avmnet_get_workqueue()
{
    return global_workqueue;
}
EXPORT_SYMBOL(avmnet_get_workqueue);

static void do_avmnet_heartbeat(struct work_struct *work)
{
    avmnet_module_t *config = avmnet_hw_config_entry->config;
    struct delayed_work *heartbeat = to_delayed_work(work);

    if(config->poll != NULL){
        config->poll(config);
    }

    queue_delayed_work(global_workqueue, heartbeat, CONFIG_HZ);
}


#ifdef CONFIG_AVM_PA
static void pa_dev_transmit(void *arg, struct sk_buff *skb)
{
   int rc;
   skb->dev = (struct net_device *)arg;
   rc = dev_queue_xmit(skb);
   if (rc != 0 && net_ratelimit()) {
      AVMNET_INFO("[%s] pa_dev_transmit(%s) %d", __func__,
            ((struct net_device *)arg)->name, rc);
   }
}

#if defined(CONFIG_IFX_PPA) || defined(CONFIG_AVM_CPMAC_ATH_PPA)
void dump_hwinfo( struct avm_pa_pid_hwinfo *pid_hwinfo __attribute__((unused)) ){

    AVMNET_INFO("[%s] flags=%#x\n", __func__, pid_hwinfo->flags );
#   if defined(CONFIG_IFX_PPA)
    AVMNET_INFO("[%s] mac_nr=%d\n", __func__, pid_hwinfo->mac_nr );
#   endif /*--- #if defined(CONFIG_IFX_PPA) ---*/
#   if defined(CONFIG_AVM_CPMAC_ATH_PPA)
    AVMNET_INFO("[%s] port_number=%d\n", __func__, pid_hwinfo->port_number );
#   endif /*--- #if defined(CONFIG_AVM_CPMAC_ATH_PPA) ---*/
}
#endif /*--- #if defined(CONFIG_IFX_PPA) || defined(CONFIG_AVM_CPMAC_ATH_PPA) ---*/

#endif

/*------------------------------------------------------------------------------------------*\
 * create netdevice (this will be called in late_initcall)
\*------------------------------------------------------------------------------------------*/
static int avmnet_create_netdevice(avmnet_device_t *avm_dev)
{
    avmnet_module_t *phy;
    struct ethtool_ops *eth_ops;
    int result;

    AVMNET_INFO("[%s] create %s \n", __func__, avm_dev->device_name);

    result = 0;

    eth_ops = kzalloc(sizeof(struct ethtool_ops), GFP_KERNEL);
    if(!eth_ops){
        AVMNET_ERR("[%s] Could not allocate struct ethtool_ops for device %s.\n", __func__,
                   avm_dev->device_name);
        result = -ENOMEM;
        goto err_out;
    }

#if defined(AVM_REGISTER_VLAN_DEVICE)
    if(avm_dev->flags & AVMNET_CONFIG_FLAG_USE_VLAN){
        if(vlan_master == NULL){
            AVMNET_ERR("[%s] No VLAN master device for %s\n", __func__, avm_dev->device_name);
            result = -ENODEV;
            goto err_out;
        }

        rtnl_lock();
        result = avm_register_vlan_device(vlan_master->master_dev, avm_dev->vlanID,
                                          avm_dev->device_name, &(avm_dev->device));
        rtnl_unlock();
        if(result != 0){
            AVMNET_ERR("[%s] Creating VLAN slave device failed for %s\n",
                       __func__,
                       avm_dev->device_name);
            goto err_out;
        }
    }else{
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,17,0)
        avm_dev->device = alloc_netdev(avm_dev->sizeof_priv, avm_dev->device_name,
                                       avm_dev->device_setup);
#else
        avm_dev->device = alloc_netdev(avm_dev->sizeof_priv, avm_dev->device_name,
                                       NET_NAME_PREDICTABLE, avm_dev->device_setup);
#endif
        if(avm_dev->device == NULL){
            AVMNET_ERR("[%s] Could not allocate net_device %s\n", __func__,
                       avm_dev->device_name);
            result = -ENOMEM;
            goto err_out;
        }

        avm_dev->device->netdev_ops = &avm_dev->device_ops;
#if defined(AVM_REGISTER_VLAN_DEVICE)
    }
#endif
    avm_dev->device->features |= avm_dev->net_dev_features;
#ifndef NETIF_F_VLAN_FEATURES
#if defined(NETIF_F_HW_VLAN_TX) /* 2.6.39 */
#define NETIF_F_VLAN_FEATURES (NETIF_F_HW_VLAN_TX|NETIF_F_HW_VLAN_RX|NETIF_F_HW_VLAN_FILTER)
#elif defined(NETIF_F_HW_VLAN_CTAG_TX) /* 3.10 */
#define NETIF_F_VLAN_FEATURES (NETIF_F_HW_VLAN_CTAG_TX|NETIF_F_HW_VLAN_CTAG_RX|NETIF_F_HW_VLAN_CTAG_FILTER)
#endif
#endif
    avm_dev->device->hw_features |= avm_dev->net_dev_features & NETIF_F_VLAN_FEATURES;

    AVMNET_INFO("[%s] register_netdev %s \n", __func__, avm_dev->device_name);

    if(avm_dev->device_setup_priv){
        avm_dev->device_setup_priv(avm_dev);
    }
    phy = get_phy_by_netdev(avm_dev->device);
    assemble_ethops(phy, &(avm_dev->ethtool_module_table), eth_ops);

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,16,0)
    SET_ETHTOOL_OPS(avm_dev->device, eth_ops);
#else
    avm_dev->device->ethtool_ops = eth_ops;
#endif

    if(!(avm_dev->flags & AVMNET_CONFIG_FLAG_USE_VLAN)){
        register_netdev(avm_dev->device);
    }

#   if defined(CONFIG_IP_MULTICAST_FASTFORWARD)
    avmnet_mcfw_init(avm_dev);
#   endif /*--- #if defined(CONFIG_IP_MULTICAST_FASTFORWARD) ---*/

#if defined(CONFIG_AVM_RTNETLINK_ENHANCEMENT)
    if (avm_dev->flags & AVMNET_DEVICE_FLAG_PHYS_OFFLOAD_LINK){
        pr_err("[%s] setup offload_cpu_link on device %s\n", __func__, avm_dev->device_name);
        link_dev_to_offload_cpu = avm_dev;
        wait_for_link_to_offload_cpu_hook = avmnet_wait_for_link_to_offload_cpu;
    }
#endif

#   if defined(CONFIG_AVM_PA)
    {
        struct avm_pa_pid_cfg *pid_cfg;
        struct avm_pa_vpid_cfg *vpid_cfg;
        int register_res = -1;

        AVMNET_INFO("[%s] registering device %s with AVM PA.\n", __func__,
                avm_dev->device->name);

        pid_cfg = kzalloc(sizeof(struct avm_pa_pid_cfg), GFP_KERNEL);
        if (!pid_cfg)
        return -ENOMEM;

        vpid_cfg = kzalloc(sizeof(struct avm_pa_vpid_cfg), GFP_KERNEL);
        if (!vpid_cfg){
            kfree(pid_cfg);
            return -ENOMEM;
        }

        pid_cfg->default_mtu = 1590;
        vpid_cfg->v4_mtu = 1500;    /*--- JZ-49321 ---*/
        vpid_cfg->v6_mtu = 1500;
        pid_cfg->framing = avm_pa_framing_dev;
        snprintf(pid_cfg->name, sizeof(pid_cfg->name), "%s", avm_dev->device->name);
        snprintf(vpid_cfg->name, sizeof(pid_cfg->name), "%s", avm_dev->device->name);

        pid_cfg->tx_arg = avm_dev->device;
        pid_cfg->tx_func = pa_dev_transmit;

#if defined(AVM_REGISTER_VLAN_DEVICE)
        if(avm_dev->flags & AVMNET_CONFIG_FLAG_USE_VLAN) {
             /* register with ingress handle of vlan master to avoid confusing the avm_pa
              * if mac addresses are seen on different pids (it would complain and
              * constantly flush sessions) */
            avm_pid_handle master_pid = AVM_PA_DEVINFO(vlan_master->master_dev)->pid_handle;
            if (master_pid == 0)
                pr_err("avmnet: consider an avm_pa pid registration for vlan master (%s), may yield better performance", vlan_master->master_dev->name);
            else
                register_res = avm_pa_dev_pid_register_with_ingress(&(avm_dev->device->avm_pa.devinfo), pid_cfg, master_pid);
        }
#endif
        if (register_res < 0) {
            register_res = avm_pa_dev_pid_register(&(avm_dev->device->avm_pa.devinfo), pid_cfg);
        }
        if (register_res < 0){
            AVMNET_ERR("[%s]: failed to register PA PID for device %s\n", __func__, avm_dev->device->name);
            kfree(vpid_cfg);
            kfree(pid_cfg);
            return register_res;
        }

        register_res = avm_pa_dev_vpid_register(&(avm_dev->device->avm_pa.devinfo), vpid_cfg);
        if (register_res < 0){
            AVMNET_ERR("[%s]: failed to register PA VPID for device %s\n", __func__, avm_dev->device->name);
            avm_pa_dev_unregister(&(avm_dev->device->avm_pa.devinfo), NULL);
            kfree(vpid_cfg);
            kfree(pid_cfg);
            return register_res;
        }

#   if defined(CONFIG_IFX_PPA) || defined(CONFIG_AVM_CPMAC_ATH_PPA)
        {
            struct avm_pa_pid_hwinfo tmp_pid_hwinfo ={0};
            int avm_pa_pid;
            avm_pa_pid = avm_dev->device->avm_pa.devinfo.pid_handle;

            AVMNET_INFO("[%s] device %s registered at AVM PA: got avm_pa_pid=%d.\n", __func__, avm_dev->device->name, avm_pa_pid );

#       if defined(CONFIG_IFX_PPA)
            if ( (avm_dev->mac_module) && (avm_dev->mac_module->type == avmnet_modtype_mac)){
                tmp_pid_hwinfo.mac_nr = avm_dev->mac_module->initdata.mac.mac_nr;
            }
#       endif /*--- #if defined(CONFIG_IFX_PPA) ---*/
#       if defined(CONFIG_AVM_CPMAC_ATH_PPA)
            {
                avmnet_module_t *phy = get_phy_by_netdev(avm_dev->device);
                if(phy == NULL){
                    AVMNET_ERR("[%s] Failed to query for module for '%s'\n", __func__, avm_dev->device->name);
                    avm_pa_dev_unregister(&(avm_dev->device->avm_pa.devinfo), NULL);
                    kfree(vpid_cfg);
                    kfree(pid_cfg);
                    return -EFAULT;
                }
                tmp_pid_hwinfo.avmnet_switch_module = (void *)get_switch_by_netdev(avm_dev->device);


                if ( phy->parent->type == avmnet_modtype_mac ){
                    /*
                     * HW_CONFIG: [cpu-mii]<->[mac]<->[switch]<->[mac]<->[phy]
                     * e.g. HW211
                     */
                    tmp_pid_hwinfo.port_number = phy->parent->initdata.mac.mac_nr;
                }
                else{
                    /*
                     * HW_CONFIG: [cpu-mii]<->[mac]<->[switch]<->[phy]
                     * e.g. HW217
                     */
                    /*------------------------------------------------------------------------------------------*\
             * die vlanID entspricht bei Atheros der Portnumber, in atheros_gmac.c 
                     * wird sie genutzt, um Daten gezielt auf diesen Port zu senden
                     \*------------------------------------------------------------------------------------------*/
                    tmp_pid_hwinfo.port_number = avm_dev->vlanID;
                }
                pr_err( "avm_pa_hwinfo: device %s, with mac_nr %d\n", avm_dev->device->name, tmp_pid_hwinfo.port_number);
            }
#       endif /*--- #if defined(CONFIG_AVM_CPMAC_ATH_PPA) ---*/

            tmp_pid_hwinfo.flags = avm_dev->flags & ( AVMNET_DEVICE_IFXPPA_ETH_LAN |
                    AVMNET_DEVICE_IFXPPA_PTM_WAN |
                    AVMNET_DEVICE_IFXPPA_ATM_WAN);
            dump_hwinfo(&tmp_pid_hwinfo);

            register_res = avm_pa_pid_set_hwinfo( avm_pa_pid, &tmp_pid_hwinfo );
            if (register_res < 0)
            AVMNET_ERR("[%s]: failed to set pa hwinfo for device %s\n", __func__, avm_dev->device->name);
        }
#   endif /*--- #if defined(CONFIG_IFX_PPA) || defined(CONFIG_AVM_CPMAC_ATH_PPA) --- */

        kfree(vpid_cfg);
        kfree(pid_cfg);
        AVMNET_INFO("[%s] %s hard_header_len after setup %d.\n", __func__, avm_dev->device->name, avm_dev->device->hard_header_len );
    }
#   endif /*--- #if defined(CONFIG_AVM_PA) ---*/

    if(avm_dev->device_setup_late)
        avm_dev->device_setup_late(avm_dev);

err_out:
    if(result != 0 && eth_ops != NULL){
        kfree(eth_ops);
    }

    return result;
}


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void avmnet_destroy_netdevice(avmnet_device_t *avm_dev) {

#if defined(CONFIG_IP_MULTICAST_FASTFORWARD)
    avmnet_mcfw_exit(avm_dev);
#endif

    // unregister_netdev uebernimmt implizit das entfernen der AVM_PA vpid u. pid
    unregister_netdev( avm_dev->device );
    free_netdev( avm_dev->device );
    avm_dev->device = NULL;
    avm_dev->flags |= AVMNET_DEVICE_FLAG_WAIT_FOR_MODULE_FUNCTIONS;
    avm_dev->flags &= ~AVMNET_DEVICE_FLAG_INITIALIZED;
    memset(&avm_dev->device_ops, 0, sizeof(struct net_device_ops));
    memset(&avm_dev->device_stats, 0, sizeof(struct net_device_stats));

}
EXPORT_SYMBOL(avmnet_destroy_netdevice);

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
avmnet_device_t *get_avmdev_by_netdev(const struct net_device *dev)
{
    avmnet_device_t *result = NULL;
    int i;

    for (i = 0; i < avmnet_hw_config_entry->nr_avm_devices; i++){
        if(avmnet_hw_config_entry->avm_devices[i]->device == dev ) {
            result = avmnet_hw_config_entry->avm_devices[i];
            break;
        }
    }

    return result;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
avmnet_device_t *get_avmdev_by_name(const char *dev_name) {
    avmnet_device_t *result = NULL;
    int i;

    for (i = 0; i < avmnet_hw_config_entry->nr_avm_devices; i++){

        if(!strcmp(dev_name, avmnet_hw_config_entry->avm_devices[i]->device_name )) {
            result = avmnet_hw_config_entry->avm_devices[i];
            break;
        }
    }

    return result;
}
EXPORT_SYMBOL(get_avmdev_by_name);


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
avmnet_module_t *get_mac_by_netdev(const struct net_device *dev)
{
    avmnet_device_t *avm_dev;
    avmnet_module_t *mac = NULL;

    avm_dev = get_avmdev_by_netdev(dev);
    if(avm_dev != NULL){
        mac = avm_dev->mac_module;
    }

    return mac;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
avmnet_module_t *get_phy_by_netdev(const struct net_device *dev)
{
    avmnet_device_t *avm_dev;
    avmnet_module_t *phy = NULL;
    avmnet_cfg_entry_t *entry;

    avm_dev = get_avmdev_by_netdev(dev);
    if(avm_dev != NULL){

        if (!down_interruptible(&module_cfg_sema)) {
            list_for_each_entry(entry, &module_list, module_list){
                if(   (entry->module->type == avmnet_modtype_phy)
                   && (entry->module->device_id == avm_dev))
                {
                    phy = entry->module;
                    break;
                }
            }
            up(&module_cfg_sema);
        } else {
            AVMNET_WARN("[%s] Unable to acquire mutex\n", __func__);
        }
    } else {
        AVMNET_WARN("[%s] Can not find avmnet_device for net_device pointer %p\n", __func__, dev);
    }

    return phy;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
avmnet_module_t *get_switch_by_netdev(const struct net_device *dev)
{
    avmnet_module_t *phy = NULL;
    avmnet_module_t *mod = NULL;

    phy = get_phy_by_netdev(dev);
    mod = phy;
    BUG_ON(!mod);

    while ( mod->parent ){
	    mod = mod->parent;
	    if (mod->type == avmnet_modtype_switch)
		    return mod;
    }

    BUG();
    return NULL;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_netdev_open(struct net_device *dev)
{
    avmnet_device_t *avm_dev;
    avmnet_module_t *phy;

    avm_dev = get_avmdev_by_netdev(dev);
    phy = get_phy_by_netdev(dev);

    if(avm_dev == NULL){
        AVMNET_ERR("[%s] Called with unknown net device.", __func__);
        return -ENODEV;
    }

    netif_carrier_off(dev);

    if(phy != NULL){
        if(phy->powerup) {
            if(phy->powerup(phy)){
                AVMNET_WARN("[%s] Powerup of PHY %s failed.\n", __func__, phy->name);
            }
        }
        if(phy->setup_irq){
            if(phy->setup_irq(phy, 1)){
                AVMNET_WARN("[%s] IRQ setup of PHY %s failed.\n", __func__, phy->name);
            }
        }
    } else {
        AVMNET_WARN("[%s] Unable to find PHY for device %s\n", __func__, avm_dev->device_name);
    }

#   if defined(CONFIG_IP_MULTICAST_FASTFORWARD)
    avmnet_mcfw_enable(dev);
#   endif /*--- #if defined(CONFIG_IP_MULTICAST_FASTFORWARD) ---*/

    netif_start_queue(dev);
//    napi_enable(&(avm_dev->napi));

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_netdev_stop(struct net_device *dev)
{
    avmnet_device_t *avm_dev;
    avmnet_module_t *phy;

    avm_dev = get_avmdev_by_netdev(dev);
    phy = get_phy_by_netdev(dev);

    if(avm_dev == NULL){
        AVMNET_ERR("[%s] Called with unknown net device.", __func__);
        return -ENODEV;
    }

    netif_stop_queue(dev);
    AVMNET_DBG_TX_QUEUE("stop queue %s", dev->name);

//    napi_disable(&(avm_dev->napi));

#if defined(CONFIG_IP_MULTICAST_FASTFORWARD)
    avmnet_mcfw_disable(dev);
#endif /*--- #if defined(CONFIG_IP_MULTICAST_FASTFORWARD) ---*/

    if(phy != NULL){
        if(phy->setup_irq){
            if(phy->setup_irq(phy, 0)){
                AVMNET_WARN("[%s] IRQ setup of PHY %s failed.\n", __func__, phy->name);
            }
        }
        if(phy->powerdown(phy)){
            AVMNET_WARN("[%s] Powerup of PHY %s failed.\n", __func__, phy->name);
        }
    } else {
        AVMNET_WARN("[%s] Unable to find PHY for device %s\n", __func__, avm_dev->device_name);
    }

    netif_carrier_off(dev);

    return 0;
}

/*------------------------------------------------------------------------------------------*\
 * assemble the net device's table of wrapper functions for ethtool operations and
 * set up a struct ethtool_ops to hand over to the kernel.
 * Should be called on a leaf node, usually a PHY.
\*------------------------------------------------------------------------------------------*/
static int assemble_ethops(avmnet_module_t *module,
                           avmnet_ethtool_table_t *table,
                           struct ethtool_ops *eth_ops)
{

    // these are genric functions and do not rely on a phy module
    eth_ops->get_sset_count = avmnet_eth_get_sset_count;
    eth_ops->get_strings = avmnet_eth_get_strings;
    eth_ops->get_ethtool_stats = avmnet_eth_get_ethtool_stats;

    // climb up the module tree and add functions found in the module but not
    //  already registered
    while(module != NULL){
        AVMNET_INFO("[%s] Checking module %s for ethtool ops\n", __func__, module->name);

        if(table->get_settings == NULL && module->ethtool_ops.get_settings != NULL){
            table->get_settings = module;
            eth_ops->get_settings = avmnet_eth_get_settings;
        }

        if(table->set_settings == NULL && module->ethtool_ops.set_settings != NULL){
            table->set_settings = module;
            eth_ops->set_settings = avmnet_eth_set_settings;
        }

        if(table->get_drvinfo == NULL && module->ethtool_ops.get_drvinfo != NULL){
            table->get_drvinfo = module;
            eth_ops->get_drvinfo = avmnet_eth_get_drvinfo;
        }

        if(table->get_regs_len == NULL && module->ethtool_ops.get_regs_len != NULL){
            table->get_regs_len = module;
            eth_ops->get_regs_len = avmnet_eth_get_regs_len;
        }

        if(table->get_regs == NULL && module->ethtool_ops.get_regs != NULL){
            table->get_regs = module;
            eth_ops->get_regs = avmnet_eth_get_regs;
        }

        if(table->get_wol == NULL && module->ethtool_ops.get_wol != NULL){
            table->get_wol = module;
            eth_ops->get_wol = avmnet_eth_get_wol;
        }

        if(table->set_wol == NULL && module->ethtool_ops.set_wol != NULL){
            table->set_wol = module;
            eth_ops->set_wol = avmnet_eth_set_wol;
        }

        if(table->get_msglevel == NULL && module->ethtool_ops.get_msglevel != NULL){
            table->get_msglevel = module;
            eth_ops->get_msglevel = avmnet_eth_get_msglevel;
        }

        if(table->set_msglevel == NULL && module->ethtool_ops.set_msglevel != NULL){
            table->set_msglevel = module;
            eth_ops->set_msglevel = avmnet_eth_set_msglevel;
        }

        if(table->nway_reset == NULL && module->ethtool_ops.nway_reset != NULL){
            table->nway_reset = module;
            eth_ops->nway_reset = avmnet_eth_nway_reset;
        }

        if(table->get_link == NULL && module->ethtool_ops.get_link != NULL){
            table->get_link = module;
            eth_ops->get_link = avmnet_eth_get_link;
        }

        if(table->get_eeprom_len == NULL && module->ethtool_ops.get_eeprom_len != NULL){
            table->get_eeprom_len = module;
            eth_ops->get_eeprom_len = avmnet_eth_get_eeprom_len;
        }

        if(table->get_eeprom == NULL && module->ethtool_ops.get_eeprom != NULL){
            table->get_eeprom = module;
            eth_ops->get_eeprom = avmnet_eth_get_eeprom;
        }

        if(table->set_eeprom == NULL && module->ethtool_ops.set_eeprom != NULL){
            table->set_eeprom = module;
            eth_ops->set_eeprom = avmnet_eth_set_eeprom;
        }

        if(table->get_coalesce == NULL && module->ethtool_ops.get_coalesce != NULL){
            table->get_coalesce = module;
            eth_ops->get_coalesce = avmnet_eth_get_coalesce;
        }

        if(table->set_coalesce == NULL && module->ethtool_ops.set_coalesce != NULL){
            table->set_coalesce = module;
            eth_ops->set_coalesce = avmnet_eth_set_coalesce;
        }

        if(table->get_ringparam == NULL && module->ethtool_ops.get_ringparam != NULL){
            table->get_ringparam = module;
            eth_ops->get_ringparam = avmnet_eth_get_ringparam;
        }

        if(table->set_ringparam == NULL && module->ethtool_ops.set_ringparam != NULL){
            table->set_ringparam = module;
            eth_ops->set_ringparam = avmnet_eth_set_ringparam;
        }

        if(table->get_pauseparam == NULL && module->ethtool_ops.get_pauseparam != NULL){
            table->get_pauseparam = module;
            eth_ops->get_pauseparam = avmnet_eth_get_pauseparam;
        }

        if(table->set_pauseparam == NULL && module->ethtool_ops.set_pauseparam != NULL){
            table->set_pauseparam = module;
            eth_ops->set_pauseparam = avmnet_eth_set_pauseparam;
        }

        if(table->self_test == NULL && module->ethtool_ops.self_test != NULL){
            table->self_test = module;
            eth_ops->self_test = avmnet_eth_self_test;
        }

        if(table->begin == NULL && module->ethtool_ops.begin != NULL){
            table->begin = module;
            eth_ops->begin = avmnet_eth_begin;
        }

        if(table->complete == NULL && module->ethtool_ops.complete != NULL){
            table->complete = module;
            eth_ops->complete = avmnet_eth_complete;
        }

        if(table->get_priv_flags == NULL && module->ethtool_ops.get_priv_flags != NULL){
            table->get_priv_flags = module;
            eth_ops->get_priv_flags = avmnet_eth_get_priv_flags;
        }

        if(table->set_priv_flags == NULL && module->ethtool_ops.set_priv_flags != NULL){
            table->set_priv_flags = module;
            eth_ops->set_priv_flags = avmnet_eth_set_priv_flags;
        }


        if(table->get_rxnfc == NULL && module->ethtool_ops.get_rxnfc != NULL){
            table->get_rxnfc = module;
            eth_ops->get_rxnfc = avmnet_eth_get_rxnfc;
        }

        if(table->set_rxnfc == NULL && module->ethtool_ops.set_rxnfc != NULL){
            table->set_rxnfc = module;
            eth_ops->set_rxnfc = avmnet_eth_set_rxnfc;
        }

        if(table->flash_device == NULL && module->ethtool_ops.flash_device != NULL){
            table->flash_device = module;
            eth_ops->flash_device = avmnet_eth_flash_device;
        }

        module = module->parent;
    }

    return 0;
}


/*------------------------------------------------------------------------------------------*\
 * wrapper functions for the ethtool ops
\*------------------------------------------------------------------------------------------*/
static int avmnet_eth_get_settings(struct net_device *netdev, struct ethtool_cmd *cmd)
{
    avmnet_device_t *avm_dev;
    avmnet_module_t *module;

    avm_dev = get_avmdev_by_netdev(netdev);
    if(avm_dev == NULL){
        AVMNET_ERR("[%s] Called for unknown device.\n", __func__);
        return -ENODEV;
    }

    module = avm_dev->ethtool_module_table.get_settings;

    if(module == NULL){
        AVMNET_ERR("[%s] Called but function not registered.\n", __func__);
        return -EFAULT;
    }

    if(module->ethtool_ops.get_settings == NULL){
        AVMNET_ERR("[%s] Function registered for module %s but not implemented.\n", __func__, module->name);
        return -EFAULT;
    }

    return module->ethtool_ops.get_settings(module, cmd);
}


static int avmnet_eth_set_settings(struct net_device *netdev, struct ethtool_cmd *cmd)
{
    avmnet_device_t *avm_dev;
    avmnet_module_t *module;

    avm_dev = get_avmdev_by_netdev(netdev);
    if(avm_dev == NULL){
        AVMNET_ERR("[%s] Called for unknown device.\n", __func__);
        return -ENODEV;
    }

    module = avm_dev->ethtool_module_table.set_settings;

    if(module == NULL){
        AVMNET_ERR("[%s] Called but function not registered.\n", __func__);
        return -EFAULT;
    }

    if(module->ethtool_ops.set_settings == NULL){
        AVMNET_ERR("[%s] Function registered for module %s but not implemented.\n", __func__, module->name);
        return -EFAULT;
    }

    return module->ethtool_ops.set_settings(module, cmd);
}


static void avmnet_eth_get_drvinfo(struct net_device *netdev, struct ethtool_drvinfo *drvinfo)
{
    avmnet_device_t *avm_dev;
    avmnet_module_t *module;

    avm_dev = get_avmdev_by_netdev(netdev);
    if(avm_dev == NULL){
        AVMNET_ERR("[%s] Called for unknown device.\n", __func__);
        return;
    }

    module = avm_dev->ethtool_module_table.get_drvinfo;

    if(module == NULL){
        AVMNET_ERR("[%s] Called but function not registered.\n", __func__);
        return;
    }

    if(module->ethtool_ops.get_drvinfo == NULL){
        AVMNET_ERR("[%s] Function registered for module %s but not implemented.\n", __func__, module->name);
        return;
    }

    module->ethtool_ops.get_drvinfo(module, drvinfo);
}


static int avmnet_eth_get_regs_len(struct net_device *netdev)
{
    avmnet_device_t *avm_dev;
    avmnet_module_t *module;

    avm_dev = get_avmdev_by_netdev(netdev);
    if(avm_dev == NULL){
        AVMNET_ERR("[%s] Called for unknown device.\n", __func__);
        return -ENODEV;
    }

    module = avm_dev->ethtool_module_table.get_regs_len;

    if(module == NULL){
        AVMNET_ERR("[%s] Called but function not registered.\n", __func__);
        return -EFAULT;
    }

    if(module->ethtool_ops.get_regs_len == NULL){
        AVMNET_ERR("[%s] Function registered for module %s but not implemented.\n", __func__, module->name);
        return -EFAULT;
    }

    return module->ethtool_ops.get_regs_len(module);
}


static void avmnet_eth_get_regs(struct net_device *netdev, struct ethtool_regs *regs, void *data)
{
    avmnet_device_t *avm_dev;
    avmnet_module_t *module;

    avm_dev = get_avmdev_by_netdev(netdev);
    if(avm_dev == NULL){
        AVMNET_ERR("[%s] Called for unknown device.\n", __func__);
        return;
    }

    module = avm_dev->ethtool_module_table.get_regs;

    if(module == NULL){
        AVMNET_ERR("[%s] Called but function not registered.\n", __func__);
        return;
    }

    if(module->ethtool_ops.get_regs == NULL){
        AVMNET_ERR("[%s] Function registered for module %s but not implemented.\n", __func__, module->name);
        return;
    }

    module->ethtool_ops.get_regs(module, regs, data);
}


static void avmnet_eth_get_wol(struct net_device *netdev, struct ethtool_wolinfo *wolinfo)
{
    avmnet_device_t *avm_dev;
    avmnet_module_t *module;

    avm_dev = get_avmdev_by_netdev(netdev);
    if(avm_dev == NULL){
        AVMNET_ERR("[%s] Called for unknown device.\n", __func__);
        return;
    }

    module = avm_dev->ethtool_module_table.get_wol;

    if(module == NULL){
        AVMNET_ERR("[%s] Called but function not registered.\n", __func__);
        return;
    }

    if(module->ethtool_ops.get_wol == NULL){
        AVMNET_ERR("[%s] Function registered for module %s but not implemented.\n", __func__, module->name);
        return;
    }

    module->ethtool_ops.get_wol(module, wolinfo);
}


static int avmnet_eth_set_wol(struct net_device *netdev, struct ethtool_wolinfo *wolinfo)
{
    avmnet_device_t *avm_dev;
    avmnet_module_t *module;

    avm_dev = get_avmdev_by_netdev(netdev);
    if(avm_dev == NULL){
        AVMNET_ERR("[%s] Called for unknown device.\n", __func__);
        return -ENODEV;
    }

    module = avm_dev->ethtool_module_table.set_wol;

    if(module == NULL){
        AVMNET_ERR("[%s] Called but function not registered.\n", __func__);
        return -EFAULT;
    }

    if(module->ethtool_ops.set_wol == NULL){
        AVMNET_ERR("[%s] Function registered for module %s but not implemented.\n", __func__, module->name);
        return -EFAULT;
    }

    return module->ethtool_ops.set_wol(module, wolinfo);
}


static u32 avmnet_eth_get_msglevel(struct net_device *netdev)
{
    avmnet_device_t *avm_dev;
    avmnet_module_t *module;

    avm_dev = get_avmdev_by_netdev(netdev);
    if(avm_dev == NULL){
        AVMNET_ERR("[%s] Called for unknown device.\n", __func__);
        return 0;
    }

    module = avm_dev->ethtool_module_table.get_msglevel;

    if(module == NULL){
        AVMNET_ERR("[%s] Called but function not registered.\n", __func__);
        return 0;
    }

    if(module->ethtool_ops.get_msglevel == NULL){
        AVMNET_ERR("[%s] Function registered for module %s but not implemented.\n", __func__, module->name);
        return 0;
    }

    return module->ethtool_ops.get_msglevel(module);
}

static void avmnet_eth_set_msglevel(struct net_device *netdev, u32 level)
{
    avmnet_device_t *avm_dev;
    avmnet_module_t *module;

    avm_dev = get_avmdev_by_netdev(netdev);
    if(avm_dev == NULL){
        AVMNET_ERR("[%s] Called for unknown device.\n", __func__);
        return;
    }

    module = avm_dev->ethtool_module_table.set_msglevel;

    if(module == NULL){
        AVMNET_ERR("[%s] Called but function not registered.\n", __func__);
        return;
    }

    if(module->ethtool_ops.set_msglevel == NULL){
        AVMNET_ERR("[%s] Function registered for module %s but not implemented.\n", __func__, module->name);
        return;
    }

    module->ethtool_ops.set_msglevel(module, level);
}


static int avmnet_eth_nway_reset(struct net_device *netdev)
{
    avmnet_device_t *avm_dev;
    avmnet_module_t *module;

    avm_dev = get_avmdev_by_netdev(netdev);
    if(avm_dev == NULL){
        AVMNET_ERR("[%s] Called for unknown device.\n", __func__);
        return -ENODEV;
    }

    module = avm_dev->ethtool_module_table.nway_reset;

    if(module == NULL){
        AVMNET_ERR("[%s] Called but function not registered.\n", __func__);
        return -EFAULT;
    }

    if(module->ethtool_ops.nway_reset == NULL){
        AVMNET_ERR("[%s] Function registered for module %s but not implemented.\n", __func__, module->name);
        return -EFAULT;
    }

    return module->ethtool_ops.nway_reset(module);
}


static u32 avmnet_eth_get_link(struct net_device *netdev)
{
    avmnet_device_t *avm_dev;
    avmnet_module_t *module;

    avm_dev = get_avmdev_by_netdev(netdev);
    if(avm_dev == NULL){
        AVMNET_ERR("[%s] Called for unknown device.\n", __func__);
        return 0;
    }

    module = avm_dev->ethtool_module_table.get_link;

    if(module == NULL){
        AVMNET_ERR("[%s] Called but function not registered.\n", __func__);
        return 0;
    }

    if(module->ethtool_ops.get_link == NULL){
        AVMNET_ERR("[%s] Function registered for module %s but not implemented.\n", __func__, module->name);
        return 0;
    }

    return module->ethtool_ops.get_link(module);
}


static int avmnet_eth_get_eeprom_len(struct net_device *netdev)
{
    avmnet_device_t *avm_dev;
    avmnet_module_t *module;

    avm_dev = get_avmdev_by_netdev(netdev);
    if(avm_dev == NULL){
        AVMNET_ERR("[%s] Called for unknown device.\n", __func__);
        return -ENODEV;
    }

    module = avm_dev->ethtool_module_table.get_eeprom_len;

    if(module == NULL){
        AVMNET_ERR("[%s] Called but function not registered.\n", __func__);
        return -EFAULT;
    }

    if(module->ethtool_ops.get_eeprom_len == NULL){
        AVMNET_ERR("[%s] Function registered for module %s but not implemented.\n", __func__, module->name);
        return -EFAULT;
    }

    return module->ethtool_ops.get_eeprom_len(module);
}


static int avmnet_eth_get_eeprom(struct net_device *netdev, struct ethtool_eeprom *eeprom, u8 *data)
{
    avmnet_device_t *avm_dev;
    avmnet_module_t *module;

    avm_dev = get_avmdev_by_netdev(netdev);
    if(avm_dev == NULL){
        AVMNET_ERR("[%s] Called for unknown device.\n", __func__);
        return -ENODEV;
    }

    module = avm_dev->ethtool_module_table.get_eeprom;

    if(module == NULL){
        AVMNET_ERR("[%s] Called but function not registered.\n", __func__);
        return -EFAULT;
    }

    if(module->ethtool_ops.get_eeprom == NULL){
        AVMNET_ERR("[%s] Function registered for module %s but not implemented.\n", __func__, module->name);
        return -EFAULT;
    }

    return module->ethtool_ops.get_eeprom(module, eeprom, data);
}


static int avmnet_eth_set_eeprom(struct net_device *netdev, struct ethtool_eeprom *eeprom, u8 *data)
{
    avmnet_device_t *avm_dev;
    avmnet_module_t *module;

    avm_dev = get_avmdev_by_netdev(netdev);
    if(avm_dev == NULL){
        AVMNET_ERR("[%s] Called for unknown device.\n", __func__);
        return -ENODEV;
    }

    module = avm_dev->ethtool_module_table.set_eeprom;

    if(module == NULL){
        AVMNET_ERR("[%s] Called but function not registered.\n", __func__);
        return -EFAULT;
    }

    if(module->ethtool_ops.set_eeprom == NULL){
        AVMNET_ERR("[%s] Function registered for module %s but not implemented.\n", __func__, module->name);
        return -EFAULT;
    }

    return module->ethtool_ops.set_eeprom(module, eeprom, data);
}


static int avmnet_eth_get_coalesce(struct net_device *netdev, struct ethtool_coalesce *coalesce)
{
    avmnet_device_t *avm_dev;
    avmnet_module_t *module;

    avm_dev = get_avmdev_by_netdev(netdev);
    if(avm_dev == NULL){
        AVMNET_ERR("[%s] Called for unknown device.\n", __func__);
        return -ENODEV;
    }

    module = avm_dev->ethtool_module_table.get_coalesce;

    if(module == NULL){
        AVMNET_ERR("[%s] Called but function not registered.\n", __func__);
        return -EFAULT;
    }

    if(module->ethtool_ops.get_coalesce == NULL){
        AVMNET_ERR("[%s] Function registered for module %s but not implemented.\n", __func__, module->name);
        return -EFAULT;
    }

    return module->ethtool_ops.get_coalesce(module, coalesce);
}


static int avmnet_eth_set_coalesce(struct net_device *netdev, struct ethtool_coalesce *coalesce)
{
    avmnet_device_t *avm_dev;
    avmnet_module_t *module;

    avm_dev = get_avmdev_by_netdev(netdev);
    if(avm_dev == NULL){
        AVMNET_ERR("[%s] Called for unknown device.\n", __func__);
        return -ENODEV;
    }

    module = avm_dev->ethtool_module_table.set_coalesce;

    if(module == NULL){
        AVMNET_ERR("[%s] Called but function not registered.\n", __func__);
        return -EFAULT;
    }

    if(module->ethtool_ops.set_coalesce == NULL){
        AVMNET_ERR("[%s] Function registered for module %s but not implemented.\n", __func__, module->name);
        return -EFAULT;
    }

    return module->ethtool_ops.set_coalesce(module, coalesce);
}


static void avmnet_eth_get_ringparam(struct net_device *netdev, struct ethtool_ringparam *ringparam)
{
    avmnet_device_t *avm_dev;
    avmnet_module_t *module;

    avm_dev = get_avmdev_by_netdev(netdev);
    if(avm_dev == NULL){
        AVMNET_ERR("[%s] Called for unknown device.\n", __func__);
        return;
    }

    module = avm_dev->ethtool_module_table.get_ringparam;

    if(module == NULL){
        AVMNET_ERR("[%s] Called but function not registered.\n", __func__);
        return;
    }

    if(module->ethtool_ops.get_ringparam == NULL){
        AVMNET_ERR("[%s] Function registered for module %s but not implemented.\n", __func__, module->name);
        return;
    }

    module->ethtool_ops.get_ringparam(module, ringparam);
}


static int avmnet_eth_set_ringparam(struct net_device *netdev, struct ethtool_ringparam *ringparam)
{
    avmnet_device_t *avm_dev;
    avmnet_module_t *module;

    avm_dev = get_avmdev_by_netdev(netdev);
    if(avm_dev == NULL){
        AVMNET_ERR("[%s] Called for unknown device.\n", __func__);
        return -ENODEV;
    }

    module = avm_dev->ethtool_module_table.set_ringparam;

    if(module == NULL){
        AVMNET_ERR("[%s] Called but function not registered.\n", __func__);
        return -EFAULT;
    }

    if(module->ethtool_ops.set_ringparam == NULL){
        AVMNET_ERR("[%s] Function registered for module %s but not implemented.\n", __func__, module->name);
        return -EFAULT;
    }

    return module->ethtool_ops.set_ringparam(module, ringparam);
}


static void avmnet_eth_get_pauseparam(struct net_device *netdev, struct ethtool_pauseparam *param)
{
    avmnet_device_t *avm_dev;
    avmnet_module_t *module;

    avm_dev = get_avmdev_by_netdev(netdev);
    if(avm_dev == NULL){
        AVMNET_ERR("[%s] Called for unknown device.\n", __func__);
        return;
    }

    module = avm_dev->ethtool_module_table.get_pauseparam;

    if(module == NULL){
        AVMNET_ERR("[%s] Called but function not registered.\n", __func__);
        return;
    }

    if(module->ethtool_ops.get_pauseparam == NULL){
        AVMNET_ERR("[%s] Function registered for module %s but not implemented.\n", __func__, module->name);
        return;
    }

    module->ethtool_ops.get_pauseparam(module, param);
}


static int avmnet_eth_set_pauseparam(struct net_device *netdev, struct ethtool_pauseparam *param)
{
    avmnet_device_t *avm_dev;
    avmnet_module_t *module;

    avm_dev = get_avmdev_by_netdev(netdev);
    if(avm_dev == NULL){
        AVMNET_ERR("[%s] Called for unknown device.\n", __func__);
        return -ENODEV;
    }

    module = avm_dev->ethtool_module_table.set_pauseparam;

    if(module == NULL){
        AVMNET_ERR("[%s] Called but function not registered.\n", __func__);
        return -EFAULT;
    }

    if(module->ethtool_ops.set_pauseparam == NULL){
        AVMNET_ERR("[%s] Function registered for module %s but not implemented.\n", __func__, module->name);
        return -EFAULT;
    }

    return module->ethtool_ops.set_pauseparam(module, param);
}


static void avmnet_eth_self_test(struct net_device *netdev, struct ethtool_test *test, u64 *data)
{
    avmnet_device_t *avm_dev;
    avmnet_module_t *module;

    avm_dev = get_avmdev_by_netdev(netdev);
    if(avm_dev == NULL){
        AVMNET_ERR("[%s] Called for unknown device.\n", __func__);
        return;
    }

    module = avm_dev->ethtool_module_table.self_test;

    if(module == NULL){
        AVMNET_ERR("[%s] Called but function not registered.\n", __func__);
        return;
    }

    if(module->ethtool_ops.self_test == NULL){
        AVMNET_ERR("[%s] Function registered for module %s but not implemented.\n", __func__, module->name);
        return;
    }

    module->ethtool_ops.self_test(module, test, data);
}


static int avmnet_eth_begin(struct net_device *netdev)
{
    avmnet_device_t *avm_dev;
    avmnet_module_t *module;

    avm_dev = get_avmdev_by_netdev(netdev);
    if(avm_dev == NULL){
        AVMNET_ERR("[%s] Called for unknown device.\n", __func__);
        return -ENODEV;
    }

    module = avm_dev->ethtool_module_table.begin;

    if(module == NULL){
        AVMNET_ERR("[%s] Called but function not registered.\n", __func__);
        return -EFAULT;
    }

    if(module->ethtool_ops.begin == NULL){
        AVMNET_ERR("[%s] Function registered for module %s but not implemented.\n", __func__, module->name);
        return -EFAULT;
    }

    return module->ethtool_ops.begin(module);
}


static void avmnet_eth_complete(struct net_device *netdev)
{
    avmnet_device_t *avm_dev;
    avmnet_module_t *module;

    avm_dev = get_avmdev_by_netdev(netdev);
    if(avm_dev == NULL){
        AVMNET_ERR("[%s] Called for unknown device.\n", __func__);
        return;
    }

    module = avm_dev->ethtool_module_table.complete;

    if(module == NULL){
        AVMNET_ERR("[%s] Called but function not registered.\n", __func__);
        return;
    }

    if(module->ethtool_ops.complete == NULL){
        AVMNET_ERR("[%s] Function registered for module %s but not implemented.\n", __func__, module->name);
        return;
    }

    module->ethtool_ops.complete(module);
}


static u32 avmnet_eth_get_priv_flags(struct net_device *netdev)
{
    avmnet_device_t *avm_dev;
    avmnet_module_t *module;

    avm_dev = get_avmdev_by_netdev(netdev);
    if(avm_dev == NULL){
        AVMNET_ERR("[%s] Called for unknown device.\n", __func__);
        return 0;
    }

    module = avm_dev->ethtool_module_table.get_priv_flags;

    if(module == NULL){
        AVMNET_ERR("[%s] Called but function not registered.\n", __func__);
        return 0;
    }

    if(module->ethtool_ops.get_priv_flags == NULL){
        AVMNET_ERR("[%s] Function registered for module %s but not implemented.\n", __func__, module->name);
        return 0;
    }

    return module->ethtool_ops.get_priv_flags(module);
}


static int avmnet_eth_set_priv_flags(struct net_device *netdev, u32 flags)
{
    avmnet_device_t *avm_dev;
    avmnet_module_t *module;

    avm_dev = get_avmdev_by_netdev(netdev);
    if(avm_dev == NULL){
        AVMNET_ERR("[%s] Called for unknown device.\n", __func__);
        return -ENODEV;
    }

    module = avm_dev->ethtool_module_table.set_priv_flags;

    if(module == NULL){
        AVMNET_ERR("[%s] Called but function not registered.\n", __func__);
        return -EFAULT;
    }

    if(module->ethtool_ops.set_priv_flags == NULL){
        AVMNET_ERR("[%s] Function registered for module %s but not implemented.\n", __func__, module->name);
        return -EFAULT;
    }

    return module->ethtool_ops.set_priv_flags(module, flags);
}

static int _avmnet_eth_get_rxnfc_compat(struct net_device *netdev,
                                        struct ethtool_rxnfc *rxnfc,
                                        u32 *data)
{
    avmnet_device_t *avm_dev;
    avmnet_module_t *module;

    avm_dev = get_avmdev_by_netdev(netdev);
    if(avm_dev == NULL){
        AVMNET_ERR("[%s] Called for unknown device.\n", __func__);
        return -ENODEV;
    }

    module = avm_dev->ethtool_module_table.get_rxnfc;

    if(module == NULL){
        AVMNET_ERR("[%s] Called but function not registered.\n", __func__);
        return -EFAULT;
    }

    if(module->ethtool_ops.get_rxnfc == NULL){
        AVMNET_ERR("[%s] Function registered for module %s but not implemented.\n", __func__, module->name);
        return -EFAULT;
    }

    return module->ethtool_ops.get_rxnfc(module, rxnfc, data);
}


static int avmnet_eth_set_rxnfc(struct net_device *netdev, struct ethtool_rxnfc *rxnfc)
{
    avmnet_device_t *avm_dev;
    avmnet_module_t *module;

    avm_dev = get_avmdev_by_netdev(netdev);
    if(avm_dev == NULL){
        AVMNET_ERR("[%s] Called for unknown device.\n", __func__);
        return -ENODEV;
    }

    module = avm_dev->ethtool_module_table.set_rxnfc;

    if(module == NULL){
        AVMNET_ERR("[%s] Called but function not registered.\n", __func__);
        return -EFAULT;
    }

    if(module->ethtool_ops.set_rxnfc == NULL){
        AVMNET_ERR("[%s] Function registered for module %s but not implemented.\n", __func__, module->name);
        return -EFAULT;
    }

    return module->ethtool_ops.set_rxnfc(module, rxnfc);
}

/*
 * this function is generic and does not rely on a phy module
 */
static int avmnet_eth_get_sset_count(struct net_device *netdev, int string_set)
{
    avmnet_device_t *avm_dev;

    avm_dev = get_avmdev_by_netdev(netdev);
    switch (string_set) {
    case ETH_SS_STATS:
        if(avm_dev == NULL){
            AVMNET_ERR("[%s] Called for unknown device.\n", __func__);
            return -ENODEV;
        }
        return avm_dev->etht_stat.hw_cnt + avm_dev->etht_stat.sw_cnt;
    default:
        return -EINVAL;
    }

}

/*
 * this function is generic and does not rely on a phy module
 */
static void avmnet_eth_get_strings(struct net_device *netdev, u32 stringset, u8 *data)
{
    size_t i;
    avmnet_device_t *avm_dev;

    avm_dev = get_avmdev_by_netdev(netdev);
    if(avm_dev == NULL){
        AVMNET_ERR("[%s] Called for unknown device.\n", __func__);
        return;
    }
    switch (stringset) {
    case ETH_SS_STATS:
        for (i = 0; i < avm_dev->etht_stat.hw_cnt; i++) {
            memcpy(data + i * ETH_GSTRING_LEN,
                   avm_dev->etht_stat.hw_entries[i].stat_string,
                   ETH_GSTRING_LEN);
        }
		data += avm_dev->etht_stat.hw_cnt * ETH_GSTRING_LEN;
        for (i = 0; i < avm_dev->etht_stat.sw_cnt; i++) {
            memcpy(data + i * ETH_GSTRING_LEN,
                   avm_dev->etht_stat.sw_entries[i].stat_string,
                   ETH_GSTRING_LEN);
        }
        break;
    }
}

struct avmnet_etht_stat_entry default_hw_stats[DEFAULT_HW_STAT_CNT] = {
    AVMNET_RMON_STAT("mac_rx_pkts_good", rx_pkts_good),
    AVMNET_RMON_STAT("mac_tx_pkts_good", tx_pkts_good),
    AVMNET_RMON_STAT("mac_rx_bytes_good", rx_bytes_good),
    AVMNET_RMON_STAT("mac_tx_bytes_good", tx_bytes_good),
    AVMNET_RMON_STAT("mac_rx_pkts_pause", rx_pkts_pause),
    AVMNET_RMON_STAT("mac_tx_pkts_pause", tx_pkts_pause),
    AVMNET_RMON_STAT("mac_rx_pkts_dropped", rx_pkts_dropped),
    AVMNET_RMON_STAT("mac_tx_pkts_dropped", tx_pkts_dropped),
    AVMNET_RMON_STAT("mac_rx_bytes_error", rx_bytes_error),
};

struct avmnet_etht_stat_entry default_netdev_stats[DEFAULT_NETDEV_STAT_CNT] = {
    LINUX_NETDEV_STAT("netif_rx_pkts_good", rx_packets),
    LINUX_NETDEV_STAT("netif_tx_pkts_good", tx_packets),
    LINUX_NETDEV_STAT("netif_rx_bytes_good", rx_bytes),
    LINUX_NETDEV_STAT("netif_tx_bytes_good", tx_bytes),
    LINUX_NETDEV_STAT("netif_rx_pkts_dropped", rx_dropped),
    LINUX_NETDEV_STAT("netif_tx_pkts_dropped", tx_dropped),
    LINUX_NETDEV_STAT("netif_rx_pkts_error", rx_errors),
    LINUX_NETDEV_STAT("netif_tx_pkts_error", tx_errors),
};

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,34)
struct avmnet_etht_stat_entry default_netdev_stats_64[DEFAULT_NETDEV_STAT_64_CNT] = {
    LINUX_NETDEV_STAT_64("netif_rx_pkts_good", rx_packets),
    LINUX_NETDEV_STAT_64("netif_tx_pkts_good", tx_packets),
    LINUX_NETDEV_STAT_64("netif_rx_bytes_good", rx_bytes),
    LINUX_NETDEV_STAT_64("netif_tx_bytes_good", tx_bytes),
    LINUX_NETDEV_STAT_64("netif_rx_pkts_dropped", rx_dropped),
    LINUX_NETDEV_STAT_64("netif_tx_pkts_dropped", tx_dropped),
    LINUX_NETDEV_STAT_64("netif_rx_pkts_error", rx_errors),
    LINUX_NETDEV_STAT_64("netif_tx_pkts_error", tx_errors),
};
#endif

struct avmnet_etht_stat_entry default_avmnet_stats[DEFAULT_AVMNET_STAT_CNT] = {
    AVMNET_NETDEV_STAT("netif_rx_pkts_good", rx_packets),
    AVMNET_NETDEV_STAT("netif_tx_pkts_good", tx_packets),
    AVMNET_NETDEV_STAT("netif_rx_bytes_good", rx_bytes),
    AVMNET_NETDEV_STAT("netif_tx_bytes_good", tx_bytes),
    AVMNET_NETDEV_STAT("netif_rx_pkts_dropped", rx_dropped),
    AVMNET_NETDEV_STAT("netif_tx_pkts_dropped", tx_dropped),
    AVMNET_NETDEV_STAT("netif_rx_pkts_error", rx_errors),
    AVMNET_NETDEV_STAT("netif_tx_pkts_error", tx_errors),
};


static bool uses_stat_type(enum avmnet_etht_stat_type type,
                           struct avmnet_etht_stat_entry *entries,
                           size_t nr_entries)
{
    size_t i;
    for (i = 0; i < nr_entries; i++) {
        if (entries[i].type == type)
            return true;
    }
    return false;
}


static void gather_stats(avmnet_device_t *avm_dev,
                           struct avmnet_etht_stat_entry *entries,
                           size_t nr_entries,
                           u64 *dst)

{
    struct net_device *netdev = NULL;
    struct avmnet_hw_rmon_counter *hw_stats = NULL;
    struct rtnl_link_stats64 *linux_netdev_stats64 = NULL;
    void *p = NULL;
    size_t i;

    /*
     * gather hw_stats
     */
    if(uses_stat_type(AVMNET_RMON_STATS, entries, nr_entries) &&
       (avm_dev->etht_stat.gather_hw_stats)){
        hw_stats = avm_dev->etht_stat.gather_hw_stats(avm_dev);
        if (!hw_stats) 
            goto free_n_exit;
    }

    /*
     * gather netdev stats
     */
    netdev = avm_dev->device;

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,34)
    if(uses_stat_type(LINUX_NETDEV_STATS_64, entries, nr_entries) &&
       netdev->netdev_ops->ndo_get_stats64){
        linux_netdev_stats64 = kzalloc(sizeof(struct rtnl_link_stats64), GFP_KERNEL);
        if (!linux_netdev_stats64){
            goto free_n_exit;
        }
        netdev->netdev_ops->ndo_get_stats64(netdev, linux_netdev_stats64);
    }
#endif

    /*
     * fill ethtool data struct
     */
    for (i = 0; i < nr_entries; i++){
        switch (entries[i].type) {

        case AVMNET_NETDEV_STATS:
            p = (char *)avm_dev +
                entries[i].stat_offset;
            break;

        case LINUX_NETDEV_STATS:
            p = (char *)netdev +
                entries[i].stat_offset;
            break;

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,34)
        case LINUX_NETDEV_STATS_64:
            p = ((char *)linux_netdev_stats64) +
                entries[i].stat_offset;
            break;
#endif

        case AVMNET_RMON_STATS:
            p = ((char *)hw_stats) +
                entries[i].stat_offset;
            break;
        }

        if (p == NULL) {
            dst[i] = 0;
        }
        else {
            dst[i] = (entries[i].sizeof_stat ==
                       sizeof(u64)) ? *(u64 *)p : *(u32 *)p;
            pr_debug("[%s] dst[%d] = %lld (%p)\n", __func__, i, dst[i], p);
        }
    }

free_n_exit:
    if (hw_stats)
        kfree(hw_stats);

    if (linux_netdev_stats64)
        kfree(linux_netdev_stats64);

}


/*
 * this function is generic and does not rely on a phy module
 */
static void avmnet_eth_get_ethtool_stats(struct net_device *netdev,
                                         struct ethtool_stats *stats __maybe_unused,
                                         u64 *dst)
{
    avmnet_device_t *avm_dev;

    avm_dev = get_avmdev_by_netdev(netdev);
    if(avm_dev == NULL){
        AVMNET_ERR("[%s] Called for unknown device.\n", __func__);
        return;
    }

    pr_debug("[%s] netdev=%s, avm_dev=%s, device_stat_ptr=%p\n", 
             __func__, netdev->name, avm_dev->device_name, &avm_dev->device_stats);


    gather_stats(avm_dev, avm_dev->etht_stat.hw_entries, 
                 avm_dev->etht_stat.hw_cnt, dst);

    gather_stats(avm_dev, avm_dev->etht_stat.sw_entries, avm_dev->etht_stat.sw_cnt, 
                 &dst[avm_dev->etht_stat.hw_cnt]);

}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static int avmnet_eth_flash_device(struct net_device *netdev, struct ethtool_flash *flash)
{
    avmnet_device_t *avm_dev;
    avmnet_module_t *module;

    avm_dev = get_avmdev_by_netdev(netdev);
    if(avm_dev == NULL){
        AVMNET_ERR("[%s] Called for unknown device.\n", __func__);
        return -ENODEV;
    }

    module = avm_dev->ethtool_module_table.flash_device;

    if(module == NULL){
        AVMNET_ERR("[%s] Called but function not registered.\n", __func__);
        return -EFAULT;
    }

    if(module->ethtool_ops.flash_device == NULL){
        AVMNET_ERR("[%s] Function registered for module %s but not implemented.\n", __func__, module->name);
        return -EFAULT;
    }

    return module->ethtool_ops.flash_device(module, flash);
}


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void avmnet_wait_for_link_to_offload_cpu(void){
    struct net_device *netdev = link_dev_to_offload_cpu->device;
    while(!(   ( netdev->flags & IFF_UP )
            && test_bit(__LINK_STATE_START, &netdev->state)
            && test_bit(__LINK_STATE_PRESENT, &netdev->state)
            && ! test_bit(__LINK_STATE_LINKWATCH_PENDING, &netdev->state)
            && ! test_bit(__LINK_STATE_NOCARRIER, &netdev->state)
            && ! test_bit(__LINK_STATE_DORMANT, &netdev->state))){

        if(printk_ratelimit()) {
            pr_err( "Waiting for link to offload device %s, flags=%#x, state=%#lx, rtnl=%d \n",netdev->name, netdev->flags, netdev->state, rtnl_is_locked() );
        }
        schedule();
    }
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
