#include <string.h>

#include "gd32_eth.h"
#include "hq_generic.h"
#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/timeouts.h"
#include "netif/etharp.h"
#include "lwip/err.h"
#include "arch/ethernetif.h"

/* define those to better describe your network interface */
#define IFNAME0 'G'
#define IFNAME1 'D'

static  sys_sem_t  tx_sem;   /* semaphore used by ethernet Tx */

/* ENET RxDMA/TxDMA descriptor */
extern enet_descriptors_struct  rxdesc_tab[ENET_RXBUF_NUM], txdesc_tab[ENET_TXBUF_NUM];

/* ENET receive buffer  */
extern uint8_t rx_buff[ENET_RXBUF_NUM][ENET_RXBUF_SIZE];

/* ENET transmit buffer */
extern uint8_t tx_buff[ENET_TXBUF_NUM][ENET_TXBUF_SIZE];

/*global transmit and receive descriptors pointers */
extern enet_descriptors_struct  *dma_current_txdesc;
extern enet_descriptors_struct  *dma_current_rxdesc;

/* preserve another ENET RxDMA/TxDMA ptp descriptor for normal mode */
enet_descriptors_struct  ptp_txstructure[ENET_TXBUF_NUM];
enet_descriptors_struct  ptp_rxstructure[ENET_RXBUF_NUM];

// static struct netif *low_netif = NULL;

/**
* In this function, the hardware should be initialized.
* Called from ethernetif_init().
*
* @param netif the already initialized lwip network interface structure
*        for this ethernetif
*/
static void low_level_init(struct netif *netif)
{
  uint32_t i;
  uint8_t  *mac;

  /* set netif MAC hardware address length */
  netif->hwaddr_len = ETHARP_HWADDR_LEN;

  /* set netif MAC hardware address */
  mac = (uint8_t *)netif->state;
  mem_copy(netif->hwaddr, mac, netif->hwaddr_len);

  /* set netif maximum transfer unit */
  netif->mtu = 1500;

  /* accept broadcast address and ARP traffic */
  netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;

  // low_netif = netif;

  /* initialize MAC address in ethernet MAC */
  enet_mac_address_set(ENET_MAC_ADDRESS0, netif->hwaddr);

  /* initialize descriptors list: chain/ring mode */
#ifdef SELECT_DESCRIPTORS_ENHANCED_MODE
  enet_ptp_enhanced_descriptors_chain_init(ENET_DMA_TX);
  enet_ptp_enhanced_descriptors_chain_init(ENET_DMA_RX);
#else

  enet_descriptors_chain_init(ENET_DMA_TX);
  enet_descriptors_chain_init(ENET_DMA_RX);
//    enet_descriptors_ring_init(ENET_DMA_TX);
//    enet_descriptors_ring_init(ENET_DMA_RX);

#endif /* SELECT_DESCRIPTORS_ENHANCED_MODE */

  /* enable ethernet Rx interrrupt */
  {
    int i;
    for(i = 0; i < ENET_RXBUF_NUM; i++)
    {
      enet_rx_desc_immediate_receive_complete_interrupt(&rxdesc_tab[i]);
    }
  }

#ifdef CHECKSUM_BY_HARDWARE
  /* enable the TCP, UDP and ICMP checksum insertion for the Tx frames */
  for(i = 0; i < ENET_TXBUF_NUM; i++)
  {
    enet_transmit_checksum_config(&txdesc_tab[i], ENET_CHECKSUM_TCPUDPICMP_FULL);
  }
#endif /* CHECKSUM_BY_HARDWARE */

  /* enable MAC and DMA transmission and reception */
  enet_enable();
}

/**
* This function should do the actual transmission of the packet. The packet is
* contained in the pbuf that is passed to the function. This pbuf
* might be chained.
*
* @param netif the lwip network interface structure for this ethernetif
* @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
* @return ERR_OK if the packet could be sent
*         an err_t value if the packet couldn't be sent
*
* @note Returning ERR_MEM here if a DMA queue of your MAC is full can lead to
*       strange results. You might consider waiting for space in the DMA queue
*       to become availale since the stack doesn't retry to send a packet
*       dropped because of memory failure (except for the TCP timers).
*/

static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
  struct pbuf *q;
  uint8_t *buffer ;
  uint16_t framelength = 0;
  ErrStatus reval = ERROR;
  int ret;

  SYS_ARCH_DECL_PROTECT(sr);

  ret = sys_arch_sem_wait(&tx_sem, ETH_TX_GUARD_TIMEO);  /* protect */
  if (ret != SYS_ARCH_TIMEOUT)
  {
    SYS_ARCH_PROTECT(sr);

    while ((uint32_t)RESET != (dma_current_txdesc->status & ENET_TDES0_DAV)) ;

    buffer = (uint8_t *)(enet_desc_information_get(dma_current_txdesc, TXDESC_BUFFER_1_ADDR));
    for (q = p; q != NULL; q = q->next)
    {
      memcpy((uint8_t *)&buffer[framelength], q->payload, q->len);
      framelength = framelength + q->len;
    }

    /* transmit descriptors to give to DMA */
#ifdef SELECT_DESCRIPTORS_ENHANCED_MODE
    reval = ENET_NOCOPY_PTPFRAME_TRANSMIT_ENHANCED_MODE(framelength, NULL);
#else
    reval = ENET_NOCOPY_FRAME_TRANSMIT(framelength);
#endif /* SELECT_DESCRIPTORS_ENHANCED_MODE */

    SYS_ARCH_UNPROTECT(sr);

    sys_sem_signal(&tx_sem);
  }

  return  (reval == SUCCESS) ? ERR_OK : ERR_IF;
}

/**
* Should allocate a pbuf and transfer the bytes of the incoming
* packet from the interface into the pbuf.
*
* @param netif the lwip network interface structure for this ethernetif
* @return a pbuf filled with the received packet (including MAC header)
*         NULL on memory error
*/
static struct pbuf * low_level_input(struct netif *netif)
{
  struct pbuf *p = NULL, *q;
  uint32_t l = 0;
  u16_t len;
  uint8_t *buffer;

  /* obtain the size of the packet and put it into the "len" variable. */
  len = enet_desc_information_get(dma_current_rxdesc, RXDESC_FRAME_LENGTH);
  buffer = (uint8_t *)(enet_desc_information_get(dma_current_rxdesc, RXDESC_BUFFER_1_ADDR));

  if (len > 0)
  {
    /* We allocate a pbuf chain of pbufs from the Lwip buffer pool */
    p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
  }

  if (p != NULL)
  {
    for(q = p; q != NULL; q = q->next)
    {
      memcpy((uint8_t *)q->payload, (u8_t*)&buffer[l], q->len);
      l = l + q->len;
    }
  }

#ifdef SELECT_DESCRIPTORS_ENHANCED_MODE
  ENET_NOCOPY_PTPFRAME_RECEIVE_ENHANCED_MODE(NULL);
#else
  ENET_NOCOPY_FRAME_RECEIVE();
#endif /* SELECT_DESCRIPTORS_ENHANCED_MODE */

  return p;
}

/**
* This function is the ethernetif_input task, it is processed when a packet
* is ready to be read from the interface. It uses the function low_level_input()
* that should handle the actual reception of bytes from the network
* interface. Then the type of the received packet is determined and
* the appropriate input function is called.
*
* @param netif the lwip network interface structure for this ethernetif
*/
// void ethernetif_input( void * pvParameters )
// {
//     struct pbuf *p;
//     SYS_ARCH_DECL_PROTECT(sr);

//     for( ;; ){
//         if(pdTRUE == xSemaphoreTake(g_rx_semaphore, LOWLEVEL_INPUT_WAITING_TIME)){
// TRY_GET_NEXT_FRAME:
//             SYS_ARCH_PROTECT(sr);
//             p = low_level_input( low_netif );
//             SYS_ARCH_UNPROTECT(sr);

//             if   (p != NULL){
//                 if (ERR_OK != low_netif->input( p, low_netif)){
//                     pbuf_free(p);
//                 }else{
//                     goto TRY_GET_NEXT_FRAME;
//                 }
//             }
//         }
//     }
// }

err_t ethernetif_input(struct netif *netif)
{
  err_t  err;
  struct pbuf *p;
  SYS_ARCH_DECL_PROTECT(sr);

  while (1)
  {
    err = ERR_OK;

    SYS_ARCH_PROTECT(sr);
    p = low_level_input(netif);
    SYS_ARCH_UNPROTECT(sr);

    if (p == NULL)
    {
      break;
    }

    err = netif->input(p, netif);
    if (err != ERR_OK)
    {
      pbuf_free(p);
      break;
    }
  }

  return  err;
}


/**
* Should be called at the beginning of the program to set up the
* network interface. It calls the function low_level_init() to do the
* actual setup of the hardware.
*
* This function should be passed as a parameter to netif_add().
*
* @param netif the lwip network interface structure for this ethernetif
* @return ERR_OK if the loopif is initialized
*         ERR_MEM if private data couldn't be allocated
*         any other err_t on error
*/
err_t ethernetif_init(struct netif *netif)
{
  err_t  err;

  LWIP_ASSERT("netif != NULL", (netif != NULL));

#if LWIP_NETIF_HOSTNAME
  /* initialize interface hostname */
  netif->hostname = "lwip";
#endif /* LWIP_NETIF_HOSTNAME */

  netif->name[0] = IFNAME0;
  netif->name[1] = IFNAME1;

  netif->output = etharp_output;
  netif->linkoutput = low_level_output;

  /* initialize the hardware */
  low_level_init(netif);

  /* create Tx-protect semaphore (mutux) */
  err = sys_sem_new(&tx_sem, 1);
  if (err != ERR_OK)
  {
    printf("NET: tx_sem create failed!\r\n");
  }

  return ERR_OK;
}
