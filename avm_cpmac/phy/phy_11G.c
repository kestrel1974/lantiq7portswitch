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
#include <linux/jiffies.h>
#include <avmnet_debug.h>
#include <avmnet_module.h>
#include <avmnet_config.h>
#include <avmnet_common.h>
#include <mdio_reg.h>

#undef INFINEON_CPU
#if (defined(CONFIG_VR9) || defined(CONFIG_AR9) || defined(CONFIG_AR10))
#define INFINEON_CPU
#define GPIO_REGISTER_BROKEN

#if defined(CONFIG_GPHY_DRIVER)
#error AVM-Treiber kollidiert mit IFX-Treiber
#else
#include "firmware/vr9_phy11g_a1x.h"
#include "firmware/vr9_phy11g_a2x.h"
#include "firmware/vr9_phy22f_a1x.h"
#include "firmware/vr9_phy22f_a2x.h"
#endif

#include <switch_api/ifxmips_sw_reg.h>
#include <ifx_gpio.h>
#include <ifx_types.h>

#include <ifx_pmu.h>
#include <ifx_rcu.h>
#include <ifx_regs.h>
#endif /*--- #if (defined(CONFIG_VR9) || defined(CONFIG_AR9) || defined(CONFIG_AR10)) ---*/

#if defined(CONFIG_MACH_ATHEROS) || defined(CONFIG_ATH79)
#if ! defined(CONFIG_MACH_ATHEROS)
#define CONFIG_MACH_ATHEROS
#endif
#include <asm/mach_avm.h>
#endif

#include <linux/crc32.h>
#include "phy_11G.h"

// "real" name for register not in include file
#ifndef GFMDIO_ADD
#define GFMDIO_ADD IFX_RCU_USB_AFE_CFG_2B
#endif

#if defined(CONFIG_VR9)
#define NUM_GPHYS 2
#elif defined(CONFIG_AR10)
#define NUM_GPHYS 3
#else
#define NUM_GPHYS 0
#endif

#define SNR_BAD_LIMIT    3           // threshold below which we consider SNR too low
#define SNR_BAD_TIME     (5 * HZ)    // duration of bad SNR readings in seconds before we restart auto-negotiation
#define SNR_THRASH_TIME  (3600 * HZ) // limit SNR autoneg restarts per this number of seconds
#define SNR_THRASH_LIMIT 1           // do not restart autoneg more than this times per  SNR_THRASH_TIME seconds

//#define DEBUG_11G_FIRWMWARE_LOAD 1

#if defined(CONFIG_VR9) || defined(CONFIG_AR10)
static int load_firmware(avmnet_module_t *this);
#endif

static int set_config(avmnet_module_t *this);

static int phy_reg_open(struct inode *inode, struct file *file);
static int phy_reg_show(struct seq_file *seq, void *data __attribute__ ((unused)) );
static ssize_t phy_reg_write(struct file *filp, const char __user *buff, size_t len, loff_t *offset);

static void reset_phy(avmnet_module_t *this, enum rst_type type);

static const struct file_operations phy_reg_fops =
{
   .open    = phy_reg_open,
   .read    = seq_read,
   .write   = phy_reg_write,
   .llseek  = seq_lseek,
   .release = single_release,
};

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_phy_11G_init(avmnet_module_t *this)
{
    struct avmnet_phy_11G_context *my_ctx;
    int i, result;

    AVMNET_INFO("[%s] Init on module %s called.\n", __func__, this->name);

    my_ctx = kzalloc(sizeof(struct avmnet_phy_11G_context), GFP_KERNEL);
    if(my_ctx == NULL){
        AVMNET_ERR("[%s] init of avmnet-module %s failed.\n", __func__, this->name);
        return -ENOMEM;
    }

    this->priv = my_ctx;

#if defined(INFINEON_CPU)
    if(this->initdata.phy.flags & AVMNET_CONFIG_FLAG_RESET){
#if !defined(GPIO_REGISTER_BROKEN)
        if(ifx_gpio_register(IFX_GPIO_MODULE_EXTPHY_RESET) != IFX_SUCCESS)
#else // !defined(GPIO_REGISTER_BROKEN)
        result = ifx_gpio_pin_reserve(this->initdata.phy.reset, IFX_GPIO_MODULE_EXTPHY_RESET);
        if(result != IFX_SUCCESS)
#endif // !defined(GPIO_REGISTER_BROKEN)
        {
            AVMNET_ERR("[%s] ERROR request resource gpio %d\n", __FUNCTION__, this->initdata.phy.reset);
            kfree(my_ctx);
            return -EIO;
        }

#if defined(GPIO_REGISTER_BROKEN)
        ifx_gpio_dir_out_set(this->initdata.phy.reset, IFX_GPIO_MODULE_EXTPHY_RESET);
        ifx_gpio_altsel0_clear(this->initdata.phy.reset, IFX_GPIO_MODULE_EXTPHY_RESET);
        ifx_gpio_altsel1_clear(this->initdata.phy.reset, IFX_GPIO_MODULE_EXTPHY_RESET);
        ifx_gpio_open_drain_set(this->initdata.phy.reset, IFX_GPIO_MODULE_EXTPHY_RESET);
        ifx_gpio_pudsel_set(this->initdata.phy.reset, IFX_GPIO_MODULE_EXTPHY_RESET);
        ifx_gpio_puden_clear(this->initdata.phy.reset, IFX_GPIO_MODULE_EXTPHY_RESET);
    }
#endif // !defined(GPIO_REGISTER_BROKEN)

#endif

    // put PHY into reset
//    AVMNET_INFO("[%s] %s: mdio-control(before reset):%x \n", __func__, this->name, avmnet_mdio_read(this, MDIO_CONTROL) );
    reset_phy(this, rst_on);

    for(i = 0; i < this->num_children; ++i){
        result = this->children[i]->init(this->children[i]);
        if(result < 0){
            AVMNET_ERR("Module %s: init() failed on child %s\n", this->name, this->children[i]->name);
            return result;
        }
    }
    return 0;
}


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

int avmnet_phy_11G_setup(avmnet_module_t *this)
{
    int i, result;
    struct avmnet_phy_11G_context *ctx;

    ctx = (struct avmnet_phy_11G_context *) this->priv;

    AVMNET_INFO("[%s] Setup on module %s called.\n", __func__, this->name);

    if (this->lock(this)) {
        AVMNET_ERR("[%s] Interrupted while waiting for lock.\n", __func__);
        return -EINTR;
    }

    sema_init(&(ctx->mutex), 1);

    // default configuration
    ctx->config.advertise = (ADVERTISED_TP
                             | ADVERTISED_Autoneg
                             | ADVERTISED_10baseT_Half
                             | ADVERTISED_10baseT_Full
                             | ADVERTISED_100baseT_Half
                             | ADVERTISED_100baseT_Full);



    // setup default ethtool flocwcontrol, based on pyh initdata
    if ( this->initdata.phy.flags & AVMNET_CONFIG_FLAG_DEFAULT_EN_FC_RX ){
        if ( this->initdata.phy.flags & AVMNET_CONFIG_FLAG_DEFAULT_EN_FC_TX ){
            ctx->config.pause_setup = avmnet_pause_rx_tx;
        }
        else {
            // most reasonable default config for ethernet switches:
            // receive pause frames, but not to send pause frames
            ctx->config.pause_setup = avmnet_pause_rx;
        }
    }
    else if ( this->initdata.phy.flags & AVMNET_CONFIG_FLAG_DEFAULT_EN_FC_TX ){
        ctx->config.pause_setup = avmnet_pause_tx;
    }
    else {
        ctx->config.pause_setup = avmnet_pause_none;
    }

    // what kind of flow control is supported by the MAC, we are attached to?
    ctx->config.advertise |= calc_advertise_pause_param(
        (this->parent->initdata.mac.flags &  AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_SYM),
        (this->parent->initdata.mac.flags &  AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_ASYM),
        ctx->config.pause_setup );

    AVMNET_INFO("[%s] default pause config is %d, asym_pause_adv=%d, sym_pause_adv=%d.\n", __func__,ctx->config.pause_setup,
        (ctx->config.advertise  &  ADVERTISED_Asym_Pause),
        (ctx->config.advertise  &  ADVERTISED_Pause));

    if(this->initdata.phy.flags & AVMNET_CONFIG_FLAG_PHY_GBIT){
        ctx->config.advertise |= (ADVERTISED_1000baseT_Half
                                  | ADVERTISED_1000baseT_Full);
    }

    ctx->config.autoneg = AUTONEG_ENABLE;
    ctx->config.port = PORT_TP;
    ctx->config.mdi = ETH_TP_MDI;
    ctx->config.speed = SPEED_100;

    ctx->snr_thrash_counter = 0;
    ctx->snr_thrash_time = jiffies;
    ctx->snr_bad_time = jiffies;

#if defined(CONFIG_VR9) || defined(CONFIG_AR10)
    // firmware for internal phys has to be loaded -
    // before reset is released for the first time
    // otherwise phy might hang!

    if(this->initdata.phy.flags & AVMNET_CONFIG_FLAG_INTERNAL){
        load_firmware(this);
    }
#endif

    // get PHY out of reset
    reset_phy(this, rst_off);

#if 0
    if(this->initdata.phy.flags & AVMNET_CONFIG_FLAG_INTERNAL){
        unsigned int dir, altsel0, altsel1, od, pudsel, puden;

        dir = (SW_READ_REG32(IFX_GPIO_P0_DIR) & (1 << 3)) ? 1:0;
        altsel0 = (SW_READ_REG32(IFX_GPIO_P0_ALTSEL0) & (1 << 3)) ? 1:0;
        altsel1 = (SW_READ_REG32(IFX_GPIO_P0_ALTSEL1) & (1 << 3)) ? 1:0;
        od = (SW_READ_REG32(IFX_GPIO_P0_OD) & (1 << 3)) ? 1:0;
        pudsel = (SW_READ_REG32(IFX_GPIO_P0_PUDSEL) & (1 << 3)) ? 1:0;
        puden = (SW_READ_REG32(IFX_GPIO_P0_PUDEN) & (1 << 3)) ? 1:0;

        AVMNET_ERR("[%s] IFX_GPHY0_CFG : 0x%08x\n", __func__, SW_READ_REG32(IFX_GPHY0_CFG));
        AVMNET_ERR("[%s] IFX_CGU_IF_CLK: 0x%08x\n", __func__, SW_READ_REG32(IFX_CGU_IF_CLK));
        AVMNET_ERR("[%s] GPIO3: DIR: %x, ALTSEL0: %x, ALTSEL1: %x, OD: %x, PUDSEL: %x, PUDEN: %x\n", __func__,
                                dir,     altsel0,     altsel1,     od,     pudsel,     puden);
        AVMNET_ERR("[%s] RST_REQ: 0x%08x\n", __func__, ifx_rcu_rst_req_read());
    }
#endif

    mdelay(200);

    AVMNET_INFO("[%s] %s: mdio-control(after reset removed):%x \n", __func__, this->name, avmnet_mdio_read(this, MDIO_CONTROL) );

    /*
     * check for hanging PHY.
     * PHY should be out of reset by now. If it still does not respond to MDIO reads,
     * reset it again up to three times.
     */
    i = 0;
    while(avmnet_mdio_read(this, MDIO_CONTROL) == 0xffff){

        if((i >= 3) || (avmnet_mdio_read(this, MDIO_PHY_ID1) == 0)){
            AVMNET_ERR("[%s] Giving up on hanging PHY %s.\n", __func__, this->name);
            panic("[%s] Giving up on hanging PHY %s.\n", __func__, this->name);
            return -EFAULT;
        }

        AVMNET_WARN("[%s] Hanging PHY %s detected, trying to reset it.\n", __func__, this->name);
        reset_phy(this, rst_on);
        mdelay(200);

#if defined(CONFIG_VR9) || defined(CONFIG_AR10)
        if(this->initdata.phy.flags & AVMNET_CONFIG_FLAG_INTERNAL){
			AVMNET_ERR("[%s] Re-Loading firmware into phy 0x%x\n", __func__, this->initdata.phy.mdio_addr);
			load_firmware(this);
			mdelay(200);
		}
#endif // defined(CONFIG_VR9) || defined(CONFIG_AR10)

        reset_phy(this, rst_off);
        mdelay(200);

        ++i;
    }

    avmnet_mdio_write(this, MDIO_CONTROL, (MDIO_CONTROL_RESET | MDIO_CONTROL_AUTONEG_ENABLE));

    /* Check certain FW version. Only enable compatibility */
    if(avmnet_mdio_read(this, MDIO_PHY_ID1) == 0x0302
       && avmnet_mdio_read(this, MDIO_PHY_ID2) == 0x60d1
       && (avmnet_mdio_read(this, PHY11G_FWV) & 0x7FFF) >= 0x20A)
    {
        AVMNET_INFO("[%s] Setting compatibility bit\n", __func__);
        /* The following 4 MMD Register accesses are required to set the Compatibility bit for GPHYv1.4 and later */
        avmnet_mdio_write(this, MDIO_MMD_CTRL, 0x001F);
        /* make an MMD access to internal registers */
        mdelay(10);
        avmnet_mdio_write(this, MDIO_MMD_ADDR, 0x01FF);
        /* Address of the internal register */
        mdelay(10);
        avmnet_mdio_write(this, MDIO_MMD_CTRL, 0x401F);
        /* Make a Data Access to it */
        mdelay(10);
        avmnet_mdio_write(this, MDIO_MMD_ADDR, 0x0001);
        /* Set the Compatibility Bit */
        mdelay(10);
        avmnet_mdio_write(this, 0x0014, 0x8106);
        /*--- Undokumentiertes lantiq feature (sticky) ---*/
        mdelay(10);
        avmnet_mdio_write(this, MDIO_MMD_CTRL, 0x9000);
        /*--- Reset, Enable Autoneg ---*/
        mdelay(30);
    }

    /*--- disable EEE Autoneg ---*/
    avmnet_mdio_write(this, MDIO_MMD_ADDR, 0x0007);
    mdelay(10);
    avmnet_mdio_write(this, MDIO_MMD_CTRL, 0x003C);
    mdelay(10);
    avmnet_mdio_write(this, MDIO_MMD_ADDR, 0x4007);
    mdelay(10);
    avmnet_mdio_write(this, MDIO_MMD_CTRL, 0x0);
    mdelay(10);

    /*--- Setup mdio master slave behavior, autoneg features ---*/
    avmnet_mdio_write(this, MDIO_MASTER_SLAVE_CTRL, MDIO_MASTER_SLAVE_CTRL_MSPT);
    mdelay(30);

    /*--- PHY Optimierung: WRITE MDIO.PHY.TPGDATA=0x0100 ---*/
    avmnet_mdio_write(this, 0x1D, 0x0100);

    this->unlock(this);

    set_config(this);

    for(i = 0; i < this->num_children; ++i){
        result = this->children[i]->setup(this->children[i]);
        if(result != 0){
            AVMNET_ERR("Module %s: setup() failed on child %s\n", this->name, this->children[i]->name);
            return result;
        }
    }

    avmnet_cfg_register_module(this);

    avmnet_cfg_add_seq_procentry(this, "mdio_regs", &phy_reg_fops); // cannot be done before avmnet_cfg_register_module

    this->powerdown(this);

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_phy_11G_exit(avmnet_module_t *this)
{
    int i, result;

    AVMNET_INFO("[%s] Exit on module %s called.\n", __func__, this->name);

    for(i = 0; i < this->num_children; ++i){
        result = this->children[i]->exit(this->children[i]);
        if(result != 0){
            AVMNET_WARN("Module %s: exit() failed on child %s\n", this->name, this->children[i]->name);
        }
    }

    /*
     * clean up our mess
     */

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_phy_11G_setup_irq(avmnet_module_t *this, unsigned int on)
{
    unsigned int irqmask = 0;
    int result;

    /*
     * register 11GIMASK (0x19) bits:
     * 15   + Wake-On-LAN
     * 14   + Master-Slave Resolution Error
     * 13   - Next Page Received
     * 12   - Next Page Transmitted
     * 11   + Auto Negotiation Error
     * 10   + Auto Negotiation Complete
     * 9-6  - Reserved
     * 5    + Link Speed Auto-Downshift
     * 4    + MDI Polarity Change
     * 3    - MDIX Change
     * 2    + Duplex Mode Change
     * 1    + Link-Speed Change
     * 0    + Link-State Change
     */
    if(on){
        // enable interrupt on link state change 
        irqmask = PHY11G_IMASK_WOL
                  | PHY11G_IMASK_MSRE
                  | PHY11G_IMASK_ANE
                  | PHY11G_IMASK_ANC
                  | PHY11G_IMASK_ADSC
                  | PHY11G_IMASK_MDIPC
                  | PHY11G_IMASK_DXMC
                  | PHY11G_IMASK_LSPC
                  | PHY11G_IMASK_LSTC;
    }

    if (this->lock(this)) {
        return -EINTR;
    }

    result = avmnet_mdio_write(this, PHY11G_IMASK, irqmask);

    this->unlock(this);
    return result;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
unsigned int avmnet_phy_11G_reg_read(avmnet_module_t *this, unsigned int addr, unsigned int reg)
{
    return this->parent->reg_read(this->parent, addr, reg);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_phy_11G_reg_write(avmnet_module_t *this, unsigned int addr,
                                unsigned int reg, unsigned int val)
{
    return this->parent->reg_write(this->parent, addr, reg, val);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_phy_11G_lock(avmnet_module_t *this) {
    return this->parent->lock(this->parent);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void avmnet_phy_11G_unlock(avmnet_module_t *this) {
    this->parent->unlock(this->parent);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_phy_11G_trylock(avmnet_module_t *this) {
    return this->parent->trylock(this->parent);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void avmnet_phy_11G_status_changed(avmnet_module_t *this, avmnet_module_t *caller)
{
    int i;
    // struct avmnet_phy_11G_context *my_ctx = (struct avmnet_phy_11G_context *) this->priv;
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
static avmnet_linkstatus_t get_status(avmnet_module_t *this)
{
    unsigned int phy_ctrl, phy_status, phy_speed, phy_perf, phy_ctl1, phy_stat1;
    unsigned int polinv_state, polinv_conf;
    unsigned int i, corr_needed;

    avmnet_linkstatus_t status = { .Status = 0 };
    struct avmnet_phy_11G_context *ctx;

    ctx = (struct avmnet_phy_11G_context *) this->priv;

    if (this->lock(this)) {
        return status;
    }

    // first check if PHY is powered up
    phy_ctrl = avmnet_mdio_read(this, MDIO_CONTROL);
    phy_status = avmnet_mdio_read(this, MDIO_STATUS);
    phy_ctl1 = avmnet_mdio_read(this, PHY11G_PHYCTL1);
    phy_stat1 = avmnet_mdio_read(this, PHY11G_PHYSTAT1);

    if(ctx->polarity_state == POL_STATE_RESET && !(phy_ctrl & MDIO_CONTROL_POWERDOWN)){
        AVMNET_INFO("[%s] PHY %s: in state RESET but not in Powerdown. Resetting polarity config.\n", __func__, this->name);

        phy_ctl1 &= ~(  PHY11G_PHYCTL1_POLA
                      | PHY11G_PHYCTL1_POLB
                      | PHY11G_PHYCTL1_POLC
                      | PHY11G_PHYCTL1_POLD
                      | PHY11G_PHYCTL1_MDICD
                      | PHY11G_PHYCTL1_MDIAB
                     );

        phy_ctl1 |= PHY11G_PHYCTL1_AMDIX;
        avmnet_mdio_write(this, PHY11G_PHYCTL1, phy_ctl1);

        avmnet_mdio_write(this, MDIO_CONTROL, phy_ctrl | MDIO_CONTROL_POWERDOWN);
        ctx->polarity_timer = jiffies;

        mdelay(5);
        phy_ctrl = avmnet_mdio_read(this, MDIO_CONTROL);
    }

    /*
     * check if the PHY is powered down to terminate an active link for
     * polarity correction. If so, make sure that 3 seconds have elapsed
     * before powering it back up again. This gives Windows a chance to
     * notice that the links is gone.
     */
    if(!ctx->powerdown
       && (ctx->polarity_state == POL_STATE_SET || ctx->polarity_state == POL_STATE_RESET)
       && (phy_ctrl & MDIO_CONTROL_POWERDOWN))
    {
        if(time_is_before_jiffies(ctx->polarity_timer + POL_PD_TIME)){
            AVMNET_SUPPORT("[%s] Getting PHY %s out of power-down from polarity correction.\n", __func__, this->name);
            avmnet_mdio_write(this, MDIO_CONTROL, (phy_ctrl & ~MDIO_CONTROL_POWERDOWN));
            mdelay(5);

            if(ctx->polarity_state == POL_STATE_RESET){
                ctx->polarity_state = POL_STATE_INIT;
            }

            // record time for link-up timeout
            ctx->polarity_timer = jiffies;
        }
        status.Details.powerup = 1;
    }

    if(!(phy_ctrl & MDIO_CONTROL_POWERDOWN)){
        status.Details.powerup = 1;

        /*
         * Check for SNR degradation
         */
        if((phy_status & MDIO_STATUS_LINK) && ctx->config.autoneg == AUTONEG_ENABLE){
            phy_perf = avmnet_mdio_read(this, PHY11G_PHYPERF);
            phy_perf &= PHY11G_PHYPERF_SNR;
            phy_perf >>= 4;

            if(time_is_before_eq_jiffies(ctx->snr_thrash_time + SNR_THRASH_TIME)){
                ctx->snr_thrash_counter = 0;
            }

            // check if SNR < 3
            if((phy_perf < SNR_BAD_LIMIT) && (ctx->snr_thrash_counter < SNR_THRASH_LIMIT)){
                // restart auto-negotiation if we have seen bad SNR values
                // BAD_SNR_TIME seconds in a row
                if(time_is_before_eq_jiffies(ctx->snr_bad_time + SNR_BAD_TIME)){
#if 0
                    AVMNET_SUPPORT("[%s] PHY %s: SNR degradation detected, restarting auto-negotiation.\n", __func__, this->name);
                    avmnet_mdio_write(this, MDIO_CONTROL, phy_ctrl | MDIO_CONTROL_AUTONEG_RESTART);
                    phy_status &= ~MDIO_STATUS_LINK;
#endif
                    ctx->snr_bad_time = jiffies;

                    if(ctx->snr_thrash_counter == 0){
                        ctx->snr_thrash_time = jiffies;
                    }
                    ctx->snr_thrash_counter++;

                    if(ctx->snr_thrash_counter == SNR_THRASH_LIMIT){
#if 0
                        AVMNET_SUPPORT("[%s] PHY %s: SNR thrash limit reached, pausing for %d seconds.\n", __func__, this->name, SNR_THRASH_TIME / HZ);
#endif
                        AVMNET_SUPPORT("[%s] PHY %s: SNR degradation detected.\n", __func__, this->name);
                    }
                }
            }else{
                // SNR is fine, reset the bad readings timer
                ctx->snr_bad_time = jiffies;
            }
        }

        phy_speed = avmnet_mdio_read(this, PHY11G_MIISTAT);

        /*
         * Check for link. If auto-negotiation is enabled, also check that
         * negotiation has finished
         */
        if(   (phy_status & MDIO_STATUS_LINK)
           && (   ctx->config.autoneg == AUTONEG_DISABLE
               || (phy_status & MDIO_STATUS_AUTONEG_COMPLETE)))
        {
            status.Details.link = 1;

            /*
             * Bits in 11GMIISTAT (0x18) :
             * 15-6     reserved
             * 5-4      Flow control: 0 none, 1 TX, 2 RX, 3 RXTX
             * 3        Duplex: 0 off, 1 on
             * 2        Resolved Energy efficient mode: 0 off, 1 on
             * 1-0      Speed: 0 10MBit, 1 100MBit, 2 1000MBit, 3 undefined/reserved 
             */
            if(phy_speed & PHY11G_MIISTAT_RX){
                status.Details.flowcontrol |= AVMNET_LINKSTATUS_FC_RX;
            }

            if( (phy_speed & PHY11G_MIISTAT_TX) && ( ctx->config.pause_setup != avmnet_pause_rx) ){

                // TX pause: there is no way to advertise RX-Only, so we advertise 
                // symetric and asymetric RX, and disable TX pause frames here
                status.Details.flowcontrol |= AVMNET_LINKSTATUS_FC_TX;
            }

            status.Details.fullduplex = (phy_speed & PHY11G_MIISTAT_DPX) ? 1 : 0;
            status.Details.speed = phy_speed & PHY11G_MIISTAT_SPEED;

            if(phy_stat1 & (PHY11G_PHYSTAT1_MDICD | PHY11G_PHYSTAT1_MDIAB)){
                status.Details.mdix = AVMNET_LINKSTATUS_MDI_MDIX;
            }else{
                status.Details.mdix = AVMNET_LINKSTATUS_MDI_MDI;
            }

            /*
             * fix issues with swapped signal lines
             */
            if(ctx->polarity_state == POL_STATE_INIT
               && (this->initdata.phy.flags & AVMNET_CONFIG_FLAG_POLARITY))
            {
                corr_needed = 0;
                polinv_state = 0;

                AVMNET_INFO("[%s] PHY %s: Checking if polarity correction is needed.\n", __func__, this->name);
                /*
                 * Build bit-mask of current link state to test against our
                 * configuration data
                 */
                switch(status.Details.speed){
                case 1:
                    polinv_state |= AVMNET_POLARITY_100MBIT;
                    break;
                case 2:
                    polinv_state |= AVMNET_POLARITY_1000MBIT;
                    break;
                case 0:
                default:
                    polinv_state |= AVMNET_POLARITY_10MBIT;
                    break;
                }

                if(status.Details.mdix == AVMNET_LINKSTATUS_MDI_MDIX){
                    polinv_state |= AVMNET_POLARITY_MDIX;
                }else{
                    polinv_state |= AVMNET_POLARITY_MDI;
                }

                /*
                 * clear out bits we might have to set
                 */
                phy_ctl1 &= ~(  PHY11G_PHYCTL1_POLA
                              | PHY11G_PHYCTL1_POLB
                              | PHY11G_PHYCTL1_POLC
                              | PHY11G_PHYCTL1_POLD
                              | PHY11G_PHYCTL1_MDICD
                              | PHY11G_PHYCTL1_MDIAB
                              | PHY11G_PHYCTL1_AMDIX
                             );

                /*
                 * check link state against configuration for each cable pair
                 */
                for(i = 0; i < 4; ++i){
                    polinv_conf = this->initdata.phy.polarity
                                    >> (i * AVMNET_POLARITY_SHIFT_WIDTH);

                    if((polinv_conf & polinv_state) == polinv_state){
                        corr_needed = 1;

                        AVMNET_SUPPORT("[%s] PHY %s: Need to correct polarity for pair %d.\n", __func__, this->name, i);
                        switch(i * AVMNET_POLARITY_SHIFT_WIDTH){
                        case AVMNET_POLARITY_A_SHIFT:
                            phy_ctl1 |= PHY11G_PHYCTL1_POLA;
                            break;
                        case AVMNET_POLARITY_B_SHIFT:
                            phy_ctl1 |= PHY11G_PHYCTL1_POLB;
                            break;
                        case AVMNET_POLARITY_C_SHIFT:
                            phy_ctl1 |= PHY11G_PHYCTL1_POLC;
                            break;
                        case AVMNET_POLARITY_D_SHIFT:
                            phy_ctl1 |= PHY11G_PHYCTL1_POLD;
                            break;
                        }

                        /*
                         * we have to make sure that the link comes back up
                         * with exactly the same MDI-X configuration we are
                         * seeing now. Disable Auto-MDIX and set MDIX manually
                         */
                        if(polinv_state & AVMNET_POLARITY_MDIX){
                            AVMNET_INFO("[%s] PHY %s: Forcing MDIX state on.\n", __func__, this->name);
                            phy_ctl1 |= (PHY11G_PHYCTL1_MDIAB | PHY11G_PHYCTL1_MDICD);
                        }else{
                            AVMNET_INFO("[%s] PHY %s: Forcing MDIX state off.\n", __func__, this->name);
                        }
                    }
                }

                if(corr_needed){
                    /*
                     * at least one of the pairs needs a polarity correction.
                     * Write the configuration to the register, terminate link
                     * and update our internal state.
                     */

                    AVMNET_INFO("[%s] PHY %s: Setting polarity correction and terminating link.\n", __func__, this->name);
                    avmnet_mdio_write(this, PHY11G_PHYCTL1, phy_ctl1);

                    // the link needs to be terminated. Send PHY into power-down
                    avmnet_mdio_write(this, MDIO_CONTROL, phy_ctrl | MDIO_CONTROL_POWERDOWN);

                    ctx->polarity_state = POL_STATE_SET;
                    ctx->polarity_timer = jiffies;

                    /*
                     * we do not want to report that we had a link
                     */
                    status.Status = 0;
                    status.Details.powerup = 1;

                } else {
                    AVMNET_INFO("[%s] PHY %s: No correction necessary.\n", __func__, this->name);
                    ctx->polarity_state = POL_STATE_UP;
                }

            } else if(ctx->polarity_state == POL_STATE_SET){
                /*
                 * Link is back up after polarity correction
                 */

                AVMNET_INFO("[%s] PHY %s: Link back up after correction.\n", __func__, this->name);
                ctx->polarity_state = POL_STATE_UP_COR;
            }else if(ctx->polarity_state == POL_STATE_RESET){
                AVMNET_INFO("[%s] PHY %s: in state RESET but not in Powerdown?!\n", __func__, this->name);
                avmnet_mdio_write(this, MDIO_CONTROL, phy_ctrl | MDIO_CONTROL_POWERDOWN);
                status.Status = 0;
                ctx->polarity_timer = jiffies;
            }
        }else{
            /*
             * No valid link detected
             */

            /*
             * make sure the bad SNR timer is not stale when the link comes back up again
             */
            ctx->snr_bad_time = jiffies;

            /*
             * check and update polarity correction state
             */
            switch(ctx->polarity_state){
            case POL_STATE_INIT:
                break;
            case POL_STATE_RESET:
                AVMNET_ERR("[%s] PHY %s: in state RESET but not in Powerdown?!\n", __func__, this->name);
                avmnet_mdio_write(this, MDIO_CONTROL, phy_ctrl | MDIO_CONTROL_POWERDOWN);
                status.Status = 0;
                ctx->polarity_timer = jiffies;
                break;
            case POL_STATE_SET:
                if(time_is_after_jiffies(ctx->polarity_timer + POL_TIMEOUT)){
                    break;
                }
                /* fall through */
            case POL_STATE_UP_COR:
                    /*
                     * polarity corrected link went down or link did not come back
                     * up after configuring the polarity correction. Reset PHY
                     * configuration and our internal state.
                     */

                    if(ctx->polarity_state == POL_STATE_UP_COR){
                        AVMNET_INFO("[%s] PHY %s: Link lost, resetting polarity correction\n", __func__, this->name);
                    }else if(ctx->polarity_state == POL_STATE_SET){
                        AVMNET_INFO("[%s] PHY %s: Timeout waiting after setting polarity correction.\n", __func__, this->name);
                    }

                    phy_ctl1 &= ~(  PHY11G_PHYCTL1_POLA
                                  | PHY11G_PHYCTL1_POLB
                                  | PHY11G_PHYCTL1_POLC
                                  | PHY11G_PHYCTL1_POLD
                                  | PHY11G_PHYCTL1_MDICD
                                  | PHY11G_PHYCTL1_MDIAB
                                 );

                    phy_ctl1 |= PHY11G_PHYCTL1_AMDIX;

                    avmnet_mdio_write(this, PHY11G_PHYCTL1, phy_ctl1);

                    // make sure that no link goes up before this settings take hold
                    avmnet_mdio_write(this, MDIO_CONTROL, phy_ctrl | MDIO_CONTROL_POWERDOWN);

                    ctx->polarity_state = POL_STATE_RESET;
                    ctx->polarity_timer = jiffies;

                    break;
            case POL_STATE_UP:
                ctx->polarity_state = POL_STATE_INIT;
                break;
            default:
                AVMNET_ERR("[%s] PHY %s: Unknown polarity state: 0x%x!\n", __func__, this->name, ctx->polarity_state);
                break;
            }
        }
    }

    this->unlock(this);
    return status;
}


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_phy_11G_poll(avmnet_module_t *this)
{
    int i, result;
    avmnet_linkstatus_t status;

    for(i = 0; i < this->num_children; ++i){
        result = this->children[i]->poll(this->children[i]);
        if(result < 0){
            AVMNET_WARN("Module %s: poll() failed on child %s\n", this->name, this->children[i]->name);
        }
    }

    status = get_status(this);

    result = this->parent->set_status(this->parent, this->device_id, status);
    if(result < 0){
        AVMNET_ERR("[%s] setting status for device %s failed.\n", __func__, this->device_id->device_name);
    }

    return result;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_phy_11G_set_status(avmnet_module_t *this, avmnet_device_t *device_id,
                                avmnet_linkstatus_t status)
{
    return this->parent->set_status(this->parent, device_id, status);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_phy_11G_powerdown(avmnet_module_t *this)
{
    unsigned int phyCtrl, phyCtl1;
    struct avmnet_phy_11G_context *ctx;

    ctx = (struct avmnet_phy_11G_context *) this->priv;

    if (this->lock(this)) {
        return -EINTR;
    }

    phyCtrl = avmnet_mdio_read(this, MDIO_CONTROL);
    phyCtl1 = avmnet_mdio_read(this, PHY11G_PHYCTL1);

    phyCtl1 &= ~(  PHY11G_PHYCTL1_POLA
                  | PHY11G_PHYCTL1_POLB
                  | PHY11G_PHYCTL1_POLC
                  | PHY11G_PHYCTL1_POLD
                  | PHY11G_PHYCTL1_MDICD
                  | PHY11G_PHYCTL1_MDIAB
                 );

    phyCtl1 |= PHY11G_PHYCTL1_AMDIX;
    avmnet_mdio_write(this, PHY11G_PHYCTL1, phyCtl1);

    ctx->powerdown = 1;
    if(ctx->polarity_state != POL_STATE_INIT && ctx->polarity_state != POL_STATE_UP){
        ctx->polarity_timer = jiffies;
        ctx->polarity_state = POL_STATE_RESET;
    }

    phyCtrl |= (MDIO_CONTROL_POWERDOWN | MDIO_CONTROL_ISOLATE);
    avmnet_mdio_write(this, MDIO_CONTROL, phyCtrl);

    this->unlock(this);
    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_phy_11G_powerup(avmnet_module_t *this)
{
    unsigned int phyCtrl;
    struct avmnet_phy_11G_context *ctx;

    ctx = (struct avmnet_phy_11G_context *) this->priv;

    if (this->lock(this)) {
        return -EINTR;
    }

    phyCtrl = avmnet_mdio_read(this, MDIO_CONTROL);

    if(phyCtrl & (MDIO_CONTROL_POWERDOWN | MDIO_CONTROL_ISOLATE)){
        phyCtrl &= ~(MDIO_CONTROL_POWERDOWN | MDIO_CONTROL_ISOLATE);

        avmnet_mdio_write(this, MDIO_CONTROL, phyCtrl);
    }

    ctx->powerdown = 0;

    this->unlock(this);
    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_phy_11G_suspend(avmnet_module_t *this __attribute__ ((unused)), avmnet_module_t *caller __attribute__ ((unused)))
{
    // TODO

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_phy_11G_resume(avmnet_module_t *this __attribute__ ((unused)), avmnet_module_t *caller __attribute__ ((unused)))
{
    // TODO

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_phy_11G_reinit(avmnet_module_t *this __attribute__ ((unused)))
{
    // TODO

    return 0;
}


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_phy_11G_ethtool_get_settings(avmnet_module_t *this, struct ethtool_cmd *cmd)
{
    avmnet_linkstatus_t status;
    unsigned int phyCtrl, phyAdv, phyGCtrl;

    status = get_status(this);

    cmd->supported = (SUPPORTED_10baseT_Half
                      | SUPPORTED_10baseT_Full
                      | SUPPORTED_100baseT_Half
                      | SUPPORTED_100baseT_Full
                      | SUPPORTED_Autoneg
                      | SUPPORTED_TP
                      | SUPPORTED_Asym_Pause
                      | SUPPORTED_Pause);

    if(this->initdata.phy.flags & AVMNET_CONFIG_FLAG_PHY_GBIT){
        cmd->supported |= (SUPPORTED_1000baseT_Half | SUPPORTED_1000baseT_Full);
    }

    cmd->advertising = 0;

    if (this->lock(this)) {
        return -EINTR;
    }

    phyCtrl = avmnet_mdio_read(this, MDIO_CONTROL);
    phyAdv = avmnet_mdio_read(this, MDIO_AUTONEG_ADV);
    phyGCtrl = avmnet_mdio_read(this, MDIO_MASTER_SLAVE_CTRL);

    this->unlock(this);

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

        if(this->initdata.phy.flags & AVMNET_CONFIG_FLAG_PHY_GBIT){
            if(phyGCtrl & MDIO_MASTER_SLAVE_CTRL_ADV_HD){
                cmd->advertising |= ADVERTISED_1000baseT_Half;
            }

            if(phyGCtrl & MDIO_MASTER_SLAVE_CTRL_ADV_FD){
                cmd->advertising |= ADVERTISED_1000baseT_Full;
            }
        }

        if(phyAdv & MDIO_AUTONEG_ADV_PAUSE_ASYM){
            cmd->advertising |= ADVERTISED_Asym_Pause;
        }

        if(phyAdv & MDIO_AUTONEG_ADV_PAUSE_SYM){
            cmd->advertising |= ADVERTISED_Pause;
        }

        cmd->advertising |= (ADVERTISED_TP | ADVERTISED_Autoneg);
    }else{
        cmd->autoneg = AUTONEG_DISABLE;
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
int avmnet_phy_11G_ethtool_set_settings(avmnet_module_t *this, struct ethtool_cmd *cmd)
{
    struct avmnet_phy_11G_context *ctx;

    ctx = (struct avmnet_phy_11G_context *) this->priv;

    // refuse GBit settings if not in GBit mode
    if(!(this->initdata.phy.flags & AVMNET_CONFIG_FLAG_PHY_GBIT)){
        if(cmd->autoneg == AUTONEG_ENABLE){
            if(cmd->advertising & (ADVERTISED_1000baseT_Half | ADVERTISED_1000baseT_Full)){
                return -EINVAL;
            }
        } else if(cmd->speed == SPEED_1000){
               return -EINVAL;
        }
    }

    down(&(ctx->mutex));

    if(cmd->autoneg == AUTONEG_ENABLE){
        // pause parameter advertising is controlled via ethtool_set_pauseparam()
        cmd->advertising &= ~(ADVERTISED_Pause | ADVERTISED_Asym_Pause);
        cmd->advertising |= (ctx->config.advertise & (ADVERTISED_Pause | ADVERTISED_Asym_Pause));
        ctx->config.advertise = cmd->advertising;
        ctx->config.autoneg = AUTONEG_ENABLE;
    }else{
        ctx->config.speed = cmd->speed;
        ctx->config.duplex = cmd->duplex;
        ctx->config.autoneg = AUTONEG_DISABLE;
    }
    AVMNET_INFO("[%s] set advertisement: (pause is done vial special func) pause=%d, asym_pause=%d\n", __func__,
            (ctx->config.advertise & ADVERTISED_Pause),
            (ctx->config.advertise & ADVERTISED_Asym_Pause) );

    up(&(ctx->mutex));

    return set_config(this);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static int set_config(avmnet_module_t *this)
{
    struct avmnet_phy_11G_context *ctx;
    unsigned int phyCtrl, phyAdv, phyGCtrl;

    ctx = (struct avmnet_phy_11G_context *) this->priv;

    if (this->lock(this)) {
        return -EINTR;
    }

    phyCtrl = avmnet_mdio_read(this, MDIO_CONTROL);
    phyCtrl &= ~(MDIO_CONTROL_RESET
                 | MDIO_CONTROL_AUTONEG_ENABLE
                 | MDIO_CONTROL_AUTONEG_RESTART
                 | MDIO_CONTROL_FULLDUPLEX
                 | MDIO_CONTROL_SPEEDSEL_MSB
                 | MDIO_CONTROL_SPEEDSEL_LSB);

    phyAdv = avmnet_mdio_read(this, MDIO_AUTONEG_ADV);
    phyAdv &= ~(MDIO_AUTONEG_ADV_SPEED_MASK
                | MDIO_AUTONEG_ADV_PAUSE_SYM
                | MDIO_AUTONEG_ADV_PAUSE_ASYM);

    phyGCtrl = avmnet_mdio_read(this, MDIO_MASTER_SLAVE_CTRL);
    phyGCtrl &= ~(MDIO_MASTER_SLAVE_CTRL_ADV_FD | MDIO_MASTER_SLAVE_CTRL_ADV_HD);

    if(ctx->config.advertise & ADVERTISED_10baseT_Half){
        phyAdv |= MDIO_AUTONEG_ADV_10BT_HDX;
    }
    if(ctx->config.advertise & ADVERTISED_10baseT_Full){
        phyAdv |= MDIO_AUTONEG_ADV_10BT_FDX;
    }
    if(ctx->config.advertise & ADVERTISED_100baseT_Half){
        phyAdv |= MDIO_AUTONEG_ADV_100BT_HDX;
    }
    if(ctx->config.advertise & ADVERTISED_100baseT_Full){
        phyAdv |= MDIO_AUTONEG_ADV_100BT_FDX;
    }

    if(this->initdata.phy.flags & AVMNET_CONFIG_FLAG_PHY_GBIT){
        if(ctx->config.advertise & ADVERTISED_1000baseT_Half){
            phyGCtrl |= MDIO_MASTER_SLAVE_CTRL_ADV_HD;
        }
        if(ctx->config.advertise & ADVERTISED_1000baseT_Full){
            phyGCtrl |= MDIO_MASTER_SLAVE_CTRL_ADV_FD;
        }
    }

    if(ctx->config.advertise & ADVERTISED_Asym_Pause){
        phyAdv |= MDIO_AUTONEG_ADV_PAUSE_ASYM;
    }
    if(ctx->config.advertise & ADVERTISED_Pause){
        phyAdv |= MDIO_AUTONEG_ADV_PAUSE_SYM;
    }

    switch(ctx->config.speed){
    case SPEED_10:
        // bits already cleared
        break;
    case SPEED_100:
        phyCtrl |= MDIO_CONTROL_SPEEDSEL_LSB;
        break;
    case SPEED_1000:
        if(this->initdata.phy.flags & AVMNET_CONFIG_FLAG_PHY_GBIT){
            phyCtrl |= MDIO_CONTROL_SPEEDSEL_MSB;
        } else {
            AVMNET_ERR("[%s] Can not set GBit speed, Phy %s is configured for FE.\n", __func__, this->name);
        }
        break;
    default:
        // default to 10MBit
        break;
    }

    if(ctx->config.duplex){
        phyCtrl |= MDIO_CONTROL_FULLDUPLEX;
    }

    if(ctx->config.autoneg == AUTONEG_ENABLE){
        phyCtrl |= MDIO_CONTROL_AUTONEG_ENABLE;
        phyCtrl |= MDIO_CONTROL_AUTONEG_RESTART;

        if(ctx->polarity_state != POL_STATE_INIT && ctx->polarity_state != POL_STATE_UP){
            ctx->polarity_timer = jiffies;
            ctx->polarity_state = POL_STATE_RESET;
        }
    }

//    AVMNET_ERR("[%s] phyCtrl: 0x%04x phyAdv: 0x%04x phyGCtrl: 0x%04x\n", __func__, phyCtrl, phyAdv, phyGCtrl);

    avmnet_mdio_write(this, MDIO_AUTONEG_ADV, phyAdv);
    avmnet_mdio_write(this, MDIO_MASTER_SLAVE_CTRL, phyGCtrl);
    avmnet_mdio_write(this, MDIO_CONTROL, phyCtrl);

    this->unlock(this);
    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
u32 avmnet_phy_11G_ethtool_get_link(avmnet_module_t *this)
{
    avmnet_linkstatus_t status;

    status = get_status(this);

    return status.Details.link;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void avmnet_phy_11G_ethtool_get_pauseparam(avmnet_module_t *this, struct ethtool_pauseparam *param)
{
    struct avmnet_phy_11G_context *ctx;
    avmnet_linkstatus_t status;

    ctx = (struct avmnet_phy_11G_context *) this->priv;

    status = get_status(this);

    if(ctx->config.autoneg){
        param->autoneg = AUTONEG_ENABLE;
    } else {
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
int avmnet_phy_11G_ethtool_set_pauseparam(avmnet_module_t *this, struct ethtool_pauseparam *param)
{
    struct avmnet_phy_11G_context *ctx;
    __u32 orig_adv, need_setting;
    enum avmnet_pause_setup orig_pause_setup;

    ctx = (struct avmnet_phy_11G_context *) this->priv;

    if(!param->autoneg && (param->rx_pause || param->tx_pause)){
        AVMNET_ERR("[%s] ethtool request to force set flow control parameters, but can only be auto-negotiated\n", __func__);
        return -EINVAL;
    }

    if (this->lock(this)) {
        return -EINTR;
    }

    orig_adv = ctx->config.advertise;
    orig_pause_setup = ctx->config.pause_setup;
    ctx->config.advertise &= ~(ADVERTISED_Asym_Pause | ADVERTISED_Pause);
    need_setting = 0;
    if(param->autoneg){

        ctx->config.pause_setup = avmnet_pause_none;

        // does our MAC support ASYM_PAUSE?
        if (this->parent->initdata.mac.flags & AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_ASYM){

            if(!param->rx_pause && param->tx_pause){
                ctx->config.pause_setup = avmnet_pause_tx;
            }
        
            if(param->rx_pause && !param->tx_pause){
                ctx->config.pause_setup = avmnet_pause_rx;
            }
        }

        // does our MAC support any PAUSE?
        if (this->parent->initdata.mac.flags & (AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_SYM | AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_ASYM)){

            if(param->rx_pause && param->tx_pause){
                ctx->config.pause_setup = avmnet_pause_rx_tx;
            }
        }

        // calculate our current advertisement
        ctx->config.advertise |= calc_advertise_pause_param(
            (this->parent->initdata.mac.flags &  AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_SYM),
            (this->parent->initdata.mac.flags &  AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_ASYM),
            ctx->config.pause_setup );


        if((ctx->config.advertise != orig_adv) || ( ctx->config.pause_setup != orig_pause_setup )){
            need_setting = 1;

            AVMNET_INFO("[%s] setup new pause advertisement: ethtool_param->rx_pause=%d, ethtool_param->tx_pause=%d, adv_pause=%d, adv_asym_pause=%d , pause_setup =%d\n",
                    __func__, param->rx_pause,param->tx_pause, (ctx->config.advertise & ADVERTISED_Pause), (ctx->config.advertise & ADVERTISED_Asym_Pause), ctx->config.pause_setup );
        }
    }

    this->unlock(this);

    if(need_setting){
        set_config(this);
    }

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static int phy_reg_show(struct seq_file *seq, void *data __attribute__ ((unused)) )
{
    int mdio_reg, mdio_addr;
    avmnet_module_t *this;

    this = (avmnet_module_t *) seq->private;

    mdio_addr = this->initdata.phy.mdio_addr;
    seq_printf(seq, "----------------------------------------\n");
    seq_printf(seq, "dumping regs for phy %s [mdio:%#x]\n", this->name, mdio_addr);

    if (this->lock(this)) {
        return -EINTR;
    }

    for(mdio_reg = 0; mdio_reg < 0x20; mdio_reg++){
        seq_printf(seq, "    reg[%#x]: %#x\n", mdio_reg, avmnet_mdio_read(this, mdio_reg));
    }

    this->unlock(this);

    seq_printf(seq, "----------------------------------------\n");
    return 0;
}

static int phy_reg_open(struct inode *inode, struct file *file)
{
    avmnet_module_t *this;

    this = (avmnet_module_t *) PDE_DATA(inode);

    return single_open(file, phy_reg_show, this);
}

static ssize_t phy_reg_write(struct file *filp, const char __user *buff,
                         size_t len, loff_t *offset __attribute__ ((unused)) )
{
    char mybuff[128];
    struct seq_file *seq;
    avmnet_module_t *this;
    unsigned int reg, value;

    if(len >= 128){
        return -ENOMEM;
    }

    seq = (struct seq_file *) filp->private_data;
    this = (avmnet_module_t *) seq->private;

    copy_from_user(&mybuff[0], buff, len);
    mybuff[len] = 0;

    if(sscanf(mybuff, "%u %u", &reg, &value) == 2){
        if (this->lock(this))
            return 0;
        avmnet_mdio_write(this, reg, value);
        this->unlock(this);
    }

    return len;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
#if defined(CONFIG_VR9) || defined(CONFIG_AR10)
static unsigned int vr9_get_phyRev(void) {

    unsigned int PHY_ID, chip_id, rev;

	PHY_ID = *IFX_MPS_CHIPID;
	chip_id = ( (PHY_ID & 0xFFF000) >> 12);

	switch(chip_id){
		case 0x1c0:
		case 0x1c1:
		case 0x1c2:
		case 0x1c8:
		case 0x1c9:
			rev = 1;
			break;
		default:
		    rev = 2;
		    break;
	}

    AVMNET_INFO("[%s] PHY revision %d (chip-ID: 0x%03x) found\n", __func__, rev, chip_id);
    AVMNET_TRC("[%s] Version 0x%x PHY_ID 0x%x\n", __func__, chip_id, PHY_ID);

    return rev;
}

/*
 * reset the VR9's internal PHYs
 */
static void reset_int_phy(avmnet_module_t *this, enum rst_type type)
{
    unsigned int rst_bit;
    unsigned int mdio_addr_reg, my_mdio;
    unsigned int gphy_id, valid, i;

    if(!(this->initdata.phy.flags & AVMNET_CONFIG_FLAG_INTERNAL)){
        AVMNET_ERR("[%s] Internal reset called for external PHY %s.\n", __func__, this->name);
        return;
    }

    /*
     * If GPHY block is split into two FE PHYs, we can not hard reset one without
     * affecting the other. In this case we just trigger a PHY reset via MDIO
     */
    if(!(this->initdata.phy.flags & AVMNET_CONFIG_FLAG_PHY_GBIT)){
        if(type == rst_on || type == rst_pulse){
            AVMNET_ERR("[%s] Can not hard reset split GPHY %s, doing MDIO reset\n", __func__, this->name);
            avmnet_mdio_write(this, MDIO_CONTROL, MDIO_CONTROL_RESET);
        }

        return;
    }

    /*
     * find out which internal GPHY block this MDIO address belongs to
     */
    mdio_addr_reg = SW_READ_REG32(GFMDIO_ADD);
    my_mdio = this->initdata.phy.mdio_addr;
    valid = 0;

    for(i = 0; i < NUM_GPHYS; ++i){
        if(((mdio_addr_reg & 0x1f) == my_mdio)
           || (mdio_addr_reg & 0x3e0) == (my_mdio << 5))
        {
            // TODO: Sonderbehandlung GPHY2 auf AR10, kann nicht gesplittet werden
            gphy_id = i;
            valid = 1;
            break;
        }

        mdio_addr_reg >>= 10;
    }

    if(!valid){
        AVMNET_ERR("[%s] No internal PHY with MDIO-address 0x%x found\n", __func__, my_mdio);
        return;
    }

    switch(gphy_id){
    case 0:
        rst_bit = IFX_RCU_RST_REQ_GPHY0;
        break;
    case 1:
        rst_bit = IFX_RCU_RST_REQ_GPHY1;
        break;
#if defined(CONFIG_AR10)
    case 2:
        rst_bit = IFX_RCU_RST_REQ_GPHY2;
        break;
#endif // CONFIG_AR10
    default:
        AVMNET_ERR("[%s] Could not determine GPHY block for PHY %s", __func__, this->name);
        return;
    }

    if(type == rst_on || type == rst_pulse){
        ifx_rcu_rst_req_write(rst_bit, rst_bit);
        mdelay(200);
    }

    if(type == rst_off || type == rst_pulse){
        ifx_rcu_rst_req_write(0, rst_bit);
        mdelay(200);
    }
}

static int load_firmware(avmnet_module_t *this)
{
    unsigned int fw_addr, fw_reg, rst_bit, old_fw_addr;
    unsigned int mdio_addr_reg, my_mdio;
    unsigned int gphy_id, valid, phy_rev, i;

    /*
     * find out which internal GPHY block this MDIO address belongs to
     */
    mdio_addr_reg = SW_READ_REG32(GFMDIO_ADD);
    my_mdio = this->initdata.phy.mdio_addr;
    valid = 0;

    for(i = 0; i < NUM_GPHYS; ++i){
        if(((mdio_addr_reg & 0x1f) == my_mdio)
           || (mdio_addr_reg & 0x3e0) == (my_mdio << 5))
        {
            // TODO: Sonderbehandlung GPHY2 auf AR10, kann nicht gesplittet werden
            gphy_id = i;
            valid = 1;
            break;
        }

        mdio_addr_reg >>= 10;
    }

    if(!valid){
        AVMNET_ERR("[%s] No internal PHY with MDIO-address 0x%x found\n", __func__, my_mdio);
        return -EINVAL;
    }

    /*
     * find out which firmware version to load into the GPHY
     */
    phy_rev = vr9_get_phyRev();
    switch(phy_rev){
    case 1:
        if(this->initdata.phy.flags & AVMNET_CONFIG_FLAG_PHY_GBIT){
            fw_addr = virt_to_phys(&vr9_phy11g_a1x_bin[0]);
            AVMNET_INFO("[%s] Loading GBit FW A1x\n", __func__);
        }else{
            fw_addr = virt_to_phys(&vr9_phy22f_a1x_bin[0]);
            AVMNET_INFO("[%s] Loading FE FW A1x\n", __func__);
        }
        break;
    default:
        AVMNET_ERR("[%s] Unknown PHY revision, trying FW for V2\n", __func__);
        /* fall through */
    case 2:
        if(this->initdata.phy.flags & AVMNET_CONFIG_FLAG_PHY_GBIT){
            fw_addr = virt_to_phys(&vr9_phy11g_a2x_bin[0]);
            AVMNET_INFO("[%s] Loading GBit FW A2x\n", __func__);
        }else{
            fw_addr = virt_to_phys(&vr9_phy22f_a2x_bin[0]);
            AVMNET_INFO("[%s] Loading FE FW A2x\n", __func__);
        }
        break;
    }

#if defined(DEBUG_11G_FIRWMWARE_LOAD)
    AVMNET_ERR("[%s] Pre: GP_strap:   0x%08x\n", __func__, SW_READ_REG32(IFX_RCU_GPIO_STRAP));
    AVMNET_ERR("[%s] Pre: GPHY0_CFG:  0x%08x\n", __func__, SW_READ_REG32(IFX_GPHY0_CFG));
    AVMNET_ERR("[%s] Pre: GPHY1_CFG:  0x%08x\n", __func__, SW_READ_REG32(IFX_GPHY1_CFG));
#if defined(CONFIG_AR10)
    AVMNET_ERR("[%s] Pre: GPHY2_CFG:  0x%08x\n", __func__, SW_READ_REG32(IFX_GPHY2_CFG));
#endif
    AVMNET_ERR("[%s] Pre: RST_REQ:    0x%08x\n", __func__, SW_READ_REG32(IFX_RCU_RST_REQ));
    AVMNET_ERR("[%s] Pre: RST_REQ2:   0x%08x\n", __func__, SW_READ_REG32(IFX_RCU_SLIC_USB_RST_REQ));
    AVMNET_ERR("[%s] Pre: FW-Addr:    0x%08x\n", __func__, fw_addr);
    AVMNET_ERR("[%s] Pre: GFMDIO_ADD: 0x%08x\n", __func__, SW_READ_REG32(GFMDIO_ADD));
    AVMNET_ERR("[%s] Pre: GFS_ADD0:   0x%08x\n", __func__, SW_READ_REG32(GFS_ADD0));
    AVMNET_ERR("[%s] Pre: GFS_ADD1:   0x%08x\n", __func__, SW_READ_REG32(GFS_ADD1));
#if defined(CONFIG_AR10)
    AVMNET_ERR("[%s] Pre: GFS_ADD2:   0x%08x\n", __func__, SW_READ_REG32(GFS_ADD2));
#endif
    /*--- AVMNET_ERR("[%s] Pre: FW     %p CRC32: %08x\n", __func__, &gphy_ge_fw_data[0], crc32_be(0, &gphy_ge_fw_data[0], sizeof(gphy_ge_fw_data))); ---*/
    /*--- AVMNET_ERR("[%s] Pre: FW-a21 %p CRC32: %08x\n", __func__, &gphy_ge_fw_data_a21[0], crc32_be(0, &gphy_ge_fw_data_a21[0], sizeof(gphy_ge_fw_data_a21))); ---*/
#endif // defined(DEBUG_11G_FIRWMWARE_LOAD)

    // check if FW is 14-bit aligned
    if((fw_addr & 0x3fff) != 0){
        AVMNET_ERR("[%s] Firmware not 16K aligned: %#x\n", __func__, fw_addr);
    }

    switch(gphy_id){
    case 0:
        GPHY0_PMU_SETUP(IFX_PMU_ENABLE);
        rst_bit = IFX_RCU_RST_REQ_GPHY0;
        fw_reg = GFS_ADD0;
        break;
    case 1:
        GPHY1_PMU_SETUP(IFX_PMU_ENABLE);
        rst_bit = IFX_RCU_RST_REQ_GPHY1;
        fw_reg = GFS_ADD1;
        break;
#if defined(CONFIG_AR10)
    case 2:
        GPHY2_PMU_SETUP(IFX_PMU_ENABLE);
        rst_bit = IFX_RCU_RST_REQ_GPHY2;
        fw_reg = GFS_ADD2;
        break;
#endif // CONFIG_AR10
    default:
        AVMNET_ERR("[%s] Refusing to load firmware into external PHY %s", __func__, this->name);
        return -EINVAL;
    }

    // if we are to load the FE firmware, check if it is already loaded
    // TODO: try to catch misconfigurations (same GPHY block configured for GB and FE)
    old_fw_addr = SW_READ_REG32(fw_reg);
    if(!(this->initdata.phy.flags & AVMNET_CONFIG_FLAG_PHY_GBIT) && old_fw_addr == fw_addr)    {
        AVMNET_ERR("[%s] %s: FE firmware already loaded into GPHY %d\n", __func__, this->name, gphy_id);
        return 0;
    }

    // set reset bit, load FW address into register and release reset again
    ifx_rcu_rst_req_write(rst_bit, rst_bit);
    SW_WRITE_REG32(fw_addr, fw_reg);
    mdelay(500);
    ifx_rcu_rst_req_write(0, rst_bit);
    mdelay(500);

#if defined(DEBUG_11G_FIRWMWARE_LOAD)
    AVMNET_ERR("[%s] Post: GP_strap:   0x%08x\n", __func__, SW_READ_REG32(IFX_RCU_GPIO_STRAP));
    AVMNET_ERR("[%s] Post: GPHY0_CFG:  0x%08x\n", __func__, SW_READ_REG32(IFX_GPHY0_CFG));
    AVMNET_ERR("[%s] Post: GPHY1_CFG:  0x%08x\n", __func__, SW_READ_REG32(IFX_GPHY1_CFG));
#if defined(CONFIG_AR10)
    AVMNET_ERR("[%s] Post: GPHY2_CFG:  0x%08x\n", __func__, SW_READ_REG32(IFX_GPHY2_CFG));
#endif
    AVMNET_ERR("[%s] Post: RST_REQ:    0x%08x\n", __func__, SW_READ_REG32(IFX_RCU_RST_REQ));
    AVMNET_ERR("[%s] Post: RST_REQ2:   0x%08x\n", __func__, SW_READ_REG32(IFX_RCU_SLIC_USB_RST_REQ));
    AVMNET_ERR("[%s] Post: GFMDIO_ADD: 0x%08x\n", __func__, SW_READ_REG32(GFMDIO_ADD));
    AVMNET_ERR("[%s] Post: GFS_ADD0:   0x%08x\n", __func__, SW_READ_REG32(GFS_ADD0));
    AVMNET_ERR("[%s] Post: GFS_ADD1:   0x%08x\n", __func__, SW_READ_REG32(GFS_ADD1));
#if defined(CONFIG_AR10)
    AVMNET_ERR("[%s] Post: GFS_ADD2:   0x%08x\n", __func__, SW_READ_REG32(GFS_ADD2));
#endif
#endif // defined(DEBUG_11G_FIRWMWARE_LOAD)

    return 0;
}

#endif // defined(CONFIG_VR9) || defined(CONFIG_AR10)

#ifdef INFINEON_CPU
static void reset_phy(avmnet_module_t *this, enum rst_type type){

#if defined(CONFIG_VR9) || defined(CONFIG_AR10)
    if(this->initdata.phy.flags & AVMNET_CONFIG_FLAG_INTERNAL){
        reset_int_phy(this, type);
        return;
    }
#endif

    if((this->initdata.phy.flags & AVMNET_CONFIG_FLAG_RESET) == 0){
        AVMNET_WARN("[%s] Reset not configured for module %s\n", __func__,  this->name);
        return;
    }

    if(type == rst_on || type == rst_pulse){
        ifx_gpio_output_clear(this->initdata.phy.reset, IFX_GPIO_MODULE_EXTPHY_RESET);
        AVMNET_INFO( "ifx_gpio_output_clear (%d, %d)\n", this->initdata.phy.reset, IFX_GPIO_MODULE_EXTPHY_RESET);
        mdelay(100);
    }

    if(type == rst_off || type == rst_pulse){
        ifx_gpio_output_set(this->initdata.phy.reset, IFX_GPIO_MODULE_EXTPHY_RESET);
        AVMNET_INFO( "ifx_gpio_output_set (%d, %d)\n", this->initdata.phy.reset, IFX_GPIO_MODULE_EXTPHY_RESET);
        mdelay(100);
    }
}
#elif defined(CONFIG_MACH_ATHEROS)
static void reset_phy(avmnet_module_t *this, enum rst_type type)
{

    if((this->initdata.phy.flags & AVMNET_CONFIG_FLAG_RESET) == 0){
        AVMNET_ERR("[%s] Reset not configured for module %s\n", __func__, this->name);
        return;
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
#endif // INFINEON_CPU
