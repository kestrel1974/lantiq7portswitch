/*------------------------------------------------------------------------------------------*\
 *   Copyright (C) 2011,2012 AVM GmbH <fritzbox_info@avm.de>
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

#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,4,46)
#include <atheros.h>
#include <atheros_gpio.h>
#else
#include <avm_atheros.h>
#include <avm_atheros_gpio.h>
#endif
#include <irq.h>

#include "avmnet_debug.h"
#include "avmnet_module.h"
#include "avmnet_config.h"
#include "avmnet_common.h"
#include "athrs_phy_ctrl.h"
#include "avmnet_ar8326.h"

#define MODULE_NAME "AVMNET_AR8326"
#define AR8326_MAX_PHYS 6

static struct resource ar8326_gpio = {
    .name  = "ar8326_reset",
    .flags = IORESOURCE_IO,		 
    .start = -1,
    .end   = -1
};

static int read_rmon_all_open(struct inode *inode, struct file *file);
static const struct file_operations read_rmon_all_fops = {
    .open    = read_rmon_all_open,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = single_release,
};
static int update_rmon_cache(avmnet_module_t *sw_mod);

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static void link_work_handler(struct work_struct *data)
{
    struct aths27_context *ctx;

    ctx = container_of(
            container_of(to_delayed_work(data), ar8326_work_t, work),
            struct aths27_context, link_work);

    ctx->this_module->poll(ctx->this_module);

    enable_irq(ctx->irq);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static irqreturn_t ar8326_link_intr(int irq __attribute__((unused)), void *dev) {

    avmnet_module_t *this = (avmnet_module_t *)dev;
    struct aths27_context *ctx = (struct aths27_context *)this->priv;

    AVMNET_DEBUG("[%s] irq %d\n", __func__, ctx->irq);
    disable_irq_nosync(ctx->irq);

    queue_delayed_work(ctx->link_work.workqueue, &(ctx->link_work.work), 0);

    return IRQ_HANDLED;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_s27_lock(avmnet_module_t *this) {
    return this->parent->lock(this->parent);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void avmnet_s27_unlock(avmnet_module_t *this) {
    this->parent->unlock(this->parent);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_s27_trylock(avmnet_module_t *this) {
    return this->parent->trylock(this->parent);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
unsigned int avmnet_athrs27_reg_read(avmnet_module_t *this, unsigned int s27_addr) {
    avmnet_module_t *parent = this->parent;
    unsigned int addr_temp;
    unsigned int s27_rd_csr_low, s27_rd_csr_high, s27_rd_csr;
    unsigned int data;
    unsigned int phy_address, reg_address;

    addr_temp = s27_addr >>2;
    data = addr_temp >> 7;

    phy_address = 0x1f;
    reg_address = 0x10;

    parent->reg_write(parent, phy_address, reg_address, data);

    phy_address = (0x17 & ((addr_temp >> 4) | 0x10));
    reg_address = ((addr_temp << 1) & 0x1e);
    s27_rd_csr_low = (uint32_t) parent->reg_read(parent, phy_address, reg_address);

    reg_address = reg_address | 0x1;
    s27_rd_csr_high = (uint32_t) parent->reg_read(parent, phy_address, reg_address);
    s27_rd_csr = (s27_rd_csr_high << 16) | s27_rd_csr_low ;

    return(s27_rd_csr);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_athrs27_reg_write(avmnet_module_t *this, unsigned int s27_addr, unsigned int s27_write_data) {
    avmnet_module_t *parent = this->parent;
    unsigned int addr_temp;
    unsigned int data;
    unsigned int phy_address, reg_address;

    addr_temp = (s27_addr ) >>2;
    data = addr_temp >> 7;

    phy_address = 0x1f;
    reg_address = 0x10;

    parent->reg_write(parent, phy_address, reg_address, data);

    phy_address = (0x17 & ((addr_temp >> 4) | 0x10));
    reg_address = (((addr_temp << 1) & 0x1e) | 0x1);
    data = (s27_write_data >> 16) & 0xffff;
    parent->reg_write(parent, phy_address, reg_address, data);

    reg_address = ((addr_temp << 1) & 0x1e);
    data = s27_write_data  & 0xffff;
    parent->reg_write(parent, phy_address, reg_address, data);

    return 0;
}


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static void avmnet_s27_busy_wait(avmnet_module_t *this,
                                 unsigned int phy_addr __attribute__ ((unused)),
                                 unsigned int reg_addr __attribute__ ((unused))) {
    unsigned int rddata, i = 100;

    /* Check MDIO_BUSY status */
    do {
        schedule();
        rddata = avmnet_athrs27_reg_read(this, S27_MDIO_CTRL_REG);
        rddata = rddata & S27_MDIO_BUSY;
    } while (rddata && --i);

    if(i == 0) {
        AVMNET_ERR("[%s] mdio access failed: phy:%d reg:%X\n", __func__, phy_addr, reg_addr);
    }
}


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
unsigned int avmnet_s27_rd_phy(avmnet_module_t *this, 
                               unsigned int phy_addr,
                               unsigned int reg_addr) {
    unsigned int rddata;

    /*--- printk(KERN_ERR "[%s] Phy 0x%x reg_addr 0x%x\n", __func__, phy_addr, reg_addr); ---*/

    /* MDIO_CMD is set for read */
    rddata =   0
             | (reg_addr << S27_MDIO_REG_ADDR) 
             | (phy_addr << S27_MDIO_PHY_ADDR) 
             | S27_MDIO_CMD_RD 
             | S27_MDIO_MASTER 
             | S27_MDIO_BUSY;

    avmnet_athrs27_reg_write(this, S27_MDIO_CTRL_REG, rddata);

    avmnet_s27_busy_wait(this, phy_addr, reg_addr);

    /* Read the data from phy */
    rddata = avmnet_athrs27_reg_read(this, S27_MDIO_CTRL_REG);
    rddata = rddata & 0xffff;

    return(rddata);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_s27_wr_phy(avmnet_module_t *this,
                      unsigned int phy_addr,
                      unsigned int reg_addr,
                      unsigned int write_data) {

    unsigned int rddata;

    /*--- printk(KERN_ERR "[%s] Phy 0x%x reg_addr 0x%x write_data 0x%x\n", __func__, phy_addr, reg_addr, write_data); ---*/

    /* MDIO_CMD is set for read */
    rddata = avmnet_athrs27_reg_read(this, S27_MDIO_CTRL_REG);
    rddata = (rddata & 0x0) | (write_data & 0xffff)
        | (reg_addr << S27_MDIO_REG_ADDR) 
        | (phy_addr << S27_MDIO_PHY_ADDR) | S27_MDIO_CMD_WR | S27_MDIO_MASTER | S27_MDIO_BUSY;

    avmnet_athrs27_reg_write(this, S27_MDIO_CTRL_REG, rddata);

    avmnet_s27_busy_wait(this, phy_addr, reg_addr);

    return 0;
}

/*------------------------------------------------------------------------------------------*\
 * der interne Switch im QCA956x hat sich etwas zickig, deshalb wird der Portstatus nicht
 * aus den PHYs gelesen, sondern aus den PORTSTATUS-Registern
\*------------------------------------------------------------------------------------------*/
#if defined(CONFIG_MACH_QCA956x) || defined(CONFIG_SOC_QCA956X) \
 || defined(CONFIG_MACH_QCA953x) || defined(CONFIG_SOC_QCA953X) \
 || defined(CONFIG_SOC_QCN550X)
int avmnet_ar8326_poll_intern(avmnet_module_t *this) {

    avmnet_linkstatus_t linkstatus;
    int i;
    
    if (this->lock(this)) {
        AVMNET_WARN("[%s] Interrupted while waiting for lock!\n", __func__);
        return -EINTR;
    }

    for (i = 0; i < this->num_children; i++) {
        union avmnet_module_initdata *initdata = &this->children[i]->initdata;
        unsigned int port = initdata->phy.mdio_addr + 1;
        unsigned int status = avmnet_athrs27_reg_read(this, S27_PORT_STATUS_REG(port));

        linkstatus.Status = 0;
        AVMNET_DEBUG("{%s} port %d status 0x%x\n", __func__, port, status);
        if (status & S27_PS_LINK)
            linkstatus.Details.link = 1;
        if (status & S27_PS_DUPLEX)
            linkstatus.Details.fullduplex = 1;
        if (status & S27_PS_TX_FLOW_EN)
            linkstatus.Details.flowcontrol |= AVMNET_LINKSTATUS_FC_TX;
        if (status & S27_PS_RX_FLOW_EN)
            linkstatus.Details.flowcontrol |= AVMNET_LINKSTATUS_FC_RX;
        linkstatus.Details.speed = status & 0x3;

        this->parent->set_status(this->parent, this->children[i]->device_id, linkstatus);
    }

    this->unlock(this);
    return 0;
}
#endif

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_ar8326_status_poll(avmnet_module_t *this) {
    int              i;
    uint32_t         IntStatus;

    update_rmon_cache(this);

    for (i = 0; i < this->num_children; i++) {
        this->children[i]->poll(this->children[i]);
    }

    if (this->lock(this)) {
        AVMNET_WARN("[%s] Interrupted while waiting for lock!\n", __func__);
        return -EINTR;
    }

    IntStatus = avmnet_athrs27_reg_read(this, S27_GLOBAL_INTR_REG);
    AVMNET_DEBUG("[%s] IntStatus 0x%x\n", __func__, IntStatus);

    avmnet_athrs27_reg_write(this, S27_GLOBAL_INTR_REG, IntStatus); /* clear global link interrupt */

    this->unlock(this);
    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_ar8326_set_status(avmnet_module_t *this, 
                             avmnet_device_t *device_id,
                             avmnet_linkstatus_t status) {
    AVMNET_DEBUG("[%s] %s Status %s changed actual %s speed %d\n", 
                __func__, device_id->device_name, this->name, status.Details.link ? "on":"off", status.Details.speed);

    this->parent->set_status(this->parent, device_id, status);
    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_ar8326_setup_interrupt(avmnet_module_t *this __attribute__((unused)), unsigned int on_off __attribute__((unused))) {

#if !(defined CONFIG_MACH_QCA953x || defined CONFIG_SOC_QCA953X ||  \
      defined CONFIG_MACH_QCA955x || defined CONFIG_SOC_QCA955X ||  \
      defined CONFIG_MACH_QCA956x || defined CONFIG_SOC_QCA956X)
    uint32_t    intr_reg_val;

    AVMNET_DEBUG("{%s} %sable PHY-Interrupt\n", __func__, on_off ? "en":"dis");

    if (this->initdata.swi.flags & AVMNET_CONFIG_FLAG_IRQ) {
        if (this->lock(this)) {
            AVMNET_WARN("[%s] Interrupted while waiting for lock!\n", __func__);
            return -EINTR;
        }

        if (on_off) {
            intr_reg_val = avmnet_athrs27_reg_read(this, S27_GLOBAL_INTR_REG);   /* clear global link interrupt */
            avmnet_athrs27_reg_write(this, S27_GLOBAL_INTR_REG, intr_reg_val);

            /* Enable global PHY link status interrupt */
            avmnet_athrs27_reg_write(this, S27_GLOBAL_INTR_MASK_REG, S27_GINT_PHY_INT);
        } else {
            /* disable global PHY link status interrupt */
            avmnet_athrs27_reg_write(this, S27_GLOBAL_INTR_MASK_REG, 0);
        }

        ath_reg_rmw_clear(RST_MISC_INTERRUPT_STATUS_ADDRESS, BIT(MISC_BIT_ENET_LINK)); /*--- also clear INT_STATUS-Register ---*/

        this->unlock(this);
        return 0;
    }

#endif /*--- #if !defined(CONFIG_MACH_QCA955x || 953x || 956x) ---*/
    return 1;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void avmnet_ar8326_status_changed(avmnet_module_t *this, avmnet_module_t *caller __attribute__ ((unused))) {

    avmnet_module_t *parent = this->parent;

    AVMNET_DEBUG("[%s] context_name %s\n", __func__, this->name);

    parent->status_changed(parent, this);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_ar8326_setup(avmnet_module_t *this) {

    struct aths27_context *ctx = (struct aths27_context *) this->priv;
    int    i, value, result;
    unsigned int port;

    AVMNET_DEBUG("[%s]\n", __func__);

    if(this->lock(this)) {
        AVMNET_WARN("[%s] Interrupted while waiting for lock!\n", __func__);
        return -EINTR;
    }

    value = avmnet_athrs27_reg_read(this, S27_MASK_CTL_REG);
    AVMNET_DEBUG("{%s} s27 resetting switch... 0x%x ", __func__, value);
    avmnet_athrs27_reg_write(this, S27_MASK_CTL_REG, value | S27_MASK_CTRL_RESET);

    i = 0x100;
    while(i--) {
        mdelay(100);
        if( ! (avmnet_athrs27_reg_read(this, S27_MASK_CTL_REG) & S27_MASK_CTRL_RESET))
            break;
    }

    AVMNET_DEBUG("done i %d ControlRegister 0x%x\n", i, avmnet_athrs27_reg_read(this, S27_MASK_CTL_REG));

    /* disable all ports. PHYs connected to MACs 1 to AR8326_MAX_PHYS */
    for (i = 1; i <= AR8326_MAX_PHYS; i++) {
        avmnet_athrs27_reg_write(this, S27_PORT_STATUS_REG(i), 0);
        avmnet_athrs27_reg_write(this, S27_PORT_CONTROL_REG(i), 0);
    }

    this->unlock(this);

    this->setup_irq(this, 0);   /*--- irq im Switch ausschalten ---*/

    for (i=0; i< this->num_children; i++) {
        result = this->children[i]->setup(this->children[i]);    /*--- die internen PHY initialisieren ---*/
        if (result != 0) {
            AVMNET_ERR("Module %s: init() failed on child %s\n", this->name, this->children[i]->name);
            return result;
        }
    }

    if (this->lock(this)) {
        goto lock_failed;
    }

    avmnet_athrs27_reg_write(this, S27_PORT_STATUS_REG(0), S27_PS_DUPLEX | S27_PS_RXMAC_EN | S27_PS_TXMAC_EN | S27_PS_SPEED_1000);

   /* Enable flow control on CPU port
    * XXX FIXME: S27 results in continous pause frames after few hours running high traffic if flow control is enabled.
    * This might need a  RTL fix, disabling by default as a WAR.
    */
    value = avmnet_athrs27_reg_read(this, S27_PORT_STATUS_REG(0));
    avmnet_athrs27_reg_write(this, S27_PORT_STATUS_REG(0), value | S27_PS_TX_FLOW_EN | S27_PS_RX_FLOW_EN | S27_PS_TXH_FLOW_EN);
  
    value = avmnet_athrs27_reg_read(this, S27_OPMODE_REG0);
    avmnet_athrs27_reg_write(this, S27_OPMODE_REG0, value | S27_MAC0_MAC_GMII_EN);  /* Set GMII mode */

    if (soc_is_ar934x()  || soc_is_qca956x()) {
        value = avmnet_athrs27_reg_read(this, S27_FLD_MASK_REG);
        // send unknown uni-, broad- and multicast frames to CPU port
        avmnet_athrs27_reg_write(this, S27_FLD_MASK_REG, value | ((1<<25) | (1<<16) | 0x1));
    } 

    value = avmnet_athrs27_reg_read(this, S27_GLOBAL_INTR_REG);   /* clear Interrupts */
    avmnet_athrs27_reg_write(this, S27_GLOBAL_INTR_REG, value);

    AVMNET_DEBUG("{%s} S27_GLOBAL_INTR_REG 0x%x\n", __func__, value);
    AVMNET_DEBUG("{%s} S27_GLOBAL_INTR_REG 0x%x\n", __func__, avmnet_athrs27_reg_read(this, S27_GLOBAL_INTR_REG));

    if (this->initdata.swi.flags & AVMNET_CONFIG_FLAG_IRQ) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,1,0)
        result = request_irq(ctx->irq, ar8326_link_intr, IRQF_DISABLED | IRQF_TRIGGER_LOW, "ar8326 link", this);
#else
        result = request_irq(ctx->irq, ar8326_link_intr, IRQF_TRIGGER_LOW, "ar8326 link", this);
#endif
        if (result < 0) {
            AVMNET_ERR("[%s] request irq ar8326 link %d failed\n", __func__, ctx->irq);
            this->unlock(this);
            return -EINTR;
        }
    }

    /* config CPU Port - disable Mirroring */
    avmnet_athrs27_reg_write(this, S27_CPU_PORT_REG,
                                   S27_CPU_PORT_REG_ENABLE |
                                   S27_CPU_PORT_REG_MIRROR_MASK(S27_CPU_PORT_REG_MIRROR_MASK_DISABLE)
                                   );

    avmnet_athrs27_reg_write(this, S27_PORT_CONTROL_REG(0),
                                   /*--- S27_EAPOL_EN | ---*/
                                   /*--- S27_ARP_LEAKY_EN | ---*/
                                   /*--- S27_IGMP_LEAVE_EN | ---*/
                                   /*--- S27_IGMP_JOIN_EN | ---*/
                                   /*--- S27_DHCP_EN | ---*/
                                   /*--- S27_IPG_DEC_EN | ---*/
                                   /*--- S27_ING_MIRROR_EN | ---*/
                                   /*--- S27_EG_MIRROR_EN | ---*/
                                   /*--- S27_LEARN_EN | ---*/
                                   /*--- S27_MAC_LOOP_BACK | ---*/
                                   S27_HEADER_EN |
                                   /*--- S27_IGMP_MLD_EN | ---*/
                                   S27_EG_VLAN_MODE( S27_VLAN_UNMODIFIED ) |
                                   /*--- S27_EG_VLAN_MODE( S27_VLAN_WITHOUT ) | ---*/
                                   /*--- S27_EG_VLAN_MODE( S27_VLAN_WITH ) | ---*/
                                   /*--- S27_LEARN_ONE_LOCK | ---*/
                                   /*--- S27_PORT_LOCK_EN | ---*/
                                   /*--- S27_PORT_DROP_EN | ---*/
                                   S27_PORT_MODE_FWD |
                                   0);


    /* enable external ports for which a config entry exists */
    for(i = 0; i < avmnet_hw_config_entry->nr_avm_devices; ++i){
        port = avmnet_hw_config_entry->avm_devices[i]->vlanID;

        // port numbers 1 to AR8326_MAX_PHYS are for SW MACs
        if(port > 0 && port <= AR8326_MAX_PHYS){

            AVMNET_DEBUG("{%s} set Port %d  StatusREG 0x%x\n", __func__, port, S27_PORT_STATUS_REG(port));

            if (this->initdata.swi.flags & AVMNET_CONFIG_FLAG_MAC_SUPPORT_FC_SYM) {
                avmnet_athrs27_reg_write(this, S27_PORT_STATUS_REG(port), S27_PORT_STATUS_DEFAULT | 
                                                                          S27_PS_RX_FLOW_EN | 
                                                                          S27_PS_TX_FLOW_EN);
            } else {
                avmnet_athrs27_reg_write(this, S27_PORT_STATUS_REG(port), S27_PORT_STATUS_DEFAULT);
            }

            /*--- enable VLAN ---*/
            avmnet_athrs27_reg_write(this, S27_PORT_CONTROL_REG(port),
                                       /*--- S27_EAPOL_EN | ---*/
                                       /*--- S27_ARP_LEAKY_EN | ---*/
                                       /*--- S27_IGMP_LEAVE_EN | ---*/
                                       /*--- S27_IGMP_JOIN_EN | ---*/
                                       /*--- S27_DHCP_EN | ---*/
                                       /*--- S27_IPG_DEC_EN | ---*/
                                       /*--- S27_ING_MIRROR_EN | ---*/
                                       /*--- S27_EG_MIRROT_EN | ---*/
                                       /*--- S27_LEARN_EN | ---*/
                                       /*--- S27_MAC_LOOP_BACK | ---*/
                                       /*--- S27_HEADER_EN | ---*/
                                       /*--- S27_IGMP_MLD_EN | ---*/
                                       /*--- S27_EG_VLAN_MODE( S27_VLAN_UNMODIFIED ) | ---*/
                                       S27_EG_VLAN_MODE( S27_VLAN_WITHOUT ) |
                                       /*--- S27_EG_VLAN_MODE( S27_VLAN_WITH ) | ---*/
                                       /*--- S27_LEARN_ONE_LOCK | ---*/
                                       S27_PORT_LOCK_EN |
                                       /*--- S27_PORT_DROP_EN | ---*/
                                       S27_PORT_MODE_FWD |
                                       0);
        }
    }

    /* clear MAC table */
    avmnet_athrs27_reg_write(this, S27_VLAN_TABLE_REG0,
                                   S27_VLAN_TABLE_REG0_VLAN_OP(S27_VLAN_TABLE_REG0_VLAN_OP_FLUSH) |
                                   S27_VLAN_TABLE_REG0_BUSY
                                   );

    /* Enable MIB counters */
    avmnet_athrs27_reg_write(this, S27_MIB_FUNCTION, S27_MIB_FUNCTION_MIB_EN);

    /* Tag Priority Mapping */
    avmnet_athrs27_reg_write(this, S27_TAG_PRI_MAP_REG, S27_TAGPRI_DEFAULT);

    /* Enable Broadcast packets to CPU port */
    avmnet_athrs27_reg_write(this, S27_FLD_MASK_REG, 
                                    S27_UNI_FLOOD_DP(0x7f) |
                                    S27_IGMP_JOIN_LEAVE_DP(0x7f) |
                                    S27_MULTI_FLOOD_DP(0x7f) |
                                    /*--- S27_ARL_MULTI_LEAKY_EN | ---*/
                                    /*--- S27_ARL_UNI_LEAKY_EN | ---*/
                                    S27_BROAD_DP(0x7f) |
                                    0);
            
    /* Enable ARP packets to CPU port */
    value = avmnet_athrs27_reg_read(this, S27_ARL_TBL_CTRL_REG);
    avmnet_athrs27_reg_write(this, S27_ARL_TBL_CTRL_REG, value | (1<<20));

    AVMNET_DEBUG("S27_CPU_PORT_REGISTER  :%x\n", avmnet_athrs27_reg_read (this,  S27_CPU_PORT_REG ));
    AVMNET_DEBUG("PORT_STATUS_REG(0)  :%x\n", avmnet_athrs27_reg_read (this,  S27_PORT_STATUS_REG(0) ));
    AVMNET_DEBUG("PORT_STATUS_REG(1)  :%x\n", avmnet_athrs27_reg_read (this,  S27_PORT_STATUS_REG(1) ));
    AVMNET_DEBUG("PORT_STATUS_REG(2)  :%x\n", avmnet_athrs27_reg_read (this,  S27_PORT_STATUS_REG(2) ));
    AVMNET_DEBUG("PORT_STATUS_REG(3)  :%x\n", avmnet_athrs27_reg_read (this,  S27_PORT_STATUS_REG(3) ));
    AVMNET_DEBUG("PORT_STATUS_REG(4)  :%x\n", avmnet_athrs27_reg_read (this,  S27_PORT_STATUS_REG(4) ));
    AVMNET_DEBUG("PORT_STATUS_REG(5)  :%x\n", avmnet_athrs27_reg_read (this,  S27_PORT_STATUS_REG(5) ));

    AVMNET_DEBUG("PORT_CONTROL_REG(0) :%x\n", avmnet_athrs27_reg_read (this,  S27_PORT_CONTROL_REG(0) ));
    AVMNET_DEBUG("PORT_CONTROL_REG(1) :%x\n", avmnet_athrs27_reg_read (this,  S27_PORT_CONTROL_REG(1) ));
    AVMNET_DEBUG("PORT_CONTROL_REG(2) :%x\n", avmnet_athrs27_reg_read (this,  S27_PORT_CONTROL_REG(2) ));
    AVMNET_DEBUG("PORT_CONTROL_REG(3) :%x\n", avmnet_athrs27_reg_read (this,  S27_PORT_CONTROL_REG(3) ));
    AVMNET_DEBUG("PORT_CONTROL_REG(4) :%x\n", avmnet_athrs27_reg_read (this,  S27_PORT_CONTROL_REG(4) ));
    AVMNET_DEBUG("PORT_CONTROL_REG(5) :%x\n", avmnet_athrs27_reg_read (this,  S27_PORT_CONTROL_REG(5) ));

    AVMNET_DEBUG("PORT_BASE_VLAN_REG(0) :%x\n", avmnet_athrs27_reg_read (this,  S27_PORT_BASE_VLAN_REG(0) ));
    AVMNET_DEBUG("PORT_BASE_VLAN_REG(1) :%x\n", avmnet_athrs27_reg_read (this,  S27_PORT_BASE_VLAN_REG(1) ));
    AVMNET_DEBUG("PORT_BASE_VLAN_REG(2) :%x\n", avmnet_athrs27_reg_read (this,  S27_PORT_BASE_VLAN_REG(2) ));
    AVMNET_DEBUG("PORT_BASE_VLAN_REG(3) :%x\n", avmnet_athrs27_reg_read (this,  S27_PORT_BASE_VLAN_REG(3) ));
    AVMNET_DEBUG("PORT_BASE_VLAN_REG(4) :%x\n", avmnet_athrs27_reg_read (this,  S27_PORT_BASE_VLAN_REG(4) ));
    AVMNET_DEBUG("PORT_BASE_VLAN_REG(5) :%x\n", avmnet_athrs27_reg_read (this,  S27_PORT_BASE_VLAN_REG(5) ));

    AVMNET_DEBUG("PORT_BASE_VLAN2_REG(0) :%x\n", avmnet_athrs27_reg_read (this,  S27_PORT_BASE_VLAN2_REG(0) ));
    AVMNET_DEBUG("PORT_BASE_VLAN2_REG(1) :%x\n", avmnet_athrs27_reg_read (this,  S27_PORT_BASE_VLAN2_REG(1) ));
    AVMNET_DEBUG("PORT_BASE_VLAN2_REG(2) :%x\n", avmnet_athrs27_reg_read (this,  S27_PORT_BASE_VLAN2_REG(2) ));
    AVMNET_DEBUG("PORT_BASE_VLAN2_REG(3) :%x\n", avmnet_athrs27_reg_read (this,  S27_PORT_BASE_VLAN2_REG(3) ));
    AVMNET_DEBUG("PORT_BASE_VLAN2_REG(4) :%x\n", avmnet_athrs27_reg_read (this,  S27_PORT_BASE_VLAN2_REG(4) ));
    AVMNET_DEBUG("PORT_BASE_VLAN2_REG(5) :%x\n", avmnet_athrs27_reg_read (this,  S27_PORT_BASE_VLAN2_REG(5) ));

#if 0
    /* Change HOL settings 
     * 25: PORT_QUEUE_CTRL_ENABLE.
     * 24: PRI_QUEUE_CTRL_EN.
     */
    for (i=0; i< this->num_children; i++) {
        uint32_t queue_ctrl_reg = (0x100 * i) + 0x218 ;
        avmnet_athrs27_reg_write(this, queue_ctrl_reg, 0x032b5555);
    }

    /* If s27 is is switch only mode, clear the PHY4_MII enable bit */
    if (mac_has_flag(mac,ETH_SWONLY_MODE)) {
        avmnet_athrs27_reg_write(this, S27_OPMODE_REG1, 0x0);
        avmnet_athrs27_reg_write(this, S27_PORT_STATUS_REGISTER5, S27_PORT_STATUS_DEFAULT);
    }
#endif
    this->unlock(this);

    this->setup_irq(this, 1);   /*--- hier den IRQ immer einschalten ---*/
    return 0;
 
lock_failed:

    if (this->initdata.swi.flags & AVMNET_CONFIG_FLAG_IRQ) {
        free_irq(ctx->irq, ar8326_link_intr);
    }

    for (i=0; i< this->num_children; i++) {
         this->children[i]->exit(this->children[i]);
    }

    return -EINTR;
}


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
                value = avmnet_athrs27_reg_read(this, reg);
                AVMNET_SUPPORT("register_read reg %#x = %#x\n", reg, value);
            }
            break;
        case 3:
            if ( ! strncasecmp(token[0], "write", 5)) {
                sscanf(token[1], "%x", &reg);
                sscanf(token[2], "%x", &value);
                avmnet_athrs27_reg_write(this, reg, value);
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

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_ar8326_init(avmnet_module_t *this) {

    struct aths27_context *ctx;
    int i, result;

    AVMNET_DEBUG("[%s] Init on module %s called.\n", __func__, this->name);

    ctx = kzalloc(sizeof(struct aths27_context) + (this->num_children * sizeof(avmnet_linkstatus_t)), GFP_KERNEL);
    if(ctx == NULL){
        AVMNET_ERR("[%s] init of avmnet-module %s failed.\n", __func__, this->name);
        return -ENOMEM;
    }

    this->priv = ctx;
    ctx->this_module = this;

    if (this->initdata.swi.flags & AVMNET_CONFIG_FLAG_RESET) {

        if(this->initdata.swi.reset < 100) {
            /*--- Hängt nicht am Shift-Register => GPIO Resourcen anmelden ---*/
            ar8326_gpio.start = this->initdata.swi.reset;
            ar8326_gpio.end = this->initdata.swi.reset;
            if(request_resource(&gpio_resource, &ar8326_gpio)) {
                AVMNET_ERR("[%s] ERROR request resource gpio %d\n", __func__, this->initdata.swi.reset);
                kfree(ctx);
                return -EIO;
            }
        }

    }

    if (this->initdata.swi.flags & AVMNET_CONFIG_FLAG_IRQ) {
        ctx->irq = this->initdata.swi.irq;
    }

    ctx->link_work.workqueue = avmnet_get_workqueue();
    if(ctx->link_work.workqueue == NULL){
        AVMNET_ERR("[%s] No global workqueue available!\n>", __FUNCTION__);
        kfree(ctx);
        return -EFAULT;
    }
    INIT_DELAYED_WORK(&(ctx->link_work.work), link_work_handler);

    AVMNET_INFO("{%s} reset %d irq %d\n", __func__, this->initdata.swi.reset, ctx->irq);

    for(i = 0; i < this->num_children; ++i){
        result = this->children[i]->init(this->children[i]);
        if(result < 0){
            return result; // handle error
        }
    }

    avmnet_cfg_register_module(this);
    avmnet_cfg_add_seq_procentry(this, "rmon_all", &read_rmon_all_fops);
    avmnet_cfg_add_procentry(this, "switch_reg", &avmnet_proc_switch_fops);

    return 0;
}
 
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_ar8326_exit(avmnet_module_t *this) {

    struct aths27_context *ctx = (struct aths27_context *)this->priv;
    int i, result;

    AVMNET_DEBUG("[%s] Exit on module %s called.\n", __func__, this->name);

    for(i = 0; i < this->num_children; ++i){
        result = this->children[i]->exit(this->children[i]);
        if(result != 0){
            // handle error
        }
    }

    if (this->initdata.swi.flags & AVMNET_CONFIG_FLAG_IRQ) {
        free_irq(ctx->irq, ar8326_link_intr);
    }

    if (this->initdata.swi.flags & AVMNET_CONFIG_FLAG_RESET) {
        if(this->initdata.swi.reset < 100) {
            release_resource(&ar8326_gpio);
        }
    }

    kfree(this->priv);

    return 0;
}

static int update_rmon_cache(avmnet_module_t *sw_mod)
{
	size_t reg;
	size_t port;
    	struct aths27_context *ctx = (struct aths27_context *)sw_mod->priv;

	if(sw_mod->lock(sw_mod)) {
		AVMNET_WARN("[%s] Interrupted while waiting for lock!\n", __func__);
        	return -EINTR;
	}

	avmnet_athrs27_reg_write(sw_mod, S27_MIB_FUNCTION, S27_MIB_FUNCTION_MIB_EN);
	for (reg=0; reg < AR8326_RMON_COUNTER; reg++){
		for (port=0; port < AR8326_PORTS; port++){
			ctx->rmon_cache[port][reg] += avmnet_athrs27_reg_read(sw_mod, 0x20000+ (port * 0x100) + (reg*4));
		}
	}
	sw_mod->unlock(sw_mod);
	return 0;
}


static inline u32 read_rmon_reg_cached(avmnet_module_t *sw_mod, u32 sw_port, u32 reg)
{
    	struct aths27_context *ctx = (struct aths27_context *)sw_mod->priv;
	return ctx->rmon_cache[sw_port][reg];
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static int read_rmon_all_show(struct seq_file *seq, void *data __attribute__ ((unused)) ) {
    char *cnt8316names[] = {
              /*  0 */    "RxBroad",
              /*  1 */    "RxPause",
              /*  2 */    "RxMulti",
              /*  3 */    "RxFcsErr",
              /*  4 */    "RxAlignErr",
              /*  5 */    "RxRunt",
              /*  6 */    "RxFragment",
              /*  7 */    "Rx64Byte",
              /*  8 */    "Rx128Byte",
              /*  9 */    "Rx256Byte",
              /* 10 */    "Rx512Byte",
              /* 11 */    "Rx1024Byte",
              /* 12 */    "Rx1518Byte",
              /* 13 */    "RxMaxByte",
              /* 14 */    "RxTooLong",
              /* 15 */    "RxGoodByteLow",
              /* 16 */    "RxGoodByteHigh",
              /* 17 */    "RxBadByteLow",
              /* 18 */    "RxBadByteHigh",
              /* 19 */    "RxOverflow",
              /* 20 */    "Filtered",
              /* 21 */    "TxBroad",
              /* 22 */    "TxPause",
              /* 23 */    "TxMulti",
              /* 24 */    "TxUnderRun",
              /* 25 */    "Tx64Byte",
              /* 26 */    "Tx128Byte",
              /* 27 */    "Tx256Byte",
              /* 28 */    "Tx512Byte",
              /* 29 */    "Tx1024Byte",
              /* 30 */    "Tx1518Byte",
              /* 31 */    "TxMaxByte",
              /* 32 */    "TxOverSize",
              /* 33 */    "TxByteLow",
              /* 34 */    "TxByteHigh",
              /* 35 */    "TxCollision",
              /* 36 */    "TxAbortCol",
              /* 37 */    "TxMultiCol",
              /* 38 */    "TxSingleCol",
              /* 39 */    "TxExcDefer",
              /* 40 */    "TxDefer",
              /* 41 */    "TxLateCol"
                           };
    unsigned int i;
    avmnet_module_t *this;

    this = (avmnet_module_t *) seq->private;

    if (update_rmon_cache(this))
	    return -EINTR;

    seq_printf(seq, "Counters\n");
    seq_printf(seq, "                    CPU port      Port 1      Port 2      Port 3      Port 4      Port 5\n");
    for(i = 0; i < AR8326_RMON_COUNTER; i ++) {
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
static int read_rmon_all_open(struct inode *inode, struct file *file) 
{

    return single_open(file, read_rmon_all_show, PDE_DATA(inode));
}

static inline u64 con32(u32 high, u32 low)
{
	return ((u64)high << 32) | (u64)low;
}

struct avmnet_hw_rmon_counter *create_ar8326_rmon_cnt(avmnet_device_t *avm_dev)
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
