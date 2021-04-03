/******************************************************************************
**
** FILE NAME    : ltq_eth_oam_handler.c
** PROJECT      : UGW
** MODULES      : Ethernet OAM
**
** DATE         : 5 APRIL 2013
** AUTHOR       : Purnendu Ghosh
** DESCRIPTION  : Driver for Ethernet OAM handling
** COPYRIGHT    :   Copyright (c) 2013
**                LANTIQ DEUTSCHLAND GMBH,
**                Lilienthalstrasse 15, 85579 Neubiberg, Germany
**
**    This program is free software; you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation; either version 2 of the License, or
**    (at your option) any later version.
**
** HISTORY
** $Date        $Author                $Comment
** 5 APRIL 2013 PURNENDU GHOSH         Initiate Version
*******************************************************************************/

/*
 * ####################################
 *              Version No.
 * ####################################
 */

#define VER_MAJOR       0
#define VER_MID         0
#define VER_MINOR       3 /* modified by AVM */

/*
 * ####################################
 *            Header Files
 * ####################################
 */

/*
 *  Common Header Files
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/byteorder/generic.h>
#include <linux/skbuff.h>
#include <linux/if.h>
#include <linux/if_ether.h>
#include <linux/netdevice.h>

#ifdef CONFIG_IFX_PPA_AVM_USAGE
#undef CONFIG_IFX_ETH_FRAMEWORK
#endif

#include <avm/net/ltq_eth_oam_handler.h>

/*
 *  Chip Specific Header Files
 */

#ifdef CONFIG_IFX_ETH_FRAMEWORK
#include <ifx_eth_framework.h>
#endif

/*
 * ####################################
 *          Compatibility
 * ####################################
 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 11, 0)
#define netdev_notifier_info_to_dev(ptr)   ((struct net_device *)(ptr))
#endif/*--- #if LINUX_VERSION_CODE < KERNEL_VERSION(3, 11, 0) ---*/

/*
 * ####################################
 *   Parameters to Configure PPE
 * ####################################
 */

static char* ethwan = "eth1"; /* underlying real device */

module_param(ethwan, charp, S_IRUGO);
MODULE_PARM_DESC(ethwan, "wan device");

#define ETH_WATCHDOG_TIMEOUT (10 * HZ)

/*
 * ####################################
 *            Declarations
 * ####################################
 */
/*
 *  Network Operations
 */

static struct net_device_stats *eth_oam_get_stats(struct net_device *);
static int                      eth_oam_open(struct net_device *);
static int                      eth_oam_stop(struct net_device *);
static int                      eth_oam_hard_start_xmit(struct sk_buff *, struct net_device *);
static int                      eth_oam_set_mac_address(struct net_device *, void *);
static int                      eth_oam_ioctl(struct net_device *, struct ifreq *, int);
static void                     eth_oam_tx_timeout(struct net_device *);
static int                      eth_oam_cb_netdev_event(struct notifier_block *, unsigned long, void *);

/*
 * ####################################
 *           Local Variables
 * ####################################
 */

static struct net_device     *g_eth_oam_netdev          = NULL;
static struct net_device     *g_wan_netdev              = NULL;

static const char            *g_eth_oam_name            = "eoam";
static const unsigned char    g_eth_oam_addr[ETH_ALEN]  = { 0x00, 0x20, 0xda, 0x86, 0x23, 0x74 };

static struct notifier_block  g_eth_oam_netdev_notifier = {
    .notifier_call = eth_oam_cb_netdev_event,
};

#ifdef CONFIG_IFX_ETH_FRAMEWORK
struct ifx_eth_fw_netdev_ops g_eth_oam_ops = {
    .get_stats       = eth_oam_get_stats,
    .open            = eth_oam_open,
    .stop            = eth_oam_stop,
    .start_xmit      = eth_oam_hard_start_xmit,
    .set_mac_address = eth_oam_set_mac_address,
    .do_ioctl        = eth_oam_ioctl,
    .tx_timeout      = eth_oam_tx_timeout,
    .watchdog_timeo  = ETH_WATCHDOG_TIMEOUT,
};
#else
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)
struct net_device_ops g_eth_oam_ops = {
    .ndo_get_stats       = eth_oam_get_stats,
    .ndo_open            = eth_oam_open,
    .ndo_stop            = eth_oam_stop,
    .ndo_start_xmit      = eth_oam_hard_start_xmit,
    .ndo_set_mac_address = eth_oam_set_mac_address,
    .ndo_do_ioctl        = eth_oam_ioctl,
    .ndo_tx_timeout      = eth_oam_tx_timeout,
};
#endif
#endif

/*
 * ####################################
 *           Local Functions
 * ####################################
 */

static int eth_oam_cb_netdev_event(struct notifier_block *nb, unsigned long event, void *ptr) {
    struct net_device *dev = netdev_notifier_info_to_dev(ptr);

    (void)nb; /* avoid compiler warning */

    switch (event) {
        case NETDEV_UNREGISTER:
            /*
             * Ensure that the reference to the WAN device is released in case
             * that the module is not removed.
             */
            if ((g_wan_netdev != NULL) && (g_wan_netdev == dev)) {
                dev_put(g_wan_netdev);
                g_wan_netdev = NULL;
            }
            break;

        case NETDEV_REGISTER:
            /*
             * If the WAN device was not present during the module
             * initialization or if it was unregistered during the lifetime of
             * the module, make it available now.
             */
            if ((g_wan_netdev == NULL) && (strcmp(dev->name, ethwan) == 0)) {
                g_wan_netdev = dev_get_by_name(&init_net, ethwan);
            }
            break;
    }
    return NOTIFY_DONE;
}

struct net_device *eth_oam_get_netdev(void)
{
    return g_eth_oam_netdev;
}

#ifdef ETH_OAM_SKB_DUMP_ENABLE
static inline void eth_oam_skb_dump(struct sk_buff *skb, unsigned int len)
{
    unsigned int i;

    if (skb->len < len) {
        len = skb->len;
    }

    if (len > 1600) {
        printk("eth_oam driver: Too big data length: skb = %08x, skb->data = %08x, skb->len = %d\n", (unsigned int)skb, (unsigned int)skb->data, skb->len);
        return;
    }

    for (i = 1; i <= len; i++) {
        if ( i % 16 == 1 ) {
            printk("  %4d:", i - 1);
        }
        printk(" %02X", (int)(*((char*)skb->data + i - 1) & 0xFF));
        if ( i % 16 == 0 ) {
            printk("\n");
        }
    }
    if ( (i - 1) % 16 != 0 ) {
        printk("\n");
    }
}
#endif

static struct net_device_stats *eth_oam_get_stats(struct net_device *dev)
{
    return &dev->stats;
}

static int eth_oam_open(struct net_device *dev)
{
    printk(KERN_INFO "Open eth_oam driver dev->name %s\n", dev->name);

#ifndef CONFIG_IFX_ETH_FRAMEWORK
    netif_start_queue(dev);
#endif

    return 0;
}

static int eth_oam_stop(struct net_device *dev)
{
#ifdef CONFIG_IFX_ETH_FRAMEWORK
    (void)dev; /* avoid compiler warning */
#else
    netif_stop_queue(dev);
#endif

    return 0;
}

static int eth_oam_hard_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
    struct ethhdr *eth;

    if (!skb) {
        printk(KERN_ERR "eth_oam driver: invalid skb\n");
    }
    else {
        skb_set_mac_header(skb, 0);
        eth = eth_hdr(skb);
#ifdef ETH_OAM_SKB_DUMP_ENABLE
        eth_oam_skb_dump(skb, 100);
#endif
        if (((eth->h_proto == __constant_htons(ETH_P_CFM))
         ||  (eth->h_proto == __constant_htons(ETH_P_SLOW))
         ||  (eth->h_proto == __constant_htons(ETH_P_8021Q)))
         && g_wan_netdev) {
            skb->dev = g_wan_netdev;
            dev_queue_xmit(skb);
            if (dev) {
                dev->stats.tx_packets++;
                dev->stats.tx_bytes += skb->len;
            }
        }
        else {
            dev_kfree_skb(skb);
            if (dev) {
                dev->stats.tx_dropped++;
            }
        }
    }

    /* always return NETDEV_TX_OK for this virtual device */
    return NETDEV_TX_OK;
}

static int eth_oam_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
    /* avoid compiler warnings */
    (void)dev;
    (void)ifr;
    (void)cmd;

    return 0;
}

static void eth_oam_tx_timeout(struct net_device *dev)
{
    dev->stats.tx_errors++;
    netif_wake_queue(dev);
}

static int eth_oam_set_mac_address(struct net_device *dev, void *p)
{
    struct sockaddr *addr = (struct sockaddr *)p;

    printk(KERN_INFO "%s: change MAC from %02X:%02X:%02X:%02X:%02X:%02X to %02X:%02X:%02X:%02X:%02X:%02X", dev->name,
        (unsigned int)dev->dev_addr[0] & 0xFF, (unsigned int)dev->dev_addr[1] & 0xFF, (unsigned int)dev->dev_addr[2] & 0xFF, (unsigned int)dev->dev_addr[3] & 0xFF, (unsigned int)dev->dev_addr[4] & 0xFF, (unsigned int)dev->dev_addr[5] & 0xFF,
        (unsigned int)addr->sa_data[0] & 0xFF, (unsigned int)addr->sa_data[1] & 0xFF, (unsigned int)addr->sa_data[2] & 0xFF, (unsigned int)addr->sa_data[3] & 0xFF, (unsigned int)addr->sa_data[4] & 0xFF, (unsigned int)addr->sa_data[5] & 0xFF);

    memcpy(dev->dev_addr, addr->sa_data, dev->addr_len);

    return 0;
}

/*
 * ####################################
 *           Init/Cleanup API
 * ####################################
 */

static int __init eth_oam_init(void)
{
    int ret = 0;
    unsigned int i;

    printk(KERN_INFO "Loading eth_oam driver for WAN interface %s\n", ethwan);

#ifdef CONFIG_IFX_ETH_FRAMEWORK
    g_eth_oam_netdev = ifx_eth_fw_alloc_netdev(0, g_eth_oam_name, &g_eth_oam_ops);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(3,17,0)
    g_eth_oam_netdev = alloc_netdev(0, g_eth_oam_name, NET_NAME_UNKNOWN, ether_setup);
#else
    g_eth_oam_netdev = alloc_netdev(0, g_eth_oam_name, ether_setup);
#endif

    if (g_eth_oam_netdev == NULL) {
        ret = -ENOMEM;
    }
    else {
#ifndef CONFIG_IFX_ETH_FRAMEWORK
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)
        g_eth_oam_netdev->netdev_ops      = &g_eth_oam_ops;
#else
        g_eth_oam_netdev->get_stats       = eth_oam_get_stats;
        g_eth_oam_netdev->open            = eth_oam_open;
        g_eth_oam_netdev->stop            = eth_oam_stop;
        g_eth_oam_netdev->hard_start_xmit = eth_oam_hard_start_xmit;
        g_eth_oam_netdev->set_mac_address = eth_oam_set_mac_address;
        g_eth_oam_netdev->do_ioctl        = eth_oam_ioctl;
        g_eth_oam_netdev->tx_timeout      = eth_oam_tx_timeout;
#endif
        g_eth_oam_netdev->watchdog_timeo  = ETH_WATCHDOG_TIMEOUT;
#endif
        /* set MAC address */
        for (i = 0; i < ETH_ALEN; i++) {
            g_eth_oam_netdev->dev_addr[i] = g_eth_oam_addr[i];
        }

#ifdef CONFIG_IFX_ETH_FRAMEWORK
        ret = ifx_eth_fw_register_netdev(g_eth_oam_netdev);
#else
        ret = register_netdev(g_eth_oam_netdev);
#endif
        if (ret != 0) {
#ifdef CONFIG_IFX_ETH_FRAMEWORK 
            ifx_eth_fw_free_netdev(g_eth_oam_netdev, 0);
#else
            free_netdev(g_eth_oam_netdev);
#endif
            g_eth_oam_netdev = NULL;
        }
        else {
            fp_ltq_eth_oam_dev = eth_oam_get_netdev;
            g_wan_netdev = dev_get_by_name(&init_net, ethwan);
            if (g_wan_netdev == NULL) {
                printk(KERN_WARNING "eth_oam driver: given ethwan device not present; waiting for its registration\n");
            }
            register_netdevice_notifier(&g_eth_oam_netdev_notifier);
        }
    }

    printk(KERN_INFO "Init eth_oam driver %s\n", (ret != 0) ? "failed" : "succeeded");

    return ret;
}

static void __exit eth_oam_exit(void)
{
    unregister_netdevice_notifier(&g_eth_oam_netdev_notifier);

    /* release reference to WAN device */
    if (g_wan_netdev) {
        dev_put(g_wan_netdev);
        g_wan_netdev = NULL;
    }

#ifdef CONFIG_IFX_ETH_FRAMEWORK
    ifx_eth_fw_unregister_netdev(g_eth_oam_netdev, 0);
    ifx_eth_fw_free_netdev(g_eth_oam_netdev, 0);
#else
    unregister_netdev(g_eth_oam_netdev);
    free_netdev(g_eth_oam_netdev);
#endif
    g_eth_oam_netdev = NULL;
    fp_ltq_eth_oam_dev = NULL;
    printk(KERN_INFO "Bye eth_oam driver\n");
}

module_init(eth_oam_init);
module_exit(eth_oam_exit);

MODULE_LICENSE("GPL");
