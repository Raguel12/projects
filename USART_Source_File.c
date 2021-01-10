#include "USART_Header_File.h"      /* Include USART header file */

//function for USART initiaization
void USART_Init(long baud_rate)
{
    TRISC6 = 0;                     // Make Tx pin as output 
    TRISC7 = 1;                     // Make Rx pin as input 
    SPBRG = (int)BAUDRATE;          // Baud rate, SPBRG = (F_CPU /(64*baud_rate))-1 */
#ifdef HIGH_BAUD_RATE
    TXSTA = 0x24;                   // Transmit Enable(TX) with High Baud rate bit enabled 
    BAUDCON = 0x08;                 // Enable 16-bit baud generator */
#else
    TXSTA = 0x20;                   // Transmit Enable(TX) enable */ 
#endif
    RCSTA = 0x90;                   // Receive Enable(RX) enable and serial port enable */
}
// function for sending transmit data  
void USART_TxChar(char data)
{        
    while(!TXIF);                   // Wait for buffer empty */
    TXREG = data;                   // Copy data to be transmit in Tx register */
}

//send string by traversing
void USART_SendString(const char *str)
{
   while(*str!='\0')                /* Transmit data until null */
   {            
        USART_TxChar(*str);
        str++;
   }
}

// delay with for loop
void MSdelay(unsigned int val)
{
     unsigned int i,j;
        for(i = 0; i <= val; i++)
            for(j = 0; j < 165; j++);               // 1 ms delay for 8MHz Frequency 
}
