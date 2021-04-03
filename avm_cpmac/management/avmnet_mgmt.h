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

#ifndef _AVMNET_MGMT_H_
#define _AVMNET_MGMT_H_
#if __has_include(<avm/event/event.h>)
#include <avm/event/event.h>
#else
#include <linux/avm_event.h>
#endif
#include <avmnet_module.h>

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int avmnet_mgmt_init(void);
void avmnet_mgmt_deinit(void);


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
struct avmnet_mgmt_struct {
    unsigned char number_of_ports;
    struct {
        avmnet_device_t                *device;
        enum _avm_event_ethernet_speed  maxspeed;
    } ports[AVM_EVENT_ETH_MAXPORTS];
    struct {
        void          *module_handle;
        unsigned char  usage_percentage[avm_event_ethernet_speed_items];
        unsigned char  throttle_on[AVM_EVENT_ETH_MAXPORTS];
    } power;
    struct cpmac_event_struct  event_data;
    void                      *event_handle;
};

#endif /*--- #ifndef _AVMNET_MGMT_H_ ---*/

