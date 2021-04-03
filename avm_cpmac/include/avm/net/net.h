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

#ifndef LINUX_AVMNET_H
#define LINUX_AVMNET_H

#include <linux/atmdev.h>
#include <linux/netdevice.h>

// These Flags will enable the following features in kdsld:
#if defined(CONFIG_IFX_PPA_QOS)
#define AVMNET_VCC_QOS
#define AVMNET_RATE_SHAPING
#endif

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
enum avmnet_phy_power_mode {
    AVMNET_PHY_POWER_OFF = 0,
    AVMNET_PHY_POWER_SAVE = 1,
    AVMNET_PHY_POWER_ON = 2,
    AVMNET_PHY_POWER_NUMBER_OF_MODES = 3
};

enum avm_cpmac_magpie_reset
{
    CPMAC_MAGPIE_RESET_ON = 0,
    CPMAC_MAGPIE_RESET_OFF = 1,
    CPMAC_MAGPIE_RESET_PULSE = 2
};

struct avmnet_master_dev {
    struct net_device *master_dev;
    int (*mdio_write)(struct avmnet_master_dev *master, uint32_t addr, uint32_t reg, uint16_t data);
    int (*mdio_read)(struct avmnet_master_dev *master, uint32_t addr, uint32_t reg, uint16_t *data);
    void *priv;
};

extern int avmnet_register_master(struct avmnet_master_dev *master);
extern int avmnet_unregister_master(struct avmnet_master_dev *master);
extern struct avmnet_master_dev * avmnet_get_master(void);

/*------------------------------------------------------------------------------------------*\
 * Steuern der GPIOs die an den Phys angeschlossen sind
 *  0 -  7  eth0
 *  7 - 15  eth1
 * 16 - 23  eth2
 * 24 - 31  eth3
 * 32 - 39  eth4
 *  ....
\*------------------------------------------------------------------------------------------*/
void avmnet_gpio(unsigned int gpio, unsigned int on);


/*
* returns ret:
*   ret == 0: setup qos queues successful
*   ret <  0: setup qos queue failure
*/
extern int (*vcc_set_nr_qos_queues)(struct atm_vcc *, unsigned int);


/*
* returns ret:
*   ret == 0: success
*   ret != 0: failure
*/
extern int (*vcc_map_skb_prio_qos_queue)(struct atm_vcc *, unsigned int, unsigned int);

/*
* returns ret:
* 	ret == 0: success
* 	ret == -1: failure
* 	ret == 1: rate shaping not possible on device - hw acl disabled for tx path
*/
extern int avmnet_set_wan_tx_rate_shaping(unsigned int kbps);

/*
* returns ret:
* 	ret == 0: success
* 	ret != 0: failure
*/
extern int avmnet_set_wan_tx_queue_rate_shaping(unsigned int qid, unsigned int kbps);
/*
* returns ret:
* 	ret == 0: success
* 	ret != 0: failure
*/
extern int avmnet_disable_wan_tx_rate_shaping(void);



#endif /*--- #ifndef LINUX_AVMNET_H ---*/

