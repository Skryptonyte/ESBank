#include <lpc17xx.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// UART registers
#define THRE        (1<<5)
#define MULVAL      1
#define DIVADDVAL   0
#define Ux_FIFO_EN  (1<<0)
#define RX_FIFO_RST (1<<1)
#define TX_FIFO_RST (1<<2)
#define DLAB_BIT    (1<<7)
#define CARRIAGE_RETURN 0x0D

#define LSR_RDR		0x01

// LCD pins

#define EN_PIN 1 << 28
#define RS_PIN 1 << 27
#define DT_PIN 0xF << 23

#define LCD_COMMAND 0
#define LCD_DATA 1

char display_msg[256];
int lcd_init_comm[] = {0x3,0x3,0x3,0x2,0x28,0x01,0x06,0xC,0x80};

int login = -1;
struct user
{
	char name[16];
	double balance;
	char passcode[5];
	char phno[20];
};

// Hardcoded database of users

// Phone numbers censored for privacy, replace with your own
struct user users[] =
{
	{"Rayhan",10000000.00,"1234","+YYXXXXXXXXXX"},
	{"Rohan", 69.00, "5678","+YYXXXXXXXXXX"},
	{"Adarsh",678.00,"5721","+YYXXXXXXXXXX"},
	{"Pratyush",1000.00,"1111","+YYXXXXXXXXXX"}
};
/*
	*****************************************************************
	* LCD Driver 																							      *
	*****************************************************************
*/
void lcd_write(int byte, int rs);
void port_write(int nibble, int rs);
void delay_lcd(int iters);

void default_input_option(int function);
// Initialize LCD using commands in lcd_init_comm array
void lcd_init(void)
{
	
	int i = 0;
	
	LPC_PINCON->PINSEL1 = 0;
	LPC_PINCON->PINSEL0 = 0;
	LPC_GPIO0->FIODIR |= 0xFF << 4;
	LPC_GPIO0->FIODIR |= EN_PIN | RS_PIN | DT_PIN;
	LPC_GPIO0->FIOCLR = DT_PIN | EN_PIN | RS_PIN;
	for(i=0;i<1000000;i++);

	for (i = 0; i < 9; i++)
	{
		lcd_write(lcd_init_comm[i],LCD_COMMAND);
		delay_lcd(50000);
	}
	return;
}
// Write full 8 bit command to LCD using two port writes
void lcd_write(int byte, int rs)
{
	int mode = 0;

	if (rs == 1 && (byte == 0x30 || byte == 0x20))
		mode = 1;
	
	port_write((byte & 0xf0) >> 4, rs);
	if (1)
	{
		port_write((byte & 0xf), rs);
	}
	

}
// Low level function to write single 4 bit data to LCD
void port_write(int nibble, int rs)
{
	LPC_GPIO0->FIOCLR = DT_PIN | EN_PIN | RS_PIN;
	//  Pass 4 bit data to data pins
	LPC_GPIO0->FIOSET = nibble << 23;
	
	// Set register pin
	if (rs == LCD_DATA)
		LPC_GPIO0->FIOSET = RS_PIN;
	else
		LPC_GPIO0->FIOCLR = RS_PIN;
	

	// Trigger enable pin on LCD
	LPC_GPIO0->FIOSET = EN_PIN;
	delay_lcd(50);
	LPC_GPIO0->FIOCLR = EN_PIN;
	delay_lcd(3000);

}
// Helper function to output full string on specific line of LCD
void lcd_print(char* s,int line)
{
	int i = 0;

	if (line == 1)
		lcd_write(0x80, 0);
	else
		lcd_write(0xc0, 0);
	while (s[i] != '\x00' && i < 16)
	{	
		lcd_write(s[i], LCD_DATA);
		i++;
	}
	// Fill rest of line with space
	while (i < 16)
	{
		lcd_write(' ', LCD_DATA);
		i++;
	}

}
// Delay LCD helper function
void delay_lcd(int iters)
{
	int i = 0;
	for (i = 0; i < iters; i++);
	
	return;
}
/*
	*****************************************************************
	* UART                    initializations and send function     *
	*****************************************************************
*/

// UART helper function to write single character to UART lines
void UART1_Write(char txData)
{
	while(!(LPC_UART1->LSR & THRE));
		LPC_UART1->THR = txData;
}
// Helper function to write entire string to UART lines.
void UART1_puts(char* data, int len)
{
		int i = 0;

		LPC_GPIO0->FIOSET = 1 << 4;

	while (i < len)
	{
		UART1_Write(data[i]);
		i += 1;
	}
		LPC_GPIO0->FIOCLR = 1 << 4;

}

// Initialize UART1. Assuming PCLK=25MHz, set DLL to 162 to get baud rate of 9645, which is very close to 9600
// Slight deviation of baud rate is fine as long as it is within 2.5-5% deviation
void UART1_Init(void)
{
	LPC_PINCON->PINSEL0 |= 1 << (15*2); // P0.15 as TXD1;
	LPC_PINCON->PINSEL1 |= 1; // P0.16 as RXD1


	LPC_UART1->LCR = 0x83  ; // 8 data bits, 1 stop bit and no parity
	LPC_UART1->DLL = 162;  // DLL = 162 -> 9645 baud
	LPC_UART1->DLM = 0;
	LPC_UART1->FCR |= Ux_FIFO_EN | RX_FIFO_RST | TX_FIFO_RST;
	LPC_UART1->FDR = (MULVAL<<4) | DIVADDVAL;
	LPC_UART1->LCR = 0x3  ;


}

// PCLK = 25MHz, set PR to 24999 to count milliseconds
// Millsecond precision delay
void timer_delay(int ms)
{
	LPC_TIM0->TCR = 0x2;
	LPC_TIM0->PR = 24999;
	LPC_TIM0->MR0 = ms;
	LPC_TIM0->EMR = 0x20;
	LPC_TIM0->MCR = 1 << 2;
	
	LPC_TIM0->TCR = 0x1;
	while (!(LPC_TIM0->EMR & 0x1));
}
//***************************
//* SMS functions
//* Interfacing with the SIM900 module via UART
//****************************

// Helper function to send SMS to number of logged in account
// TODO: Use actual timer delays here, I only used these loops for testing.
void sms_send(char* message)
{
	int i = 0;
	char test[32] = "AT+CMGS=\"+YYXXXXXXXXXX\"\r\n";
	char end[] = "\x1a";
	
 // Enter send mode on SIM900A with logged in phone number as recipient
	sprintf(test,"AT+CMGS=\"%s\"\r\n",users[login].phno);
		UART1_puts(test,strlen(test));
	for (i = 0; i < 10000000; i++);

		UART1_puts(message,strlen(message));
	for (i = 0; i < 10000000; i++);

// Use \x1a to signal end of message
		UART1_puts(end,1);
	for (i = 0; i < 300000000; i++);
}

// Initialize SIM900A by setting to SMS text mode and activating SMS live mode
void sms_init()
{
	int i =0;
		char initcomm[] = "AT+CMGF=1\r\n";
		char recv_active[] = "AT+CNMI=2,2,0,0,0\r\n";
		UART1_puts(initcomm,strlen(initcomm));
	for (i = 0; i < 10000000; i++);
	
			UART1_puts(recv_active,strlen(recv_active));
	for (i = 0; i < 10000000; i++);
}

/*
	*****************************************************************
	* Menu functions                                                *
	*****************************************************************
*/
// Fallback function for testing general keyboard input and SMS sending, unused function
void fallback_input(int function)
{
		char lcd_message_fn[32], msg[64];
					sprintf(lcd_message_fn,"SMS function: %d",function);
				lcd_print(lcd_message_fn,1);
				sprintf(msg,"Message function: %d",function);
				sms_send(msg);
				lcd_print("READY",1);
}

// Waits for SMS message and compares content to pincodes in database for matching account.
int verify_fn()
{
		char msg[64], len = 0, cmti = 0;
		char c;
		int i = 0;
		lcd_print("-----LOGIN------",1);
		lcd_print("SMS secret pin",2);
	 
		while (1)
		{
			while (!(LPC_UART1->LSR & LSR_RDR));
			
			c = LPC_UART1->RBR;
			msg[len++] = c;
			if (c == '\n')
			{
				
				if (cmti) // If on line after CMTI, we have received message.
				{
					msg[len] = 0;
					break;
				} // If first line is +CMT, this indicates SMS message. Set flag to receive next line which contains actual contents
				else if (msg[0] == '+' && msg[1] == 'C' && msg[2] == 'M' && msg[3] == 'T')
				{msg[len] = 0;
					len = 0;
					cmti = 1;
					}
				else
					len = 0;
			}
		}
		msg[len-1] = 0;
		msg[len-2] = 0;
		
		for (i = 0; i < 4; i++)
		{
				if (!strcmp(msg,users[i].passcode)) // Check if code and database matches
				{
					lcd_print("Welcome back",1);
					lcd_print(users[i].name,2);
					login = i;
					timer_delay(3000);
					return 1;
				}
		}
		lcd_print("Invalid code!",1);
		return 0;
}

// Helper function to display main menu
void display_menu()
{
	lcd_print("1. Balance",1);
	lcd_print("2. Withdraw",2);
}

// Display balance through both SMS and LCD
void display_balance()
{
	char msg[64];
	lcd_print("Balance:",1);
	sprintf(msg,"Rs. %.2f",users[login].balance);
	lcd_print(msg,2);
	
	sprintf(msg,"Hello %s, your balance is %.2f rupees.",users[login].name,users[login].balance);
	sms_send(msg);
	timer_delay(1000);
}

// Perform withdrawl of money from logged in account, report results via SMS
void withdraw_balance()
{
	
	int row,x,col,num,balancelen=0;
	char balance[6] = {0,0,0,0,0,0};
	char message[64];
	int balance_num;
	
	lcd_print("Enter balance:",1);

	// Start keyboard loop to receive input
	while (1)
	{
		for (row = 0; row < 4; row++)
		{
			LPC_GPIO2->FIOPIN = 1 << (row+10);
			
			
			x = LPC_GPIO1->FIOPIN & ( 0xf << 23);
			col = -1;
			switch (x)
			{
				case 1 << 23:
					col = 0;
					break;
				case 1 << 24:
					col = 1;
					break;
				case 1 << 25:
					col = 2;
					break;
				case 1 << 26:
					col = 3;
					break;
				default:
					col = -1;
			}
			
			
			num = ((row+3)%4)*4 + col;
			if (col != -1)
			{
				// If number between 0 and 9, it is numeric input so append to displayed amount on LCD
				if ((num >= 0 && num <= 9) && balancelen < 6)
				{
					balance[balancelen++] = 0x30 + num;
					lcd_print(balance,2);
				}
				if (num == 10)  // Act as backspace to delete digit
				{
					
					if (balancelen > 0)
						balance[--balancelen] = 0;
					lcd_print(balance,2);
				}    // Act as enter button to submit amount to withdraw
				if (num == 11)
				{
					balance_num = strtol(balance,NULL,10);
					// If amount greater than balance, then report insufficient balance
					if (balance_num > users[login].balance)
					{
						lcd_print("Insufficient Balance!",1);
						lcd_print("",2);
						timer_delay(3000);
					}
					else // Subtract amount from balance of logged in account and report via SMS
					{
						users[login].balance -= balance_num;
						sprintf(message,"Your account has been charged Rs. %d. Current balance: %.2f!",balance_num,users[login].balance);
						lcd_print("Transaction",1);
						lcd_print("in progress!",2);
						sms_send(message);
						lcd_print("Account",1);
						lcd_print("debitted!",2);
						timer_delay(6000);
					}
					return;
				}
				timer_delay(200);
			}

		}
	}
}

// Driver main function
int main(void)
{
	int row = 0;
	int function = 0;
	int i = 0,x,col = -1;

	SystemInit();
	SystemCoreClockUpdate();
	lcd_init();

    // Set output for pins 2.10 - 2.13, used as keyboard rows
	LPC_GPIO2->FIODIR = 0xf << 10;

	UART1_Init();

// Initial startup screen
	lcd_print("-----ESBANK-----",1);
	lcd_print("SMS secured ATM",2);
	
// Initialize SMS
	sms_init();
	// Wait for SMS code and verify login
	verify_fn();
	if (login == -1)
	{
		while (1);
	}
	display_menu();

// Start keyboard input loop
		while (1)
		{
			for (row = 0; row < 4; row++)
			{
				LPC_GPIO2->FIOPIN = 1 << (row+10);
				
				// Keyboard scan logic
				x = LPC_GPIO1->FIOPIN & ( 0xf << 23);
				col = -1;
				switch (x)
				{
					case 1 << 23:
						col = 0;
						break;
					case 1 << 24:
						col = 1;
						break;
					case 1 << 25:
						col = 2;
						break;
					case 1 << 26:
						col = 3;
						break;
					default:
						col = -1;
				}
				
				// Compute function number using selected row and column
				function = ((row+3)%4)*4 + col;

				if (col != -1)
					{
						// If function is 0, do display balance
							if (function == 0)
							{
								display_balance();
								display_menu();
							}
							else if (function == 1) // If function is 1, do withdraw balance menu
							{
								withdraw_balance();
								display_menu();
							}
					}
			}
		}

	
}