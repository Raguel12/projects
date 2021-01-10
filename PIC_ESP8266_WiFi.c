#include <xc.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pic18f4550.h>
#include "USART_Header_File.h"
#include "Configuration_header_file.h"

#define DEFAULT_BUFFER_SIZE		160
#define DEFAULT_TIMEOUT			10000

/* Connection Mode */
#define SINGLE				0
#define MULTIPLE			1

/* Application Mode */
#define NORMAL				0
#define TRANSPERANT			1

/* Application Mode */
#define STATION				1
#define ACCESSPOINT			2
#define BOTH_STATION_AND_ACCESPOINT	3

#define SEND_DEMO			/* Define SEND demo */

/* Define Required fields shown below */
#define DOMAIN				"api.thingspeak.com"
#define PORT				"80"
#define API_WRITE_KEY			"S6QH2RIGY8YP0J0Q"


#define SSID				"itel"
#define PASSWORD			"12345678"


//DHT 11 defines
#define Data_Out LATAbits.LATA0       // assign Port pin for data
#define Data_In PORTAbits.RA0  //read data from Port pin
#define Data_Dir TRISAbits.TRISA0 // Port direction 

#define _XTAL_FREQ 8000000     // define _XTAL_FREQ for using internal delay s



enum ESP8266_RESPONSE_STATUS{
	ESP8266_RESPONSE_WAITING,
	ESP8266_RESPONSE_FINISHED,
	ESP8266_RESPONSE_TIMEOUT,
	ESP8266_RESPONSE_BUFFER_FULL,
	ESP8266_RESPONSE_STARTING,
	ESP8266_RESPONSE_ERROR
};

enum ESP8266_CONNECT_STATUS {
	ESP8266_CONNECTED_TO_AP,
	ESP8266_CREATED_TRANSMISSION,
	ESP8266_TRANSMISSION_DISCONNECTED,
	ESP8266_NOT_CONNECTED_TO_AP,
	ESP8266_CONNECT_UNKNOWN_ERROR
};

enum ESP8266_JOINAP_STATUS {
	ESP8266_WIFI_CONNECTED,
	ESP8266_CONNECTION_TIMEOUT,
	ESP8266_WRONG_PASSWORD,
	ESP8266_NOT_FOUND_TARGET_AP,
	ESP8266_CONNECTION_FAILED,
	ESP8266_JOIN_UNKNOWN_ERROR
};
char DHT11_ReadData()
{
  char i,data = 0;  
    for(i=0;i<8;i++)
    {
        while(!(Data_In & 1));  /* wait till 0 pulse, this is start of data pulse */
        __delay_us(30);         
        if(Data_In & 1)  /* check whether data is 1 or 0 */    
          data = ((data<<1) | 1); 
        else
          data = (data<<1);  
        while(Data_In & 1);
    }
  return data;
}

void DHT11_Start()
{    
    Data_Dir = 0;  /* set as output port */
    Data_Out = 0;  /* send low pulse of min. 18 ms width */
    __delay_ms(18);
    Data_Out = 1;  /* pull data bus high */
    __delay_us(20);
    Data_Dir = 1;  /* set as input port */    
}

void DHT11_CheckResponse()
{
    while(Data_In & 1);  /* wait till bus is High */     
    while(!(Data_In & 1));  /* wait till bus is Low */
    while(Data_In & 1);  /* wait till bus is High */
}

int8_t Response_Status,temprature;
volatile int16_t Counter = 0, pointer = 0;
uint32_t TimeOut = 0;
char RESPONSE_BUFFER[DEFAULT_BUFFER_SIZE];

void Read_Response( const char* _Expected_Response)
{
  uint8_t EXPECTED_RESPONSE_LENGTH = strlen(_Expected_Response);
  uint32_t TimeCount = 0, ResponseBufferLength;
  char RECEIVED_CRLF_BUF[30];

  while(1)
  {
    if(TimeCount >= (DEFAULT_TIMEOUT+TimeOut))
    {
	TimeOut = 0;
	Response_Status = ESP8266_RESPONSE_TIMEOUT;
	return;
    }

    if(Response_Status == ESP8266_RESPONSE_STARTING)
    {
	Response_Status = ESP8266_RESPONSE_WAITING;
    }

    ResponseBufferLength = strlen(RESPONSE_BUFFER);
    if (ResponseBufferLength)
    {
	MSdelay(1);
	TimeCount++;
	if (ResponseBufferLength==strlen(RESPONSE_BUFFER))
	{
	    for (uint16_t i=0;i<ResponseBufferLength;i++)
	    {
		memmove(RECEIVED_CRLF_BUF, RECEIVED_CRLF_BUF + 1, EXPECTED_RESPONSE_LENGTH-1);
		RECEIVED_CRLF_BUF[EXPECTED_RESPONSE_LENGTH-1] = RESPONSE_BUFFER[i];
		if(!strncmp(RECEIVED_CRLF_BUF, _Expected_Response, EXPECTED_RESPONSE_LENGTH))
		{
			TimeOut = 0;
			Response_Status = ESP8266_RESPONSE_FINISHED;
			return;
		}
	    }
	}
    }
    MSdelay(1);
    TimeCount++;
  }
}

void ESP8266_Clear()
{
	memset(RESPONSE_BUFFER,0,DEFAULT_BUFFER_SIZE);
	Counter = 0;	pointer = 0;
}

void Start_Read_Response(const char* _ExpectedResponse)
{
	Response_Status = ESP8266_RESPONSE_STARTING;
	do {
		Read_Response(_ExpectedResponse);
	} while(Response_Status == ESP8266_RESPONSE_WAITING);

}


bool WaitForExpectedResponse(const char* ExpectedResponse)
{
	Start_Read_Response(ExpectedResponse);	/* First read response */
	if((Response_Status != ESP8266_RESPONSE_TIMEOUT))
	return true;				/* Return true for success */
	return false;				/* Else return false */
}

bool SendATandExpectResponse(char* ATCommand, const char* ExpectedResponse)
{
	ESP8266_Clear();
	USART_SendString(ATCommand);		/* Send AT command to ESP8266 */
	USART_SendString("\r\n");
	return WaitForExpectedResponse(ExpectedResponse);
}

bool ESP8266_ApplicationMode(uint8_t Mode)
{
	char _atCommand[20];
	memset(_atCommand, 0, 20);
	sprintf(_atCommand, "AT+CIPMODE=%d", Mode);
	_atCommand[19] = 0;
	return SendATandExpectResponse(_atCommand, "\r\nOK\r\n");
}

bool ESP8266_ConnectionMode(uint8_t Mode)
{
	char _atCommand[20];
	memset(_atCommand, 0, 20);
	sprintf(_atCommand, "AT+CIPMUX=%d", Mode);
	_atCommand[19] = 0;
	return SendATandExpectResponse(_atCommand, "\r\nOK\r\n");
}

bool ESP8266_Begin()
{
	for (uint8_t i=0;i<5;i++)
	{
		if(SendATandExpectResponse("ATE0","\r\nOK\r\n")||SendATandExpectResponse("AT","\r\nOK\r\n"))
		return true;
	}
	return false;
}



bool ESP8266_WIFIMode(uint8_t _mode)
{
	char _atCommand[20];
	memset(_atCommand, 0, 20);
	sprintf(_atCommand, "AT+CWMODE=%d", _mode);
	_atCommand[19] = 0;
	return SendATandExpectResponse(_atCommand, "\r\nOK\r\n");
}

uint8_t ESP8266_JoinAccessPoint(const char* _SSID, const char* _PASSWORD)
{
	char _atCommand[60];
	memset(_atCommand, 0, 60);
	sprintf(_atCommand, "AT+CWJAP=\"%s\",\"%s\"", _SSID, _PASSWORD);
	_atCommand[59] = 0;
	if(SendATandExpectResponse(_atCommand, "\r\nWIFI CONNECTED\r\n"))
	return ESP8266_WIFI_CONNECTED;
	else{
		if(strstr(RESPONSE_BUFFER, "+CWJAP:1"))
		return ESP8266_CONNECTION_TIMEOUT;
		else if(strstr(RESPONSE_BUFFER, "+CWJAP:2"))
		return ESP8266_WRONG_PASSWORD;
		else if(strstr(RESPONSE_BUFFER, "+CWJAP:3"))
		return ESP8266_NOT_FOUND_TARGET_AP;
		else if(strstr(RESPONSE_BUFFER, "+CWJAP:4"))
		return ESP8266_CONNECTION_FAILED;
		else
		return ESP8266_JOIN_UNKNOWN_ERROR;
	}
}

uint8_t ESP8266_connected() 
{
	SendATandExpectResponse("AT+CIPSTATUS", "\r\nOK\r\n");
	if(strstr(RESPONSE_BUFFER, "STATUS:2"))
	return ESP8266_CONNECTED_TO_AP;
	else if(strstr(RESPONSE_BUFFER, "STATUS:3"))
	return ESP8266_CREATED_TRANSMISSION;
	else if(strstr(RESPONSE_BUFFER, "STATUS:4"))
	return ESP8266_TRANSMISSION_DISCONNECTED;
	else if(strstr(RESPONSE_BUFFER, "STATUS:5"))
	return ESP8266_NOT_CONNECTED_TO_AP;
	else
	return ESP8266_CONNECT_UNKNOWN_ERROR;
}

uint8_t ESP8266_Start(uint8_t _ConnectionNumber, const char* Domain, const char* Port)
{
	bool _startResponse;
	char _atCommand[60];
	memset(_atCommand, 0, 60);
	_atCommand[59] = 0;

	if(SendATandExpectResponse("AT+CIPMUX?", "CIPMUX:0"))
		sprintf(_atCommand, "AT+CIPSTART=\"TCP\",\"%s\",%s", Domain, Port);
	else
		sprintf(_atCommand, "AT+CIPSTART=\"%d\",\"TCP\",\"%s\",%s", _ConnectionNumber, Domain, Port);

	_startResponse = SendATandExpectResponse(_atCommand, "CONNECT\r\n");
	if(!_startResponse)
	{
		if(Response_Status == ESP8266_RESPONSE_TIMEOUT)
		return ESP8266_RESPONSE_TIMEOUT;
		return ESP8266_RESPONSE_ERROR;
	}
	return ESP8266_RESPONSE_FINISHED;
}

uint8_t ESP8266_Send(char* Data)
{
	char _atCommand[20];
	memset(_atCommand, 0, 20);
	sprintf(_atCommand, "AT+CIPSEND=%d", (strlen(Data)+2));
	_atCommand[19] = 0;
	SendATandExpectResponse(_atCommand, "\r\nOK\r\n>");
	if(!SendATandExpectResponse(Data, "\r\nSEND OK\r\n"))
	{
		if(Response_Status == ESP8266_RESPONSE_TIMEOUT)
		return ESP8266_RESPONSE_TIMEOUT;
		return ESP8266_RESPONSE_ERROR;
	}
	return ESP8266_RESPONSE_FINISHED;
}

int16_t ESP8266_DataAvailable()
{
	return (Counter - pointer);
}

uint8_t ESP8266_DataRead()
{
	if(pointer < Counter)
	return RESPONSE_BUFFER[pointer++];
	else{
		ESP8266_Clear();
		return 0;
	}
}

uint16_t Read_Data(char* _buffer)
{
	uint16_t len = 0;
	MSdelay(100);
	while(ESP8266_DataAvailable() > 0)
	_buffer[len++] = ESP8266_DataRead();
	return len;
}
void ADC_Init()
{    
    TRISA = 0xFF;       //set as input port
    ADCON1 = 0x00;      //ref vtg is VDD and Configure pin as analog pin*/ 
    ADCON0 =0x01;       //analog channel select bit is RA0 
    ADCON2 = 0xC0;      //Right Justified, 4Tad and Fosc/32. 
    ADRESH=0;           //Flush ADC output Register
    ADRESL=0;  
    
}
int ADC_Read(int channel)
{
    int digital;
    ADCON0 =(ADCON0 & 0b11000011)|((channel<<2) & 0b00111100);      /*channel is selected i.e (CHS3:CHS0) 
                                                                      and ADC is disabled i.e ADON=0*/
    ADCON0 |= ((1<<ADON)|(1<<GO));                   /*Enable ADC and start conversion*/
    while(ADCON0bits.GO_nDONE==1);                  /*wait for End of conversion i.e. Go/done'=0 conversion completed*/        
    digital = (ADRESL*256|ADRESH );              /*Combine 8-bit LSB and 2-bit MSB*/
    return(digital);
}
//void ADC_Read(){
//    INTCONbits.GIE = 1;
//    PIR1bits.ADIF = 0;
//    PIE1bits.ADIE = 1;
//    
//    ADCON0bits.ADON = 0;
//    ADCON0bits.GO = 1;
//    if(PIR1bits.ADIF == 1){
//        temprature = ADRESH;
//    }
//    PIR1bits.ADIF = 0;
//}

void __interrupt () ISR()				// Receive ISR routine 
{
    uint8_t oldstatus = STATUS;
    INTCONbits.GIE = 0;
    if(RCIF==1){
        RESPONSE_BUFFER[Counter] = RCREG;	// Copy data to buffer & increment counter */
        Counter++;
        if(RCSTAbits.OERR)                      // check if any overrun occur due to continuous reception */
        {           
            CREN = 0;
            NOP();
            CREN = 1;
        }
        if(Counter == DEFAULT_BUFFER_SIZE){
            Counter = 0; pointer = 0;
        }
    }
    INTCONbits.GIE = 1;
    
    STATUS = oldstatus;
}



int main(void)
{
	char _buffer[150];
	uint8_t Connect_Status;
    char value[10];
    char RH_Decimal,RH_Integral,T_Decimal,T_Integral;
    char Checksum;
    
    
    #ifdef SEND_DEMO
	uint8_t Sample = 0;
    
	#endif

	OSCCON = 0x72;				// Set internal clock to 8MHz 
    
    ADCON1=0x0F;
    
    
	USART_Init(115200);			// Initiate USART with 115200 baud rate 
	INTCONbits.GIE=1;			// enable Global Interrupt 
	INTCONbits.PEIE=1;			// enable Peripheral Interrupt 
	PIE1bits.RCIE=1;			// enable Receive Interrupt 	
    
	while(!ESP8266_Begin());
	ESP8266_WIFIMode(BOTH_STATION_AND_ACCESPOINT);// 3 = Both (AP and STA) 
	ESP8266_ConnectionMode(SINGLE);		// 0 = Single; 1 = Multi 
	ESP8266_ApplicationMode(NORMAL);	// 0 = Normal Mode; 1 = Transperant Mode 
	if(ESP8266_connected() == ESP8266_NOT_CONNECTED_TO_AP)
	ESP8266_JoinAccessPoint(SSID, PASSWORD);
	ESP8266_Start(0, DOMAIN, PORT);
    int celsius;        
    //ADC_Init();                 //ADC initialization

    ADCON1=0x0F;
	while(1)
	{
		Connect_Status = ESP8266_connected();
		if(Connect_Status == ESP8266_NOT_CONNECTED_TO_AP)
		ESP8266_JoinAccessPoint(SSID, PASSWORD);
		if(Connect_Status == ESP8266_TRANSMISSION_DISCONNECTED)
		ESP8266_Start(0, DOMAIN, PORT);
        
    DHT11_Start();                  /* send start pulse to DHT11 module */
    DHT11_CheckResponse();          /* wait for response from DHT11 module */
    
    // read 40-bit data from DHT11 module 
    RH_Integral = DHT11_ReadData(); /* read Relative Humidity's integral value */
    RH_Decimal = DHT11_ReadData();  /* read Relative Humidity's decimal value */
    T_Integral = DHT11_ReadData();  /* read Temperature's integral value */
    T_Decimal = DHT11_ReadData();   /* read Relative Temperature's decimal value */
    Checksum = DHT11_ReadData();    /* read 8-bit checksum value */
    
        
		#ifdef SEND_DEMO
		memset(_buffer, 0, 150);
		//sprintf(_buffer, "GET /update?api_key=%s&field1=%d", API_WRITE_KEY, Sample++);  
       sprintf(_buffer, "GET /update?api_key=%s&field1=%d.%d", API_WRITE_KEY,T_Integral,T_Decimal);
      
		ESP8266_Send(_buffer);
		MSdelay(20000);	
        
		#endif
	}
}