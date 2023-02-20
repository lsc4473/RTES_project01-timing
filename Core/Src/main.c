#include "stm32l476xx.h"
#include "timer.h"
#include "clock.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <uart.h>

uint8_t message[50];    // message to print to the user
int histogram[101];		// histogram array

///////////////////////////////////////
// Function declarations
///////////////////////////////////////

// runs the power on self-test. Returns true if the test passes, false otherwise
_Bool power_on_self_test( void );

// initializes all variables prior to a measurement.
void init_measurement( uint32_t limit );

// measures timing of 1000 rising edges.
void make_measurements( uint32_t limit );

// print the non-zero bucket times and counts
void print_measurements( uint32_t limit );

// Captures 1 line of text from the console. Returns nul terminated string when \n is entered
void get_line ( uint8_t *buffer, int max_length );

// Parses a line of user input into a new lower limit (unchanged if no response or invalid response)
void get_limit ( uint8_t *buffer, uint32_t *lower_limit );


//////////////////////////////////////////////////////////////
// Embedded code usually consists of 2 components
//  - The init section is run once at startup and initializes all low level drivers and modules
//  - The main loop runs forever and calls the application tasks repeatedly.
////////////////
int main(void) {
    uint32_t lower_limit = 1000 - 50;	// the default lower limit in the problem statement
    uint8_t input;
    uint8_t buffer[20];	     // holds the user response
    int n;

    //////////
    // Initialization executed once at startup
    //////////
    clock_init();
    USART2_Init(115200);
    TIM_Init();

	n = sprintf((char *) message, "Press <CR> to initialize program...\r\n");
	USART_Write(USART2, message, n);

	while(input != (int)'\r' && input != (int)'\n'){
		input = USART_Read(USART2);
	}
	input = 0;

    while(power_on_self_test() == 0);

    //////////
    // Main loop runs forever
    //////////
    while(1)
    {
    	// 1. Print “Enter expected period or <CR> if no change”. Wait for user response
    	//  print(message);    // Consider using use print code from P0-UART
    	n = sprintf((char *) message, "Enter expected period (us) or <CR> if no change: ");
    	USART_Write(USART2, message, n);

    	get_line(buffer, sizeof(buffer));

    	// 2. Set a new expected interval time based on user input
    	get_limit(buffer, &lower_limit);

        // 3. measure 1000 pulses
		init_measurement( lower_limit );
		make_measurements( lower_limit );

        // 4. print time/count for non-zero counts
		print_measurements( lower_limit );

		n = sprintf((char *) message, "Press <CR> to restart program...\r\n");
		USART_Write(USART2, message, n);

		while(input != (int)'\r' && input != (int)'\n'){
			input = USART_Read(USART2);
		}
		input = 0;
    }
}


//////////////////////////////////////////////////////////////
// Function implementation stubs
////////////////


// runs the power on self-test. Returns true if the test passes, false otherwise
_Bool power_on_self_test( void ) {
	int n;
	int pass = 0;
	uint8_t input;

	n = sprintf((char *) message, "Initializing POST...\r\n");
	USART_Write(USART2, message, n);

	/* Enable and Reset Timer */
	TIM2->CNT = 0;
	TIM2->CR1 |= TIM_CR1_CEN;
	n = TIM2->CCR1;

	while(TIM2->CNT < 10000){
		if(TIM2->SR & TIM_SR_CC1IF){
			pass = 1;
		}
	}

	if(pass == 1){
		n = sprintf((char *) message, "POST Successful.\r\n");
		USART_Write(USART2, message, n);
	}else{
		n = sprintf((char *) message, "POST failed. Press <CR> to restart: \r\n");
		USART_Write(USART2, message, n);
		while(input != (int)'\r' && input != (int)'\n'){
			input = USART_Read(USART2);
		}
		input = 0;
	}

	/* Disable Timer */
	TIM2->CR1 &= ~TIM_CR1_CEN;
	TIM2->CNT = 0;

	return pass;
}

// initializes all variables prior to a measurement.
void init_measurement( uint32_t limit ) {
	for(int i = 0; i < (sizeof(histogram) / sizeof(int)); i++){
		histogram[i] = 0;
	}
	return;
}

void make_measurements( uint32_t limit ) {
	int period, n, input;
	int i = 0;

	n = sprintf((char *) message, "Press <CR> to start taking measurements: ");
	USART_Write(USART2, message, n);

	while(input != (int)'\r' && input != (int)'\n'){
		input = USART_Read(USART2);
	}
	input = 0;

	n = sprintf((char *) message, "\r\nMeasuring input signal...\r\n");
	USART_Write(USART2, message, n);

	/* Enable Timer */
	TIM2->CR1 |= TIM_CR1_CEN;

	/* Reset */
	period = TIM2->CCR1;
	while(!(TIM2->SR & TIM_SR_CC1IF));		// Wait for next rising edge
	period = TIM2->CCR1;
	TIM2->CNT = 0;

	/* Take measurements */
	while(i < 1000){
		while(!(TIM2->SR & TIM_SR_CC1IF));
		period = TIM2->CCR1;
		TIM2->CNT = 0;
		if ((period >= limit) && (period <= limit + 100)) {
			histogram[period - limit] += 1;
			i++;
		}
	}

	/* Disable Timer */
	TIM2->CR1 %= ~TIM_CR1_CEN;
	TIM2->CNT = 0;

	return;
}

// print the non-zero bucket times and counts
void print_measurements( uint32_t limit ) {
	int n;

	n = sprintf((char *) message, "\r\n~~~~~Measurement Results~~~~~\r\n");
	USART_Write(USART2, message, n);
	for(int i = 0; i < (sizeof(histogram) / sizeof(int)); i++){
		if(histogram[i] != 0){
			n = sprintf((char *) message, "%d:\t%d\r\n", (int)limit + i, histogram[i]);
			USART_Write(USART2, message, n);
		}
	}
	n = sprintf((char *) message, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\r\n\r\n");
	USART_Write(USART2, message, n);
	return;
}

// Captures 1 line of text from the console. Returns nul terminated string when \n is entered
void get_line ( uint8_t *buffer, int max_length ) {
	uint8_t input;
	int n;
	int i = 0;
	while(1){
		input = USART_Read(USART2);
		if(input == '\r' || input == '\n'){
			n = sprintf((char *) message, "\r\n");
			USART_Write(USART2, message, n);
			break;
		}
		if(i < max_length){
			buffer[i] = input;
			USART_Write(USART2, &input, 1);
		}
		i++;
	}
	return;
}

// Parses a line of user input into a new lower limit (unchanged if no response or invalid response)
void get_limit ( uint8_t *buffer, uint32_t *lower_limit ) {
	int temp = atoi((char *)buffer);
	int n;

	if(temp <= 10000 && temp >= 100) {
		*lower_limit = temp - 50;
	}

	n = sprintf((char *) message, "Period set to: %d microseconds.\r\n", (int)(*lower_limit + 50));
	USART_Write(USART2, message, n);

	return;
}


