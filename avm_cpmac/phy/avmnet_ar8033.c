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

#if defined(CONFIG_MACH_ATHEROS) || defined(CONFIG_ATH79)
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,4,46)
#include <atheros.h>
#include <atheros_gpio.h>
#else
#include <avm_atheros.h>
#include <avm_atheros_gpio.h>
#endif
#endif

#define MODULE_NAME "AVMNET_AR8033"

#define AR803x_PHY_ID1 0x004D
#define AR803x_PHY_ID2 0xD072
#define AR8326_PHY_ID2 0xD042
#define AR8033_PHY_ID2 0xD043

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
static struct resource ar8033_gpio ={.name = "ar8033_reset",
                                     .flags = IORESOURCE_IO,
                                     .start = -1,
                                     .end = -1};
#endif // CONFIG_MACH_ATHEROS != n
static int set_config(avmnet_module_t *this);
static int setup_phy(avmnet_module_t *this);
static void reset_phy(avmnet_module_t *this, enum rst_type type);
static unsigned int setup_sgmii(void);
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

#define SGMII_LINK_WAR_MAX_TRY 10

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
#define LED_OVERRIDE 0x19
#define LED_NORMAL  0
#define LED_ACT     1
#define LED_OFF     2
#define LED_ON      3

union ar8033_led_override {
    unsigned short Register;
    struct _ar8033_led_override {
        unsigned int res13_15   : 3;
        unsigned int act_ctrl   : 1;
        unsigned int res8_11    : 4;
        unsigned int led_link   : 2;
        unsigned int res45      : 2;
        unsigned int led_tx     : 2;
        unsigned int led_rx     : 2;
    } Bits;
};

static void gpio_work_handler(struct work_struct *data)
{
    struct athphy_context *phy;
    avmnet_module_t *this;
    unsigned int gpio;
    unsigned int on;
    union ar8033_led_override value;

    phy = container_of(
            container_of(to_delayed_work(data), avmnet_work_t, work),
            struct athphy_context, gpio_work);

    this = phy->this_module;

    if (this->lock(this)) {
        AVMNET_ERR("[%s] down_interruptible\n", __func__);
        return;
    }

    AVMNET_DEBUG("(%s) read %d write %d\n", __func__, phy->gpio.read, phy->gpio.write);

#if 0
    {
        unsigned short tmp = avmnet_mdio_read(this, LED_OVERRIDE - 1);

        AVMNET_ERR("(%s) LED_OVERRIDE - 1 0x%x\n", __func__, tmp);
        avmnet_mdio_write(this, LED_OVERRIDE - 1, tmp & ~(0x1c));
        AVMNET_ERR("(%s) LED_OVERRIDE - 1 0x%x\n", __func__, avmnet_mdio_read(this, LED_OVERRIDE - 1));
    }
#endif

    while (phy->gpio.read != phy->gpio.write) {

        value.Register = avmnet_mdio_read(this, LED_OVERRIDE);
        AVMNET_DEBUG("(%s) LED_OVERRIDE 0x%x read %d gpio 0x%x\n", __func__, 
                value.Register, phy->gpio.read, phy->gpio.queue[phy->gpio.read]);

        gpio = phy->gpio.queue[phy->gpio.read] & 0xff;
        on   = phy->gpio.queue[phy->gpio.read] >> 8;
        phy->gpio.read++;
        if (phy->gpio.read == AVMNET_GPIO_QUEUELEN)
            phy->gpio.read = 0;

        if (gpio & (1<<0)) {
            if (on) {
                value.Bits.led_tx = LED_ON;
                value.Bits.led_rx = LED_ON;
            } else {
                value.Bits.led_tx = LED_OFF;
                value.Bits.led_rx = LED_OFF;
            }
        }

        if (gpio & (1<<1)) {
            if (on) {
                value.Bits.led_link = LED_ON;
            } else {
                value.Bits.led_link = LED_OFF;
            }
        }

        avmnet_mdio_write(this, LED_OVERRIDE, value.Register);

        AVMNET_DEBUG("(%s) LED_OVERRIDE value 0x%x\n", __func__, value.Register);
    }

    this->unlock(this);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_ar8033_gpio_set(avmnet_module_t *this, unsigned int gpio, unsigned int on_off) {

    struct athphy_context *phy = (struct athphy_context *) this->priv;

    AVMNET_DEBUG("[%s] device %s gpio %d on_off %d\n", __func__, this->name, gpio, on_off);

    if (gpio > 2) { 
        AVMNET_ERR("[%s] ERROR GPIO unknown\n", __func__);
        return -1;
    }
    phy->gpio.queue[phy->gpio.write++] = (1 << gpio) | (on_off << 8);
    if (phy->gpio.write == AVMNET_GPIO_QUEUELEN) 
        phy->gpio.write = 0;

    queue_delayed_work(phy->gpio_work.workqueue, &(phy->gpio_work.work), 0);

    return 0;
}

/*------------------------------------------------------------------------------------------*\
 * wird nur gebraucht, um das Linkdesaster in den Griff zu bekommen
\*------------------------------------------------------------------------------------------*/
static void link_work_handler(struct work_struct *data)
{
    struct athphy_context *phy;
    unsigned int status, count = SGMII_LINK_WAR_MAX_TRY;
    avmnet_module_t *this;

    phy = container_of(
            container_of(to_delayed_work(data), avmnet_work_t, work),
            struct athphy_context, link_work);

    this = phy->this_module;

    if (this->lock(this)) {
        AVMNET_ERR("[%s] failed to get locked\n", __func__);
        return;
    }

    AVMNET_ERR("[%s] Link changed status 0x%x\n", __func__, ath_reg_rd(SGMII_DEBUG_ADDRESS));

    status = ath_reg_rd(SGMII_DEBUG_ADDRESS);
    if (((status >> 24) & 0xF) == 0x3) {
        /*--- hier im IRQ gibt es erstmal nichts zu tun ---*/
        this->unlock(this);
        return;
    } 

    status = (ath_reg_rd(SGMII_DEBUG_ADDRESS) & 0xff);
    while (!(status == 0xf || status == 0x10 || status == 0x14)) {

        ath_reg_rmw_set(MR_AN_CONTROL_ADDRESS, MR_AN_CONTROL_PHY_RESET_SET(1));
        udelay(100);
        ath_reg_rmw_clear(MR_AN_CONTROL_ADDRESS, MR_AN_CONTROL_PHY_RESET_SET(1));
        if ( ! --count) {
            AVMNET_ERR("[%s] Max resets limit reached exiting...\n", __func__);
            break;
        }
        udelay(100);
        status = (ath_reg_rd(SGMII_DEBUG_ADDRESS) & 0xff);
    }

    this->unlock(this);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static irqreturn_t ar8033_link_intr(int irq __attribute__((unused)), void *dev)
{
    avmnet_module_t *this = (avmnet_module_t *) dev;
    struct athphy_context *phy = (struct athphy_context *) this->priv;
    unsigned int irqStatus, status;

    irqStatus = ath_reg_rd(SGMII_INTERRUPT_ADDRESS);

    AVMNET_DEBUG("[%s] device %s irq %d 0x%x\n", __func__, this->name, phy->irq, irqStatus);

    ath_reg_rmw_clear(SGMII_INTERRUPT_ADDRESS, irqStatus);     /*--- clear IRQ-Bits ---*/

    if ((irqStatus & SGMII_LINK_MAC_CHANGE) || (irqStatus & SGMII_MR_AN_COMPLETE)) {
        AVMNET_ERR("[%s] SGMII Link mac change  Status 0x%x\n", __func__, irqStatus);

        status = ath_reg_rd(SGMII_SERDES_ADDRESS);
        if (SGMII_SERDES_FIBER_SDO_GET(status)) {
            queue_delayed_work(phy->link_work.workqueue, &(phy->link_work.work), 0);
        }
    }

    return IRQ_HANDLED;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static unsigned int sgmii_reset_count = 0;
static avmnet_linkstatus_t get_status(avmnet_module_t *this)
{
    struct athphy_context *phy = (struct athphy_context *) this->priv;
    avmnet_linkstatus_t status;
    uint32_t phyStatus, phyHwStatus, phyControl, phyResolve; 
    uint32_t sgmii_status;
#if AVMNET_DEBUG_LEVEL & AVMNET_DEBUG_LEVEL_DEBUG
    uint16_t irqStatus;
#endif

    status.Status = 0;

    if(phy->powerdown){
        return status;
    }

    if (this->lock(this)) {
        return status;
    }

#if AVMNET_DEBUG_LEVEL & AVMNET_DEBUG_LEVEL_DEBUG
    irqStatus   = ath_reg_rd(SGMII_INTERRUPT_ADDRESS);
#endif
    phyControl  = ath_reg_rd(MR_AN_CONTROL_ADDRESS);
    phyStatus   = ath_reg_rd(MR_AN_STATUS_ADDRESS);
    phyHwStatus = ath_reg_rd(SGMII_MAC_RX_CONFIG_ADDRESS);
    phyResolve  = ath_reg_rd(SGMII_RESOLVE_ADDRESS);

    AVMNET_DEBUG("[%s] device %s irqStatus 0x%x phyControl 0x%x phyStatus 0x%x phyHwStatus 0x%x 0x%x\n",
            __func__, this->name, irqStatus, phyControl, phyStatus, phyHwStatus, ath_reg_rd(SGMII_DEBUG_ADDRESS));

    /*--- beim Linkaufbau kann sich der SGMII verheddern oder der Link geht sonstwie verloren ---*/
    if (sgmii_reset_count == SGMII_LINK_WAR_MAX_TRY) {
        union ar8033_led_override led_status;

        AVMNET_ERR("[%s] %s Reset PHY - sgmii_reset_count %d SGMII-Debug 0x%x\n", __func__, this->name, 
                sgmii_reset_count, ath_reg_rd(SGMII_DEBUG_ADDRESS));

        led_status.Register = avmnet_mdio_read(this, LED_OVERRIDE);       /*--- remember LED-Status ---*/
        if (setup_phy(this)) {
            BUG();      /*--- damit hier ein Fehler nicht unbemerkt bleibt ---*/
        }
        avmnet_mdio_write(this, LED_OVERRIDE, led_status.Register);
        sgmii_reset_count = 0;
    }

    /*--- wenn der PHY im Powersave ist, geht der SGMII-Link verloren ---*/
    sgmii_status = ath_reg_rd(SGMII_DEBUG_ADDRESS);
    if (((sgmii_status >> 24) & 0xF) == 0x3) {
        /*--- hier sofort reseten, sonst wird kein Link mehr erkannt ---*/
        AVMNET_ERR("[%s] Reset SGMII sgmii_status 0x%x 0x%x\n", __func__, ath_reg_rd(SGMII_DEBUG_ADDRESS), sgmii_status);
        sgmii_reset_count = setup_sgmii();
        this->unlock(this);
        return status;
    } 

    /*--- nur testen wenn der PHY nicht im Powersave ist, funktioniert automatisch Stichwort SmartEEE SmartSpeed EEE advertisement ---*/
    if(MR_AN_STATUS_AN_COMPLETE_GET(phyStatus)) {       
        sgmii_status = ath_reg_rd(SGMII_DEBUG_ADDRESS);
        if ( ! (((sgmii_status & 0xFF) == 0x10) || ((sgmii_status & 0xFF) == 0xF) || ((sgmii_status & 0xFF) == 0x14)) ) { 
            if (sgmii_reset_count++ > 8) {          /*--- erst später im Poll reseten, manchmal ist der Wert != 0x10 oder 0xF ---*/
                AVMNET_ERR("[%s] Reset SGMII sgmii_status 0x%x 0x%x sgmii_reset_count %d\n", __func__, ath_reg_rd(SGMII_DEBUG_ADDRESS), sgmii_status, sgmii_reset_count);
                sgmii_reset_count = setup_sgmii();
            }
        } else {
            sgmii_reset_count = 0;
        }
    }

    // first check that  phy is not in power-down mode
    if( ! MR_AN_CONTROL_POWER_DOWN_GET(phyControl)){
        status.Details.powerup = 1;

        if((phyStatus & (1<<2)) && 
           (phy->config.autoneg == AUTONEG_DISABLE || (phyStatus & (1<<5))))
        {
            AVMNET_DEBUG("[%s] device %s <PHY found LINK>\n", __func__, this->name);
            status.Details.link = 1;
            status.Details.fullduplex = SGMII_MAC_RX_CONFIG_DUPLEX_MODE_GET(phyHwStatus);
            status.Details.speed = SGMII_MAC_RX_CONFIG_SPEED_MODE_GET(phyHwStatus);

#if 0
            if(phyHwStatus & PHY_SPEC_STATUS_MDIX){
                status.Details.mdix = AVMNET_LINKSTATUS_MDI_MDIX;
            }else{
                status.Details.mdix = AVMNET_LINKSTATUS_MDI_MDI;
            }
#endif

            if( SGMII_RESOLVE_RX_PAUSE_GET(phyResolve)){
                status.Details.flowcontrol |= AVMNET_LINKSTATUS_FC_RX;
            }

            if( SGMII_RESOLVE_RX_PAUSE_GET(phyResolve) && ( phy->config.pause_setup != avmnet_pause_rx) ){

                // TX pause: there is no way to advertise RX-Only, so we advertise
                // symetric and asymetric RX, and disable TX pause frames here
                status.Details.flowcontrol |= AVMNET_LINKSTATUS_FC_TX;
            }
        } else {
            AVMNET_DEBUG("[%s] device %s <PHY no LINK>\n", __func__, this->name);
        }

    } else {
        AVMNET_DEBUG("[%s] device %s PHY in POWERDOWN\n", __func__, this->name);
    }

    this->unlock(this);
    return status;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_ar8033_status_poll(avmnet_module_t *this)
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

    parent->set_status(parent, this->device_id, status);

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static int setup_interrupt(avmnet_module_t *this, int on_off)
{
    struct athphy_context *phy = (struct athphy_context *) this->priv;

    AVMNET_DEBUG("[%s] %sable PHY-Interrupt on %s\n", __FUNCTION__, on_off ? "en":"dis", this->name);

    // warn on unsafe call
    if ( ! this->trylock(this)) {
        AVMNET_ERR("[%s] Called without MUTEX held!\n", __func__);
        dump_stack();
        this->unlock(this);
    }

    if(this->initdata.phy.flags & AVMNET_CONFIG_FLAG_IRQ) {
        if(on_off){
            ath_reg_wr(SGMII_INTERRUPT_MASK_ADDRESS, SGMII_INTR);
        }else{
            ath_reg_wr(SGMII_INTERRUPT_MASK_ADDRESS, 0);
        }

        phy->irq_on = on_off;

        return 0;
    }else{
        return 1;
    }
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_ar8033_setup_interrupt(avmnet_module_t *this, unsigned int on_off) {

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
int avmnet_ar8033_setup(avmnet_module_t *this)
{
    struct athphy_context *phy = (struct athphy_context *) this->priv;
    int i, result;

    AVMNET_INFO("[%s] Called for PHY %s\n", __func__, this->name);

    if (this->lock(this)) {
        return -EINTR;
    }

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
    phy->config.port = PORT_TP;
    phy->config.mdi = ETH_TP_MDI;

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

    result = setup_phy(this);

    AVMNET_DEBUG("[%s] Called for module %s SGMII_CONFIG 0x%x\n", __func__, this->name, ath_reg_rd(SGMII_CONFIG_ADDRESS));

    this->unlock(this);

    if(result != 0){
        return result;
    }

    if(this->initdata.phy.flags & AVMNET_CONFIG_FLAG_IRQ){
        result = request_irq(phy->irq,
                             ar8033_link_intr,
                             IRQF_SHARED | IRQF_TRIGGER_LOW,
                             "ar8033 link",
                             this);
        if(result < 0){
            AVMNET_ERR("[%s] <request irq ar8033 link %d failed>\n", __FUNCTION__, phy->irq);
            return result;
        }
    }

    for(i = 0; i < this->num_children; ++i){
        result = this->children[i]->setup(this->children[i]);
        if(result != 0){
            // handle error
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

    if (this->powerdown)
        this->powerdown(this);

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
/*--- #define ATHR_SGMII_FORCED ---*/
static unsigned int setup_sgmii(void) {

	uint32_t status = 0, count = 0;

#if defined(ATHR_SGMII_FORCED)
		ath_reg_wr(MR_AN_CONTROL_ADDRESS, MR_AN_CONTROL_SPEED_SEL1_SET(1) |
				                           MR_AN_CONTROL_PHY_RESET_SET(1) | 
                                           MR_AN_CONTROL_DUPLEX_MODE_SET(1));
		udelay(10);
		ath_reg_wr(SGMII_CONFIG_ADDRESS, SGMII_CONFIG_MODE_CTRL_SET(2) |
				                          SGMII_CONFIG_FORCE_SPEED_SET(1) | 
                                          SGMII_CONFIG_SPEED_SET(2));
#else
		ath_reg_wr(SGMII_CONFIG_ADDRESS, SGMII_CONFIG_MODE_CTRL_SET(2));
		ath_reg_wr(MR_AN_CONTROL_ADDRESS, MR_AN_CONTROL_AN_ENABLE_SET(1) |
				                          MR_AN_CONTROL_PHY_RESET_SET(1));
#endif

    /*------------------------------------------------------------------------------------------*\
	 * SGMII reset sequence suggested by systems team.
    \*------------------------------------------------------------------------------------------*/
	ath_reg_wr(SGMII_RESET_ADDRESS, SGMII_RESET_RX_CLK_N_RESET);
	ath_reg_wr(SGMII_RESET_ADDRESS, SGMII_RESET_HW_RX_125M_N_SET(1));
	ath_reg_wr(SGMII_RESET_ADDRESS, SGMII_RESET_HW_RX_125M_N_SET(1) |
			                         SGMII_RESET_RX_125M_N_SET(1));

	ath_reg_wr(SGMII_RESET_ADDRESS, SGMII_RESET_HW_RX_125M_N_SET(1) |
			                         SGMII_RESET_TX_125M_N_SET(1) |
			                         SGMII_RESET_RX_125M_N_SET(1));

	ath_reg_wr(SGMII_RESET_ADDRESS, SGMII_RESET_HW_RX_125M_N_SET(1) |
			                         SGMII_RESET_TX_125M_N_SET(1) |
			                         SGMII_RESET_RX_125M_N_SET(1) |
			                         SGMII_RESET_RX_CLK_N_SET(1));

	ath_reg_wr(SGMII_RESET_ADDRESS, SGMII_RESET_HW_RX_125M_N_SET(1) |
			                         SGMII_RESET_TX_125M_N_SET(1) |
			                         SGMII_RESET_RX_125M_N_SET(1) |
			                         SGMII_RESET_RX_CLK_N_SET(1) |
			                         SGMII_RESET_TX_CLK_N_SET(1));

	ath_reg_rmw_clear(MR_AN_CONTROL_ADDRESS, MR_AN_CONTROL_PHY_RESET_SET(1));

    /*------------------------------------------------------------------------------------------*\
     * WAR::Across resets SGMII link status goes to weird
	 * state.
	 * if 0xb8070058 (SGMII_DEBUG register) reads other then 0x1f or 0x10
	 * for sure we are in bad  state.
	 * Issue a PHY reset in MR_AN_CONTROL_ADDRESS to keep going.
    \*------------------------------------------------------------------------------------------*/
    do {
		ath_reg_rmw_set(MR_AN_CONTROL_ADDRESS, MR_AN_CONTROL_PHY_RESET_SET(1));
		udelay(200);
		ath_reg_rmw_clear(MR_AN_CONTROL_ADDRESS, MR_AN_CONTROL_PHY_RESET_SET(1));
        mdelay(1000);
		if (count++ == SGMII_LINK_WAR_MAX_TRY) {
			AVMNET_ERR("[%s] Max resets limit reached exiting...\n", __func__);
			return SGMII_LINK_WAR_MAX_TRY;
		}
		status = (ath_reg_rd(SGMII_DEBUG_ADDRESS) & 0xff);
    } while (!(status == 0xf || status == 0x10));

	AVMNET_INFO("[%s] Done\n", __func__);
    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static int setup_phy(avmnet_module_t *this)
{
    struct athphy_context *phy = (struct athphy_context *) this->priv;
    unsigned int i;
    int status = 0;

    AVMNET_INFO("[%s] called for PHY %s\n", __func__, this->name);

    // warn on unsafe call
    if ( ! this->trylock(this)) {
        AVMNET_ERR("[%s] Called without MUTEX held!\n", __func__);
        dump_stack();
        this->unlock(this);
    }

    reset_phy(this, rst_pulse);

    i = 0;
    while(avmnet_mdio_read(this, MDIO_PHY_ID1) == 0xFFFF && i < 10){
        ++i;
        AVMNET_WARN("[%s] PHY %s hanging, trying to reset it.\n", __func__, this->name);
        reset_phy(this, rst_pulse);
        mdelay(1000);
    }

    AVMNET_DEBUG("{%s} PHY ID 0x%x\n", __func__, avmnet_mdio_read(this, MDIO_PHY_ID1));

    if((i >= 10) || ( ! avmnet_mdio_read(this, MDIO_PHY_ID1))) {
        AVMNET_ERR("[%s] Giving up on hanging PHY %s ID 0x%x\n", __func__, this->name, avmnet_mdio_read(this, MDIO_PHY_ID1));
        return -EFAULT;
    }

    /*--- select CoperPageRegister, Media Copper, SGMII-Interface ---*/
    avmnet_mdio_write(this, ATHR_PHY_CHIP_CONFIG, PHY_CHIP_CONFIG_BT_BX_REG_SEL | PHY_CHIP_CONFIG_BASET_SGMII);
    mdelay(10);

    { 
        unsigned short dummy;
        // setup SGMII-TX Outputvoltage to 600mV
        avmnet_mdio_write(this, MDIO_MMD_CTRL, 0x7);
        avmnet_mdio_write(this, MDIO_MMD_ADDR, 0x8011);
        avmnet_mdio_write(this, MDIO_MMD_CTRL, 0x4007);
        dummy = avmnet_mdio_read(this, MDIO_MMD_ADDR);
        AVMNET_DEBUG("[%s] SGMII Drive CTRL: %04x\n", __func__, dummy);
        avmnet_mdio_write(this, MDIO_MMD_ADDR, (dummy & 0x1FFF) | 1 << 13);
    }

    if(this->initdata.phy.flags & AVMNET_CONFIG_FLAG_RX_DELAY){
        avmnet_mdio_write(this, ATHR_PHY_DEBUG_PORT_ADDRESS, 0);
        avmnet_mdio_write(this, ATHR_PHY_DEBUG_PORT_DATA, this->initdata.phy.rx_delay);
    }
    if(this->initdata.phy.flags & AVMNET_CONFIG_FLAG_TX_DELAY){
        avmnet_mdio_write(this, ATHR_PHY_DEBUG_PORT_ADDRESS, 5);
        avmnet_mdio_write(this, ATHR_PHY_DEBUG_PORT_DATA, this->initdata.phy.tx_delay);
    }

    if(this->initdata.phy.flags & AVMNET_CONFIG_FLAG_IRQ) {
        status |= setup_interrupt(this, phy->irq_on);
    }

    phy->state = LINK_DOWN;

    status |= set_config(this);

    status |= setup_sgmii();

    if (phy->powerdown) {
        this->powerdown(this);
    }

    return status;

}
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_ar8033_init(avmnet_module_t *this)
{
    struct athphy_context *phy;
    int i, result;

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

        if(this->initdata.phy.reset < 100){
            /*--- Hängt nicht am Shift-Register => GPIO Resourcen anmelden ---*/
            ar8033_gpio.start = this->initdata.phy.reset;
            ar8033_gpio.end = this->initdata.phy.reset;
            if(request_resource(&gpio_resource, &ar8033_gpio)){
                AVMNET_ERR(KERN_ERR "[%s] ERROR request resource gpio %d\n", __func__, this->initdata.phy.reset);
                kfree(phy);
                return -EIO;
            }
        }

        if (this->lock(this)) {
            AVMNET_ERR(KERN_ERR "[%s] ERROR lock\n", __func__);
            kfree(phy);
            return -EINTR;
        }
        reset_phy(this, rst_on);
        this->unlock(this);
    } else {
        AVMNET_ERR("[%s] No RESET configured for PHY %s.\n", __func__, this->name);
        BUG();
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

    if (this->set_gpio) {
        phy->gpio_work.workqueue = avmnet_get_workqueue();
        if(phy->gpio_work.workqueue == NULL){
            AVMNET_ERR("[%s] No global workqueue available!\n>", __FUNCTION__);
            kfree(phy);
            return -EFAULT;
        }
        INIT_DELAYED_WORK(&(phy->gpio_work.work), gpio_work_handler);
        phy->gpio.write = 0;
        phy->gpio.read = 0;
    }

    if(this->initdata.phy.flags & AVMNET_CONFIG_FLAG_IRQ){
        phy->irq = this->initdata.phy.irq;
    }

    for(i = 0; i < this->num_children; ++i){
        result = this->children[i]->init(this->children[i]);
        if(result != 0){
            // handle error
        }
    }

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_ar8033_exit(avmnet_module_t *this)
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
        free_irq(phy->irq, ar8033_link_intr);
        cancel_delayed_work_sync(&(phy->link_work.work));
    }

    kfree(this->priv);

    return result;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_ar8033_powerdown(avmnet_module_t *this)
{
    struct athphy_context *phy = (struct athphy_context *) this->priv;

    if (this->lock(this)) {
        return -EINTR;
    }

    phy->powerdown = 1;

    avmnet_mdio_write(this, MDIO_CONTROL, (MDIO_CONTROL_POWERDOWN | MDIO_CONTROL_ISOLATE));

    AVMNET_ERR("[%s] Called for module %s\n", __func__, this->name);

    this->unlock(this);
    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_ar8033_powerup(avmnet_module_t *this)
{
    struct athphy_context *phy = (struct athphy_context *) this->priv;
    int result = 0;

    if (this->lock(this)) {
        return -EINTR;
    }

    avmnet_mdio_write(this, MDIO_CONTROL, MDIO_CONTROL_RESET | MDIO_CONTROL_AUTONEG_ENABLE | MDIO_CONTROL_ISOLATE);
    msleep(500);

    phy->powerdown = 0;

    AVMNET_ERR("[%s] Called for module %s\n", __func__, this->name);

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
    return result;
}
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_ar8033_ethtool_get_settings(avmnet_module_t *this, struct ethtool_cmd *cmd)
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
    phyAdv = avmnet_mdio_read(this, MDIO_AUTONEG_ADV);
    phyGCtrl = avmnet_mdio_read(this, MDIO_MASTER_SLAVE_CTRL);

    this->unlock(this);

    cmd->supported = phy->config.supported;

    /*
     * Due to unreliable register reads in power-down mode, we just return
     * the configuration that will be set on power-up for advertising and autoneg
     */
    if(phy->powerdown){
        cmd->advertising = phy->config.advertise;
        cmd->autoneg = phy->config.autoneg;
    }else{
        cmd->advertising = 0;

        if(phyCtrl & MDIO_CONTROL_AUTONEG_ENABLE){
            cmd->autoneg = AUTONEG_ENABLE;

            if(phyAdv & MDIO_AUTONEG_ADV_10BT_HDX){
                cmd->advertising |= ADVERTISED_10baseT_Half;
            }

            if(phyAdv & MDIO_AUTONEG_ADV_10BT_FDX){
                cmd->advertising |= ADVERTISED_10baseT_Full;
            }

            if(phyAdv & MDIO_AUTONEG_ADV_100BT_HDX){
                cmd->advertising |= ADVERTISED_100baseT_Half;
            }

            if(phyAdv & MDIO_AUTONEG_ADV_100BT_FDX){
                cmd->advertising |= ADVERTISED_100baseT_Full;
            }

            if(phyAdv & MDIO_AUTONEG_ADV_10BT_HDX){
                cmd->advertising |= ADVERTISED_10baseT_Half;
            }

            if(phyAdv & MDIO_AUTONEG_ADV_PAUSE_ASYM){
                cmd->advertising |= ADVERTISED_Asym_Pause;
            }

            if(phyAdv & MDIO_AUTONEG_ADV_PAUSE_SYM){
                cmd->advertising |= ADVERTISED_Pause;
            }

            cmd->advertising |= ADVERTISED_TP | ADVERTISED_Autoneg;
        }else{
            cmd->autoneg = AUTONEG_DISABLE;
        }

        if(phy->gbit){
            if(cmd->autoneg == AUTONEG_ENABLE){
                if(phyGCtrl & MDIO_MASTER_SLAVE_CTRL_ADV_HD){
                    cmd->advertising |= ADVERTISED_1000baseT_Half;
                }

                if(phyGCtrl & MDIO_MASTER_SLAVE_CTRL_ADV_FD){
                    cmd->advertising |= ADVERTISED_1000baseT_Full;
                }
            }
        }
    }

    cmd->port = PORT_TP;
    cmd->phy_address = this->initdata.phy.mdio_addr;

    if(status.Details.link){
        switch(status.Details.speed){
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

        if(status.Details.fullduplex){
            cmd->duplex = DUPLEX_FULL;
        }else{
            cmd->duplex = DUPLEX_HALF;
        }

        if(status.Details.mdix == AVMNET_LINKSTATUS_MDI_MDIX){
            cmd->eth_tp_mdix = ETH_TP_MDI_X;
        }else{
            cmd->eth_tp_mdix = ETH_TP_MDI;
        }
    }else{
        cmd->speed = -1;
        cmd->duplex = -1;
        cmd->eth_tp_mdix = ETH_TP_MDI_INVALID;
    }

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_ar8033_ethtool_set_settings(avmnet_module_t *this, struct ethtool_cmd *cmd)
{
    struct athphy_context *phy = (struct athphy_context *) this->priv;
    int result;
    __u32 dummy_adv;

    AVMNET_DEBUG("[%s] Called for module %s\n", __func__, this->name);

    if (this->lock(this)) {
        return -EINTR;
    }

    if(cmd->autoneg == AUTONEG_DISABLE){

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
            AVMNET_ERR("[%s] PHY %s: received bogus ethtool_cmd.\n", __func__, this->name);
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
void avmnet_ar8033_ethtool_get_pauseparam(avmnet_module_t *this,
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
int avmnet_ar8033_ethtool_set_pauseparam(avmnet_module_t *this,
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
static int set_config(avmnet_module_t *this)
{
    struct athphy_context *phy = (struct athphy_context *) this->priv;
    unsigned int phyCtrl, phyAdv, phyGCtrl = 0;
    /*--- unsigned int dummy; ---*/

    if( ! this->trylock(this)) {
        AVMNET_ERR("[%s] Called without MUTEX held!\n", __func__);
        dump_stack();
        this->unlock(this);
    }

#if 0
    // disable SmartSpeed
    avmnet_mdio_write(this, ATHR_PHY_SMARTSPEED, 0);
    avmnet_mdio_write(this, MDIO_CONTROL, MDIO_CONTROL_RESET);

    // disable SmartEEE
    avmnet_mdio_write(this, MDIO_MMD_CTRL, 0x3);
    avmnet_mdio_write(this, MDIO_MMD_ADDR, 0x805D);
    avmnet_mdio_write(this, MDIO_MMD_CTRL, 0x4003);
    dummy = avmnet_mdio_read(this, MDIO_MMD_ADDR);
    avmnet_mdio_write(this, MDIO_MMD_ADDR, 0x0);
    AVMNET_ERR("[%s] SmartEEE CTRL3: %04x\n", __func__, dummy);

    // disable EEE advertisement
    avmnet_mdio_write(this, MDIO_MMD_CTRL, 0x7);
    avmnet_mdio_write(this, MDIO_MMD_ADDR, 0x3C);
    avmnet_mdio_write(this, MDIO_MMD_CTRL, 0x4007);
    dummy = avmnet_mdio_read(this, MDIO_MMD_ADDR);
    avmnet_mdio_write(this, MDIO_MMD_ADDR, 0x0);
    AVMNET_ERR("[%s] EEE Adv: %04x\n", __func__, dummy);

    // disable hibernation
    avmnet_mdio_write(this, ATHR_PHY_DEBUG_PORT_ADDRESS, 0xB);
    avmnet_mdio_write(this, ATHR_PHY_DEBUG_PORT_DATA, 0);
#endif

    phyCtrl = MDIO_CONTROL_ISOLATE;     /*--- RGMII-Interface abschalten ---*/

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

    if(phy->config.autoneg == AUTONEG_ENABLE){
        phyCtrl |= MDIO_CONTROL_AUTONEG_ENABLE;
    }

    phyGCtrl = avmnet_mdio_read(this, MDIO_MASTER_SLAVE_CTRL);
    phyGCtrl &= ~(MDIO_MASTER_SLAVE_CTRL_ADV_FD | MDIO_MASTER_SLAVE_CTRL_ADV_HD);

    if(phy->gbit){
        if(phy->config.advertise & ADVERTISED_1000baseT_Full){
            phyGCtrl |= MDIO_MASTER_SLAVE_CTRL_ADV_FD;
        }
    } 

    switch(phy->config.speed){
    case SPEED_10:
        // bits already cleared
        break;
    case SPEED_100:
        phyCtrl |= MDIO_CONTROL_SPEEDSEL_LSB;
        break;
    case SPEED_1000:
        if(phy->gbit){
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

    avmnet_mdio_write(this, MDIO_AUTONEG_ADV, phyAdv);
    avmnet_mdio_write(this, MDIO_CONTROL, phyCtrl);

    avmnet_mdio_write(this, MDIO_MASTER_SLAVE_CTRL, phyGCtrl);   /*--- set or clear 1GBit Advertisement ---*/

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
u32 avmnet_ar8033_ethtool_get_link(avmnet_module_t *this)
{
    avmnet_linkstatus_t status;

    status = get_status(this);

    return status.Details.link;
}
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
unsigned int avmnet_ar8033_reg_read(avmnet_module_t *this, unsigned int addr, unsigned int reg)
{
    return this->parent->reg_read(this->parent, addr, reg);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_ar8033_reg_write(avmnet_module_t *this, unsigned int addr, unsigned int reg, unsigned int val)
{
    return this->parent->reg_write(this->parent, addr, reg, val);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_ar8033_lock(avmnet_module_t *this) {
    return this->parent->lock(this->parent);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void avmnet_ar8033_unlock(avmnet_module_t *this) {
    this->parent->unlock(this->parent);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_ar8033_trylock(avmnet_module_t *this) {
    return this->parent->trylock(this->parent);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void avmnet_ar8033_status_changed(avmnet_module_t *this, avmnet_module_t *caller)
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
int avmnet_ar8033_suspend(avmnet_module_t *this __attribute__ ((unused)), avmnet_module_t *caller __attribute__ ((unused)))
{
    // TODO

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_ar8033_resume(avmnet_module_t *this __attribute__ ((unused)), avmnet_module_t *caller __attribute__ ((unused)))
{
    // TODO

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_ar8033_reinit(avmnet_module_t *this __attribute__ ((unused)))
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

static void reset_phy(avmnet_module_t *this, enum rst_type type)
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
