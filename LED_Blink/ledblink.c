#include "xparameters.h"
#include "xgpio.h"
#include "xil_printf.h"
#include "sleep.h"

/* AXI GPIO device ID (from xparameters.h) */
#define LED_GPIO_DEVICE_ID  XPAR_AXI_GPIO_0_DEVICE_ID

/* Channel connected to LEDs */
#define LED_CHANNEL 1

int main()
{
    XGpio Gpio;       // GPIO instance
    int status;
    u32 led_value = 0x0;

    xil_printf("LED Blink Application Started\r\n");

    /* Initialize GPIO */
    status = XGpio_Initialize(&Gpio, LED_GPIO_DEVICE_ID);
    if (status != XST_SUCCESS) {
        xil_printf("GPIO Initialization Failed\r\n");
        return XST_FAILURE;
    }

    /* Set GPIO direction: 0 = output */
    XGpio_SetDataDirection(&Gpio, LED_CHANNEL, 0x0);

    while (1)
    {
        /* Turn ON LEDs */
        led_value = 0xFF;   // All LEDs ON
        XGpio_DiscreteWrite(&Gpio, LED_CHANNEL, led_value);
        sleep(1);

        /* Turn OFF LEDs */
        led_value = 0x00;   // All LEDs OFF
        XGpio_DiscreteWrite(&Gpio, LED_CHANNEL, led_value);
        sleep(1);
    }

    return 0;
}

