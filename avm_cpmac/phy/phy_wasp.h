#if !defined(__AVM_NET_PHY_WASP_)
#define __AVM_NET_PHY_WASP_

#include <avmnet_module.h>
#include <avm/net/net.h>

int avmnet_phy_wasp_init(avmnet_module_t *this);
int avmnet_phy_wasp_setup(avmnet_module_t *this);
int avmnet_phy_wasp_exit(avmnet_module_t *this);

int avm_net_phy_wasp_proc_phy_dump(char *page, char **start __attribute__ ((unused)),
                                    off_t off, int count __attribute__ ((unused)),
                                    int *eof __attribute__ ((unused)),
                                    void *data __attribute__ ((unused)));

unsigned int avmnet_phy_wasp_reg_read(avmnet_module_t *this, unsigned int addr,
                                        unsigned int reg);

int avmnet_phy_wasp_reg_write(avmnet_module_t *this, unsigned int addr, unsigned int reg,
                                unsigned int val);

int avmnet_phy_wasp_lock(avmnet_module_t *this);
void avmnet_phy_wasp_unlock(avmnet_module_t *this);
int avmnet_phy_wasp_trylock(avmnet_module_t *this);
void avmnet_phy_wasp_status_changed(avmnet_module_t *this, avmnet_module_t *child);
int avmnet_phy_wasp_poll(avmnet_module_t *this);
int avmnet_phy_wasp_setup_irq(avmnet_module_t *this, unsigned int on);
int avmnet_phy_wasp_set_status(avmnet_module_t *this, avmnet_device_t *device_id,
                                avmnet_linkstatus_t status);

int avmnet_phy_wasp_powerup(avmnet_module_t *this);
int avmnet_phy_wasp_powerdown(avmnet_module_t *this);

void cpmac_magpie_reset(enum avm_cpmac_magpie_reset value);
unsigned int cpmac_magpie_mdio_read(unsigned short regadr, unsigned short phyadr);
void cpmac_magpie_mdio_write(unsigned short regadr, unsigned short phyadr, unsigned short data);
extern void (*cpmac_magpie_reset_cb_hook)(void); /*--- wird aufgerufen, 10 ms nachdem die WLan-CPU aus dem Reset geholt wurde ---*/

typedef struct _phy_wasp_heartbeat_work
{
    struct delayed_work work;
    avmnet_module_t *this;
} phy_wasp_heartbeat_work_t;

struct avmnet_phy_wasp_context
{
    struct semaphore mutex;
    struct workqueue_struct *heartbeat_wq;
    phy_wasp_heartbeat_work_t heartbeat;
};

#endif
