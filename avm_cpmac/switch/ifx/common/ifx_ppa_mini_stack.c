
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/types.h>

#include <avm/net/ifx_ppa_api.h>
#include <avm/net/ifx_ppa_ppe_hal.h>

#define MAC_ARG(x) ((u8 *)(x))[0], ((u8 *)(x))[1], ((u8 *)(x))[2], ((u8 *)(x))[3], ((u8 *)(x))[4], ((u8 *)(x))[5]

#ifndef MAC_FMT  
#define MAC_FMT "%02x:%02x:%02x:%02x:%02x:%02x"
#endif

static DEFINE_SPINLOCK(g_local_irq_save_flag_lock);

/*------------------------------------------------------------------------------------------*\
 * locking was moved to header
\*------------------------------------------------------------------------------------------*/
#if 0

static inline void ppe_lock_init(PPE_LOCK *p_lock)
{
    spin_lock_init(p_lock);
}

static inline void ppe_lock_get(PPE_LOCK *p_lock)
{
    spin_lock_bh(p_lock);
}

static inline void ppe_lock_release(PPE_LOCK *p_lock)
{
    spin_unlock_bh(p_lock);
}

#endif
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int32_t ppa_mem_cache_create(const char *name, uint32_t size, PPA_MEM_CACHE **pp_cache)
{
    PPA_MEM_CACHE* p_cache;

    if ( !pp_cache )
        return IFX_EINVAL;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,32)
    p_cache = kmem_cache_create(name, size, 0, SLAB_HWCACHE_ALIGN, NULL, NULL);
#else
    p_cache = kmem_cache_create(name, size, 0, SLAB_HWCACHE_ALIGN, NULL);
#endif
    if ( !p_cache )
        return IFX_ENOMEM;

    *pp_cache = p_cache;
    return IFX_SUCCESS;
}

int32_t ppa_mem_cache_destroy(PPA_MEM_CACHE *p_cache)
{
    if ( !p_cache )
        return IFX_EINVAL;

    kmem_cache_destroy(p_cache);

    return IFX_SUCCESS;
}

void *ppa_mem_cache_alloc(PPA_MEM_CACHE *p_cache)
{
    return kmem_cache_alloc(p_cache, GFP_ATOMIC);
}

void ppa_mem_cache_free(void *buf, PPA_MEM_CACHE *p_cache)
{
    kmem_cache_free(p_cache, buf);
}

int32_t ppa_kmem_cache_shrink(PPA_MEM_CACHE *cachep)
{
    return kmem_cache_shrink(cachep);
}
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

uint32_t ppa_get_time_in_10msec(void)
{
    if ( HZ == 100 )
        return jiffies;
    else if ( HZ >= 1000 )
    {
        return (jiffies + HZ / 200) / (HZ / 100);
    }
    else
    {
        uint64_t divident = (uint64_t)jiffies * 100 + HZ / 2;

        return __div64_32(&divident, HZ);
    }
}

uint32_t ppa_get_time_in_sec(void)
{
    return (jiffies + HZ / 2) / HZ;
}

int32_t ppa_atomic_read(PPA_ATOMIC *v)
{
    return atomic_read(v);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

void ppa_memcpy(void *dst, const void *src, uint32_t count)
{
#if defined(ENABLE_MY_MEMCPY) && ENABLE_MY_MEMCPY
    char *d = (char *)dst, *s = (char *)src;

    if (count >= 32) {
        int i = 8 - (((unsigned long) d) & 0x7);

        if (i != 8)
            while (i-- && count--) {
                *d++ = *s++;
            }

        if (((((unsigned long) d) & 0x7) == 0) &&
                ((((unsigned long) s) & 0x7) == 0)) {
            while (count >= 32) {
                unsigned long long t1, t2, t3, t4;
                t1 = *(unsigned long long *) (s);
                t2 = *(unsigned long long *) (s + 8);
                t3 = *(unsigned long long *) (s + 16);
                t4 = *(unsigned long long *) (s + 24);
                *(unsigned long long *) (d) = t1;
                *(unsigned long long *) (d + 8) = t2;
                *(unsigned long long *) (d + 16) = t3;
                *(unsigned long long *) (d + 24) = t4;
                d += 32;
                s += 32;
                count -= 32;
            }
            while (count >= 8) {
                *(unsigned long long *) d =
                                            *(unsigned long long *) s;
                d += 8;
                s += 8;
                count -= 8;
            }
        }

        if (((((unsigned long) d) & 0x3) == 0) &&
                ((((unsigned long) s) & 0x3) == 0)) {
            while (count >= 4) {
                *(unsigned long *) d = *(unsigned long *) s;
                d += 4;
                s += 4;
                count -= 4;
            }
        }

        if (((((unsigned long) d) & 0x1) == 0) &&
                ((((unsigned long) s) & 0x1) == 0)) {
            while (count >= 2) {
                *(unsigned short *) d = *(unsigned short *) s;
                d += 2;
                s += 2;
                count -= 2;
            }
        }
    }

    while (count--) {
        *d++ = *s++;
    }

//    return d;
#else
    memcpy(dst, src, count);
#endif
}

void ppa_memset(void *dst, uint32_t pad, uint32_t n)
{
    memset(dst, pad, n);
}

int ppa_memcmp(const void *src, const void *dest, uint32_t count)
{
    return memcmp(src, dest, count);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
uint32_t ppa_disable_int(void)
{
    unsigned long sys_flag = 0;

    spin_lock_irqsave(&g_local_irq_save_flag_lock, sys_flag);
    return sys_flag;
}

void ppa_enable_int(uint32_t flag)
{
    spin_unlock_irqrestore(&g_local_irq_save_flag_lock, (unsigned long)flag);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
int ppa_sprintf(uint8_t * buf, const uint8_t *fmt, ...) {
    va_list args;
    int i;

    va_start(args, fmt);
    i=vsprintf(buf,fmt,args);
    va_end(args);
    return i;
}

int8_t *ppa_get_pkt_ip_string(PPA_IPADDR ppa_ip, uint32_t flag, int8_t *strbuf)
{
    if(!strbuf)
        return strbuf;

    strbuf[0] = 0;
    if(flag){
#if defined(CONFIG_IFX_PPA_IPv6_ENABLE)
        ppa_sprintf(strbuf, "%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x", NIP6(ppa_ip.ip6));
#endif

    }else{
        ppa_sprintf(strbuf, NIPQUAD_FMT, NIPQUAD(ppa_ip.ip));
    }

    return strbuf;
}


int8_t *ppa_get_pkt_mac_string(uint8_t *mac, int8_t *strbuf) {
    if(strbuf){
        ppa_sprintf(strbuf, MAC_FMT, MAC_ARG(mac));
    }

    return strbuf;
}
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

EXPORT_SYMBOL(ppa_mem_cache_create);
EXPORT_SYMBOL(ppa_mem_cache_destroy);
EXPORT_SYMBOL(ppa_mem_cache_alloc);
EXPORT_SYMBOL(ppa_mem_cache_free);
EXPORT_SYMBOL(ppa_kmem_cache_shrink);

EXPORT_SYMBOL(ppa_get_time_in_10msec);
EXPORT_SYMBOL(ppa_get_time_in_sec);

EXPORT_SYMBOL(ppa_memcpy);
EXPORT_SYMBOL(ppa_memset);
EXPORT_SYMBOL(ppa_memcmp);

EXPORT_SYMBOL(ppa_disable_int);
EXPORT_SYMBOL(ppa_enable_int);

EXPORT_SYMBOL(ppa_get_pkt_ip_string);
EXPORT_SYMBOL(ppa_get_pkt_mac_string);

MODULE_LICENSE("GPL");


