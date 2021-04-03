#if !defined(_AVM_NET_MDIO_BITBANG_H_)
#define _AVM_NET_MDIO_BITBANG_H_

#include "avmnet_debug.h"
#include "avmnet_config.h"
#include "avmnet_module.h"
#include "avmnet_common.h"
#include "athrs_phy_ctrl.h"

int avmnet_bb_mdio_init(avmnet_module_t *this);
int avmnet_bb_mdio_write(avmnet_module_t *this, uint32_t phy_addr, uint32_t reg, uint32_t data);
uint32_t avmnet_bb_mdio_read(avmnet_module_t *this, uint32_t phy_addr, uint32_t reg);

#endif
