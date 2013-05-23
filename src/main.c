/*
 * main.c
 *
 *  Created on: May 11, 2013
 *      Author: Beannayak
 */

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdlib.h>

#define LCD_DATA PORTB
#define ctrl PORTD
#define en PD5
#define rw PD4
#define rs PD3
#define SR 13
#define SN 10
#define checkPattern "\r\n"

#define NOTHING 0
#define MESSAGE_SEND_SUCCESSFULLY 1
#define PRODUCT_DROPPING 2
#define ERROR 3
#define ERRORV 10
#define ERRORP 6
#define ERRORR 7
#define ERRORO 8
#define ERRORA 9
#define ERRORPA 4
#define DISABLED 0
#define ENABLED 1

volatile char okComplete = 0;
volatile int bufferCount = 0;
volatile char buffer[255] = {0};
volatile char msgReceived = 0;
volatile char msgNumber = -1;
volatile char msgNumber1 = -1;
volatile char ENTER=13;
volatile char ringReceived = 0;
volatile char debugFlag = 0;
volatile int counter = 0;

char *pinCode;
char *vmCode;
char *productCode;
char whatIsGoingOn = NOTHING;
char *AUTH_PHONENO = "+9779803217838";
char *phoneNo = "+9779849410132";
char authCheck = DISABLED;
char newBuffer[] = "\r\n+ctmi: \"REC READ\", \"+9779807383434\", \"asdfsdf\"\r\nbutros VM001 A03\r\nOK\r\n";

void LCDCmd(unsigned char cmd);
void initLCD(void);
void LCDData(unsigned char data);
void LCDPrint(char *str);
void LCDClear();
void LCDGotoXY(uint8_t x,uint8_t y);
void LCDSmartWrite(int x,int y,char *str);

void initLCD(void) {
	LCDCmd(0x38);	 // initialization of 16X2 LCD in 8bit mode
 	_delay_ms(1);

	LCDCmd(0x01);	 // clear LCD
 	_delay_ms(1);

 	LCDCmd(0x0C);	 // cursor ON
 	_delay_ms(1);

 	LCDCmd(0x80);	 // ---8 go to first line and --0 is for 0th position
 	_delay_ms(1);
}
void LCDCmd(unsigned char cmd) {
	LCD_DATA=cmd;
	ctrl =(0<<rs)|(0<<rw)|(1<<en);
	_delay_ms(1);
	ctrl =(0<<rs)|(0<<rw)|(0<<en);
	_delay_ms(50);
}
void LCDData(unsigned char data) {
	LCD_DATA= data;
	ctrl = (1<<rs)|(0<<rw)|(1<<en);
	_delay_ms(1);
	ctrl = (1<<rs)|(0<<rw)|(0<<en);
	_delay_ms(5);
}
void LCDPrint(char *str){
 	unsigned int i=0;
 	while(str[i]!='\0')	{
    	LCDData(str[i]);	 // sending data on LCD byte by byte
    	i++;
  	}
}
void LCDClear() {
	LCDCmd(0x01);
}
void LCDGotoXY(uint8_t x,uint8_t y) {
 	uint8_t address=0x80;
 	if(y==1) {
  		address=0xC0;
  	}

	address+=x;
  	LCDCmd(address);
}
void LCDSmartWrite(int x,int y,char *str) {
	int j=0;
 	while(str[j]) {
  		if(x>15) {
    		x=0;
			y++;
   		}

		LCDGotoXY(x,y);
  		LCDData(str[j]);
  		j++;
  		x++;
 	}
}
void initLcd() { initLCD(); }
void LCDPrintLines(char *line1, char *line2){
	LCDClear();
	LCDPrint (line1);
	LCDGotoXY(0, 1);
	LCDPrint (line2);
}

void USART_Init( unsigned char ubrr) {
	UBRRH = 0;
	UBRRL = ubrr;

	UCSRB|= (1<<RXEN)|(1<<TXEN) | (1<<RXCIE);
	UCSRC |= (1 << URSEL)|(3<<UCSZ0);
}
void USART_Transmit( unsigned char data ) {
	while ( !( UCSRA & (1<<UDRE))) ;
	UDR = data;
}
unsigned char USART_Receive( void ) {
	while ( !(UCSRA & (1<<RXC))) ;
	return UDR;
}
void usartPrintString(char *buffer){
	int count = 0;
	while (buffer[count] != '\0'){
		USART_Transmit(buffer[count]);
		count++;
	}
}

ISR(USART_RXC_vect){
	unsigned char a;
	char done = 0;
	a = UDR;
	buffer[bufferCount] = a;
	bufferCount++;

	/************* Check for OK *******************************/
	if (buffer[bufferCount-4] == 'O'){
		if (buffer[bufferCount-3] == 'K'){
			if (buffer[bufferCount-2] == SR){
				if (buffer[bufferCount-1] == SN){
					buffer[bufferCount] = '\0';
					okComplete = 1;

					bufferCount = 0;
					done = 1;
				}
			}
		}
	}
	/***********************************************************/

	/************************ Check for ERROR ******************/
	if (buffer[bufferCount-4] == 'O' && !(done)){
		if (buffer[bufferCount-3] == 'R'){
			if (buffer[bufferCount-2] == SR){
				if (buffer[bufferCount-1] == SN){
					buffer[bufferCount] = '\0';
					okComplete = 2;

					bufferCount = 0;
					done = 1;
				}
			}
		}
	}
	/***********************************************************/

	/************** Check for New Message 1 bit**********************/
	if (buffer[bufferCount-1] == SN && !(done)){
		if (buffer[bufferCount-2] == SR){
			if (buffer[bufferCount-4] == ','){
				if (buffer[bufferCount-5] == 34){
					if (buffer[bufferCount-6] == 'M'){
						if (buffer[bufferCount-7] == 'S'){
							if (buffer[bufferCount-8] == 34){
								msgReceived = 1;
								msgNumber = buffer[bufferCount-3];
								msgNumber1 = -1;
								buffer[bufferCount] = '\0';

								done = 1;
								bufferCount = 0;
							}
						}
					}
				}
			}
		}
	}
	/***********************************************************/

	/************** Check for New Message 2 bit**********************/
		if (buffer[bufferCount-1] == SN && !(done)){
			if (buffer[bufferCount-2] == SR){
				if (buffer[bufferCount-5] == ','){
					if (buffer[bufferCount-6] == 34){
						if (buffer[bufferCount-7] == 'M'){
							if (buffer[bufferCount-8] == 'S'){
								if (buffer[bufferCount-9] == 34){
									msgReceived = 1;
									msgNumber = buffer[bufferCount - 4];
									msgNumber1 = buffer[bufferCount - 3];
									buffer[bufferCount] = '\0';

									done = 1;
									bufferCount = 0;
								}
							}
						}
					}
				}
			}
		}
		/***********************************************************/

	/********************** check for RING *********************/
	if (buffer[bufferCount-1] == SN && !(done)){
		if (buffer[bufferCount - 2] == SR){
			if (buffer[bufferCount - 3] == 'G'){
				if (buffer[bufferCount - 4] == 'N'){
					if (buffer[bufferCount - 5] == 'I'){
						if (buffer[bufferCount - 6] == 'R'){
							ringReceived = 1;

							done = 1;
							bufferCount = 0;
						}
					}
				}
			}
		}
	}
	/**************************************************************/

	/*************** bit advance check for error ******************/
	if (buffer[bufferCount - 1] == 'R' && !(done)){
		if (buffer[bufferCount - 2] == 'O'){
			if (buffer[bufferCount - 3] == 'R'){
				if (buffer[bufferCount - 4] == 'R'){
					if (buffer[bufferCount - 5] == 'E'){
						buffer[bufferCount] = '\0';
						okComplete = 2;
						bufferCount = 0;
						done = 1;
					}
				}
			}
		}
	}
	/**************************************************************/
}

/***************** String Manipulation Functions *********************/
//@ string manipulation functions algorithm
//@ copyright: Binayak Dhakal,
//@ Functions: length, indexOf, indexOfWithStart, subString, strCmp, occuranceOf
//@ split, freeSplitedString, replace
int length(char *string){
	int x = 0;
	while (string[x] != '\0'){
		x++;
	}
	return x;
}
int indexOf(char *string, char *subString) {
	//@ result is zero base
	int lenSubString = 0;
	int lenString = 0;
	int x, loopEnd, y;
	int flag;
	int indexOf = -1;

	lenSubString = length(subString);
	lenString = length(string);
	loopEnd = (lenString-lenSubString);

	for (x=0; x<=loopEnd; x++){
		flag = 1;
		for (y=0; y<lenSubString; y++){
			if (string[x] != subString[y]){
				flag = 0;
				break;
			} else {
				x++;
			}
		}
		if (flag == 1){
			indexOf = x - lenSubString;
			break;
		}
	}
	return indexOf;
}
int indexOfWithStart(char *string, char *subString, int start) {
	//@ result is zero base
	int lenSubString = 0;
	int lenString = 0;
	int x, loopEnd, y;
	int flag;
	int indexOf = -1;

	lenSubString = length(subString);
	lenString = length(string);
	loopEnd = (lenString-lenSubString);

	for (x=start; x<=loopEnd; x++){
		flag = 1;
		for (y=0; y<lenSubString; y++){
			if (string[x] != subString[y]){
				flag = 0;
				break;
			} else {
				x++;
			}
		}
		if (flag == 1){
			indexOf = x - lenSubString;
			break;
		}
	}
	return indexOf;
}
char* subString(char* string, int start, int length){
	//@ Start is zero based
	char *a = (char*) malloc (length+1);
	int x;
	for (x=0; x<length; x++){
		a[x] = string[x+start];
	}
	a[x] = '\0';

	return a;
}
char equals(char *stringA, char *stringB){
	char returnResult = 0;
	int x;

	if (length(stringA) == length(stringB)){
		returnResult = 1;
		for (x=0; x<=length(stringA); x++){
			if (stringA[x] != stringB[x]){
				returnResult = 0;
				break;
			}
		}
	}
	return returnResult;
}
int occuranceOf(char *string, char *delimiter){
	int index, start;
	int delimiterCount=0;
	start = 0;
	while (1){
		index = indexOfWithStart(string, delimiter, start);
		if (index != -1){
			delimiterCount ++;
		} else {
			break;
		}
		start = index + 1;
	}
	return delimiterCount;
}
void strCpy(char *source, char *dest){
	int count =0;
	while (source[count] != '\0'){
		dest[count] = source[count];
		count ++;
	}
	dest[count] = '\0';
}
char **split(char *string, char *delimiter, int *ubound){
		char **retVal;
		int delimiterCount = 0;
		int index, start = 0;
		int count = 0, prevCount = 0;

		delimiterCount = occuranceOf(string, delimiter);
		*ubound = delimiterCount;

		retVal = (char **) malloc ((sizeof(char*)) * ((delimiterCount) + 1));
		start = 0;
		while (1){
			index = indexOfWithStart(string, delimiter, start);

			if (index != -1)
			{
				retVal[count] = subString(string, prevCount, index-prevCount);
				prevCount = index + length(delimiter);
				count++;
			} else {
				break;
			}
			start = index + length(delimiter);
		}
		retVal[count] = subString(string, prevCount, length(string)-prevCount);
		return retVal;
}
void freeSplitedString(char **splitedString, int length){
	int a;
	for (a=0; a<=length; a++){
		free (splitedString[a]);
	}
	free (splitedString);
}
char *replace(char *string, char *replaceFor, char *replaceWith){
	int occurance;
	int mallocLength = 0, lengthString = 0, lengthReplaceFor = 0, lengthReplaceWith=0;
	char *retVal;
	int start, index, count, prevCount;
	int x;

	occurance = occuranceOf(string, replaceFor);
	lengthString = length(string);
	lengthReplaceFor = length(replaceFor);
	lengthReplaceWith = length(replaceWith);
	mallocLength = lengthString - lengthReplaceFor * occurance + lengthReplaceWith * occurance;
	retVal = (char*) malloc(mallocLength+1);

	start = 0;
	prevCount = 0;
	count = 0;
	while (1){
		index = indexOfWithStart(string, replaceFor, start);
		if (index != -1){
			for (x = 0; x<index - prevCount; x++){
				retVal[count] = string[x+prevCount];
				count++;
			}
			for (x=0; x<lengthReplaceWith; x++){
				retVal[count] = replaceWith[x];
				count++;
			}
			prevCount = index + lengthReplaceFor;
		} else {
			break;
		}
		start = index + lengthReplaceFor;
	}
	char *temp = subString(string, prevCount, length(string)-prevCount);
	for (x=0; x<length(temp); x++){
		retVal[count] = temp[x];
		count++;
	}
	free(temp);
	retVal[count] = '\0';
	return retVal;
}
char *toLower(char *upperCasedString){
     char *a;
     int count = 0;

     a = (char *)malloc(length(upperCasedString)+1);
     while (upperCasedString[count] != '\0'){
           if (upperCasedString[count] >= 'A' && upperCasedString[count] <= 'Z'){
               a[count] = upperCasedString[count] + 32;
           } else {
             a[count] = upperCasedString[count];
           }
           count++;
     }
     a[count] = '\0';
     return a;
}
int toInteger(char *number){
	int count = 0;
	int value = 0;
	while (number[count] != '\0'){
		value = value * 10 + (number[count]-48);
		count++;
	}
	return value;
}
/***********************************************************************/

void sendATCommand(char *str){
	usartPrintString(str);
	USART_Transmit(ENTER);
}
void sendATCommand1(char *str, char number){
	usartPrintString(str);
	USART_Transmit(number);
	USART_Transmit(ENTER);
}
void sendATCommand2(char *str, char number, char number2){
	usartPrintString(str);
	USART_Transmit(number);
	USART_Transmit(number2);
	USART_Transmit(ENTER);
}
void sendMessage(char *number, char *message, char success){
	usartPrintString("at+cmgs=\"");
	usartPrintString (number);
	usartPrintString ("\"\r");
	usartPrintString(message);
	if (success == 1){
		usartPrintString(" success");
	} else if (success == 2){
		usartPrintString(" failure");
	}
	USART_Transmit(26);
	USART_Transmit(ENTER);
}
void sendMessage1(char *number, char *message, char *message2, char success){
	usartPrintString("at+cmgs=\"");
	usartPrintString (number);
	usartPrintString ("\"\r");
	usartPrintString(message);
	USART_Transmit (' ');
	usartPrintString(message2);
	if (success == 1){
		usartPrintString(" success");
	} else if (success == 2){
		usartPrintString(" failure");
	}
	USART_Transmit(26);
	USART_Transmit(ENTER);
}

int main(){
	char **a;
	char **userNo;
	char **textContent;
	int textContentCounter = 0;
	int userNoCounter;

	DDRB = 0xFF;
	DDRD = 0b11111100;
	DDRC = 0xFF;
	DDRA = 0x00;
	PORTA = 0x00;

	initLCD();
	USART_Init(78);
	sei();
	/************************* display Init message ********************/
	LCDClear();
	LCDPrint ("uC working");
	_delay_ms(1000);
	/*******************************************************************/

	/****************************** GSM INIT ***************************/
	okComplete = 0;
	sendATCommand("at");
	while (okComplete == 0);

	if (okComplete == 1) {
		/************** GSM initilization success ******************/
		LCDClear();
		LCDPrintLines ("GSM init.:", "successful");
		/***********************************************************/

		/****** Check for EEPROM value and reset it ****************/
		/***********************************************************/
	}else {
		/************ GSM unsuccessful in initilization *************/
		LCDClear();
		LCDPrintLines ("GSM init.:", "failure");
		_delay_ms(1000);
		LCDClear();
		LCDPrint ("System Halting...");
		while (1);
		/************************************************************/
	}
	_delay_ms(1000);
	okComplete = 0;
	sendATCommand("at+cmgda=\"DEL ALL\"");
	while (okComplete == 0);
	if (okComplete == 1){
		LCDPrintLines ("message Delete",  "successful");
	} else if (okComplete == 2) {
		LCDPrintLines ("can't del. msg.", "system halting...");
		while (1);
	}
	_delay_ms(1000);
	okComplete = 1;
	/*******************************************************************/

	whatIsGoingOn = NOTHING;
	bufferCount = 0;
	while (1){
		if (okComplete == 1){
			//OK received
			LCDClear();
			if (whatIsGoingOn == NOTHING){
				LCDPrintLines("Vending Machine", "Code: vm001");
			} else if (whatIsGoingOn == MESSAGE_SEND_SUCCESSFULLY){
				LCDPrintLines("Product Dropped", "Thank You");
			} else if (whatIsGoingOn == ERRORV){
				LCDPrintLines ("Last Message:", "vmCode invalid");
			} else if (whatIsGoingOn == ERRORP){
				LCDPrintLines ("Last Message:", "itemCode invalid");
			} else if (whatIsGoingOn == ERRORR){
				LCDPrintLines ("Last Message:", "5 Retries Error");
			} else if (whatIsGoingOn == ERRORO){
				LCDPrintLines ("Last Message:", "Overfall");
			} else if (whatIsGoingOn == ERRORA){
				LCDPrintLines ("Last Message:", "unavail. product");
			} else if (whatIsGoingOn == ERRORPA){
				LCDPrintLines ("Last Message:", "und. pinCode len");
			}
			_delay_ms(1000);
			if (whatIsGoingOn != NOTHING){
				whatIsGoingOn = NOTHING;
				okComplete = 1;
			} else {
				okComplete = 0;
			}
		} else if (okComplete == 2){
			//ERROR Received
			LCDClear();
			LCDPrint ("Error Received");
			_delay_ms(1000);

			okComplete = 1;
		}else if (msgReceived == 1){
			//=CMTI Received
			LCDClear();
			LCDPrint ("msg rec @: ");

			if (msgNumber1 == 255){
				LCDData ((char)msgNumber);
			} else {
				LCDData ((char)msgNumber);
				LCDData ((char)msgNumber1);
			}

			/******** read message and process on it **************/

			//@1 set EEPROM
			//@1 (set EEPROM) complete

			okComplete = 0;
			/************** try 5 times to read SMS ***************/
			char repeatCounter = 2;
			while (repeatCounter <= 5){

				if (msgNumber1 == 255){
					sendATCommand1("at+cmgr=", msgNumber);
				} else {
					sendATCommand2("at+cmgr=", msgNumber, msgNumber1);
				}

				while ((okComplete == 0)){
					_delay_ms(200);
				}

				repeatCounter ++;
				if (okComplete == 1 || repeatCounter > 5){
					break;
				} else {
					okComplete = 0;
				}
			}
			/******************************************************/

			if (okComplete == 1){
				/*********** AT+CMGR value received in buffer *********/

				/*********** check for user number ********************/
				if (authCheck){
				userNo = split((char *) buffer, "\"", &userNoCounter);
				if (equals(userNo[3], AUTH_PHONENO) == 0){
					freeSplitedString(userNo, userNoCounter);
					sendMessage1 (phoneNo, "Unknown number detected: ", userNo[3], 2);
					msgReceived = 0;
					msgNumber = -1;
					msgNumber1 = -1;
					continue;
				}
				freeSplitedString(userNo, userNoCounter);
				}
				/******************************************************/

				/********** Check for message format ******************/
				//@1 Spliting message
				char *tempA2Lower;
				a = split((char*) buffer, checkPattern, (int *)&counter);

				tempA2Lower = toLower(a[2]);
				free(a[2]);
				a[2] = tempA2Lower;
				textContent = split (a[2], " ", &textContentCounter);

				pinCode = textContent[0];
				vmCode = textContent[1];
				productCode = textContent[2];
				//@1 Complete

				//@2 Check for 6 digit pinCode
				if (length(pinCode) == 6){
					//@3 check for vmMachine code
					if (equals(vmCode, "vm001") == 1){
						//@4 check for itemcode
						char *subSubString;
						subSubString = subString(productCode, 0, 1);
						if (equals(subSubString, "a")){
							char *subSubString2;

							//@5 check for productValue
							int productValue;
							subSubString2 = subString(productCode, 1, 2);
							productValue = toInteger(subSubString2);
							if (productValue > 6 || productValue < 1){
								/**** unavailable product ***********/
								sendMessage1 (phoneNo, pinCode, "unavailable product", 2);
								LCDPrintLines("Error: ", "unavailable product");
								whatIsGoingOn = ERRORA;
								/************************************/
							} else {
								/******** Every thing is correct drop the product *****/
								//@6 drop the product and display in LCD
								PORTC = (1 << (5 - (productValue - 1)));
								LCDPrintLines("Dropping", "Product: ");
								LCDPrint(subSubString2);
								//@6 (drop the product and display in LCD) complete

								//@7 start motor and count for 2 sec
								unsigned int fallCounter = 0;
								while ((!(bit_is_set(PINA, PA6))) && (!(bit_is_set(PINA, PA7)))) {
									/******* find out time take here *******/
									_delay_ms(1);
									if (fallCounter > 2000){
										break;
									}
									fallCounter++;
									/****************************************/
								}
								//@8 stop the motor and check for time
								PORTC = 0x00;
								if (fallCounter >= 2000) {
									/*********** overFall error **************/
									sendMessage1 (phoneNo, pinCode, "Overfall error", 2);
									LCDPrintLines("Error: ", "Overfall");
									whatIsGoingOn = ERRORO;
									/*****************************************/
								} else {
									/*********** success **************/
									sendMessage (phoneNo, pinCode, 1);
									LCDPrintLines("Drop: ", "successful");
									whatIsGoingOn = MESSAGE_SEND_SUCCESSFULLY;
									/*****************************************/
								}
								//@7 () Complete
								/******************************************************/
							}
							free (subSubString2);
							//@5 (Product value check) complete
						} else {
							/****** invalid vmCode *******/
							sendMessage1 (phoneNo, pinCode, "productCode invalid", 2);
							LCDPrintLines("Error: ", "productCode invalid");
							whatIsGoingOn = ERRORP;
							/*****************************/
						}
						free (subSubString);
						//@4 (check for itemcode) complete
					} else {
						/****** invalid vmCode *******/
						sendMessage1 (phoneNo, pinCode, "vmCode invalid", 2);
						LCDPrintLines("Error: ", "vmCode invalid");
						whatIsGoingOn = ERRORV;
						/*****************************/
					}
					//@3 (check for vmMachine code) completed
				} else {
					/******* invalid pinCode *******/
					sendMessage1 (phoneNo, pinCode, "pinCode invalid", 2);
					LCDPrintLines("Error: und.", "pinCode length");
					whatIsGoingOn = ERRORPA;
					/*******************************/
				}
				//@2 (check for 6 digit pinCode) complete
				freeSplitedString (a, counter);
				freeSplitedString (textContent, textContentCounter);
				/******************************************************/

				/******************************************************/
			} else if (okComplete == 2){
				/**************** error reading sms *******************/
				sendMessage (phoneNo, "couldn't read message from GSM modem", 2);
				LCDPrintLines("Err Reading SMS", "5 Retries Error");
				whatIsGoingOn = ERRORR;
				/******************************************************/
			}

			/*********** delete the message **************/
			if (msgNumber1 == 255) {
				sendATCommand1("at+cmgd=", msgNumber);
			} else {
				sendATCommand2("at+cmgd=", msgNumber, msgNumber1);
			}
			/*********************************************/

			//@2 reset EEPROM
			//@2 (reset EEPROM) complete

			_delay_ms(1000);

			msgNumber = -1;
			msgNumber1 = -1;
			msgReceived = 0;
		} else if (ringReceived == 1){
			//RING received
			LCDClear();
			LCDPrintLines("Message Received:","dummy message");
			_delay_ms(200);
			/******** read message and process on it **************/

			//@1 set EEPROM
			//@1 (set EEPROM) complete

			okComplete = 1;
			strCpy((char*)newBuffer, (char*)buffer);
			if (okComplete == 1){
				/*********** AT+CMGR value received in buffer *********/

				/*********** check for user number ********************/
				if (authCheck){
				userNo = split((char *) buffer, "\"", &userNoCounter);
				if (equals(userNo[3], AUTH_PHONENO) == 0){
					freeSplitedString(userNo, userNoCounter);
					sendMessage1 (phoneNo, "Unknown number detected: ", userNo[3], 2);
					msgReceived = 0;
					msgNumber = -1;
					msgNumber1 = -1;
					continue;
				}
				freeSplitedString(userNo, userNoCounter);
				}
				/******************************************************/

				/********** Check for message format ******************/
				//@1 Spliting message
				char *tempA2Lower;
				a = split((char*) buffer, checkPattern, (int *)&counter);

				tempA2Lower = toLower(a[2]);
				free(a[2]);
				a[2] = tempA2Lower;

				textContent = split (a[2], " ", &textContentCounter);

				pinCode = textContent[0];
				vmCode = textContent[1];
				productCode = textContent[2];
				//@1 Complete

				//@2 Check for 6 digit pinCode
				if (length(pinCode) == 6){
					//@3 check for vmMachine code
					if (equals(vmCode, "vm001") == 1){
						//@4 check for itemcode
						char *subSubString;
						subSubString = subString(productCode, 0, 1);
						if (equals(subSubString, "a")){
							char *subSubString2;

							//@5 check for productValue
							int productValue;
							subSubString2 = subString(productCode, 1, 2);
							productValue = toInteger(subSubString2);
							if (productValue > 6 || productValue < 1){
								/**** unavailable product ***********/
								LCDPrintLines("Error: ", "unavailable product");
								whatIsGoingOn = ERRORA;
								/************************************/
							} else {
								/******** Every thing is correct drop the product *****/
								//@6 drop the product and display in LCD
								PORTC = (1 << (5 - (productValue - 1)));
								LCDPrintLines("Dropping", "Product: ");
								LCDPrint(subSubString2);
								//@6 (drop the product and display in LCD) complete

								//@7 start motor and count for 2 sec
								unsigned int fallCounter = 0;
								while ((!(bit_is_set(PINA, PA6))) && (!(bit_is_set(PINA, PA7)))) {
									/******* find out time take here *******/
									_delay_ms(1);
									if (fallCounter > 2000){
										break;
									}
									fallCounter++;
									/****************************************/
								}
								//@8 stop the motor and check for time
								PORTC = 0x00;
								if (fallCounter >= 2000) {
									/*********** overFall error **************/
									LCDPrintLines("Error: ", "Overfall");
									whatIsGoingOn = ERRORO;
									/*****************************************/
								} else {
									/*********** success **************/
									LCDPrint("Drop success");
									whatIsGoingOn = MESSAGE_SEND_SUCCESSFULLY;
									/*****************************************/
								}
								//@7 () Complete
								/******************************************************/
							}
							free (subSubString2);
							//@5 (Product value check) complete
						} else {
							/****** invalid vmCode *******/
							LCDPrintLines("Error: ", "productCode invalid");
							whatIsGoingOn = ERRORP;
							/*****************************/
						}
						free (subSubString);
						//@4 (check for itemcode) complete
					} else {
						/****** invalid vmCode *******/
						LCDPrintLines("Error: ", "vmCode invalid");
						whatIsGoingOn = ERRORV;
						/*****************************/
					}
					//@3 (check for vmMachine code) completed
				} else {
					/******* invalid pinCode *******/
					LCDPrintLines("Error: und.", "pinCode length");
					whatIsGoingOn = ERRORPA;
					/*******************************/
				}
				//@2 (check for 6 digit pinCode) complete
				freeSplitedString (a, counter);
				freeSplitedString (textContent, textContentCounter);
				/******************************************************/

				/******************************************************/
			} else if (okComplete == 2){
				/**************** error reading sms *******************/
				LCDPrintLines("Err Reading SMS", "5 Retries Error");
				whatIsGoingOn = ERRORR;
				/******************************************************/
			}

			//@2 reset EEPROM
			//@2 (reset EEPROM) complete

			_delay_ms(1000);

			msgNumber = -1;
			msgNumber1 = -1;
			msgReceived = 0;
			/***********************************************************************/

			ringReceived = 0;
		}
	}
	return 0;
}
