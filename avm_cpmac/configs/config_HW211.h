#ifndef __AVM_NET_CFG_HW211_H_
#define __AVM_NET_CFG_HW211_H_
/* avmnet configuration for FB4080 */

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
#endif

#include <avmnet_module.h>
#include <avmnet_config.h>

#include <switch/akronite/akronite.h>
#include <phy/avmnet_ar803x.h>
#ifdef CONFIG_DUMMY_PHY
#include <phy/avmnet_dummy.h>
#define AVMNET_HAS_RAW_DEVICE
#endif
#include <phy/avmnet_ar8337.h>
#include <linux/etherdevice.h>

#if defined(CONFIG_AVM_SCATTER_GATHER)
#define NET_DEV_FEATURES	NETIF_F_SG | NETIF_F_IP_CSUM | NETIF_F_RXCSUM
#else
#define NET_DEV_FEATURES	NETIF_F_IP_CSUM | NETIF_F_RXCSUM
#endif

/*
  FBOX ETH PORTS             8337N                               
     +------+            +------------+                            
     | WAN  +------------+ 1 -----+   |                            
     +------+            |        |   |         
                         |        |   |         Akronite                              
     +------+            |        |   |        +--------+          
     | LAN1 +------------+ 2 -+   |   |        |        |          
     +------+            |    |   |   | RGMII  |        |          
                         |    |   + 0 +--------+ gmac0  |          
     +------+            |    |       |        |        |          
     | LAN2 +------------+ 3 -+       |        |        |          
     +------+            |    |       | SGMII  |        |          
                         |    +---- 6 +--------+ gmac1  |          
     +------+            |    |       |        |        |          
     | LAN3 +------------+ 4 -+       |        |        |          
     +------+            |    |       |        |        |          
                         |    |       |        |        |          
     +------+            |    |       |        +--------+          
     | LAN4 +------------+ 5 -+       |                            
     +------+            +------------+                            
 */

static avmnet_module_t hw211_avmnet_akronite_rgmii, hw211_avmnet_akronite_sgmii;

static avmnet_module_t hw211_avmnet_switch_mac0_rgmii;
static avmnet_module_t hw211_avmnet_switch_mac1;
static avmnet_module_t hw211_avmnet_switch_mac2;
static avmnet_module_t hw211_avmnet_switch_mac3;
static avmnet_module_t hw211_avmnet_switch_mac4;
static avmnet_module_t hw211_avmnet_switch_mac5;
static avmnet_module_t hw211_avmnet_switch_mac6_sgmii;
#ifdef AVMNET_HAS_RAW_DEVICE
static avmnet_module_t hw211_avmnet_switch_mac7_virtual;
#endif

static avmnet_module_t avmnet_hw211 ____cacheline_aligned = {
	.name = "akronite",
	.type = avmnet_modtype_switch,
	.priv = NULL,
	.initdata = {.mac = {.flags = 0}},

	.init = avmnet_akronite_init,
	.setup = avmnet_akronite_setup,
	.exit = avmnet_akronite_exit,

#ifdef AVMNET_HAS_RAW_DEVICE
	.reg_read32 = avmnet_akronite_ar8337_reg_read32,
	.reg_write32 = avmnet_akronite_ar8337_reg_write32,
#endif

	.reg_read = avmnet_akronite_mdio_read,
	.reg_write = avmnet_akronite_mdio_write,

	.lock = avmnet_akronite_lock,
	.unlock = avmnet_akronite_unlock,
	.poll = avmnet_poll_children,

	.parent = NULL,
	.num_children = 2,
	.children = {
		&hw211_avmnet_akronite_rgmii,
		&hw211_avmnet_akronite_sgmii,
	},
};

static avmnet_module_t hw211_avmnet_akronite_rgmii = {
	.name = "akronite_rgmii",
	.type = avmnet_modtype_switch,
	.initdata.mac = {
		.flags = AVMNET_CONFIG_FLAG_OPEN_FIRMWARE,
		.of_compatible = "qcom,nss-gmac0",
		.mac_mode = MAC_MODE_RGMII_1000,
	},

	.init = avmnet_ipq_mii_init,
	.setup = avmnet_ipq_mii_setup,
	.exit = avmnet_generic_exit,		//avmnet_ar8326_exit,

#ifdef AVMNET_HAS_RAW_DEVICE
	.reg_read32 = avmnet_reg_read32_parent,
	.reg_write32 = avmnet_reg_write32_parent,
#endif
	.reg_read = avmnet_reg_read_parent,
	.reg_write = avmnet_reg_write_parent,
	.lock = avmnet_lock_parent,
	.unlock = avmnet_unlock_parent,
	.status_changed = NULL,	//avmnet_ar8326_status_changed,
	.poll = avmnet_poll_children,		//avmnet_ar8326_status_poll,
	.set_status = avmnet_set_status_parent,

	.parent = &avmnet_hw211,
#ifdef AVMNET_HAS_RAW_DEVICE
	.num_children = 2,
	.children = {
		&hw211_avmnet_switch_mac0_rgmii,
		&hw211_avmnet_switch_mac7_virtual,
	},
#else
	.num_children = 1,
	.children = {
		&hw211_avmnet_switch_mac0_rgmii,
	},
#endif
};

static avmnet_module_t hw211_avmnet_akronite_sgmii = {
	.name = "akronite_sgmii",
	.type = avmnet_modtype_switch,
	.initdata.mac = {
		.flags = AVMNET_CONFIG_FLAG_OPEN_FIRMWARE,
		.of_compatible = "qcom,nss-gmac1",
		.mac_mode = MAC_MODE_SGMII,
	},

	.init = avmnet_ipq_mii_init,
	.setup = avmnet_ipq_mii_setup,
	.exit = avmnet_generic_exit,	//avmnet_ar8326_exit,

	.reg_read = avmnet_reg_read_parent,
	.reg_write = avmnet_reg_write_parent,
	.lock = avmnet_lock_parent,
	.unlock = avmnet_unlock_parent,
	.status_changed = NULL,	//avmnet_ar8326_status_changed,
	.poll = avmnet_poll_children,		//avmnet_ar8326_status_poll,
	.set_status = avmnet_set_status_parent,

	.parent = &avmnet_hw211,
	.num_children = 1,
	.children = {
		&hw211_avmnet_switch_mac6_sgmii,
	},
};


static avmnet_module_t hw211_avmnet_switch_mac0_rgmii = {
	.name = "switch_mac0_rgmii",
	.type = avmnet_modtype_switch,
	.initdata       = {
		.swi = {
			.flags = AVMNET_CONFIG_FLAG_RESET | AVMNET_CONFIG_FLAG_SWITCH_WAN,
			.reset = 53,	/* TODO: Read from device-tree */
			.cpu_mac_port = 0,
		},
	},
	.init           = avmnet_ar8337_init,
	.setup          = avmnet_ar8337_ipq8064_setup,
	.exit           = avmnet_ar8337_exit,

	.reg_read       = avmnet_s17_rd_phy,
	.reg_write      = avmnet_s17_wr_phy,
	.lock           = avmnet_s17_lock,
	.unlock         = avmnet_s17_unlock,
	.status_changed = avmnet_ar8337_status_changed,
	.poll           = avmnet_ar8337_status_poll,
	.set_status     = avmnet_ar8337_set_status,
	.setup_irq      = avmnet_ar8337_setup_interrupt,

	.parent = &hw211_avmnet_akronite_rgmii,
	.num_children = 1,
	.children = {
		&hw211_avmnet_switch_mac1,
	},
};


static avmnet_module_t hw211_avmnet_switch_mac6_sgmii = {
	.name = "switch_mac6_sgmii",
	.type = avmnet_modtype_switch,
	.initdata	={
		.swi = {
			.flags = AVMNET_CONFIG_FLAG_RESET | AVMNET_CONFIG_FLAG_SWITCH_LAN,
			.reset = 53,	/* TODO: Read from device-tree */
			.cpu_mac_port = 6,
		},
	},

	.init  = avmnet_ar8337_init,
	.setup = avmnet_generic_setup,
			/* Switch-Setup durch hw211_avmnet_switch_mac0_rgmii */
	.exit  = avmnet_ar8337_exit,

	.reg_read       = avmnet_s17_rd_phy,
	.reg_write      = avmnet_s17_wr_phy,
	.lock           = avmnet_s17_lock,
	.unlock         = avmnet_s17_unlock,
	.status_changed = avmnet_ar8337_status_changed,
	.poll           = avmnet_ar8337_status_poll,
	.set_status     = avmnet_ar8337_set_status,
	.setup_irq      = avmnet_ar8337_setup_interrupt,

	.parent = &hw211_avmnet_akronite_sgmii,
	.num_children = 4,
	.children = {
		&hw211_avmnet_switch_mac2,
		&hw211_avmnet_switch_mac3,
		&hw211_avmnet_switch_mac4,
		&hw211_avmnet_switch_mac5,
	},
};

static avmnet_device_t avmnet_hw211_avm_device_wan = {
	.device = NULL,
	.device_name = "wan",
	.external_port_no = 0,
	.net_dev_features  =  NET_DEV_FEATURES,
	.device_ops = {
		.ndo_init = NULL,	/* TODO */
		.ndo_get_stats = NULL,	/* TODO */
		.ndo_open = avmnet_netdev_open,
		.ndo_stop = avmnet_netdev_stop,
		.ndo_set_mac_address = eth_mac_addr,
		.ndo_do_ioctl = NULL,	/* TODO */
		.ndo_tx_timeout = NULL,	/* TODO */
		.ndo_start_xmit = avmnet_ipq_gmac_start_xmit, /* cmp. mac_module */
		.ndo_change_mtu = avmnet_ipq_gmac_change_mtu,
	},
	.mac_module = &hw211_avmnet_akronite_rgmii,
	.sizeof_priv = sizeof(struct avmnet_ipq_gmac_netdev_private),
	.device_setup = avmnet_ipq_gmac_setup_netdev,
	.device_setup_priv = avmnet_ipq_gmac_setup_netdev_private,
	.device_setup_late = avmnet_ipq_gmac_setup_netdev_late,
    .flags = AVMNET_CONFIG_FLAG_SWITCH_WAN | AVMNET_DEVICE_FLAG_NO_MCFW,
};

static avmnet_module_t hw211_avmnet_phy_0;

static avmnet_module_t hw211_avmnet_switch_mac1 = {
	.name = "switch_mac1",
	.type = avmnet_modtype_mac,
	.priv = NULL,
	.initdata.mac = {
		.mac_nr = 1,
	},

	.init = avmnet_akronite_switch_mac_init,
	.setup = avmnet_generic_setup,

	.reg_read = avmnet_reg_read_parent,
	.reg_write = avmnet_reg_write_parent,

	.poll = avmnet_poll_children,
	.set_status = avmnet_akronite_mac_set_status,

	.parent = &hw211_avmnet_switch_mac0_rgmii,
	.num_children = 1,
	.children = { &hw211_avmnet_phy_0, },
};

static avmnet_module_t hw211_avmnet_phy_0 ____cacheline_aligned = {
	.name = "ar803x0",
	.device_id = &avmnet_hw211_avm_device_wan,
	.type = avmnet_modtype_phy,
	.priv = NULL,
	.initdata.phy = {
		.flags = AVMNET_CONFIG_FLAG_MDIOADDR |
			 AVMNET_CONFIG_FLAG_MDIOPOLLING |
			 AVMNET_CONFIG_FLAG_PHY_GBIT |
			 AVMNET_CONFIG_FLAG_INTERNAL,
		.mdio_addr = 0
	},	/* TODO: MDIO!  */

	AR803X_STDFUNCS,
	.ethtool_ops = AR803X_ETHOPS,
	.parent = &hw211_avmnet_switch_mac1,
};

static avmnet_device_t avmnet_hw211_avm_device_eth0 = {
	.device = NULL,
	.device_name = "eth0",
	.external_port_no = 1,
	.device_ops = {
		.ndo_init = NULL,	/* TODO */
		.ndo_get_stats = NULL,	/* TODO */
		.ndo_open = avmnet_netdev_open,
		.ndo_stop = avmnet_netdev_stop,
		.ndo_set_mac_address = eth_mac_addr,
		.ndo_do_ioctl = NULL,	/* TODO */
		.ndo_tx_timeout = NULL,	/* TODO */
		.ndo_start_xmit = avmnet_ipq_gmac_start_xmit, /* cmp. mac_module */
		.ndo_change_mtu = avmnet_ipq_gmac_change_mtu,
	},
	.mac_module = &hw211_avmnet_akronite_sgmii,
	.sizeof_priv = sizeof(struct avmnet_ipq_gmac_netdev_private),
	.device_setup = avmnet_ipq_gmac_setup_netdev,
	.device_setup_priv = avmnet_ipq_gmac_setup_netdev_private,
	.device_setup_late = avmnet_ipq_gmac_setup_netdev_late,
	.flags = AVMNET_DEVICE_IFXPPA_ETH_LAN,
};

static avmnet_module_t hw211_avmnet_phy_1 ____cacheline_aligned = {
	.name = "ar803x1",
	.device_id = &avmnet_hw211_avm_device_eth0,
	.type = avmnet_modtype_phy,
	.priv = NULL,
	.initdata.phy = {
		.flags = AVMNET_CONFIG_FLAG_MDIOADDR |
			 AVMNET_CONFIG_FLAG_MDIOPOLLING |
			 AVMNET_CONFIG_FLAG_PHY_GBIT |
			 AVMNET_CONFIG_FLAG_INTERNAL,
		.mdio_addr = 1,		/* TODO: Read from device-tree */
	},

	AR803X_STDFUNCS,
	.ethtool_ops = AR803X_ETHOPS,
	.parent = &hw211_avmnet_switch_mac2,
};

static avmnet_module_t hw211_avmnet_switch_mac2 = {
	.name = "switch_mac2",
	.type = avmnet_modtype_mac,
	.priv = NULL,
	.initdata.mac = {
		.mac_nr = 2,
	},

	.init = avmnet_akronite_switch_mac_init,
	.setup = avmnet_generic_setup,

	.reg_read = avmnet_reg_read_parent,
	.reg_write = avmnet_reg_write_parent,

	.poll = avmnet_poll_children,
	.set_status = avmnet_akronite_mac_set_status,

	.parent = &hw211_avmnet_switch_mac6_sgmii,
	.num_children = 1,
	.children = { &hw211_avmnet_phy_1, },
};

static avmnet_device_t avmnet_hw211_avm_device_eth1 = {
	.device = NULL,
	.device_name = "eth1",
	.external_port_no = 2,
	.net_dev_features  =  NET_DEV_FEATURES,
	.device_ops = {
		.ndo_init = NULL,	/* TODO */
		.ndo_get_stats = NULL,	/* TODO */
		.ndo_open = avmnet_netdev_open,
		.ndo_stop = avmnet_netdev_stop,
		.ndo_set_mac_address = eth_mac_addr,
		.ndo_do_ioctl = NULL,	/* TODO */
		.ndo_tx_timeout = NULL,	/* TODO */
		.ndo_start_xmit = avmnet_ipq_gmac_start_xmit, /* cmp. mac_module */
		.ndo_change_mtu = avmnet_ipq_gmac_change_mtu,
	},
	.mac_module = &hw211_avmnet_akronite_sgmii,
	.sizeof_priv = sizeof(struct avmnet_ipq_gmac_netdev_private),
	.device_setup = avmnet_ipq_gmac_setup_netdev,
	.device_setup_priv = avmnet_ipq_gmac_setup_netdev_private,
	.device_setup_late = avmnet_ipq_gmac_setup_netdev_late,
	.flags = AVMNET_DEVICE_IFXPPA_ETH_LAN,
};

static avmnet_module_t hw211_avmnet_phy_2 ____cacheline_aligned = {
	.name = "ar803x2",
	.device_id = &avmnet_hw211_avm_device_eth1,
	.type = avmnet_modtype_phy,
	.priv = NULL,
	.initdata.phy = {
		.flags = AVMNET_CONFIG_FLAG_MDIOADDR |
			 AVMNET_CONFIG_FLAG_MDIOPOLLING |
			 AVMNET_CONFIG_FLAG_PHY_GBIT |
			 AVMNET_CONFIG_FLAG_INTERNAL,
		.mdio_addr = 2,
	},	/* TODO: MDIO! */

	AR803X_STDFUNCS,
	.ethtool_ops = AR803X_ETHOPS,
	.parent = &hw211_avmnet_switch_mac3,
};

static avmnet_module_t hw211_avmnet_switch_mac3 = {
	.name = "switch_mac3",
	.type = avmnet_modtype_mac,
	.priv = NULL,
	.initdata.mac = {
		.mac_nr = 3,
	},

	.init = avmnet_akronite_switch_mac_init,
	.setup = avmnet_generic_setup,

	.reg_read = avmnet_reg_read_parent,
	.reg_write = avmnet_reg_write_parent,

	.poll = avmnet_poll_children,
	.set_status = avmnet_akronite_mac_set_status,

	.parent = &hw211_avmnet_switch_mac6_sgmii,
	.num_children = 1,
	.children = { &hw211_avmnet_phy_2 },
};

static avmnet_device_t avmnet_hw211_avm_device_eth2 = {
	.device = NULL,
	.device_name = "eth2",
	.external_port_no = 3,
	.net_dev_features  =  NET_DEV_FEATURES,
	.device_ops = {
		.ndo_init = NULL,	/* TODO */
		.ndo_get_stats = NULL,	/* TODO */
		.ndo_open = avmnet_netdev_open,
		.ndo_stop = avmnet_netdev_stop,
		.ndo_set_mac_address = eth_mac_addr,
		.ndo_do_ioctl = NULL,	/* TODO */
		.ndo_tx_timeout = NULL,	/* TODO */
		.ndo_start_xmit = avmnet_ipq_gmac_start_xmit, /* cmp. mac_module */
		.ndo_change_mtu = avmnet_ipq_gmac_change_mtu,
	},
	.mac_module = &hw211_avmnet_akronite_sgmii,
	.sizeof_priv = sizeof(struct avmnet_ipq_gmac_netdev_private),
	.device_setup = avmnet_ipq_gmac_setup_netdev,
	.device_setup_priv = avmnet_ipq_gmac_setup_netdev_private,
	.device_setup_late = avmnet_ipq_gmac_setup_netdev_late,
	.flags = AVMNET_DEVICE_IFXPPA_ETH_LAN,
};

static avmnet_module_t hw211_avmnet_phy_3 ____cacheline_aligned = {
	.name = "ar803x3",
	.device_id = &avmnet_hw211_avm_device_eth2,
	.type = avmnet_modtype_phy,
	.priv = NULL,
	.initdata.phy = {
		.flags = AVMNET_CONFIG_FLAG_MDIOADDR |
			 AVMNET_CONFIG_FLAG_MDIOPOLLING |
			 AVMNET_CONFIG_FLAG_PHY_GBIT |
			 AVMNET_CONFIG_FLAG_INTERNAL,
		.mdio_addr = 3,		/* TODO: Read from device-tree */
	},

	AR803X_STDFUNCS,
	.ethtool_ops = AR803X_ETHOPS,
	.parent = &hw211_avmnet_switch_mac4,
};

static avmnet_module_t hw211_avmnet_switch_mac4 = {
	.name = "switch_mac4",
	.type = avmnet_modtype_mac,
	.priv = NULL,
	.initdata.mac = {
		.mac_nr = 4,
	},

	.init = avmnet_akronite_switch_mac_init,
	.setup = avmnet_generic_setup,

	.reg_read = avmnet_reg_read_parent,
	.reg_write = avmnet_reg_write_parent,

	.poll = avmnet_poll_children,
	.set_status = avmnet_akronite_mac_set_status,

	.parent = &hw211_avmnet_switch_mac6_sgmii,
	.num_children = 1,
	.children = { &hw211_avmnet_phy_3 },
};

static avmnet_device_t avmnet_hw211_avm_device_eth3 = {
	.device = NULL,
	.device_name = "eth3",
	.external_port_no = 4,
	.net_dev_features  =  NET_DEV_FEATURES,
	.device_ops = {
		.ndo_init = NULL,	/* TODO */
		.ndo_get_stats = NULL,	/* TODO */
		.ndo_open = avmnet_netdev_open,
		.ndo_stop = avmnet_netdev_stop,
		.ndo_set_mac_address = eth_mac_addr,
		.ndo_do_ioctl = NULL,	/* TODO */
		.ndo_tx_timeout = NULL,	/* TODO */
		.ndo_start_xmit = avmnet_ipq_gmac_start_xmit, /* cmp. mac_module */
		.ndo_change_mtu = avmnet_ipq_gmac_change_mtu,
	},
	.mac_module = &hw211_avmnet_akronite_sgmii,
	.sizeof_priv = sizeof(struct avmnet_ipq_gmac_netdev_private),
	.device_setup = avmnet_ipq_gmac_setup_netdev,
	.device_setup_priv = avmnet_ipq_gmac_setup_netdev_private,
	.device_setup_late = avmnet_ipq_gmac_setup_netdev_late,
	.flags = AVMNET_DEVICE_IFXPPA_ETH_LAN,
};

static avmnet_module_t hw211_avmnet_phy_4 ____cacheline_aligned = {
	.name = "ar803x4",
	.device_id = &avmnet_hw211_avm_device_eth3,
	.type = avmnet_modtype_phy,
	.priv = NULL,
	.initdata.phy = {
		.flags = AVMNET_CONFIG_FLAG_MDIOADDR |
			 AVMNET_CONFIG_FLAG_MDIOPOLLING |
			 AVMNET_CONFIG_FLAG_PHY_GBIT |
			 AVMNET_CONFIG_FLAG_INTERNAL,
		.mdio_addr = 4,		/* TODO: Read from device-tree */
	},

	AR803X_STDFUNCS,
	.ethtool_ops = AR803X_ETHOPS,
	.parent = &hw211_avmnet_switch_mac5,
};

static avmnet_module_t hw211_avmnet_switch_mac5 = {
	.name = "switch_mac5",
	.type = avmnet_modtype_mac,
	.priv = NULL,
	.initdata.mac = {
		.mac_nr = 5,
	},

	.init = avmnet_akronite_switch_mac_init,
	.setup = avmnet_generic_setup,

	.reg_read = avmnet_reg_read_parent,
	.reg_write = avmnet_reg_write_parent,

	.poll = avmnet_poll_children,
	.set_status = avmnet_akronite_mac_set_status,

	.parent = &hw211_avmnet_switch_mac6_sgmii,
	.num_children = 1,
	.children = { &hw211_avmnet_phy_4 },
};
#if 0
#ifdef AVMNET_HAS_RAW_DEVICE
static avmnet_device_t avmnet_hw211_avm_device_raw = {
	.device = NULL,
	.device_name = "raw",
	.external_port_no = AVMNET_DEVICE_INVALID_EXTERNAL_PORT,
	.device_ops = {
		.ndo_init = NULL,	/* TODO */
		.ndo_get_stats = NULL,	/* TODO */
		.ndo_open = avmnet_netdev_open,
		.ndo_stop = avmnet_netdev_stop,
		.ndo_set_mac_address = eth_mac_addr,
		.ndo_do_ioctl = NULL,	/* TODO */
		.ndo_tx_timeout = NULL,	/* TODO */
		.ndo_start_xmit = avmnet_ipq_gmac_start_xmit, /* cmp. mac_module */
		.ndo_change_mtu = avmnet_ipq_gmac_change_mtu,
	},
	.mac_module = &hw211_avmnet_akronite_rgmii,
	.sizeof_priv = sizeof(struct avmnet_ipq_raw_gmac_netdev_private),
	.device_setup = avmnet_ipq_gmac_setup_netdev,
	.device_setup_priv = avmnet_ipq_raw_setup_netdev_private,
	.device_setup_late = avmnet_ipq_gmac_setup_netdev_late,
};
#endif

static avmnet_module_t hw211_avmnet_phy_5_virtual ____cacheline_aligned = {
	.name = "virtual_phy",
	.device_id = &avmnet_hw211_avm_device_raw,
	.type = avmnet_modtype_phy,
	.priv = NULL,

	.init = avmnet_dummy_init,
	.setup = avmnet_generic_setup,
	.exit = avmnet_generic_exit,
	.lock = avmnet_dummy_lock,
	.unlock = avmnet_dummy_unlock,

	.powerup = avmnet_dummy_powerup,
	.powerdown = avmnet_dummy_powerdown,

	.suspend = avmnet_dummy_suspend, // stub
	.resume = avmnet_dummy_resume, // stub
	.reinit = avmnet_dummy_reinit, // stub
	.poll = avmnet_dummy_poll,
	.ethtool_ops =
	 {
	  .get_link = avmnet_dummy_ethtool_get_link,
	 },
	.parent = &hw211_avmnet_switch_mac7_virtual,
};

static avmnet_module_t hw211_avmnet_switch_mac7_virtual = {
	.name = "switch_mac7_virtual",
	.type = avmnet_modtype_mac,
	.priv = NULL,
	.initdata.mac = {
		.mac_nr = 7,
	},

	.init = avmnet_akronite_switch_mac_init,
	.setup = avmnet_generic_setup,

	.reg_read = avmnet_reg_read_parent,
	.reg_write = avmnet_reg_write_parent,

	.poll = avmnet_poll_children,
	.set_status = avmnet_akronite_mac_set_status,

	.parent = &hw211_avmnet_switch_mac0_rgmii,
	.num_children = 1,
	.children = { &hw211_avmnet_phy_5_virtual },
};
#endif

static avmnet_device_t *avmnet_hw211_avm_devices[] = {
	// order matters
	&avmnet_hw211_avm_device_wan,
	&avmnet_hw211_avm_device_eth0,
	&avmnet_hw211_avm_device_eth1,
	&avmnet_hw211_avm_device_eth2,
	&avmnet_hw211_avm_device_eth3,
#ifdef AVMNET_HAS_RAW_DEVICE
	&avmnet_hw211_avm_device_raw
#endif
};
#endif
