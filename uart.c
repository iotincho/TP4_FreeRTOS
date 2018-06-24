/*
 * uart.c
 *
 *  Created on: 24 de jun. de 2018
 *      Author: Tincho
 */
#include "LPC17xx.h"
void UART3_Init()
{
/////////////////////configuracion UART //////////////////////////////////////////

		LPC_SC->PCONP |= (1<<25); //*PCONP PCUART0 UART3 power/clock control bit. and desabited UART1
		LPC_SC->PCONP &= ~(3<<3);// *PCONP desabilito uart 0,1
		LPC_SC->PCLKSEL1 |= (1<<18); //*PCLKSEL1

		LPC_UART3->LCR = 0x03; // *U3LCR palabra 8 bits
		LPC_UART3->LCR |= (1<<2); // *U3LCR bit de parada
		LPC_UART3->LCR |= 0b10000000; //*U3LCR habilito el latch para configurar
		LPC_UART3->DLL = 54; //*U3LCR 0b10100001 ; // 115200
		LPC_UART3->DLM =0; //*U3LCR
		LPC_UART3->LCR &=~(1<<7);//*U3LCR desabilito el latch

		//pin 0 TXD0 pin 1 RXD0 puerto 0
		LPC_PINCON->PINSEL0 =0b1010; // *PINSEL0 configurar los pines port 0
		LPC_PINCON->PINMODE0 = 0; // *PINMODE0 pin a pull up

	/*LPC_UART3->IER = 1; // *U3IER habilito la interrupcion por Recive Data Aviable

	    *ISER0 |= 1<<8; //activate interrup uart3
	*/
}

void UART_Send(char* datos , int size){

	for(int i =0 ; i < size ; i++){
		while((LPC_UART3->LSR & (1<<5))==0){}//*U3LSR // Wait for Previous transmission
		LPC_UART3->THR = datos[i]; //;*U3THR
	}
}
