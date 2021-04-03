/*------------------------------------------------------------------------------------------*\
 *   Copyright (C) 2011,2012 AVM GmbH <fritzbox_info@avm.de>
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


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static int check_dma_status_pause(athr_gmac_t *mac) {

    int RxFsm,TxFsm,RxFD,RxCtrl;

    /*------------------------------------------------------------------------------------------*\
     * If DMA is in pause state update the watchdog timer to avoid MAC reset.
    \*------------------------------------------------------------------------------------------*/
    RxFsm = athr_gmac_reg_rd(mac,ATHR_GMAC_DMA_RXFSM);
    TxFsm = athr_gmac_reg_rd(mac,ATHR_GMAC_DMA_TXFSM);
    RxFD  = athr_gmac_reg_rd(mac,ATHR_GMAC_DMA_XFIFO_DEPTH);
    RxCtrl = athr_gmac_reg_rd(mac,ATHR_GMAC_DMA_RX_CTRL);


    if (((RxFsm & ATHR_GMAC_DMA_DMA_STATE) == 0x3)
        && (((RxFsm >> 4) & ATHR_GMAC_DMA_AHB_STATE) == 0x1)
        && (RxCtrl == 0x1) && (((RxFsm >> 11) & 0x1ff) == 0x1ff))  {
        DPRINTF("mac:%d RxFsm:%x TxFsm:%x\n",mac->mac_unit,RxFsm,TxFsm);
        return 0;
    }
    else if (((((TxFsm >> 4) & ATHR_GMAC_DMA_AHB_STATE) <= 0x4) && 
            ((RxFsm & ATHR_GMAC_DMA_DMA_STATE) == 0x0) && 
            (((RxFsm >> 4) & ATHR_GMAC_DMA_AHB_STATE) == 0x0)) || 
            (((RxFD >> 16) <= 0x20) && (RxCtrl == 1)) ) {
        return 1;
    }
    else {
        DPRINTF(" FIFO DEPTH = %x",RxFD);
        DPRINTF(" RX DMA CTRL = %x",RxCtrl);
        DPRINTF("mac:%d RxFsm:%x TxFsm:%x\n",mac->mac_unit,RxFsm,TxFsm);
        return 2;
    }
}


/*------------------------------------------------------------------------------------------*\
\*------------------------------------------------------------------------------------------*/
static int 
check_for_dma_status(void *arg,int ac) {

    athr_gmac_t *mac         = (athr_gmac_t *)arg;
    athr_gmac_ring_t   *r    = &mac->mac_txring[ac];
    int                head  = r->ring_head, tail = r->ring_tail;
    athr_gmac_desc_t   *ds;
    athr_gmac_buffer_t *bp;

    /*------------------------------------------------------------------------------------------*\
     * If Tx hang is asserted reset the MAC and restore the descriptors and interrupt state.
    \*------------------------------------------------------------------------------------------*/
    while (tail != head) {  // Check for TX DMA.
        ds   = &r->ring_desc[tail];
        bp   =  &r->ring_buffer[tail];

        if(athr_gmac_tx_owned_by_dma(ds)) {
            if ((athr_gmac_get_diff(bp->trans_start,jiffies)) > ((1 * HZ/10))) {

                /*--- If the DMA is in pause state reset kernel watchdog timer ---*/
                if(check_dma_status_pause(mac)) { 
                    mac->mac_dev->trans_start = jiffies;
                    return 0;
                }
 
                printk(MODULE_NAME ": Tx Dma status eth%d : %s\n",mac->mac_unit,
                            athr_gmac_tx_stopped(mac) ? "inactive" : "active");
               
                athr_gmac_fast_reset(mac,ds,ac);

                break;
            }
        }
        athr_gmac_ring_incr(tail);
    } 

    if (check_dma_status_pause(mac) == 0) { //Check for RX DMA
        if (mac->rx_dma_check) { // see if we holding the rx for 100ms
            uint32_t RxFsm;

            if (check_dma_status_pause(mac) == 0) {
                RxFsm = athr_gmac_reg_rd(mac,ATHR_GMAC_DMA_RXFSM);
                printk(MODULE_NAME ": Rx Dma status eth%d : %X\n",mac->mac_unit,RxFsm);
                athr_gmac_fast_reset(mac,NULL,ac);
            }
            mac->rx_dma_check = 0;
        } else {
            mac->rx_dma_check = 1;
        }
    }

    return 0;
}

