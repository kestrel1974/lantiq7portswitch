#ifndef __IFX_PPA_API_COMMON_H__20100203__1740__
#define __IFX_PPA_API_COMMON_H__20100203__1740__

/*******************************************************************************
**
** FILE NAME    : ifx_ppa_api_common.h
** PROJECT      : PPA
** MODULES      : PPA Common header file
**
** DATE         : 3 NOV 2008
** AUTHOR       : Xu Liang
** DESCRIPTION  : PPA Common Header File
** COPYRIGHT    :              Copyright (c) 2009
**                          Lantiq Deutschland GmbH
**                   Am Campeon 3; 85579 Neubiberg, Germany
**
**   For licensing information, see the file 'LICENSE' in the root folder of
**   this software module.
**
** HISTORY
** $Date        $Author         $Comment
** 03 NOV 2008  Xu Liang        Initiate Version
*******************************************************************************/

#define NO_DOXY                 1

#ifndef CONFIG_IFX_PPA_DSLITE   //if not defined in kernel's .configure file, then use local's definition
#define CONFIG_IFX_PPA_DSLITE            1
#endif

 /*force dynamic ppe driver's module parameter */
#ifndef RTP_SAMPLING_ENABLE     //if not defined in kernel's .configure file, then use local's definition
#define RTP_SAMPLING_ENABLE               1
#endif

#ifndef MIB_MODE_ENABLE     //if not defined in kernel's .configure file, then use local's definition
#define MIB_MODE_ENABLE              1
#endif

#ifndef CAP_WAP_CONFIG     //if not defined in kernel's .configure file, then use local's definition
#define CAP_WAP_CONFIG               1
#endif

#ifndef CONFIG_IFX_PPA_MFE      //if not defined in kernel's .configure file, then use local's definition
#define CONFIG_IFX_PPA_MFE               0
#endif


 /*force dynamic ppe driver's module parameter */
#define IFX_PPA_DP_DBG_PARAM_ENABLE  0   //for PPA automation purpose. for non-linux os porting, just disable it

#define CONFIG_IFX_PPA_IF_MIB 1   //Flag to enable/disable PPA software interface based mib counter
#define SESSION_STATISTIC_DEBUG 1 //flag to enable session management statistics support

#if IFX_PPA_DP_DBG_PARAM_ENABLE
    extern int ifx_ppa_drv_dp_dbg_param_enable;
    extern int  ifx_ppa_drv_dp_dbg_param_ethwan;
    extern int ifx_ppa_drv_dp_dbg_param_wanitf;
    extern int ifx_ppa_drv_dp_dbg_param_ipv6_acc_en;
    extern int ifx_ppa_drv_dp_dbg_param_wanqos_en;
#endif // end of IFX_PPA_DP_DBG_PARAM_ENABLE
#endif

