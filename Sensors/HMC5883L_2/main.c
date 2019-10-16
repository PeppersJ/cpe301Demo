/*    
    Copyright (C) 2019 Elia Ritterbusch 

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <math.h>
#include <stdlib.h>

#include "i2c_master.h"

#define HMC5883L_WRITE 0x1E
#define HMC5883L_READ 0x1F 

void init_uart(uint16_t baudrate){

	uint16_t UBRR_val = (F_CPU/16)/(baudrate-1);

	UBRR0H = UBRR_val >> 8;
	UBRR0L = UBRR_val;

	UCSR0B |= (1<<TXEN0) | (1<<RXEN0) | (1<<RXCIE0); // UART TX (Transmit - senden) einschalten
	UCSR0C |= (1<<USBS0) | (3<<UCSZ00); //Modus Asynchron 8N1 (8 Datenbits, No Parity, 1 Stopbit)
}

void uart_putc(unsigned char c){

	while(!(UCSR0A & (1<<UDRE0))); // wait until sending is possible
	UDR0 = c; // output character saved in c
}

void uart_puts(char *s){
	while(*s){
		uart_putc(*s);
		s++;
	}
}

void USART_tx_string( char *data )
{
	while ((*data != '\0'))
	{
		while (!(UCSR0A & (1 <<UDRE0)));
		UDR0 = *data;
		data++;
	}
}
char buffer[6];

int16_t raw_x = 0;
int16_t raw_y = 0;
int16_t raw_z = 0;
float headingDegrees = 0;

void init_HMC5883L(void){

	i2c_start(HMC5883L_WRITE);
	i2c_write(0x00); // set pointer to CRA
	i2c_write(0x70); // write 0x70 to CRA
	i2c_stop();

	i2c_start(HMC5883L_WRITE);
	i2c_write(0x01); // set pointer to CRB
	i2c_write(0xA0);
	i2c_stop();

	i2c_start(HMC5883L_WRITE);
	i2c_write(0x02); // set pointer to measurement mode
	i2c_write(0x00); // continous measurement
	i2c_stop();
}

float getHeading(void){

	i2c_start(HMC5883L_WRITE);
	i2c_write(0x03); // set pointer to X axis MSB
	i2c_stop();

	i2c_start(HMC5883L_READ);

	raw_x = ((uint8_t)i2c_read_ack())<<8;
	raw_x |= i2c_read_ack();

	raw_z = ((uint8_t)i2c_read_ack())<<8;
	raw_z |= i2c_read_ack();

	raw_y = ((uint8_t)i2c_read_ack())<<8;
	raw_y |= i2c_read_nack();

	i2c_stop();

	headingDegrees = atan2((double)raw_y,(double)raw_x) * 180 / 3.141592654 + 180;

	return headingDegrees;
}

int main(void){
	
	init_uart(9600);
	USART_tx_string("Connected!\r\n");    // we're alive!
	_delay_ms(125);         // wait a bit
	i2c_init();
	init_HMC5883L();
	
	while(1){
		getHeading();
		
		itoa(raw_x, buffer, 10);
		uart_puts(buffer);
		uart_puts("  ");
		
		itoa(raw_y, buffer, 10);
		uart_puts(buffer);
		uart_puts("  ");
		
		itoa(raw_z, buffer, 10);
		uart_puts(buffer);
		uart_puts("  ");
		
		dtostrf(headingDegrees, 6, 2, buffer);
		uart_puts(buffer);
		uart_puts("  \r\n");
		
		_delay_ms(1000);
	}
	
	return 0;
}
