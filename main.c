/*
    FreeRTOS V6.1.0 - Copyright (C) 2010 Real Time Engineers Ltd.

    ***************************************************************************
    *                                                                         *
    * If you are:                                                             *
    *                                                                         *
    *    + New to FreeRTOS,                                                   *
    *    + Wanting to learn FreeRTOS or multitasking in general quickly       *
    *    + Looking for basic training,                                        *
    *    + Wanting to improve your FreeRTOS skills and productivity           *
    *                                                                         *
    * then take a look at the FreeRTOS books - available as PDF or paperback  *
    *                                                                         *
    *        "Using the FreeRTOS Real Time Kernel - a Practical Guide"        *
    *                  http://www.FreeRTOS.org/Documentation                  *
    *                                                                         *
    * A pdf reference manual is also available.  Both are usually delivered   *
    * to your inbox within 20 minutes to two hours when purchased between 8am *
    * and 8pm GMT (although please allow up to 24 hours in case of            *
    * exceptional circumstances).  Thank you for your support!                *
    *                                                                         *
    ***************************************************************************

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation AND MODIFIED BY the FreeRTOS exception.
    ***NOTE*** The exception to the GPL is included to allow you to distribute
    a combined work that includes FreeRTOS without being obliged to provide the
    source code for proprietary components outside of the FreeRTOS kernel.
    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
    more details. You should have received a copy of the GNU General Public
    License and the FreeRTOS license exception along with FreeRTOS; if not it
    can be viewed here: http://www.freertos.org/a00114.html and also obtained
    by writing to Richard Barry, contact details for whom are available on the
    FreeRTOS WEB site.

    1 tab == 4 spaces!

    http://www.FreeRTOS.org - Documentation, latest information, license and
    contact details.

    http://www.SafeRTOS.com - A version that is certified for use in safety
    critical systems.

    http://www.OpenRTOS.com - Commercial support, development, porting,
    licensing and training services.
*/

/*
 * This is a very simple demo that demonstrates task and queue usages only.
 * Details of other FreeRTOS features (API functions, tracing features,
 * diagnostic hook functions, memory management, etc.) can be found on the
 * FreeRTOS web site (http://www.FreeRTOS.org) and in the FreeRTOS book.
 * Details of this demo (what it does, how it should behave, etc.) can be found
 * in the accompanying PDF application note.
 *
*/


#include <string.h>
#include <math.h>
/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Priorities at which the tasks are created. */
#define RECEIVE_TASK_PRIORITY		( tskIDLE_PRIORITY + 2 )
#define	PERIODIC_TASK_PRIORITY		( tskIDLE_PRIORITY + 1 )
#define	APERIODIC_TASK_PRIORITY		( tskIDLE_PRIORITY + 1 )
/* The bit of port 0 that the LPCXpresso LPC13xx LED is connected. */
#define mainLED_BIT 						( 22 )

/* The rate at which data is sent to the queue, specified in milliseconds. */
#define PERIODIC_SEND_FREQUENCY_MS			( 50 / portTICK_RATE_MS )

/* The number of items the queue can hold.  This is 1 as the receive task
will remove items as they are added, meaning the send task should always find
the queue empty. */
#define TEMP_QUEUE_LENGTH					( 10 )
#define USER_QUEUE_LENGTH					( 10 )
/*
 * The tasks as described in the accompanying PDF application note.
 */
static void prvReceiveTask( void *pvParameters );
static void prvTEMPSendTask( void *pvParameters );
static void prvUSERSendTask( void *pvParameters );

/*
 * Simple function to toggle the LED on the LPCXpresso LPC17xx board.
 */
static void prvToggleLED( void );
char * prvRandomString(int size);
/* The queue used by both tasks. */
static xQueueHandle xTEMPQueue = NULL;
static xQueueHandle xUSERQueue = NULL;

QueueHandle_t xActivatedQueue;
QueueHandle_t xQueueSet;
/*-----------------------------------------------------------*/

int main(void)
{
	/* Initialise P0_22 for the LED. */
	LPC_PINCON->PINSEL1	&= ( ~( 3 << 12 ) );
	LPC_GPIO0->FIODIR |= ( 1 << mainLED_BIT );

	/* Create the queue. */
	vTraceEnable(TRC_START);

	xTEMPQueue = xQueueCreate( TEMP_QUEUE_LENGTH, sizeof(char));
	xUSERQueue = xQueueCreate( USER_QUEUE_LENGTH, sizeof(char*));
	/* Init Queue Set */
	xQueueSet = xQueueCreateSet(TEMP_QUEUE_LENGTH+USER_QUEUE_LENGTH);
	configASSERT(xQueueSet);
	configASSERT(xTEMPQueue);
	configASSERT(xUSERQueue);
	xQueueAddToSet(xTEMPQueue,xQueueSet);
	xQueueAddToSet(xUSERQueue,xQueueSet);

	if( xTEMPQueue != NULL && xUSERQueue != NULL )
	{
		/* Start the two tasks as described in the accompanying application
		note. */
		xTaskCreate( prvReceiveTask,"RECIVER", configMINIMAL_STACK_SIZE, NULL, RECEIVE_TASK_PRIORITY, NULL );
		xTaskCreate( prvTEMPSendTask,"TEMP", configMINIMAL_STACK_SIZE, NULL, PERIODIC_TASK_PRIORITY, NULL );
		xTaskCreate( prvUSERSendTask,"USER", configMINIMAL_STACK_SIZE, NULL, PERIODIC_TASK_PRIORITY, NULL );
		/* Start the tasks running. */
		vTaskStartScheduler();
	}

	/* If all is well we will never reach here as the scheduler will now be
	running.  If we do reach here then it is likely that there was insufficient
	heap available for the idle task to be created. */
	for( ;; );
}





/*-----------------------------------------------------------*/
/**
 * Tarea de envio periodico representando el sensor de Temperatura (TEMP)
 * Se comunica con el proceso centra a travez de la coa xTEMPQueue
 */
static void prvTEMPSendTask( void *pvParameters )
{
	portTickType xNextWakeTime;
	char cValueToSend=0;

	/* Initialise xNextWakeTime - this only needs to be done once. */
	xNextWakeTime = xTaskGetTickCount();
	for( ;; )
	{
		/* Place this task in the blocked state until it is time to run again.
		The block state is specified in ticks, the constant used converts ticks
		to ms.  While in the blocked state this task will not consume any CPU
		time. */
		vTaskDelayUntil( &xNextWakeTime, PERIODIC_SEND_FREQUENCY_MS );

		/* Send to the queue - causing the queue receive task to flash its LED.
		0 is used as the block time so the sending operation will not block -
		it shouldn't need to block as the queue should always be empty at this
		point in the code. */
		cValueToSend = (char)(rand() % 255); //random value de temperatura
		xQueueSend( xTEMPQueue, &cValueToSend, 0 );
	}
}





/*-----------------------------------------------------------*/
/**
 * Tarea de Recepcion, recibe los datos de usuario y del sensor de Temperatura
 * y los envia por serial
 */
static void prvReceiveTask( void *pvParameters )
{
	char cReceivedValue;
	UART3_Init();
	char *buffer="Hello\r\n";
	UART_Send(buffer,strlen(buffer));
	char *TEMPbuf[10];

	for( ;; )
	{
		/* Wait until something arrives in the queue - this task will block
		indefinitely provided INCLUDE_vTaskSuspend is set to 1 in
		FreeRTOSConfig.h. */
		//xQueueReceive( xTEMPQueue, &cReceivedValue, portMAX_DELAY );
		xActivatedQueue = xQueueSelectFromSet(xQueueSet,portMAX_DELAY);

		if(xActivatedQueue == xTEMPQueue){
			xQueueReceive( xTEMPQueue, &cReceivedValue, 200 );
			itoa(cReceivedValue,TEMPbuf,10);
			strcat(TEMPbuf,"\r\n");
			UART_Send(TEMPbuf,strlen(TEMPbuf));
		}
		else if(xActivatedQueue == xUSERQueue){
			xQueueReceive( xUSERQueue, &buffer,200 );
			UART_Send(buffer,strlen(buffer));
			vPortFree(buffer);
		}

		prvToggleLED();

	}
}






/*-----------------------------------------------------------*/
/**
 * Funcion representado a un usuario. Aperiodica
 */
static void prvUSERSendTask( void *pvParameters ){

		portTickType xNextWakeTime;
		char *Buffer = "HOLA MUNDIRIJILLO\r\n";

		/* Initialise xNextWakeTime - this only needs to be done once. */
		xNextWakeTime = xTaskGetTickCount();
		for( ;; )
		{
			/* Place this task in the blocked state until it is time to run again.
			The block state is specified in ticks, the constant used converts ticks
			to ms.  While in the blocked state this task will not consume any CPU
			time. */
			vTaskDelayUntil( &xNextWakeTime, rand()%200/portTICK_PERIOD_MS );

			/* Send to the queue - causing the queue receive task to flash its LED.
			0 is used as the block time so the sending operation will not block -
			it shouldn't need to block as the queue should always be empty at this
			point in the code. */
			Buffer =prvRandomString(rand()%30);
			xQueueSend( xUSERQueue, &Buffer, 0 );
		}

}


char * prvRandomString(int size){
	static char* leters="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvqxyz";
	size+=3;
	char*string = pvPortMalloc(size);
	for(int i=0;i<size-3;i++){
		string[i]=leters[rand()%52];
	}
	string[size-3]='\r';
	string[size-2]='\n';
	string[size-1]='\0';

	return string;
}




















static void prvToggleLED( void )
{
unsigned long ulLEDState;

	/* Obtain the current P0 state. */
	ulLEDState = LPC_GPIO0->FIOPIN;

	/* Turn the LED off if it was on, and on if it was off. */
	LPC_GPIO0->FIOCLR = ulLEDState & ( 1 << mainLED_BIT );
	LPC_GPIO0->FIOSET = ( ( ~ulLEDState ) & ( 1 << mainLED_BIT ) );
}
/*-----------------------------------------------------------*/

/**** FUNCIONES NECESARIAS PARA COMPILaR FreeRTOS  ***/
void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
	/* This function will get called if a task overflows its stack. */

	( void ) pxTask;
	( void ) pcTaskName;

	for( ;; );
}
/*-----------------------------------------------------------*/

void vConfigureTimerForRunTimeStats( void )
{
const unsigned long TCR_COUNT_RESET = 2, CTCR_CTM_TIMER = 0x00, TCR_COUNT_ENABLE = 0x01;

	/* This function configures a timer that is used as the time base when
	collecting run time statistical information - basically the percentage
	of CPU time that each task is utilising.  It is called automatically when
	the scheduler is started (assuming configGENERATE_RUN_TIME_STATS is set
	to 1). */

	/* Power up and feed the timer. */
	LPC_SC->PCONP |= 0x02UL;
	LPC_SC->PCLKSEL0 = (LPC_SC->PCLKSEL0 & (~(0x3<<2))) | (0x01 << 2);

	/* Reset Timer 0 */
	LPC_TIM0->TCR = TCR_COUNT_RESET;

	/* Just count up. */
	LPC_TIM0->CTCR = CTCR_CTM_TIMER;

	/* Prescale to a frequency that is good enough to get a decent resolution,
	but not too fast so as to overflow all the time. */
	LPC_TIM0->PR =  ( configCPU_CLOCK_HZ / 10000UL ) - 1UL;

	/* Start the counter. */
	LPC_TIM0->TCR = TCR_COUNT_ENABLE;
}
/*-----------------------------------------------------------*/

