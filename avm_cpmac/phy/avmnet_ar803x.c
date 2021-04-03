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
#include <linux/delay.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/types.h>
#include <linux/ethtool.h>
#include <linux/avm_hw_config.h>

#include "avmnet_debug.h"
#include "avmnet_config.h"
#include "avmnet_module.h"
#include "avmnet_common.h"
#include "avmnet_ar803x.h"
#include "athrs_phy_ctrl.h"
#include "mdio_reg.h"

#undef INFINEON_CPU
#if defined(CONFIG_VR9) || defined(CONFIG_AR9) || defined(CONFIG_AR10)
#define INFINEON_CPU
#define GPIO_REGISTER_BROKEN
#include <ifx_gpio.h>
#include <ifx_types.h>
#endif

#if defined(CONFIG_MACH_ATHEROS) || defined(CONFIG_ATH79)
#include <asm/mach_avm.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,4,46)
#include <atheros_gpio.h>
#else
#include <avm_atheros_gpio.h>
#endif
#endif

#define MODULE_NAME "AVMNET_AR8035"

#define AR803x_PHY_ID1 0x004D
#define AR803x_PHY_ID2 0xD072
#define AR8326_PHY_ID2 0xD042

/* Special mdio registers for 8033 in fiber mode */
#define MDIO_FIBER_AUTONEG_ADV                 0x04u
#  define MDIO_FIBER_AUTONEG_ADV_NEXT_PAGE     (1u << 15)
#  define MDIO_FIBER_AUTONEG_ADV_ACK           (1u << 14)
#  define MDIO_FIBER_AUTONEG_ADV_REMOTE_FAULT_SHIFT 12
#  define MDIO_FIBER_AUTONEG_ADV_REMOTE_FAULT_MASK  (3u << MDIO_FIBER_AUTONEG_ADV_REMOTE_FAULT_SHIFT)
#  define MDIO_FIBER_AUTONEG_ADV_REMOTE_FAULT(x)    (((x) & MDIO_FIBER_AUTONEG_ADV_REMOTE_FAULT_MASK) << MDIO_FIBER_AUTONEG_ADV_REMOTE_FAULT_SHIFT)
#  define MDIO_FIBER_AUTONEG_ADV_RESERVED_2    (1u <<  9)
#  define MDIO_FIBER_AUTONEG_ADV_PAUSE_ASYM    (1u <<  8)
#  define MDIO_FIBER_AUTONEG_ADV_PAUSE_SYM     (1u <<  7)
#  define MDIO_FIBER_AUTONEG_ADV_1000B_X_HD    (1u <<  6)
#  define MDIO_FIBER_AUTONEG_ADV_1000B_X_FD    (1u <<  5)
#  define MDIO_FIBER_AUTONEG_ADV_RESERVED      (1u <<  0)

#define SUSPEND_PARENT(this) do{ \
                                 if(this->parent->suspend){ \
                                     this->parent->suspend(this->parent, this); \
                                 } \
                             }while(0);

#define RESUME_PARENT(this) do{ \
                                if(this->parent->resume){ \
                                    this->parent->resume(this->parent, this); \
                                } \
                            }while(0);

#if defined(CONFIG_MACH_ATHEROS) || defined(CONFIG_ATH79)
static struct resource ar803x_gpio ={.name = "ar803x_reset",
                                     .flags = IORESOURCE_IO,
                                     .start = -1,
                                     .end = -1};
#endif // CONFIG_MACH_ATHEROS != n
static int set_config(avmnet_module_t *this);
static int setup_phy(avmnet_module_t *this);
static void reset_phy(avmnet_module_t *this, enum rst_type type);
static avmnet_linkstatus_t get_status(avmnet_module_t *this);

static int phy_reg_open(struct inode *inode, struct file *file);
static int phy_reg_show(struct seq_file *seq, void *data __attribute__ ((unused)));
static ssize_t phy_reg_write(struct file *filp, const char __user *buff, size_t len,
        loff_t *offset);

static const struct file_operations phy_reg_fops = {
        .open = phy_reg_open,
        .read = seq_read,
        .write = phy_reg_write,
        .llseek = seq_lseek,
        .release = single_release 
};

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static void link_work_handler(struct work_struct *data)
{
    struct athphy_context *phy;
    avmnet_module_t *this;
    avmnet_module_t *parent;
    avmnet_linkstatus_t status;

    phy = container_of(
            container_of(to_delayed_work(data), avmnet_work_t, work),
            struct athphy_context, link_work);

    this = (avmnet_module_t *) phy->this_module;
    parent = this->parent;

//    AVMNET_ERR("[%s] Called for module %s\n", __func__, this->name);

    status = get_status(this);
    parent->set_status(parent, this->device_id, status);

}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static irqreturn_t ar803x_link_intr(int irq __attribute__((unused)), void *dev)
{
    avmnet_module_t *this = (avmnet_module_t *) dev;
    struct athphy_context *phy = (struct athphy_context *) this->priv;
    unsigned long flags;

    spin_lock_irqsave(&(phy->irq_lock), flags);

    AVMNET_DEBUG("[%s] device %s irq %d\n", __func__, this->name, phy->irq);
    if(!phy->irq_disabled){
        AVMNET_DEBUG("[%s] device %s disabling IRQ\n", __func__, this->name);
        disable_irq_nosync(phy->irq);
        phy->irq_disabled++;
    }

    queue_delayed_work(phy->link_work.workqueue, &(phy->link_work.work), 0);

    spin_unlock_irqrestore(&(phy->irq_lock), flags);

    return IRQ_HANDLED;
}

#define STATE_NAME(phy)      (phy)->state == LINK_DOWN ?    "down" : \
                             (phy)->state == LINK_UP ?      "up" : \
                             (phy)->state == LINK_LOST ?    "lost" : \
                             (phy)->state == LINK_FAILED ?  "failed" : \
                             (phy)->state == LINK_PWRDOWN ? "pwrdown" : "invalid"

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static avmnet_linkstatus_t check_linkfail(avmnet_module_t *this,
                                          avmnet_linkstatus_t status,
                                          unsigned int phyStatus)
{
    struct athphy_context *phy = (struct athphy_context *) this->priv;
    unsigned int IntStatus, phyHwStatus, link_lost;

    /*
     * get and clear global link interrupt, register is clear-on-read
     */
    IntStatus = avmnet_mdio_read(this, ATHR_PHY_INTR_STATUS);
    phyHwStatus = avmnet_mdio_read(this, ATHR_PHY_SPEC_STATUS);

    /*
     * are link status fix-ups configured?
     */
    if(!(this->initdata.phy.flags
         & (AVMNET_CONFIG_FLAG_RST_ON_LINKFAIL | AVMNET_CONFIG_FLAG_LINKFAIL_TIME)))
    {
        return status;
    }

    AVMNET_TRC("[%s] Phy %s: old state: %s\n", __func__, this->name, STATE_NAME(phy));

    AVMNET_TRC("[%s] Phy %s status: link: %d, speed: %d, powerup: %d\n", __func__, this->name,
            status.Details.link, status.Details. speed, status.Details.powerup);

    /*
     * reset thrash monitor variables if link has been stable (up or down) for the
     * configured time
     */
    if(time_is_before_eq_jiffies(phy->thrash_time + (LINK_THRASH_TIME * HZ))){
        phy->thrash_time = jiffies;
        phy->thrash_count = 0;
    }

    /*
     * link fail has been detected earlier and upper modules should have stopped
     * sending frames. Now we can hard reset the PHY.
     */
    if(phy->state == LINK_FAILED){
        /*
         * only reset PHY if the thrash limit has not been met
         */
        if(this->initdata.phy.flags & AVMNET_CONFIG_FLAG_RST_ON_LINKFAIL){
            if(phy->thrash_count < LINK_THRASH_LIMIT){
                AVMNET_ERR("[%s] Link fail detected on broken PHY %s, re-initialising.\n", __func__, this->name);
                setup_phy(this);
                phy->last_status.Status = 0;
                status.Status = 0;
                status.Details.powerup = phy->powerdown ? 0 : 1;
            }else{
                AVMNET_ERR("[%s] Phy: %s Too many link loss events, not resetting PHY.\n", __func__, this->name);
                AVMNET_TRC("[%s] Phy: %s thrash_count: %u, thrash_time: %lu, jiffies: %lu.\n", __func__, this->name, phy->thrash_count, phy->thrash_time, jiffies);
            }

            /*
             * ask parent to resume sending us frames
             */
            RESUME_PARENT(this);
        }


        AVMNET_TRC("[%s] Phy: %s state: failed -> down\n", __func__, this->name);
        phy->state = LINK_DOWN;
        phy->jabber_time = jiffies;
    }

    /*
     * PHY is powered down
     */
    if(status.Details.powerup == 0){
        AVMNET_TRC("[%s] Phy: %s state %s -> pwrdown\n", __func__, this->name, STATE_NAME(phy));
        phy->state = LINK_PWRDOWN;
    }

    /*
     * Continuous jabber on link seems to indicate that the PHY is in a broken state
     */
    if(status.Details.link
       && (phyHwStatus & PHY_SPEC_STATUS_JABBER)
       && (this->initdata.phy.flags & AVMNET_CONFIG_FLAG_RST_ON_LINKFAIL))
    {
        if(time_is_before_eq_jiffies(phy->jabber_time + HZ)){
            AVMNET_ERR("[%s] Continuous jabber on PHY %s detected.\n", __func__, this->name);
            AVMNET_TRC("[%s] Phy: %s state: %s -> failed\n", __func__, this->name, STATE_NAME(phy));
            phy->state = LINK_FAILED;
        }
    }else {
        phy->jabber_time = jiffies;
    }

    /*
     * Link went up
     */
    if(status.Details.link && phy->state == LINK_DOWN){
        AVMNET_TRC("[%s] Phy: %s state: down -> up\n", __func__, this->name);
        phy->state = LINK_UP;
    }

    /*
     * Link loss detected. Either real-time or via latched-low link status flag in
     * MDIO_STATUS. This way we can detect a prior link fail even is the real-time
     * link status is up.
     * Change state to link_lost and record the time we detected this.
     */
    link_lost = 0;
    link_lost |= !((phyStatus & MDIO_STATUS_LINK) && status.Details.link);
    link_lost |= (IntStatus & ( PHY_INT_STATUS_LINK_UP
                              | PHY_INT_STATUS_LINK_DOWN
                              | PHY_INT_STATUS_SPEED));

    if(link_lost && phy->state == LINK_UP){
        AVMNET_TRC("[%s] Phy: %s state: up -> lost\n", __func__, this->name);
        phy->lost_time = jiffies;
        phy->state = LINK_LOST;
    }

    /*
     * Workaround 1:
     * On broken PHYs, change state to link_failed and report link-down upwards.
     * This will stop the upper layer modules from sending more frames. This way
     * we can re-initialise the PHY cleanly and undisturbed the next time we get called.
     */
    if(phy->state == LINK_LOST
       && (this->initdata.phy.flags & AVMNET_CONFIG_FLAG_RST_ON_LINKFAIL)
       && status.Details.link
       && status.Details.speed == AVMNET_LINKSTATUS_SPD_10
       && status.Details.speed != phy->last_status.Details.speed)
    {
        AVMNET_ERR("[%s] Phy %s: Link speed dropped to 10 MBit.\n", __func__, this->name);
        phy->state = LINK_FAILED;
    }


    /*
     * Workaround 1 + 2:
     */
    if(phy->state == LINK_LOST){
        if(status.Details.link == 0){
            /*
             * Workaround 2:
             * Internal PHYs on Wasp lose the link all the time. We only want to report this
             * upwards if the link is really down for at least linkf_time jiffies, otherwise
             * we report the previous link-state
             */
            if(time_is_after_jiffies(phy->lost_time + phy->linkf_time)){
                AVMNET_TRC("[%s] Phy: %s state lost, using saved status\n", __func__, this->name);
                status = phy->last_status;
            }
            /*
             * Workaround 1:
             * Link has been down for at least 5 seconds. It is a safe bet that we can reset
             * it without the user noticing any disruption.
             */
            if(time_is_before_eq_jiffies(phy->lost_time + phy->linkdown_delay)){
                AVMNET_TRC("[%s] Phy: %s down for %lu jiffies. state lost -> failed\n", __func__, this->name, phy->linkdown_delay);
                phy->state = LINK_FAILED;
            }
        }else{
            AVMNET_TRC("[%s] Phy: %s state lost -> up\n", __func__, this->name);
            phy->state = LINK_UP;
        }
    }

    if(phy->state == LINK_FAILED){
        status.Details.link = 0;
        phy->last_status = status;

        /*
         * if we are going to reset the phy, ask parent to stop sending us frames
         */
        if(this->initdata.phy.flags & AVMNET_CONFIG_FLAG_RST_ON_LINKFAIL){
            SUSPEND_PARENT(this);
        }

        if(phy->thrash_count == 0){
            AVMNET_TRC("[%s] Phy: %s thrash_time = now\n", __func__, this->name);
            phy->thrash_time = jiffies;
        }
        AVMNET_TRC("[%s] Phy: %s thrash_count++\n", __func__, this->name);

        phy->thrash_count++;
    }

    /*
     * Workaround 1 + 2:
     * Schedule an extra poll before the next heartbeat poll will run.
     */
    if(phy->state == LINK_LOST || phy->state == LINK_FAILED){
        queue_delayed_work(phy->link_work.workqueue, &(phy->link_work.work), HZ / 2);
    }

    /*
     * Workaround 2:
     * Remember last working link
     */
    if(phy->state == LINK_UP){
        AVMNET_TRC("[%s] Phy: %s saving status\n", __func__, this->name);
        phy->last_status = status;
    }

    AVMNET_TRC("[%s] Phy: %s status: link: %d, speed: %d, powerup: %d\n", __func__, this->name,
            status.Details.link, status.Details. speed, status.Details.powerup);

    AVMNET_TRC("[%s] Phy: %s new state: %s\n", __func__, this->name, STATE_NAME(phy));

    return status;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static avmnet_linkstatus_t get_status(avmnet_module_t *this)
{
    struct athphy_context *phy = (struct athphy_context *) this->priv;
    avmnet_linkstatus_t status;
    uint16_t phyStatus, phyHwStatus, phyControl;
    unsigned long flags;

    status.Status = 0;

    if(phy->powerdown){
        return status;
    }

    if (this->lock(this)) {
        return status;
    }

    phyControl = avmnet_mdio_read(this, MDIO_CONTROL);
    phyStatus = avmnet_mdio_read(this, MDIO_STATUS);
    phyHwStatus = avmnet_mdio_read(this, ATHR_PHY_SPEC_STATUS);

    AVMNET_TRC("{%s} %s Control 0x%x Status 0x%x HWStatus 0x%x\n", __func__, this->name, phyControl, phyStatus, phyHwStatus);

    // first check that  phy is not in power-down mode
    if(!(phyControl & MDIO_CONTROL_POWERDOWN)){
        status.Details.powerup = 1;

        if( (phyHwStatus & PHY_SPEC_STATUS_LINK_PASS) && 
           ((phy->config.autoneg == AUTONEG_DISABLE) || (phyHwStatus & PHY_SPEC_STATUS_RESOLVED)))
        {
            status.Details.link = 1;

            if(phyHwStatus & PHY_SPEC_STATUS_FULL_DUPLEX){
                status.Details.fullduplex = 1;
            }

            status.Details.speed = ((phyHwStatus & PHY_SPEC_STATUS_LINK_MASK)
                                     >> PHY_SPEC_STATUS_LINK_SHIFT);

            if(phyHwStatus & PHY_SPEC_STATUS_MDIX){
                status.Details.mdix = AVMNET_LINKSTATUS_MDI_MDIX;
            }else{
                status.Details.mdix = AVMNET_LINKSTATUS_MDI_MDI;
            }

            if(phyHwStatus & PHY_SPEC_STATUS_RXPAUSE_EN){
                status.Details.flowcontrol |= AVMNET_LINKSTATUS_FC_RX;
            }

            if( (phyHwStatus & PHY_SPEC_STATUS_TXPAUSE_EN ) && ( phy->config.pause_setup != avmnet_pause_rx) ){

                // TX pause: there is no way to advertise RX-Only, so we advertise
                // symetric and asymetric RX, and disable TX pause frames here
                status.Details.flowcontrol |= AVMNET_LINKSTATUS_FC_TX;
            }

        }

    }

    status = check_linkfail(this, status, phyStatus);

    if(this->initdata.phy.flags & AVMNET_CONFIG_FLAG_IRQ){
        spin_lock_irqsave(&(phy->irq_lock), flags);

        while(phy->irq_disabled > 0){
            AVMNET_DEBUG("[%s] device %s re-enabling IRQ\n", __func__, this->name);
            enable_irq(phy->irq);
            phy->irq_disabled--;
        }

        spin_unlock_irqrestore(&(phy->irq_lock), flags);
    }

    this->unlock(this);

    return status;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_ar803x_status_poll(avmnet_module_t *this)
{
    avmnet_module_t *parent = this->parent;
    avmnet_linkstatus_t status;
    int i, result;

    status = get_status(this);

    if(!(this->device_id->flags & AVMNET_DEVICE_FLAG_INITIALIZED)){
        AVMNET_ERR("[%s] %s not initialized yet\n", __func__, this->device_id->device_name);
        dump_stack();
        return 0;
    }

    for(i = 0; i < this->num_children; ++i){
        result = this->children[i]->poll(this->children[i]);
        if(result < 0){
            AVMNET_WARN("Module %s: poll() failed on child %s\n", this->name, this->children[i]->name);
        }
    }

#if 0
    AVMNET_DEBUG("[%s] Status %s changed actual %s speed %d\n", 
                __func__, this->name, status.Details.link ? "on":"off", status.Details.speed);
#endif

    parent->set_status(parent, this->device_id, status);

    return 0;
}

/*------------------------------------------------------------------------------------------*\
 * there is no autoneg on the fibermodul, so we have to switch the speed by hand
\*------------------------------------------------------------------------------------------*/
#if defined(CONFIG_VR9)
int avmnet_hw223_fiber_poll(avmnet_module_t *this) {

    avmnet_module_t *parent = this->parent;
    avmnet_linkstatus_t status;
    int i, result;

    status = get_status(this);

    if(!(this->device_id->flags & AVMNET_DEVICE_FLAG_INITIALIZED)){
        AVMNET_ERR("[%s] %s not initialized yet\n", __func__, this->device_id->device_name);
        dump_stack();
        return 0;
    }

    for(i = 0; i < this->num_children; ++i){
        result = this->children[i]->poll(this->children[i]);
        if(result < 0){
            AVMNET_WARN("Module %s: poll() failed on child %s\n", this->name, this->children[i]->name);
        }
    }

    parent->set_status(parent, this->device_id, status);
    return 0;
}
#endif

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static int setup_interrupt(avmnet_module_t *this, unsigned int on_off)
{
    struct athphy_context *phy = (struct athphy_context *) this->priv;

    AVMNET_DEBUG("[%s] %sable PHY-Interrupt on %s\n", __FUNCTION__, on_off ? "en":"dis", this->name);

    // warn on unsafe call
    if ( ! this->trylock(this)) {
        AVMNET_ERR("[%s] Called without MUTEX held!\n", __func__);
        dump_stack();
        this->unlock(this);
    }

    if(this->initdata.phy.flags & (AVMNET_CONFIG_FLAG_IRQ | AVMNET_CONFIG_FLAG_INTERNAL)){
        if(on_off){
            avmnet_mdio_write(this, ATHR_PHY_INTR_ENABLE, PHY_STATUS_INTS);
        }else{
            avmnet_mdio_write(this, ATHR_PHY_INTR_ENABLE, 0);
        }

        phy->irq_on = on_off;
    }

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_ar803x_setup_interrupt(avmnet_module_t *this, unsigned int on_off)
{
    int result;

    if (this->lock(this)) {
        return -EINTR;
    }

    result = setup_interrupt(this, on_off);

    this->unlock(this);
    return result;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_ar803x_setup(avmnet_module_t *this)
{
    struct athphy_context *phy = (struct athphy_context *) this->priv;
    int i, result;

    AVMNET_INFO("[%s] Called for PHY %s\n", __func__, this->name);

    // default configuration
    if(this->initdata.phy.flags & AVMNET_CONFIG_FLAG_PHY_GBIT){
        phy->gbit = 1;
    }

    if(this->initdata.phy.flags & AVMNET_CONFIG_FLAG_PHY_MODE_CONF){
        phy->config.supported = this->initdata.phy.mode_conf;

        if((phy->config.supported & ADVERTISED_1000baseT_Full) && phy->gbit == 0)
        {
            AVMNET_ERR("[%s] Config error for PHY %s: GBit modes configured on FE PHY.\n", __func__, this->name);
            phy->config.supported &= ~(ADVERTISED_1000baseT_Full | ADVERTISED_1000baseT_Half);
        }
    }else{
        phy->config.supported = ( ADVERTISED_TP
                                | ADVERTISED_Autoneg
                                | ADVERTISED_10baseT_Half
                                | ADVERTISED_10baseT_Full
                                | ADVERTISED_100baseT_Half
                                | ADVERTISED_100baseT_Full);

        if(phy->gbit){
            phy->config.supported |= ADVERTISED_1000baseT_Full;
        }
    }

    // setup default ethtool flocwcontrol, based on pyh initdata
    if ( this->initdata.phy.flags & AVMNET_CONFIG_FLAG_DEFAULT_EN_FC_RX ){
        if ( this->initdata.phy.flags & AVMNET_CONFIG_FLAG_DEFAULT_EN_FC_TX ){
            phy->config.pause_setup = avmnet_pause_rx_tx;
        }
        else {
            // most reasonable default config for ethernet switches:
            // receive pause frames, but not to send pause frames
            phy->config.pause_setup = avmnet_pause_rx;
        }
    }
    else if ( this->initdata.phy.flags & AVMNET_CONFIG_FLAG_DEFAULT_EN_FC_TX ){
        phy->config.pause_setup = avmnet_pause_tx;
    }
    else {
        phy->config.pause_setup = avmnet_pause_none;
    }

    // what kind of flow control is supported by the MAC, we are attached to?
    phy->config.supported |= calc_advertise_pause_param(
            (this->parent->initdata.mac.flags &  AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_SYM),
            (this->parent->initdata.mac.flags &  AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_ASYM),
            phy->config.pause_setup );

    AVMNET_INFO("[%s] default pause config is %d, asym_pause_adv=%d, sym_pause_adv=%d.\n", __func__,phy->config.pause_setup,
            (phy->config.supported  &  ADVERTISED_Asym_Pause),
            (phy->config.supported  &  ADVERTISED_Pause));

    if(phy->config.supported & ADVERTISED_Autoneg){
        phy->config.autoneg = AUTONEG_ENABLE;
    }else{
        phy->config.autoneg = AUTONEG_DISABLE;
    }

    if(phy->config.supported & ADVERTISED_1000baseT_Full){
        phy->config.speed = SPEED_1000;
    }else if(phy->config.supported & (ADVERTISED_100baseT_Full | ADVERTISED_100baseT_Half)){
        phy->config.speed = SPEED_100;
    }else{
        phy->config.speed = SPEED_10;
    }

    phy->config.advertise = phy->config.supported;
    if(phy->config.supported & ADVERTISED_FIBRE) {
        phy->config.port = PORT_FIBRE;
        phy->config.mdi = ETH_TP_MDI_INVALID;
    } else {
        phy->config.port = PORT_TP;
#       if(LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0))
        phy->config.mdi = ETH_TP_MDI;
#       else
        phy->config.mdi = ETH_TP_MDI_AUTO;
#       endif
    }

    if(this->initdata.phy.flags & AVMNET_CONFIG_FLAG_IRQ_ON){
        phy->irq_on = 1;
    }

    /*
     * how long to wait before the transition from link_failed to link_down
     */
    if(this->initdata.phy.flags & AVMNET_CONFIG_FLAG_RST_ON_LINKFAIL){
        phy->linkdown_delay = max(phy->linkf_time, (unsigned long int) (LINK_DOWN_DELAY * HZ));
    }else{
        phy->linkdown_delay = phy->linkf_time;
    }

    spin_lock_init(&(phy->irq_lock));

    if (this->lock(this)) {
        AVMNET_ERR("[%s] lock failed for module %s\n", __func__, this->name);
        return -EINTR;
    }

    result = setup_phy(this);

    this->unlock(this);

    if(result != 0){
        AVMNET_ERR("[%s] setup failed for module %s\n", __func__, this->name);
        return result;
    }

    if(this->initdata.phy.flags & AVMNET_CONFIG_FLAG_IRQ){
        result = request_irq(phy->irq,
                             ar803x_link_intr,
                             IRQF_SHARED | IRQF_TRIGGER_LOW,
                             "ar803x link",
                             this);
        if(result < 0){
            AVMNET_ERR("[%s] <request irq ar803x link %d failed>\n", __FUNCTION__, phy->irq);
            return result;
        }
    }

    for(i = 0; i < this->num_children; ++i){
        result = this->children[i]->setup(this->children[i]);
        if(result != 0){
            AVMNET_ERR("Module %s: init() failed on child %s\n", this->name, this->children[i]->name);
            return result;
        }
    }

    result = avmnet_cfg_register_module(this);
    if(result < 0){
        AVMNET_ERR("[%s] avmnet_cfg_register_module failed for module %s\n", __func__, this->name);
        return result;
    }


    result = avmnet_cfg_add_seq_procentry(this, "mdio_regs", &phy_reg_fops); // cannot be done before avmnet_cfg_register_module
    if(result != 0){
        AVMNET_ERR("[%s] avmnet_cfg_add_seq_procentry failed for module %s\n", __func__, this->name);
        return result;
    }

    this->powerdown(this);
    return 0;
}

/*------------------------------------------------------------------------------------------*\
 * special Setup für PHYs des AR8337 & AR8334 - wird bei 5490 verwendet
\*------------------------------------------------------------------------------------------*/
#if defined(CONFIG_ATHRS17_PHY)
int avmnet_athrs17_setup_phy(avmnet_module_t *this) {

    if ( ! this->trylock(this)) {
        AVMNET_ERR("[%s] Called without MUTEX held!\n", __func__);
        dump_stack();
        this->unlock(this);
    }

    /* fine-tune PHY 0 and PHY 1*/
    if ((this->initdata.phy.mdio_addr == 0) || (this->initdata.phy.mdio_addr == 1)) {
        avmnet_mdio_write(this,  MDIO_MMD_CTRL, 0x3);
        avmnet_mdio_write(this,  MDIO_MMD_ADDR, 0x8007);
        avmnet_mdio_write(this,  MDIO_MMD_CTRL, 0x4003);
        avmnet_mdio_write(this,  MDIO_MMD_ADDR, 0x8315);
    }

    /*--- Errata: 80-Y0619-15 Rev. B ---*/
    /*--- set short & long cable control ---*/
    avmnet_mdio_write(this,  MDIO_MMD_CTRL, 0x3);
    avmnet_mdio_write(this,  MDIO_MMD_ADDR, 0x8008);
    avmnet_mdio_write(this,  MDIO_MMD_CTRL, 0x4003);
    avmnet_mdio_write(this,  MDIO_MMD_ADDR, 0x3045);

    /*--- AZ debug set AZ_WAKE state ---*/
    avmnet_mdio_write(this,  MDIO_MMD_CTRL, 0x3);
    avmnet_mdio_write(this,  MDIO_MMD_ADDR, 0x800d);
    avmnet_mdio_write(this,  MDIO_MMD_CTRL, 0x4003);
    avmnet_mdio_write(this,  MDIO_MMD_ADDR, 0x4b3f);

    avmnet_mdio_write(this,  ATHR_PHY_DEBUG_PORT_ADDRESS, 0x3d);
    avmnet_mdio_write(this,  ATHR_PHY_DEBUG_PORT_DATA, 0x6860);

    /*--- do not advertise EEE ---*/
    avmnet_mdio_write(this,  MDIO_MMD_CTRL, 0x7);
    avmnet_mdio_write(this,  MDIO_MMD_ADDR, 0x3c);
    avmnet_mdio_write(this,  MDIO_MMD_CTRL, 0x4007);
    avmnet_mdio_write(this,  MDIO_MMD_ADDR, 0x0);

    return 0;
}
#endif

/*------------------------------------------------------------------------------------------*\
 * special Setup für internen Switch Dragonfly
\*------------------------------------------------------------------------------------------*/
int avmnet_4010_setup_phy(avmnet_module_t *this) {
    u32 value;

    AVMNET_TRC("{%s}\n", __func__);

    if ( ! this->trylock(this)) {
        AVMNET_ERR("[%s] Called without MUTEX held!\n", __func__);
        dump_stack();
        this->unlock(this);
    }

    /* Enable MDIO Access if PHY is Powered-down */
    avmnet_mdio_write(this,  ATHR_PHY_DEBUG_PORT_ADDRESS, 0x3);
    value = avmnet_mdio_read(this,  ATHR_PHY_DEBUG_PORT_DATA);
    avmnet_mdio_write(this,  ATHR_PHY_DEBUG_PORT_ADDRESS, 0x3);
    avmnet_mdio_write(this,  ATHR_PHY_DEBUG_PORT_DATA, (value & 0xfffffeff));

    /* extend the cable length */
    avmnet_mdio_write(this, ATHR_PHY_DEBUG_PORT_ADDRESS, 0x14);
    avmnet_mdio_write(this, ATHR_PHY_DEBUG_PORT_DATA, 0xf52);

    /* Force Class A setting phys */
    avmnet_mdio_write(this, ATHR_PHY_DEBUG_PORT_ADDRESS, 4);
    avmnet_mdio_write(this, ATHR_PHY_DEBUG_PORT_DATA, 0xebbb);
    avmnet_mdio_write(this, ATHR_PHY_DEBUG_PORT_ADDRESS, 5);
    avmnet_mdio_write(this, ATHR_PHY_DEBUG_PORT_DATA, 0x2c47);

    /* fine-tune PHYs */
    avmnet_mdio_write(this, ATHR_PHY_DEBUG_PORT_ADDRESS, 0x3c);
    avmnet_mdio_write(this, ATHR_PHY_DEBUG_PORT_DATA, 0x1c1);
    avmnet_mdio_write(this, ATHR_PHY_DEBUG_PORT_ADDRESS, 0x37);
    avmnet_mdio_write(this, ATHR_PHY_DEBUG_PORT_DATA, 0xd600);

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static int setup_phy(avmnet_module_t *this)
{
    struct athphy_context *phy = (struct athphy_context *) this->priv;
    u32 value;
    unsigned int i;
    int status = 0;

    AVMNET_INFO("[%s] called for PHY %s\n", __func__, this->name);

    // warn on unsafe call
    if ( ! this->trylock(this)) {
        AVMNET_ERR("[%s] Called without MUTEX held!\n", __func__);
        dump_stack();
        this->unlock(this);
    }

    if(this->initdata.phy.flags & AVMNET_CONFIG_FLAG_RESET){

        /*------------------------------------------------------------------------------------------*\
            externe PHYs
        \*------------------------------------------------------------------------------------------*/
        reset_phy(this, rst_pulse);

        i = 0;
        while((avmnet_mdio_read(this, ATHR_PHY_ID1) == 0xFFFF) && (i < 10)){
            ++i;
            AVMNET_ERR("[%s] PHY %s hanging, trying to reset it.\n", __func__, this->name);
            reset_phy(this, rst_pulse);
            mdelay(1000);
        }

        if((i >= 10) || ( ! avmnet_mdio_read(this, ATHR_PHY_ID1))) {
            AVMNET_ERR("[%s] Giving up on hanging PHY %s\n", __func__, this->name);
            return -EFAULT;
        }

        avmnet_mdio_write(this, ATHR_PHY_CONTROL, PHY_CTRL_AUTONEGOTIATION_ENABLE | PHY_CTRL_SOFTWARE_RESET);
        mdelay(100);

    }else{
        /*--------------------------------------------------------------------*\
            der PHY ist "intern" im Switch verbaut, kein externer Reset-PIN
        \*--------------------------------------------------------------------*/

        /*--- reset PHY ---*/
        avmnet_mdio_write(this, ATHR_PHY_CONTROL, PHY_CTRL_AUTONEGOTIATION_ENABLE | PHY_CTRL_SOFTWARE_RESET);
        mdelay(100);

        i = 0;
        while (i < 10) {
            unsigned short phyid = avmnet_mdio_read(this, MDIO_PHY_ID1);
            if ((phyid != 0xFFFF) && (phyid != 0))
                break;
            i++;
            mdelay(1000);
        }

        if (i >= 10) {
            AVMNET_ERR("[%s] Giving up on hanging PHY %s\n", __func__, this->name);
            return -EFAULT;
        }

        if (this->setup_special_hw) {
            this->setup_special_hw(this);
        } else {
            avmnet_mdio_write(this,  ATHR_PHY_DEBUG_PORT_ADDRESS, 5);
            value = avmnet_mdio_read(this,  ATHR_PHY_DEBUG_PORT_DATA) & ((~2) & 0xffff);
            avmnet_mdio_write(this,  ATHR_PHY_DEBUG_PORT_DATA, value);

            /* Enable MDIO Access if PHY is Powered-down */
            avmnet_mdio_write(this,  ATHR_PHY_DEBUG_PORT_ADDRESS, 0x3);
            value = avmnet_mdio_read(this,  ATHR_PHY_DEBUG_PORT_DATA);
            avmnet_mdio_write(this,  ATHR_PHY_DEBUG_PORT_ADDRESS, 0x3);
            avmnet_mdio_write(this,  ATHR_PHY_DEBUG_PORT_DATA, (value & 0xfffffeff));

            /* fix IOT */
            avmnet_mdio_write(this,  ATHR_PHY_DEBUG_PORT_ADDRESS, 0x14);
            avmnet_mdio_write(this,  ATHR_PHY_DEBUG_PORT_DATA, 0x1352);

            /* Force Class A setting phys */
            avmnet_mdio_write(this,  ATHR_PHY_DEBUG_PORT_ADDRESS, 4);
            avmnet_mdio_write(this,  ATHR_PHY_DEBUG_PORT_DATA, 0xebbb);
            avmnet_mdio_write(this,  ATHR_PHY_DEBUG_PORT_ADDRESS, 5);
            avmnet_mdio_write(this,  ATHR_PHY_DEBUG_PORT_DATA, 0x2c47);
        }

        AVMNET_TRC("PHY_FUNC_CONTROL (%d): 0x%x\n", this->initdata.phy.mdio_addr, avmnet_mdio_read(this, ATHR_PHY_FUNC_CONTROL)); 
        AVMNET_TRC("PHY_ID           (%d): 0x%x\n", this->initdata.phy.mdio_addr, avmnet_mdio_read(this, MDIO_PHY_ID1)); 
        AVMNET_TRC("PHY_CTRL         (%d): 0x%x\n", this->initdata.phy.mdio_addr, avmnet_mdio_read(this, ATHR_PHY_SPEC_STATUS)); 
        AVMNET_TRC("PHY_STATUS       (%d): 0x%x\n", this->initdata.phy.mdio_addr, avmnet_mdio_read(this, MDIO_STATUS));
    }

    if(this->initdata.phy.flags & AVMNET_CONFIG_FLAG_RX_DELAY){
        avmnet_mdio_write(this, ATHR_PHY_DEBUG_PORT_ADDRESS, 0);
        avmnet_mdio_write(this, ATHR_PHY_DEBUG_PORT_DATA, this->initdata.phy.rx_delay);
    }
    if(this->initdata.phy.flags & AVMNET_CONFIG_FLAG_TX_DELAY){
        avmnet_mdio_write(this, ATHR_PHY_DEBUG_PORT_ADDRESS, 5);
        avmnet_mdio_write(this, ATHR_PHY_DEBUG_PORT_DATA, this->initdata.phy.tx_delay);
    }

    if(this->initdata.phy.flags & (AVMNET_CONFIG_FLAG_IRQ | AVMNET_CONFIG_FLAG_INTERNAL)){
        status |= setup_interrupt(this, phy->irq_on);

        value = avmnet_mdio_read(this, ATHR_PHY_INTR_STATUS); /*--- den IRQ-Status löschen ---*/
        avmnet_mdio_write(this, ATHR_PHY_INTR_STATUS, value);
    }

    phy->state = LINK_DOWN;

    status |= set_config(this);

    if(phy->powerdown){
        this->powerdown(this);
    }

    return status;

}
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_ar803x_init(avmnet_module_t *this)
{
    struct athphy_context *phy;
    int i, result;
#if defined(INFINEON_CPU)
    static enum _avm_hw_param pin_param = avm_hw_param_no_param;
    int irq_pin;
#endif

    AVMNET_TRC("{%s} Init on module %s called.\n", __func__, this->name);

    phy = kzalloc(sizeof(struct athphy_context), GFP_KERNEL);
    if(phy == NULL){
        AVMNET_ERR("{%s} init of avmnet-module %s failed.\n", __func__, this->name);
        return -ENOMEM;
    }

    this->priv = phy;
    phy->this_module = this;

    if(this->initdata.phy.flags & AVMNET_CONFIG_FLAG_MDIOADDR){
        phy->phyAddr = this->initdata.phy.mdio_addr;
    }else{
        AVMNET_ERR("[%s] No MDIO address for PHY %s.\n", __func__, this->name);
        BUG();
    }

    if(this->initdata.phy.flags & AVMNET_CONFIG_FLAG_RESET){
#if defined(CONFIG_MACH_ATHEROS) || defined(CONFIG_ATH79)
        if(this->initdata.phy.reset < 100){
            /*--- Hängt nicht am Shift-Register => GPIO Resourcen anmelden ---*/
            ar803x_gpio.start = this->initdata.phy.reset;
            ar803x_gpio.end = this->initdata.phy.reset;
            if(request_resource(&gpio_resource, &ar803x_gpio)){
                AVMNET_ERR(KERN_ERR "[%s] ERROR request resource gpio %d\n", __FUNCTION__, this->initdata.phy.reset);
                kfree(phy);
                return -EIO;
            }
        }

#elif defined(INFINEON_CPU)

#if defined(GPIO_REGISTER_BROKEN)
        result = ifx_gpio_pin_reserve(this->initdata.phy.reset, IFX_GPIO_MODULE_EXTPHY_RESET);
        if(result != IFX_SUCCESS)
#else // defined(GPIO_REGISTER_BROKEN)
        if(ifx_gpio_register(IFX_GPIO_MODULE_EXTPHY_RESET) != IFX_SUCCESS)
#endif // defined(GPIO_REGISTER_BROKEN)
        {
            AVMNET_ERR("[%s] ERROR request resource gpio %d\n", __FUNCTION__, this->initdata.phy.reset);
            kfree(phy);
            return -EIO;
        }

#if defined(GPIO_REGISTER_BROKEN)
        ifx_gpio_dir_out_set(this->initdata.phy.reset, IFX_GPIO_MODULE_EXTPHY_RESET);
        ifx_gpio_altsel0_clear(this->initdata.phy.reset, IFX_GPIO_MODULE_EXTPHY_RESET);
        ifx_gpio_altsel1_clear(this->initdata.phy.reset, IFX_GPIO_MODULE_EXTPHY_RESET);
        ifx_gpio_open_drain_set(this->initdata.phy.reset, IFX_GPIO_MODULE_EXTPHY_RESET);
        ifx_gpio_pudsel_set(this->initdata.phy.reset, IFX_GPIO_MODULE_EXTPHY_RESET);
        ifx_gpio_puden_set(this->initdata.phy.reset, IFX_GPIO_MODULE_EXTPHY_RESET);
#endif // defined(GPIO_REGISTER_BROKEN)

        if(this->initdata.phy.flags & AVMNET_CONFIG_FLAG_IRQ){
            result = avm_get_hw_config(AVM_HW_CONFIG_VERSION,
                                       "gpio_avm_extphy_int",
                                       &irq_pin,
                                       &pin_param);

            if(result < 0){
                AVMNET_ERR("[%s]: Could not read IRQ pin data from avm_hw_config!\n", __func__);
                return -EIO;
            }

#if defined(GPIO_REGISTER_BROKEN)
            result = ifx_gpio_pin_reserve(irq_pin, IFX_GPIO_MODULE_EXTPHY_INT);
            if(result != IFX_SUCCESS){
                AVMNET_DEBUG("[%s]: GPIO EXTPHY_INT already registered, assuming it is configured by other PHY\n", __func__);
            }else{
                ifx_gpio_dir_in_set(irq_pin, IFX_GPIO_MODULE_EXTPHY_INT);
                ifx_gpio_altsel0_clear(irq_pin, IFX_GPIO_MODULE_EXTPHY_INT);
                ifx_gpio_altsel1_set(irq_pin, IFX_GPIO_MODULE_EXTPHY_INT);
            }
#else // defined(GPIO_REGISTER_BROKEN)
            if(ifx_gpio_register(IFX_GPIO_MODULE_EXTPHY_INT) != IFX_SUCCESS){
                AVMNET_ERR("[%s] ERROR registering IFX_GPIO_MODULE_EXTPHY_INT\n", __FUNCTION__);
                kfree(phy);
                return -EIO;
            }
#endif // defined(GPIO_REGISTER_BROKEN)
        }

#endif // defined(INFINEON_CPU)
        if (this->lock(this)) {
            AVMNET_ERR("[%s] ERROR lock\n", __func__);
            kfree(phy);
            return -EINTR;
        }

        reset_phy(this, rst_on);
        this->unlock(this);
    }

    if(this->initdata.phy.flags & AVMNET_CONFIG_FLAG_LINKFAIL_TIME){
        phy->linkf_time = this->initdata.phy.lnkf_time;
    }else{
        phy->linkf_time = HZ;
    }

    phy->thrash_time = jiffies;

    phy->link_work.workqueue = avmnet_get_workqueue();
    if(phy->link_work.workqueue == NULL){
        AVMNET_ERR("[%s] No global workqueue available!\n>", __FUNCTION__);
        kfree(phy);
        return -EFAULT;
    }
    INIT_DELAYED_WORK(&(phy->link_work.work), link_work_handler);

    if(this->initdata.phy.flags & AVMNET_CONFIG_FLAG_IRQ){
        phy->irq = this->initdata.phy.irq;
    }

    for(i = 0; i < this->num_children; ++i){
        result = this->children[i]->init(this->children[i]);
        if(result != 0){
            // handle error
            AVMNET_ERR("Module %s: init() failed on child %s\n", this->name, this->children[i]->name);
            return result;
        }
    }

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_ar803x_exit(avmnet_module_t *this)
{

    int i, result = 0;
    struct athphy_context *phy = (struct athphy_context *) this->priv;

    AVMNET_TRC("{%s} Exit on module %s called.\n", __func__, this->name);

    for(i = 0; i < this->num_children; ++i){
        result = this->children[i]->exit(this->children[i]);
        if(result != 0){
            // handle error
        }
    }

    if(this->initdata.phy.flags & AVMNET_CONFIG_FLAG_IRQ){
        free_irq(phy->irq, ar803x_link_intr);
        cancel_delayed_work_sync(&(phy->link_work.work));
    }

    kfree(this->priv);

    return result;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_ar803x_powerdown(avmnet_module_t *this)
{
    struct athphy_context *phy = (struct athphy_context *) this->priv;
    int i, result;

    AVMNET_TRC("[%s] Called for module %s\n", __func__, this->name);

    for(i = 0; i < this->num_children; ++i){
        result = this->children[i]->powerdown(this->children[i]);
        if(result < 0){
            AVMNET_WARN("Module %s: powerdown() failed on child %s\n", this->name, this->children[i]->name);
        }
    }

    if (this->lock(this)) {
        return -EINTR;
    }

    avmnet_mdio_write(this, MDIO_CONTROL, (MDIO_CONTROL_POWERDOWN | MDIO_CONTROL_ISOLATE));
    phy->powerdown = 1;

    this->unlock(this);
    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_ar803x_powerup(avmnet_module_t *this)
{
    struct athphy_context *phy = (struct athphy_context *) this->priv;
    int i, result;

    AVMNET_TRC("[%s] Called for module %s\n", __func__, this->name);

    if (this->lock(this)) {
        return -EINTR;
    }

    avmnet_mdio_write(this, MDIO_CONTROL, MDIO_CONTROL_RESET);
    msleep(500);

    phy->powerdown = 0;

    /*
     * On broken PHYs, do a full reset on power-up. Otherwise just restore its
     * configuration
     */
    if(this->initdata.phy.flags & AVMNET_CONFIG_FLAG_RST_ON_LINKFAIL){
        SUSPEND_PARENT(this);

        result = setup_phy(this);

        RESUME_PARENT(this);
    } else {
        result = set_config(this);
        phy->state = LINK_DOWN;
    }

    this->unlock(this);

    for(i = 0; i < this->num_children; ++i){
        result = this->children[i]->powerup(this->children[i]);
        if(result < 0){
            AVMNET_WARN("Module %s: powerup() failed on child %s\n", this->name, this->children[i]->name);
        }
    }

    return result;
}
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_ar803x_ethtool_get_settings(avmnet_module_t *this, struct ethtool_cmd *cmd)
{
    unsigned int phyCtrl, phyAdv, phyGCtrl;
    avmnet_linkstatus_t status;
    struct athphy_context *phy = (struct athphy_context *) this->priv;

    AVMNET_TRC("[%s] Called for module %s\n", __func__, this->name);

    status = get_status(this);

    if (this->lock(this)) {
        return -EINTR;
    }

    phyCtrl = avmnet_mdio_read(this, MDIO_CONTROL);
    if(phy->config.port == PORT_FIBRE) {
        phyAdv = avmnet_mdio_read(this, MDIO_FIBER_AUTONEG_ADV);
        phyGCtrl = 0;
    } else {
        phyAdv = avmnet_mdio_read(this, MDIO_AUTONEG_ADV);
        phyGCtrl = avmnet_mdio_read(this, MDIO_MASTER_SLAVE_CTRL);
    }

    this->unlock(this);

    cmd->supported = phy->config.supported;

    /*
     * Due to unreliable register reads in power-down mode, we just return
     * the configuration that will be set on power-up for advertising and autoneg
     */
    if(phy->powerdown) {
        cmd->advertising = phy->config.advertise;
        cmd->autoneg = phy->config.autoneg;
    } else {
        cmd->advertising = 0;

        if(phy->config.port == PORT_FIBRE) {
            if(phyCtrl & MDIO_CONTROL_AUTONEG_ENABLE) {
                cmd->autoneg = AUTONEG_ENABLE;

                if(phyAdv & MDIO_FIBER_AUTONEG_ADV_1000B_X_HD) {
                    cmd->advertising |= ADVERTISED_1000baseT_Half;
                    cmd->advertising |= ADVERTISED_100baseT_Half;
                }

                if(phyAdv & MDIO_FIBER_AUTONEG_ADV_1000B_X_FD) {
                    cmd->advertising |= ADVERTISED_1000baseT_Full;
                    cmd->advertising |= ADVERTISED_100baseT_Full;
                }

                if(phyAdv & MDIO_FIBER_AUTONEG_ADV_PAUSE_ASYM) {
                    cmd->advertising |= ADVERTISED_Asym_Pause;
                }

                if(phyAdv & MDIO_FIBER_AUTONEG_ADV_PAUSE_SYM) {
                    cmd->advertising |= ADVERTISED_Pause;
                }

                cmd->advertising |= ADVERTISED_FIBRE | ADVERTISED_Autoneg;
            } else {
                cmd->autoneg = AUTONEG_DISABLE;
            }
            cmd->port = PORT_FIBRE;
        } else {
            if(phyCtrl & MDIO_CONTROL_AUTONEG_ENABLE) {
                cmd->autoneg = AUTONEG_ENABLE;

                if(phyAdv & MDIO_AUTONEG_ADV_10BT_HDX) {
                    cmd->advertising |= ADVERTISED_10baseT_Half;
                }

                if(phyAdv & MDIO_AUTONEG_ADV_10BT_FDX) {
                    cmd->advertising |= ADVERTISED_10baseT_Full;
                }

                if(phyAdv & MDIO_AUTONEG_ADV_100BT_HDX) {
                    cmd->advertising |= ADVERTISED_100baseT_Half;
                }

                if(phyAdv & MDIO_AUTONEG_ADV_100BT_FDX) {
                    cmd->advertising |= ADVERTISED_100baseT_Full;
                }

                if(phyAdv & MDIO_AUTONEG_ADV_10BT_HDX) {
                    cmd->advertising |= ADVERTISED_10baseT_Half;
                }

                if(phyAdv & MDIO_AUTONEG_ADV_PAUSE_ASYM) {
                    cmd->advertising |= ADVERTISED_Asym_Pause;
                }

                if(phyAdv & MDIO_AUTONEG_ADV_PAUSE_SYM) {
                    cmd->advertising |= ADVERTISED_Pause;
                }
                cmd->advertising |= ADVERTISED_TP | ADVERTISED_Autoneg;
            }else{
                cmd->autoneg = AUTONEG_DISABLE;
            }

            if(phy->gbit) {
                if(cmd->autoneg == AUTONEG_ENABLE) {
                    if(phyGCtrl & MDIO_MASTER_SLAVE_CTRL_ADV_HD) {
                        cmd->advertising |= ADVERTISED_1000baseT_Half;
                    }

                    if(phyGCtrl & MDIO_MASTER_SLAVE_CTRL_ADV_FD) {
                        cmd->advertising |= ADVERTISED_1000baseT_Full;
                    }
                }
            }
            cmd->port = PORT_TP;
        }
    }

    cmd->phy_address = this->initdata.phy.mdio_addr;

    if(status.Details.link) {
        switch(status.Details.speed) {
            case 0:
                cmd->speed = SPEED_10;
                break;
            case 1:
                cmd->speed = SPEED_100;
                break;
            case 2:
                cmd->speed = SPEED_1000;
                break;
            default:
                cmd->speed = -1;
                break;
        }

        if(status.Details.fullduplex) {
            cmd->duplex = DUPLEX_FULL;
        } else {
            cmd->duplex = DUPLEX_HALF;
        }

        if(status.Details.mdix == AVMNET_LINKSTATUS_MDI_MDIX) {
            cmd->eth_tp_mdix = ETH_TP_MDI_X;
        } else {
            cmd->eth_tp_mdix = ETH_TP_MDI;
        }
    } else {
        cmd->speed = -1;
        cmd->duplex = -1;
        cmd->eth_tp_mdix = ETH_TP_MDI_INVALID;
    }

    return 0;
}


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_ar803x_ethtool_set_settings(avmnet_module_t *this, struct ethtool_cmd *cmd)
{
    struct athphy_context *phy = (struct athphy_context *) this->priv;
    int result;
    __u32 dummy_adv;

    AVMNET_DEBUG("[%s] Called for module %s\n", __func__, this->name);

    if (this->lock(this)) {
        return -EINTR;
    }

    if(cmd->autoneg == AUTONEG_DISABLE) {
        switch(cmd->speed){
            case SPEED_10:
                dummy_adv = (cmd->duplex == DUPLEX_FULL) ? ADVERTISED_10baseT_Full 
                                                         : ADVERTISED_10baseT_Half;
                break;
            case SPEED_100:
                dummy_adv = (cmd->duplex == DUPLEX_FULL) ? ADVERTISED_100baseT_Full 
                                                         : ADVERTISED_100baseT_Half;
                break;
            case SPEED_1000:
                dummy_adv = (cmd->duplex == DUPLEX_FULL) ? ADVERTISED_1000baseT_Full 
                                                         : ADVERTISED_1000baseT_Half;
                break;
            default:
                AVMNET_ERR("[%s] PHY %s: unsupported speed given by ethtool_cmd.\n", __func__, this->name);
                this->unlock(this);
                return -EINVAL;
        }
    }else{
        dummy_adv = cmd->advertising;
    }

    if((phy->config.supported & dummy_adv) != dummy_adv){
        AVMNET_ERR("[%s] PHY %s: requested mode(s) not supported. (supported %#x, requested %#x)\n", 
                   __func__, this->name, phy->config.supported, dummy_adv);
        this->unlock(this);
        return -EINVAL;
    }

#if 0
    // refuse GBit settings if not in GBit mode
    if(!(phy->config.supported & AVMNET_CONFIG_FLAG_PHY_GBIT)){
        if(cmd->autoneg == AUTONEG_ENABLE){
            if(cmd->advertising & (ADVERTISED_1000baseT_Half | ADVERTISED_1000baseT_Full)){
                AVMNET_ERR("[%s] Asked to set GBit-Adv for FE Phy!\n", __func__);
                dump_stack();
                up(&(phy->mutex));
                return -EINVAL;
            }
        }else if(cmd->speed == SPEED_1000){
            AVMNET_ERR("[%s] Asked to set GBit speed on FE Phy!\n", __func__);
            dump_stack();
            up(&(phy->mutex));
            return -EINVAL;
        }
    }
#endif

    if(cmd->autoneg == AUTONEG_ENABLE){
        // pause parameter advertising is controlled via ethtool_set_pauseparam()
        cmd->advertising &= ~(ADVERTISED_Pause | ADVERTISED_Asym_Pause);
        cmd->advertising |= (phy->config.advertise & (ADVERTISED_Pause | ADVERTISED_Asym_Pause));
        phy->config.advertise = cmd->advertising;
        phy->config.autoneg = AUTONEG_ENABLE;
    }else{
        phy->config.speed = cmd->speed;
        phy->config.duplex = cmd->duplex;
        phy->config.autoneg = AUTONEG_DISABLE;
    }

    result = set_config(this);

    this->unlock(this);
    return result;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void avmnet_ar803x_ethtool_get_pauseparam(avmnet_module_t *this,
        struct ethtool_pauseparam *param)
{
    struct athphy_context *phy = (struct athphy_context *) this->priv;
    avmnet_linkstatus_t status;

    status = get_status(this);

    if(phy->config.autoneg){
        param->autoneg = AUTONEG_ENABLE;
    }else{
        param->autoneg = AUTONEG_DISABLE;
    }

    if(status.Details.flowcontrol & AVMNET_LINKSTATUS_FC_RX){
        param->rx_pause = 1;
    }

    if(status.Details.flowcontrol & AVMNET_LINKSTATUS_FC_TX){
        param->tx_pause = 1;
    }
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_ar803x_ethtool_set_pauseparam(avmnet_module_t *this,
        struct ethtool_pauseparam *param)
{
    struct athphy_context *phy = (struct athphy_context *) this->priv;
    u32 new_adv;
    enum avmnet_pause_setup orig_pause_setup;

    if(!param->autoneg && (param->rx_pause || param->tx_pause)){
        AVMNET_ERR("[%s] ethtool request to force set flow control parameters, but can only be auto-negotiated\n", __func__);
        return -EINVAL;
    }

    if (this->lock(this)) {
        return -EINTR;
    }

    orig_pause_setup = phy->config.pause_setup;
    new_adv = phy->config.advertise;
    new_adv &= ~(ADVERTISED_Asym_Pause | ADVERTISED_Pause);
    if(param->autoneg){

        phy->config.pause_setup = avmnet_pause_none;

        // does our MAC support ASYM_PAUSE?
        if (this->parent->initdata.mac.flags & AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_ASYM){

            if(!param->rx_pause && param->tx_pause){
                phy->config.pause_setup = avmnet_pause_tx;
            }

            if(param->rx_pause && !param->tx_pause){
                phy->config.pause_setup = avmnet_pause_rx;
            }
        }

        // does our MAC support any PAUSE?
        if (this->parent->initdata.mac.flags & (AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_SYM | AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_ASYM)){

            if(param->rx_pause && param->tx_pause){
                phy->config.pause_setup = avmnet_pause_rx_tx;
            }
        }

        // calculate our current advertisement
        new_adv |= calc_advertise_pause_param(
            (this->parent->initdata.mac.flags &  AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_SYM),
            (this->parent->initdata.mac.flags &  AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_ASYM),
            phy->config.pause_setup );

        if((phy->config.advertise == new_adv) && (orig_pause_setup == phy->config.pause_setup)){
            this->unlock(this);
            return -EINVAL;
        }

        phy->config.advertise = new_adv;
        set_config(this);
    }

    this->unlock(this);
    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static int set_config(avmnet_module_t *this) {
    struct athphy_context *phy = (struct athphy_context *) this->priv;
    unsigned int phyCtrl, phyAdv, phyGCtrl = 0;
    /*--- unsigned int dummy; ---*/

    if(! this->trylock(this)) {
        AVMNET_ERR("[%s] Called without MUTEX held!\n", __func__);
        dump_stack();
        this->unlock(this);
    }

#if 0
    // disable SmartSpeed
    MDIO_WRITE(ATHR_PHY_SMARTSPEED, 0);
    MDIO_WRITE(MDIO_CONTROL, MDIO_CONTROL_RESET);

    // disable SmartEEE
    MDIO_WRITE(MDIO_MMD_CTRL, 0x3);
    MDIO_WRITE(MDIO_MMD_ADDR, 0x805D);
    MDIO_WRITE(MDIO_MMD_CTRL, 0x4003);
    dummy = MDIO_READ(MDIO_MMD_ADDR);
    MDIO_WRITE(MDIO_MMD_ADDR, 0x0);
    AVMNET_ERR("[%s] SmartEEE CTRL3: %04x\n", __func__, dummy);

    // disable EEE advertisement
    MDIO_WRITE(MDIO_MMD_CTRL, 0x7);
    MDIO_WRITE(MDIO_MMD_ADDR, 0x3C);
    MDIO_WRITE(MDIO_MMD_CTRL, 0x4007);
    dummy = MDIO_READ(MDIO_MMD_ADDR);
    MDIO_WRITE(MDIO_MMD_ADDR, 0x0);
    AVMNET_ERR("[%s] EEE Adv: %04x\n", __func__, dummy);

    // disable hibernation
    MDIO_WRITE(ATHR_PHY_DEBUG_PORT_ADDRESS, 0xB);
    MDIO_WRITE(ATHR_PHY_DEBUG_PORT_DATA, 0);
#endif

    phyCtrl = 0;

    if(phy->config.port == PORT_FIBRE) {
        phyAdv = avmnet_mdio_read(this, MDIO_FIBER_AUTONEG_ADV);
        phyAdv &= ~(  MDIO_FIBER_AUTONEG_ADV_1000B_X_HD
                    | MDIO_FIBER_AUTONEG_ADV_1000B_X_FD
                    | MDIO_FIBER_AUTONEG_ADV_PAUSE_ASYM
                    | MDIO_FIBER_AUTONEG_ADV_PAUSE_SYM);

        if(phy->config.advertise & ADVERTISED_1000baseT_Full) {
            phyAdv |= MDIO_FIBER_AUTONEG_ADV_1000B_X_FD;
        }
        if(phy->config.advertise & ADVERTISED_1000baseT_Half) {
            phyAdv |= MDIO_FIBER_AUTONEG_ADV_1000B_X_HD;
        }
        if(phy->config.advertise & ADVERTISED_Asym_Pause) {
            phyAdv |= MDIO_FIBER_AUTONEG_ADV_PAUSE_ASYM;
        }
        if(phy->config.advertise & ADVERTISED_Pause) {
            phyAdv |= MDIO_FIBER_AUTONEG_ADV_PAUSE_SYM;
        }
    } else {
        phyAdv = avmnet_mdio_read(this, MDIO_AUTONEG_ADV);
        phyAdv &= ~(MDIO_AUTONEG_ADV_SPEED_MASK | MDIO_AUTONEG_ADV_PAUSE_SYM | MDIO_AUTONEG_ADV_PAUSE_ASYM);

        if(phy->config.advertise & ADVERTISED_10baseT_Half){
            phyAdv |= MDIO_AUTONEG_ADV_10BT_HDX;
        }
        if(phy->config.advertise & ADVERTISED_10baseT_Full){
            phyAdv |= MDIO_AUTONEG_ADV_10BT_FDX;
        }
        if(phy->config.advertise & ADVERTISED_100baseT_Half){
            phyAdv |= MDIO_AUTONEG_ADV_100BT_HDX;
        }
        if(phy->config.advertise & ADVERTISED_100baseT_Full){
            phyAdv |= MDIO_AUTONEG_ADV_100BT_FDX;
        }
        if(phy->config.advertise & ADVERTISED_Asym_Pause){
            phyAdv |= MDIO_AUTONEG_ADV_PAUSE_ASYM;
        }
        if(phy->config.advertise & ADVERTISED_Pause){
            phyAdv |= MDIO_AUTONEG_ADV_PAUSE_SYM;
        }
    }

    if(phy->config.autoneg == AUTONEG_ENABLE) {
        phyCtrl |= MDIO_CONTROL_AUTONEG_ENABLE;
    }

    if(phy->config.port != PORT_FIBRE) {
        phyGCtrl = avmnet_mdio_read(this, MDIO_MASTER_SLAVE_CTRL);
        phyGCtrl &= ~(MDIO_MASTER_SLAVE_CTRL_ADV_FD | MDIO_MASTER_SLAVE_CTRL_ADV_HD);

        if(phy->gbit) {
            if(phy->config.advertise & ADVERTISED_1000baseT_Full) {
                phyGCtrl |= MDIO_MASTER_SLAVE_CTRL_ADV_FD;
            }
        }
    }

    switch(phy->config.speed) {
        case SPEED_10:
            // bits already cleared
            break;
        case SPEED_100:
            if (phy->config.port == PORT_FIBRE) {
                avmnet_mdio_write(this, ATHR_PHY_CHIP_CONFIG, PHY_CHIP_CONFIG_FX100_RGMII_50);
            }
            phyCtrl |= MDIO_CONTROL_SPEEDSEL_LSB;
            break;
        case SPEED_1000:
            if(phy->gbit){
                if (phy->config.port == PORT_FIBRE) {
                    avmnet_mdio_write(this, ATHR_PHY_CHIP_CONFIG, PHY_CHIP_CONFIG_BX1000_RGMII_50);
                }
                phyCtrl |= MDIO_CONTROL_SPEEDSEL_MSB;
            }else{
                AVMNET_ERR("[%s] PHY %s not supporting GBit speed\n", __func__, this->name);
            }
            break;
        default:
            // default to 10MBit?
            break;
    }

    if(phy->config.duplex){
        phyCtrl |= MDIO_CONTROL_FULLDUPLEX;
    }

    if(phy->config.autoneg == AUTONEG_ENABLE){
        phyCtrl |= MDIO_CONTROL_AUTONEG_RESTART;
    }

    AVMNET_TRC("[%s] module '%s' phyCtrl: 0x%04x phyAdv: 0x%04x phyGCtrl: 0x%04x\n",
            __func__, this->name, phyCtrl, phyAdv, phyGCtrl);

    if(phy->config.port == PORT_FIBRE) {
        if(phy->config.autoneg == AUTONEG_ENABLE) {
            /* No autonegotiation for 100BASE-FX */
            avmnet_mdio_write(this, ATHR_PHY_CHIP_CONFIG, PHY_CHIP_CONFIG_BX1000_RGMII_50);
            phy->config.speed = SPEED_1000;
        }
        avmnet_mdio_write(this, MDIO_FIBER_AUTONEG_ADV, phyAdv);
    } else {
        avmnet_mdio_write(this, MDIO_AUTONEG_ADV, phyAdv);
        avmnet_mdio_write(this, MDIO_MASTER_SLAVE_CTRL, phyGCtrl);   /*--- set or clear 1GBit Advertisement ---*/
    }
    avmnet_mdio_write(this, MDIO_CONTROL, phyCtrl);

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
u32 avmnet_ar803x_ethtool_get_link(avmnet_module_t *this)
{
    avmnet_linkstatus_t status;

    status = get_status(this);

    return status.Details.link;
}
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
unsigned int avmnet_ar803x_reg_read(avmnet_module_t *this, unsigned int addr, unsigned int reg)
{
    return this->parent->reg_read(this->parent, addr, reg);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_ar803x_reg_write(avmnet_module_t *this, unsigned int addr,
                                unsigned int reg, unsigned int val)
{
    return this->parent->reg_write(this->parent, addr, reg, val);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_ar803x_lock(avmnet_module_t *this) {
    return this->parent->lock(this->parent);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void avmnet_ar803x_unlock(avmnet_module_t *this) {
    this->parent->unlock(this->parent);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_ar803x_trylock(avmnet_module_t *this) {
    return this->parent->trylock(this->parent);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void avmnet_ar803x_status_changed(avmnet_module_t *this, avmnet_module_t *caller)
{
    int i;
    // struct athphy_context *my_ctx = (struct athphy_context *) this->priv;
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
int avmnet_ar803x_suspend(avmnet_module_t *this __attribute__ ((unused)), avmnet_module_t *caller __attribute__ ((unused)))
{
    // TODO

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_ar803x_resume(avmnet_module_t *this __attribute__ ((unused)), avmnet_module_t *caller __attribute__ ((unused)))
{
    // TODO

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_ar803x_reinit(avmnet_module_t *this __attribute__ ((unused)))
{
    // TODO

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static int phy_reg_show(struct seq_file *seq, void *data __attribute__ ((unused)))
{
    avmnet_module_t *this;
    struct athphy_context *phy;
    int mdio_reg, mdio_addr, result;
    unsigned int reg_val;

    this = (avmnet_module_t *) seq->private;
    phy = (struct athphy_context *) this->priv;
    result = 0;

    mdio_addr = this->initdata.phy.mdio_addr;
    seq_printf(seq, "----------------------------------------\n");
    seq_printf(seq, "dumping regs for phy %s [mdio:%#x]\n", this->name, mdio_addr);

    for(mdio_reg = 0; mdio_reg < 0x20; ++mdio_reg){
        if (this->lock(this)) {
            AVMNET_INFO("[%s] Interrupted while waiting for lock.\n", __func__);
            result = -EINTR;
            break;
        }
        reg_val = avmnet_mdio_read(this, mdio_reg);
        /*
         * link state is latched low in status reg, so . Make sure we don't miss a lost link
         * indicator.
         */
        if(mdio_reg == MDIO_STATUS && phy->state == LINK_UP && !(reg_val & MDIO_STATUS_LINK)){
            phy->state = LINK_LOST;
            phy->lost_time = jiffies;
        }
        this->unlock(this);

        seq_printf(seq, "    reg[%#x]: %#x\n", mdio_reg, reg_val);
    }
    seq_printf(seq, "----------------------------------------\n");

    return result;
}

static int phy_reg_open(struct inode *inode, struct file *file)
{
    avmnet_module_t *this;

    this = (avmnet_module_t *) PDE_DATA(inode);

    return single_open(file, phy_reg_show, this);
}

static ssize_t phy_reg_write(struct file *filp, const char __user *buff, size_t len,
        loff_t *offset __attribute__ ((unused)))
{
    avmnet_module_t *this;
    struct seq_file *seq;
    char mybuff[128];
    unsigned int reg, value;

    if(len >= 128){
        return -ENOMEM;
    }

    seq = (struct seq_file *) filp->private_data;
    this = (avmnet_module_t *) seq->private;

    copy_from_user(&mybuff[0], buff, len);
    mybuff[len] = 0;

    if(sscanf(mybuff, "%u %u", &reg, &value) == 2){
        if (this->lock(this)) {
            AVMNET_INFO("[%s] Interrupted while waiting for lock.\n", __func__);
            return -EINTR;
        }

        avmnet_mdio_write(this, reg, value);
        this->unlock(this);
    }

    return len;
}

#ifdef INFINEON_CPU
static void reset_phy(avmnet_module_t *this, enum rst_type type)
{
    if((this->initdata.phy.flags & AVMNET_CONFIG_FLAG_RESET) == 0){
        AVMNET_ERR("[%s] Reset not configured for module %s\n", __func__, this->name);
        return;
    }

    if ( ! this->trylock(this)) {
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
static void reset_phy(avmnet_module_t *this, enum rst_type type)
{
    if((this->initdata.phy.flags & AVMNET_CONFIG_FLAG_RESET) == 0){
        AVMNET_ERR("[%s] Reset not configured for module %s\n", __func__, this->name);
        return;
    }

    if ( ! this->trylock(this)) {
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
#else
static void reset_phy(avmnet_module_t *this, enum rst_type type __maybe_unused)
{
    AVMNET_ERR("[%s] not implemented yet for module %s!\n", __func__, this->name);
}
#endif // INFINEON_CPU
