#include <avmnet_module.h>
#include <avmnet_config.h>
#include "avmnet_dummy_module.h"

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int dummy_init(avmnet_module_t *this)
{
    struct dummy_context *my_ctx;
    int i, result;

    printk(KERN_ERR "{%s} Init on module %s called.\n", __func__, this->name);

    my_ctx = kzalloc(sizeof(struct dummy_context), GFP_KERNEL);
    if(my_ctx == NULL){
        printk(KERN_ERR "{%s} init of avmnet-module %s failed.\n", __func__, this->name);
        return -ENOMEM;
    }

    this->priv = my_ctx;

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
int dummy_setup(avmnet_module_t *this)
{
    int i, result;
    
    printk(KERN_ERR "{%s} Setup on module %s called.\n", __func__, this->name);
    
    /*
     * do cunning setup-stuff
     */
    
    for(i = 0; i < this->num_children; ++i){
        result = this->children[i]->setup(this->children[i]);
        if(result != 0){
            // handle error
        }
    }

    avmnet_cfg_register_module(this);

    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int dummy_exit(avmnet_module_t *this)
{
    int i, result;
   
    printk(KERN_ERR "{%s} Init on module %s called.\n", __func__, this->name);
    
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
int dummy_setup_irq(avmnet_module_t *this_modul __attribute__ ((unused)), unsigned int on __attribute__ ((unused)) )
{
    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int dummy_reg_read(avmnet_module_t *this __attribute__ ((unused)), unsigned int addr __attribute__ ((unused)), unsigned int reg __attribute__ ((unused)))
{
    return 42;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int dummy_reg_write(avmnet_module_t *this __attribute__ ((unused)), unsigned int addr __attribute__ ((unused)), unsigned int reg __attribute__ ((unused)), unsigned int val __attribute__ ((unused)))
{
    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int dummy_lock(avmnet_module_t *this __attribute__ ((unused)))
{
    return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void dummy_unlock(avmnet_module_t *this __attribute__ ((unused)))
{
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void dummy_status_changed(avmnet_module_t *this, avmnet_module_t *caller)
{
    int i;
    // struct dummy_context *my_ctx = (struct dummy_context *) this->priv;
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
int dummy_poll(avmnet_module_t *this __attribute__ ((unused)))
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

int dummy_set_status(avmnet_module_t *this, avmnet_device_t *device_id,
                                avmnet_linkstatus_t status)
{
    return this->parent->set_status(this->parent, device_id, status);
}

int dummy_powerdown(avmnet_module_t *this)
{
    return 0;
}

int dummy_powerup(avmnet_module_t *this)
{
    return 0;
}
