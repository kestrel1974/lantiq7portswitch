#include "include/avmnet_module.h"
#include "include/avmnet_config.h"
#include "avmnet_dummy_module.h"

extern avmnet_module_t module_switch1;
extern avmnet_module_t module_switch2;

avmnet_module_t module_topdev =
{
    .name           = "topdev",
    .type           = avmnet_modtype_switch,
    .priv           = NULL,
    .init           = dummy_init,
    .setup          = dummy_setup,
    .exit           = dummy_exit,
    
    .reg_read       = dummy_reg_read,
    .reg_write      = dummy_reg_write,
    .lock           = dummy_lock,
    .unlock         = dummy_unlock,
    .status_changed = dummy_status_changed,
    .poll           = dummy_poll,
    .setup_irq      = dummy_setup_irq,
    
    .parent         = NULL,
    .num_children   = 2,
    .children       = { &module_switch1, &module_switch2 }
};

extern avmnet_module_t module_eth0, module_eth1;
avmnet_module_t module_switch1 =
{
    .name           = "switch1",
    .type           = avmnet_modtype_switch,
    .priv           = NULL,
    .init           = dummy_init,
    .setup          = dummy_setup,
    .exit           = dummy_exit,

    .reg_read       = dummy_reg_read,
    .reg_write      = dummy_reg_write,
    .lock           = dummy_lock,
    .unlock         = dummy_unlock,
    .status_changed = dummy_status_changed,
    .poll           = dummy_poll,
    .setup_irq      = dummy_setup_irq,

    .parent         = &module_topdev,
    .num_children   = 2,
    .children       = { &module_eth0, &module_eth1 }
};

extern avmnet_module_t module_eth2, module_eth3;
avmnet_module_t module_switch2 =
{
    .name           = "switch2",
    .type           = avmnet_modtype_switch,
    .priv           = NULL,
    .init           = dummy_init,
    .setup          = dummy_setup,
    .exit           = dummy_exit,
    
    .reg_read       = dummy_reg_read,
    .reg_write      = dummy_reg_write,
    .lock           = dummy_lock,
    .unlock         = dummy_unlock,
    .status_changed = dummy_status_changed,
    .poll           = dummy_poll,
    .setup_irq      = dummy_setup_irq,
    
    .parent         = &module_topdev,
    .num_children   = 2,
    .children       = { &module_eth2, &module_eth3 }
};

avmnet_module_t module_eth0 =
{
    .name           = "eth0",
    .type           = avmnet_modtype_phy,
    .priv           = NULL,
    .init           = dummy_init,
    .setup          = dummy_setup,
    .exit           = dummy_exit,
    
    .reg_read       = dummy_reg_read,
    .reg_write      = dummy_reg_write,
    .lock           = dummy_lock,
    .unlock         = dummy_unlock,
    .status_changed = dummy_status_changed,
    .poll           = dummy_poll,
    .setup_irq      = dummy_setup_irq,
    
    .parent         = &module_switch1,
    .num_children   = 0,
    .children       = {}
};

avmnet_module_t module_eth1 =
{
    .name           = "eth1",
    .type           = avmnet_modtype_phy,
    .priv           = NULL,
    .init           = dummy_init,
    .setup          = dummy_setup,
    .exit           = dummy_exit,
        
    .reg_read       = dummy_reg_read,
    .reg_write      = dummy_reg_write,
    .lock           = dummy_lock,
    .unlock         = dummy_unlock,
    .status_changed = dummy_status_changed,
    .poll           = dummy_poll,
    .setup_irq      = dummy_setup_irq,
    
    .parent         = &module_switch1,
    .num_children   = 0,
    .children       = {}
};

avmnet_module_t module_eth2 =
{
    .name           = "eth2",
    .type           = avmnet_modtype_phy,
    .priv           = NULL,
    .init           = dummy_init,
    .setup          = dummy_setup,
    .exit           = dummy_exit,
    
    .reg_read       = dummy_reg_read,
    .reg_write      = dummy_reg_write,
    .lock           = dummy_lock,
    .unlock         = dummy_unlock,
    .status_changed = dummy_status_changed,
    .poll           = dummy_poll,
    .setup_irq      = dummy_setup_irq,

    .parent         = &module_switch2,
    .num_children   = 0,
    .children       = {}
};

avmnet_module_t module_eth3 =
{
    .name           = "eth3",
    .type           = avmnet_modtype_phy,
    .priv           = NULL,
    .init           = dummy_init,
    .setup          = dummy_setup,
    .exit           = dummy_exit,
    
    .reg_read       = dummy_reg_read,
    .reg_write      = dummy_reg_write,
    .lock           = dummy_lock,
    .unlock         = dummy_unlock,
    .status_changed = dummy_status_changed,
    .poll           = dummy_poll,
    .setup_irq      = dummy_setup_irq,

    .parent         = &module_switch2,
    .num_children   = 0,
    .children       = {}
};

