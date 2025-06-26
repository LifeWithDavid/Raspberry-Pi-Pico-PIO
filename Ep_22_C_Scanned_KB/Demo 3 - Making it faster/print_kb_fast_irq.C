//C code
/* 
PIO keyboard scanning: print scan code

 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "hardware/irq.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "print_kb_fast_irq.pio.h"
#include "hardware/uart.h"
#include "pico/stdlib.h"

static PIO PIO_O;       // pio object
static uint SM;         // pio state machine index
static uint PIO_IRQ;    // NVIC ARM CPU interrupt number


void pioIRQ(){          //This is our callback function
   
   int keyCode = pio_sm_get(PIO_O,SM); // This is the ISR shifted left (LSB last)  
   pio_interrupt_clear(PIO_O, 0);
// This prints the keycode after the interrupt
   int keyRow = keyCode & 0b1111; // isolates the key row 
   int keyCol = keyCode & 0b1111110000; // isolates the key column
   keyCol = keyCol >> 4; // shifts the key column 4 to the right to get rid of trailing zeros
   printf("KB Code: %010b KB Row: %04b  KB Column: %06b \n", keyCode, keyRow, keyCol); //watchdog printout
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

    // enable all PIO IRQs at the same time
	pio_set_irq0_source_mask_enabled(PIO_O, 3840, true); //setting all 4 at once
    
	irq_set_exclusive_handler(PIO_IRQ, pioIRQ);          //Set the handler in the NVIC
    irq_set_enabled(PIO_IRQ, true);                      //enabling the PIO1_IRQ_0
}

int main(void)
{
  stdio_init_all();
  sleep_ms(10000);  //gives me time to start PuTTY
  printf("start program\n");
  kbScanPio(0); // instantiates PIO 0
  uint msg = 0b10001000010001000010001000010001; //special sequencing word
  pio_sm_put(PIO_O, SM, msg); // this sends the special sequencing word
  
  while (1)
  {
   sleep_ms(100);  
   //printf("."); //watchdog printout
  }

  return 0;
}