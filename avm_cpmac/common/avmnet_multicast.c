/*------------------------------------------------------------------------------------------*\
 *   Copyright (C) 2012, 2013 AVM GmbH <fritzbox_info@avm.de>
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


#include <stdarg.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/skbuff.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <net/pkt_sched.h>
#include <avmnet_debug.h>
#include <avmnet_module.h>

#if defined(CONFIG_IP_MULTICAST_FASTFORWARD)
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static void avmnet_mcfw_handle_tasklet(unsigned long data) {
    avmnet_device_t *avm_dev = (avmnet_device_t *) data;
    avmnet_netdev_priv_t *priv = netdev_priv(avm_dev->device);
    struct sk_buff *skb;
    int i = 32;

    while(i-- > 0 && ((skb = skb_dequeue(&priv->mcfw_queue)) != NULL)) {
        dev_queue_xmit(skb);
    }
    if(unlikely(skb_queue_len(&priv->mcfw_queue))) {
        tasklet_schedule(&priv->mcfw_tasklet);
    }
}


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static void avmnet_dev_queue_xmit(avmnet_device_t *avm_dev, struct sk_buff *skb) {
    struct ethhdr *eth = (struct ethhdr *) skb->data;
    struct net_device *dev = avm_dev->device;
    avmnet_netdev_priv_t *priv = netdev_priv(dev);

    memcpy(eth->h_source, dev->dev_addr, ETH_ALEN);
    skb->dev = dev;

    if(irqs_disabled()) {       /* tiatm is source */
        skb_queue_tail(&priv->mcfw_queue, skb);
        tasklet_schedule(&priv->mcfw_tasklet);
    } else {
        dev_queue_xmit(skb);
    }
}


#if 0
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static void avmnet_mc_transmit(void           *privatedata,
                              int             sourceid __attribute__ ((unused)),
                              mcfw_portset    portset,
                              struct sk_buff *skb) {
    unsigned char instance_number = (unsigned char) (((unsigned int) privatedata) & 0xff);
    cpmac_instance_t *instance;
    struct net_device *p_dev;
    struct sk_buff *tskb;
    unsigned short vid;
    t_cpphy_switch_config *switch_config;

    assert(instance_number < AVM_CPMAC_MAX_INSTANCES);
    instance = &cpmac_global.instance[instance_number];

#   if defined(CONFIG_ARCH_PUMA5)
    p_dev = cpmac_puma_get_dev();
#   else /*--- #if defined(CONFIG_ARCH_PUMA5) ---*/
    p_dev = instance->dev;
    switch_config = &instance->mdio.switch_config;
#   endif /*--- #else ---*/ /*--- #if defined(CONFIG_ARCH_PUMA5) ---*/

    if(p_dev == 0) {
        dev_kfree_skb_any(skb);
        return;
    }

    /* MC streams go to the LAN */
    skb->uniq_id &= 0xffffff;
    skb->uniq_id |= (CPPHY_PRIO_QUEUE_LAN << 24);
    assert(portset <= AVM_CPMAC_MAX_PORTSET);
    vid = switch_config->map_portset_out[portset];
    vid = cpmac_global.instance[0].mdio.switch_config.map_portset_out[portset];
    if(vid != 0) {
        if((tskb = __vlan_put_tag(skb, vid)) != 0) {
            avmnet_dev_queue_xmit(p_dev, tskb, instance_number);
        } else {
            dev_kfree_skb_any(skb);
        }
    } else {
        avmnet_dev_queue_xmit(p_dev, skb, instance_number);
    }
}

#endif /*--- #if 0 ---*/

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static void avmnet_mc_transmit_single(void            *privatedata,
                                      int              sourceid __attribute__ ((unused)),
                                      unsigned         n __attribute__ ((unused)),
                                      mcfw_memberinfo *members __attribute__ ((unused)),
                                      struct sk_buff  *skb) {
    avmnet_device_t *avm_dev = (avmnet_device_t *) privatedata;

    if(unlikely(skb == NULL)) {
        AVMNET_ERR("[%s] skb == NULL!\n", __FUNCTION__);
        return;
    }

    avmnet_dev_queue_xmit(avm_dev, skb);
}

/*------------------------------------------------------------------------------------------*\
 * register this device with the multicast fast forward framework and mark it as enabled
\*------------------------------------------------------------------------------------------*/
void avmnet_mcfw_enable(struct net_device *dev)
{
    avmnet_netdev_priv_t *priv;
    int err;

    BUG_ON(dev == NULL);

    priv = netdev_priv(dev);

    if(priv->mcfw_init_done == 1){
        if(priv->mcfw_used == 0){
            AVMNET_TRC("[%s] Enabling mcfw driver for device %s\n", __func__, dev->name);

            err = mcfw_netdriver_register(&priv->mcfw_netdrv);
            if(err){
                AVMNET_ERR("[%s] Could not register mcfw_netdriver!\n", __FUNCTION__);
            } else {
                priv->mcfw_used = 1;
            }
        } else {
            AVMNET_WARN("[%s] Device %s was enabled already\n", __func__, dev->name);
        }
    } else {
        AVMNET_ERR("[%s] Device %s has not been initialised!\n", __func__, dev->name);
    }
}
EXPORT_SYMBOL(avmnet_mcfw_enable);

/*------------------------------------------------------------------------------------------*\
 * remove this device from the multicast fast forward framework and mark it as disabled
\*------------------------------------------------------------------------------------------*/
void avmnet_mcfw_disable(struct net_device *dev)
{
    avmnet_netdev_priv_t *priv;

    BUG_ON(dev == NULL);

    priv = netdev_priv(dev);

    if(priv->mcfw_init_done == 1){
        if(priv->mcfw_used == 1){
            AVMNET_TRC("[%s] Disabling mcfw driver for device %s\n", __func__, dev->name);

            priv->mcfw_used = 0;
            mcfw_netdriver_unregister(&priv->mcfw_netdrv);
        } else {
            AVMNET_WARN("[%s] Device %s was disabled already\n", __func__, dev->name);
        }
    } else {
        AVMNET_ERR("[%s] Device %s has not been initialised!\n", __func__, dev->name);
    }
}
EXPORT_SYMBOL(avmnet_mcfw_disable);

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_mcfw_init(avmnet_device_t *avm_dev) {

    avmnet_netdev_priv_t *priv;
    struct net_device *dev = avm_dev->device;
    int err = 0;

    if (!dev)
        return err;

    if(avm_dev->flags & AVMNET_DEVICE_FLAG_NO_MCFW){
        return 0;
    }

    priv = netdev_priv(dev);

    AVMNET_INFO("[%s] Register multicast forward for '%s', priv=%p,priv->mcfw_used=%d \n", __FUNCTION__, dev->name,priv, priv->mcfw_used );
    if( priv && (priv->mcfw_init_done == 0) ) {
        strncpy(priv->mcfw_netdrv.name, dev->name, MCFWDRV_NAMSIZ - 1);
        priv->mcfw_sourceid = 0;
        priv->mcfw_netdrv.netdriver_mc_transmit_single = avmnet_mc_transmit_single;
        priv->mcfw_netdrv.privatedata = (void *) avm_dev;

        skb_queue_head_init(&priv->mcfw_queue);
        tasklet_init(&priv->mcfw_tasklet, avmnet_mcfw_handle_tasklet, (unsigned long) avm_dev);

#if 0
        err = mcfw_netdriver_register(&priv->mcfw_netdrv);
        if(err) {
            AVMNET_WARN("[%s] Could not register mcfw_netdriver!\n", __FUNCTION__);
            return err;
        }
#endif

        priv->mcfw_init_done = 1;
        priv->mcfw_sourceid = mcfw_multicast_forward_alloc_id(priv->mcfw_netdrv.name);
        AVMNET_INFO("[%s] Register multicast forward for '%s'\n",
                    __FUNCTION__, dev->name);
    }

    return err;
}


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void avmnet_mcfw_exit(avmnet_device_t *avm_dev) {
    struct sk_buff *skb;
    avmnet_netdev_priv_t *priv;
    struct net_device *dev = avm_dev->device;

    if (!dev)
        return;

    priv = netdev_priv(dev);

    if(priv->mcfw_init_done){
        if(priv->mcfw_used) {
            priv->mcfw_used = 0;
            mcfw_netdriver_unregister(&priv->mcfw_netdrv);
        }

        mcfw_multicast_forward_free_id(priv->mcfw_sourceid);
        tasklet_kill(&priv->mcfw_tasklet);
        while((skb = skb_dequeue(&priv->mcfw_queue)) != NULL) {
            dev_kfree_skb_any(skb);
        }

        priv->mcfw_init_done = 0;

        AVMNET_INFO("[%s] done\n", __FUNCTION__);
    }
}

#endif /*--- #if defined(CONFIG_IP_MULTICAST_FASTFORWARD) ---*/

