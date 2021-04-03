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

#include <linux/mm.h>
#if __has_include(<avm/event/event.h>)
#include <avm/event/event.h>
#else
#include <linux/avm_event.h>
#endif

#include <avm/net/net.h>
#include <avmnet_debug.h>
#include <avmnet_config.h>
#include "avmnet_mgmt.h"
#include "avmnet_links.h"
#include "avmnet_power.h"


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
struct avmnet_mgmt_struct avmnet_mgmt = {};


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static int avmnet_mgmt_init_ports(void) {
    unsigned char dev_nr, port;
    avmnet_device_t *device;

    avmnet_mgmt.number_of_ports = 0;
    for(dev_nr = 0; dev_nr < avmnet_hw_config_entry->nr_avm_devices; dev_nr++) {
        port = avmnet_hw_config_entry->avm_devices[dev_nr]->external_port_no;
        if(port == 255) {
            continue;
        }
        if(port > AVM_EVENT_ETH_MAXPORTS - 1) {
            AVMNET_ERR("[%s] Error! External port (%u) for device '%s' too big!  Aborting!\n", 
                       __FUNCTION__, port,
                       avmnet_hw_config_entry->avm_devices[dev_nr]->device_name);
            return -1;
        }
        if(avmnet_mgmt.ports[port].device != 0) {
            AVMNET_ERR("[%s] Error! External port (%u) given for two devices '%s' and '%s'!  Aborting!\n", 
                       __FUNCTION__, port,
                       avmnet_hw_config_entry->avm_devices[dev_nr]->device_name,
                       avmnet_mgmt.ports[port].device->device_name);
            return -1;
        }
        avmnet_mgmt.ports[port].device = avmnet_hw_config_entry->avm_devices[dev_nr];
        avmnet_mgmt.number_of_ports++;
    }
    for(port = 0; port < avmnet_mgmt.number_of_ports; port++) {
        struct ethtool_cmd cmd;

        device = avmnet_mgmt.ports[port].device;

        if(device == NULL) {
            AVMNET_ERR("[%s] Error! No device for external port %u! Aborting!\n", __FUNCTION__, port);
            return -1;
        }

        if(0 != device->device->ethtool_ops->get_settings(device->device, &cmd)) {
            AVMNET_ERR("[%s] Error! Could not get the settings for the device '%s'!\n",
                       __FUNCTION__, device->device_name);
            return -1;
        }
        if(   (cmd.supported & SUPPORTED_1000baseT_Full)
           || (cmd.supported & SUPPORTED_1000baseT_Half)) {
            avmnet_mgmt.ports[port].maxspeed = avm_event_ethernet_speed_1G;
        } else if(   (cmd.supported & SUPPORTED_100baseT_Full)
                  || (cmd.supported & SUPPORTED_100baseT_Half)) {
            avmnet_mgmt.ports[port].maxspeed = avm_event_ethernet_speed_100M;
        } else if(   (cmd.supported & SUPPORTED_10baseT_Full)
                  || (cmd.supported & SUPPORTED_10baseT_Half)) {
            avmnet_mgmt.ports[port].maxspeed = avm_event_ethernet_speed_10M;
        }
        AVMNET_INFO("[%s] Port %u with maxspeed %s\n", __FUNCTION__, port,
            (avmnet_mgmt.ports[port].maxspeed == avm_event_ethernet_speed_1G) ? "1G" :
            (avmnet_mgmt.ports[port].maxspeed == avm_event_ethernet_speed_100M) ? "100M" :
            (avmnet_mgmt.ports[port].maxspeed == avm_event_ethernet_speed_10M) ? "10M" : "undefined");
    }

    AVMNET_INFO("[%s] This hardware has %u external ethernet ports\n", __FUNCTION__, avmnet_mgmt.number_of_ports);

    return 0;
}


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_mgmt_init(void) {
    int ret;

    AVMNET_INFO("[%s]\n", __FUNCTION__);

    if(0 != (ret = avmnet_mgmt_init_ports())) {
        AVMNET_ERR("[%s] ERROR: avmnet_mgmt_init_ports failed\n", __func__);
        return ret;
    }

    if(0 != (ret = avmnet_links_event_init())) {
        AVMNET_ERR("[%s] ERROR: avmnet_links_event_init failed\n", __func__);
        return ret;
    }

    ret = avmnet_power_init();
    if(ret) {
        AVMNET_ERR("[%s] ERROR: avmnet_power_init failed\n", __func__);
        return ret;
    }
    return ret;
}


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void avmnet_deinit_mgmt(void) {
    AVMNET_INFO("[%s]\n", __FUNCTION__);

    avmnet_power_deinit();
    avmnet_links_event_deinit();
}

