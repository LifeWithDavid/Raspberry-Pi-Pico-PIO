//C code
/* 
PIO keyboard scanning: print scan code
This prints the key value using the UART0 to USB convertor
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include "hardware/irq.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "kb_scan_decode.pio.h"
#include "hardware/uart.h"
#include "pico/stdlib.h"

static PIO PIO_O;       // pio object
static uint SM;         // pio state machine index
static uint PIO_IRQ;    // NVIC ARM CPU interrupt number
volatile uint kbRow;    // Global data to pass kb row decimal value to main program
volatile uint kbCol;    // Global data to pass kb column decimal value to main program
static uint keyDecode[4][6] = {{51,50,49,48,34,33},{55,54,53,52,36,35},{66,65,57,56,38,37},{70,69,68,67,40,39}};
/*
#define UART_ID uart0
#define BAUD_RATE 115200
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY UART_PARITY_NONE
#define UART_TX_PIN 1
#define UART_RX_PIN 2 
	
void setup_uart(){
   uart_init(UART_ID, BAUD_RATE);
   gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
   gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
   uart_set_hw_flow(UART_ID, false, false);
   uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);
   }

void send_data(const uint8_t *src, size_t len) {  // not really needed if you only use printf()
   uart_write_blocking(UART_ID, src, len);
   }
 */  
uint decodeKeys(uint row, uint col)  {   //This takes the row and column and returns an ASCII Character
	char keyChar = (char)keyDecode[row][col];
	return keyChar;
	}
	
uint convertBit2Dec(uint bitwiseData) { //Converts the bit position number to decimal (bit 0=0, ie. 0b0010->0d1)
	int decimalData;
	for (decimalData = 0; decimalData < 6; ++decimalData) { 
		if ((bitwiseData & (1 << decimalData)) != 0 ) // keep shifting 1 to the left until it finds the set bit position
			break;  //Set bit found, quit the loop
		else;		// Set bit not found, move 1 over an additionsl space and check again
	}
	return decimalData;  // return the bit position decimal number
}   

void pioIRQ(){          //This is our callback function
//   int keyRow = 1;
//   int keyCol = 0;
   uint keyCode = pio_sm_get(PIO_O,SM); // This is the ISR shifted left (LSB last)  
   pio_interrupt_clear(PIO_O, 0);
// This prints the keycode after the interrupt
   uint keyRowBit = keyCode & 0b1111; // isolates the key row 
   uint keyColBit = keyCode & 0b1111110000; // isolates the key column
   keyColBit = keyColBit >> 4; // shifts the key column 4 to the right to get rid of trailing zeros
   uint keyRow = convertBit2Dec(keyRowBit); //converts row bit position to decimal row number (start at 0)
   uint keyCol = convertBit2Dec(keyColBit); // converts column bit position to decimal column number (start at 0)
   char keyAscii = decodeKeys(keyRow,keyCol); // this outputs an ascii character based on the row and column
   printf("KB Code: %010b KB Row: %1i  KB Column: %1i ASCII: %c \n", keyCode, keyRow, keyCol,keyAscii); 
   
}
	
void kbScanPio(uint pioNum) {
    PIO_O = pioNum ? pio1 : pio0; //Selects the pio instance (0 or 1 for pioNUM)
    PIO_IRQ = pioNum ? PIO1_IRQ_0 : PIO0_IRQ_0;  // Selects the NVIC PIO_IRQ to use
		
    // Our assembled program needs to be loaded into this PIO's instruction
    // memory. This SDK function will find a location (offset) in the
    // instruction memory where there is enough space for our program. We need
    // to remember this location!
    uint offset = pio_add_program(PIO_O, &pio_kb_scan_irq_program); 
	

	uint ROW_START = 12; // the start of the row GPIOs (4 outputs)
	uint COL_START = 6;  // the start of the column GPIOs (6 inputs)
	
	// select the desired state machine clock frequency (2000 is about the lower limit)
	float SM_CLK_FREQ = 2000;

    // Find a free state machine on our chosen PIO (erroring if there are
    // none). Configure it to run our program, and start it, using the
    // helper function we included in our .pio file.
    SM = pio_claim_unused_sm(PIO_O, true);
    pio_kb_scan_irq_program_init(PIO_O, SM, offset, ROW_START, COL_START, SM_CLK_FREQ);	
	// this defines and enables the PIO IRQ handler
 //Enable one IRQ at a time
	//enables IRQ for the statemachine - setting IRQ0_INTE - interrupt enable register
    //pio_set_irq0_source_enabled(PIO_O, pis_interrupt0, true); // sets IRQ0
    //pio_set_irq0_source_enabled(PIO_O, pis_interrupt1, true); // sets IRQ1
	//pio_set_irq0_source_enabled(PIO_O, pis_interrupt2, true); // sets IRQ2
	//pio_set_irq0_source_enabled(PIO_O, pis_interrupt3, true); // sets IRQ3
 //*********or*******************************************************************
    // enable all PIO IRQs at the same time
	pio_set_irq0_source_mask_enabled(PIO_O, 3840, true); //setting all 4 at once
    
	irq_set_exclusive_handler(PIO_IRQ, pioIRQ);          //Set the handler in the NVIC
    irq_set_enabled(PIO_IRQ, true);                      //enabling the PIO1_IRQ_0

	
}



int main(void)
{
  stdio_init_all();
  sleep_ms(20000);  //gives me time to start PuTTY

  /*
  setup_uart();
  char message[] = "Hello, UART!\n";
  send_data((const uint8_t*)message, strlen(message));
  */
  printf("start program\n");
  kbScanPio(0); // instantiates PIO 0
  uint msg = 0b10001000010001000010001000010001; //special sequencing word
  pio_sm_put(PIO_O, SM, msg); // this sends the special sequencing word
  
  while (1)
  {
   sleep_ms(100);  

  }

  return 0;
}