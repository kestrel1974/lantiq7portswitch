/*------------------------------------------------------------------------------------------*\
 *   Copyright (C) 2011, ..., 2014 AVM GmbH <fritzbox_info@avm.de>
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

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/semaphore.h>
#include <linux/string.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/if_vlan.h>

#if defined(CONFIG_MACH_ATHEROS) || defined(CONFIG_ATH79)
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,4,46)
#include <atheros.h>
#include <atheros_gpio.h>
#else
#include <avm_atheros.h>
#include <avm_atheros_gpio.h>
#endif
#include <irq.h>
#endif

#undef INFINEON_CPU
#if defined(CONFIG_VR9) || defined(CONFIG_AR9) || defined(CONFIG_AR10)
#define INFINEON_CPU
#include <ifx_gpio.h>
#include <ifx_types.h>
#endif

#include "avmnet_debug.h"
#include "avmnet_module.h"
#include "avmnet_config.h"
#include "avmnet_common.h"
#include "athrs_phy_ctrl.h"
#include "avmnet_ar8337.h"

/* Communication with AVM_PA for programming network sessions */
#if defined(CONFIG_AVM_CPMAC_ATH_PPA)
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

static signed int avmnet_ar8337_add_session(struct avm_pa_session *avm_session);
static signed int avmnet_ar8337_remove_session(struct avm_pa_session *avm_session);
#if defined(AVM_PA_HARDWARE_PA_HAS_SESSION_STATS)
static signed int avmnet_ar8337_session_stats(struct avm_pa_session *avm_session, struct avm_pa_session_stats *ingress);
static void work_acl_policer(struct aths17_context *ctx);
#endif
static void bridge_work_acl(struct aths17_context *ctx);

static struct avm_hardware_pa ar8337_hw_pa = {
   .add_session               = avmnet_ar8337_add_session,
   .remove_session            = avmnet_ar8337_remove_session,
   .change_session            = NULL,
   .session_state             = NULL,
   .try_to_accelerate         = NULL,
   .alloc_rx_channel          = NULL,
   .alloc_tx_channel          = NULL,
   .free_rx_channel           = NULL,
   .free_tx_channel           = NULL,
#if defined(AVM_PA_HARDWARE_PA_HAS_SESSION_STATS)
   .session_stats             = avmnet_ar8337_session_stats,
#endif
};
#endif /*--- #if defined(CONFIG_AVM_CPMAC_ATH_PPA) ---*/

#define MODULE_NAME "AVMNET_AR8337"
#define AR8337_MAX_PHYS 6

#define SGMII_LINK_WAR_MAX_TRY 10

#if defined(CONFIG_MACH_ATHEROS) || defined(CONFIG_ATH79)
static struct resource ar8337_gpio = {
    .name  = "ar8337_reset",
    .flags = IORESOURCE_IO,		 
    .start = -1,
    .end   = -1
};
#endif

static int read_rmon_all_open(struct inode *inode, struct file *file);
static const struct file_operations read_rmon_all_fops = {
    .open    = read_rmon_all_open,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = single_release,
};

static int update_rmon_cache(avmnet_module_t *sw_mod);

#if defined(CONFIG_AVM_PA) && defined(CONFIG_AVM_CPMAC_ATH_PPA)
static int acl_read_open(struct inode *inode, struct file *file);
static const struct file_operations read_acl_fops = {
    .open    = acl_read_open,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = single_release,
};
#endif // defined(CONFIG_AVM_PA) && defined(CONFIG_AVM_CPMAC_ATH_PPA)

static void reset_switch(avmnet_module_t *this, enum rst_type type);

#define WORK_EVENT_BIT_TRIGGER  (BITS_PER_LONG - 1)
#define WORK_EVENT_BIT_IRQ       0
#define WORK_EVENT_BIT_ACL       1
#define WORK_EVENT_BIT_ACL_STAT  2
#define WORK_EVENT_TRIGGER      (1 << WORK_EVENT_BIT_TRIGGER)
#define WORK_EVENT_IRQ          (1 << WORK_EVENT_BIT_IRQ)
#define WORK_EVENT_ACL          (1 << WORK_EVENT_BIT_ACL)
#define WORK_EVENT_ACL_STAT     (1 << WORK_EVENT_BIT_ACL_STAT)

static int work_thread(void *data)
{
    struct aths17_context *ctx;
    int result;

    ctx = (struct aths17_context *) data;
    AVMNET_DEBUG("[%s] worker thread starting\n", __func__);
    result = 0;
    do{
        AVMNET_DEBUG("[%s] waiting for events\n", __func__);
        if(wait_event_interruptible(ctx->event_wq,
               test_and_clear_bit(WORK_EVENT_BIT_TRIGGER, &ctx->pending_events)))
        {
            AVMNET_ERR("[%s] interrupted while waiting for events, exiting\n", __func__);
            result = -EINTR;
            break;
        }

        /*--- auswerten des Events ---*/
        AVMNET_DEBUG("[%s] processing events 0x%08lx\n", __func__, ctx->pending_events);

        if(test_and_clear_bit(WORK_EVENT_BIT_IRQ, &ctx->pending_events)){
            AVMNET_DEBUG("[%s] link irq triggered\n", __func__);
            ctx->this_module->poll(ctx->this_module);
            enable_irq(ctx->this_module->initdata.swi.irq);
        }
#if defined(CONFIG_AVM_PA) && defined(CONFIG_AVM_CPMAC_ATH_PPA)
        if(test_and_clear_bit(WORK_EVENT_BIT_ACL, &ctx->pending_events)){
            AVMNET_DEBUG("[%s] ACL update triggered\n", __func__);
            bridge_work_acl(ctx);
        }
#if defined(AVM_PA_HARDWARE_PA_HAS_SESSION_STATS)
        if(test_and_clear_bit(WORK_EVENT_BIT_ACL_STAT, &ctx->pending_events)){
            AVMNET_DEBUG("[%s] ACL update triggered\n", __func__);
            work_acl_policer(ctx);
        }
#endif /*--- #if defined(AVM_PA_HARDWARE_PA_HAS_SESSION_STATS) ---*/
#endif
    }while(!kthread_should_stop());

    AVMNET_DEBUG("[%s] event thread dead\n", __func__);

    ctx->kthread      = NULL;

    return result;
}

/*-----------------------------------------------------------------------------------------------*\
\*-----------------------------------------------------------------------------------------------*/
static void work_send_event(struct aths17_context *ctx, unsigned int event)
{
    unsigned int i = 0;

    AVMNET_DEBUG("[%s] Called with event 0x%08x\n", __func__, event);

    event &= ~(WORK_EVENT_TRIGGER);
    if(event == 0){
        return;
    }

    do{
        i = ffs(event);
        set_bit(i - 1, &ctx->pending_events);
        event >>= i;
    }while(event);

    mb();
    set_bit(WORK_EVENT_BIT_TRIGGER, &ctx->pending_events);

    wake_up_interruptible(&ctx->event_wq);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static irqreturn_t ar8337_link_intr(int irq __attribute__((unused)), void *dev) {

    avmnet_module_t *this = (avmnet_module_t *)dev;
    struct aths17_context *ctx = (struct aths17_context *) this->priv;

    AVMNET_DEBUG("[%s] irq %d\n", __func__, this->initdata.swi.irq);
    disable_irq_nosync(this->initdata.swi.irq);

    work_send_event(ctx, WORK_EVENT_IRQ);

    return IRQ_HANDLED;
}


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_s17_lock(avmnet_module_t *this) {
    return this->parent->lock(this->parent);
}


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void avmnet_s17_unlock(avmnet_module_t *this) {
    this->parent->unlock(this->parent);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_s17_trylock(avmnet_module_t *this) {
    return this->parent->trylock(this->parent);
}

static unsigned int rd_reg_unlocked(avmnet_module_t *this, unsigned int phy_addr, unsigned int reg_addr)
{
    udelay(10);
    return this->parent->reg_read(this->parent, phy_addr, reg_addr);
}

static int wr_reg_unlocked(avmnet_module_t *this, unsigned int phy_addr, unsigned int reg_addr, unsigned int write_data)
{
    udelay(10);
    return this->parent->reg_write(this->parent, phy_addr, reg_addr, write_data & 0xffff);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
unsigned int avmnet_athrs37_reg_read(avmnet_module_t *this, unsigned int s37_addr)
{
    unsigned int addr_temp;
    unsigned int s37_rd_csr_low, s37_rd_csr_high, s37_rd_csr;
    unsigned int data;
    unsigned int phy_address, reg_address;
    int result;

    s37_rd_csr = 0;
    addr_temp = s37_addr >> 1;
    data = addr_temp >> 8;

    phy_address = 0x18;
    reg_address = 0x00;

    result = wr_reg_unlocked(this, phy_address, reg_address, data);
    if(result != 0){
        goto err_out;
    }

    phy_address = (((addr_temp >> 5) & 0x07) | 0x10);
    reg_address =   (addr_temp & 0x1f);
    s37_rd_csr_low = (uint32_t) rd_reg_unlocked(this, phy_address, reg_address);

    reg_address++;
    s37_rd_csr_high = (uint32_t) rd_reg_unlocked(this, phy_address, reg_address);
    s37_rd_csr = (s37_rd_csr_high << 16) | s37_rd_csr_low ;

err_out:
    return(s37_rd_csr);
}

unsigned int __maybe_unused avmnet_athrs37_reg_read_locked(avmnet_module_t *this, unsigned int s37_addr)
{
    unsigned int data;
    int result;

    data = 0;
    result = this->lock(this);
    if(result != 0){
        AVMNET_WARN("[%s] Interrupted while waiting for lock\n", __func__);
        goto err_out;
    }

    data = avmnet_athrs37_reg_read(this, s37_addr);

    this->unlock(this);

err_out:
    return(data);
}
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_athrs37_reg_write(avmnet_module_t *this, unsigned int s37_addr, unsigned int s37_write_data)
{
    unsigned int addr_temp;
    unsigned int data;
    unsigned int phy_address, reg_address;
    int result;

    addr_temp = s37_addr >> 1;
    data = addr_temp >> 8;

    phy_address = 0x18;
    reg_address = 0x0;

    result = wr_reg_unlocked(this, phy_address, reg_address, data);
    if(result != 0){
        goto err_out;
    }

    phy_address = (((addr_temp >> 5) & 0x7) | 0x10);
    reg_address = ((addr_temp & 0x1f) | 0x1);

    reg_address--;
    data = s37_write_data  & 0xffff;
    result = wr_reg_unlocked(this, phy_address, reg_address, data);
    if(result != 0){
        goto err_out;
    }

    reg_address++;
    data = (s37_write_data >> 16) & 0xffff;
    result = wr_reg_unlocked(this, phy_address, reg_address, data);

err_out:
    return result;
}

int __maybe_unused avmnet_athrs37_reg_write_locked(avmnet_module_t *this, unsigned int s37_addr, unsigned int s37_write_data)
{
    int result;

    result = this->lock(this);
    if(result != 0){
        AVMNET_WARN("[%s] Interrupted while waiting for lock\n", __func__);
        goto err_out;
    }

    result = avmnet_athrs37_reg_write(this, s37_addr, s37_write_data);

    this->unlock(this);
err_out:
    return result;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
unsigned int avmnet_s17_rd_phy(avmnet_module_t *this, unsigned int phy_addr, unsigned int reg_addr)
{
    /*--- AVMNET_DEBUG(KERN_ERR "[%s] Phy 0x%x reg_addr 0x%x\n", __func__, phy_addr, reg_addr); ---*/

    /*--- wir koennen hier gleich per MDIO auf die einzelnen PHYs zugreifen ---*/

    return rd_reg_unlocked(this, phy_addr, reg_addr);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_s17_wr_phy(avmnet_module_t *this, unsigned int phy_addr, unsigned int reg_addr, unsigned int write_data)
{
    /*--- AVMNET_DEBUG(KERN_ERR "[%s] Phy 0x%x reg_addr 0x%x write_data 0x%x\n", __func__, phy_addr, reg_addr, write_data); ---*/

    /*--- wir koennen hier gleich per MDIO auf die einzelnen PHYs zugreifen ---*/

    return wr_reg_unlocked(this, phy_addr, reg_addr, write_data);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_ar8337_status_poll(avmnet_module_t *this) {
    int              i;
    /*--- uint32_t         IntStatus; ---*/

    AVMNET_DEBUG("[%s] on %s\n", __func__, this->name);
    
    update_rmon_cache(this);

    for (i = 0; i < this->num_children; i++) {
        /*--- AVMNET_DEBUG("[%s] call %s\n", __func__, this->children[i]->name); ---*/
        this->children[i]->poll(this->children[i]);
    }

#if defined(AVM_PA_HARDWARE_PA_HAS_SESSION_STATS)
    {
        struct aths17_context *ctx = (struct aths17_context *) this->priv;
        work_send_event(ctx, WORK_EVENT_ACL_STAT);
    }
#endif
#if 0
    if(this->lock(this)){
        AVMNET_WARN("[%s] Interrupted while waiting for lock!\n", __func__);
        return -EINTR;
    }

    IntStatus = avmnet_athrs37_reg_read(this, S17_GLOBAL_INT1_REG);
    AVMNET_DEBUG("[%s] IntStatus 0x%x\n", __func__, IntStatus);

    avmnet_athrs37_reg_write(this, S17_GLOBAL_INT1_REG, IntStatus); /* clear global link interrupt */

    this->unlock(this);
#endif

    return 0;
}

/*------------------------------------------------------------------------------------------*\
 * das Interface des Switch ist immer aktiv
\*------------------------------------------------------------------------------------------*/
int avmnet_ar8337_HW223_poll(avmnet_module_t *this) {

    int i, result;
    avmnet_linkstatus_t status = { .Status = 0 };

    AVMNET_DEBUG("[%s] on %s\n", __func__, this->name);

    if(this->device_id != NULL){
        status.Details.link = 1;
        status.Details.speed = 2; // GBit
        status.Details.fullduplex = 1;
        status.Details.powerup = 1;
        status.Details.flowcontrol = 3; // RXTX

        AVMNET_DEBUG("{%s} power%s, %slink, Speed %d, %sduplex\n", __FUNCTION__, status.Details.powerup ? "up":"down", status.Details.link ? "":"no", status.Details.speed, status.Details.fullduplex ? "full":"half");

        result = this->parent->set_status(this->parent, this->device_id, status);
        if(result < 0){
            AVMNET_ERR("[%s] setting status for device %s failed.\n", __func__, (this->device_id != NULL) ? this->device_id->device_name : "NULL");
        }
    }

    for (i = 0; i < this->num_children; i++) {
        /*--- AVMNET_DEBUG("[%s] call %s\n", __func__, this->children[i]->name); ---*/
        this->children[i]->poll(this->children[i]);
    }

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_ar8337_set_status(avmnet_module_t *this, avmnet_device_t *device_id, avmnet_linkstatus_t status) {

    AVMNET_DEBUG("[%s] %s Status %s changed actual %s speed %d\n", 
                __func__, device_id->device_name, this->name, status.Details.link ? "on":"off", status.Details.speed);

    AVMNET_DEBUG("PORT_STATUS_REG(0)  :%x\n", avmnet_athrs37_reg_read_locked(this,  S17_PORT_STATUS_REG(0) ));
    AVMNET_DEBUG("PORT_STATUS_REG(1)  :%x\n", avmnet_athrs37_reg_read_locked(this,  S17_PORT_STATUS_REG(1) ));
    AVMNET_DEBUG("PORT_STATUS_REG(2)  :%x\n", avmnet_athrs37_reg_read_locked(this,  S17_PORT_STATUS_REG(2) ));
    AVMNET_DEBUG("PORT_STATUS_REG(3)  :%x\n", avmnet_athrs37_reg_read_locked(this,  S17_PORT_STATUS_REG(3) ));
    AVMNET_DEBUG("PORT_STATUS_REG(4)  :%x\n", avmnet_athrs37_reg_read_locked(this,  S17_PORT_STATUS_REG(4) ));
    AVMNET_DEBUG("PORT_STATUS_REG(5)  :%x\n", avmnet_athrs37_reg_read_locked(this,  S17_PORT_STATUS_REG(5) ));

    this->parent->set_status(this->parent, device_id, status);

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_ar8337_setup_interrupt(avmnet_module_t *this, unsigned int on_off) {

    uint32_t    intr_reg_val;

    AVMNET_DEBUG("{%s} %sable PHY-Interrupt\n", __func__, on_off ? "en":"dis");

    if (this->initdata.swi.flags & AVMNET_CONFIG_FLAG_IRQ) {
        if (this->lock(this)) {
            AVMNET_WARN("[%s] Interrupted while waiting for lock!\n", __func__);
            return -EINTR;
        }

        if (on_off) {
            intr_reg_val = avmnet_athrs37_reg_read(this, S17_GLOBAL_INT1_REG);   /* clear global link interrupt */
            avmnet_athrs37_reg_write(this, S17_GLOBAL_INT1_REG, intr_reg_val);

            /* Enable global PHY link status interrupt */
            avmnet_athrs37_reg_write(this, S17_GLOBAL_INT1_MASK_REG, 0xFE);
        } else {
            /* disable global PHY link status interrupt */
            avmnet_athrs37_reg_write(this, S17_GLOBAL_INT1_MASK_REG, 0);
        }

        /*--- ath_reg_rmw_clear(ATH_MISC_INT_STATUS, BIT(MISC_BIT_ENET_LINK)); ---*/ /*--- also clear INT_STATUS-Register ---*/

        this->unlock(this);
        return 0;
    }

    return 1;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void avmnet_ar8337_status_changed(avmnet_module_t *this, avmnet_module_t *caller __attribute__ ((unused))) {

    avmnet_module_t *parent = this->parent;

    AVMNET_DEBUG("[%s] context_name %s\n", __func__, this->name);

    parent->status_changed(parent, this);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
#if defined(CONFIG_SGMII_INTERFACE)
#define ATHR_SGMII_FORCED
static unsigned int setup_sgmii(void) {

	uint32_t status = 0, count = 0;

    ath_reg_wr ( ATH_GMAC_MR_AN_CONTROL, 
        ATH_GMAC_MR_AN_CONTROL_POWER_DOWN_SET (1) | 
        ATH_GMAC_MR_AN_CONTROL_PHY_RESET_SET  (1) );

#if defined(ATHR_SGMII_FORCED)
    AVMNET_DEBUG("{%s} SGMII forced Mode\n", __func__);
    ath_reg_wr ( ATH_GMAC_MR_AN_CONTROL, 
        ATH_GMAC_MR_AN_CONTROL_SPEED_SEL1_SET  (1) |
        ATH_GMAC_MR_AN_CONTROL_PHY_RESET_SET   (1) |                                        
        ATH_GMAC_MR_AN_CONTROL_DUPLEX_MODE_SET (1) );

    udelay(10);

    ath_reg_wr ( ATH_GMAC_SGMII_CONFIG, 
        ATH_GMAC_SGMII_CONFIG_MODE_CTRL_SET   (2) |
        ATH_GMAC_SGMII_CONFIG_FORCE_SPEED_SET (1) |                               
        ATH_GMAC_SGMII_CONFIG_SPEED_SET       (2) );
#else
    ath_reg_wr ( ATH_GMAC_SGMII_CONFIG, 
        ATH_GMAC_SGMII_CONFIG_MODE_CTRL_SET (2));

    ath_reg_wr ( ATH_GMAC_MR_AN_CONTROL, 
        ATH_GMAC_MR_AN_CONTROL_AN_ENABLE_SET (1) |
        ATH_GMAC_MR_AN_CONTROL_PHY_RESET_SET (1) );
#endif

    /*------------------------------------------------------------------------------------------*\
	 * SGMII reset sequence suggested by systems team.
    \*------------------------------------------------------------------------------------------*/
	ath_reg_wr ( ATH_GMAC_SGMII_RESET, 
        ATH_GMAC_SGMII_RESET_RX_CLK_N_RESET);

    udelay(10);

	ath_reg_wr ( ATH_GMAC_SGMII_RESET, 
        ATH_GMAC_SGMII_RESET_HW_RX_125M_N_SET (1));

    udelay(10);

	ath_reg_wr ( ATH_GMAC_SGMII_RESET, 
        ATH_GMAC_SGMII_RESET_HW_RX_125M_N_SET (1) |
		ATH_GMAC_SGMII_RESET_RX_125M_N_SET    (1) );

    udelay(10);

	ath_reg_wr ( ATH_GMAC_SGMII_RESET, 
        ATH_GMAC_SGMII_RESET_HW_RX_125M_N_SET (1) |
		ATH_GMAC_SGMII_RESET_TX_125M_N_SET    (1) |
		ATH_GMAC_SGMII_RESET_RX_125M_N_SET    (1) );

    udelay(10);

	ath_reg_wr ( ATH_GMAC_SGMII_RESET, 
        ATH_GMAC_SGMII_RESET_HW_RX_125M_N_SET (1) |
		ATH_GMAC_SGMII_RESET_TX_125M_N_SET    (1) |
		ATH_GMAC_SGMII_RESET_RX_125M_N_SET    (1) |
	    ATH_GMAC_SGMII_RESET_RX_CLK_N_SET     (1) );

    udelay(10);

	ath_reg_wr ( ATH_GMAC_SGMII_RESET, 
        ATH_GMAC_SGMII_RESET_HW_RX_125M_N_SET (1) |
		ATH_GMAC_SGMII_RESET_TX_125M_N_SET    (1) |
		ATH_GMAC_SGMII_RESET_RX_125M_N_SET    (1) |
		ATH_GMAC_SGMII_RESET_RX_CLK_N_SET     (1) |
		ATH_GMAC_SGMII_RESET_TX_CLK_N_SET     (1) );

    mdelay(100);

    /*------------------------------------------------------------------------------------------*\
     * WAR::Across resets SGMII link status goes to weird
	 * state.
	 * if 0xb8070058 (SGMII_DEBUG register) reads other then 0x1f or 0x10
	 * for sure we are in bad  state.
	 * Issue a PHY reset in ATH_GMAC_MR_AN_CONTROL to keep going.
    \*------------------------------------------------------------------------------------------*/
    do {
		ath_reg_rmw_set ( ATH_GMAC_MR_AN_CONTROL, 
            ATH_GMAC_MR_AN_CONTROL_PHY_RESET_SET (1));

		mdelay(200);

		ath_reg_rmw_clear ( ATH_GMAC_MR_AN_CONTROL, 
            ATH_GMAC_MR_AN_CONTROL_PHY_RESET_SET (1));

        mdelay(1000);

		if (count++ == SGMII_LINK_WAR_MAX_TRY) {
			AVMNET_ERR("[%s] Max resets limit reached exiting...\n", __func__);

			return SGMII_LINK_WAR_MAX_TRY;
		}

		status = (ath_reg_rd (ATH_GMAC_SGMII_DEBUG) & 0xff);
    } while (!(status == 0xf || status == 0x10));

	AVMNET_INFO("[%s] Done\n", __func__);

    return 0;
}
#endif /*--- #if defined(CONFIG_SGMII_INTERFACE) ---*/

/*------------------------------------------------------------------------------------------*\
 * bei Honeybee Powerline 1240 ist die CPU an Port2 des AR8334 angeschossen
 * am RGMII-Interface hängt der Powerlinecontroller
\*------------------------------------------------------------------------------------------*/
#if defined(CONFIG_MACH_QCA953x) || defined(CONFIG_SOC_QCA953X)
int avmnet_ar8334_hbee_setup(avmnet_module_t *this) {

    int i, value; 
    unsigned int port;
    /*--- struct aths17_context *ctx = (struct aths17_context *) this->priv; ---*/

    AVMNET_DEBUG("[%s] %s\n", __func__, this->name);

    if (this->lock(this)) {
        AVMNET_WARN("[%s] Interrupted while waiting for lock!\n", __func__);
        return -EINTR;
    }

    if (this->initdata.swi.flags & AVMNET_CONFIG_FLAG_RESET) {
        AVMNET_DEBUG("[%s] reset Switch gpio %d\n", __func__, this->initdata.swi.reset);
        reset_switch(this, rst_pulse);
    } else {
        value = avmnet_athrs37_reg_read(this, S17_MASK_CTL_REG);
        AVMNET_DEBUG("{%s} s37 resetting switch... 0x%x ", __func__, value);
        avmnet_athrs37_reg_write(this, S17_MASK_CTL_REG, value | S17_MASK_CTRL_RESET);
    }

    i = 0x10;
    while(i--) {
        mdelay(100);
        if( ! (avmnet_athrs37_reg_read(this, S17_MASK_CTL_REG) & S17_MASK_CTRL_RESET))
            break;
    }

    if (i == 0) {
        AVMNET_ERR("[%s] Giving up on hanging Switch %s\n", __func__, this->name);
        return -EFAULT;
    }

    AVMNET_DEBUG("done i %d ControlRegister 0x%x\n", i, avmnet_athrs37_reg_read(this, S17_MASK_CTL_REG));

    /*--- TODO config Interfacetype ---*/
    avmnet_athrs37_reg_write(this, S17_P0_PADCTRL_REG0, S17_MAC0_RGMII_EN);     /* Set RGMII MAC-mode */
    value = avmnet_athrs37_reg_read(this, S17_PWS_REG);
    avmnet_athrs37_reg_write(this, S17_PWS_REG, value | (1<<28) | (1<<7));   /*--- RGMII interface enabled ---*/

    if(this->initdata.swi.flags & AVMNET_CONFIG_FLAG_RX_DELAY){
        AVMNET_DEBUG("{%s} set AVMNET_CONFIG_FLAG_RX_DELAY 0x%x\n", __func__, this->initdata.swi.rx_delay);
        value = avmnet_athrs37_reg_read(this, S17_P0_PADCTRL_REG0);
        value &= ~S17_MAC0_RGMII_RXCLK_MASK;
        value |=  (this->initdata.swi.rx_delay << S17_MAC0_RGMII_RXCLK_SHIFT);
        avmnet_athrs37_reg_write(this, S17_P0_PADCTRL_REG0, value);
        /*--- ACHTUNG: S17_MAC0_RGMII_RXCLK_DELAY befindet sich im S17_P5_PADCTRL_REG1 ---*/
        avmnet_athrs37_reg_write(this, S17_P5_PADCTRL_REG1, S17_MAC0_RGMII_RXCLK_DELAY );
    }
    if(this->initdata.swi.flags & AVMNET_CONFIG_FLAG_TX_DELAY){
        AVMNET_DEBUG("{%s} set AVMNET_CONFIG_FLAG_TX_DELAY 0x%x\n", __func__, this->initdata.swi.tx_delay);
        value = avmnet_athrs37_reg_read(this, S17_P0_PADCTRL_REG0);
        value &= ~S17_MAC0_RGMII_TXCLK_MASK;
        value |= S17_MAC0_RGMII_TXCLK_DELAY | (this->initdata.swi.tx_delay << S17_MAC0_RGMII_TXCLK_SHIFT);
        avmnet_athrs37_reg_write(this, S17_P0_PADCTRL_REG0, value);
    }

    this->unlock(this);

    this->setup_irq(this, 0);   /*--- irq im Switch ausschalten ---*/

    for (i=0; i< this->num_children; i++) {
        this->children[i]->setup(this->children[i]);    /*--- die internen PHY initialisieren ---*/
    }

    if (this->lock(this)) {
        goto lock_failed;
    }

    value = avmnet_athrs37_reg_read(this, S17_GLOBAL_INT0_REG);   /* clear Interrupts */
    avmnet_athrs37_reg_write(this, S17_GLOBAL_INT0_REG, value);
    value = avmnet_athrs37_reg_read(this, S17_GLOBAL_INT1_REG);   /* clear Interrupts */
    avmnet_athrs37_reg_write(this, S17_GLOBAL_INT1_REG, value);

    AVMNET_DEBUG("{%s} S17_GLOBAL_INTR_REG 0x%x\n", __func__, value);
    AVMNET_DEBUG("{%s} S17_GLOBAL_INTR_REG 0x%x\n", __func__, avmnet_athrs37_reg_read(this, S17_GLOBAL_INT0_REG));

    /*--- clear all VLAN-Entries, disable EEE ---*/
    avmnet_athrs37_reg_write(this, S17_VTU_FUNC1_REG, S17_VTU_FUNC1_VTBUSY | S17_VTU_FUNC1_VT_FUNC_FLUSH);
    /*--- wait for VT_DONE, signals end of Tableoperation ---*/
    do {
        value = avmnet_athrs37_reg_read(this, S17_GLOBAL_INT0_REG );   /*--- clear VT_DONE Irq ---*/ 
    } while ( ! (value & S17_INT0_VTDONE));
    avmnet_athrs37_reg_write(this, S17_VTU_FUNC1_REG, 0);   /*--- clear VT_BUSY ---*/

    avmnet_athrs37_reg_write(this, S17_EEE_CTRL_REG, S17_LPI_DISABLE_ALL);
    avmnet_athrs37_reg_write(this, S17_HDRCTRL_REG, 0);

    value = avmnet_athrs37_reg_read(this, S17_PWR_SEL_REG);
    avmnet_athrs37_reg_write(this, S17_PWR_SEL_REG, value | S17_PWR_SEL_RGMII0_1_8V);

    /*--- value = avmnet_athrs37_reg_read(this, S17_QM_CTRL_REG); ---*/
    /*--- avmnet_athrs37_reg_write(this, S17_QM_CTRL_REG, value | 1 << 6); ---*/    /*--- no packetdrop due flowcontrol ---*/


#if 0       /*--- not connected ---*/
    if (this->initdata.swi.flags & AVMNET_CONFIG_FLAG_IRQ) {
        result = request_irq(ctx->irq, ar8337_link_intr, IRQF_DISABLED | IRQF_TRIGGER_LOW, "ar8337 link", this);
        if (result < 0) {
            AVMNET_ERR("[%s] request irq ar8337 link %d failed\n", __func__, ctx->irq);
            up(&(ctx->mdio_lock));
            return -EINTR;
        }
    }
#endif

    /*--- set packet routing on CPU port ---*/
    /*--- ACHTUNG CPU_PORT_EN muss gesetzt sein, sonst werden Pakete vom Port2 nach Port3 gefiltert ---*/
    value = avmnet_athrs37_reg_read(this, S17_GLOFW_CTRL0_REG);
    value &= ~((3 << 24) | (3 << 22));
    avmnet_athrs37_reg_write(this, S17_GLOFW_CTRL0_REG, value | (2 << 26) | S17_CPU_PORT_EN);
    avmnet_athrs37_reg_write(this, S17_GLOFW_CTRL1_REG, S17_IGMP_JOIN_LEAVE_DPALL | 
                                                        S17_BROAD_DPALL | 
                                                        S17_MULTI_FLOOD_DPALL | 
                                                        S17_UNI_FLOOD_DPALL);

    /*--- enable Atheros-Header 2Bytes ---*/
    avmnet_athrs37_reg_write(this, S17_HDRCTRL_REG, 0);

    /*--- configure Port 0 incl. Flowcontrol ---*/
    avmnet_athrs37_reg_write(this, S17_PORT_STATUS_REG(0), S17_PS_PORT_STATUS_DEFAULT);

    /*--- Port 2 connects to CPU ---*/
    avmnet_athrs37_reg_write(this, S17_PORT_HDRCTRL_REG(2), S17_TXHDR_MODE_ALL | S17_RXHDR_MODE_ALL);
    avmnet_athrs37_reg_write(this, S17_Px_HOL_CTRL0(2), S17_HOL_CTRL0_WAN);
    /*--- avmnet_athrs37_reg_write(this, S17_Px_HOL_CTRL1(2), S17_HOL_CTRL1_DEFAULT); ---*/
    avmnet_athrs37_reg_write(this, S17_Px_HOL_CTRL1(2),  0xCF);
    avmnet_athrs37_reg_write(this, S17_PxLOOKUP_CTRL_REG(2), S17_LOOKUP_CTRL_PORT_VLAN_EN |
                                                             S17_LOOKUP_CTRL_PORTSTATE_FORWARD |
                                                             S17_LOOKUP_CTRL_VLAN_MODE_DISABLE |
                                                             0x7b);

    /* enable external ports for which a config entry exists */
    AVMNET_DEBUG("{%s} avmnet_hw_config_entry->nr_avm_devices %d\n", __func__, avmnet_hw_config_entry->nr_avm_devices);
    for(i = 0; i < avmnet_hw_config_entry->nr_avm_devices; ++i) {
        port = avmnet_hw_config_entry->avm_devices[i]->vlanID;

        avmnet_athrs37_reg_write(this, S17_Px_HOL_CTRL0(port), S17_HOL_CTRL0_LAN);
        avmnet_athrs37_reg_write(this, S17_Px_HOL_CTRL1(port), S17_HOL_CTRL1_DEFAULT);

        if(port <= AR8337_MAX_PHYS) {
            AVMNET_ERR("{%s} set port %d 0x%X\n", __func__, port, avmnet_athrs37_reg_read(this, S17_PORT_STATUS_REG(port)));
            avmnet_athrs37_reg_write(this, S17_PORT_HDRCTRL_REG(port), 0); /*--- Atheros-Header on Ports ---*/
            avmnet_athrs37_reg_write(this, S17_PxLOOKUP_CTRL_REG(port), S17_LOOKUP_CTRL_PORT_VLAN_EN |
                                                                        S17_LOOKUP_CTRL_PORTSTATE_FORWARD |
                                                                        S17_LOOKUP_CTRL_VLAN_MODE_DISABLE |
                                                                        1 << 2);
        }
    }

    /* clear MAC table */
    avmnet_athrs37_reg_write(this, S17_ATU_FUNC_REG, S17_ATU_FUNC_BUSY | S17_ATU_FUNC_FLUSH_ALL);
    do {
        value = avmnet_athrs37_reg_read(this, S17_GLOBAL_INT0_REG );   /*--- clear ARL_DONE Irq ---*/ 
    } while ( ! (value & S17_INT0_ARLDONE));
    avmnet_athrs37_reg_write(this, S17_ATU_FUNC_REG, 0);   /*--- clear ARL_BUSY ---*/

    /* Enable MIB counters */
    value = avmnet_athrs37_reg_read(this, S17_MODULE_EN_REG);
    avmnet_athrs37_reg_write(this, S17_MODULE_EN_REG, value | S17_MODULE_MIB_EN | S17_MODULE_ACL_EN);

    /*--- AVMNET_DEBUG("S17_CPU_PORT_REGISTER  :%x\n", avmnet_athrs37_reg_read(this,  S17_CPU_PORT_REG )); ---*/
    for (i = 0; i < 6; i++) 
        AVMNET_DEBUG("PORT_STATUS_REG(%d)  : 0x%x\n", i, avmnet_athrs37_reg_read(this, S17_PORT_STATUS_REG(i) ));

    this->unlock(this);
    return 0;
 
lock_failed:

    if (this->initdata.swi.flags & AVMNET_CONFIG_FLAG_IRQ) {
        free_irq(this->initdata.swi.irq, ar8337_link_intr);
    }

    for (i=0; i< this->num_children; i++) {
         this->children[i]->exit(this->children[i]);
    }

    return -EINTR;
}
#endif

/*------------------------------------------------------------------------------------------*\
 * 7490 Fiber - die Ports im Switch werden hier per VLAN getrennt
\*------------------------------------------------------------------------------------------*/
#if defined(INFINEON_CPU) || defined(CONFIG_AVMNET_VLAN_MASTER)
int avmnet_ar8334_vlan_setup(avmnet_module_t *this) {
    int    i, value, result;
    unsigned int port;

    AVMNET_ERR("[%s]\n", __func__);

    if (this->lock(this)) {
        AVMNET_WARN("[%s] Interrupted while waiting for lock!\n", __func__);
        return -EINTR;
    }

    if (this->initdata.swi.flags & AVMNET_CONFIG_FLAG_RESET) {
        AVMNET_DEBUG("[%s] reset Switch gpio %d\n", __func__, this->initdata.swi.reset);
        reset_switch(this, rst_pulse);
    } else {
        value = avmnet_athrs37_reg_read(this, S17_MASK_CTL_REG);
        AVMNET_DEBUG("{%s} s37 resetting switch... 0x%x ", __func__, value);
        avmnet_athrs37_reg_write(this, S17_MASK_CTL_REG, value | S17_MASK_CTRL_RESET);
    }

    i = 0x10;
    while(i--) {
        mdelay(100);
        if( ! (avmnet_athrs37_reg_read(this, S17_MASK_CTL_REG) & S17_MASK_CTRL_RESET))
            break;
    }
    if (i == 0) {
        AVMNET_ERR("[%s] Giving up on hanging Switch %s\n", __func__, this->name);
        return -EFAULT;
    }

    AVMNET_INFO("done i %d ControlRegister 0x%x\n", i, avmnet_athrs37_reg_read(this, S17_MASK_CTL_REG));
    
    this->unlock(this);

    this->setup_irq(this, 0);   /*--- irq im Switch ausschalten ---*/

    for (i=0; i< this->num_children; i++) {
        this->children[i]->setup(this->children[i]);    /*--- die internen PHY initialisieren ---*/
    }

    if (this->lock(this)) {
        goto lock_failed;
    }

    /*--- TODO config Interfacetype ---*/
    avmnet_athrs37_reg_write(this, S17_P0_PADCTRL_REG0, S17_MAC0_RGMII_EN);     /* Set RGMII MAC-mode */
    value = avmnet_athrs37_reg_read(this, S17_PWS_REG);
    avmnet_athrs37_reg_write(this, S17_PWS_REG, value | (1<<28) | (1<<7));   /*--- RGMII interface enabled ---*/

    if(this->initdata.swi.flags & AVMNET_CONFIG_FLAG_RX_DELAY){
        AVMNET_INFO("{%s} set AVMNET_CONFIG_FLAG_RX_DELAY 0x%x\n", __func__, this->initdata.swi.rx_delay);
        value = avmnet_athrs37_reg_read(this, S17_P0_PADCTRL_REG0);
        value &= ~S17_MAC0_RGMII_RXCLK_MASK;
        value |=  (this->initdata.swi.rx_delay << S17_MAC0_RGMII_RXCLK_SHIFT);
        avmnet_athrs37_reg_write(this, S17_P0_PADCTRL_REG0, value);
        /*--- ACHTUNG: S17_MAC0_RGMII_RXCLK_DELAY befindet sich im S17_P5_PADCTRL_REG1 ---*/
        avmnet_athrs37_reg_write(this, S17_P5_PADCTRL_REG1, S17_MAC0_RGMII_RXCLK_DELAY );
    }
    if(this->initdata.swi.flags & AVMNET_CONFIG_FLAG_TX_DELAY){
        AVMNET_INFO("{%s} set AVMNET_CONFIG_FLAG_TX_DELAY 0x%x\n", __func__, this->initdata.swi.tx_delay);
        value = avmnet_athrs37_reg_read(this, S17_P0_PADCTRL_REG0);
        value &= ~S17_MAC0_RGMII_TXCLK_MASK;
        value |= S17_MAC0_RGMII_TXCLK_DELAY | (this->initdata.swi.tx_delay << S17_MAC0_RGMII_TXCLK_SHIFT);
        avmnet_athrs37_reg_write(this, S17_P0_PADCTRL_REG0, value);
    }

    value = avmnet_athrs37_reg_read(this, S17_PWR_SEL_REG);
    avmnet_athrs37_reg_write(this, S17_PWR_SEL_REG, value | S17_PWR_SEL_RGMII1_1_8V);

    value = avmnet_athrs37_reg_read(this, S17_GLOBAL_INT0_REG);   /* clear Interrupts */
    avmnet_athrs37_reg_write(this, S17_GLOBAL_INT0_REG, value);
    value = avmnet_athrs37_reg_read(this, S17_GLOBAL_INT1_REG);   /* clear Interrupts */
    avmnet_athrs37_reg_write(this, S17_GLOBAL_INT1_REG, value);

    AVMNET_DEBUG("{%s} S17_GLOBAL_INTR_REG 0x%x\n", __func__, value);
    AVMNET_DEBUG("{%s} S17_GLOBAL_INTR_REG 0x%x\n", __func__, avmnet_athrs37_reg_read(this, S17_GLOBAL_INT0_REG));

    if (this->initdata.swi.flags & AVMNET_CONFIG_FLAG_IRQ) {
        result = request_irq(this->initdata.swi.irq, ar8337_link_intr, IRQF_DISABLED | IRQF_TRIGGER_LOW, "ar8334 link", this);
        if (result < 0) {
            AVMNET_ERR("[%s] request irq ar8334 link %d failed\n", __func__, this->initdata.swi.irq);
            this->unlock(this);
            return -EINTR;
        }
    }

    /*--- clear all VLAN-Entries, disable EEE ---*/
    avmnet_athrs37_reg_write(this, S17_VTU_FUNC1_REG, S17_VTU_FUNC1_VTBUSY | S17_VTU_FUNC1_VT_FUNC_FLUSH);
    avmnet_athrs37_reg_write(this, S17_EEE_CTRL_REG, S17_LPI_DISABLE_ALL);
    avmnet_athrs37_reg_write(this, S17_HDRCTRL_REG, 0);

    value = avmnet_athrs37_reg_read(this, S17_QM_CTRL_REG);
    avmnet_athrs37_reg_write(this, S17_QM_CTRL_REG, value | S17_QM_CTRL_FLOW_DROP_EN);    /*--- packet drop on congestion ---*/

#if defined(CONFIG_AVMNET_VLAN_MASTER_STAG_8021Q)
    /*--- enable SVID-Mode - with 0x8100 TAGS (for interop with VRX-PPA) ---*/
    avmnet_athrs37_reg_write(this, S17_SERVICE_REG, ETH_P_8021Q | S17_SERVICE_STAG_MODE);    /*--- enable SVID-Mode ---*/
#else
#if ! defined(ETH_P_8021AD)
#define ETH_P_8021AD	0x88A8          /* 802.1ad Service VLAN		*/
#endif
    avmnet_athrs37_reg_write(this, S17_SERVICE_REG, ETH_P_8021AD | S17_SERVICE_STAG_MODE);    /*--- enable SVID-Mode ---*/
#endif

    /*------------------------------------------------------------------------------------------*\
     * config Port0 - CPU-Port
    \*------------------------------------------------------------------------------------------*/
    avmnet_athrs37_reg_write(this, S17_PORT_STATUS_REG(0), S17_PORT0_STATUS_DEFAULT); /*--- Disable flow control on CPU port ---*/
    AVMNET_DEBUG("S17_PORT_STATUS_REG 0x%x\n", avmnet_athrs37_reg_read(this, S17_PORT_STATUS_REG(0)));

    /*--- value = avmnet_athrs37_reg_read(this, S17_GLOFW_CTRL0_REG); ---*/ /*--- enable CPU-PORT on Port0 ---*/
    avmnet_athrs37_reg_write(this, S17_GLOFW_CTRL0_REG, value | S17_CPU_PORT_EN | (2 << 24) | (2 << 22));
    avmnet_athrs37_reg_write(this, S17_GLOFW_CTRL1_REG, S17_IGMP_JOIN_LEAVE_DPALL | 
                                                        S17_BROAD_DPALL | 
                                                        S17_MULTI_FLOOD_DPALL | 
                                                        S17_UNI_FLOOD_DPALL);

    /*--- disable Atheros-Header 2Bytes ---*/
    avmnet_athrs37_reg_write(this, S17_PORT_HDRCTRL_REG(0), S17_TXHDR_MODE_NO | S17_RXHDR_MODE_NO);

    avmnet_athrs37_reg_write(this, S17_Px_HOL_CTRL0(0), S17_HOL_CTRL0_WAN);
    avmnet_athrs37_reg_write(this, S17_Px_HOL_CTRL1(0), S17_HOL_CTRL1_DEFAULT);

    /*--- enable VLAN Port0---*/
    avmnet_athrs37_reg_write(this, S17_PxLOOKUP_CTRL_REG(0), S17_LOOKUP_CTRL_LEARN_EN_0 |
                                                             S17_LOOKUP_CTRL_PORTSTATE_FORWARD |
                                                             S17_LOOKUP_CTRL_VLAN_MODE_CHECK |
                                                             0x3E);

    /*--- CVID ---*/
    /*--- avmnet_athrs37_reg_write(this, S17_PxVLAN_CTRL0_REG(0), 1<<16); ---*/ /*--- set default CVID ---*/
    /*--- avmnet_athrs37_reg_write(this, S17_PxVLAN_CTRL1_REG(0), S17_EG_VLAN_MODE_WITH | S17_PxVLAN_PROP_EN_2); ---*/
    /*--- SVID ---*/
    avmnet_athrs37_reg_write(this, S17_PxVLAN_CTRL0_REG(0),  1); /*--- set default SVID ---*/
    avmnet_athrs37_reg_write(this, S17_PxVLAN_CTRL1_REG(0), S17_EG_VLAN_MODE_WITH | S17_PxVLAN_CORE_PORT | S17_PxVLAN_PROP_EN);

    /* enable external ports for which a config entry exists */
    for(i = 0; i < avmnet_hw_config_entry->nr_avm_devices; ++i) {
        port = avmnet_hw_config_entry->avm_devices[i]->vlanID;

        // port numbers 1 to AR8337_MAX_PHYS are for SW MACs
        if((port > 0) && (port <= AR8337_MAX_PHYS)) {

            AVMNET_DEBUG("{%s} set port %d 0x%x 0x%X\n", __func__, port, S17_PS_PORT_STATUS_DEFAULT, avmnet_athrs37_reg_read(this, S17_PORT_STATUS_REG(port)));
            avmnet_athrs37_reg_write(this, S17_PORT_HDRCTRL_REG(port), S17_TXHDR_MODE_NO | S17_RXHDR_MODE_NO);

            avmnet_athrs37_reg_write(this, S17_Px_HOL_CTRL0(port), S17_HOL_CTRL0_LAN);
            avmnet_athrs37_reg_write(this, S17_Px_HOL_CTRL1(port), S17_HOL_CTRL1_DEFAULT);

            avmnet_athrs37_reg_write(this, S17_PxLOOKUP_CTRL_REG(port), S17_LOOKUP_CTRL_LEARN_EN_0 |
                                                                        S17_LOOKUP_CTRL_PORTSTATE_FORWARD |
                                                                        S17_LOOKUP_CTRL_VLAN_MODE_CHECK |
                                                                        (~(1<<port) & 0x3F));

            /*--- avmnet_athrs37_reg_write(this, S17_PxVLAN_CTRL0_REG(port), port << 16); ---*/ /*--- set default CVID ---*/ 
            /*--- avmnet_athrs37_reg_write(this, S17_PxVLAN_CTRL1_REG(port), S17_EG_VLAN_MODE_WITHOUT | S17_PxVLAN_PROP_EN_2); ---*/

            avmnet_athrs37_reg_write(this, S17_PxVLAN_CTRL0_REG(port), port); /*--- set default SVID ---*/ 
            avmnet_athrs37_reg_write(this, S17_PxVLAN_CTRL1_REG(port), S17_PxVLAN_PORT_TLS_MODE);

            /*--- set VLAN-VID & program VLAN-Table ---*/
            avmnet_athrs37_reg_write(this, S17_GLOBAL_INT0_REG, S17_INT0_VTDONE);   /*--- clear VT_DONE Irq ---*/ 
            
            AVMNET_DEBUG("{%s} GLOBAL_INT0 0x%x\n", __func__, avmnet_athrs37_reg_read(this, S17_GLOBAL_INT0_REG ));
            AVMNET_DEBUG("{%s} vlanCFG 0x%x\n", __func__, avmnet_hw_config_entry->avm_devices[i]->vlanCFG);
            avmnet_athrs37_reg_write(this, S17_VTU_FUNC0_REG, avmnet_hw_config_entry->avm_devices[i]->vlanCFG);

            value = S17_VTU_FUNC1_VT_FUNC_LOAD | S17_VTU_FUNC1_VID(port);
            avmnet_athrs37_reg_write(this, S17_VTU_FUNC1_REG, value);
            avmnet_athrs37_reg_write(this, S17_VTU_FUNC1_REG, S17_VTU_FUNC1_VTBUSY | value);
            /*--- wait for VT_DONE, signals end of Tableoperation ---*/
            do {
                value = avmnet_athrs37_reg_read(this, S17_GLOBAL_INT0_REG );   /*--- clear VT_DONE Irq ---*/ 
            } while ( ! (value & S17_INT0_VTDONE));
            avmnet_athrs37_reg_write(this, S17_VTU_FUNC1_REG, 0);   /*--- clear VT_BUSY ---*/
        }
    }

    /* clear MAC table */
    avmnet_athrs37_reg_write(this, S17_ATU_FUNC_REG, S17_ATU_FUNC_BUSY | S17_ATU_FUNC_FLUSH_ALL);

    /* Enable MIB counters */
    value = avmnet_athrs37_reg_read(this, S17_MODULE_EN_REG);
    avmnet_athrs37_reg_write(this, S17_MODULE_EN_REG, value | S17_MODULE_MIB_EN | S17_MODULE_ACL_EN);

#if AVMNET_DEBUG_LEVEL & AVMNET_DEBUG_LEVEL_DEBUG
    for (i = 0; i < 6; i++) 
        AVMNET_ERR("PORT_STATUS_REG(%d)  : 0x%x\n", i, avmnet_athrs37_reg_read(this, S17_PORT_STATUS_REG(i) ));
#endif

    this->unlock(this);
    /*--- this->setup_irq(this, 1); ---*/   /*--- hier den IRQ immer einschalten ---*/
    return 0;
 
lock_failed:

    if (this->initdata.swi.flags & AVMNET_CONFIG_FLAG_IRQ) {
        free_irq(this->initdata.swi.irq, ar8337_link_intr);
    }

    for (i=0; i< this->num_children; i++) {
         this->children[i]->exit(this->children[i]);
    }

    return -EINTR;
}
#endif /*--- #if defined(INFINEON_CPU) ---*/


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static int avmnet_proc_switch_write(struct file *file __attribute__ ((unused)), const char __user *buf, 
                                    size_t count, 
                                    loff_t *offset __attribute__((unused))) {

    static char cmd[256];
    char *token[3];
    unsigned int number_of_tokens = 0;
    int len;
    unsigned int reg, value;
    avmnet_module_t *this = (avmnet_module_t *) file->private_data;

    len = (sizeof(cmd) - 1) < count ? (sizeof(cmd) - 1) : count;
    len = len - copy_from_user(cmd, buf, len);
    cmd[len] = 0;

    number_of_tokens = avmnet_tokenize(cmd, " ", 3, token);

    if (this->lock(this)) {
        AVMNET_WARN("[%s] Interrupted while waiting for lock!\n", __func__);
        return -EINTR;
    }

    switch (number_of_tokens) {
        case 2:
            if ( ! strncasecmp(token[0], "read", 4)) {
                sscanf(token[1], "%x", &reg);
                value = avmnet_athrs37_reg_read(this, reg);
                AVMNET_SUPPORT("register_read reg %#x = %#x\n", reg, value);
            }
            break;
        case 3:
            if ( ! strncasecmp(token[0], "write", 5)) {
                sscanf(token[1], "%x", &reg);
                sscanf(token[2], "%x", &value);
                avmnet_athrs37_reg_write(this, reg, value);
                AVMNET_SUPPORT("register_write reg %#x = %#x\n", reg, value);
            }
            break;
    }

    this->unlock(this);
    return count;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static int avmnet_proc_switch_open(struct inode *inode, struct file *file)
{
	file->private_data = PDE_DATA(inode);
	return 0;
}

static const struct file_operations avmnet_proc_switch_fops = {
	.open = avmnet_proc_switch_open,
	.write = avmnet_proc_switch_write,
};


#if defined(CONFIG_AVM_PA) && defined(CONFIG_AVM_CPMAC_ATH_PPA)
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static DECLARE_BITMAP(acl_cnt_bmp, AVMNET_AR8337_MAX_ACL_COUNTER);
static void avmnet_ar8337_init_pa(struct aths17_context *ctx)
{
    unsigned int i;
    struct acl_rule *rule;


    INIT_LIST_HEAD(&ctx->list_free);
    INIT_LIST_HEAD(&ctx->list_process);
    INIT_LIST_HEAD(&ctx->list_active);
    spin_lock_init(&ctx->lock_free);
    spin_lock_init(&ctx->lock_process);
    spin_lock_init(&ctx->lock_active);

	bitmap_zero(acl_cnt_bmp, AVMNET_AR8337_MAX_ACL_COUNTER);

    for(i = 0; i < AVMNET_AR8337_ACL_RULES; ++i){
        rule = kzalloc(sizeof(*rule), GFP_KERNEL);
        if(rule == NULL){
            continue;
        }

        rule->hw_index = i;

        list_add_tail(&rule->list, &ctx->list_free);
    }

    avm_pa_register_hardware_pa(&ar8337_hw_pa);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static void acl_busy_wait(struct aths17_context *ctx)
{
    uint32_t reg;

    reg = 0;
    do{
        if(reg & S17_ACL_FUNC0_REG_ACL_BUSY){
            schedule();
        }
        reg = avmnet_athrs37_reg_read(ctx->this_module, S17_ACL_FUNC0_REG);
    }
    while(reg & S17_ACL_FUNC0_REG_ACL_BUSY);
}

/*--------------------------------------------------------------------------------*\
\*--------------------------------------------------------------------------------*/
static inline void read_acl_data_regs(struct aths17_context *ctx,
                                      unsigned int *acl_data_regs,
                                      unsigned int rule_nr,
                                      unsigned int rule_select) {
    unsigned int i;

    acl_busy_wait(ctx);
    avmnet_athrs37_reg_write(ctx->this_module,
                                      S17_ACL_FUNC0_REG,
                                      (S17_ACL_FUNC0_REG_ACL_FUNC_INDEX(rule_nr)
                                       | S17_ACL_FUNC0_REG_ACL_RULE_SELECT(rule_select)
                                       | S17_ACL_FUNC0_REG_ACL_FUNC(S17_ACL_FUNC0_REG_ACL_FUNC_READ)
                                       | S17_ACL_FUNC0_REG_ACL_BUSY));

    acl_busy_wait(ctx);

    for(i = 0; i < 5; i++) {
        acl_data_regs[i] = avmnet_athrs37_reg_read(ctx->this_module,
                                                            S17_ACL_FUNC1_REG + 4 * i);
    }
}

/*--------------------------------------------------------------------------------*\
\*--------------------------------------------------------------------------------*/
static inline void write_acl_data_regs(struct aths17_context *ctx,
                                       unsigned int *acl_data_regs,
                                       unsigned int rule_nr,
                                       unsigned int rule_select)
{
	unsigned int i;

	acl_busy_wait(ctx);

	for(i = 0; i < 5; i++) {
		avmnet_athrs37_reg_write(ctx->this_module,
		                                  S17_ACL_FUNC1_REG + 4 * i,
		                                  acl_data_regs[i]);
#if defined(ACL_DEBUG)
		{
		    uint32_t data;
		    data = avmnet_athrs37_reg_read(ctx->this_module, S17_ACL_FUNC1_REG + 4 * i);
		    if(acl_data_regs[i] != data){
		        AVMNET_ERR("[%s] written: 0x%08x read: 0x%08x %s\n", __func__, acl_data_regs[i], data, acl_data_regs[i] != data ? "*" :"");
		    }
		}
#endif
	}
	avmnet_athrs37_reg_write(ctx->this_module,
	                                  S17_ACL_FUNC0_REG,
	                                  (S17_ACL_FUNC0_REG_ACL_FUNC_INDEX(rule_nr)
	                                   | S17_ACL_FUNC0_REG_ACL_RULE_SELECT(rule_select)
	                                   | S17_ACL_FUNC0_REG_ACL_FUNC(S17_ACL_FUNC0_REG_ACL_FUNC_WRITE)
	                                   | S17_ACL_FUNC0_REG_ACL_BUSY));

	acl_busy_wait(ctx);
}

#define X XARGS_DECODE
static inline void acl_pattern_mac_decode(struct acl_pattern_mac *dst,
                                          uint32_t *src) {
    memset(dst, 0, sizeof(*dst));
    ACL_MAC_PATTERN_XARGS
}
static inline void acl_mask_mac_decode(struct acl_mask_mac *dst,
                                       uint32_t *src) {
    memset(dst, 0, sizeof(*dst));
    ACL_MAC_MASK_XARGS
}
static inline void acl_pattern_ipv4_decode(struct acl_pattern_ipv4 *dst,
                                           uint32_t *src) {
    memset(dst, 0, sizeof(*dst));
    ACL_IPV4_PATTERN_XARGS
}
static inline void acl_mask_ipv4_decode(struct acl_mask_ipv4 *dst,
                                        uint32_t *src) {
    memset(dst, 0, sizeof(*dst));
    ACL_IPV4_MASK_XARGS
}
static inline void acl_action_decode(struct acl_rule *rule, uint32_t *src) {
    struct acl_action *dst = &rule->action;
    memset(dst, 0, sizeof(*dst));
    ACL_ACTION_XARGS
}
static inline void acl_mask_common_decode(struct acl_mask_common *dst,
                                          uint32_t *src) {
    memset(dst, 0, sizeof(*dst));
    ACL_COMMON_MASK_XARGS
}
#undef X

#define X XARGS_ENCODE
static inline void acl_pattern_mac_encode(struct acl_pattern_mac *src,
                                          uint32_t *dst) {
    ACL_MAC_PATTERN_XARGS
}
static inline void acl_mask_mac_encode(struct acl_mask_mac *src,
                                       uint32_t *dst) {
    ACL_MAC_MASK_XARGS
}
static inline void acl_pattern_ipv4_encode(struct acl_pattern_ipv4 *src,
                                           uint32_t *dst) {
    ACL_IPV4_PATTERN_XARGS
}
static inline void acl_mask_ipv4_encode(struct acl_mask_ipv4 *src,
                                        uint32_t *dst) {
    ACL_IPV4_MASK_XARGS
}
static inline void acl_action_encode(struct acl_rule *rule, uint32_t *dst) {
    struct acl_action *src = &rule->action;
    ACL_ACTION_XARGS
}
#undef X

/*--------------------------------------------------------------------------------*\
\*--------------------------------------------------------------------------------*/
static inline void acl_pattern_encode(struct acl_rule *rule, uint32_t *regs) {
    switch(rule->type) {
        case ACL_RULE_MAC:
            acl_pattern_mac_encode(&rule->pattern.mac, regs);
            break;
        case ACL_RULE_IPV4:
            acl_pattern_ipv4_encode(&rule->pattern.ipv4, regs);
            break;
        default:
            pr_err("[%s] ACL rule type not implemented\n", __func__);
    }
}

/*--------------------------------------------------------------------------------*\
\*--------------------------------------------------------------------------------*/
static inline void acl_mask_encode(struct acl_rule *rule, uint32_t *regs) {
    switch(rule->type) {
        case ACL_RULE_MAC:
            acl_mask_mac_encode(&rule->mask.mac, regs);
            break;
        case ACL_RULE_IPV4:
            acl_mask_ipv4_encode(&rule->mask.ipv4, regs);
            break;
        default:
            pr_err("[%s] ACL rule type not implemented\n", __func__);
    }
}

/*--------------------------------------------------------------------------------*\
\*--------------------------------------------------------------------------------*/
static struct acl_rule *acl_rule_alloc(struct aths17_context *ctx)
{
    struct acl_rule *new;
    unsigned long flags;

    new = NULL;

    spin_lock_irqsave(&ctx->lock_free, flags);

    if(!list_empty(&ctx->list_free)){
        new = list_first_entry(&ctx->list_free, struct acl_rule, list);
        list_del_init(&new->list);
    }

    spin_unlock_irqrestore(&ctx->lock_free, flags);

    return new;
}

/*--------------------------------------------------------------------------------*\
\*--------------------------------------------------------------------------------*/
static void acl_rule_free(struct aths17_context *ctx, struct acl_rule *rule)
{
    unsigned long flags;

    spin_lock_irqsave(&ctx->lock_free, flags);

    list_add(&rule->list, &ctx->list_free);

    spin_unlock_irqrestore(&ctx->lock_free, flags);
}

/*--------------------------------------------------------------------------------*\
\*--------------------------------------------------------------------------------*/
#if defined(ACL_DEBUG)
static void dump_acl_regs(const char *prefix __maybe_unused,
                          unsigned int *acl_regs __maybe_unused)
{
    int i;
    AVMNET_ERR("   %s: \n", prefix);
    for(i = 4; i >= 0; i--) {
        AVMNET_ERR("    byte[%2d:%2d] %#.8x\n", (i * 4) + 3, (i * 4),
                   acl_regs[i]);
    }
}
#endif

/*--------------------------------------------------------------------------------*\
\*--------------------------------------------------------------------------------*/
#if defined(AVM_PA_HARDWARE_PA_HAS_SESSION_STATS)
static int acl_alloc_cnt(void) {

	int unused;

	do {
		unused = find_next_zero_bit(acl_cnt_bmp, AVMNET_AR8337_MAX_ACL_COUNTER, 0);
		if (unused >= AVMNET_AR8337_MAX_ACL_COUNTER)
            return -1;
	} while(test_and_set_bit(unused, acl_cnt_bmp));

	return unused;
}

/*--------------------------------------------------------------------------------*\
\*--------------------------------------------------------------------------------*/
static void acl_free_cnt(uint32_t index) {

    if (index < AVMNET_AR8337_MAX_ACL_COUNTER) {
        clear_bit(index, acl_cnt_bmp);
    }
    return;
}

/*--------------------------------------------------------------------------------*\
 * es wird jede Sekunde gelesen - bei max. 1GB/s reicht ein Counter aus
\*--------------------------------------------------------------------------------*/
static inline void acl_rate_policer_get_count(struct aths17_context *ctx, struct acl_rule *acl_rule) {

    unsigned int cnt0 = 0;
    unsigned long flags;
    int result;

    /*--- AVMNET_DEBUG("[%s] %sabled %d\n", __func__, acl_rule->action.rate_en ? "en" : "dis", acl_rule->action.rate_sel); ---*/

    BUG_ON(acl_rule->action.rate_sel > AVMNET_AR8337_MAX_ACL_COUNTER);

    result = ctx->this_module->lock(ctx->this_module);
    if(result != 0){
        AVMNET_WARN("[%s] Interrupted while waiting for lock\n", __func__);
        return;
    }

    cnt0 = avmnet_athrs37_reg_read(ctx->this_module, S17_ACL_COUNTER0(acl_rule->action.rate_sel));
    AVMNET_DEBUG("{%s} cnt0 0x%x\n", __func__, cnt0);

    /*--- reset counter ---*/
    avmnet_athrs37_reg_write(ctx->this_module, S17_ACL_CNT_RESET_REG, 1 << acl_rule->action.rate_sel);
    avmnet_athrs37_reg_write(ctx->this_module, S17_ACL_CNT_RESET_REG, 0);

    ctx->this_module->unlock(ctx->this_module);

    spin_lock_irqsave(&ctx->lock_active, flags);
    acl_rule->byte_count += cnt0;
    spin_unlock_irqrestore(&ctx->lock_active, flags);


    return;
}

/*--------------------------------------------------------------------------------*\
\*--------------------------------------------------------------------------------*/
static void work_acl_policer(struct aths17_context *ctx) {

    struct acl_rule *acl_rule;
    unsigned long flags;

    spin_lock_irqsave(&ctx->lock_active, flags);
    list_for_each_entry(acl_rule, &ctx->list_active, list) {
        if (acl_rule->action.rate_en)
            acl_rate_policer_get_count(ctx, acl_rule);
    }
    spin_unlock_irqrestore(&ctx->lock_active, flags);

    return;
}

/*--------------------------------------------------------------------------------*\
\*--------------------------------------------------------------------------------*/
static inline void acl_rate_policer_set(struct aths17_context *ctx, struct acl_rule *acl_rule) {

    unsigned int value;

    BUG_ON(acl_rule->action.rate_sel > AVMNET_AR8337_MAX_ACL_COUNTER);

    AVMNET_DEBUG("[%s] rate_sel %d %sable\n", __func__, acl_rule->action.rate_sel, 
            acl_rule->action.rate_en ? "en" : "dis");

    value = 1 << acl_rule->action.rate_sel;
    avmnet_athrs37_reg_write(ctx->this_module, S17_ACL_CNT_RESET_REG, value);

    value = 0;
    avmnet_athrs37_reg_write(ctx->this_module, S17_ACL_CNT_RESET_REG, value);

    value = avmnet_athrs37_reg_read(ctx->this_module, S17_ACL_POLICY_MODE_REG);
    if (acl_rule->action.rate_en) 
        value |= 1 << acl_rule->action.rate_sel;
    else
        value &= ~(1 << acl_rule->action.rate_sel);
    avmnet_athrs37_reg_write(ctx->this_module, S17_ACL_POLICY_MODE_REG, value);

    if ( ! acl_rule->action.rate_en )
        return;
    
    value = avmnet_athrs37_reg_read(ctx->this_module,  S17_ACL_COUNTER_MODE_REG);
    value |= 1 << acl_rule->action.rate_sel;       /*--- bytecounter ---*/
    avmnet_athrs37_reg_write(ctx->this_module, S17_ACL_COUNTER_MODE_REG, value);

    return;
}
#endif /*--- #if defined(AVM_PA_HARDWARE_PA_HAS_SESSION_STATS) ---*/

/*--------------------------------------------------------------------------------*\
\*--------------------------------------------------------------------------------*/
static inline void acl_rule_add(struct aths17_context *ctx, struct acl_rule *acl_rule)
{
    uint32_t acl_regs[5];
    int result;

    AVMNET_DEBUG("[%s] %d\n", __func__, acl_rule->hw_index);

    result = ctx->this_module->lock(ctx->this_module);
    if(result != 0){
        AVMNET_WARN("[%s] Interrupted while waiting for lock\n", __func__);
        goto err_out;
    }

#if defined(AVM_PA_HARDWARE_PA_HAS_SESSION_STATS)
    {
        int32_t acl_cnt_id = acl_alloc_cnt();
        if (acl_cnt_id >= 0) {
            acl_rule->action.rate_en = 1;   /*--- rate_enable zur acl_rule hinzufuegen ---*/
            acl_rule->action.rate_sel = acl_cnt_id;
            acl_rate_policer_set(ctx, acl_rule);
        }
    }
#endif

    /* pattern */
    memset(acl_regs, 0, sizeof(acl_regs));
    acl_pattern_encode(acl_rule, acl_regs);
#if defined(ACL_DEBUG)
    dump_acl_regs("pattern", acl_regs);
#endif
    write_acl_data_regs(ctx, acl_regs, acl_rule->hw_index,
                        S17_ACL_FUNC0_REG_ACL_RULE_SELECT_RULE);

    /* mask */
    memset(acl_regs, 0, sizeof(acl_regs));
    acl_mask_encode(acl_rule, acl_regs);
#if defined(ACL_DEBUG)
    dump_acl_regs("mask", acl_regs);
#endif
    write_acl_data_regs(ctx, acl_regs, acl_rule->hw_index,
                        S17_ACL_FUNC0_REG_ACL_RULE_SELECT_MASK);


    AVMNET_DEBUG("[%s] acl_rule %d rate_sel = %d %s\n", __func__, acl_rule->hw_index,
             acl_rule->action.rate_sel, acl_rule->action.rate_en ? "enabled" : "disabled");

    /* action */
    memset(acl_regs, 0, sizeof(acl_regs));
    acl_action_encode(acl_rule, acl_regs);
#if defined(ACL_DEBUG)
    dump_acl_regs("action", acl_regs);
#endif
    write_acl_data_regs(ctx, acl_regs, acl_rule->hw_index,
                        S17_ACL_FUNC0_REG_ACL_RULE_SELECT_RESULT);

    ctx->this_module->unlock(ctx->this_module);

err_out:
    return;
}

/*--------------------------------------------------------------------------------*\
\*--------------------------------------------------------------------------------*/
static inline void acl_rule_remove(struct aths17_context *ctx,
                                   struct acl_rule *acl_rule)
{
    uint32_t acl_regs[5];
    int result;

    AVMNET_DEBUG("[%s] %d\n", __func__, acl_rule->hw_index);

    result = ctx->this_module->lock(ctx->this_module);
    if(result != 0){
        AVMNET_WARN("[%s] Interrupted while waiting for lock\n", __func__);
        goto err_out;
    }

#if defined(AVM_PA_HARDWARE_PA_HAS_SESSION_STATS)
    if (acl_rule->action.rate_en) {
        acl_rule->action.rate_en = 0;
        acl_rate_policer_set(ctx, acl_rule);
        acl_free_cnt(acl_rule->action.rate_sel);
    }
#endif

    memset(acl_regs, 0, sizeof(acl_regs));

    write_acl_data_regs(ctx, acl_regs, acl_rule->hw_index,
                        S17_ACL_FUNC0_REG_ACL_RULE_SELECT_RULE);

    write_acl_data_regs(ctx, acl_regs, acl_rule->hw_index,
                        S17_ACL_FUNC0_REG_ACL_RULE_SELECT_MASK);

    write_acl_data_regs(ctx, acl_regs, acl_rule->hw_index,
                        S17_ACL_FUNC0_REG_ACL_RULE_SELECT_RESULT);

    ctx->this_module->unlock(ctx->this_module);

err_out:
    return;
}

/*--------------------------------------------------------------------------------*\
\*--------------------------------------------------------------------------------*/
#if !defined(set_mb)
#define set_mb(var, value)  do { var = value;  mb(); } while (0)
#endif
static void bridge_work_acl(struct aths17_context *ctx)
{
    unsigned long flags, do_add;
    struct acl_rule *rule, *tmp;

    /* find all rules on the active list that need processing and
     * move them to the process list */
    AVMNET_DEBUG("[%s] cleaning active rules\n", __func__);
    do{
        rule = NULL;
        spin_lock_irqsave(&ctx->lock_active, flags);
        list_for_each_entry(tmp, &ctx->list_active, list){
            if(tmp->state != acl_state_active){
                list_del_init(&tmp->list);

                /* set state to acl_state_erase to indicate that the
                 * rule was on the active list and must be cleared on
                 * the switch */
                rule = tmp;
                set_mb(rule->state, acl_state_erase);
                break;
            }
        }
        spin_unlock_irqrestore(&ctx->lock_active, flags);

        if(rule != NULL){
            spin_lock_irqsave(&ctx->lock_process, flags);
            list_add(&rule->list, &ctx->list_process);
            spin_unlock_irqrestore(&ctx->lock_process, flags);
        }
    }while(rule != NULL);

    AVMNET_DEBUG("[%s] processing pending rules\n", __func__);

    while(!list_empty(&ctx->list_process)){
        do_add = 0;

        /* get first entry from process list */
        spin_lock_irqsave(&ctx->lock_process, flags);
        smp_mb();
        rule = list_first_entry(&ctx->list_process, struct acl_rule, list);
        list_del_init(&rule->list);
        spin_unlock_irqrestore(&ctx->lock_process, flags);

        /* lock_active is the synchronization point between bridge_work_acl
         * and bridge_remove_pa_session.
         * bridge_remove_pa_session might be called  before we had a chance to
         * add the acl to the switch. Once we reach this point and the rule's
         * state is still acl_state_add, it will be programmed.
         */
        spin_lock_irqsave(&ctx->lock_active, flags);
        smp_rmb();
        if(rule->state == acl_state_add){
            set_mb(rule->state, acl_state_active);
            list_add(&rule->list, &ctx->list_active);
            do_add = 1;
        }
        spin_unlock_irqrestore(&ctx->lock_active, flags);

        /* program rule to switch and move on to next list entry (if any) */
        if(do_add){
            acl_rule_add(ctx, rule);
            continue;
        }

        /* at this point we are only dealing with rule removal. If
         * bridge_remove_pa_session was called after the rule had been added
         * to the active list, it will have state acl_state_erase and we must
         * remove it from the switch. Otherwise we can just move it to the
         * free list.
         */
        if(rule->state == acl_state_erase){
            acl_rule_remove(ctx, rule);
        }

        acl_rule_free(ctx, rule);
    };
}

static const struct acl_mask_mac bridge_acl_mask = {
    .da = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff },
    .sa = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff },
    .rule_type = S17_ACL_RULE_TYPE_MAC,
    .rule_valid = S17_ACL_RULE_VALID_SE,
};

/*--------------------------------------------------------------------------------*\
\*--------------------------------------------------------------------------------*/
static int bridge_add_pa_session(struct avm_pa_session *avm_pa_session)
{
    unsigned int n;
    unsigned int negress = 0;
    struct avm_pa_pid_hwinfo *ingress_hw;
    struct avm_pa_pid_hwinfo *egress_hw;
    struct avm_pa_egress     *avm_egress;
    struct aths17_context *ctx = NULL;
    struct acl_rule *acl_rule;
    struct acl_pattern_mac *mac_pattern;
    struct acl_action *action;
    unsigned long flags;

    AVMNET_DEBUG("[%s]", __func__);

    ingress_hw = avm_pa_pid_get_hwinfo(avm_pa_session->ingress_pid_handle);

    if((ingress_hw == NULL) ||
       (ingress_hw->flags & AVMNET_DEVICE_IFXPPA_ETH_WAN)) {
        pr_debug(
          "[%s] no bridging acceleration for AVM ATA device [ingress]\n",
          __func__);
        return AVM_PA_TX_ERROR_SESSION;
    }

    ctx = (struct aths17_context *) 
        ((avmnet_module_t *)(ingress_hw->avmnet_switch_module))->priv;

    acl_rule = acl_rule_alloc(ctx);
    if(!acl_rule) {
        pr_err("[%s] No ACL slots left.\n", __func__);
        return AVM_PA_TX_ERROR_SESSION;
    }

    acl_rule->type = ACL_RULE_MAC;
    acl_rule->state = acl_state_add;
    acl_rule->mask.mac = bridge_acl_mask;
    mac_pattern = &acl_rule->pattern.mac;
    action = &acl_rule->action;

    memset(mac_pattern, 0x0, sizeof(*mac_pattern));
    memset(action, 0x0, sizeof(*action));
#if defined(AVM_PA_BSESSION_STRUCT) && AVM_PA_BSESSION_STRUCT == 2
#  define BSESSION_ETH(bsession) ((bsession)->hdr)
#else
#  define BSESSION_ETH(bsession) (&((bsession)->ethh))
#endif

    mac_pattern->source_port = (1 << ingress_hw->port_number);
#if defined(__LITTLE_ENDIAN)
    for(n = 0; n < ETH_ALEN; n++)
        mac_pattern->sa[ETH_ALEN - 1 - n] =
          BSESSION_ETH(avm_pa_session->bsession)->h_source[n];
#else
    mac_pattern->sa[0] = BSESSION_ETH(avm_pa_session->bsession)->h_source[2];
    mac_pattern->sa[1] = BSESSION_ETH(avm_pa_session->bsession)->h_source[3];
    mac_pattern->sa[2] = BSESSION_ETH(avm_pa_session->bsession)->h_source[4];
    mac_pattern->sa[3] = BSESSION_ETH(avm_pa_session->bsession)->h_source[5];
    mac_pattern->sa[4] = BSESSION_ETH(avm_pa_session->bsession)->h_source[0];
    mac_pattern->sa[5] = BSESSION_ETH(avm_pa_session->bsession)->h_source[1];
#endif
    // memcpy(mac_pattern->da, avm_pa_session->bsession->ethh.h_dest, ETH_ALEN);
#if defined(__LITTLE_ENDIAN)
    for(n = 0; n < ETH_ALEN; n++)
        mac_pattern->da[ETH_ALEN - 1 - n] =
          BSESSION_ETH(avm_pa_session->bsession)->h_dest[n];
#else
    mac_pattern->da[0] = BSESSION_ETH(avm_pa_session->bsession)->h_dest[2];
    mac_pattern->da[1] = BSESSION_ETH(avm_pa_session->bsession)->h_dest[3];
    mac_pattern->da[2] = BSESSION_ETH(avm_pa_session->bsession)->h_dest[4];
    mac_pattern->da[3] = BSESSION_ETH(avm_pa_session->bsession)->h_dest[5];
    mac_pattern->da[4] = BSESSION_ETH(avm_pa_session->bsession)->h_dest[0];
    mac_pattern->da[5] = BSESSION_ETH(avm_pa_session->bsession)->h_dest[1];
#endif

    avm_pa_for_each_egress(avm_egress, avm_pa_session) {
        egress_hw = avm_pa_pid_get_hwinfo(avm_egress->pid_handle);
        if(egress_hw && (egress_hw->flags & (AVMNET_DEVICE_IFXPPA_ETH_LAN))) {
            /* TODO IFX flag?! */
            action->des_port |= 1 << (egress_hw->port_number);
            action->des_port_over_en = 1;
            negress++;
        } else {
            pr_debug("[%s] Ignoring unknown egress device for pid handle %d\n",
                     __func__, avm_pa_session->ingress_pid_handle);
        }
    }
    if(negress == 0) {
        pr_debug("[%s] No egress ports for session found, aborting\n", __func__);
        acl_rule_free(ctx, acl_rule);
        return AVM_PA_TX_ERROR_SESSION;
    }

    avm_pa_set_hw_session(avm_pa_session, acl_rule);

    spin_lock_irqsave(&ctx->lock_process, flags);
    list_add_tail(&acl_rule->list, &ctx->list_process);
    smp_wmb();
    spin_unlock_irqrestore(&ctx->lock_process, flags);

    work_send_event(ctx, WORK_EVENT_ACL);

    AVMNET_DEBUG("[%s] added\n ", __func__);

    return AVM_PA_TX_SESSION_ADDED;
}

/*--------------------------------------------------------------------------------*\
\*--------------------------------------------------------------------------------*/
static signed int
bridge_remove_pa_session(struct avm_pa_session *avm_pa_session)
{
    struct acl_rule *acl_rule;
    struct avm_pa_pid_hwinfo *ingress_hw;
    struct aths17_context *ctx = NULL;
    unsigned long flags;

    AVMNET_DEBUG("[%s]\n ", __func__);

    acl_rule = avm_pa_get_hw_session(avm_pa_session);

    ingress_hw = avm_pa_pid_get_hwinfo(avm_pa_session->ingress_pid_handle);
    if(ingress_hw == NULL) {
        pr_debug("[%s] Illegal hw handle for AVM_PA pid %u!\n", __func__,
                       avm_pa_session->ingress_pid_handle);
        return AVM_PA_TX_ERROR_SESSION;
    }
    ctx = (struct aths17_context *)
        ((avmnet_module_t *)(ingress_hw->avmnet_switch_module))->priv;

    /* clear hw_session pointer, otherwise PA GC does not work */
    avm_pa_set_hw_session(avm_pa_session, NULL);

    /* mark rule as to be removed */
    spin_lock_irqsave(&ctx->lock_active, flags);
    smp_rmb();
    if(acl_rule && (acl_rule->state == acl_state_active)) {
        set_mb(acl_rule->state, acl_state_remove);
    }
    spin_unlock_irqrestore(&ctx->lock_active, flags);

    work_send_event(ctx, WORK_EVENT_ACL);

    return AVM_PA_TX_OK;
}

/*--------------------------------------------------------------------------------*\
\*--------------------------------------------------------------------------------*/
static int avmnet_ar8337_remove_session(struct avm_pa_session *avm_pa_session)
{
    if(avm_pa_session->bsession) {
        return bridge_remove_pa_session(avm_pa_session);
    }
    return AVM_PA_TX_ERROR_SESSION;
}

/*--------------------------------------------------------------------------------*\
\*--------------------------------------------------------------------------------*/
static void acl_regs_show(struct seq_file *m,
                             const char *prefix,
                             unsigned int *acl_regs)
{
    int i;
    seq_printf(m, "   %s: \n", prefix);
    for(i = 4; i >= 0; i--){
        seq_printf(m, "    byte[%2d:%2d] %#.8x\n", (i * 4) + 3, (i * 4),
                   acl_regs[i]);
    }
}

/*--------------------------------------------------------------------------------*\
\*--------------------------------------------------------------------------------*/
static int acl_table_show(struct seq_file *m, void *data __maybe_unused)
{
    unsigned int rule;
    unsigned int acl_regs_rule[5];
    unsigned int acl_regs_mask[5];
    unsigned int acl_regs_result[5];
    struct aths17_context *ctx;
    struct acl_mask_common mask;
    avmnet_module_t *this = (avmnet_module_t *) m->private;
    int result;

    ctx = (struct aths17_context *)this->priv;
    result = 0;

    for(rule = 0; rule < AVMNET_AR8337_ACL_RULES; rule++) {
        memset(acl_regs_rule, 0, sizeof(acl_regs_rule));
        memset(acl_regs_mask, 0, sizeof(acl_regs_mask));
        memset(acl_regs_result, 0, sizeof(acl_regs_result));

        result = ctx->this_module->lock(ctx->this_module);

        if(result != 0){
            AVMNET_WARN("[%s] Interrupted while waiting for lock\n", __func__);
            goto err_out;
        }

        read_acl_data_regs(ctx, acl_regs_mask, rule,
                           S17_ACL_FUNC0_REG_ACL_RULE_SELECT_MASK);

        acl_mask_common_decode(&mask, acl_regs_mask);

        if(mask.rule_valid) {
            read_acl_data_regs(ctx, acl_regs_rule, rule,
                               S17_ACL_FUNC0_REG_ACL_RULE_SELECT_RULE);
            read_acl_data_regs(ctx, acl_regs_result, rule,
                               S17_ACL_FUNC0_REG_ACL_RULE_SELECT_RESULT);
        }

        ctx->this_module->unlock(ctx->this_module);

        if(mask.rule_valid) {
            seq_printf(m, "-- valid entry %d: --\n", rule);
            seq_printf(m, " type %d (raw output):\n", mask.rule_type);
            acl_regs_show(m, "rule", acl_regs_rule);
            acl_regs_show(m, "mask", acl_regs_mask);
            acl_regs_show(m, "result", acl_regs_result);
        }
    }

err_out:
    return result;
}

/*--------------------------------------------------------------------------------*\
\*--------------------------------------------------------------------------------*/
static int acl_read_open(struct inode *inode, struct file *file)
{
    return single_open(file, acl_table_show, PDE_DATA(inode));
}

/*------------------------------------------------------------------------------------------*\
 * Add AVM_PA session on MAC address layer
 *
 * Return: AVM_PA_TX_ERROR_SESSION or AVM_PA_TX_SESSION_ADDED
\*------------------------------------------------------------------------------------------*/
static int avmnet_ar8337_add_session(struct avm_pa_session *avm_pa_session)
{
    if(avm_pa_session->negress == 0) {
        pr_debug("[%s] No egress defined. Refusing session\n", __func__);
        return AVM_PA_TX_ERROR_SESSION;
    }

    if(avm_pa_session->bsession != NULL) {
        return bridge_add_pa_session(avm_pa_session);
    }

    pr_debug("[%s] Session type not supported. Refusing session\n", __func__);

    return AVM_PA_TX_ERROR_SESSION;
}

/*--------------------------------------------------------------------------------*\
\*--------------------------------------------------------------------------------*/
#if defined(AVM_PA_HARDWARE_PA_HAS_SESSION_STATS)
static signed int avmnet_ar8337_session_stats(struct avm_pa_session *avm_pa_session, 
                                              struct avm_pa_session_stats *stats) {

    struct avm_pa_pid_hwinfo *ingress_hw;
    struct aths17_context *ctx = NULL;
    struct acl_rule *acl_rule;
    unsigned long flags;

    AVMNET_DEBUG("[%s]", __func__);

	BUG_ON(avm_pa_session->session_handle >= CONFIG_AVM_PA_MAX_SESSION);
	memset(stats, 0, sizeof(struct avm_pa_session_stats));

    ingress_hw = avm_pa_pid_get_hwinfo(avm_pa_session->ingress_pid_handle);
    if(ingress_hw == NULL) {
        pr_debug("[%s] Illegal hw handle for AVM_PA pid %u!\n", __func__,
                       avm_pa_session->ingress_pid_handle);
        return AVM_PA_TX_ERROR_SESSION;
    }

    ctx = (struct aths17_context *)
        ((avmnet_module_t *)(ingress_hw->avmnet_switch_module))->priv;

    acl_rule = avm_pa_get_hw_session(avm_pa_session);

    spin_lock_irqsave(&ctx->lock_active, flags);
    smp_rmb();
    if(acl_rule && (acl_rule->state == acl_state_active)) {
        if (acl_rule->byte_count && acl_rule->action.rate_en) {
            AVMNET_DEBUG("{%s} rule %d %lld ", __func__, acl_rule->hw_index, acl_rule->byte_count);
            stats->validflags |= AVM_PA_SESSION_STATS_VALID_HIT;
            stats->validflags |= AVM_PA_SESSION_STATS_VALID_BYTES;
            stats->tx_bytes = acl_rule->byte_count;
            acl_rule->byte_count = 0;
        }
    }
    smp_wmb();
    spin_unlock_irqrestore(&ctx->lock_active, flags);

	return 0;
}
#endif /*--- #if defined(AVM_PA_HARDWARE_PA_HAS_SESSION_STATS) ---*/

#endif /*--- #if defined(CONFIG_AVM_PA) && defined(CONFIG_AVM_CPMAC_ATH_PPA) ---*/

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_ar8337_init(avmnet_module_t *this) {

    struct aths17_context *ctx;
    int i, result;

    AVMNET_DEBUG("[%s] Init on module %s called.\n", __func__, this->name);

    ctx = kzalloc(sizeof(struct aths17_context) + (this->num_children * sizeof(avmnet_linkstatus_t)), GFP_KERNEL);
    if(ctx == NULL){
        AVMNET_ERR("[%s] init of avmnet-module %s failed.\n", __func__, this->name);
        return -ENOMEM;
    }

    this->priv = ctx;
    ctx->this_module = this;
    sema_init(&(ctx->mdio_lock), 1);
    sema_init(&(ctx->lock), 1);

    if (this->initdata.swi.flags & AVMNET_CONFIG_FLAG_RESET) {

#if defined(CONFIG_MACH_ATHEROS) || defined(CONFIG_ATH79)
        if(this->initdata.swi.reset < 100) {
            /*--- Hängt nicht am Shift-Register => GPIO Resourcen anmelden ---*/
            ar8337_gpio.start = this->initdata.swi.reset;
            ar8337_gpio.end = this->initdata.swi.reset;
            if(request_resource(&gpio_resource, &ar8337_gpio)) {
                AVMNET_ERR("[%s] ERROR request resource gpio %d\n", __func__, this->initdata.swi.reset);
                kfree(ctx);
                return -EIO;
            }
        }
#elif defined(INFINEON_CPU)
        if(ifx_gpio_register(IFX_GPIO_MODULE_EXTPHY_RESET) != IFX_SUCCESS) {
            AVMNET_ERR("[%s] ERROR request resource gpio %d\n", __FUNCTION__, this->initdata.swi.reset);
            kfree(ctx);
            return -EIO;
        }
#endif
    }

    AVMNET_INFO("{%s} reset %d irq %d\n", __func__, this->initdata.swi.reset, this->initdata.swi.irq);

    for(i = 0; i < this->num_children; ++i){
        result = this->children[i]->init(this->children[i]);
        if(result < 0){
            // handle error
        }
    }

    ctx->pending_events = 0;
    init_waitqueue_head(&ctx->event_wq);
    ctx->kthread = kthread_run(work_thread, (void * )ctx, "ar8337_worker");
    BUG_ON( ! (void *)ctx->kthread);
    BUG_ON(IS_ERR((void *)ctx->kthread));

    avmnet_cfg_register_module(this);
    avmnet_cfg_add_seq_procentry(this, "rmon_all", &read_rmon_all_fops);
    avmnet_cfg_add_procentry(this, "switch_reg", &avmnet_proc_switch_fops);

#if defined(CONFIG_AVM_PA) && defined(CONFIG_AVM_CPMAC_ATH_PPA)
    avmnet_cfg_add_seq_procentry(this, "acl", &read_acl_fops);
    avmnet_ar8337_init_pa(ctx);
#endif /*--- #if defined(CONFIG_AVM_PA) ---*/

    return 0;
}
 
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_ar8337_exit(avmnet_module_t *this) {

    int i, result;

    AVMNET_DEBUG("[%s] Exit on module %s called.\n", __func__, this->name);

    for(i = 0; i < this->num_children; ++i){
        result = this->children[i]->exit(this->children[i]);
        if(result != 0){
            // handle error
        }
    }

    if (this->initdata.swi.flags & AVMNET_CONFIG_FLAG_IRQ) {
        free_irq(this->initdata.swi.irq, ar8337_link_intr);
    }

    kfree(this->priv);

    return 0;
}

static int update_rmon_cache(avmnet_module_t *sw_mod)
{
	size_t reg;
	size_t port;
    	struct aths17_context *ctx = (struct aths17_context *)sw_mod->priv;

	if(sw_mod->lock(sw_mod)) {
		AVMNET_WARN("[%s] Interrupted while waiting for lock!\n", __func__);
        	return -EINTR;
	}

	for (reg=0; reg < AR8337_RMON_COUNTER; reg++){
		for (port=0; port < AR8337_PORTS; port++){
			ctx->rmon_cache[port][reg] += avmnet_athrs37_reg_read(sw_mod, 0x1000 + (port * 0x100) + (reg*4));
		}
	}
	sw_mod->unlock(sw_mod);
	return 0;
}


static inline u32 read_rmon_reg_cached(avmnet_module_t *sw_mod, u32 sw_port, u32 reg)
{
    	struct aths17_context *ctx = (struct aths17_context *)sw_mod->priv;
	return ctx->rmon_cache[sw_port][reg];
}


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static int read_rmon_all_show(struct seq_file *seq, void *data __attribute__ ((unused)) ) {
    char *cnt8316names[] = {
              /* 0x00 */    "RxBroad",
              /* 0x04 */    "RxPause",
              /* 0x08 */    "RxMulti",
              /* 0x0c */    "RxFcsErr",
              /* 0x10 */    "RxAlignErr",
              /* 0x14 */    "RxRunt",
              /* 0x18 */    "RxFragment",
              /* 0x1c */    "Rx64Byte",
              /* 0x20 */    "Rx128Byte",
              /* 0x24 */    "Rx256Byte",
              /* 0x28 */    "Rx512Byte",
              /* 0x2c */    "Rx1024Byte",
              /* 0x30 */    "Rx1518Byte",
              /* 0x34 */    "RxMaxByte",
              /* 0x38 */    "RxTooLong",
              /* 0x3c */    "RxGoodByteLow",
              /* 0x40 */    "RxGoodByteHigh",
              /* 0x44 */    "RxBadByteLow",
              /* 0x48 */    "RxBadByteHigh",
              /* 0x4c */    "RxOverflow",
              /* 0x50 */    "Filtered",
              /* 0x54 */    "TxBroad",
              /* 0x58 */    "TxPause",
              /* 0x5c */    "TxMulti",
              /* 0x60 */    "TxUnderRun",
              /* 0x64 */    "Tx64Byte",
              /* 0x68 */    "Tx128Byte",
              /* 0x6c */    "Tx256Byte",
              /* 0x70 */    "Tx512Byte",
              /* 0x74 */    "Tx1024Byte",
              /* 0x78 */    "Tx1518Byte",
              /* 0x7c */    "TxMaxByte",
              /* 0x80 */    "TxOverSize",
              /* 0x84 */    "TxByteLow",
              /* 0x88 */    "TxByteHigh",
              /* 0x8c */    "TxCollision",
              /* 0x90 */    "TxAbortCol",
              /* 0x94 */    "TxMultiCol",
              /* 0x98 */    "TxSingleCol",
              /* 0x9c */    "TxExcDefer",
              /* 0xa0 */    "TxDefer",
              /* 0xa4 */    "TxLateCol"
                           };
    unsigned int i;
    avmnet_module_t *this;

    this = (avmnet_module_t *) seq->private;

    if (update_rmon_cache(this))
	    return -EINTR;

    seq_printf(seq, "Counters\n");
    seq_printf(seq, "                    CPU port      Port 1      Port 2      Port 3      Port 4      Port 5\n");
    for(i = 0; i < AR8337_RMON_COUNTER; i++) {
        seq_printf(seq, "  %14s  %10.u  %10.u  %10.u  %10.u  %10.u  %10.u\n", 
                        cnt8316names[i],
			read_rmon_reg_cached(this, 0, i),
			read_rmon_reg_cached(this, 1, i),
			read_rmon_reg_cached(this, 2, i),
			read_rmon_reg_cached(this, 3, i),
			read_rmon_reg_cached(this, 4, i),
			read_rmon_reg_cached(this, 5, i));
    }
    seq_printf(seq, "\n");

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static int read_rmon_all_open(struct inode *inode, struct file *file) {

    return single_open(file, read_rmon_all_show, PDE_DATA(inode));
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
#ifdef INFINEON_CPU
static void reset_switch(avmnet_module_t *this, enum rst_type type)
{
    if((this->initdata.swi.flags & AVMNET_CONFIG_FLAG_RESET) == 0){
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
        ifx_gpio_output_clear(this->initdata.swi.reset, IFX_GPIO_MODULE_EXTPHY_RESET);
        mdelay(10);
    }

    if(type == rst_off || type == rst_pulse){
        AVMNET_INFOTRC("[%s] PHY %s reset off\n", __func__, this->name);
        ifx_gpio_output_set(this->initdata.swi.reset, IFX_GPIO_MODULE_EXTPHY_RESET);
        mdelay(10);
    }
}
#elif defined(CONFIG_MACH_QCA953x) || defined(CONFIG_SOC_QCA953X)
static void reset_switch(avmnet_module_t *this, enum rst_type type)
{
    if((this->initdata.swi.flags & AVMNET_CONFIG_FLAG_RESET) == 0){
        AVMNET_ERR("[%s] Reset not configured for module %s\n", __func__, this->name);
        return;
    }

    if ( ! this->trylock(this)) {
        AVMNET_ERR("[%s] Called without MUTEX held!\n", __func__);
        dump_stack();
        this->unlock(this);
    }

    if(type == rst_on || type == rst_pulse){
        ath_avm_gpio_out_bit(this->initdata.swi.reset, 0);
        mdelay(100);
    }

    if(type == rst_off || type == rst_pulse){
        ath_avm_gpio_out_bit(this->initdata.swi.reset, 1);
        mdelay(100);
    }
}
#else
static void reset_switch(avmnet_module_t *this __maybe_unused, enum rst_type type __maybe_unused)
{
    /*
     * dummy
     */
}
#endif // INFINEON_CPU

static inline u64 con32(u32 high, u32 low)
{
	return ((u64)high << 32) | (u64)low;
}

struct avmnet_hw_rmon_counter *create_ar8337_rmon_cnt(avmnet_device_t *avm_dev)
{
    int switch_port;
    struct avmnet_hw_rmon_counter *res;
    avmnet_module_t *sw_module; 

    if (!avm_dev)
        return NULL;

    sw_module = get_switch_by_netdev(avm_dev->device);

    if (!sw_module)
        return NULL;

    res = kzalloc(sizeof(struct avmnet_hw_rmon_counter),GFP_KERNEL);
    if (!res)
        return NULL;

    switch_port = avm_dev->vlanID;
    update_rmon_cache(sw_module);

    res->rx_pkts_good =
        read_rmon_reg_cached(sw_module, switch_port, 7) +
        read_rmon_reg_cached(sw_module, switch_port, 8) +
        read_rmon_reg_cached(sw_module, switch_port, 9) +
        read_rmon_reg_cached(sw_module, switch_port, 10) +
        read_rmon_reg_cached(sw_module, switch_port, 11) +
        read_rmon_reg_cached(sw_module, switch_port, 12) +
        read_rmon_reg_cached(sw_module, switch_port, 13);

    res->tx_pkts_good = 
        read_rmon_reg_cached(sw_module, switch_port, 25) +
        read_rmon_reg_cached(sw_module, switch_port, 26) +
        read_rmon_reg_cached(sw_module, switch_port, 27) +
        read_rmon_reg_cached(sw_module, switch_port, 28) +
        read_rmon_reg_cached(sw_module, switch_port, 29) +
        read_rmon_reg_cached(sw_module, switch_port, 30) +
        read_rmon_reg_cached(sw_module, switch_port, 31);

    res->rx_bytes_good = con32(read_rmon_reg_cached(sw_module, switch_port, 16),
                               read_rmon_reg_cached(sw_module, switch_port, 15));

    res->tx_bytes_good = con32(read_rmon_reg_cached(sw_module, switch_port, 34),
                               read_rmon_reg_cached(sw_module, switch_port, 33));

    res->rx_pkts_pause = read_rmon_reg_cached(sw_module, switch_port, 1);
    res->tx_pkts_pause = read_rmon_reg_cached(sw_module, switch_port, 22);

    res->rx_pkts_dropped =
        read_rmon_reg_cached(sw_module, switch_port, 3) +
        read_rmon_reg_cached(sw_module, switch_port, 4) +
        read_rmon_reg_cached(sw_module, switch_port, 19);

    res->tx_pkts_dropped =
        read_rmon_reg_cached(sw_module, switch_port, 24) +
        read_rmon_reg_cached(sw_module, switch_port, 32) +
        read_rmon_reg_cached(sw_module, switch_port, 35) +
        read_rmon_reg_cached(sw_module, switch_port, 41);

    res->rx_bytes_error = con32(read_rmon_reg_cached(sw_module, switch_port, 18),  
                                read_rmon_reg_cached(sw_module, switch_port, 17));
    return res;
}
