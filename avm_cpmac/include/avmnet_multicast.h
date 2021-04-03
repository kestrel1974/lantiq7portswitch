/*------------------------------------------------------------------------------------------*\
 *   Copyright (C) 2012 AVM GmbH <fritzbox_info@avm.de>
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

#ifndef _AVMNET_MULTICAST_H_
#define _AVMNET_MULTICAST_H_

#ifdef CONFIG_IP_MULTICAST_FASTFORWARD

extern int avmnet_mcfw_init(avmnet_device_t *avm_dev);
extern void avmnet_mcfw_exit(avmnet_device_t *avm_dev);
extern void avmnet_mcfw_enable(struct net_device *dev);
extern void avmnet_mcfw_disable(struct net_device *dev);

#else /* !CONFIG_IP_MULTICAST_FASTFORWARD */

static inline int avmnet_mcfw_init(avmnet_device_t *_d __maybe_unused)
{
        return 0;
}

static inline void avmnet_mcfw_exit(avmnet_device_t *_d __maybe_unused)
{
        return;
}

static inline void avmnet_mcfw_enable(struct net_device *dev __maybe_unused)
{
        return;
}

static inline void avmnet_mcfw_disable(struct net_device *dev __maybe_unused)
{
        return;
}

#endif

#endif /*--- #ifndef _AVMNET_MULTICAST_H_ ---*/

