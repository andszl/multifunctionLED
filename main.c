/*
 * projekt.c
 *
 * Created: 08.08.2020 19:20:30
 * Author : Andrzej Szalas
 */ 

#define F_CPU 16000000UL
#define LED_PIN 6
#define BUTTON1_PIN 2	
#define BUTTON2_PIN 3
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <math.h>
#include "LCD_HD44780_IO.h"
#define BAUD 9600
#define MYUBRR  ((F_CPU/16/BAUD)-1)
#define NR_of_commands 7

short stan=0;
bool pot=1;
unsigned short int adc_val=0;
bool change_flag=0;

#define BUFER_SIZE 16

char RX_bufer[BUFER_SIZE];
unsigned int RX_write_pos=0;
unsigned value_rx=0;
bool bufer_overflow=0;

char *msg[NR_of_commands];
char *com[3];



void PWM_disable()
	{
		TCCR0A = 0x00;
		TCCR0B = 0x00;
		TIMSK0 = 0x00;
	}

void blink_setup()
	{
		PWM_enable();
		TIMSK0 = 0x01;
	}

void blink_disable()
	{
		TIMSK0 = 0;
		PWM_disable();
	}
	
void PWM_enable()
	{
		TCCR0A |= 0x83;
		TCCR0B |= 0x82;
	}

void LED_flash_enable()
	{
		TCCR0A |= 0x00;
		TCCR0B |= 0x03;
		TIMSK0 = 0x01;
	}

void mod_change()
	{
		switch (stan)
		{
		case 0:
		PWM_disable();
		_delay_ms(5);
		LED_flash_enable();
			break;
		case 1:
		PWM_disable();
		_delay_ms(5);
		blink_setup();
			break;
		case 2:
		PWM_disable();
		_delay_ms(5);
		blink_setup();
			break;
		}
		
		change_flag=1;
		
	}
	
void val_prep(char* buf)
	{
		int i=0;
		while(buf[i]!=0) i++;
		if(i>=4)
		{
			for(int j=i; j>0; j--)
			{
				buf[j+1]=buf[j];
			}
			buf[i]=0;
			buf[1]='.';
		
		}
		else
		{
			for(int j=i; j>=0; j--)
			{
				buf[j+1]=buf[j];
			}
			buf[0]='0';
			i++;
			for(int j=i; j>0; j--)
			{
				buf[j+1]=buf[j];
			}
			buf[1]='.';
			buf[4]=0;
		}
	
	}	

void USART_send(char data)
	{
		while ( !( UCSR0A & (1<<5)) );
		UDR0 = data;
	}
void USART_enable()
	{
		UBRR0H = (unsigned char)(MYUBRR>>8);
		UBRR0L = (unsigned char)MYUBRR;
		
		UCSR0B |= 0x98;
		UCSR0C |= 0x1E;
	}
void USART_string(char *data)
	{
		for(int i=0; *(data+i)!=0; i++)
			{
				USART_send(*(data+i));
			}
	}


SIGNAL(USART_RX_vect)
{
	/*odczyt z USART do bufora*/
	
		volatile char dat  = UDR0;
		if(RX_write_pos<BUFER_SIZE)
			{
				RX_bufer[RX_write_pos]=dat;
				RX_bufer[RX_write_pos+1]=0;
				RX_write_pos++;
			}
		else
			{
				USART_string("bufer overflow\n");
				RX_write_pos=0;
				bufer_overflow=1;
			}
			
			
	/* interpretacja komend*/
	
		if(dat=='\n' || dat=='\r')
		{
			RX_bufer[RX_write_pos-1]=0;
			
			if(bufer_overflow==1)
				{
					bufer_overflow=0;
					RX_write_pos=0;
					return;
				}
		
	/* wczytywanie i zmiana parametru */
		
			if(value_rx==1)
			{
				RX_write_pos=0;
				value_rx=0;
				float temp_f;
				int temp_i;
	
				switch (stan)
				{
				case 0:
					temp_f=atof(RX_bufer);
					temp_f=temp_f*1000;
					if(temp_f<100 || temp_f>2000)
					{
						USART_string("incorrect value\n");
					}
					else
					{
						adc_val=ceil((temp_f-100.352)/(1.8571264));
						USART_string("Value: ");
						USART_string(RX_bufer);
						USART_send('s');
						USART_send('\n');
						pot=0;
					}
					break;
				case 1:
							temp_i=atoi(RX_bufer);
							if(temp_i<0 || temp_i>100)
							{
								USART_string("incorrect value\n");
							}
							else
							{
								OCR0A=(int)(temp_i*2.55);
			
								adc_val=(int)((temp_i*2.55)/0.2492);
								
								USART_string("Value: ");
								USART_string(RX_bufer);
								USART_send('%');
								USART_send('\n');
								pot=0;
							}
						break;
				case 2:
					temp_f=atof(RX_bufer);
					temp_f=temp_f*1000;
						if(temp_f<1000 || temp_f>4000)
							{
								USART_string("incorrect value\n");
							}
						else
							{
								adc_val=ceil((temp_f-1000.07936)/(2.932736));
								USART_string("Value: ");
								USART_string(RX_bufer);
								USART_send('s');
								USART_send('\n');
								pot=0;
							}
					break;
				}
			}
			
			else
			
			{
				
					unsigned i=0;
					for(i=0; i<NR_of_commands; i++)
					{
						if(!strcmp(*(msg+i), RX_bufer)) break;
					}
					
					if(i==NR_of_commands)
					{
						for(i=1; i<=NR_of_commands; i++)
							{
								char nr[1];
								nr[0]=i+48;
								nr[1]=0;
								if(!strcmp(nr, RX_bufer)) break;
							}
							i--;
						if((RX_bufer[0]=='i' || RX_bufer[0]=='I')&& RX_bufer[1]==0) i=6;
					}
					
					if(i<3)
						{
							stan=i;
							mod_change();
							change_flag=1;
							USART_string(msg[stan]);
							USART_send('\n');
						}	
					else
						{
							char 
							bu_i[6];
							int z;
							
							switch (i)
							{
							case 3:
							
								/* sprawdzanie stanu */
								
								USART_string(msg[stan]);
									switch (stan)
									{
									case 0:
									
										z=(int)floor(2*1.024*((0.9068*adc_val)+49));
										itoa(z, bu_i, 10);
										val_prep(bu_i);
										USART_string("   ");
										USART_string(bu_i);
										USART_string(" s");
										USART_send('\n');
										break;
									case 1:
																				
										itoa((int)(ceil(OCR0A/2.56)), bu_i, 10);
										USART_string("   ");
										USART_string(bu_i);
										USART_string(" %");
										USART_send('\n');
										break;
									case 2:
									
										z=(int)floor(65.536*((0.04475*adc_val)+15.26));
										itoa(z, bu_i, 10);
										val_prep(bu_i);
										USART_string("   ");
										USART_string(bu_i);
										USART_string(" s");
										USART_send('\n');
										break;
									default:
										break;
									}
						
									change_flag=1;
								break;
															
							case 4:
							
									USART_string(com[stan]);
									USART_send('\n');
									value_rx=1;
								break;
								
							case 5:
							
								pot=1;
								change_flag=1;
								break;
								
							case 6:
							
								USART_string("comands:\n");
								for(int j=1; j<=NR_of_commands; j++)
								{
								 USART_send((j+48));
								 USART_send(' ');
								 USART_string(msg[j-1]);
								 USART_send('\n');
								}
								break;
							
							default:
							
								USART_string("Unknown command: ");
								USART_string(RX_bufer);
								USART_send('\n');
								break;
							}
							
						}	
			}
			
			RX_write_pos=0;
			
		}
	
}

int adc_prev=0;
SIGNAL (ADC_vect)
	{
		if(pot==1) adc_val=ADC;
	}

unsigned pre = 0;
bool up_down=0;

SIGNAL (TIMER0_OVF_vect)
	{
		pre++;	
			switch (stan)
			{
			case 0:
				 if(pre>=((int)((0.9062*adc_val)+49))) 
					{
						PORTD ^= (1<<LED_PIN);
						pre=0;
					}
				break;
			case 1:
				if(pot==1) OCR0A = (int)(0.2492*adc_val);
				break;
			case 2:
				if(pre >=((int)((0.0449*adc_val)+15)))
					{
						pre = 0;
						if(up_down==0) OCR0A++;
						else OCR0A--;
						if(OCR0A >= 255) up_down=1;
						if(OCR0A <=0) up_down=0;
					}
				break;
			}
	}
	
void ADC_setup()
	{
		ADMUX = 0x40;
		ADCSRA = 0xEF;
	}

SIGNAL (INT0_vect)
	{
		if(!(PIND & (1<<BUTTON1_PIN)))
		{
			stan--;
			while(!(PIND & (1<<BUTTON1_PIN)));
		}
		if(stan<0) stan = 2;
		if(stan>2) stan = 0;
		pot=1;
		USART_string(msg[stan]);
		USART_send('\n');
		
		mod_change();
		
	}
SIGNAL (INT1_vect)
	{
		if(!(PIND & (1<<BUTTON2_PIN)))
		{
			stan++;
			while(!(PIND & (1<<BUTTON2_PIN)));
		}
		if(stan<0) stan = 2;
		if(stan>2) stan = 0;
		pot=1;
		USART_string(msg[stan]);
		USART_send('\n');
		
		mod_change();
		
		
	}
unsigned short int length_prev=0;

void LCD_refresh()
{
	if(change_flag==1)
	{
		LCDclr();
		_delay_ms(5);
		LCDGotoXY(0,0);
		_delay_ms(5);
		LCDstring(msg[stan], strlen(msg[stan]));
		_delay_ms(5);
		
		if(stan==0 || stan==2)
		{
			LCDGotoXY(4,1);
			_delay_ms(1);
			LCDstring("s", 1);
			_delay_ms(1);
		}
		change_flag=0;
	}
	
	
	LCDGotoXY(0,1);
	_delay_ms(10);
	char buf[5];
	int z=0;
	switch (stan)
	{
		case 0:
		z=(int)floor(2*1.024*((0.9068*adc_val)+49));

		itoa(z, buf, 10);
		val_prep(buf);
		LCDstring(buf, 4);
		break;
		case 1:
		itoa((int)(ceil(OCR0A/2.56)), buf, 10);
		if((length_prev==3 && strlen(buf)==2) || (length_prev==2 && strlen(buf)==1) || (length_prev==3 && strlen(buf)==1))
		{
			_delay_ms(5);
			LCDstring("     ", 5);
		}
		length_prev=strlen(buf);
		_delay_ms(5);
		LCDGotoXY(0, 1);
		_delay_ms(5);
		LCDstring(buf, strlen(buf));
		LCDGotoXY(strlen(buf),1);
		_delay_ms(5);
		LCDstring("%", 1);;
		break;
		case 2:
		z=(int)floor(65.536*((0.04475*adc_val)+15.26));
		itoa(z, buf, 10);
		val_prep(buf);
		LCDstring(buf, 4);
		break;
		default:
		break;
	}
	_delay_ms(25);
	if(pot==1)
	{
		LCDGotoXY(12,1);
		_delay_ms(1);
		LCDstring("POT", 3);
	}
	else
	{
		LCDGotoXY(10,1);
		_delay_ms(1);
		LCDstring("USART", 5);
	}
}

	
int main(void)
{	
	
	adc_prev=adc_val;
	unsigned count=0;
	
	/*Ustawianie tablicy z polece�*/
	
	char msg_1[]="LED flash";
	char msg_2[]="LED modulation";
	char msg_3[]="LED blink";
	char msg_4[]="state";
	char msg_5[]="change";
	char msg_6[]="potentiometer on";
	char msg_7[]="info";

	char com_1[]="Insert value from 2s to 0.1s";
	char com_2[]="Insert value from 0% to 100%";
	char com_3[]="Insert value from 4s to 1s";
	
	msg[0]=msg_1;
	msg[1]=msg_2;
	msg[2]=msg_3;
	msg[3]=msg_4;
	msg[4]=msg_5;
	msg[5]=msg_6;
	msg[6]=msg_7;
	
	com[0]=com_1;
	com[1]=com_2;
	com[2]=com_3;
	
	/*ustawianie wyj��ia LED i wej�� przycisk�w*/
	
	DDRD |= (1<<LED_PIN);
	DDRD &= ~(1<<BUTTON1_PIN);
	DDRD &= ~(1<<BUTTON2_PIN);
	
	EIMSK |= 0x03; //w��czenie przewania zewn�trznego na wej�ciach PD2 i PD3
		
		ADC_setup();
		USART_enable();
		sei();
	
		LCDinit();
		LCDhome();
	
		mod_change();
		LCD_refresh();
    
	while (1) 
	{
		count++;
		
		if(count==10000)
			{
					
				if(adc_val<adc_prev-3 || adc_val>adc_prev+3 || change_flag==1)
				{
					adc_prev=adc_val;
					LCD_refresh();
					count=0;
				}
				
			}
		
		
	}
	
}

