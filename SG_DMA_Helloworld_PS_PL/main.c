/******************************************************************************
* AXI DMA Scatter-Gather Example
* PS → PL only (MM2S)
* Sends "HELLO WORLD" via AXI-Stream
******************************************************************************/

#include "xaxidma.h"
#include "xparameters.h"
#include "xil_printf.h"
#include "xil_cache.h"
#include "sleep.h"
#include <string.h>

/**************************** Definitions ****************************/

#define DMA_DEV_ID        XPAR_AXIDMA_0_DEVICE_ID

#ifdef XPAR_PSU_DDR_0_S_AXI_BASEADDR
#define DDR_BASE_ADDR     XPAR_PSU_DDR_0_S_AXI_BASEADDR
#elif defined (XPAR_MIG_0_C0_DDR4_MEMORY_MAP_BASEADDR)
#define DDR_BASE_ADDR     XPAR_MIG_0_C0_DDR4_MEMORY_MAP_BASEADDR
#else
#define DDR_BASE_ADDR     0x01000000
#endif

#define MEM_BASE_ADDR     (DDR_BASE_ADDR + 0x1000000)

/* TX BD Space */
#define TX_BD_SPACE_BASE  (MEM_BASE_ADDR)
#define TX_BD_SPACE_HIGH  (MEM_BASE_ADDR + 0x00000FFF)

/* TX Buffer */
#define TX_BUFFER_BASE    (MEM_BASE_ADDR + 0x00100000)

/**************************** Globals ****************************/

XAxiDma AxiDma;
u8 *TxBufferPtr = (u8 *)TX_BUFFER_BASE;

/**************************** Prototypes ****************************/

static int TxSetup(XAxiDma *AxiDmaInstPtr);
static int SendHelloWorld(XAxiDma *AxiDmaInstPtr);

/**************************** Main ****************************/

int main(void)
{
    XAxiDma_Config *CfgPtr;
    int Status;

    xil_printf("\r\n--- AXI DMA PS -> PL HELLO WORLD ---\r\n");

    CfgPtr = XAxiDma_LookupConfig(DMA_DEV_ID);
    if (!CfgPtr) {
        xil_printf("No DMA config found\r\n");
        return XST_FAILURE;
    }

    Status = XAxiDma_CfgInitialize(&AxiDma, CfgPtr);
    if (Status != XST_SUCCESS) {
        xil_printf("DMA init failed\r\n");
        return XST_FAILURE;
    }

    if (!XAxiDma_HasSg(&AxiDma)) {
        xil_printf("DMA is not in SG mode\r\n");
        return XST_FAILURE;
    }

    Status = TxSetup(&AxiDma);
    if (Status != XST_SUCCESS) {
        xil_printf("TX setup failed\r\n");
        return XST_FAILURE;
    }

    Status = SendHelloWorld(&AxiDma);
    if (Status != XST_SUCCESS) {
        xil_printf("TX transfer failed\r\n");
        return XST_FAILURE;
    }

    xil_printf("HELLO WORLD sent successfully!\r\n");
    xil_printf("--- DONE ---\r\n");

    return XST_SUCCESS;
}

/**************************** TX Setup ****************************/

static int TxSetup(XAxiDma *AxiDmaInstPtr)
{
    XAxiDma_BdRing *TxRingPtr;
    XAxiDma_Bd BdTemplate;
    u32 BdCount;
    int Status;

    TxRingPtr = XAxiDma_GetTxRing(AxiDmaInstPtr);

    XAxiDma_BdRingIntDisable(TxRingPtr, XAXIDMA_IRQ_ALL_MASK);

    BdCount = XAxiDma_BdRingCntCalc(
        XAXIDMA_BD_MINIMUM_ALIGNMENT,
        TX_BD_SPACE_HIGH - TX_BD_SPACE_BASE + 1
    );

    Status = XAxiDma_BdRingCreate(
        TxRingPtr,
        TX_BD_SPACE_BASE,
        TX_BD_SPACE_BASE,
        XAXIDMA_BD_MINIMUM_ALIGNMENT,
        BdCount
    );
    if (Status != XST_SUCCESS) {
        xil_printf("BD ring create failed\r\n");
        return XST_FAILURE;
    }

    XAxiDma_BdClear(&BdTemplate);
    Status = XAxiDma_BdRingClone(TxRingPtr, &BdTemplate);
    if (Status != XST_SUCCESS) {
        xil_printf("BD clone failed\r\n");
        return XST_FAILURE;
    }

    Status = XAxiDma_BdRingStart(TxRingPtr);
    if (Status != XST_SUCCESS) {
        xil_printf("TX ring start failed\r\n");
        return XST_FAILURE;
    }

    return XST_SUCCESS;
}

/**************************** Send HELLO WORLD ****************************/

static int SendHelloWorld(XAxiDma *AxiDmaInstPtr)
{
    XAxiDma_BdRing *TxRingPtr;
    XAxiDma_Bd *BdPtr;
    int Status;

    const char *msg = "HELLO WORLD";
    u32 msg_len = strlen(msg) + 1;   // include '\0'

    memcpy(TxBufferPtr, msg, msg_len);

    /* Flush cache so DMA sees correct data */
    Xil_DCacheFlushRange((UINTPTR)TxBufferPtr, msg_len);

    TxRingPtr = XAxiDma_GetTxRing(AxiDmaInstPtr);

    Status = XAxiDma_BdRingAlloc(TxRingPtr, 1, &BdPtr);
    if (Status != XST_SUCCESS) {
        xil_printf("BD alloc failed\r\n");
        return XST_FAILURE;
    }

    Status = XAxiDma_BdSetBufAddr(BdPtr, (UINTPTR)TxBufferPtr);
    if (Status != XST_SUCCESS) {
        xil_printf("Set buffer addr failed\r\n");
        return XST_FAILURE;
    }

    Status = XAxiDma_BdSetLength(
        BdPtr,
        msg_len,
        TxRingPtr->MaxTransferLen
    );
    if (Status != XST_SUCCESS) {
        xil_printf("Set length failed\r\n");
        return XST_FAILURE;
    }

    XAxiDma_BdSetCtrl(
        BdPtr,
        XAXIDMA_BD_CTRL_TXSOF_MASK |
        XAXIDMA_BD_CTRL_TXEOF_MASK
    );

    XAxiDma_BdSetId(BdPtr, (UINTPTR)TxBufferPtr);

    Status = XAxiDma_BdRingToHw(TxRingPtr, 1, BdPtr);
    if (Status != XST_SUCCESS) {
        xil_printf("BD submit failed\r\n");
        return XST_FAILURE;
    }

    return XST_SUCCESS;
}

