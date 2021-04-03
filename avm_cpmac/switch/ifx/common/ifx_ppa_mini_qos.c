#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/vmalloc.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <avm/net/net.h>
#include <avm/net/ifx_ppa_stack_al.h>
#include <avm/net/ifx_ppa_ppe_hal.h>
#include <avm/net/ifx_ppa_api.h>

#ifdef CONFIG_AVM_PA
#if __has_include(<avm/pa/pa.h>)
#include <avm/pa/pa.h>
#else
#include <linux/avm_pa.h>
#endif
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
#endif

#include <avmnet_config.h>

#include "swi_ifx_common.h"
#include "ifx_ppe_drv_wrapper.h"

#define PPE_SETUP_QOS_PROCFILE_NAME "ppe_setup_qos"

static int ppe_setup_qos_open(struct inode *inode, struct file *file);
static ssize_t ppe_setup_qos_write(struct file *filp, const char __user *buff, size_t len, loff_t *offset __attribute__ ((unused)) );

static const struct file_operations ppe_setup_qos_fops=
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 32)
   .owner   = THIS_MODULE,
#endif
   .open    = ppe_setup_qos_open,
   .read    = seq_read,
   .llseek  = seq_lseek,
   .release = single_release,
   .write    = ppe_setup_qos_write,
};



/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/

static int ppe_setup_qos_show(struct seq_file *seq, void *data __attribute__((unused))){
	PPA_QOS_STATUS qos_state = {0};
	uint32_t res;

	res = ifx_ppa_drv_get_qos_status(&qos_state, 0);
	if ( res != IFX_SUCCESS )
		return 0;

	seq_printf (seq, "port_id        = %u\n", qos_state.qos_queue_portid );
	seq_printf (seq, "time_tick      = %u\n", qos_state.time_tick     );
	seq_printf (seq, "overhd_bytes   = %u\n", qos_state.overhd_bytes  );
	seq_printf (seq, "eth1_eg_qnum   = %u\n", qos_state.eth1_eg_qnum  );
	seq_printf (seq, "eth1_burst_chk = %u\n", qos_state.eth1_burst_chk);
	seq_printf (seq, "eth1_qss       = %u\n", qos_state.eth1_qss      );
	seq_printf (seq, "shape_en       = %u\n", qos_state.shape_en      );
	seq_printf (seq, "wfq_en         = %u\n", qos_state.wfq_en        );

	if (qos_state.qos_queue_portid == 7){
		u32 i;
		for (i = 0; i < 4; i++){
			PPE_QOS_RATE_SHAPING_CFG rate_cfg = {0};
			rate_cfg.portid = 7;
			rate_cfg.queueid = ~i;

			if ( ifx_ppa_drv_get_qos_rate(&rate_cfg, 0) == IFX_SUCCESS)
				seq_printf( seq, "ptm_port %d: rate(kbps)=%d\n", i,
					    rate_cfg.rate_in_kbps);
			else
				seq_printf( seq, "ptm_port %d: failure\n", i);
		}
	}

	return 0;
}

/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static int ppe_setup_qos_open(struct inode *inode __attribute__((unused)), struct file *file){
    return single_open(file, ppe_setup_qos_show, NULL);
}


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static ssize_t ppe_setup_qos_write(struct file *filp __maybe_unused, const char __user *buff,
                                    size_t len, loff_t *offset __attribute__ ((unused)) ) {
    char mybuff[128], switchit[8];
    unsigned int port, overhd_bytes, qid, rate, weight;
    int status;
    int flags = 0;

    if(len >= 128){
        return -ENOMEM;
    }
    copy_from_user(&mybuff[0], buff, len);
    mybuff[len] = 0;

    status = len;

    if(sscanf(mybuff, "setctrlwfq %u %u %7s" , &port, &overhd_bytes, &switchit[0]) == 2){
        if (strncmp(switchit, "enable",6 ) == 0){
            PPE_QOS_ENABLE_CFG enable_cfg = {0};

            printk(KERN_ERR "[%s] setctrlwfq %u enable\n", __func__, port);
            enable_cfg.portid = port;
            enable_cfg.f_enable = 1;
            enable_cfg.overhd_bytes = overhd_bytes;
            enable_cfg.flag |= PPE_QOS_ENABLE_CFG_FLAG_SETUP_OVERHD_BYTES;
            ifx_ppa_drv_set_ctrl_qos_wfq(&enable_cfg, flags);

        }
        else if (strncmp(switchit, "disable", 7 ) == 0){
            PPE_QOS_ENABLE_CFG enable_cfg = {0};

            printk(KERN_ERR "[%s] setctrlwfq %u disable\n", __func__, port);
            enable_cfg.portid = port;
            enable_cfg.f_enable = 0;
            ifx_ppa_drv_set_ctrl_qos_wfq(&enable_cfg, flags);
        }
    }

    else if(sscanf(mybuff, "setwfq %u %u %u" , &port, &qid, &weight) == 3){
        PPE_QOS_WFQ_CFG wfq_cfg = {0};

        printk(KERN_ERR "[%s] setwfq %u %u %u \n", __func__, port, qid, weight);
        wfq_cfg.portid = port;
        wfq_cfg.queueid = qid;
        wfq_cfg.weight_level = weight;
        ifx_ppa_drv_set_qos_wfq(&wfq_cfg, flags);

    }

    else if(sscanf(mybuff, "setctrlrate %u %u %7s" ,&port, &overhd_bytes, &switchit[0]) == 3){
        if (strncmp(switchit, "enable",6 ) == 0){
            PPE_QOS_ENABLE_CFG enable_cfg = {0};

            printk(KERN_ERR "[%s] setctrlrate %u enable\n", __func__, port);

            enable_cfg.portid = port;
            enable_cfg.f_enable = 1;
            enable_cfg.overhd_bytes = overhd_bytes;
            enable_cfg.flag |= PPE_QOS_ENABLE_CFG_FLAG_SETUP_OVERHD_BYTES ;
            ifx_ppa_drv_set_ctrl_qos_rate(&enable_cfg, flags);
        }
        else if (strncmp(switchit, "disable", 7 ) == 0){
            PPE_QOS_ENABLE_CFG enable_cfg = {0};

            printk(KERN_ERR "[%s] setctrlrate %u disable\n", __func__, port);

            enable_cfg.portid = port;
            enable_cfg.f_enable = 0;
            ifx_ppa_drv_set_ctrl_qos_rate(&enable_cfg, flags);
        }
    }

    else if(sscanf(mybuff, "setrateport %u %u" , &port, &rate ) == 2){
            PPE_QOS_RATE_SHAPING_CFG rate_cfg = {0};
            printk(KERN_ERR "[%s] setrateport %u %u\n", __func__, port, rate);
            rate_cfg.burst = 0; //default burst
            rate_cfg.rate_in_kbps = rate;
            rate_cfg.portid = port;
            rate_cfg.queueid = -1; // If queue id bigger than muximum queue id, it will be regarded as port based rate shaping.
            ifx_ppa_drv_set_qos_rate( &rate_cfg , flags);
    }

    else if(sscanf(mybuff, "setratequeue %u %u %u" , &port, &qid, &rate) == 3){
            PPE_QOS_RATE_SHAPING_CFG rate_cfg = {0};
            printk(KERN_ERR "[%s] setratequeue %u %u %u \n", __func__, port, qid, rate);
            rate_cfg.burst = 0; //default burst
            rate_cfg.rate_in_kbps = rate;
            rate_cfg.portid = port;
            rate_cfg.queueid = qid;
            ifx_ppa_drv_set_qos_rate( &rate_cfg , flags);
    }
    else if(sscanf(mybuff, "avmwansetrate %u" , &rate ) == 1){
            printk(KERN_ERR "[%s] avmwansetrate %u\n", __func__, rate);
            avmnet_set_wan_tx_rate_shaping(rate);
    }

    else if(sscanf(mybuff, "avmwanqueuesetrate %u %u" , &qid, &rate) == 2){
            printk(KERN_ERR "[%s] avmwanqueuesetrate %u %u\n", __func__, qid, rate);
            avmnet_set_wan_tx_queue_rate_shaping(qid, rate);
    }

    else {
        status = -EINVAL;
    }

    if(status == -EINVAL){
        printk(KERN_ERR "[%s] Usage:\n", __func__);
        printk(KERN_ERR "[%s]     setctrlwfq <port> <overhd_bytes> <enable|disable>\n", __func__);
        printk(KERN_ERR "[%s]     setwfq <port> <qid> <weight-level-in-percent>\n", __func__);
        printk(KERN_ERR "[%s]     setctrlrate <port> <overhd_bytes> <enable|disable>\n", __func__);
        printk(KERN_ERR "[%s]     setrateport <port> <rate_in_kbps>\n", __func__);
        printk(KERN_ERR "[%s]     setratequeue <port> <queue> <rate_in_kbps>\n", __func__);
        printk(KERN_ERR "[%s]     avmwansetrate <rate_in_kbps>\n", __func__);
        printk(KERN_ERR "[%s]     avmwanqueuesetrate <queue> <rate_in_kbps>\n", __func__);
    }

    return status;
}


int __init ifx_ppa_mini_qos_init(void) {

    avmnet_cfg_add_seq_procentry( avmnet_hw_config_entry->config, PPE_SETUP_QOS_PROCFILE_NAME, &ppe_setup_qos_fops);
    printk("AVM - Lantiq-PPA: minimalist and slim QoS Management: --- init successful\n");
    return 0;

}

void __exit ifx_ppa_mini_qos_exit(void) {
    avmnet_cfg_remove_procentry( avmnet_hw_config_entry->config, PPE_SETUP_QOS_PROCFILE_NAME );
}

module_init(ifx_ppa_mini_qos_init);
module_exit(ifx_ppa_mini_qos_exit);
