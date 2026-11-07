//lcd header 
#include<lpc21xx.h>
#define LCD_d (0XF<<14)   //P0.14 - P0.17 (D4 TO D7)
#define RS (1<<12)	 //P0.12
#define E (1<<13)	 //P0.13

void LCD_INIT(void);
void LCD_COMMAND(unsigned char);
void LCD_DATA(unsigned char);
void LCD_STR(unsigned char *);
void delay_milliseconds(unsigned int);
void delay_seconds(unsigned int);


void LCD_INIT(void){
	IODIR0|=LCD_d|RS|E;
	LCD_COMMAND(0x01);
	LCD_COMMAND(0x02);
	LCD_COMMAND(0x0c);
	LCD_COMMAND(0x28); //4bit interface mode with both lines
}


void LCD_COMMAND(unsigned char cmd){
//send upper nibble 
IOCLR0=LCD_d;
IOSET0=(cmd & 0xF0)<<10;
IOCLR0=RS;
IOSET0=E;
delay_milliseconds(2);
IOCLR0=E;
//send lower nibble 
IOCLR0=LCD_d;
IOSET0=(cmd & 0x0F)<<14;
IOCLR0=RS;
IOSET0=E;
delay_milliseconds(2);
IOCLR0=E;
}

void LCD_DATA(unsigned char data){
//send upper nibble 
IOCLR0=LCD_d;
IOSET0=(data & 0xF0)<<10;
IOSET0=RS;
IOSET0=E;
delay_milliseconds(2);
IOCLR0=E;
//send lower nibble 
IOCLR0=LCD_d;
IOSET0=(data & 0x0F)<<14;
IOSET0=RS;
IOSET0=E;
delay_milliseconds(2);
IOCLR0=E;
}



void LCD_STR(unsigned char *s){
	signed char pos=0;
	LCD_COMMAND(0X01);
	while(*s){
		LCD_DATA(*s++);
		pos++;
		if(pos==16){
			LCD_COMMAND(0xc0);
		}
	}
}

void delay_milliseconds(unsigned int ms)
{
	T0PR=15000-1; //configured for 1 milliseconds
	T0TCR=0x01;  
	while(T0TC<ms);
	T0TCR|=(1<<1);
	T0TCR=0x00;
}

void delay_seconds(unsigned int seconds)
{
	T0PR=15000000-1; //configured for 1 seconds
	T0TCR=0x01;
	while(T0TC<seconds);
	T0TCR|=(1<<1);
	T0TCR=0x00;
}
