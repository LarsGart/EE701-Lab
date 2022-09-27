/*
 * Copyright (c) 2015-2016, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ======== empty.c ========
 */
/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>

/* TI-RTOS Header files */
// #include <ti/drivers/I2C.h>
#include <ti/drivers/PIN.h>
// #include <ti/drivers/SPI.h>
// #include <ti/drivers/UART.h>
// #include <ti/drivers/Watchdog.h>

/* TI - RTOS Header files */
#include <ti/drivers/PIN.h>
#include <ti/drivers/I2C.h>

/* Example / Board Header files */
#include "Board.h"
#define TASKSTACKSIZE 1024
#define Board_initI2C() I2C_init();

/* Global memory storage for a PIN_Config table */
Task_Struct task0Struct;
Char task0Stack[TASKSTACKSIZE];

/* Pin driver handle */
static PIN_Handle ledPinHandle;
static PIN_State ledPinState;

/*
 * Application LED pin configuration table:
 *   - All LEDs board LEDs are off.
 */
PIN_Config ledPinTable[] = {
    Board_LED0 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    Board_LED1 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    PIN_TERMINATE
};



Void taskFxn ( UArg arg0 , UArg arg1 )
{
    unsigned int i;
    unsigned int tempc, tempf;
    uint8_t txBuffer[1];
    uint8_t rxBuffer[2];
    I2C_Handle i2c;
    I2C_Params i2cParams;
    I2C_Transaction i2cTransaction;
    /* Create I2C for usage */
    I2C_Params_init(&i2cParams);
    i2cParams.bitRate = I2C_400kHz;
    i2c = I2C_open( Board_I2C , &i2cParams);
    if (i2c == NULL)
    {
        System_abort("Error Initializing I2C\n");
    }
    else
    {
        System_printf("I2C Initialized!\n");
    }
    txBuffer[0] = 0xE3;
    i2cTransaction.slaveAddress = 0x40;
    i2cTransaction.writeBuf = txBuffer;
    i2cTransaction.writeCount = 1;
    i2cTransaction.readBuf = rxBuffer;
    i2cTransaction.readCount = 2;
    /* Take 20 samples and print them out onto the console */
    for (i = 0; i < 20; i++)
    {
        if (I2C_transfer(i2c, &i2cTransaction ))
        {
            tempc = (rxBuffer[0] << 8) | rxBuffer[1];
            tempc = (175.72 * tempc) / 65536 - 46.85;
            tempf = (9/5 * tempc) + 32;
            System_printf("Sample %u : %d (C), %d (F)\n", i , tempc, tempf);
        }
        else
        {
            System_printf("I2C Bus fault \n");
        }
        System_flush();
        if (tempc > 26)
        {
            PIN_setOutputValue(ledPinHandle, Board_LED1, 1);
        }
        else
        {
            PIN_setOutputValue(ledPinHandle, Board_LED1, 0);
        }
        Task_sleep(1000000 / Clock_tickPeriod );
    }
    /* De-initialized I2C */
    I2C_close( i2c );
    System_printf("I2C closed !\n");
    System_flush();
}

/*
* ======== main ========
*/
int main ( void )
{
    Task_Params taskParams;

    /* Call board init functions */
    Board_initGeneral();
    Board_initI2C();

    /* Open LED pins */
    ledPinHandle = PIN_open(&ledPinState, ledPinTable);
    if (! ledPinHandle )
    {
        System_abort(" Error initializing board LED pins \n ");
    }
    PIN_setOutputValue( ledPinHandle , Board_LED1 , 0);


    /* Construct Task thread */
    Task_Params_init(&taskParams);
    taskParams.stackSize = TASKSTACKSIZE;
    taskParams.stack = &task0Stack;
    Task_construct(&task0Struct, (Task_FuncPtr)taskFxn, &taskParams, NULL);

    System_printf(" Starting the I2C example \nSystem provider is set to SysMin ."
        " Halt the target to view any SysMin contents in ROV .\n ");

    /* SysMin will only print to the console when you call flush or exit */
    System_flush();
    /* Start BIOS */
    BIOS_start();
    return(0);
}
