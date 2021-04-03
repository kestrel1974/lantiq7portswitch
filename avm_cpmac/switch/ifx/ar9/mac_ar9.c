
#include <avmnet_module.h>
#include <avmnet_config.h>
#include "mac_ar9.h"
#include "swi_ar9_reg.h"

static void setup_mac_speed(avmnet_module_t *this, avmnet_linkstatus_t status);

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_mac_ar9_init(avmnet_module_t *this)
{
    struct avmnet_mac_ar9_context *my_ctx;
    int i, result;

    AVMNET_INFO("[%s] Init on module %s called.\n", __func__, this->name);

    my_ctx = kzalloc(sizeof(struct avmnet_mac_ar9_context), GFP_KERNEL);
    if(my_ctx == NULL){
        printk(KERN_ERR "{%s} init of avmnet-module %s failed.\n", __func__, this->name);
        return -ENOMEM;
    }

    this->priv = my_ctx;

    init_MUTEX(&(my_ctx->mutex));
    my_ctx->mac_nr = this->initdata.mac.mac_nr;
    my_ctx->mode = this->initdata.mac.mac_mode;
    my_ctx->last_known_state.Status = 0;

    for(i = 0; i < this->num_children; ++i){
        result = this->children[i]->init(this->children[i]);
        if(result < 0){
            // handle error
        }
    }

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_mac_ar9_setup(avmnet_module_t *this)
{
    int i, result, mac_nr, mode;
    unsigned int rgmii_ctl, eth_ctl, vlan_reg, valid;

    AVMNET_INFO("[%s] Setup on module %s called.\n", __func__, this->name);

    /*
     * do cunning setup-stuff
     */

    mac_nr = this->initdata.mac.mac_nr;
    mode = this->initdata.mac.mac_mode;

    if(this->lock(this)){
        AVMNET_ERR("[%s] interrupted while trying to lock parent!\n", __func__);
        return -EFAULT;
    }

    // configure MAC
    eth_ctl = IFX_REG_R32(IFX_AR9_ETH_CTL(mac_nr));
    eth_ctl &= ~(PX_CTL_FLP | PX_CTL_AD);
    eth_ctl |= (PX_CTL_FLD              // force link down /disable MAC
//                    | PX_CTL_DFWD       // direct forward all frames to CPU port
                    | PX_CTL_BYPASS     // bypass VLAN processing
                    | PX_CTL_LD         // disable learning
                    | PX_CTL_DMDIO);    // turn off MDIO polling
    
    AVMNET_DEBUG("[%s] IFX_AR9_ETH_CTL(%d): %#08x\n", __func__, mac_nr, eth_ctl);

    rgmii_ctl = IFX_REG_R32(IFX_AR9_ETH_RGMII_CTL);
    rgmii_ctl &= ~(RGMII_CTL_FCE(mac_nr)
                    | RGMII_CTL_SPD_MASK(mac_nr)
                    | RGMII_CTL_IS_MASK(mac_nr)
                    | RGMII_CTL_DUP(mac_nr)
                    | RGMII_CTL_RDLY_MASK(mac_nr)
                    | RGMII_CTL_TDLY_MASK(mac_nr));


    // enable flow control and full duplex mode
    rgmii_ctl |= (RGMII_CTL_FCE(mac_nr) | RGMII_CTL_DUP(mac_nr));

    // set default FID to 0 and put all ports into the VLAN portmap
    vlan_reg = IFX_REG_R32(IFX_AR9_ETH_PORT_VLAN(mac_nr));
    vlan_reg &= ~(PORT_VLAN_DFID_MASK | PORT_VLAN_DVPM_MASK);
    vlan_reg |= PORT_VLAN_DVPM(0x7); // ports 0, 1, 2

    AVMNET_DEBUG("[%s] mac_nr: %d, vlan_reg: %#08x\n", __func__, mac_nr, vlan_reg);
    IFX_REG_W32(vlan_reg, IFX_AR9_ETH_PORT_VLAN(mac_nr));

    valid = 1;

    // set MII mode and interface speed according to configuration
    switch(mode){
    case MAC_MODE_RGMII_1000:
        rgmii_ctl |= (RGMII_CTL_IS_RGMII(mac_nr)
                      | RGMII_CTL_SPD_1000(mac_nr));
        break;
    case MAC_MODE_RGMII_100:
        rgmii_ctl |= (RGMII_CTL_IS_RGMII(mac_nr)
                      | RGMII_CTL_SPD_100(mac_nr));
        break;
    case MAC_MODE_RMII:
        rgmii_ctl |= (RGMII_CTL_IS_RMII(mac_nr)
                      | RGMII_CTL_SPD_100(mac_nr));
        break;
    case MAC_MODE_MII:
        rgmii_ctl |= (RGMII_CTL_IS_MII(mac_nr)
                      | RGMII_CTL_SPD_100(mac_nr));
        break;
    default:
        AVMNET_ERR("[%s] Invalid mode for MAC number %d (%s)\n", __func__, mac_nr, this->name);
        valid = 0;
        break;
    }

    if(valid){
        AVMNET_INFO("[%s] mac_nr: %d eth_ctl: 0x%08x rgmii_ctl: 0x%08x\n",
                          __func__, mac_nr, eth_ctl, rgmii_ctl);
        IFX_REG_W32(eth_ctl, IFX_AR9_ETH_CTL(mac_nr));
        IFX_REG_W32(rgmii_ctl, IFX_AR9_ETH_RGMII_CTL);
    }

    this->unlock(this);

    for(i = 0; i < this->num_children; ++i){
        result = this->children[i]->setup(this->children[i]);
        if(result != 0){
            // handle error
        }
    }

#if 0
    if(this->parent->lock(this->parent)){
        AVMNET_ERR("[%s] interrupted while trying to lock parent!\n", __func__);
        return -EFAULT;
    }

    avmnet_mac_ar9_force_ls_up( this );

    this->parent->unlock(this->parent);
#endif

    avmnet_cfg_register_module(this);

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_mac_ar9_exit(avmnet_module_t *this)
{
    int i, result;
   
    AVMNET_INFO("[%s] Init on module %s called.\n", __func__, this->name);
    
    for(i = 0; i < this->num_children; ++i){
        result = this->children[i]->exit(this->children[i]);
        if(result != 0){
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
int avmnet_mac_ar9_setup_irq(avmnet_module_t *this_modul __attribute__ ((unused)), unsigned int on __attribute__ ((unused)) )
{
    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
unsigned int avmnet_mac_ar9_reg_read(avmnet_module_t *this, unsigned int addr, unsigned int reg)
{
    return this->parent->reg_read(this->parent, addr, reg);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_mac_ar9_reg_write(avmnet_module_t *this, unsigned int addr, unsigned int reg, unsigned int val)
{
    return this->parent->reg_write(this->parent, addr, reg, val);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_mac_ar9_lock(avmnet_module_t *this) 
{
    return this->parent->lock(this->parent);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void avmnet_mac_ar9_unlock(avmnet_module_t *this) 
{
    this->parent->unlock(this->parent);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_mac_ar9_trylock(avmnet_module_t *this) 
{
    return this->parent->trylock(this->parent);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void avmnet_mac_ar9_status_changed(avmnet_module_t *this, avmnet_module_t *caller)
{
    int i;
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
        printk(KERN_ERR "{%s}: module %s: received status_changed from unknown module.\n", __func__, this->name);
        return;
    }

    /*
     * handle status change
     */

    return;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_mac_ar9_poll(avmnet_module_t *this __attribute__ ((unused)))
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

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_mac_ar9_set_status(avmnet_module_t *this, avmnet_device_t *device_id,
                                avmnet_linkstatus_t status)
{
    struct avmnet_mac_ar9_context *ctx;

    ctx = (struct avmnet_mac_ar9_context *) this->priv;

    if(this->lock(this)){
        AVMNET_ERR("[%s] interrupted while trying to lock parent!\n", __func__);
        return -EINTR;
    }

    if(ctx->last_known_state.Status != status.Status && (this->initdata.mac.mac_mode != MAC_MODE_AUTO)) {
        setup_mac_speed(this, status);
    }

    ctx->last_known_state = status;

    if(ctx->suspend_cnt > 0){
        status.Details.link = 0;
    }

    this->unlock(this);
    return this->parent->set_status(this->parent, device_id, status);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_mac_ar9_powerdown(avmnet_module_t *this __attribute__ ((unused)))
{
    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_mac_ar9_powerup(avmnet_module_t *this __attribute__ ((unused)))
{
    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static void setup_mac_speed(avmnet_module_t *this, avmnet_linkstatus_t status)
{
    int mac_nr;
    unsigned int rgmii_ctl, port_ctl;
    struct avmnet_mac_ar9_context *ctx = (struct avmnet_mac_ar9_context *) this->priv;

    AVMNET_INFO("[%s] mod: %s, status=%#x \n", __func__, this->name, status.Status);

    // warn on unsafe call
    if( ! this->trylock(this)) {
        AVMNET_ERR("[%s] Called without MUTEX held!\n", __func__);
        dump_stack();
        this->unlock(this);
    }

    mac_nr = this->initdata.mac.mac_nr;

    /*
     * FIXME: is it safe to lock the parent while holding our own lock?
     * What if parent tries to get our lock while holding his own?
     */
    rgmii_ctl = IFX_REG_R32(IFX_AR9_ETH_RGMII_CTL);

    AVMNET_INFO("[%s] mac_nr: %d eth_ctl: 0x%08x rgmii_ctl: 0x%08x\n",
                      __func__, mac_nr, IFX_REG_R32(IFX_AR9_ETH_CTL(mac_nr)), rgmii_ctl);

    rgmii_ctl &= ~(RGMII_CTL_FCE(mac_nr) | RGMII_CTL_DUP(mac_nr) | RGMII_CTL_SPD_MASK(mac_nr));

    switch(status.Details.speed){
    case 2: // GBit
        rgmii_ctl |= RGMII_CTL_SPD_1000_SET(mac_nr);
        break;
    case 1: // 100 MBit
        rgmii_ctl |= RGMII_CTL_SPD_100_SET(mac_nr);
        break;
    case 0: // 10 MBit
    default:
        // the speed bits are 0 for 10 MBit
        break;
    }

    if (( status.Details.flowcontrol & AVMNET_LINKSTATUS_FC_TX ) && !( status.Details.flowcontrol & AVMNET_LINKSTATUS_FC_TX )){
        AVMNET_ERR("[%s] Enable TX-Pause, Disable RX-Pause: Not possible - we don't support ASYM_FC\n", __func__);
    }
    else if ( !(status.Details.flowcontrol & AVMNET_LINKSTATUS_FC_TX) && ( status.Details.flowcontrol & AVMNET_LINKSTATUS_FC_TX )){
        AVMNET_ERR("[%s] Disable TX-Pause, Enable RX-Pause: Not possible - we don't support ASYM_FC\n", __func__);
    }
    else{
        rgmii_ctl |= ( status.Details.flowcontrol & AVMNET_LINKSTATUS_FC_TX ) ? RGMII_CTL_FCE(mac_nr) : 0;
    }

    rgmii_ctl |= (status.Details.fullduplex) ? RGMII_CTL_DUP(mac_nr) : 0;


    IFX_REG_W32(rgmii_ctl, IFX_AR9_ETH_RGMII_CTL);

    port_ctl = IFX_REG_R32( IFX_AR9_ETH_CTL(mac_nr));
    port_ctl &= ~(PX_CTL_FLP | PX_CTL_FLD);

    if(status.Details.link && (ctx->suspend_cnt == 0)){
        port_ctl |= PX_CTL_FLP;
    }else{
        port_ctl |= PX_CTL_FLD;
    }

    IFX_REG_W32(port_ctl, IFX_AR9_ETH_CTL(mac_nr));

    AVMNET_INFO("[%s] mac_nr: %d eth_ctl: 0x%08x rgmii_ctl: 0x%08x, flow_ctrl=%d\n",
                      __func__, mac_nr, IFX_REG_R32(IFX_AR9_ETH_CTL(mac_nr)), rgmii_ctl, (rgmii_ctl & RGMII_CTL_FCE(mac_nr)) );

}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_mac_ar9_suspend(avmnet_module_t *this, avmnet_module_t *caller __attribute__((unused))) {
    int port_ctrl_reg;
    int mac_nr = this->initdata.mac.mac_nr;
    struct avmnet_mac_ar9_context *ctx;

    ctx = (struct avmnet_mac_ar9_context *) this->priv;

    if ( ! this->trylock(this)) {
        AVMNET_ERR("[%s] Called without MUTEX held!\n", __func__);
        dump_stack();
        this->unlock(this);
    }

    AVMNET_INFO("[%s] suspend called on module %s by module %s.\n", __func__, this->name, caller != NULL ? caller->name:"unknown");

    if(ctx->suspend_cnt == 0){
        port_ctrl_reg = IFX_REG_R32( IFX_AR9_ETH_CTL(mac_nr));
        AVMNET_INFO("[%s] P%d_CTL_REG %#x (vor shutdown mac link)\n", __func__, mac_nr, port_ctrl_reg);
        port_ctrl_reg &= ~PX_CTL_FLP;
        port_ctrl_reg |= PX_CTL_FLD;
        IFX_REG_W32( port_ctrl_reg, IFX_AR9_ETH_CTL(mac_nr));
        AVMNET_INFO("[%s] P%d_CTL_REG %#x (nach shutdown mac link)\n", __func__, mac_nr, port_ctrl_reg);
    }

    ctx->suspend_cnt++;
    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_mac_ar9_resume(avmnet_module_t *this, avmnet_module_t *caller) {
    struct avmnet_mac_ar9_context *ctx = (struct avmnet_mac_ar9_context *) this->priv;

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
        /*
         * pretend that the last known state was link down, so it will be
         * restored by the next set_status coming through
         */
        ctx->last_known_state.Details.link = 0;
    }

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_mac_ar9_reinit(avmnet_module_t *this __attribute__((unused)))
{
    return 0;
}


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
