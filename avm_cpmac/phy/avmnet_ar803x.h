#if !defined(_AVM_NET_AR803X_H_)
#define _AVM_NET_AR803X_H_

#include <linux/ethtool.h>

struct athphy_config
{
    unsigned int supported;
    unsigned int advertise;
    unsigned int autoneg;
    unsigned int speed;
    unsigned int duplex;
    unsigned int port;
    unsigned int mdi;
    enum avmnet_pause_setup pause_setup;
};

enum link_state
{
    LINK_DOWN = 0,
    LINK_UP,
    LINK_LOST,
    LINK_FAILED,
    LINK_PWRDOWN,
    LINK_THRASHING,
};

#define LINK_THRASH_TIME 60
#define LINK_THRASH_LIMIT 5
#define LINK_DOWN_DELAY 5

#define AVMNET_GPIO_QUEUELEN 8
struct avmnet_gpio
{
    unsigned int    read;
    unsigned int    write;
    unsigned short  queue[AVMNET_GPIO_QUEUELEN];
};

struct avmnet_bbmdio
{
    unsigned int   clk;
    unsigned int   data;
    unsigned int   delay;
};

typedef struct avmnet_work_struct
{
    struct workqueue_struct *workqueue;
    struct delayed_work work;
} avmnet_work_t;

struct athphy_context {
    avmnet_module_t         *this_module;
    spinlock_t              irq_lock;
    avmnet_work_t           link_work;
    avmnet_work_t           gpio_work;
    unsigned int            phyAddr;
    int                     irq;
    unsigned int            irq_disabled;
    unsigned int            gbit;
    struct athphy_config    config;
    enum link_state         state;
    long unsigned int       lost_time;
    long unsigned int       thrash_time;
    unsigned int            thrash_count;
    long unsigned int       jabber_time;
    unsigned long           linkf_time;
    avmnet_linkstatus_t     last_status;
    unsigned int            irq_on;
    unsigned int            powerdown;
    unsigned long           linkdown_delay;
    struct avmnet_gpio      gpio;
    struct avmnet_bbmdio    bb_mdio;
};

int avmnet_ar803x_status_poll(avmnet_module_t *this_module);
int avmnet_ar803x_setup_interrupt(avmnet_module_t *this_module, unsigned int on_off);
int avmnet_ar803x_setup(avmnet_module_t *this_module);
int avmnet_ar803x_init(avmnet_module_t *this_module);
int avmnet_ar803x_exit(avmnet_module_t *this_module);
int avmnet_ar803x_powerup(avmnet_module_t *this);
int avmnet_ar803x_powerdown(avmnet_module_t *this);
unsigned int avmnet_ar803x_reg_read(avmnet_module_t *this, unsigned int addr, unsigned int reg);
int avmnet_ar803x_reg_write(avmnet_module_t *this, unsigned int addr, unsigned int reg, unsigned int val);
int avmnet_ar803x_lock(avmnet_module_t *this);
void avmnet_ar803x_unlock(avmnet_module_t *this);
int avmnet_ar803x_trylock(avmnet_module_t *this);
void avmnet_ar803x_status_changed(avmnet_module_t *this, avmnet_module_t *child);
int avmnet_ar803x_suspend(avmnet_module_t *this, avmnet_module_t *caller);
int avmnet_ar803x_resume(avmnet_module_t *this, avmnet_module_t *caller);
int avmnet_ar803x_reinit(avmnet_module_t *this);

#if defined(CONFIG_ATHRS17_PHY)
int avmnet_athrs17_setup_phy(avmnet_module_t *this);
#endif
int avmnet_4010_setup_phy(avmnet_module_t *this);

#if defined(CONFIG_VR9)
int avmnet_hw223_fiber_poll(avmnet_module_t *this_module);
#endif

int avmnet_ar803x_ethtool_get_settings(avmnet_module_t *this, struct ethtool_cmd *cmd);
int avmnet_ar803x_ethtool_set_settings(avmnet_module_t *this, struct ethtool_cmd *cmd);
u32 avmnet_ar803x_ethtool_get_link(avmnet_module_t *this_module);
void avmnet_ar803x_ethtool_get_pauseparam(avmnet_module_t *this, struct ethtool_pauseparam *param);
int avmnet_ar803x_ethtool_set_pauseparam(avmnet_module_t *this, struct ethtool_pauseparam *param);

int avmnet_ar8033_gpio_set(avmnet_module_t *, unsigned int gpio, unsigned int on_off);

#define AR803X_ETHOPS \
{ \
    .get_settings   = avmnet_ar803x_ethtool_get_settings,      \
    .set_settings   = avmnet_ar803x_ethtool_set_settings,      \
    .get_pauseparam = avmnet_ar803x_ethtool_get_pauseparam,    \
    .set_pauseparam = avmnet_ar803x_ethtool_set_pauseparam,    \
    .get_link       = avmnet_ar803x_ethtool_get_link,          \
}

#define AR803X_STDFUNCS \
    .init               = avmnet_ar803x_init,            \
    .setup              = avmnet_ar803x_setup,           \
    .exit               = avmnet_ar803x_exit,            \
    .reg_read           = avmnet_ar803x_reg_read,        \
    .reg_write          = avmnet_ar803x_reg_write,       \
    .lock               = avmnet_ar803x_lock,            \
    .unlock             = avmnet_ar803x_unlock,          \
    .trylock            = avmnet_ar803x_trylock,         \
    .status_changed     = avmnet_ar803x_status_changed,  \
    .poll               = avmnet_ar803x_status_poll,     \
    .setup_irq          = avmnet_ar803x_setup_interrupt, \
    .powerup            = avmnet_ar803x_powerup,         \
    .powerdown          = avmnet_ar803x_powerdown,       \
    .suspend            = avmnet_ar803x_suspend,         \
    .resume             = avmnet_ar803x_resume,          \
    .reinit             = avmnet_ar803x_reinit



int avmnet_ar8033_status_poll(avmnet_module_t *this_module);
int avmnet_ar8033_setup_interrupt(avmnet_module_t *this_module, unsigned int on_off);
int avmnet_ar8033_setup(avmnet_module_t *this_module);
int avmnet_ar8033_init(avmnet_module_t *this_module);
int avmnet_ar8033_exit(avmnet_module_t *this_module);
int avmnet_ar8033_powerup(avmnet_module_t *this);
int avmnet_ar8033_powerdown(avmnet_module_t *this);
unsigned int avmnet_ar8033_reg_read(avmnet_module_t *this, unsigned int addr, unsigned int reg);
int avmnet_ar8033_reg_write(avmnet_module_t *this, unsigned int addr, unsigned int reg, unsigned int val);
int avmnet_ar8033_lock(avmnet_module_t *this);
void avmnet_ar8033_unlock(avmnet_module_t *this);
int avmnet_ar8033_trylock(avmnet_module_t *this);
void avmnet_ar8033_status_changed(avmnet_module_t *this, avmnet_module_t *child);
int avmnet_ar8033_suspend(avmnet_module_t *this, avmnet_module_t *caller);
int avmnet_ar8033_resume(avmnet_module_t *this, avmnet_module_t *caller);
int avmnet_ar8033_reinit(avmnet_module_t *this);

int avmnet_ar8033_ethtool_get_settings(avmnet_module_t *this, struct ethtool_cmd *cmd);
int avmnet_ar8033_ethtool_set_settings(avmnet_module_t *this, struct ethtool_cmd *cmd);
u32 avmnet_ar8033_ethtool_get_link(avmnet_module_t *this_module);
void avmnet_ar8033_ethtool_get_pauseparam(avmnet_module_t *this, struct ethtool_pauseparam *param);
int avmnet_ar8033_ethtool_set_pauseparam(avmnet_module_t *this, struct ethtool_pauseparam *param);

#define AR8033_ETHOPS \
{ \
    .get_settings   = avmnet_ar8033_ethtool_get_settings,      \
    .set_settings   = avmnet_ar8033_ethtool_set_settings,      \
    .get_pauseparam = avmnet_ar8033_ethtool_get_pauseparam,    \
    .set_pauseparam = avmnet_ar8033_ethtool_set_pauseparam,    \
    .get_link       = avmnet_ar8033_ethtool_get_link,          \
}

#define AR8033_STDFUNCS \
    .init               = avmnet_ar8033_init,            \
    .setup              = avmnet_ar8033_setup,           \
    .exit               = avmnet_ar8033_exit,            \
    .reg_read           = avmnet_ar8033_reg_read,        \
    .reg_write          = avmnet_ar8033_reg_write,       \
    .lock               = avmnet_ar8033_lock,            \
    .unlock             = avmnet_ar8033_unlock,          \
    .trylock            = avmnet_ar8033_trylock,         \
    .status_changed     = avmnet_ar8033_status_changed,  \
    .poll               = avmnet_ar8033_status_poll,     \
    .setup_irq          = avmnet_ar8033_setup_interrupt, \
    .powerup            = avmnet_ar8033_powerup,         \
    .powerdown          = avmnet_ar8033_powerdown,       \
    .suspend            = avmnet_ar8033_suspend,         \
    .resume             = avmnet_ar8033_resume,          \
    .reinit             = avmnet_ar8033_reinit

#endif
