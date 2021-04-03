
#if !defined(__AVM_NET_COMMON_)
#define __AVM_NET_COMMON_

#include <linux/version.h>
#include <linux/netdevice.h>
#include <avmnet_module.h>

int avmnet_set_macaddr(int device_nr, struct net_device *dev);

extern int avmnet_generic_init(avmnet_module_t *this);
extern int avmnet_generic_setup(avmnet_module_t *this);
extern int avmnet_generic_exit(avmnet_module_t *this __maybe_unused);
extern struct net_device_stats * avmnet_generic_get_stats(struct net_device *dev);

int avmnet_timer_add(avmnet_module_t *module, avmnet_timer_enum_t type);
int avmnet_timer_del(avmnet_module_t *module, avmnet_timer_enum_t type);

unsigned int avmnet_tokenize(char          *str, 
                             const char    *delim,
                             unsigned int   max_tokens,
                             char         **tokens);

unsigned int calc_advertise_pause_param( unsigned int mac_support_sym, unsigned int mac_support_asym, enum avmnet_pause_setup pause_setup );

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0)
#define PDE_DATA(_inode) (PDE(_inode)->data)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 35)
#define DEFINE_SEMAPHORE(name)	\
	struct semaphore name = __SEMAPHORE_INITIALIZER(name, 1)
#endif

extern const char *avmnet_version;
extern int avmnet_set_macaddr(int device_nr, struct net_device *dev);

#endif
