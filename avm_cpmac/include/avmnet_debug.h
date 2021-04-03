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

#ifndef _AVMNET_DEBUG_H_
#define _AVMNET_DEBUG_H_

#include <linux/kernel.h>

/* If temporary debug buffers are necessary, use this size */
#define AVMNET_DEBUG_BUFFER_LENGTH 300u

/* Use the following debug settings */
#define AVMNET_USE_ASSERT             /* Use assert or comment it out */

/*******************************************************************************\
 * There exist several debug levels. They are intended to be used as described * 
 * below: AVMNET_<level>                                                       *
 *                                                                             *
 * SUPPORT : generic information, e.g. requested by user (KERN_ERR)            *
 * ERR     : severe problems (KERN_ERR)                                        *
 * WARN    : important, but not severe problems (KERN_WARNING)                 *
 *                                                                             *
 * The following describe several debug levels, that should be disabled in     *
 * normal builds. As they are intended for debug purposes only, all are        *
 * represented as KERN_ERR.                                                    *
 *                                                                             *
 * INFO    : e.g. information about infrequent state changes                   *
 * INFOTRC : e.g. information about frequent state changes                     *
 * TRC     : normal trace output                                               *
 * DEBUG   : "information flood", e.g. dumps of all packets                    *
 * TEST    : temporary debugging output with extra "[test]" field              *
\*******************************************************************************/

#define AVMNET_USE_DEBUG_LEVEL_SUPPORT   1u
#define AVMNET_USE_DEBUG_LEVEL_ERROR     1u
#define AVMNET_USE_DEBUG_LEVEL_WARNING   1u
//#define AVMNET_USE_DEBUG_LEVEL_INFO      1u
//#define AVMNET_USE_DEBUG_LEVEL_INFOTRACE 1u
//#define AVMNET_USE_DEBUG_LEVEL_TRACE     1u
//#define AVMNET_USE_DEBUG_LEVEL_TEST      1u
//#define AVMNET_USE_DEBUG_LEVEL_DEBUG     1u

#ifdef DEBUG
#undef AVMNET_USE_DEBUG_LEVEL_DEBUG
#define AVMNET_USE_DEBUG_LEVEL_DEBUG 1
#endif

/* Debug level values */
#define AVMNET_DEBUG_LEVEL_SUPPORT   0x01u
#define AVMNET_DEBUG_LEVEL_ERROR     0x02u
#define AVMNET_DEBUG_LEVEL_WARNING   0x04u
#define AVMNET_DEBUG_LEVEL_INFO      0x08u
#define AVMNET_DEBUG_LEVEL_INFOTRACE 0x10u
#define AVMNET_DEBUG_LEVEL_TRACE     0x20u
#define AVMNET_DEBUG_LEVEL_TEST      0x40u
#define AVMNET_DEBUG_LEVEL_DEBUG     0x80u

/* #define AVMNET_DBG_TX_QUEUE_ENABLE */

#if defined(AVMNET_DBG_TX_QUEUE_ENABLE)
#define AVMNET_DBG_TX_QUEUE(format, arg...)  do { printk(KERN_ERR "[dbg_tx_queue]" __FILE__"[%d:%s]: " format "\n", __LINE__, __FUNCTION__, ##arg); }  while ( 0 )
#define AVMNET_DBG_TX_QUEUE_RATE(format, arg...)  do { if (printk_ratelimit()){ printk(KERN_ERR "[dbg_tx_queue]" __FILE__"[%d:%s]: " format "\n", __LINE__, __FUNCTION__, ##arg); } } while ( 0 )
#define AVMNET_DBG_TX_QUEUE_STATUS(format, arg...)
#else
#define AVMNET_DBG_TX_QUEUE(format, arg...)
#define AVMNET_DBG_TX_QUEUE_RATE(format, arg...)
#define AVMNET_DBG_TX_QUEUE_STATUS(format, arg...)
#endif

/*------------------------------------------------------------------------------------------*\
 * Below this part the above definitions are realized and nothing should be changed there   *
\*------------------------------------------------------------------------------------------*/

/* My own definition of an assert */
#if defined(AVMNET_USE_ASSERT)
#define assert(i) do { \
                      if(unlikely(!(i))) { \
                          restore_printk(); \
                          printk(KERN_ERR "Assertion failed in %s: '"#i"' (at %s:%d)\n", __FUNCTION__, __FILE__, __LINE__); \
                          panic("assert failed!"); \
                      } \
                  } while(0)
#else /*--- #if defined(AVMNET_USE_ASSERT) ---*/
#define assert(exp)
#endif /*--- #else ---*/ /*--- #if defined(AVMNET_USE_ASSERT) ---*/

/* Assert, that the defines used to calculate the DEBUG_LEVEL value exist */
#if !defined(AVMNET_USE_DEBUG_LEVEL_SUPPORT)
#   define AVMNET_USE_DEBUG_LEVEL_SUPPORT 0
#endif
#if !defined(AVMNET_USE_DEBUG_LEVEL_ERROR)
#   define AVMNET_USE_DEBUG_LEVEL_ERROR 0
#endif
#if !defined(AVMNET_USE_DEBUG_LEVEL_WARNING)
#   define AVMNET_USE_DEBUG_LEVEL_WARNING 0
#endif
#if !defined(AVMNET_USE_DEBUG_LEVEL_INFO)
#   define AVMNET_USE_DEBUG_LEVEL_INFO 0
#endif
#if !defined(AVMNET_USE_DEBUG_LEVEL_INFOTRACE)
#   define AVMNET_USE_DEBUG_LEVEL_INFOTRACE 0
#endif
#if !defined(AVMNET_USE_DEBUG_LEVEL_TRACE)
#   define AVMNET_USE_DEBUG_LEVEL_TRACE 0
#endif
#if !defined(AVMNET_USE_DEBUG_LEVEL_TEST)
#   define AVMNET_USE_DEBUG_LEVEL_TEST 0
#endif
#if !defined(AVMNET_USE_DEBUG_LEVEL_DEBUG)
#   define AVMNET_USE_DEBUG_LEVEL_DEBUG 0
#else
#undef DEBUG
#define DEBUG 1
#endif

#define AVMNET_DEBUG_LEVEL (  0 \
                           | (AVMNET_USE_DEBUG_LEVEL_SUPPORT   * AVMNET_DEBUG_LEVEL_SUPPORT  ) \
                           | (AVMNET_USE_DEBUG_LEVEL_ERROR     * AVMNET_DEBUG_LEVEL_ERROR    ) \
                           | (AVMNET_USE_DEBUG_LEVEL_WARNING   * AVMNET_DEBUG_LEVEL_WARNING  ) \
                           | (AVMNET_USE_DEBUG_LEVEL_INFO      * AVMNET_DEBUG_LEVEL_INFO     ) \
                           | (AVMNET_USE_DEBUG_LEVEL_INFOTRACE * AVMNET_DEBUG_LEVEL_INFOTRACE) \
                           | (AVMNET_USE_DEBUG_LEVEL_TRACE     * AVMNET_DEBUG_LEVEL_TRACE    ) \
                           | (AVMNET_USE_DEBUG_LEVEL_TEST      * AVMNET_DEBUG_LEVEL_TEST     ) \
                           | (AVMNET_USE_DEBUG_LEVEL_DEBUG     * AVMNET_DEBUG_LEVEL_DEBUG    ) \
                          )

/* Clear perhaps existing external defines */
#if defined(AVMNET_SUPPORT)
#  undef AVMNET_SUPPORT
#endif
#if defined(AVMNET_ERR)
#  undef AVMNET_ERR
#endif
#if defined(AVMNET_WARN)
#  undef AVMNET_WARN
#endif
#if defined(AVMNET_INFO)
#  undef AVMNET_INFO
#endif
#if defined(AVMNET_INFOTRC)
#  undef AVMNET_INFOTRC
#endif
#if defined(AVMNET_TRC)
#  undef AVMNET_TRC
#endif
#if defined(AVMNET_TEST)
#  undef AVMNET_TEST
#endif
#if defined(AVMNET_DEBUG)
#  undef AVMNET_DEBUG
#endif

/* Now define the debug routines according to the above information */
#if AVMNET_DEBUG_LEVEL & AVMNET_DEBUG_LEVEL_SUPPORT
#  define AVMNET_SUPPORT(a...) printk(KERN_ERR "[avmnet] " a);
#endif
#if AVMNET_DEBUG_LEVEL & AVMNET_DEBUG_LEVEL_ERROR
#  define AVMNET_ERR(a...) printk(KERN_ERR "[avmnet] " a);
#endif
#if AVMNET_DEBUG_LEVEL & AVMNET_DEBUG_LEVEL_WARNING
#  define AVMNET_WARN(a...) printk(KERN_WARNING "[avmnet] " a);
#endif
#if AVMNET_DEBUG_LEVEL & AVMNET_DEBUG_LEVEL_INFO
#  define AVMNET_INFO(a...) printk (KERN_ERR "[avmnet] " a);
#endif
#if AVMNET_DEBUG_LEVEL & AVMNET_DEBUG_LEVEL_INFOTRACE
#  define AVMNET_INFOTRC(a...) printk(KERN_ERR "[avmnet] " a);
#endif
#if AVMNET_DEBUG_LEVEL & AVMNET_DEBUG_LEVEL_TRACE
#  define AVMNET_TRC(a...) printk(KERN_ERR "[avmnet] " a);
#endif
#if AVMNET_DEBUG_LEVEL & AVMNET_DEBUG_LEVEL_TEST
#  define AVMNET_TEST(a...) printk(KERN_ERR "[avmnet] [test] " a);
#endif
#if AVMNET_DEBUG_LEVEL & AVMNET_DEBUG_LEVEL_DEBUG
#  define AVMNET_DEBUG(a...) printk(KERN_ERR "[avmnet] " a);
#endif

/* Now define all unused error levels as empty */
#if !defined(AVMNET_SUPPORT)
#  define AVMNET_SUPPORT(a...)
#endif
#if !defined(AVMNET_ERR)
#  define AVMNET_ERR(a...)
#endif
#if !defined(AVMNET_WARN)
#  define AVMNET_WARN(a...)
#endif
#if !defined(AVMNET_INFO)
#  define AVMNET_INFO(a...)
#endif
#if !defined(AVMNET_INFOTRC)
#  define AVMNET_INFOTRC(a...)
#endif
#if !defined(AVMNET_TRC)
#  define AVMNET_TRC(a...)
#endif
#if !defined(AVMNET_TEST)
#  define AVMNET_TEST(a...)
#endif
#if !defined(AVMNET_DEBUG)
#  define AVMNET_DEBUG(a...)
#endif

/* Allow compiler optimisable testing of DEBUG set in C/CPP expressions via
 * IS_ENABLED(DEBUG): */
#ifdef DEBUG
#undef DEBUG
#define DEBUG y
#endif

#endif /*--- #ifndef _AVMNET_DEBUG_H_ ---*/

