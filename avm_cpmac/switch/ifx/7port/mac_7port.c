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

#include <avmnet_debug.h>
#include <avmnet_module.h>
#include <avmnet_config.h>
#include <asm/delay.h>
#include <linux/init.h>
#include <linux/list.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/prom.h>

#include <ifx_types.h>
#include <ifx_regs.h>
#include <common_routines.h>
#include <ifx_pmu.h>
#include <ifx_rcu.h>
#include <ifx_dma_core.h>
#include "mac_7port.h"
#include "swi_7port_reg.h"

static void proc_registers_output(struct seq_file *seq, void *data);
static void proc_mac_control_output(struct seq_file *seq, void *data);
static int proc_mac_control_input(char *input ,void *priv_data);

static void setup_mac_speed(avmnet_module_t *this, avmnet_linkstatus_t status);

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_mac_vr9_init(avmnet_module_t *this)
{
    struct avmnet_mac_vr9_context *my_ctx;
    int i, result;

    AVMNET_INFO("[%s] Init on module %s called.\n", __func__, this->name);

    my_ctx = kzalloc(sizeof(struct avmnet_mac_vr9_context), GFP_KERNEL);
    if(my_ctx == NULL){
        AVMNET_ERR("[%s] init of avmnet-module %s failed.\n", __func__, this->name);
        return -ENOMEM;
    }

    this->priv = my_ctx;

    for(i = 0; i < this->num_children; ++i){
        result = this->children[i]->init(this->children[i]);
        if(result != 0){
            AVMNET_ERR("Module %s: init() failed on child %s\n", this->name, this->children[i]->name);
            return result;
        }
    }

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_mac_vr9_setup(avmnet_module_t *this)
{
    int i, result, mac_nr, mode;
    unsigned int cfg_reg, phy_reg, mac_ctrl_0, pcdu, valid;

    AVMNET_INFO("[%s] Setup on module %s called.\n", __func__, this->name);

    /*
     * do cunning setup-stuff
     */

    if (this->lock(this)) {
        AVMNET_ERR("[%s] Interrupted while waiting for lock.\n", __func__);
        return -EINTR;
    }

    mac_nr = this->initdata.mac.mac_nr;
    mode = this->initdata.mac.mac_mode;
    cfg_reg = SW_READ_REG32(MII_CFG_REG(mac_nr));
    AVMNET_INFO("[%s] Setup on mac nr %d called.\n", __func__, mac_nr);
    AVMNET_INFO("[%s] MII_CFG_REG(%#x) = %#x (vor reset)\n", __func__, (unsigned int) MII_CFG_REG(mac_nr), cfg_reg);

    phy_reg = SW_READ_REG32(PHY_ADDR(mac_nr));
    AVMNET_INFO("[%s] PHY_ADDR(%#x) = %#x (vor reset)\n", __func__, (unsigned int) PHY_ADDR(mac_nr), phy_reg);

    mac_ctrl_0 = SW_READ_REG32(MAC_CTRL(mac_nr, 0));
    AVMNET_INFO("[%s] MAC_CTRL(%#x) = %#x (vor reset)\n", __func__, (unsigned int) MAC_CTRL(mac_nr, 0), mac_ctrl_0);

    // reset the MAC port
    //
    cfg_reg |= MII_CFG_RES;
    SW_WRITE_REG32(cfg_reg, MII_CFG_REG(mac_nr));
    udelay(100);

    cfg_reg = SW_READ_REG32(MII_CFG_REG(mac_nr));
    AVMNET_INFO("[%s] MII_CFG_REG(%#x) = %#x (nach reset)\n", __func__, (unsigned int) MII_CFG_REG(mac_nr), cfg_reg);

    phy_reg = SW_READ_REG32(PHY_ADDR(mac_nr));
    AVMNET_INFO("[%s] PHY_ADDR(%#x) = %#x (nach reset)\n", __func__, (unsigned int) PHY_ADDR(mac_nr), phy_reg);

    mac_ctrl_0 = SW_READ_REG32(MAC_CTRL(mac_nr, 0));
    AVMNET_INFO("[%s] MAC_CTRL(%#x) = %#x (nach reset)\n", __func__, (unsigned int) MAC_CTRL(mac_nr, 0 ), mac_ctrl_0);

    mac_ctrl_0 &= ~(MAC_CTRL_0_FCON_MASK | MAC_CTRL_0_FDUP_MASK | MAC_CTRL_0_GMII_MASK);
    phy_reg &= ~(PHY_ADDR_LINKST_MASK | PHY_ADDR_SPEED_MASK | PHY_ADDR_FDUP_MASK | PHY_ADDR_ADDR_MASK);
    cfg_reg &= ~(MII_CFG_MIIRATE_MASK | MII_CFG_MIIMODE_MASK | MII_CFG_EN);

    // Disable Isolate & Link_down_clock_disable
    cfg_reg &= ~( MII_CFG_ISOLATE | MII_CFG_LNK_DWN_CLK_DIS );

    AVMNET_INFO("[%s] mac_nr = %d, mode =%d \n", __func__, mac_nr, mode);
    valid = 1;

    switch(mac_nr){
    case 0:
#if !defined(CONFIG_AR10)
    case 1:
#endif
        switch(mode){
        case MAC_MODE_AUTO:
            cfg_reg |= (MII_CFG_MIIRATE_AUTO | MII_CFG_EN | MII_CFG_MIIMODE_RGMII);
            mac_ctrl_0 |= (MAC_CTRL_0_FCON_AUTO | MAC_CTRL_0_FDUP_AUTO | MAC_CTRL_0_GMII_AUTO);
            phy_reg = 0;
            phy_reg |= (PHY_ADDR_LINKST_AUTO | PHY_ADDR_SPEED_AUTO | PHY_ADDR_FDUP_AUTO
                      | PHY_ADDR_FCONTX_AUTO | PHY_ADDR_FCONRX_AUTO);

            // FIXME: Zugriff auf fremde Moduldaten, sollte Config-Parameter im MAC sein
            phy_reg |= this->children[0]->initdata.phy.mdio_addr;
            break;
        case MAC_MODE_MII:
            cfg_reg |= (MII_CFG_MIIMODE_MIIM | MII_CFG_MIIRATE_25MHZ | MII_CFG_EN);
            mac_ctrl_0 |= MAC_CTRL_0_GMII_MII;
            phy_reg |= PHY_ADDR_SPEED_100;
            break;
        case MAC_MODE_RMII:
            cfg_reg |= (MII_CFG_MIIMODE_RMIIM | MII_CFG_MIIRATE_50MHZ | MII_CFG_EN);
            mac_ctrl_0 |= MAC_CTRL_0_GMII_MII;
            phy_reg |= PHY_ADDR_SPEED_100;
            break;
        case MAC_MODE_RGMII_100:
            cfg_reg |= (MII_CFG_MIIMODE_RGMII | MII_CFG_MIIRATE_25MHZ | MII_CFG_EN);
            mac_ctrl_0 |= MAC_CTRL_0_GMII_RGMII;
            phy_reg |= PHY_ADDR_SPEED_100;
            break;
        case MAC_MODE_RGMII_1000:
            cfg_reg |= (MII_CFG_MIIMODE_RGMII | MII_CFG_MIIRATE_125MHZ | MII_CFG_EN);
            mac_ctrl_0 |= MAC_CTRL_0_GMII_RGMII;
            phy_reg |= PHY_ADDR_SPEED_1000;
            break;
        default:
            AVMNET_ERR("[%s] Invalid interface mode 0x%x for MAC %d.\n", __func__, mode, mac_nr);
            valid = 0;
            break;
        }
        break;
#if defined(CONFIG_AR10)
    case 1:
#endif
    case 2:
    case 4:
        if(mode == MAC_MODE_AUTO){
            cfg_reg |= (MII_CFG_MIIRATE_AUTO | MII_CFG_EN | MII_CFG_MIIMODE_RGMII);
            mac_ctrl_0 |= (MAC_CTRL_0_FCON_AUTO | MAC_CTRL_0_FDUP_AUTO | MAC_CTRL_0_GMII_AUTO);
            phy_reg = 0;
            phy_reg |= (PHY_ADDR_LINKST_AUTO | PHY_ADDR_SPEED_AUTO | PHY_ADDR_FDUP_AUTO
                      | PHY_ADDR_FCONTX_AUTO | PHY_ADDR_FCONRX_AUTO);
            phy_reg |= this->children[0]->initdata.phy.mdio_addr;
        }else if(mode == MAC_MODE_MII){
            cfg_reg |= (MII_CFG_MIIMODE_MIIM | MII_CFG_MIIRATE_25MHZ | MII_CFG_EN);
            mac_ctrl_0 |= MAC_CTRL_0_GMII_MII;
            phy_reg |= PHY_ADDR_SPEED_100;
        }else if(mode == MAC_MODE_GMII){
            cfg_reg |= (MII_CFG_MIIRATE_125MHZ | MII_CFG_EN);
            mac_ctrl_0 |= MAC_CTRL_0_GMII_RGMII;
            phy_reg |= PHY_ADDR_SPEED_1000;
        }else{
            AVMNET_ERR("[%s] Invalid interface mode 0x%x for MAC %d.\n", __func__, mode, mac_nr);
            valid = 0;
        }
        break;
    case 3:
        if(mode == MAC_MODE_AUTO){
            cfg_reg |= (MII_CFG_MIIRATE_AUTO | MII_CFG_EN | MII_CFG_MIIMODE_MIIP);
            mac_ctrl_0 |= (MAC_CTRL_0_FCON_AUTO | MAC_CTRL_0_FDUP_AUTO | MAC_CTRL_0_GMII_AUTO);
            phy_reg = 0;
            phy_reg |= (PHY_ADDR_LINKST_AUTO | PHY_ADDR_SPEED_AUTO | PHY_ADDR_FDUP_AUTO
                      | PHY_ADDR_FCONTX_AUTO | PHY_ADDR_FCONRX_AUTO);
            phy_reg |= this->children[0]->initdata.phy.mdio_addr;
        }else if(mode == MAC_MODE_MII){
            cfg_reg |= (MII_CFG_MIIMODE_MIIM | MII_CFG_MIIRATE_25MHZ | MII_CFG_EN);
            mac_ctrl_0 |= MAC_CTRL_0_GMII_MII;
            phy_reg |= PHY_ADDR_SPEED_100;
        }else{
            AVMNET_ERR("[%s] Invalid interface mode 0x%x for MAC %d.\n", __func__, mode, mac_nr);
            valid = 0;
        }
        break;
    case 5:

        switch(mode){
        case MAC_MODE_AUTO:
            cfg_reg |= (MII_CFG_MIIRATE_AUTO | MII_CFG_EN | MII_CFG_MIIMODE_MIIM);
            mac_ctrl_0 |= (MAC_CTRL_0_FCON_AUTO | MAC_CTRL_0_FDUP_AUTO | MAC_CTRL_0_GMII_AUTO);
            phy_reg = 0;
            phy_reg |= (PHY_ADDR_LINKST_AUTO | PHY_ADDR_SPEED_AUTO | PHY_ADDR_FDUP_AUTO
                      | PHY_ADDR_FCONTX_AUTO | PHY_ADDR_FCONRX_AUTO);
            phy_reg |= this->children[0]->initdata.phy.mdio_addr;
            break;
        case MAC_MODE_MII:
            cfg_reg |= (MII_CFG_MIIMODE_MIIM | MII_CFG_MIIRATE_25MHZ | MII_CFG_EN);
            mac_ctrl_0 |= MAC_CTRL_0_GMII_MII;
            phy_reg |= PHY_ADDR_SPEED_100;
            break;
        case MAC_MODE_RGMII_100:
            cfg_reg |= (MII_CFG_MIIMODE_RGMII | MII_CFG_MIIRATE_25MHZ | MII_CFG_EN);
            mac_ctrl_0 |= MAC_CTRL_0_GMII_RGMII;
            phy_reg |= PHY_ADDR_SPEED_100;
            break;
        case MAC_MODE_RGMII_1000:
            cfg_reg |= (MII_CFG_MIIMODE_RGMII | MII_CFG_MIIRATE_125MHZ | MII_CFG_EN);
            mac_ctrl_0 |= MAC_CTRL_0_GMII_RGMII;
            phy_reg |= PHY_ADDR_SPEED_1000;
            break;
        default:
            AVMNET_ERR("[%s] Invalid interface mode 0x%x for MAC %d.\n", __func__, mode, mac_nr);
            valid = 0;
            break;
        }
        break;
    default:
        AVMNET_ERR("[%s] Invalid MAC number %d\n", __func__, mac_nr);
        valid = 0;
        break;
    }

    if(valid && (this->initdata.mac.flags & AVMNET_CONFIG_FLAG_MDIOPOLLING)){
        unsigned int child_mdio_addr = 0;
        AVMNET_INFO("[%s] Setup MDIO_Polling, try to find corresponding mdio-address for mac %d\n", __func__, mac_nr);
        for(i = 0; i < this->num_children; i++){
            if(this->children[i]->type == avmnet_modtype_phy){
                child_mdio_addr = this->children[i]->initdata.phy.mdio_addr;
                child_mdio_addr &= ((1 << 5) - 1);
                AVMNET_INFO("[%s] Setup MDIO_Polling, mdio-address =%#x (from %s)\n", __func__, child_mdio_addr, this->children[i]->name);
                phy_reg |= child_mdio_addr;
                break;
            }
        }
    }

    if(valid && (this->initdata.mac.flags & (AVMNET_CONFIG_FLAG_RX_DELAY | AVMNET_CONFIG_FLAG_TX_DELAY))){
        pcdu = SW_READ_REG32(MII_PCDU_REG(mac_nr));
        if(this->initdata.mac.flags & AVMNET_CONFIG_FLAG_RX_DELAY){
            AVMNET_DEBUG("[%s] Setting MAC%d RXDLY to 0x%x\n", __func__, mac_nr,
                                                             this->initdata.mac.rx_delay);
            pcdu &= ~MII_PCDU_RXDLY_MASK;
            pcdu |= ((this->initdata.mac.rx_delay << 7) & MII_PCDU_RXDLY_MASK);
        }

        if(this->initdata.mac.flags & AVMNET_CONFIG_FLAG_TX_DELAY){
            AVMNET_DEBUG("[%s] Setting MAC%d TXDLY to 0x%x\n", __func__, mac_nr,
                                                             this->initdata.mac.tx_delay);
            pcdu &= ~MII_PCDU_TXDLY_MASK;
            pcdu |= (this->initdata.mac.tx_delay & MII_PCDU_TXDLY_MASK);
        }

        AVMNET_DEBUG("[%s] Register 0x%08x: 0x%04x\n", __func__, MII_PCDU_REG(mac_nr), pcdu);
        SW_WRITE_REG32(pcdu, MII_PCDU_REG(mac_nr));
    }

    if(valid){
        AVMNET_INFO("[%s] Setting MAC_%d config: cfg_reg: 0x%08x, mac_ctrl_0: 0x%08x, phy_reg: 0x%08x\n", __func__, mac_nr, cfg_reg, mac_ctrl_0, phy_reg);
        mac_ctrl_0 |= (MAC_CTRL_0_FDUP_AUTO | MAC_CTRL_0_FCON_AUTO);

        SW_WRITE_REG32(cfg_reg, MII_CFG_REG(mac_nr));
        SW_WRITE_REG32(mac_ctrl_0, MAC_CTRL(mac_nr, 0));
        SW_WRITE_REG32(phy_reg, PHY_ADDR(mac_nr));
    }

    cfg_reg = SW_READ_REG32(MII_CFG_REG(mac_nr));
    AVMNET_INFO("[%s] MII_CFG_REG(%#x) = %#x (nach setup)\n", __func__, (unsigned int) MII_CFG_REG(mac_nr), cfg_reg);

    phy_reg = SW_READ_REG32(PHY_ADDR(mac_nr));
    AVMNET_INFO("[%s] PHY_ADDR(%#x) = %#x (nach setup)\n", __func__, (unsigned int) PHY_ADDR(mac_nr), phy_reg);

    mac_ctrl_0 = SW_READ_REG32(MAC_CTRL(mac_nr, 0));
    AVMNET_INFO("[%s] MAC_CTRL(%#x) = %#x (nach setup)\n", __func__, (unsigned int) MAC_CTRL(mac_nr, 0), mac_ctrl_0);

    this->unlock(this);

    for(i = 0; i < this->num_children; ++i){
        result = this->children[i]->setup(this->children[i]);
        if(result != 0){
            AVMNET_ERR("Module %s: setup() failed on child %s\n", this->name, this->children[i]->name);
            return result;
        }
    }

    if(this->initdata.mac.flags & AVMNET_CONFIG_FLAG_SWITCHPORT) {
        avmnet_linkstatus_t status = { .Status = 0 }; 

        if (this->lock(this)) {
            AVMNET_ERR("[%s] Interrupted while waiting for lock.\n", __func__);
            return -EINTR;
        }

        status.Details.link = 1;
        status.Details.speed = 2;
        status.Details.fullduplex = 1; 
        status.Details.powerup = 1; 
        status.Details.flowcontrol = 1; 
                                       
        AVMNET_INFO("[%s] AVMNET_CONFIG_FLAG_SWITCHPORT %s\n", __func__, this->name);

        /*--- result = this->parent->set_status(this->parent, this->device_id, status); ---*/
        setup_mac_speed(this, status);

        this->unlock(this);
    }

    avmnet_cfg_register_module(this);
    avmnet_cfg_add_simple_procentry(this, "mac_cfg", NULL, proc_registers_output );
    avmnet_cfg_add_simple_procentry(this, "mac_control", proc_mac_control_input, proc_mac_control_output);

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static int proc_mac_control_input(char *input ,void *priv_data){
    avmnet_module_t *this = (avmnet_module_t *) priv_data;

    if (this->lock(this)) {
        AVMNET_ERR("[%s] Interrupted while waiting for lock.\n", __func__);
        return -EINTR;
    }

    if(!strncmp( "suspend", input, 7)){
        AVMNET_ERR("[%s] doing suspend\n", __func__);
        avmnet_mac_vr9_suspend(this, NULL);
    }
    if(!strncmp( "resume", input, 6)){
        avmnet_mac_vr9_resume(this, NULL);
        AVMNET_ERR("[%s] doing resume\n", __func__);
    }
    this->unlock(this);
    return 0;
}
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

int avmnet_mac_vr9_suspend(avmnet_module_t *this, avmnet_module_t *caller __attribute__((unused))) {
    uint32_t phy_reg;
    int i;
    int mac_nr = this->initdata.mac.mac_nr;
    struct avmnet_mac_vr9_context *ctx;

    ctx = (struct avmnet_mac_vr9_context *) this->priv;

    if ( ! this->trylock(this)) {
        AVMNET_ERR("[%s] Called without MUTEX held!\n", __func__);
        dump_stack();
        this->unlock(this);
    }

    AVMNET_INFO("[%s] suspend called on (mac_nr %d) module %s by module %s.\n", __func__, mac_nr, this->name, caller != NULL ? caller->name:"unknown");

    if(ctx->suspend_cnt == 0){
        phy_reg = SW_READ_REG32(PHY_ADDR(mac_nr));
        AVMNET_INFO("[%s] PHY_ADDR(%#x) = %#x (vor shutdown mac link)\n", __func__, (unsigned int) PHY_ADDR(mac_nr), phy_reg);
        phy_reg &= ~(PHY_ADDR_LINKST_MASK);
        phy_reg |= PHY_ADDR_LINKST_DOWN;
        SW_WRITE_REG32(phy_reg, PHY_ADDR(mac_nr));
        AVMNET_INFO("[%s] PHY_ADDR(%#x) = %#x (nach shutdown mac link)\n", __func__, (unsigned int) PHY_ADDR(mac_nr), phy_reg);

        if ( this->initdata.mac.flags & AVMNET_CONFIG_FLAG_PHY_PWDWN_ON_MAC_SUSPEND ){
            for(i = 0; i < this->num_children; i++){
                if ((this->children[i]->type == avmnet_modtype_phy ) && ( this->children[i]->powerdown ) ){
                    this->children[i]->powerdown( this->children[i] );
                    AVMNET_INFO("[%s] call %pF for %s\n", __func__, this->children[i]->powerdown, this->children[i]->name  );
                }
            }
        }
    }

    ctx->suspend_cnt++;
    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void avmnet_mac_vr9_disable_all(void) {
    uint32_t cfg_reg;
    uint32_t mac_nr;
    for (mac_nr = 0; mac_nr < 5; mac_nr++) {
        cfg_reg = SW_READ_REG32(MII_CFG_REG(mac_nr));
        AVMNET_ERR("[%s] Pre: MII_CFG(%#x) = %#x \n", __func__, mac_nr, cfg_reg);
        cfg_reg &= ~MII_CFG_EN;
        SW_WRITE_REG32(cfg_reg, MII_CFG_REG(mac_nr));
        AVMNET_ERR("[%s] Post: MII_CFG(%#x) = %#x \n", __func__, mac_nr, SW_READ_REG32(MII_CFG_REG(mac_nr)));
    }
}
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_mac_vr9_resume(avmnet_module_t *this, avmnet_module_t *caller) {

    int i;
    int mac_nr = this->initdata.mac.mac_nr;
    struct avmnet_mac_vr9_context *ctx;

    ctx = (struct avmnet_mac_vr9_context *) this->priv;

    if ( ! this->trylock(this)) {
        AVMNET_ERR("[%s] Called without MUTEX held!\n", __func__);
        dump_stack();
        this->unlock(this);
    }

    AVMNET_INFO("[%s] resume called on module %s by module %s.\n", __func__, this->name, caller != NULL ? caller->name:"unknown");

    if(ctx->suspend_cnt == 0){
        AVMNET_ERR("[%s] unbalanced call on %s by %s, suspend_cnt = 0.\n", __func__, this->name, caller != NULL ? caller->name:"unknown");
        this->unlock(this);
        return -EFAULT;
    }

    ctx->suspend_cnt--;

    if(ctx->suspend_cnt == 0){
        AVMNET_INFO("[%s] PHY_ADDR(%#x) = %#x (vor shutdown mac link)\n", __func__, (unsigned int) PHY_ADDR(mac_nr), SW_READ_REG32(PHY_ADDR(mac_nr)));

        if(this->initdata.mac.flags & AVMNET_CONFIG_FLAG_SWITCHPORT) {
            uint32_t phy_reg = SW_READ_REG32(PHY_ADDR(mac_nr));
            phy_reg &= ~(PHY_ADDR_LINKST_MASK);
            phy_reg |= PHY_ADDR_LINKST_UP;
            SW_WRITE_REG32(phy_reg, PHY_ADDR(mac_nr));
        } else {
            ctx->last_known_state.Details.link = 0;
        }

        AVMNET_INFO("[%s] PHY_ADDR(%#x) = %#x (nach shutdown mac link)\n", __func__, (unsigned int) PHY_ADDR(mac_nr), SW_READ_REG32(PHY_ADDR(mac_nr)));

        if ( this->initdata.mac.flags & AVMNET_CONFIG_FLAG_PHY_PWDWN_ON_MAC_SUSPEND ){
            for(i = 0; i < this->num_children; i++){
                if ((this->children[i]->type == avmnet_modtype_phy ) && ( this->children[i]->powerup ) ){
                    this->children[i]->powerup( this->children[i] );
                    AVMNET_INFO("[%s] call %pF for %s \n", __func__, this->children[i]->powerup, this->children[i]->name );
                }
            }
        }
    }

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_mac_vr9_reinit(avmnet_module_t *this __attribute__((unused)))
{
    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_mac_vr9_exit(avmnet_module_t *this)
{
    int i, result;

    AVMNET_INFO("[%s] Init on module %s called.\n", __func__, this->name);

    for(i = 0; i < this->num_children; ++i){
        result = this->children[i]->exit(this->children[i]);
        if(result != 0){
            AVMNET_WARN("Module %s: exit() failed on child %s\n", this->name, this->children[i]->name);
            // handle error
        }
    }

    /*
     * clean up our mess
     */

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_mac_vr9_setup_irq(avmnet_module_t *this __attribute__ ((unused)),
                                unsigned int on __attribute__ ((unused)))
{
    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
unsigned int avmnet_mac_vr9_reg_read(avmnet_module_t *this, unsigned int addr,
                                        unsigned int reg)
{
    return this->parent->reg_read(this->parent, addr, reg);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_mac_vr9_reg_write(avmnet_module_t *this, unsigned int addr, unsigned int reg,
                                unsigned int val)
{
    return this->parent->reg_write(this->parent, addr, reg, val);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_mac_vr9_lock(avmnet_module_t *this) {
    return this->parent->lock(this->parent);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void avmnet_mac_vr9_unlock(avmnet_module_t *this) {
    this->parent->unlock(this->parent);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_mac_vr9_trylock(avmnet_module_t *this) {
    return this->parent->trylock(this->parent);
}

/*------------------------------------------------------------------------------------------*\
 * wird im Interrupt-Kontext aufgerufen
\*------------------------------------------------------------------------------------------*/
void avmnet_mac_vr9_status_changed(avmnet_module_t *this, avmnet_module_t *caller)
{
    int i;
    // struct avmnet_mac_vr9_context *my_ctx = (struct avmnet_mac_vr9_context *) this->priv;
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
     * report status change to parent
     */

    this->parent->status_changed(this->parent, this);

    return;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static void setup_mac_speed(avmnet_module_t *this, avmnet_linkstatus_t status)
{
    int mac_nr;
    unsigned int cfg_reg, phy_reg, mac_ctrl_0;
    struct avmnet_mac_vr9_context *ctx;

    ctx = (struct avmnet_mac_vr9_context *) this->priv;

    // warn on unsafe call
    if ( ! this->trylock(this)) {
        AVMNET_ERR("[%s] Called without MUTEX held!\n", __func__);
        dump_stack();
        this->trylock(this);
    }

    AVMNET_INFO("[%s] mod: %s, status=%#x \n", __func__, this->name, status.Status);

    mac_nr = this->initdata.mac.mac_nr;

    cfg_reg = SW_READ_REG32(MII_CFG_REG(mac_nr));
    AVMNET_INFO("[%s] MII_CFG_REG(%#x) = %#x (vor setup_mac_speed)\n", __func__, (unsigned int) MII_CFG_REG(mac_nr), cfg_reg);

    phy_reg = SW_READ_REG32(PHY_ADDR(mac_nr));
    AVMNET_INFO("[%s] PHY_ADDR(%#x) = %#x (vor setup_mac_speed)\n", __func__, (unsigned int) PHY_ADDR(mac_nr), phy_reg);

    mac_ctrl_0 = SW_READ_REG32(MAC_CTRL(mac_nr, 0));
    AVMNET_INFO("[%s] MAC_CTRL(%#x) = %#x (vor setup_mac_speed)\n", __func__, (unsigned int) MAC_CTRL(mac_nr, 0), mac_ctrl_0);

    cfg_reg &= ~MII_CFG_MIIRATE_MASK;
    cfg_reg |= (status.Details.speed == 0) ? MII_CFG_MIIRATE_2_5MHZ : 0;
    cfg_reg |= (status.Details.speed == 1) ? MII_CFG_MIIRATE_25MHZ : 0;
    cfg_reg |= (status.Details.speed == 2) ? MII_CFG_MIIRATE_125MHZ : 0;

    mac_ctrl_0 &= ~(MAC_CTRL_0_GMII_MASK | MAC_CTRL_0_FDUP_MASK | MAC_CTRL_0_FCON_MASK );
    mac_ctrl_0 |= (status.Details.speed == 2 ) ? MAC_CTRL_0_GMII_RGMII : MAC_CTRL_0_GMII_MII;
    mac_ctrl_0 |= (status.Details.fullduplex == 1 ) ? MAC_CTRL_0_FDUP_EN : MAC_CTRL_0_FDUP_DIS;
    mac_ctrl_0 |= (status.Details.flowcontrol << 4) & MAC_CTRL_0_FCON_MASK;

    phy_reg &= PHY_ADDR_ADDR_MASK;
    phy_reg |= (status.Details.flowcontrol & 0x0001 ) ? PHY_ADDR_FCONRX_EN : PHY_ADDR_FCONRX_DIS;
    phy_reg |= (status.Details.flowcontrol & 0x0002 ) ? PHY_ADDR_FCONTX_EN : PHY_ADDR_FCONTX_DIS;
    phy_reg |= (status.Details.fullduplex) ? PHY_ADDR_FDUP_EN : PHY_ADDR_FDUP_DIS;
    phy_reg |= (status.Details.speed == 0 ) ? PHY_ADDR_SPEED_10 : 0;
    phy_reg |= (status.Details.speed == 1 ) ? PHY_ADDR_SPEED_100 : 0;
    phy_reg |= (status.Details.speed == 2 ) ? PHY_ADDR_SPEED_1000 : 0;

    if((status.Details.link) && (ctx->suspend_cnt == 0)){
        phy_reg |= PHY_ADDR_LINKST_UP;
    }else{
        phy_reg |= PHY_ADDR_LINKST_DOWN;
    }

    SW_WRITE_REG32(cfg_reg, MII_CFG_REG(mac_nr));
    SW_WRITE_REG32(mac_ctrl_0, MAC_CTRL(mac_nr, 0));
    SW_WRITE_REG32(phy_reg, PHY_ADDR(mac_nr));

    AVMNET_INFO("[%s] MII_CFG_REG(%#x) = %#x (nach setup_mac_speed)\n", __func__, (unsigned int) MII_CFG_REG(mac_nr), cfg_reg);
    AVMNET_INFO("[%s] PHY_ADDR(%#x) = %#x (nach setup_mac_speed)\n", __func__, (unsigned int) PHY_ADDR(mac_nr), phy_reg);
    AVMNET_INFO("[%s] MAC_CTRL(%#x) = %#x (nach setup_mac_speed)\n", __func__, (unsigned int) MAC_CTRL(mac_nr, 0), mac_ctrl_0);

}

/*------------------------------------------------------------------------------------------*\
 *  Wird vom Child bei der Bearbeitung von poll aufgerufen
 \*------------------------------------------------------------------------------------------*/
int avmnet_mac_vr9_set_status(avmnet_module_t *this, avmnet_device_t *device_id,
                                avmnet_linkstatus_t status)
{
    struct avmnet_mac_vr9_context *ctx;

    ctx = (struct avmnet_mac_vr9_context *) this->priv;

    if (this->lock(this)) {
        AVMNET_INFO("[%s] Interrupted while waiting for lock.\n", __func__);
        return -EINTR;
    }

    if ( ! (this->initdata.mac.flags & AVMNET_CONFIG_FLAG_SWITCHPORT)) {
        if((ctx->last_known_state.Status != status.Status) && (this->initdata.mac.mac_mode != MAC_MODE_AUTO)) {
            setup_mac_speed(this, status);
        }
    }

    ctx->last_known_state = status;

    if(ctx->suspend_cnt > 0){
        status.Details.link = 0;
    }

    this->unlock(this);
    return this->parent->set_status(this->parent, device_id, status);
}

/*------------------------------------------------------------------------------------------*\
 *
 \*------------------------------------------------------------------------------------------*/
int avmnet_mac_vr9_poll(avmnet_module_t *this)
{
    int i, result;

    for(i = 0; i < this->num_children; ++i){
        result = this->children[i]->poll(this->children[i]);
        if(result < 0){
            AVMNET_WARN("Module %s: poll() failed on child %s\n", this->name, this->children[i]->name);
        }
    }

    return 0;
}

int avmnet_mac_vr9_powerdown(avmnet_module_t *this __attribute__((unused)))
{
    return 0;
}

int avmnet_mac_vr9_powerup(avmnet_module_t *this __attribute__((unused)))
{
    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static void proc_registers_output(struct seq_file *seq, void *data){

    unsigned int mac_nr, cfg_reg, phy_reg, mac_ctrl_0;
    avmnet_module_t *this;

    this = (avmnet_module_t *) data;

    mac_nr = this->initdata.mac.mac_nr;

    cfg_reg = SW_READ_REG32(MII_CFG_REG(mac_nr));
    phy_reg = SW_READ_REG32(PHY_ADDR(mac_nr));
    mac_ctrl_0 = SW_READ_REG32(MAC_CTRL(mac_nr, 0));

    seq_printf(seq, "Configuration for MAC %d\n", mac_nr);
    seq_printf(seq, "MII_CFG  : 0x%04x\n", cfg_reg);
    seq_printf(seq, "PHY_ADDR : 0x%04x\n", phy_reg);
    seq_printf(seq, "MAC_CTRL0: 0x%04x\n", mac_ctrl_0);

}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

static void proc_mac_control_output(struct seq_file *seq, void *data){
    unsigned int mac_nr, cfg_reg, phy_reg, mac_ctrl_0;
    avmnet_module_t *this;

    this = (avmnet_module_t *) data;

    mac_nr = this->initdata.mac.mac_nr;

    cfg_reg = SW_READ_REG32(MII_CFG_REG(mac_nr));
    phy_reg = SW_READ_REG32(PHY_ADDR(mac_nr));
    mac_ctrl_0 = SW_READ_REG32(MAC_CTRL(mac_nr, 0));

    seq_printf(seq, "Configuration for MAC %d\n", mac_nr);
    seq_printf(seq, "MII_CFG  : 0x%04x\n", cfg_reg);
    seq_printf(seq, "PHY_ADDR : 0x%04x\n", phy_reg);
    seq_printf(seq, "MAC_CTRL0: 0x%04x\n", mac_ctrl_0);
    seq_printf(seq, "commands: suspend / resume\n");

}


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
