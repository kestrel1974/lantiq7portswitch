#ifndef ATHRS_PHY_CTRL_H
#define ATHRS_PHY_CTRL_H


#define athr_mib(x)    (0x20000 |(x*(0x100)))
#define athr_mibs17(x) (0x1000  |(x*(0x100)))
#define athr_flctrl17(x) (0x7c  + x*(0x4))
#define athr_flctrl(x)   (0x100 + x*(0x100))

#if defined(CONFIG_ATHRS17_PHY)
#define athr_mibreg(x)  athr_mibs17(x)
#define ATHR_PHY_MIB_CTRL_REG		     0x0030
#define ATHR_PHY_MIB_ENABLE		     0x1
#define athr_flctrl_off(x) athr_flctrl17(x)
#else
#define ATHR_PHY_MIB_CTRL_REG		     0x0080
#define ATHR_PHY_MIB_ENABLE		     (1 << 30)
#define athr_mibreg(x)  athr_mib(x) 
#define athr_flctrl_off(x) athr_flctrl(x)
#endif

#define ATHR_PHY_TXFCTL_EN                   (1 << 4)
#define ATHR_PHY_RXFCTL_EN                   (1 << 5)

/*
 * PHY Registers common 
 * to all phys
 */
#define ATHR_PHY_CONTROL                 0
#define     PHY_CTRL_SOFTWARE_RESET           (1 << 15)
#define     PHY_CTRL_SPEED_LSB                (1 << 13)
#define     PHY_CTRL_AUTONEGOTIATION_ENABLE   (1 << 12)
#define     PHY_CTRL_ISOLATE                  (1 << 10)
#define     PHY_CTRL_RESTART_AUTONEGOTIATION  (1 <<  9)
#define     PHY_CTRL_SPEED_FULL_DUPLEX        (1 <<  8)
#define     PHY_CTRL_SPEED_MSB                (1 <<  6)

#define ATHR_PHY_STATUS                  1
#define     PHY_STATUS_AUTO_NEG_IN_PROCESS    (1 <<  5)
#define ATHR_PHY_ID1                     2
#define ATHR_PHY_ID2                     3
#define ATHR_PHY_AUTONEG_ADVERT          4
#define     PHY_ADVERTISE_NEXT_PAGE           (1 << 15)
#define     PHY_ADVERTISE_ASYM_PAUSE          (1 << 11)
#define     PHY_ADVERTISE_PAUSE               (1 << 10)
#define     PHY_ADVERTISE_100FULL             (1 <<  8)
#define     PHY_ADVERTISE_100HALF             (1 <<  7)
#define     PHY_ADVERTISE_10FULL              (1 <<  6)
#define     PHY_ADVERTISE_10HALF              (1 <<  5)

#define ATHR_PHY_LINK_PARTNER_ABILITY    5
#define     PHY_LINK_100BASETX_FULL_DUPLEX    (1 <<  8)
#define     PHY_LINK_100BASETX                (1 <<  7)
#define     PHY_LINK_10BASETX_FULL_DUPLEX     (1 <<  6)
#define     PHY_LINK_10BASETX                 (1 <<  5)

#define ATHR_PHY_AUTONEG_EXPANSION       6
#define ATHR_PHY_NEXT_PAGE_TRANSMIT      7
#define ATHR_PHY_LINK_PARTNER_NEXT_PAGE  8
#define ATHR_PHY_1000BASET_CONTROL       9
#define     PHY_ADVERTISE_1000FULL            (1 <<  9)
#define     PHY_ADVERTISE_1000HALF		      (1 <<  8)

#define ATHR_PHY_1000BASET_STATUS        10
#define ATHR_PHY_FUNC_CONTROL            16
#define ATHR_PHY_SPEC_STATUS             17
#define     PHY_SPEC_STATUS_LINK_MASK         (3 << 14)
#define     PHY_SPEC_STATUS_LINK_SHIFT        14
#define     PHY_SPEC_STATUS_LINK_10M		   0
#define     PHY_SPEC_STATUS_LINK_100M		   1
#define     PHY_SPEC_STATUS_LINK_1000M		   2
#define     PHY_SPEC_STATUS_FULL_DUPLEX       (1 << 13)
#define     PHY_SPEC_STATUS_RESOLVED          (1 << 11)
#define     PHY_SPEC_STATUS_LINK_PASS         (1 << 10)
#define     PHY_SPEC_STATUS_MR_AN_COMPLETE    (1 << 9)      /*--- nur ar8033 ---*/
#define     PHY_SPEC_STATUS_SYNC              (1 << 8)      /*--- nur ar8033 ---*/
#define		PHY_SPEC_STATUS_MDIX			  (1 << 6)
#define     PHY_SPEC_STATUS_SMSPD_DOWN        (1 << 5)
#define     PHY_SPEC_STATUS_ENRGY_DETECT      (1 << 4)
#define     PHY_SPEC_STATUS_TXPAUSE_EN        (1 << 3)
#define     PHY_SPEC_STATUS_RXPAUSE_EN        (1 << 2)
#define     PHY_SPEC_STATUS_POLARITY          (1 << 1)
#define     PHY_SPEC_STATUS_JABBER            (1 << 0)

#define ATHR_PHY_INTR_ENABLE             18
#define ATHR_PHY_INTR_STATUS             19
#define     PHY_INT_STATUS_JABBER           (1 <<  0)
#define     PHY_INT_STATUS_POLARITY         (1 <<  1)
/* 2 - 4 reserved */
#define     PHY_INT_STATUS_WIRE_DOWNGRADE   (1 <<  5)
#define     PHY_INT_STATUS_MDI_CROSSOVER    (1 <<  6)
#define     PHY_INT_STATUS_FIFO_ERROR       (1 <<  7)
#define     PHY_INT_STATUS_CARRIER          (1 <<  8)
#define     PHY_INT_STATUS_SYMBOLERROR      (1 <<  9)
#define     PHY_INT_STATUS_LINK_UP          (1 << 10)
#define     PHY_INT_STATUS_LINK_DOWN        (1 << 11)
#define     PHY_INT_STATUS_PAGERCV          (1 << 12)
#define     PHY_INT_STATUS_DUPLEX           (1 << 13)
#define     PHY_INT_STATUS_SPEED            (1 << 14)
#define     PHY_INT_STATUS_AUTONEG_ERR      (1 << 15)

#define ATHR_PHY_SMARTSPEED              20
#define     PHY_SMARTSPEED_ENABLE           (1 << 5)
#define     PHY_SMARTSPEED_RETRY_MASK       (7 << 2)
#define     PHY_SMARTSPEED_BYPASS_TIMER     (1 << 1)

#define     PHY_STATUS_INTS (PHY_INT_STATUS_SPEED | PHY_INT_STATUS_LINK_UP | PHY_INT_STATUS_LINK_DOWN)

#define ATHR_PHY_DEBUG_PORT_ADDRESS      29
#define ATHR_PHY_DEBUG_PORT_DATA         30

#define ATHR_PHY_CHIP_CONFIG             31

#define     PHY_CHIP_CONFIG_BT_BX_REG_SEL   (1<<15)
#define     PHY_CHIP_CONFIG_PRIORITY_SEL    (1<<10)

#define     PHY_CHIP_CONFIG_BASET_RGMII         0x0
#define     PHY_CHIP_CONFIG_BASET_SGMII         0x1
#define     PHY_CHIP_CONFIG_FX100_RGMII_75      0xe
#define     PHY_CHIP_CONFIG_FX100_RGMII_50      0x6
#define     PHY_CHIP_CONFIG_FX100_CONV_75       0xf
#define     PHY_CHIP_CONFIG_FX100_CONV_50       0x7
#define     PHY_CHIP_CONFIG_BX1000_RGMII_75     0x3
#define     PHY_CHIP_CONFIG_BX1000_RGMII_50     0x2
#define     PHY_CHIP_CONFIG_BX1000_CONV_75      0x5
#define     PHY_CHIP_CONFIG_BX1000_CONV_50      0x4
#define     PHY_CHIP_CONFIG_RG_AUTO_MDET        0xb


/* 
 * ATHR_PHY_CONTROL fields
 * common to all phys 
 */
#define ATHR_CTRL_SOFTWARE_RESET                    0x8000
#define ATHR_CTRL_SPEED_LSB                         0x2000
#define ATHR_CTRL_AUTONEGOTIATION_ENABLE            0x1000
#define ATHR_CTRL_RESTART_AUTONEGOTIATION           0x0200
#define ATHR_CTRL_SPEED_FULL_DUPLEX                 0x0100
#define ATHR_CTRL_SPEED_MSB                         0x0040


/*
 * control status register common for
 * all phys
 */

#define PORT_STATUS_REGISTER0                0x0100 
#define PORT_STATUS_REGISTER1                0x0200
#define PORT_STATUS_REGISTER2                0x0300
#define PORT_STATUS_REGISTER3                0x0400
#define PORT_STATUS_REGISTER4                0x0500
#define PORT_STATUS_REGISTER5                0x0600

#define PORT_BASE_VLAN_REGISTER0             0x0108 
#define PORT_BASE_VLAN_REGISTER1             0x0208
#define PORT_BASE_VLAN_REGISTER2             0x0308
#define PORT_BASE_VLAN_REGISTER3             0x0408
#define PORT_BASE_VLAN_REGISTER4             0x0508
#define PORT_BASE_VLAN_REGISTER5             0x0608

#define RATE_LIMIT_REGISTER0                 0x010C
#define RATE_LIMIT_REGISTER1                 0x020C
#define RATE_LIMIT_REGISTER2                 0x030C
#define RATE_LIMIT_REGISTER3                 0x040C
#define RATE_LIMIT_REGISTER4                 0x050C
#define RATE_LIMIT_REGISTER5                 0x060C

#define PORT_CONTROL_REGISTER0               0x0104
#define PORT_CONTROL_REGISTER1               0x0204
#define PORT_CONTROL_REGISTER2               0x0304
#define PORT_CONTROL_REGISTER3               0x0404
#define PORT_CONTROL_REGISTER4               0x0504
#define PORT_CONTROL_REGISTER5               0x0604

#define RATE_LIMIT1_REGISTER0                0x011c
#define RATE_LIMIT2_REGISTER0                0x0120

#define CPU_PORT_REGISTER                    0x0078
#define MDIO_CTRL_REGISTER                   0x0098




#define ARL_TBL_FUNC_REG0                 0x0050
#define ARL_TBL_FUNC_REG1                 0x0054
#define ARL_TBL_FUNC_REG2                 0x0058
#define FLD_MASK_REG                      0x002c
#define ARL_TBL_CTRL_REG                  0x005c

/*
 *MIB register
 */

/*
 * Rx Mib counter offset 
 */
#define ATHR_PHY_RX_BROAD_REG		      0x00   /* No of good broad cast frames rcvd*/
#define ATHR_PHY_RX_PAUSE_REG		      0x04   /* No of pause frames recieved */
#define ATHR_PHY_RX_MULTI_REG                 0x08   /* No of good multi cast frames rcvd*/
#define ATHR_PHY_RX_FCSERR_REG                0x0c   /* invalid FCS and an integral no of octets*/
#define ATHR_PHY_RX_ALIGNERR_REG              0x10   /* No of frames rcvd with valid lngth and do 
							not have integral no of octets and invalid FCS*/  	 
#define ATHR_PHY_RX_RUNT_REG		      0x14   /* No of frames rcvd with less than 64B long and have good FCS*/
#define ATHR_PHY_RX_FRAGMENT_REG              0x18   /* No of frames rcvd with less than 64B long and have
                                                        bad FCS*/
#define ATHR_PHY_RX_64B_REG                   0x1c   /* No of frames rcvd that are exactly 64B long with err*/
#define ATHR_PHY_RX_128B_REG                  0x20   /* No of frames rcvd with length 65 and 127  including err*/
#define ATHR_phy_RX_256B_REG		      0x24   /* No of frames rcvd with length 128 and 255 including err */
#define ATHR_PHY_RX_512B_REG		      0x28   /* No of frames rcvd with length 255 and 512 includingg err*/
#define ATHR_PHY_RX_1024B_REG		      0x2c   /* No of frames rcvd with lengeth 512 and 1023 including errs*/
#define ATHR_PHY_RX_1518B_REG		      0x30   /* No of frames rcvd with lengeth 1024 and 1518 including err*/
#define ATHR_PHY_RX_MAXB_REG		      0x34   /* No of frames rcvd with length b/w 1519 and amx length incl of err*/
#define ATHR_PHY_RX_TOLO_REG		      0x38   /* No of frames rcvd whose length exceeds the max length with FCS err*/
#define ATHR_PHY_RX_GOODBL_REG	              0x40   
#define ATHR_PHY_RX_GOODBU_REG	              0x3c
#define ATHR_PHY_RX_BADBL_REG		      0x48   
#define ATHR_PHY_RX_BADBU_REG		      0x44   
#define ATHR_PHY_RX_OVERFLW_REG               0x4c
#define ATHR_PHY_RX_FILTERD_REG               0x50    /*port disabled and unknown VID*/


/*
 * Tx Mib counter off 
 */ 	 

#define ATHR_PHY_TX_BROAD_REG                 0x54
#define ATHR_PHY_TX_PAUSE_REG                 0x58
#define ATHR_PHY_TX_MULTI_REG                 0x5c
#define ATHR_PHY_TX_UNDERRN_REG               0x60
#define ATHR_PHY_TX_64B_REG		      0x64
#define ATHR_PHY_TX_128B_REG                  0x68
#define ATHR_PHY_TX_256B_REG                  0x6c
#define ATHR_PHY_TX_512B_REG                  0x70
#define ATHR_PHY_TX_1024B_REG                 0x74
#define ATHR_PHY_TX_1518B_REG                 0x78
#define ATHR_PHY_TX_MAXB_REG                  0x7c
#define ATHR_PHY_TX_OVSIZE_REG                0x80
#define ATHR_PHY_TX_TXBYTEL_REG               0x88
#define ATHR_PHY_TX_TXBYTEU_REG               0x84
#define ATHR_PHY_TX_COLL_REG                  0x8c
#define ATHR_PHY_TX_ABTCOLL_REG               0x90
#define ATHR_PHY_TX_MLTCOL_REG                0x94
#define ATHR_PHY_TX_SINGCOL_REG               0x98
#define ATHR_PHY_TX_EXDF_REG                  0x9c
#define ATHR_PHY_TX_DEF_REG                   0xA0
#define ATHR_PHY_TX_LATECOL_REG               0xA4


#define ATHR_PHY_FUNC_CONTROL                   16
#define ATHR_PHY_MAX				5
#define ATHR_PHY_CONTROL			0


struct rx_stats{
	int rx_broad;
	int rx_pause;
	int rx_multi;
	int rx_fcserr;
	int rx_allignerr;
	int rx_runt;
	int rx_frag;
	int rx_64b;
	int rx_128b;
	int rx_256b;
	int rx_512b;
	int rx_1024b;
	int rx_1518b;
	int rx_maxb;
	int rx_tool;
	int rx_goodbl;
	int rx_goodbh;
	int rx_overflow;
	int rx_badbl;
	int rx_badbu;
};

struct tx_stats{
	int tx_broad;
	int tx_pause;
	int tx_multi;
	int tx_underrun;
	int tx_64b;
	int tx_128b;
	int tx_256b;
	int tx_512b;
	int tx_1024b;
	int tx_1518b;
	int tx_maxb;
	int tx_oversiz;
	int tx_bytel;
	int tx_byteh;
	int tx_collision;
	int tx_abortcol;
	int tx_multicol;
	int tx_singalcol;
	int tx_execdefer;
	int tx_defer;
	int tx_latecol;
};

struct tx_mac_stats {
        
	int pkt_cntr;
	int byte_cntr;
	int mcast_cntr;
	int bcast_cntr;
	int pctrlframe_cntr;
	int deferal_cntr;
	int excess_deferal_cntr;
	int single_col_cntr;
        int multi_col_cntr;
	int late_col_cntr;
	int excess_col_cntr;
	int total_col_cntr;
	int honored_cntr;
	int dropframe_cntr;
	int jabberframe_cntr;
	int fcserr_cntr;
	int ctrlframe_cntr;
	int oz_frame_cntr;
	int us_frame_cntr;
	int frag_frame_cntr;
       
};
struct rx_mac_stats {

	int byte_cntr;
	int pkt_cntr;
	int fcserr_cntr;
	int mcast_cntr;
	int bcast_cntr;
	int ctrlframe_cntr;
	int pausefr_cntr;
	int unknownop_cntr;
	int allignerr_cntr;
	int framelerr_cntr;
	int codeerr_cntr;
	int carriersenseerr_cntr;
	int underszpkt_cntr;
	int ozpkt_cntr;
	int fragment_cntr;
	int jabber_cntr;
	int rcvdrop_cntr;
};



#endif //ATHRS_PHY_CTRL_H


