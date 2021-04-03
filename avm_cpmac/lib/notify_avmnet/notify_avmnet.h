#ifndef NOTIFY_AVMNET_H
#define NOTIFY_AVMNET_H

#include <stdint.h>

/*
 * BROADBAND_TYPE is the reference to our caller
 * DSL / multid in ATA Scenario
 */
#define BROADBAND_TYPE_DSL   1
#define BROADBAND_TYPE_ATA   2

/*
 * PPA Kernel Module State
 */
#define TRANSFER_MODE_NONE  0
#define TRANSFER_MODE_ATM   1
#define TRANSFER_MODE_PTM   2
#define TRANSFER_MODE_ETH   4
#define TRANSFER_MODE_KDEV  8 // kernel attached device (like WLAN, LTE, UTMS or other
                              // usb-devices which need data beeing routed via the kernel)

/*
 * Start Training using transfer_mode
 */
void notify_avmnet_hw_start_selected_tm( uint32_t transfer_mode, uint32_t broadband_type );

/*
 * DSL Showtime using transfer_mode
 */
void notify_avmnet_hw_link_established( uint32_t transfer_mode, uint32_t broadband_type );

/*
 * obsolete: was used to setup ATA, use notify_avmnet_hw_setup_wan_dev now
 */
void notify_avmnet_hw_disable_phy( uint32_t broadband_type );

/*
 * setup PPA for AVM_ATA-Mode
 * WAN_DEV is Ethernet-Device
 * wan_dev_name:
 *                  NULL - no dedicated WAN_DEV, e.g. IP-Client-Mode
 *                  eth[0-4]
 */
void notify_avmnet_hw_setup_ata_kdev( char *wan_dev_name );

/*
 * setup PPA for AVM_ATA-Mode
 * WAN_DEV is kernel attached device (like WLAN, LTE, UTMS or other
 * usb-devices which need data beeing routed via the kernel)
 *
 * wan_dev_name:
 *                  NULL - no dedicated WAN_DEV, e.g. IP-Client-Mode
 *                  eth[0-4]
 */
void notify_avmnet_hw_setup_ata_eth_dev( char *wan_dev_name );


/*
 * shutdown PPA
 */
void notify_avmnet_hw_exit( uint32_t broadband_type );

#endif /*--- #ifndef NOTIFY_AVMNET_H ---*/
