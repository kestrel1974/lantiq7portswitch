/******************************************************************************
**
** FILE NAME    : ltq_eth_oam_netif.c
** PROJECT      : UGW
** MODULES      : Ethernet OAM
**
** DATE         : 5 APRIL 2013
** AUTHOR       : Purnendu Ghosh
** DESCRIPTION  : Driver for Ethernet OAM handling
** COPYRIGHT    :   Copyright (c) 2013
**                LANTIQ DEUTSCHLAND GMBH,
**                Lilienthalstrasse 15, 85579 Neubiberg, Germany
**
**    This program is free software; you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation; either version 2 of the License, or
**    (at your option) any later version.
**
** HISTORY
** $Date        $Author                $Comment
** 5 APRIL 2013 PURNENDU GHOSH         Initiate Version
*******************************************************************************/

/* modified by AVM */

#include <linux/netdevice.h>

/*
 * ####################################
 *            Local Variable
 * ####################################
 */


/*
 * ####################################
 *            Local Function
 * ####################################
 */

struct net_device * (*fp_ltq_eth_oam_dev)(void)=NULL;
EXPORT_SYMBOL(fp_ltq_eth_oam_dev);

