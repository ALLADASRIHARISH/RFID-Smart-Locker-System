#include <LPC21xx.h>
#include <string.h>

/* =====================================================
   SMART BANK LOCKER ACCESS SYSTEM
   Controller : LPC2129 (ARM7)
   Compiler   : Keil uVision
   PCLK       : 15 MHz

   LCD        : 16x2, 4-bit mode
   RFID       : EM-18, UART0
   KEYPAD     : 4x4 matrix
   PC         : UART1
   ===================================================== */

/* ---------------- LCD CONNECTIONS ---------------- */

#define LCD_D4   (1U << 16)
#define LCD_D5   (1U << 17)
#define LCD_D6   (1U << 18)
#define LCD_D7   (1U << 19)

#define LCD_RS   (1U << 20)
#define LCD_EN   (1U << 21)

#define LCD_DATA (LCD_D4 | LCD_D5 | LCD_D6 | LCD_D7)

/* ---------------- OUTPUT CONNECTIONS ------------- */

#define RELAY   (1U << 22)
#define BUZZER  (1U << 23)
#define GREEN   (1U << 24)

/* ---------------- KEYPAD CONNECTIONS ------------- */

#define ROWS (0x0FU << 4)

#define COLS ((1U << 12) | \
              (1U << 13) | \
              (1U << 14) | \
              (1U << 15))

/* ---------------- SECURITY SETTINGS -------------- */

/* Replace with your actual 12-character RFID frame */
#define AUTHORIZED_RFID "0123456789AB"

/* Change this to your desired password */
#define LOCKER_PASSWORD "2580"

#define MAX_ATTEMPTS 3

/* ---------------- GLOBAL VARIABLES --------------- */

unsigned int wrong_attempts = 0;

char entered_password[5];

char rfid_buffer[13];

/* =================================================
   DELAY FUNCTIONS
   ================================================= */

void delay_ms(unsigned int ms)
{
    volatile unsigned int i;
    volatile unsigned int j;

    for(i = 0; i < ms; i++)
    {
        for(j = 0; j < 3500; j++);
    }
}

void delay_us(unsigned int us)
{
    volatile unsigned int i;

    for(i = 0; i < us * 4; i++);
}

/* =================================================
   GPIO INITIALIZATION
   ================================================= */

void GPIO_Init(void)
{
    /*
       Keep P0.0, P0.1 as UART0.
       Keep P0.8, P0.9 as UART1.

       Configure other assigned pins as GPIO.
    */

    PINSEL0 &= ~((3U << 8)  |
                 (3U << 10) |
                 (0xFFU << 24));

    PINSEL1 &= ~0x0003FFFFU;

    /* LCD outputs */

    IODIR0 |= LCD_DATA;
    IODIR0 |= LCD_RS;
    IODIR0 |= LCD_EN;

    /* Relay, buzzer and LED outputs */

    IODIR0 |= RELAY;
    IODIR0 |= BUZZER;
    IODIR0 |= GREEN;

    /* Keypad rows output */

    IODIR0 |= ROWS;

    /* Keypad columns input */

    IODIR0 &= ~COLS;

    /* Initial output state */

    IOCLR0 = LCD_DATA | LCD_RS | LCD_EN;
    IOCLR0 = RELAY | BUZZER | GREEN;

    IOSET0 = ROWS;
}

/* =================================================
   LCD DRIVER
   ================================================= */

void LCD_Enable(void)
{
    IOSET0 = LCD_EN;

    delay_us(10);

    IOCLR0 = LCD_EN;

    delay_us(100);
}

void LCD_SendNibble(unsigned char data)
{
    IOCLR0 = LCD_DATA;

    if(data & 0x01)
        IOSET0 = LCD_D4;

    if(data & 0x02)
        IOSET0 = LCD_D5;

    if(data & 0x04)
        IOSET0 = LCD_D6;

    if(data & 0x08)
        IOSET0 = LCD_D7;

    LCD_Enable();
}

void LCD_Command(unsigned char cmd)
{
    IOCLR0 = LCD_RS;

    LCD_SendNibble((cmd >> 4) & 0x0F);

    LCD_SendNibble(cmd & 0x0F);

    delay_ms(2);
}

void LCD_Data(unsigned char data)
{
    IOSET0 = LCD_RS;

    LCD_SendNibble((data >> 4) & 0x0F);

    LCD_SendNibble(data & 0x0F);

    delay_ms(1);
}

void LCD_String(char *str)
{
    while(*str)
    {
        LCD_Data(*str);
        str++;
    }
}

void LCD_Clear(void)
{
    LCD_Command(0x01);
    delay_ms(3);
}

void LCD_GotoXY(unsigned char row,
                unsigned char col)
{
    if(row == 0)
        LCD_Command(0x80 + col);
    else
        LCD_Command(0xC0 + col);
}

void LCD_Init(void)
{
    delay_ms(50);

    IOCLR0 = LCD_RS;
    IOCLR0 = LCD_EN;

    /* Initialize LCD in 4-bit mode */

    LCD_SendNibble(0x03);
    delay_ms(5);

    LCD_SendNibble(0x03);
    delay_ms(1);

    LCD_SendNibble(0x03);
    delay_ms(1);

    LCD_SendNibble(0x02);

    LCD_Command(0x28);  /* 4-bit, 2-line */
    LCD_Command(0x0C);  /* Display ON */
    LCD_Command(0x06);  /* Entry mode */
    LCD_Command(0x01);  /* Clear */

    delay_ms(5);
}

/* =================================================
   UART0 - RFID COMMUNICATION
   ================================================= */

void UART0_Init(void)
{
    /*
       P0.0 = RXD0
       P0.1 = TXD0
    */

    PINSEL0 &= ~0x0000000FU;

    PINSEL0 |=  0x00000005U;

    U0LCR = 0x83;

    /*
       PCLK = 15 MHz
       Baud = 9600

       Divisor approximately 98
    */

    U0DLM = 0;
    U0DLL = 98;

    U0FCR = 0x07;

    U0LCR = 0x03;
}

char UART0_Rx(void)
{
    while(!(U0LSR & 0x01));

    return U0RBR;
}

void UART0_Tx(char data)
{
    while(!(U0LSR & 0x20));

    U0THR = data;
}

/*
   Receive exactly 12 RFID characters.
   CR and LF are ignored.
   Returns 1 on success, 0 on timeout.
*/

int RFID_Read(char *buffer)
{
    unsigned int count;
    unsigned int timeout;
    char data;

    count = 0;

    while(count < 12)
    {
        timeout = 0;

        while(!(U0LSR & 0x01))
        {
            timeout++;

            if(timeout > 1000000)
                return 0;
        }

        data = U0RBR;

        if(data == '\r' || data == '\n')
            continue;

        buffer[count] = data;

        count++;
    }

    buffer[12] = '\0';

    return 1;
}

/* =================================================
   UART1 - PC COMMUNICATION
   ================================================= */

void UART1_Init(void)
{
    /*
       P0.8 = TXD1
       P0.9 = RXD1
    */

    PINSEL0 &= ~0x000F0000U;

    PINSEL0 |=  0x00050000U;

    U1LCR = 0x83;

    U1DLM = 0;
    U1DLL = 98;

    U1FCR = 0x07;

    U1LCR = 0x03;
}

void UART1_Tx(char data)
{
    while(!(U1LSR & 0x20));

    U1THR = data;
}

void UART1_String(char *str)
{
    while(*str)
    {
        UART1_Tx(*str);
        str++;
    }
}

/* =================================================
   KEYPAD DRIVER - 4x4 MATRIX
   ================================================= */

char keypad_map[4][4] =
{
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};

char KEYPAD_Read(void)
{
    unsigned int row;
    unsigned int col;
    unsigned int row_mask;
    unsigned int col_mask;

    for(row = 0; row < 4; row++)
    {
        /* Make all rows HIGH */

        IOSET0 = ROWS;

        /* Select one row */

        row_mask = (1U << (row + 4));

        IOCLR0 = row_mask;

        delay_ms(2);

        for(col = 0; col < 4; col++)
        {
            col_mask = (1U << (col + 12));

            if(!(IOPIN0 & col_mask))
            {
                delay_ms(25);

                if(!(IOPIN0 & col_mask))
                {
                    /* Wait for key release */

                    while(!(IOPIN0 & col_mask));

                    delay_ms(20);

                    return keypad_map[row][col];
                }
            }
        }
    }

    return 0;
}

/* =================================================
   BUZZER CONTROL
   ================================================= */

void BUZZER_ON(void)
{
    IOSET0 = BUZZER;
}

void BUZZER_OFF(void)
{
    IOCLR0 = BUZZER;
}

void BUZZER_BEEP(void)
{
    BUZZER_ON();

    delay_ms(500);

    BUZZER_OFF();
}

/* =================================================
   GREEN LED CONTROL
   ================================================= */

void GREEN_LED_ON(void)
{
    IOSET0 = GREEN;
}

void GREEN_LED_OFF(void)
{
    IOCLR0 = GREEN;
}

/* =================================================
   LOCKER MOTOR / RELAY CONTROL
   ================================================= */

void LOCKER_UNLOCK(void)
{
    /*
       This controls the relay input.
       A suitable external driver is required.
    */

    IOSET0 = RELAY;
}

void LOCKER_LOCK(void)
{
    IOCLR0 = RELAY;
}

/* =================================================
   RFID AUTHENTICATION
   ================================================= */

int RFID_Verify(void)
{
    LCD_Clear();

    LCD_GotoXY(0,0);

    LCD_String("SCAN RFID CARD");

    LCD_GotoXY(1,0);

    LCD_String("Waiting...");

    UART1_String("\r\nWaiting for RFID card...\r\n");

    if(RFID_Read(rfid_buffer) == 0)
    {
        return 0;
    }

    UART1_String("RFID: ");
    UART1_String(rfid_buffer);
    UART1_String("\r\n");

    if(strcmp(rfid_buffer, AUTHORIZED_RFID) == 0)
    {
        LCD_Clear();

        LCD_GotoXY(0,0);

        LCD_String("RFID VERIFIED");

        UART1_String("Authorized RFID\r\n");

        delay_ms(1000);

        return 1;
    }

    LCD_Clear();

    LCD_GotoXY(0,0);

    LCD_String("INVALID RFID");

    LCD_GotoXY(1,0);

    LCD_String("ACCESS DENIED");

    UART1_String("Unauthorized RFID\r\n");

    BUZZER_BEEP();

    delay_ms(1000);

    return 0;
}

/* =================================================
   PASSWORD AUTHENTICATION
   ================================================= */

int Password_Verify(void)
{
    unsigned int index;
    char key;

    index = 0;

    entered_password[0] = '\0';

    LCD_Clear();

    LCD_GotoXY(0,0);

    LCD_String("ENTER PASSWORD");

    LCD_GotoXY(1,0);

    while(1)
    {
        key = KEYPAD_Read();

        if(key == 0)
            continue;

        /* Clear entered password */

        if(key == '*')
        {
            index = 0;

            entered_password[0] = '\0';

            LCD_GotoXY(1,0);

            LCD_String("                ");

            LCD_GotoXY(1,0);
        }

        /* Submit password */

        else if(key == '#')
        {
            entered_password[index] = '\0';

            break;
        }

        /* Accept only numeric keys */

        else if(key >= '0' && key <= '9')
        {
            if(index < 4)
            {
                entered_password[index] = key;

                index++;

                LCD_Data('*');
            }
        }
    }

    if(strcmp(entered_password,
              LOCKER_PASSWORD) == 0)
    {
        return 1;
    }

    return 0;
}

/* =================================================
   SECURITY LOCKOUT
   ================================================= */

void SECURITY_LOCKOUT(void)
{
    LCD_Clear();

    LCD_GotoXY(0,0);

    LCD_String("SYSTEM LOCKED");

    LCD_GotoXY(1,0);

    LCD_String("CALL ADMIN");

    UART1_String("\r\nSECURITY ALERT!\r\n");
    UART1_String("3 WRONG PASSWORDS\r\n");
    UART1_String("SYSTEM LOCKED\r\n");

    LOCKER_LOCK();

    GREEN_LED_OFF();

    while(1)
    {
        BUZZER_ON();

        delay_ms(300);

        BUZZER_OFF();

        delay_ms(300);
    }
}

/* =================================================
   ACCESS GRANTED
   ================================================= */

void ACCESS_GRANTED(void)
{
    LCD_Clear();

    LCD_GotoXY(0,0);

    LCD_String("ACCESS GRANTED");

    LCD_GotoXY(1,0);

    LCD_String("LOCKER OPEN");

    UART1_String("ACCESS GRANTED\r\n");

    GREEN_LED_ON();

    LOCKER_UNLOCK();

    BUZZER_BEEP();

    /* Keep locker open for 5 seconds */

    delay_ms(5000);

    LOCKER_LOCK();

    GREEN_LED_OFF();

    LCD_Clear();

    LCD_GotoXY(0,0);

    LCD_String("LOCKER LOCKED");

    UART1_String("LOCKER LOCKED\r\n");

    delay_ms(1500);
}

/* =================================================
   MAIN PROGRAM
   ================================================= */

int main(void)
{
    /*
       Initialize all peripherals.
    */

    GPIO_Init();

    LCD_Init();

    UART0_Init();

    UART1_Init();

    LOCKER_LOCK();

    GREEN_LED_OFF();

    BUZZER_OFF();

    LCD_Clear();

    LCD_GotoXY(0,0);

    LCD_String("SMART BANK");

    LCD_GotoXY(1,0);

    LCD_String("LOCKER SYSTEM");

    UART1_String("\r\n");
    UART1_String("============================\r\n");
    UART1_String(" SMART BANK LOCKER SYSTEM\r\n");
    UART1_String(" LPC2129 ARM7\r\n");
    UART1_String("============================\r\n");

    delay_ms(2000);

    while(1)
    {
        /* STEP 1: RFID AUTHENTICATION */

        if(RFID_Verify())
        {
            /* STEP 2: PASSWORD AUTHENTICATION */

            if(Password_Verify())
            {
                wrong_attempts = 0;

                /* STEP 3: OPEN LOCKER */

                ACCESS_GRANTED();
            }

            else
            {
                wrong_attempts++;

                LCD_Clear();

                LCD_GotoXY(0,0);

                LCD_String("WRONG PASSWORD");

                LCD_GotoXY(1,0);

                LCD_String("ATTEMPTS: ");

                LCD_Data(wrong_attempts + '0');

                UART1_String("Wrong password\r\n");

                BUZZER_BEEP();

                delay_ms(1500);

                /* STEP 4: THREE ATTEMPT LOCKOUT */

                if(wrong_attempts >= MAX_ATTEMPTS)
                {
                    SECURITY_LOCKOUT();
                }
            }
        }

        delay_ms(200);
    }
}