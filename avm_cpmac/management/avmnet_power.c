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
#if defined(CONFIG_AVM_POWER)
#if __has_include(<avm/power/power.h>)
#include <avm/power/power.h>
#else
#include <linux/avm_power.h>
#endif
#endif /*--- #if defined(CONFIG_AVM_POWER) ---*/

#include <avm/net/net.h>
#include <avmnet_config.h>
#include <avmnet_debug.h>
#include "avmnet_mgmt.h"


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
extern struct avmnet_mgmt_struct avmnet_mgmt;


/******************************************************************************\
 * Powermanagement                                                            *
 *                                                                            *
 * To report reasonable values to the powermanagement module there has to be  *
 * distinguished between products with different hardware capabilities. As an *
 * example and base for the chosen values, here is an overview of values for  *
 * the 7320 with two Gigabit PHYs. The values were obtained in the urloader.  *
 *                                                                            *
 * two links with  overall power  derived values                              *
 *                                for one port                                *
 *   1 GiBits/s       4,6 W          550 mW                                   *
 * 100 MiBits/s       3,7 W          100 mW                                   *
 *  10 MiBits/s       3,6 W           50 mW                                   *
 * no link            3,5 W            0 mW                                   *
 *                                                                            *
\******************************************************************************/

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
/* TODO Work with individual values for different PHYs, switches and speeds */
/* TODO Is this data really used by the power management module of the system? */
#ifdef CONFIG_AVM_POWERMETER
static unsigned int power_array[avm_event_ethernet_speed_items] = {  0, /* no link      */
                                                                    50, /* 10 MiBits/s  */
                                                                   100, /* 100 MiBits/s */
                                                                   550  /* 1 GiBits/s   */
                                                                  };
#endif

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void avmnet_power_update_percentage(void) {
#   if defined(CONFIG_AVM_POWERMETER)
    unsigned char port;
    unsigned int power = 0, maxpower = 0, portset = 0;

    for(port = 0; port < avmnet_mgmt.event_data.ports; port++) {
        maxpower += power_array[avmnet_mgmt.event_data.port[port].maxspeed];
        power    += power_array[avmnet_mgmt.event_data.port[port].speed];
        if(avmnet_mgmt.event_data.port[port].link) {
            portset |= (1 << port);
        }
    }

    if(maxpower == 0){
        maxpower = power_array[avm_event_ethernet_speed_1G];
    }

    PowerManagmentRessourceInfo(powerdevice_ethernet, PM_ETHERNET_PARAM(portset, (100u * power) / maxpower));
#   endif /*--- #if defined(CONFIG_AVM_POWERMETER) ---*/
}


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
#if defined(CONFIG_AVM_POWER)
static int avmnet_power_config(int state) {
    union _powermanagment_ethernet_state pstate;
    struct ethtool_cmd cmd;
    __u32 current_advertising;
    unsigned char port;
    struct net_device *device;

    pstate.Register = state;
    port = (unsigned char) pstate.Bits.port;
    if(port >= avmnet_mgmt.number_of_ports) {
        AVMNET_ERR("[%s] tried to set mode for port %u\n", __FUNCTION__, port);
        return -1;
    }

    device = avmnet_mgmt.ports[port].device->device;
    if(0 != device->ethtool_ops->get_settings(device, &cmd)) {
        AVMNET_ERR("[%s] Error! Could not get the settings for the device '%s'!\n",
                   __FUNCTION__, avmnet_mgmt.ports[port].device->device_name);
        return -1;
    }
    current_advertising = cmd.advertising;
    avmnet_mgmt.power.throttle_on[port] = (unsigned char) (pstate.Bits.throttle_eth ? 1 : 0);
    cmd.advertising &= ~(ADVERTISED_1000baseT_Full | ADVERTISED_1000baseT_Half);
    if(pstate.Bits.throttle_eth == 0) {
        if(cmd.supported & SUPPORTED_1000baseT_Half) {
            cmd.advertising |= ADVERTISED_1000baseT_Half;
        }
        if(cmd.supported & SUPPORTED_1000baseT_Full) {
            cmd.advertising |= ADVERTISED_1000baseT_Full;
        }
    }
    if(cmd.advertising != current_advertising) {
        AVMNET_INFO("[%s] %sctivating ethernet throttle for port %u\n", 
                 __FUNCTION__, pstate.Bits.throttle_eth ? "A" : "Dea", port);
        if(0 != device->ethtool_ops->set_settings(device, &cmd)) {
            AVMNET_ERR("[%s] Error! Could not set the settings for the device '%s'!\n",
                       __FUNCTION__, avmnet_mgmt.ports[port].device->device_name);
            return -1;
        }
    }
    return 0;
}
#endif /*--- #if defined(CONFIG_AVM_POWER) ---*/


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_power_init(void) {
#   if defined(CONFIG_AVM_POWER)
    avmnet_mgmt.power.module_handle = PowerManagmentRegister("ethernet", avmnet_power_config);
#   endif /*--- #if defined(CONFIG_AVM_POWER) ---*/
    return 0;
}


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void avmnet_power_deinit(void) {
#ifdef CONFIG_AVM_POWER
    PowerManagmentRelease(avmnet_mgmt.power.module_handle);
    avmnet_mgmt.power.module_handle = NULL;
#endif
}

