#ifndef USART_HEADER_FILE_H
#define	USART_HEADER_FILE_H

#include <pic18f4550.h>             /* Include PIC18F4550 header file */

#define HIGH_BAUD_RATE

#ifdef HIGH_BAUD_RATE
    #define F_CPU 8000000/4            //Define ferquency 
#else
    #define F_CPU 8000000/64            //Define ferquency 
#endif

#define BAUDRATE (((float)(F_CPU)/(float)baud_rate)-1)      // Define Baud value for 16 bit generator with asynchronous and high baudrate mode endabled 

void USART_Init(long);                  // USART Initialization function 
void USART_TxChar(char);                // USART character transmit function 
char USART_RxChar();                    // USART character receive function 
void USART_SendString(const char *);    // USART String transmit function
void MSdelay(unsigned int val);         // millisecond Delay function 
void ADC_Init();                        // analog to digital converter intializer
int ADC_Read();                      // analog read function 

#endif	

