
#include <linux/kernel.h>
#include <linux/types.h>
#include <net/sock.h>
#if __has_include(<avm/pa/pa.h>)
#include <avm/pa/pa.h>
#else
#include <linux/avm_pa.h>
#endif
#include <linux/spinlock.h>
#if __has_include(<avm/pa/hw.h>)
#include <avm/pa/hw.h>
#else
#include <linux/avm_pa_hw.h>
#endif
#if __has_include(<avm/pa/ifx_multiplexer.h>)
#include <avm/pa/ifx_multiplexer.h>
#else
#include <linux/avm_pa_ifx_multiplexer.h>
#endif

#include <avmnet_config.h>

#include "swi_ifx_common.h"

#define MULTIPLEXER_PROCFILE_NAME "avm_pa_ifx_multiplexer"
#define DEFAULT_AVM_HIT_POLLING_TIME            1

/*------------------------------------------------------------------------------------------*\
 * AVM PA multiplexer
\*------------------------------------------------------------------------------------------*/
struct avm_pa_multiplexer_session {
    struct avm_hardware_pa_instance *hw_pa_instance;	/* identifiziert den HW_PA von dem die Session verarbeitet wird */
    struct avm_pa_session *avm_session;
    atomic_t valid;
    atomic_t active;
};


static int avm_pa_multiplexer_add_session(struct avm_pa_session *avm_session);
static int avm_pa_multiplexer_remove_one_session(struct avm_pa_session *avm_session);
static int avm_pa_multiplexer_change_session(struct avm_pa_session *avm_session);
static void avm_pa_multiplexer_remove_all_sessions( struct avm_hardware_pa_instance *hw_pa_instance );
static int avm_pa_multiplexer_stats(struct avm_pa_session *avm_session,
					                struct avm_pa_session_stats *stats);

/*--- static DEFINE_RWLOCK( hw_pa_list_lock ); ---*/
static DEFINE_SPINLOCK( hw_pa_list_lock );

#ifndef AVM_PA_HARDWARE_PA_HAS_SESSION_STATS
static struct timer_list stat_timer;
static void check_stat(unsigned long dummy __attribute__((unused)));
#endif

static struct avm_hardware_pa avm_pa_multiplexer = {
   .add_session               = avm_pa_multiplexer_add_session,
   .remove_session            = avm_pa_multiplexer_remove_one_session,
   .change_session            = avm_pa_multiplexer_change_session,
   .try_to_accelerate         = ifx_ppa_try_to_accelerate,
   .alloc_rx_channel          = ifx_ppa_alloc_virtual_rx_device,
   .alloc_tx_channel          = ifx_ppa_alloc_virtual_tx_device,
   .free_rx_channel           = ifx_ppa_free_virtual_rx_device,
   .free_tx_channel           = ifx_ppa_free_virtual_tx_device,
#ifdef AVM_PA_HARDWARE_PA_HAS_SESSION_STATS
   .session_stats             = avm_pa_multiplexer_stats,
#endif
};


static struct avm_pa_multiplexer_session multiplexer_session_array[CONFIG_AVM_PA_MAX_SESSION];
static struct avm_hardware_pa_instance *pa_instance_array[MAX_HW_PA_INSTANCES];

#define PROC_BUFF_LEN 1024
char proc_buff[PROC_BUFF_LEN];


static int proc_read_avm_pa_ifx_multiplexer_open(struct inode *inode, struct file *file);
static int proc_read_avm_pa_ifx_multiplexer_show(struct seq_file *seq, void *data);

static int proc_write_avm_pa_ifx_multiplexer(struct file *file __attribute__ ((unused)), const char *buf,
		size_t count, loff_t *data __attribute__ ((unused)));

static const struct file_operations avm_pa_ifx_multiplexer_fops =
{
   .open    = proc_read_avm_pa_ifx_multiplexer_open,
   .read    = seq_read,
   .write   = proc_write_avm_pa_ifx_multiplexer,
   .llseek  = seq_lseek,
   .release = single_release,
};

void clear_session_entry(size_t pos){

	multiplexer_session_array[pos].hw_pa_instance = NULL;
	multiplexer_session_array[pos].avm_session = NULL;
	atomic_set(&multiplexer_session_array[pos].valid, 0);
}
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void avm_pa_multiplexer_init(void){

	size_t i;

	for ( i = 0; i < CONFIG_AVM_PA_MAX_SESSION; i++){
		clear_session_entry(i);
	    atomic_set(&multiplexer_session_array[i].active, 0);
	}

	for (i = 0; i < MAX_HW_PA_INSTANCES; i++)
		pa_instance_array[i] = NULL;

	avm_pa_register_hardware_pa( &avm_pa_multiplexer );
	avmnet_cfg_add_seq_procentry( avmnet_hw_config_entry->config, MULTIPLEXER_PROCFILE_NAME, &avm_pa_ifx_multiplexer_fops);

#ifndef AVM_PA_HARDWARE_PA_HAS_SESSION_STATS
	setup_timer(&stat_timer, check_stat, 0 );
	mod_timer(&stat_timer,  jiffies + HZ * DEFAULT_AVM_HIT_POLLING_TIME - 1);
#endif

}
EXPORT_SYMBOL(avm_pa_multiplexer_init);


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
#if 0 //we never shut down
void avm_pa_multiplexer_exit(void){
    avmnet_cfg_remove_procentry( avmnet_hw_config_entry->config, MULTIPLEXER_PROCFILE_NAME );
}
#endif
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

void avm_pa_multiplexer_register_instance( struct avm_hardware_pa_instance *hw_pa_instance ) {
	unsigned long hlock_flags;
	int i;

	spin_lock_irqsave( &hw_pa_list_lock, hlock_flags);
	for (i = 0; i < MAX_HW_PA_INSTANCES; i++){
		if ( pa_instance_array[i] == NULL){
	        hw_pa_instance->enabled = 1;
			pa_instance_array[i] = hw_pa_instance;
			break;
		}
	}
	spin_unlock_irqrestore( &hw_pa_list_lock, hlock_flags);
}
EXPORT_SYMBOL(avm_pa_multiplexer_register_instance);

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
void avm_pa_multiplexer_unregister_instance( struct avm_hardware_pa_instance *hw_pa_instance ) {
    unsigned long hlock_flags;
    int i;

    spin_lock_irqsave( &hw_pa_list_lock, hlock_flags);
    for ( i = 0; i < MAX_HW_PA_INSTANCES; i++ ){
        if ( pa_instance_array[i] == hw_pa_instance ){
            hw_pa_instance->enabled = 0;
            pa_instance_array[i] = NULL;
            break;
        }
    }
    spin_unlock_irqrestore( &hw_pa_list_lock, hlock_flags);

    avm_pa_multiplexer_remove_all_sessions( hw_pa_instance );
}
EXPORT_SYMBOL(avm_pa_multiplexer_unregister_instance);

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static int avm_pa_multiplexer_change_session(struct avm_pa_session *avm_session) {
    unsigned long hlock_flags;
	int status = AVM_PA_TX_ERROR_EGRESS;
	struct avm_pa_multiplexer_session *session_to_change;

	BUG_ON( avm_session->session_handle >= CONFIG_AVM_PA_MAX_SESSION);

    spin_lock_irqsave( &hw_pa_list_lock, hlock_flags);
	if ( atomic_read(&multiplexer_session_array[ avm_session->session_handle ].valid) ) {
		session_to_change = &multiplexer_session_array[ avm_session->session_handle ];
        atomic_set(&multiplexer_session_array[ avm_session->session_handle ].active, 0);
	} else {
        spin_unlock_irqrestore( &hw_pa_list_lock, hlock_flags);
        printk(KERN_ERR "[%s] invalid session_id=%hu \n",  __func__ ,avm_session->session_handle );
		return status;
	}
	status = session_to_change->hw_pa_instance->change_session( avm_session );

	if ( status != AVM_PA_TX_EGRESS_ADDED ){
		clear_session_entry(avm_session->session_handle);
		avm_session->hw_session = NULL;
	} else {
        atomic_set(&multiplexer_session_array[ avm_session->session_handle ].active, 1);
    }
    spin_unlock_irqrestore( &hw_pa_list_lock, hlock_flags);

	return status;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
bool (*filter_session_hook)(struct avm_pa_session *session) = NULL;
EXPORT_SYMBOL( filter_session_hook );

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

static int avm_pa_multiplexer_add_session(struct avm_pa_session *avm_session) {
    unsigned long hlock_flags;
    int i, res = AVM_PA_TX_ERROR_SESSION;

    BUG_ON( avm_session->session_handle >= CONFIG_AVM_PA_MAX_SESSION);

    if (filter_session_hook && filter_session_hook(avm_session))
        return res;

    for (i = 0; i < MAX_HW_PA_INSTANCES; i++){
        struct avm_hardware_pa_instance *hw_pa_instance;

        spin_lock_irqsave( &hw_pa_list_lock, hlock_flags);
        hw_pa_instance = pa_instance_array[i];

        if ( hw_pa_instance && hw_pa_instance->enabled && 
           ( hw_pa_instance->add_session( avm_session ) == AVM_PA_TX_SESSION_ADDED )) {

            if ( atomic_add_return( 1, &multiplexer_session_array[ avm_session->session_handle ].valid) == 1) {
                multiplexer_session_array[ avm_session->session_handle ].hw_pa_instance = hw_pa_instance;
                multiplexer_session_array[ avm_session->session_handle ].avm_session = avm_session;
                atomic_set( &multiplexer_session_array[ avm_session->session_handle ].active, 1);
                res = AVM_PA_TX_SESSION_ADDED;
            }
            else {
                hw_pa_instance->remove_session( avm_session );
                printk(KERN_ERR "[%s] hw session already registered for session_id=%hu \n",  __func__ ,avm_session->session_handle );
                res = AVM_PA_TX_ERROR_SESSION;
            }
        } 
        spin_unlock_irqrestore( &hw_pa_list_lock, hlock_flags);
    }
    return res;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static int avm_pa_multiplexer_remove_one_session( struct avm_pa_session *avm_session ){
    unsigned long hlock_flags;
	int status = AVM_PA_TX_ERROR_SESSION;
	struct avm_pa_multiplexer_session *session_to_remove;

	BUG_ON( avm_session->session_handle >= CONFIG_AVM_PA_MAX_SESSION);

    spin_lock_irqsave( &hw_pa_list_lock, hlock_flags);
	if ( atomic_sub_if_positive ( 1, &multiplexer_session_array[ avm_session->session_handle ].active ) == 0 ) {
		session_to_remove = &multiplexer_session_array[ avm_session->session_handle ];
	    session_to_remove->hw_pa_instance->remove_session( avm_session );
	} else {
		// no valid session found
		printk( KERN_ERR "[%s] session %hu (invalid) will not be removed\n",
		   __func__, avm_session->session_handle );
        spin_unlock_irqrestore( &hw_pa_list_lock, hlock_flags);
		return status;
	}

	clear_session_entry( avm_session->session_handle );
	avm_session->hw_session = NULL;
    spin_unlock_irqrestore( &hw_pa_list_lock, hlock_flags);

	return AVM_PA_TX_OK;
}


/*------------------------------------------------------------------------------------------*\
 * Remove all Sessions dedicated to hw_pa_instance
\*------------------------------------------------------------------------------------------*/
static void avm_pa_multiplexer_remove_all_sessions( struct avm_hardware_pa_instance *hw_pa_instance ) {
	struct avm_pa_multiplexer_session *session_to_remove;
	unsigned int i;

	for (i = 0 ; i < CONFIG_AVM_PA_MAX_SESSION; i++) {

        if ( multiplexer_session_array[i].hw_pa_instance == hw_pa_instance ) {
            if ( atomic_sub_if_positive( 1, &multiplexer_session_array[ i ].active) == 0)  {

                session_to_remove = &multiplexer_session_array[i];

                if (session_to_remove->avm_session){
                    session_to_remove->hw_pa_instance->remove_session( session_to_remove->avm_session );
                    session_to_remove->avm_session->hw_session = NULL;
                }

                clear_session_entry(i);
            }
        }
	}
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

static int proc_write_avm_pa_ifx_multiplexer(struct file *file __attribute__ ((unused)),
					     const char *buf, size_t count, loff_t *data __attribute__ ((unused))) {
	int len;
	int parse_res;
	int pa_nr;
	int enable;
	unsigned long hlock_flags;

	len = sizeof(proc_buff) < count ? sizeof(proc_buff) - 1 : count;
	len = len - copy_from_user(proc_buff, buf, len);
	proc_buff[len] = 0;

	parse_res = sscanf(proc_buff, "enable %d", &pa_nr);
	if ( parse_res == 1) {
		enable = 1;
	}
	else {
		parse_res = sscanf(proc_buff, "disable %d", &pa_nr);
		if ( parse_res == 1){
			enable = 0;
		}
		else {
			// neither enable nor disable
			return count;
		}
	}

	spin_lock_irqsave( &hw_pa_list_lock, hlock_flags);
	if ( (pa_nr >= 0) && (pa_nr < MAX_HW_PA_INSTANCES) && pa_instance_array[pa_nr] ) {
		if (enable) {
			pa_instance_array[pa_nr]->enabled = 1;
		}
		else {
			pa_instance_array[pa_nr]->enabled = 0;
			avm_pa_multiplexer_remove_all_sessions( pa_instance_array[pa_nr] );
		}
	}
	spin_unlock_irqrestore( &hw_pa_list_lock, hlock_flags);

	return count;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/


static int proc_read_avm_pa_ifx_multiplexer_show(struct seq_file *seq, void *data __attribute__((unused))){

    int i;
    unsigned long hlock_flags;

    spin_lock_irqsave( &hw_pa_list_lock, hlock_flags);
    for (i = 0; i < MAX_HW_PA_INSTANCES; i++){
		if ( pa_instance_array[i] ){
            seq_printf(seq, "[%d] %s: {%s}\n", i, pa_instance_array[i]->name, (pa_instance_array[i]->enabled)?"enabled":"disabled" );
            seq_printf(seq, "           add: %pF\n", pa_instance_array[i]->add_session );
            seq_printf(seq, "           del: %pF\n", pa_instance_array[i]->remove_session );
            seq_printf(seq, "\n");
		}
	}
    spin_unlock_irqrestore( &hw_pa_list_lock, hlock_flags);
    ifx_ppa_show_virtual_devices(seq);
	return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

static int proc_read_avm_pa_ifx_multiplexer_open(struct inode *inode __attribute__((unused)), struct file *file){
    return single_open(file, proc_read_avm_pa_ifx_multiplexer_show, NULL);
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

static int avm_pa_multiplexer_stats(struct avm_pa_session *avm_session,
					                struct avm_pa_session_stats *stats)
{
	struct avm_pa_multiplexer_session *mp_session;

	memset(stats, 0, sizeof(struct avm_pa_session_stats));
	BUG_ON(avm_session->session_handle >= CONFIG_AVM_PA_MAX_SESSION);

	mp_session = &multiplexer_session_array[avm_session->session_handle];

	if ( ! atomic_read(&mp_session->active)){
		return -1;
	}

	if ( ! mp_session->hw_pa_instance->session_stats ){
		return -1;
	}

	return mp_session->hw_pa_instance->session_stats(avm_session, stats);
}

#ifndef AVM_PA_HARDWARE_PA_HAS_SESSION_STATS
static void check_stat(unsigned long dummy __attribute__((unused)))
{
	size_t i;
	struct avm_pa_multiplexer_session *session;

	for (i = 0 ; i < CONFIG_AVM_PA_MAX_SESSION; i++) {
		session = &multiplexer_session_array[ i ];
		if ( atomic_read(&session->active) ){
			struct avm_pa_session_stats stats;
			if (avm_pa_multiplexer_stats(session->avm_session, &stats) >= 0){
				avm_pa_hardware_session_report(i, stats.tx_pkts, stats.tx_bytes );
				if (stats.tx_bytes || stats.tx_pkts) {
					pr_debug("multiplexer_report %d: bytes=%llu, pkts=%d [%s]\n",
						i, stats.tx_bytes, stats.tx_pkts,
						session->hw_pa_instance->name);
                }
			}
		}
	}
	mod_timer(&stat_timer,  jiffies + HZ * DEFAULT_AVM_HIT_POLLING_TIME - 1);
}
#endif
/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

avm_session_handle (*ifx_lookup_routing_entry)(unsigned int routing_entry, unsigned int is_lan, unsigned int is_mc) = NULL;
EXPORT_SYMBOL( ifx_lookup_routing_entry );

