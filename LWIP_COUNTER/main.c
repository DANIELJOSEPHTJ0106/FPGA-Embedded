#include <stdio.h>
#include <string.h>
#include "xparameters.h"
#include "xaxidma.h"
#include "xil_cache.h"
#include "sleep.h"

#include "lwip/init.h"
#include "lwip/tcp.h"
#include "netif/xadapter.h"
#include "platform.h"
#include "platform_config.h"

/* lwIP timers */
extern volatile int TcpFastTmrFlag;
extern volatile int TcpSlowTmrFlag;

/* Network interface */
struct netif server_netif;
struct netif *echo_netif;

/* GLOBAL PCB to track connection */
struct tcp_pcb *ConnectedPCB = NULL;

#define DMA_DEV_ID XPAR_AXIDMA_0_DEVICE_ID
#define COUNTER_SAMPLES 1024
#define RX_BYTES (COUNTER_SAMPLES * sizeof(u32))

// Using Custom Address (Optional: You can switch back to array if needed)
#define MEM_BASE_ADDR 0x10000000
u32 *RxBuffer = (u32 *)MEM_BASE_ADDR;

XAxiDma AxiDma;

int InitializeDMA()
{
    XAxiDma_Config *CfgPtr;
    int Status;

    CfgPtr = XAxiDma_LookupConfig(DMA_DEV_ID);
    if (!CfgPtr) return XST_FAILURE;

    Status = XAxiDma_CfgInitialize(&AxiDma, CfgPtr);
    if (Status != XST_SUCCESS) return XST_FAILURE;

    if (XAxiDma_HasSg(&AxiDma)) return XST_FAILURE;

    XAxiDma_IntrDisable(&AxiDma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DEVICE_TO_DMA);

    return XST_SUCCESS;
}

int RunDMARxTransfer(struct tcp_pcb *tpcb)
{
    int i;
    char msg[32];
    err_t err;

    // 1. Invalidate Cache
    Xil_DCacheInvalidateRange((UINTPTR)RxBuffer, RX_BYTES);

    // 2. Start DMA Transfer
    if (XAxiDma_SimpleTransfer(&AxiDma,
                              (UINTPTR)RxBuffer,
                              RX_BYTES,
                              XAXIDMA_DEVICE_TO_DMA) != XST_SUCCESS)
        return XST_FAILURE;

    // 3. Wait for DMA
    while (XAxiDma_Busy(&AxiDma, XAXIDMA_DEVICE_TO_DMA));

    // 4. Invalidate Cache again
    Xil_DCacheInvalidateRange((UINTPTR)RxBuffer, RX_BYTES);

    // 5. Send Data
    for (i = 0; i < COUNTER_SAMPLES; i++) {

        snprintf(msg, sizeof(msg), "%lu\r\n", RxBuffer[i]);

        // Try to send
        err = tcp_write(tpcb, msg, strlen(msg), TCP_WRITE_FLAG_COPY);

        // If buffer is full (ERR_MEM)
        if (err == ERR_MEM) {
            tcp_output(tpcb); // Force send whatever is in buffer

            // --- CRITICAL FIX START ---
            // We MUST process incoming packets (ACKs) to free up buffer space!
            xemacif_input(echo_netif);

            // Handle timers just in case
            if (TcpFastTmrFlag) { tcp_fasttmr(); TcpFastTmrFlag = 0; }
            if (TcpSlowTmrFlag) { tcp_slowtmr(); TcpSlowTmrFlag = 0; }
            // --- CRITICAL FIX END ---

            i--; // Retry this number again
            continue;
        }
        // If connection is lost
        else if (err != ERR_OK) {
            ConnectedPCB = NULL;
            return XST_FAILURE;
        }

        // Force send periodically
        if ((i % 50) == 0) {
            tcp_output(tpcb);
            // Good practice: Check input periodically too
            xemacif_input(echo_netif);
        }
    }

    tcp_output(tpcb);
    return XST_SUCCESS;
}

// Called when errors occur (like abrupt disconnect)
void err_callback(void *arg, err_t err) {
    xil_printf("Connection Error/Closed\r\n");
    ConnectedPCB = NULL; // Stop streaming
}

// Called when a new client connects
err_t accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    xil_printf("Client Connected! Starting Stream...\r\n");

    // Set global variable to start loop in main()
    ConnectedPCB = newpcb;

    // Register error callback to handle disconnects
    tcp_err(newpcb, err_callback);

    return ERR_OK;
}

int start_application()
{
    struct tcp_pcb *pcb;
    err_t err;

    pcb = tcp_new();
    if (!pcb) return -1;

    err = tcp_bind(pcb, IP_ADDR_ANY, 7);
    if (err != ERR_OK) return -1;

    pcb = tcp_listen(pcb);
    tcp_accept(pcb, accept_callback);

    return 0;
}

int main()
{
    ip_addr_t ipaddr, netmask, gw;
    unsigned char mac[] = {0x00,0x0A,0x35,0x00,0x01,0x02};

    echo_netif = &server_netif;
    init_platform();
    InitializeDMA();

    IP4_ADDR(&ipaddr,  192,168,1,10);
    IP4_ADDR(&netmask, 255,255,255,0);
    IP4_ADDR(&gw,      192,168,1,1);

    lwip_init();
    xemac_add(echo_netif, &ipaddr, &netmask, &gw, mac, XPAR_XEMACPS_0_BASEADDR);
    netif_set_default(echo_netif);
    netif_set_up(echo_netif);
    platform_enable_interrupts();

    start_application();

    xil_printf("System Ready. Waiting for connection...\r\n");

    while (1) {
        // Standard lwIP processing
        if (TcpFastTmrFlag) { tcp_fasttmr(); TcpFastTmrFlag = 0; }
        if (TcpSlowTmrFlag) { tcp_slowtmr(); TcpSlowTmrFlag = 0; }
        xemacif_input(echo_netif);

        // CONTINUOUS STREAMING LOGIC
        // If a client is connected, keep sending data
        if (ConnectedPCB != NULL) {
            RunDMARxTransfer(ConnectedPCB);
        }
    }

    cleanup_platform();
    return 0;
}

