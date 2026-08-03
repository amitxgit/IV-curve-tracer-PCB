/* This is a SKELETON showing where your application code plugs into
 * CubeMX's generated main.c -- do not overwrite your generated file
 * with this; merge these calls into it instead. See firmware/README.md.
 *
 * Generated sections (USER CODE BEGIN/END blocks, peripheral init
 * calls like MX_SPI1_Init(), MX_I2C1_Init(), MX_USB_DEVICE_Init())
 * are omitted here for clarity -- they come from CubeMX and should
 * be left exactly as generated.
 */

#include "main.h"
#include "dac_driver.h"
#include "ina219_driver.h"
#include "ads1115_driver.h"

int main(void)
{
    /* --- CubeMX-generated init calls go here, unmodified --- */
    /* HAL_Init(); SystemClock_Config(); MX_GPIO_Init();
     * MX_SPI1_Init(); MX_I2C1_Init(); MX_USB_DEVICE_Init(); */

    /* --- Application init: add these three lines after the
     *     generated init calls above --- */
    DAC_Init();
    INA219_Init();
    ADS1115_Init();

    while (1)
    {
        /* Nothing to do here -- all work happens event-driven inside
         * CDC_Receive_FS() in usbd_cdc_if.c, which calls
         * SCPI_ProcessLine() once per complete command line.
         * See firmware/README.md step 5 for exactly what to add
         * there. */
    }
}
