/******************************************************************************
* AXI DMA SIMPLE POLL EXAMPLE - MODIFIED FOR CORRECT BYTE ORDER ON ILA
******************************************************************************/

#include "xaxidma.h"
#include "xparameters.h"
#include "xdebug.h"
#include "sleep.h"
#include <string.h>

#if defined(XPAR_UARTNS550_0_BASEADDR)
#include "xuartns550_l.h"
#endif

#define DMA_DEV_ID              XPAR_AXIDMA_0_DEVICE_ID

#ifdef XPAR_AXI_7SDDR_0_S_AXI_BASEADDR
#define DDR_BASE_ADDR           XPAR_AXI_7SDDR_0_S_AXI_BASEADDR
#elif defined (XPAR_MIG7SERIES_0_BASEADDR)
#define DDR_BASE_ADDR           XPAR_MIG7SERIES_0_BASEADDR
#elif defined (XPAR_MIG_0_C0_DDR4_MEMORY_MAP_BASEADDR)
#define DDR_BASE_ADDR           XPAR_MIG_0_C0_DDR4_MEMORY_MAP_BASEADDR
#elif defined (XPAR_PSU_DDR_0_S_AXI_BASEADDR)
#define DDR_BASE_ADDR           XPAR_PSU_DDR_0_S_AXI_BASEADDR
#endif

#ifndef DDR_BASE_ADDR
#define MEM_BASE_ADDR           0x01000000
#else
#define MEM_BASE_ADDR           (DDR_BASE_ADDR + 0x1000000)
#endif

#define TX_BUFFER_BASE          (MEM_BASE_ADDR + 0x00100000)
#define RX_BUFFER_BASE          (MEM_BASE_ADDR + 0x00300000)
#define RX_BUFFER_HIGH          (MEM_BASE_ADDR + 0x004FFFFF)

#define MAX_PKT_LEN             0x20
#define NUMBER_OF_TRANSFERS     10
#define POLL_TIMEOUT_COUNTER    1000000U

XAxiDma AxiDma;

int XAxiDma_SimplePollExample(u16 DeviceId);
static int CheckData(void);


int main()
{
    int Status;

    xil_printf("\r\n--- Entering DMA Test ---\r\n");

    Status = XAxiDma_SimplePollExample(DMA_DEV_ID);

    if (Status != XST_SUCCESS) {
        xil_printf("DMA Example Failed\r\n");
        return XST_FAILURE;
    }

    xil_printf("DMA Example Completed Successfully\r\n");
    return XST_SUCCESS;
}


int XAxiDma_SimplePollExample(u16 DeviceId)
{
    XAxiDma_Config *CfgPtr;
    int Status, Tries = NUMBER_OF_TRANSFERS;
    int Index, TimeOut;
    u8 *TxBufferPtr = (u8 *)TX_BUFFER_BASE;
    u8 *RxBufferPtr = (u8 *)RX_BUFFER_BASE;

    CfgPtr = XAxiDma_LookupConfig(DeviceId);
    if (!CfgPtr) {
        xil_printf("No DMA config found\r\n");
        return XST_FAILURE;
    }

    Status = XAxiDma_CfgInitialize(&AxiDma, CfgPtr);
    if (Status != XST_SUCCESS) {
        xil_printf("DMA Init Failed\r\n");
        return XST_FAILURE;
    }

    if (XAxiDma_HasSg(&AxiDma)) {
        xil_printf("DMA in Scatter-Gather mode, not supported here\n");
        return XST_FAILURE;
    }

    XAxiDma_IntrDisable(&AxiDma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DEVICE_TO_DMA);
    XAxiDma_IntrDisable(&AxiDma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DMA_TO_DEVICE);

    // ================================================================
    // *** MODIFIED: Proper byte order so ILA displays characters correctly ***
    // ================================================================
    char message[] = "HELLO WORLD";
    int msg_len = strlen(message);

    // Clear TX buffer
    for (Index = 0; Index < MAX_PKT_LEN; Index++)
        TxBufferPtr[Index] = 0;

    // Pack data so MSB shows first on ILA
    int pos = 0;
    for (Index = 0; Index < msg_len; ) {

        u8 b0 = (Index < msg_len) ? message[Index++] : 0;
        u8 b1 = (Index < msg_len) ? message[Index++] : 0;
        u8 b2 = (Index < msg_len) ? message[Index++] : 0;
        u8 b3 = (Index < msg_len) ? message[Index++] : 0;

        // Reverse order so ILA shows H→E→L→L
        TxBufferPtr[pos++] = b3;   // MSB (shows first in ILA)
        TxBufferPtr[pos++] = b2;
        TxBufferPtr[pos++] = b1;
        TxBufferPtr[pos++] = b0;   // LSB
    }
    // ================================================================

    Xil_DCacheFlushRange((UINTPTR)TxBufferPtr, MAX_PKT_LEN);
    Xil_DCacheFlushRange((UINTPTR)RxBufferPtr, MAX_PKT_LEN);

    for (Index = 0; Index < Tries; Index++) {

        TimeOut = POLL_TIMEOUT_COUNTER;

        Status = XAxiDma_SimpleTransfer(&AxiDma, (UINTPTR)RxBufferPtr,
                                        MAX_PKT_LEN, XAXIDMA_DEVICE_TO_DMA);

        Status |= XAxiDma_SimpleTransfer(&AxiDma, (UINTPTR)TxBufferPtr,
                                         MAX_PKT_LEN, XAXIDMA_DMA_TO_DEVICE);

        if (Status != XST_SUCCESS)
            return XST_FAILURE;

        while (TimeOut) {
            if (!XAxiDma_Busy(&AxiDma, XAXIDMA_DMA_TO_DEVICE) &&
                !XAxiDma_Busy(&AxiDma, XAXIDMA_DEVICE_TO_DMA))
                break;

            TimeOut--;
            usleep(1U);
        }

        if (CheckData() != XST_SUCCESS)
            return XST_FAILURE;
    }

    return XST_SUCCESS;
}


static int CheckData(void)
{
    u8 *RxPacket = (u8 *)RX_BUFFER_BASE;
    char message[] = "HELLO WORLD";
    int msg_len = strlen(message);
    int Index;

    Xil_DCacheInvalidateRange((UINTPTR)RxPacket, MAX_PKT_LEN);

    for (Index = 0; Index < msg_len; Index++) {
        if (RxPacket[Index] != message[Index]) {
            xil_printf("Mismatch at %d: got %c expected %c\r\n",
                        Index, RxPacket[Index], message[Index]);
            return XST_FAILURE;
        }
    }

    xil_printf("Received: %s\r\n", RxPacket);

    return XST_SUCCESS;

}
