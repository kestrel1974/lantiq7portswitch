/*------------------------------------------------------------------------------------------*\
 *   Copyright (C) 2011,...,2015 AVM GmbH <fritzbox_info@avm.de>
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

#include <linux/init.h>
#include <linux/mm.h>
#if __has_include(<avm/event/event.h>)
#include <avm/event/event.h>
#else
#include <linux/avm_event.h>
#endif
#include <linux/avm_led_event.h>
#include <avmnet_debug.h>
#include <avmnet_module.h>
#include <avmnet_config.h>
#include "avmnet_mgmt.h"
#include "avmnet_links.h"
#include "avmnet_power.h"

extern struct avmnet_mgmt_struct avmnet_mgmt;

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
#if(AVMNET_DEBUG_LEVEL & AVMNET_DEBUG_LEVEL_TRACE)
static unsigned int avmnet_links_print_link_state_output(unsigned char *output, unsigned int length) {
    unsigned char port;
    unsigned int written = 0;

    if(length < 200) { /* Rough upper bound for output size */
        AVMNET_WARN("[%s] Buffer length (%u) too short for output.\n", __FUNCTION__, length);
        return 0;
    }
    written += snprintf(output + written, length - written, "Port stati: ");
    for(port = 0; port < avmnet_mgmt.event_data.ports; port++) {
        if(avmnet_mgmt.event_data.port[port].link) {
            written += snprintf(output + written,
                                AVMNET_DEBUG_BUFFER_LENGTH - written,
                                "[%u - %s%s (%s)]  ", 
                                port,
                                (avmnet_mgmt.event_data.port[port].speed == avm_event_ethernet_speed_1G) 
                                ? "1000" 
                                : ((avmnet_mgmt.event_data.port[port].speed == avm_event_ethernet_speed_100M) 
                                ? "100" 
                                : ((avmnet_mgmt.event_data.port[port].speed == avm_event_ethernet_speed_10M) 
                                ? "10" : "speed?")),
                                avmnet_mgmt.event_data.port[port].fullduplex ? " FD" : " HD",
                                (avmnet_mgmt.event_data.port[port].maxspeed == avm_event_ethernet_speed_1G) 
                                ? "1000" 
                                : ((avmnet_mgmt.event_data.port[port].maxspeed == avm_event_ethernet_speed_100M) 
                                ? "100" 
                                : ((avmnet_mgmt.event_data.port[port].maxspeed == avm_event_ethernet_speed_10M) 
                                ? "10" : "speed?"))
                                );
        } else {
            written += snprintf(output + written,
                                AVMNET_DEBUG_BUFFER_LENGTH - written,
                                "[%u - down (%s)]  ", port,
                                (avmnet_mgmt.event_data.port[port].maxspeed == avm_event_ethernet_speed_1G) 
                                ? "1000" 
                                : ((avmnet_mgmt.event_data.port[port].maxspeed == avm_event_ethernet_speed_100M) 
                                ? "100" 
                                : ((avmnet_mgmt.event_data.port[port].maxspeed == avm_event_ethernet_speed_10M) 
                                ? "10" : "speed?"))
                                );
        }
    }
    written += snprintf(output + written, AVMNET_DEBUG_BUFFER_LENGTH - written, "\n");
    return written;
}
#endif /*--- #if(AVMNET_DEBUG_LEVEL & AVMNET_DEBUG_LEVEL_TRACE) ---*/


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static void avmnet_links_print_link_state(void) {
#   if(AVMNET_DEBUG_LEVEL & AVMNET_DEBUG_LEVEL_TRACE)
    unsigned char *output;
    
    output = kmalloc(AVMNET_DEBUG_BUFFER_LENGTH + 1, GFP_KERNEL);
    if(output == NULL) {
        AVMNET_WARN("[%s] out of memory\n", __FUNCTION__);
        return;
    }
    avmnet_links_print_link_state_output(output, AVMNET_DEBUG_BUFFER_LENGTH);
    AVMNET_TRC("%s", output);
    kfree(output);
#   endif /*--- #if(AVMNET_DEBUG_LEVEL & AVMNET_DEBUG_LEVEL_TRACE) ---*/
}


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
#ifdef CONFIG_AVM_EVENT
static void avmnet_links_event_update(void)
{
    struct cpmac_event_struct *event_struct;

    AVMNET_INFOTRC("[%s]\n", __FUNCTION__);

    if(!avm_event_source_check_id(avmnet_mgmt.event_handle, avm_event_id_ethernet_connect_status)) {
        AVMNET_TRC("[%s] no one interested in event\n", __FUNCTION__);
        return;
    }
    if(!(event_struct = (struct cpmac_event_struct *) kmalloc(sizeof(struct cpmac_event_struct), GFP_ATOMIC))) {
        AVMNET_ERR("[%s] Error! No memory for status update!\n", __FUNCTION__);
        return;
    }

    *event_struct = avmnet_mgmt.event_data;
    event_struct->event_header.id = avm_event_id_ethernet_connect_status;
    avm_event_source_trigger(avmnet_mgmt.event_handle,
                             avm_event_id_ethernet_connect_status,
                             sizeof(struct cpmac_event_struct),
                             (void *) event_struct);
}

/*------------------------------------------------------------------------------------------*\
  Event update requested from application
\*------------------------------------------------------------------------------------------*/
static void avmnet_links_event_notify(void *context __attribute__ ((unused)), 
                                          enum _avm_event_id id) {
    switch(id) {
        case avm_event_id_ethernet_connect_status:
            avmnet_links_event_update();
            break;
        default:
            AVMNET_WARN("[%s] unhandled id %u!\n", __FUNCTION__, id);
            break;
    }
}


/*------------------------------------------------------------------------------------------*\
 * Klockwork beschwert sich hier über den Aufruf von avm_event_build_id_mask,
 * der Rückgabewert von avm_event_build_id_mask wird in avm_event_source_register überprüft!
 *
 * Diesen Code nicht ändern, um Klockwork zu befriedigen, der Code läuft sonst nicht mehr 
 * im Kernel 2.6.32 !
\*------------------------------------------------------------------------------------------*/
int avmnet_links_event_init(void) {
	struct _avm_event_id_mask id_mask;
    unsigned char port;

    AVMNET_INFO("[%s] running\n", __FUNCTION__);

    avmnet_mgmt.event_data.ports = avmnet_mgmt.number_of_ports;
    for(port = 0; port < avmnet_mgmt.number_of_ports; port++) {
        avmnet_mgmt.event_data.port[port].maxspeed = avmnet_mgmt.ports[port].maxspeed;
    }

    avmnet_mgmt.event_handle = avm_event_source_register("Ethernet status",
												  avm_event_build_id_mask(&id_mask, 1, avm_event_id_ethernet_connect_status),
                                                  avmnet_links_event_notify, NULL);
    if(!avmnet_mgmt.event_handle) {
        AVMNET_ERR("[%s] Error! Could not register avm_event source!\n", __FUNCTION__);
        return -1;
    }
    return 0;
}


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void avmnet_links_event_deinit(void) {
    avm_event_source_release(avmnet_mgmt.event_handle);
    avmnet_mgmt.event_handle = NULL;
    AVMNET_INFO("[%s] done\n", __FUNCTION__);
}

#else /* !CONFIG_AVM_EVENT */
static void avmnet_links_event_update(void)
{
	return;
}

int avmnet_links_event_init(void)
{
	return 0;
}

void avmnet_links_event_deinit(void)
{
	return;
}
#endif /* !CONFIG_AVM_EVENT */


#if((LED_EVENT_VERSION > 2) || (LED_EVENT_SUBVERSION >= 12))
#   define MAX_LED_EVENT 5
#else /*--- #if((LED_EVENT_VERSION > 2) || (LED_EVENT_SUBVERSION >= 12)) ---*/
#   define MAX_LED_EVENT 4
#endif /*--- #else ---*/ /*--- #if((LED_EVENT_VERSION > 2) || (LED_EVENT_SUBVERSION >= 12)) ---*/
static enum _led_event led_event_codes[MAX_LED_EVENT][2] = {
    { event_lan1_active, event_lan1_inactive, },
    { event_lan2_active, event_lan2_inactive, },
    { event_lan3_active, event_lan3_inactive, },
    { event_lan4_active, event_lan4_inactive, },
#   if (LED_EVENT_SUBVERSION >= 12)
    { event_lan5_active, event_lan5_inactive, },
#   endif /*--- #if (LED_EVENT_SUBVERSION >= 12) ---*/
};
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void avmnet_links_port_update(avmnet_device_t *device_id) {
    unsigned char port = device_id->external_port_no; 
    unsigned char link = device_id->status.Details.link; 
    enum _avm_event_ethernet_speed speed;
    unsigned char fullduplex = device_id->status.Details.fullduplex;

    if(port == 255) {
        AVMNET_INFOTRC("[%s] no external port\n", __FUNCTION__);
        return;
    }
    if(port >= AVM_EVENT_ETH_MAXPORTS) {
        AVMNET_ERR("[%s] port (%u) exceeds event system port maximum\n", __FUNCTION__, port);
        return;
    }
    switch(device_id->status.Details.speed) {
        case 0: speed = avm_event_ethernet_speed_10M; break;
        case 1: speed = avm_event_ethernet_speed_100M; break;
        case 2: speed = avm_event_ethernet_speed_1G; break;
        default:
            AVMNET_WARN("[%s] Illegal speed value (%u) given!\n", 
                       __FUNCTION__, device_id->status.Details.speed);
            return;
    }

    if(port >= avmnet_mgmt.event_data.ports) {
        AVMNET_WARN("[%s] Maximum port number %u exceeded with %u!\n", 
                   __FUNCTION__, avmnet_mgmt.event_data.ports, port);
    }
    if(   (avmnet_mgmt.event_data.port[port].link == link)
       && (avmnet_mgmt.event_data.port[port].speed == speed)
       && (avmnet_mgmt.event_data.port[port].fullduplex == fullduplex)) {
        /* No link details changed */
//        AVMNET_TRC("[%s] Got port %u update without changes\n", __FUNCTION__, port);
        return;
    }

    if(led_event_action && (port < MAX_LED_EVENT)) {
        enum _led_event event_code = led_event_codes[port][link ? 0 : 1];

#       if((LED_EVENT_VERSION > 2) || (LED_EVENT_SUBVERSION >= 12))
        if(device_id->flags & AVMNET_CONFIG_FLAG_SWITCH_WAN) {
            event_code = link ? event_wan_active : event_wan_inactive;
        }
#       endif /*--- #if(LED_EVENT_VERSION > 2) || (LED_EVENT_SUBVERSION >= 12)) ---*/

        if( ! (device_id->flags & AVMNET_CONFIG_FLAG_USE_GPON)) {
        (*led_event_action)(LEDCTL_VERSION, event_code, 1);
        }
    }

    avmnet_mgmt.event_data.port[port].link = link;
    avmnet_mgmt.event_data.port[port].speed = speed;
    avmnet_mgmt.event_data.port[port].fullduplex = fullduplex;

    avmnet_links_print_link_state();
    avmnet_power_update_percentage();
    avmnet_links_event_update();
}


