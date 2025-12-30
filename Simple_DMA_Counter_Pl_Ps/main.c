//working code of the simple dma +counter-----

#include "xaxidma.h"
#include "xparameters.h"
#include "xil_cache.h"
#include "xdebug.h"
#include "sleep.h"

#define DMA_DEV_ID          XPAR_AXIDMA_0_DEVICE_ID
#define DDR_BASE_ADDR       XPAR_PS7_DDR_0_S_AXI_BASEADDR
#define RX_BUFFER_BASE      (DDR_BASE_ADDR + 0x01300000)
#define MAX_PKT_LEN         4096 // 1024 samples * 4 bytes

XAxiDma AxiDma;

int main() {
    XAxiDma_Config *CfgPtr;
    int Status;
    u32 *RxBufferPtr = (u32 *)RX_BUFFER_BASE;

    xil_printf("\r\n--- DMA Sync Fix: Capturing 0-1023 ---\r\n");

    /* 1. Initialize DMA */
    CfgPtr = XAxiDma_LookupConfig(DMA_DEV_ID);
    if (!CfgPtr) return XST_FAILURE;

    Status = XAxiDma_CfgInitialize(&AxiDma, CfgPtr);
    if (Status != XST_SUCCESS) return XST_FAILURE;

    XAxiDma_IntrDisable(&AxiDma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DEVICE_TO_DMA);



    for(int i = 0; i < 1024; i++) RxBufferPtr[i] = 0x0;
    Xil_DCacheFlushRange((UINTPTR)RxBufferPtr, MAX_PKT_LEN);


    Status = XAxiDma_SimpleTransfer(&AxiDma, (UINTPTR)RxBufferPtr,
                                    MAX_PKT_LEN, XAXIDMA_DEVICE_TO_DMA);
    while (XAxiDma_Busy(&AxiDma, XAXIDMA_DEVICE_TO_DMA));


    Status = XAxiDma_SimpleTransfer(&AxiDma, (UINTPTR)RxBufferPtr,
                                    MAX_PKT_LEN, XAXIDMA_DEVICE_TO_DMA);


    while (XAxiDma_Busy(&AxiDma, XAXIDMA_DEVICE_TO_DMA));


    Xil_DCacheInvalidateRange((UINTPTR)RxBufferPtr, MAX_PKT_LEN);

    xil_printf("Data Captured:\r\n");
    for(int i = 0; i < 1024; i++) {
        xil_printf("%u\t", RxBufferPtr[i]);
        if ((i + 1) % 8 == 0) xil_printf("\r\n");
    }

    return 0;
}

