/*
===============================================================================
 Name        : SOTp3_Embds.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
*/

#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif

#include <cr_section_macros.h>
#include <stdlib.h>
/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Priorities at which the tasks are created. */
#define mainQUEUE_RECEIVE_TASK_PRIORITY		( tskIDLE_PRIORITY + 2 )
#define	mainQUEUE_SEND_TASK_PRIORITY		( tskIDLE_PRIORITY + 1 )

/* The bit of port 0 that the LPCXpresso LPC13xx LED is connected. */
#define mainLED_BIT 						( 22 )

/* The rate at which data is sent to the queue, specified in milliseconds. */
#define mainQUEUE_SEND_FREQUENCY_MS			( 10 / portTICK_RATE_MS )

/* The number of items the queue can hold.  This is 1 as the receive task
will remove items as they are added, meaning the send task should always find
the queue empty. */
#define mainQUEUE_LENGTH					( 1 )

/*
 * The tasks as described in the accompanying PDF application note.
 */
static void prvQueueReceiveTask( void *pvParameters );
static void prvQueueSendTask( void *pvParameters );
void vConfigureTimerForRunTimeStats( void );

/*
 * Simple function to toggle the LED on the LPCXpresso LPC17xx board.
 */
static void prvToggleLED( void );

/* The queue used by both tasks. */
static xQueueHandle xQueue = NULL;

/*-----------------------------------------------------------*/

int main(void)
{
	/* Initialise P0_22 for the LED. */
	LPC_PINCON->PINSEL1	&= ( ~( 3 << 12 ) );
	LPC_GPIO0->FIODIR |= ( 1 << mainLED_BIT );

	/* Init and start tracing */
	vTraceEnable(TRC_START);

	/* Create the queue. */
	xQueue = xQueueCreate( mainQUEUE_LENGTH, sizeof(char)*4 );

	if( xQueue != NULL )
	{
		/* Start the two tasks as described in the accompanying application
		note. */
		xTaskCreate( prvQueueReceiveTask, ( signed char * ) "Rx", configMINIMAL_STACK_SIZE, NULL, mainQUEUE_RECEIVE_TASK_PRIORITY, NULL );
		xTaskCreate( prvQueueSendTask, ( signed char * ) "TX", configMINIMAL_STACK_SIZE, NULL, mainQUEUE_SEND_TASK_PRIORITY, NULL );

		/* Start the tasks running. */
		vTaskStartScheduler();
	}

	/* If all is well we will never reach here as the scheduler will now be
	running.  If we do reach here then it is likely that there was insufficient
	heap available for the idle task to be created. */
	for( ;; );
}
/*-----------------------------------------------------------*/

static void prvQueueSendTask( void *pvParameters )
{
	portTickType xNextWakeTime;
	//const unsigned long ulValueToSend = 100UL;
	char* log;
	/* Initialise xNextWakeTime - this only needs to be done once. */
	xNextWakeTime = xTaskGetTickCount();

	/* agregados */
	traceString cht = xTraceRegisterString("MyChannelTx");

	for( ;; )
	{
		/* Place this task in the blocked state until it is time to run again.
		The block state is specified in ticks, the constant used converts ticks
		to ms.  While in the blocked state this task will not consume any CPU
		time. */

		vTracePrint(cht, "soy Tx!");
		vTaskDelayUntil( &xNextWakeTime, mainQUEUE_SEND_FREQUENCY_MS );

		/* Send to the queue - causing the queue receive task to flash its LED.
		0 is used as the block time so the sending operation will not block -
		it shouldn't need to block as the queue should always be empty at this
		point in the code. */
		log = pvPortMalloc(sizeof(char)*(rand()%8));
		xQueueSend( xQueue, &log, 0 );
	}
}
/*-----------------------------------------------------------*/

static void prvQueueReceiveTask( void *pvParameters )
{
	//unsigned long ulReceivedValue;
	char * log;
	traceString chr = xTraceRegisterString("MyChannelRx");

	for( ;; )
	{

		vTracePrint(chr, "soy Rx!");

		/* Wait until something arrives in the queue - this task will block
		indefinitely provided INCLUDE_vTaskSuspend is set to 1 in
		FreeRTOSConfig.h. */
		xQueueReceive( xQueue, &log, portMAX_DELAY );

		/*  To get here something must have been received from the queue, but
		is it the expected value?  If it is, toggle the LED. */
		vPortFree(log);
		prvToggleLED();
	}
}
/*-----------------------------------------------------------*/

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


