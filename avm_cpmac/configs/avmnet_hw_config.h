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

#if !defined(__AVM_NET_HW_CONFIG_)
#define __AVM_NET_HW_CONFIG_

#include <avmnet_module.h>
#include <avmnet_config.h>

#if defined(CONFIG_VR9)
#include "config_3370.h"
#include "config_3370_1.h"
#include "config_6840.h"
#include "config_7360.h"
#include "config_HW185.h"
#include "config_HW202.h"
#include "config_HW209.h"
#include "config_HW218.h"
#endif

#if defined(CONFIG_VR9) && defined(CONFIG_ATHRS17_PHY)
#include "config_HW223.h"
#include "config_HW243.h"
#endif

#if defined(CONFIG_AR9)
#include "config_7320.h"
#include "config_7330.h"
#include "config_7312.h"
#endif

#if defined(CONFIG_AR10)
#include "config_HW192.h"
#include "config_HW198.h"
#endif

#if (defined(CONFIG_MACH_ATHEROS) && defined(CONFIG_MACH_AR724x)) || (defined(CONFIG_ATH79) && defined(CONFIG_SOC_AR724X)) 
#include "config_HW173.h"
#endif

#if (defined(CONFIG_MACH_ATHEROS) && defined(CONFIG_MACH_AR934x)) || (defined(CONFIG_ATH79) && defined(CONFIG_SOC_AR934X))
#include "config_HW180.h"
#include "config_6841.h"
#include "config_HW190.h"
#include "config_HW194.h"
#include "config_HW195.h"
#include "config_HW201v2.h"
#include "config_HW207.h"
#endif
#if (defined(CONFIG_MACH_ATHEROS) && defined(CONFIG_MACH_QCA955x)) || (defined(CONFIG_ATH79) && defined(CONFIG_SOC_QCA955X))
#if defined(CONFIG_ATH79_MACH_AVM_HW238)
#include "config_HW238.h"
#else
#include "config_HW200.h"
#include "config_HW205.h"
#include "config_HW214.h"
#endif
#endif

#if defined(CONFIG_SOC_QCN550X)
#include "config_HW240.h"
#include "config_HW241.h"
#include "config_HW263.h"
#endif

#if (defined(CONFIG_MACH_ATHEROS) && defined(CONFIG_MACH_QCA953x)) || (defined(CONFIG_ATH79) && defined(CONFIG_SOC_QCA953X))
#define WLAN_PROD_TEST 254
#include "config_HW215.h"
#include "config_HW216.h"
#include "config_HW222.h"
#include "config_HW222wtest.h"
#endif

#if (defined(CONFIG_ARCH_QCOM))
#include "config_HW211.h"
#endif

#if (defined(CONFIG_MACH_ATHEROS) && defined(CONFIG_MACH_QCA956x)) || (defined(CONFIG_ATH79) && defined(CONFIG_SOC_QCA956X))
/*--- #include "config_HW217.h" ---*/
#include "config_HW219.h"
#endif

#if (defined(CONFIG_ARCH_GEN3))
#include "config_HW220x.h"
#include "config_HW231x.h"
#endif

#ifndef NUM_ENTITY
#if defined(NUM_ENTITY)
#  undef NUM_ENTITY
#endif /*--- #if defined(NUM_ENTITY) ---*/
#define NUM_ENTITY(x)   (sizeof(x) / sizeof(*(x)))
#endif
#define AVMNET_HW_CONFIG_TABLE_SIZE (sizeof avmnet_hw_config_table / sizeof(struct avmnet_hw_config_entry_struct))

struct avmnet_hw_config_entry_struct avmnet_hw_config_table[] =
{
#if defined(CONFIG_AR9)
    {
        // 7320
        .hw_id = 172,
        .config = &hw172_avmnet,
        .nr_avm_devices = NUM_ENTITY( hw172_avmnet_avm_devices ),
        .avm_devices = hw172_avmnet_avm_devices
    },
    {
        // 7330
        .hw_id = 179,
        .config = &hw179_avmnet,
        .nr_avm_devices = NUM_ENTITY( hw179_avmnet_avm_devices ),
        .avm_devices = hw179_avmnet_avm_devices
    },
    {
        // 7330 SL
        .hw_id = 188,
        .config = &hw179_avmnet,
        .nr_avm_devices = NUM_ENTITY( hw179_avmnet_avm_devices ),
        .avm_devices = hw179_avmnet_avm_devices
    },
    {
        // 7312
        .hw_id = 189,
        .config = &hw189_avmnet,
        .nr_avm_devices = NUM_ENTITY( hw189_avmnet_avm_devices ),
        .avm_devices = hw189_avmnet_avm_devices
    },
#endif // defined(CONFIG_AR9)

#if defined(CONFIG_AR10)
    {
        .hw_id = 192,
        .config = &hw192_avmnet,
        .nr_avm_devices = NUM_ENTITY( hw192_avmnet_avm_devices ),
        .avm_devices = hw192_avmnet_avm_devices
    },
    {
        .hw_id = 198,
        .config = &hw198_avmnet,
        .nr_avm_devices = NUM_ENTITY( hw198_avmnet_avm_devices ),
        .avm_devices = hw198_avmnet_avm_devices
    },
#endif // defined(CONFIG_AR10)

#if defined(CONFIG_VR9)
    {   // 3370
        .hw_id = 175, 
        .config = &hw175_avmnet,
        .nr_avm_devices = NUM_ENTITY( hw175_avmnet_avm_devices ),
        .avm_devices = hw175_avmnet_avm_devices
    },
    {
        // 3370 alternatives Profil, Port 1 aufgespalten in 2*100MBit
        .hw_id = 175,
        .profile_id = 1,
        .config = &hw175_1_avmnet,
        .nr_avm_devices = NUM_ENTITY( hw175_1_avmnet_avm_devices ),
        .avm_devices = hw175_1_avmnet_avm_devices
    },
    {
        // 6840
        .hw_id = 177,
        .config = &hw177_avmnet,
        .nr_avm_devices = NUM_ENTITY( hw177_avmnet_avm_devices ),
        .avm_devices = hw177_avmnet_avm_devices
    },
    {   // 7360 SL
        .hw_id = 181,
        .config = &hw181_avmnet,
        .nr_avm_devices = NUM_ENTITY( hw181_avmnet_avm_devices ),
        .avm_devices = hw181_avmnet_avm_devices
    },
    {
         // 7360
        .hw_id = 183,
        .config = &hw181_avmnet,
        .nr_avm_devices = NUM_ENTITY( hw181_avmnet_avm_devices ),
        .avm_devices = hw181_avmnet_avm_devices
    },
    {
         // 7360 v2 (32MB)
        .hw_id = 196,
        .config = &hw181_avmnet,
        .nr_avm_devices = NUM_ENTITY( hw181_avmnet_avm_devices ),
        .avm_devices = hw181_avmnet_avm_devices
    },
    {
         // 7362
        .hw_id = 202,
        .config = &hw202_avmnet,
        .nr_avm_devices = NUM_ENTITY( hw202_avmnet_avm_devices ),
        .avm_devices = hw202_avmnet_avm_devices
    },
    {
         // 7362 SL
        .hw_id = 203,
        .config = &hw202_avmnet,
        .nr_avm_devices = NUM_ENTITY( hw202_avmnet_avm_devices ),
        .avm_devices = hw202_avmnet_avm_devices
    },
    {
         // 7412
        .hw_id = 209,
        .config = &hw209_avmnet,
        .nr_avm_devices = NUM_ENTITY( hw209_avmnet_avm_devices ),
        .avm_devices = hw209_avmnet_avm_devices
    },
    {
         // 7391
        .hw_id = 185,
        .config = &hw185_avmnet,
        .nr_avm_devices = NUM_ENTITY( hw185_avmnet_avm_devices ),
        .avm_devices = hw185_avmnet_avm_devices
    },
    {
        // 3390
        .hw_id = 193,
        .config = &hw185_avmnet,
        .nr_avm_devices = NUM_ENTITY( hw185_avmnet_avm_devices ),
        .avm_devices = hw185_avmnet_avm_devices
    },
    {
        // 3490 
        .hw_id = 212,
        .config = &hw185_avmnet,
        .nr_avm_devices = NUM_ENTITY( hw185_avmnet_avm_devices ),
        .avm_devices = hw185_avmnet_avm_devices
    },
    {
         // 7430
        .hw_id = 218,
        .config = &hw218_avmnet,
        .nr_avm_devices = NUM_ENTITY( hw218_avmnet_avm_devices ),
        .avm_devices = hw218_avmnet_avm_devices
    },
#endif // defined(CONFIG_VR9)
#if defined(CONFIG_VR9) && defined(CONFIG_ATHRS17_PHY)
    {
         // 5490-Fiber
        .hw_id = 223,
        .config = &hw223_avmnet,
        .nr_avm_devices = NUM_ENTITY( hw223_avmnet_avm_devices ),
        .avm_devices = hw223_avmnet_avm_devices
    },
    {
         // 5491-Fiber
        .hw_id = 243,
        .config = &hw243_avmnet,
        .nr_avm_devices = NUM_ENTITY( hw243_avmnet_avm_devices ),
        .avm_devices = hw243_avmnet_avm_devices
    },
#endif
#if (defined(CONFIG_MACH_ATHEROS) && defined(CONFIG_MACH_AR724x)) || (defined(CONFIG_ATH79) && defined(CONFIG_SOC_AR724X))
    {
        // 300E
        .hw_id = 173,
        .config = &avmnet_HW173,
        .nr_avm_devices = NUM_ENTITY( avmnet_hw173_avm_devices ),
        .avm_devices = avmnet_hw173_avm_devices
    },
#endif /*--- #if defined(CONFIG_MACH_AR724x) ---*/
#if (defined(CONFIG_MACH_ATHEROS) && defined(CONFIG_MACH_AR934x)) || (defined(CONFIG_ATH79) && defined(CONFIG_SOC_AR934X))
    {
        // 6810
        .hw_id = 180,
        .config = &avmnet_HW180,
        .nr_avm_devices = NUM_ENTITY( avmnet_hw180_avm_devices ),
        .avm_devices = avmnet_hw180_avm_devices
    },
    {   
        // 6841
        .hw_id = 184, 
        .config = &avmnet_6841,
        .nr_avm_devices = NUM_ENTITY( avmnet_hw184_avm_devices ),
        .avm_devices = avmnet_hw184_avm_devices
    },
    {   
        // 546E
        .hw_id = 190, 
        .config = &avmnet_HW190,
        .nr_avm_devices = NUM_ENTITY( avmnet_hw190_avm_devices ),
        .avm_devices = avmnet_hw190_avm_devices
    },
    {
        // 540E
        .hw_id = 201,
        .config = &avmnet_HW190,
        .nr_avm_devices = NUM_ENTITY( avmnet_hw190_avm_devices ),
        .avm_devices = avmnet_hw190_avm_devices
    },
    {
        // 540E SubRevision 2
        .hw_id = 201,
        .hw_sub_id = 2,
        .config = &avmnet_HW201v2,
        .nr_avm_devices = NUM_ENTITY( avmnet_hw201v2_avm_devices ),
        .avm_devices = avmnet_hw201v2_avm_devices
    },
    {
        // 310
        .hw_id = 194,
        .config = &avmnet_HW194,
        .nr_avm_devices = NUM_ENTITY( avmnet_hw194_avm_devices ),
        .avm_devices = avmnet_hw194_avm_devices
    },
    {
        // 6842
        .hw_id = 195,
        .config = &avmnet_HW195,
        .nr_avm_devices = NUM_ENTITY( avmnet_hw195_avm_devices ),
        .avm_devices = avmnet_hw195_avm_devices
    },
    {
        // 6842 intel
        .hw_id = 207,
        .config = &avmnet_HW207,
        .nr_avm_devices = NUM_ENTITY( avmnet_hw207_avm_devices ),
        .avm_devices = avmnet_hw207_avm_devices
    },
#endif /*--- #if defined(CONFIG_MACH_ATHEROS) && defined(CONFIG_MACH_QCA955x) ---*/
#if (defined(CONFIG_MACH_ATHEROS) && defined(CONFIG_MACH_QCA955x)) || (defined(CONFIG_ATH79) && defined(CONFIG_SOC_QCA955X))
#if !defined(CONFIG_ATH79_MACH_AVM_HW238)
    {
        // Repeater 450
        .hw_id = 200,
        .config = &avmnet_hw200,
        .nr_avm_devices = NUM_ENTITY( avmnet_hw200_avm_devices ),
        .avm_devices = avmnet_hw200_avm_devices
    },
    {
        // Repeater DVBC
        .hw_id = 205,
        .config = &avmnet_hw205,
        .nr_avm_devices = NUM_ENTITY( avmnet_hw205_avm_devices ),
        .avm_devices = avmnet_hw205_avm_devices
    },
    {
        // Repeater AC (wie Repeater DVBC) 
        .hw_id = 206,
        .config = &avmnet_hw205,
        .nr_avm_devices = NUM_ENTITY( avmnet_hw205_avm_devices ),
        .avm_devices = avmnet_hw205_avm_devices
    },
    {
        // 6820 LTE (qca9558 rgmii)
        .hw_id = 214,
        .config = &avmnet_hw214,
        .nr_avm_devices = NUM_ENTITY( avmnet_hw214_avm_devices ),
        .avm_devices = avmnet_hw214_avm_devices
    },
    {
        // 6820v3 LTE (qca9558 rgmii)
        .hw_id = 254,
        .config = &avmnet_hw214,
        .nr_avm_devices = NUM_ENTITY( avmnet_hw214_avm_devices ),
        .avm_devices = avmnet_hw214_avm_devices
    },
#else
    {
        // 7490 Scorpion Target (qca9558 rgmii)
        .hw_id = 238,
        .config = &avmnet_hw238,
        .nr_avm_devices = NUM_ENTITY( avmnet_hw238_avm_devices ),
        .avm_devices = avmnet_hw238_avm_devices
    },
#endif /*--- #if !defined(CONFIG_ATH79_MACH_AVM_HW238) ---*/
#endif /*--- #if defined(CONFIG_MACH_QCA955x) ---*/
#if (defined(CONFIG_MACH_ATHEROS) && defined(CONFIG_MACH_QCA953x)) || (defined(CONFIG_ATH79) && defined(CONFIG_SOC_QCA953X))
    {
        // 310 II
        .hw_id = 215,
        .config = &avmnet_HW215,
        .nr_avm_devices = NUM_ENTITY( avmnet_hw215_avm_devices ),
        .avm_devices = avmnet_hw215_avm_devices
    },
    {
        // Repeater AC 1160
        .hw_id = 216,
        .config = &avmnet_HW216,
        .nr_avm_devices = NUM_ENTITY( avmnet_hw216_avm_devices ),
        .avm_devices = avmnet_hw216_avm_devices
    },
    {
        // Powerline 1240
        .hw_id = 222,
        .config = &avmnet_HW222,
        .nr_avm_devices = NUM_ENTITY( avmnet_hw222_avm_devices ),
        .avm_devices = avmnet_hw222_avm_devices
    },
    {
        // Powerline 1240 WLAN_PROD_TEST
        .hw_id = 222,
        .hw_sub_id = WLAN_PROD_TEST,
        .config = &avmnet_HW222_wlanprodtest,
        .nr_avm_devices = NUM_ENTITY( avmnet_hw222_wlanprodtest_avm_devices ),
        .avm_devices = avmnet_hw222_wlanprodtest_avm_devices
    },
#endif /*--- #if (defined(CONFIG_MACH_ATHEROS) && defined(CONFIG_MACH_QCA953x)) || (defined(CONFIG_ATH79) && defined(CONFIG_SOC_QCA953X)) ---*/
#if defined(CONFIG_SOC_QCN550X)
    {
        // Repeater 600
        .hw_id = 240,
        .config = &avmnet_hw240,
        .nr_avm_devices = NUM_ENTITY( avmnet_hw240_avm_devices ),
        .avm_devices = avmnet_hw240_avm_devices
    },
    {
        // Repeater 600
        .hw_id = 241,
        .config = &avmnet_hw241,
        .nr_avm_devices = NUM_ENTITY( avmnet_hw241_avm_devices ),
        .avm_devices = avmnet_hw241_avm_devices
    },
    {
        // Repeater 600v2
        .hw_id = 263,
        .config = &avmnet_hw263,
        .nr_avm_devices = NUM_ENTITY( avmnet_hw263_avm_devices ),
        .avm_devices = avmnet_hw263_avm_devices
    },
#endif

#if (defined(CONFIG_ARCH_QCOM)) /* includes Akronite-based FB4080 */
    {
        /* F!B 4080 */
        .hw_id = 211,
        .config = &avmnet_hw211,
        .nr_avm_devices = NUM_ENTITY(avmnet_hw211_avm_devices),
        .avm_devices = avmnet_hw211_avm_devices,
    },
#endif

#if (defined(CONFIG_MACH_ATHEROS) && defined(CONFIG_MACH_QCA956x)) || (defined(CONFIG_ATH79) && defined(CONFIG_SOC_QCA956X))
#if 0
    {
        // Fritzbox 4020
        .hw_id = 217,
        .config = &avmnet_HW217,
        .nr_avm_devices = NUM_ENTITY( avmnet_hw217_avm_devices ),
        .avm_devices = avmnet_hw217_avm_devices
    },
#endif
    {
        // Fritzbox 4020
        .hw_id = 219,
        .config = &avmnet_HW219,
        .nr_avm_devices = NUM_ENTITY( avmnet_hw219_avm_devices ),
        .avm_devices = avmnet_hw219_avm_devices
    },
#endif /*--- #if (defined(CONFIG_MACH_ATHEROS) && defined(CONFIG_MACH_QCA956x)) || (defined(CONFIG_ATH79) && defined(CONFIG_SOC_QCA956X)) ---*/

#if (defined(CONFIG_ARCH_GEN3)) /* Puma6/7 Atom side  */
    {
        /* F!B 6430 */
        .hw_id = 231,
        .config = &hw231x_avmnet,
        .nr_avm_devices = NUM_ENTITY(hw231x_avmnet_avm_devices),
        .avm_devices = hw231x_avmnet_avm_devices,
    },
    {
        /* F!B 6590 */
        .hw_id = 220,
        .config = &hw220x_avmnet,
        .nr_avm_devices = NUM_ENTITY(hw220x_avmnet_avm_devices),
        .avm_devices = hw220x_avmnet_avm_devices,
    },
    {
        /* F!B 6490 */
        .hw_id = 213,
        .config = &hw231x_avmnet,
        .nr_avm_devices = NUM_ENTITY(hw231x_avmnet_avm_devices),
        .avm_devices = hw231x_avmnet_avm_devices,
    },
    {
        /* F!B 6460 */
        .hw_id = 199,
        .config = &hw231x_avmnet,
        .nr_avm_devices = NUM_ENTITY(hw231x_avmnet_avm_devices),
        .avm_devices = hw231x_avmnet_avm_devices,
    },
#endif
};

#endif
