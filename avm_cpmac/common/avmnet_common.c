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
#include <linux/list.h>
#include <linux/string.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/proc_fs.h>
#include <linux/avm_hw_config.h>
#include <linux/env.h>
#include <asm/prom.h>
#include <avmnet_common.h>
#include <avmnet_debug.h>
#include <avmnet_module.h>
#include <avmnet_config.h>
#include <linux/ethtool.h>

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_set_macaddr(int device_nr, struct net_device *dev) {

    struct sockaddr addr;
    unsigned int a[6];
    char *p = NULL;
    char *urlader_name = NULL;

    switch (device_nr) {
        case 0:
        case 1:
        case 2:
        case 3:
            urlader_name = "maca";
            break;
        case 4:
            urlader_name = "macdsl";
            break;
        case -1:
		    p = "00:de:ad:be:ef:ca";      /*--- wird nur intern verwendet z.B. 7490 WASP ---*/
            break;
        case -2:
		    p = "00:de:ad:be:ef:cb";      /*--- wird nur intern verwendet z.B. 5490 Switch ---*/
            break;
        default:
            return -EINVAL;
    }

    if ( urlader_name ) {
        p = prom_getenv(urlader_name);
        if (p == NULL) {
            return -EINVAL;
        }
    }

    sscanf(p, "%x:%x:%x:%x:%x:%x", &a[0], &a[1], &a[2], &a[3], &a[4], &a[5]);
    addr.sa_data[0] = (char) (a[0]);
    addr.sa_data[1] = (char) (a[1]);
    addr.sa_data[2] = (char) (a[2]);
    addr.sa_data[3] = (char) (a[3]);
    addr.sa_data[4] = (char) (a[4]);
    addr.sa_data[5] = (char) (a[5]);

    if (!is_valid_ether_addr(addr.sa_data)) {
        printk(KERN_ERR "[%s] invalid addr %02x:%02x:%02x:%02x:%02x:%02x (%s) for device '%s' \n", __FUNCTION__, a[0], a[1], a[2], a[3], a[4], a[5] , urlader_name, dev->name);
        return -EINVAL;
    }

    memcpy(dev->dev_addr, addr.sa_data, dev->addr_len);

    printk(KERN_ERR "[%s] Setup Mac Addr for Device(%s): %02x:%02x:%02x:%02x:%02x:%02x \n",
            __FUNCTION__, dev->name, a[0], a[1], a[2], a[3], a[4], a[5]);

    return 0;

}

/*---------------------------------------------------------------------------*\
\*---------------------------------------------------------------------------*/

int avmnet_generic_init(avmnet_module_t *this)
{
    avmnet_module_t *child;
    int ret;
    int i;

    pr_debug("%s: initialising module %s.\n", __func__, this->name);

    for (i = ret = 0; i < this->num_children; i++) {
        child = this->children[i];

        if (!child->init) {
            pr_emerg("%s: %s: child %d (%s) has no init function!\n",
                     __func__, this->name, i, child->name);
            ret |= ENOSYS;
            ret |= avmnet_generic_init(child);
        }
        ret |= child->init(child);
    }

    return ret;
}

/*---------------------------------------------------------------------------*\
\*---------------------------------------------------------------------------*/

int avmnet_generic_setup(avmnet_module_t *this)
{
    avmnet_module_t *child;
    int ret;
    int i;

    pr_debug("%s: %s, by %pF\n", __func__, this->name, (void *)_RET_IP_);

    for (i = ret = 0; i < this->num_children; i++) {
        child = this->children[i];

        if (!child->setup) {
            pr_emerg("%s: %s: child %d (%s) has no setup function!\n",
                     __func__, this->name, i, child->name);
            ret |= ENOSYS;
            ret |= avmnet_generic_setup(child);
        }
        ret |= child->setup(child);
    }

    avmnet_cfg_register_module(this);
    pr_debug("%s: done - registering new module %s \n", __func__, this->name);

    return ret;
}

/*---------------------------------------------------------------------------*\
\*---------------------------------------------------------------------------*/

int avmnet_generic_exit(avmnet_module_t *this __maybe_unused)
{
    avmnet_module_t *child;
    int ret;
    int i;

    for (i = ret = 0; i < this->num_children; i++) {
        child = this->children[i];

        if (!child->exit) {
            pr_emerg("%s: %s: child %d (%s) has no exit function!\n",
                     __func__, this->name, i, child->name);
            ret |= ENOSYS;
            ret |= avmnet_generic_exit(child);
        }
        ret |= child->exit(child);
    }

    pr_debug("%s: exiting module %s.\n", __func__, this->name);

    avmnet_cfg_unregister_module(this);

    return ret;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,37)
DECLARE_MUTEX(avmnet_timer_sema);
#else
DEFINE_SEMAPHORE(avmnet_timer_sema);
#endif

static avmnet_timer_t avmnet_timer;

static int avmnet_timer_init(void) {

    sema_init(&avmnet_timer_sema, 1);

    INIT_LIST_HEAD(&avmnet_timer.list);
    avmnet_timer.workqueue = create_singlethread_workqueue("avmnet_timer");
    if(avmnet_timer.workqueue == NULL) {
        printk(KERN_ERR "[%s] create_singlethread_workqueue(\"avmnet_timer\" failed\n", __FUNCTION__);
        return -EFAULT;
    }
    return 0;
}

late_initcall(avmnet_timer_init);

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static void avmnet_timer_work(struct work_struct *work) {

    avmnet_timer_t *item, *timerhead = container_of(to_delayed_work(work), avmnet_timer_t, work);
    int queue_next = 0;
    
    if (!down_interruptible(&avmnet_timer_sema)) {
        list_for_each_entry(item, &timerhead->list, list) {
            AVMNET_DEBUG("{%s} module %s\n", __func__, item->module->name);
            queue_next = 1;

            switch (item->type) {
                case none:
                    AVMNET_DEBUG("{%s} type none %s\n", __func__, item->module->name);
                    break;
                case poll:
                    AVMNET_DEBUG("{%s} poll %s\n", __func__, item->module->name);
                    if (item->module->poll)
                        item->module->poll(item->module);
                    break;
            }
        }
        up(&avmnet_timer_sema);
    }

    if (queue_next) {
        AVMNET_DEBUG("{%s} queue new timer\n", __func__);
        queue_delayed_work(timerhead->workqueue, &timerhead->work, CONFIG_HZ * 10);
    } 

}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_timer_add(avmnet_module_t *module, avmnet_timer_enum_t type) {

    unsigned int setup_handler = 0;
    avmnet_timer_t  *timer_item = kzalloc(sizeof(avmnet_timer_t), GFP_KERNEL);

    printk(KERN_DEBUG "{%s} module %s\n", __func__, module->name);
    if(!timer_item) {
        AVMNET_ERR("{%s} <no memory>\n", __func__);
        return -ENOMEM;
    }

    timer_item->module = module;
    timer_item->type = type;

    if (!down_interruptible(&avmnet_timer_sema)) {
        if(list_empty(&avmnet_timer.list)) {    
            /*--- kein Element - den Handler aufsetzen ---*/
            AVMNET_DEBUG("{%s} kein Element vorhanden\n", __func__);
            setup_handler = 1;
        }
        list_add(&timer_item->list, &avmnet_timer.list);
    
        if(setup_handler) {
            AVMNET_DEBUG("{%s} init timer\n", __func__);
            INIT_DELAYED_WORK(&avmnet_timer.work, avmnet_timer_work);
            queue_delayed_work(avmnet_timer.workqueue, &avmnet_timer.work, CONFIG_HZ * 10);
        }

        up(&avmnet_timer_sema);

        return 0;
    }
    kfree(timer_item);
    return 1;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_timer_del(avmnet_module_t *module, avmnet_timer_enum_t type __attribute__((unused))) {

    avmnet_timer_t *timer = &avmnet_timer;
    avmnet_timer_t *item;

    if (!down_interruptible(&avmnet_timer_sema)) {
        
        AVMNET_DEBUG("{%s} delete timer\n", __func__);
        list_for_each_entry(item, &timer->list, list) {

            AVMNET_DEBUG("{%s} found timer\n", __func__);
            if (item->module == module) {
                list_del(&item->list);
                kfree(item);
                break;
            }
        }
        up(&avmnet_timer_sema);

        return 0;
    }
    return 1;
}


/*------------------------------------------------------------------------------------------*\
 * From a given string extract a list of pointers delimited by delim
\*------------------------------------------------------------------------------------------*/
unsigned int avmnet_tokenize(char          *str, 
                             const char    *delim,
                             unsigned int   max_tokens,
                             char         **tokens) {
    unsigned int number_of_tokens;

    number_of_tokens = 0;
    do {
        tokens[number_of_tokens] = strsep(&str, delim);
        if(tokens[number_of_tokens] == NULL) {
            break;
        }
        if(0 == strlen(tokens[number_of_tokens])) {
            continue;
        }
        number_of_tokens++;
    } while(number_of_tokens < max_tokens);

    return number_of_tokens;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

unsigned int calc_advertise_pause_param( unsigned int mac_support_sym, unsigned int mac_support_asym, enum avmnet_pause_setup pause_setup){
    unsigned int advertise  = 0;


    switch(pause_setup){
        case avmnet_pause_rx:
            if ( mac_support_asym ){
                /*
                 * there is no way to advertise rx-pause only
                 * we have to advertise both and disable tx later on
                */
                advertise  |= ADVERTISED_Pause;
                advertise  |= ADVERTISED_Asym_Pause;
            }
            break;

        case avmnet_pause_tx:
            if ( mac_support_asym ){
                advertise  |= ADVERTISED_Asym_Pause;
            }
            break;

        case avmnet_pause_rx_tx:
            if ( mac_support_asym ){
                advertise  |= ADVERTISED_Pause;
                advertise  |= ADVERTISED_Asym_Pause;
            }
            else if ( mac_support_sym ){
               advertise |= ADVERTISED_Pause;
            }

            break;

        case avmnet_pause_none:
        default:
            advertise = 0;
            break;

    }
    return advertise;
}

/*------------------------------------------------------------------------------------------*\
 * Steuern der GPIOs die an den Phys angeschlossen sind
 *  0 -  7  eth0
 *  7 - 15  eth1
 * 16 - 23  eth2
 * 24 - 31  eth3
 * 32 - 39  eth4
 *  ....
\*------------------------------------------------------------------------------------------*/
int avmnet_gpio(unsigned int gpio, unsigned int on) {
    unsigned int device_index = gpio / 8;
    avmnet_module_t *module = NULL;
    static avmnet_device_t *dev_lookup[8]    = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
    static avmnet_module_t *module_lookup[8] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
    int i;

    AVMNET_DEBUG("{%s} gpio %d on %d\n", __func__, gpio, on);

    if(avmnet_hw_config_entry == NULL) {
        return -EIO;
    }

    if (device_index > 7)
        return -EIO;

    AVMNET_DEBUG("{%s} device_index %d\n", __func__, device_index);

    if(dev_lookup[device_index] == NULL) {  /* das richtige Device ermitteln */
        for(i = 0 ; i < avmnet_hw_config_entry->nr_avm_devices ; i++) {
            static const char *eth_names[8] = { "eth0", "eth1", "eth2", "eth3", NULL, NULL, NULL, "plc" };
            avmnet_device_t *dev = avmnet_hw_config_entry->avm_devices[i];
            if(eth_names[device_index] && !strcmp(dev->device_name, eth_names[device_index])) {
                dev_lookup[device_index] = dev;
                break;
            }
        }
    }
    if(dev_lookup[device_index] == NULL) {
        return -EIO;
    }
    module = dev_lookup[device_index]->mac_module;
    if(module == NULL) {
        return -EIO;  /* kein zuständiges Modul gefunden */
    }
    if(module_lookup[device_index] == NULL) {
        /* nach unten durchhangeln */
        while(module->num_children) {
            module = module->children[0]; /* jeweils das erste Child innerhalb eines Devices ist für die GPIOs zuständig */
        }
        module_lookup[device_index] = module;
    }
    if(module_lookup[device_index] == NULL) {
        return -EIO;
    }
    if(module_lookup[device_index]->set_gpio) {
        return module_lookup[device_index]->set_gpio(module_lookup[device_index], gpio & 0xFF, on);
    }
    return -EIO;
}

EXPORT_SYMBOL(avmnet_gpio);

struct net_device_stats * avmnet_generic_get_stats(struct net_device *dev) {
    avmnet_device_t **avm_devp = netdev_priv(dev);
    return &(*avm_devp)->device_stats;
}
