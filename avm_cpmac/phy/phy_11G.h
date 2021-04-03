#if !defined(__AVM_NET_PHY_11G_)
#define __AVM_NET_PHY_11G_

#include <avmnet_module.h>
#include <linux/ethtool.h>

int avmnet_phy_11G_init(avmnet_module_t *this);
int avmnet_phy_11G_setup(avmnet_module_t *this);
int avmnet_phy_11G_exit(avmnet_module_t *this);

int avm_net_phy_11G_proc_phy_dump(char *page, char **start __attribute__ ((unused)),
                                    off_t off,
                                    int count __attribute__ ((unused)),
                                    int *eof __attribute__ ((unused)),
                                    void *data __attribute__ ((unused)));

unsigned int avmnet_phy_11G_reg_read(avmnet_module_t *this, unsigned int addr, unsigned int reg);

int avmnet_phy_11G_reg_write(avmnet_module_t *this, unsigned int addr, unsigned int reg, unsigned int val);

int avmnet_phy_11G_lock(avmnet_module_t *this);
void avmnet_phy_11G_unlock(avmnet_module_t *this);
int avmnet_phy_11G_trylock(avmnet_module_t *this);
void avmnet_phy_11G_status_changed(avmnet_module_t *this, avmnet_module_t *child);
int avmnet_phy_11G_poll(avmnet_module_t *this);
int avmnet_phy_11G_setup_irq(avmnet_module_t *this, unsigned int on);
int avmnet_phy_11G_set_status(avmnet_module_t *this, avmnet_device_t *device_id, avmnet_linkstatus_t status);
int avmnet_phy_11G_powerdown(avmnet_module_t *this);
int avmnet_phy_11G_powerup(avmnet_module_t *this);
int avmnet_phy_11G_suspend(avmnet_module_t *this, avmnet_module_t *caller);
int avmnet_phy_11G_resume(avmnet_module_t *this, avmnet_module_t *caller);
int avmnet_phy_11G_reinit(avmnet_module_t *this);

int avmnet_phy_11G_ethtool_get_settings(avmnet_module_t *this, struct ethtool_cmd *cmd);
int avmnet_phy_11G_ethtool_set_settings(avmnet_module_t *this, struct ethtool_cmd *cmd);
void avmnet_phy_11G_ethtool_get_pauseparam(avmnet_module_t *this, struct ethtool_pauseparam *param);
int avmnet_phy_11G_ethtool_set_pauseparam(avmnet_module_t *this, struct ethtool_pauseparam *param);

u32 avmnet_phy_11G_ethtool_get_link(avmnet_module_t *this);


enum pol_state {
    POL_STATE_INIT = 0,      // new link, no correction configured yet
    POL_STATE_SET,           // polarity has been configured, waiting for link to re-establish
    POL_STATE_RESET,         // polarity configuration has been reset
    POL_STATE_UP,            // link established without corrections
    POL_STATE_UP_COR,        // link re-established with corrections
};

struct avmnet_phy_11G_config
{
    unsigned int advertise;
    unsigned int autoneg;
    enum avmnet_pause_setup pause_setup;
    unsigned int speed;
    unsigned int duplex;
    unsigned int port;
    unsigned int mdi;
};


#define POL_TIMEOUT    (10 * HZ) // number jiffies to wait for certain link state changes
#define POL_PD_TIME     (3 * HZ) // number of jiffies to keep the link in PD after correction

struct avmnet_phy_11G_context
{
    struct semaphore  mutex;
    struct avmnet_phy_11G_config config;
    unsigned int      powerdown;
    long unsigned int snr_bad_time;
    unsigned int      snr_thrash_counter;
    long unsigned int snr_thrash_time;
    enum pol_state    polarity_state;
    unsigned long int polarity_timer;
};


#define PHY_11G_ETHOPS \
{ \
    .get_settings   = avmnet_phy_11G_ethtool_get_settings,      \
    .set_settings   = avmnet_phy_11G_ethtool_set_settings,      \
    .get_pauseparam = avmnet_phy_11G_ethtool_get_pauseparam,    \
    .set_pauseparam = avmnet_phy_11G_ethtool_set_pauseparam,    \
    .get_link       = avmnet_phy_11G_ethtool_get_link,          \
}

#define PHY_11G_STDFUNCS \
    .init               = avmnet_phy_11G_init,            \
    .setup              = avmnet_phy_11G_setup,           \
    .exit               = avmnet_phy_11G_exit,            \
    .reg_read           = avmnet_phy_11G_reg_read,        \
    .reg_write          = avmnet_phy_11G_reg_write,       \
    .lock               = avmnet_phy_11G_lock,            \
    .unlock             = avmnet_phy_11G_unlock,          \
    .trylock            = avmnet_phy_11G_trylock,         \
    .status_changed     = avmnet_phy_11G_status_changed,  \
    .set_status         = avmnet_phy_11G_set_status,      \
    .poll               = avmnet_phy_11G_poll,            \
    .setup_irq          = avmnet_phy_11G_setup_irq,       \
    .powerup            = avmnet_phy_11G_powerup,         \
    .powerdown          = avmnet_phy_11G_powerdown,       \
    .suspend            = avmnet_phy_11G_suspend,         \
    .resume             = avmnet_phy_11G_resume,          \
    .reinit             = avmnet_phy_11G_reinit



#define PHY11G_PHYPERF          0x10
#  define PYH11G_PHYPERF_FREQ   (0xff << 8)
#  define PHY11G_PHYPERF_SNR    (0xf  << 4)
#  define PHY11G_PHYPERF_LEN    (0xf  << 0)

#define PHY11G_PHYSTAT1			0x11
#  define PHY11G_PHYSTAT1_LSADS (1 << 8)
#  define PHY11G_PHYSTAT1_POLD	(1 << 7)
#  define PHY11G_PHYSTAT1_POLC 	(1 << 6)
#  define PHY11G_PHYSTAT1_POLB	(1 << 5)
#  define PHY11G_PHYSTAT1_POLA 	(1 << 4)
#  define PHY11G_PHYSTAT1_MDICD (1 << 3)
#  define PHY11G_PHYSTAT1_MDIAB (1 << 2)

#define PHY11G_PHYCTL1			0x13
#  define PHY11G_PHYCTL1_TLOOP_MASK	(7 << 13)
#  define PHY11G_PHYCTL1_TLOOP_NETL	(1 << 13)
#  define PHY11G_PHYCTL1_TLOOP_FETL	(2 << 13)
#  define PHY11G_PHYCTL1_TLOOP_ECHO	(3 << 13)
#  define PHY11G_PHYCTL1_TLOOP_RJTL	(4 << 13)
#  define PHY11G_PHYCTL1_TXOFF		(1 << 12)
#  define PHY11G_PHYCTL1_TXADJ_MASK	(0xf << 8)
#  define PHY11G_PHYCTL1_POLD		(1 << 7)
#  define PHY11G_PHYCTL1_POLC		(1 << 6)
#  define PHY11G_PHYCTL1_POLB		(1 << 5)
#  define PHY11G_PHYCTL1_POLA		(1 << 4)
#  define PHY11G_PHYCTL1_MDICD		(1 << 3)
#  define PHY11G_PHYCTL1_MDIAB		(1 << 2)
#  define PHY11G_PHYCTL1_TXEEE10	(1 << 1)
#  define PHY11G_PHYCTL1_AMDIX		(1 << 0)

#define PHY11G_ERRCNT           0x15
#  define PHY11G_ERRCNT_SEL_MASK        (0xf << 8)
#  define PHY11G_ERRCNT_SEL_RXERR       (0x0) // default
#  define PHY11G_ERRCNT_SEL_RXACT       (0x1 << 8)
#  define PHY11G_ERRCNT_SEL_ESDERR      (0x2 << 8)
#  define PHY11G_ERRCNT_SEL_SSDERR      (0x3 << 8)
#  define PHY11G_ERRCNT_SEL_TXERR       (0x4 << 8)
#  define PHY11G_ERRCNT_SEL_TXACT       (0x5 << 8)
#  define PHY11G_ERRCNT_SEL_COL         (0x6 << 8)
#  define PHY11G_ERRCNT_SEL_LINKDOWN    (0x8 << 8)
#  define PHY11G_ERRCNT_COUNT           (0xff)

#define PHY11G_MIISTAT          0x18u
#  define PHY11G_MIISTAT_PS     (3 << 4)
#  define PHY11G_MIISTAT_TX     (1 << 4)
#  define PHY11G_MIISTAT_RX     (1 << 5)
#  define PHY11G_MIISTAT_DPX    (1 << 3)
#  define PHY11G_MIISTAT_EEE    (1 << 2)
#  define PHY11G_MIISTAT_SPEED  (3 << 0)

#define PHY11G_IMASK            0x19u
#  define PHY11G_IMASK_WOL      (1 << 15)
#  define PHY11G_IMASK_MSRE     (1 << 14)
#  define PHY11G_IMASK_NPRX     (1 << 13)
#  define PHY11G_IMASK_NPTX     (1 << 12)
#  define PHY11G_IMASK_ANE      (1 << 11)
#  define PHY11G_IMASK_ANC      (1 << 10)
#  define PHY11G_IMASK_ADSC     (1 << 5)
#  define PHY11G_IMASK_MDIPC    (1 << 4)
#  define PHY11G_IMASK_MDIXC    (1 << 3)
#  define PHY11G_IMASK_DXMC     (1 << 2)
#  define PHY11G_IMASK_LSPC     (1 << 1)
#  define PHY11G_IMASK_LSTC     (1 << 0)

#define PHY11G_ISTAT            0x1Au
#define PHY11G_LED              0x1Bu
#define PHY11G_TPGCTRL          0x1Cu
#define PHY11G_TPGDATA          0x1Du
#define PHY11G_FWV              0x1Eu

#endif
