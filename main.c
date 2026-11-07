			 //main.c

#include <lpc21xx.h>
//#include "lcd_header.h"
#include "matrix_keypad.h"
#include <string.h>

void UART0_CONFIG(void);
void UART0_TX(unsigned char);
unsigned char UART0_RX(void);
void UART0_SEND_STR(char *);

unsigned char rfid_buf[15];
char password[5];   // 4 digit + '\0'
char correct_pass[] = "1234";   //password

int main(){
    unsigned char i, ch,j;
    unsigned int key, attempts;
	 IODIR0|=(1<<3)|(1<<4);	//P1.16 AL LED 
	 IODIR0|=(1<<22)|(1<<21);  //P0.20 AND P0.21 DC MOTOR ON
	 IOSET0=(1<<3);//led off 
	IOSET0 = (1<<4); //AL BUZZER OFF
	LCD_INIT();
	UART0_CONFIG();
	LCD_COMMAND(0X80);
	LCD_STR("BANKING SYSTEM PROJECT");
	delay_milliseconds(500);
	UART0_SEND_STR("BANKING SYSTEM PROJECT\r\n");
	LCD_COMMAND(0X80);
    LCD_STR("RFID READY...");
    UART0_SEND_STR("RFID EM-18 READER READY...\r\n");

    while(1){
        // Step 1: RFID read
        for(i=0;i<12;){
            ch = UART0_RX();
            if((ch>='0' && ch<='9') || (ch>='A' && ch<='F')){
                rfid_buf[i] = ch;
                i++;
            }
        }
        rfid_buf[12] = '\0';

        LCD_COMMAND(0x80);
        LCD_STR("CARD DETECTED");
        UART0_SEND_STR("CARD DETECTED TAG ID:");
        UART0_SEND_STR((char*)rfid_buf);
        UART0_SEND_STR("\r\n");

        // Step 2: RFID matching or checking
        if(strcmp((char*)rfid_buf,"060067DBFC46")==0)
		{
            attempts = 0;
			//IOSET1 = (1<<17); //AL BUZZER OFF
            while(attempts < 3){
                LCD_COMMAND(0x80);
                LCD_STR("ENTER 4 DIGIT PASSWORD:");
				UART0_SEND_STR("ENTER 4 DIGIT PASSWORD:\r\n");

                // Step 3: Collect 4 digits
                for(i=0;i<4;i++){
                    key = key_scan();
                    password[i] = '0' + key;
                    LCD_DATA(password[i]);
                }
                password[4] = '\0';

                // Step 4: Comparing password
                if(strcmp(password,correct_pass)==0){
                    LCD_COMMAND(0x01);
                    LCD_STR("ACCESS GRANTED");
					UART0_SEND_STR("ACCESS GRANTED \r\n");

                    // LED blink
                    IOCLR0=(1<<3);	   //Active LOW LED ON
					IOSET0 =(1<<21);
					IOCLR0=(1<<22);//Active high motor ON
					delay_seconds(5);
					IOSET0=(1<<3);//Active LOW LED 
                    IOCLR0=(1<<21)|(1<<22) ;
					 //MOTOR OFF

                    break;
                } else {
                    attempts++;
                    LCD_COMMAND(0x80);
                    LCD_STR("WRONG PASSWORD");
					UART0_SEND_STR("WRONG PASSWORD\r\n");
                    LCD_COMMAND(0xC0); // second line
                    LCD_STR("Attempts:");
					//UART0_SEND_STR("ATTEMPTS:\r\n");
                    LCD_DATA('0' + attempts);
                    delay_seconds(2);
                }
            }

            // Step 5: After 3 wrong attempts
            if(attempts==3){
                LCD_COMMAND(0x01);
                LCD_STR("ACCESS DENIED");
				UART0_SEND_STR("ACCESS DENIED\r\n");
				for(j=0;j<5;j++){
                IOCLR0 = (1<<3)|(1<<4); // ACTIVE LOW buzzer ON
                delay_seconds(1);
                IOSET0 = (1<<3)|(1<<4); //ACTIVE HIGH buzzer off
				}
            }
        } else {
            LCD_COMMAND(0x01);
            LCD_STR("INVALID CARD");
			UART0_SEND_STR("INVALID CARD\r\n");
        }
    }
}

// UART functions
void UART0_CONFIG(void){
    PINSEL0=0X05;
    U0LCR=0X83;
    U0DLL=97;
    U0DLM=0;
    U0LCR=0X03;
}

void UART0_TX(unsigned char ch){
    while(((U0LSR>>5)&1)==0);
    U0THR=ch;
}

unsigned char UART0_RX(void){
    while((U0LSR&0x1)==0);
    return U0RBR;
}

void UART0_SEND_STR(char *p){
    while(*p){
        UART0_TX(*p++);
    }
}
