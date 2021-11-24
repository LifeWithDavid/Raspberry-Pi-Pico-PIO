/* Life with David, PIO Episode 9
   Uses PWM and DMA to step through 8 LEDs (GPIO 0-7) slowly
   brightening and then fading, one at a time.  After the 
   sequence is running, NO processor time is used.
*/

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/pwm.h"

int main()
{
	uint led_pwm_slice_num[8]; //set up to write to 8 different LEDs
	for (int k = 0; k<8; ++k)  {
		gpio_set_function(k, GPIO_FUNC_PWM);
		led_pwm_slice_num[k] = pwm_gpio_to_slice_num(k);
	}

    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 8.f);
    pwm_init(0, &config, true);
	pwm_init(1, &config, true);
	pwm_init(2, &config, true);
	pwm_init(3, &config, true);
		
	// fade_dma_chan_a loads increasing/ decreasing brightness values into PWM channel a(0)
	// fade_dma_chan_b loads increasing/ decreasing brightness values into PWM channel b(1)
	int fade_dma_chan_a = dma_claim_unused_channel(true);
    int fade_dma_chan_b = dma_claim_unused_channel(true);
	// control_dma_chan_a writes to fade_dma_chan_a write pointer register
	// control_dma_chan_b writes to fade_dma_chan_b write pointer register
	int control_dma_chan_a = dma_claim_unused_channel(true);
	int control_dma_chan_b = dma_claim_unused_channel(true);
	
	// declare the two wave tables for fade and fadeoff
    uint32_t fade_a[512] __attribute__((aligned(2048))); 
	uint32_t fade_b[512] __attribute__((aligned(2048)));

	for (int i = 0; i <= 256; ++i) {
		/* 	We need a value from 0 to (2^16 - 1) and back to 0, i ranges from 0 - 256. Squaring 
			here gives us almost the full range of values whilst provided some gamma correction 
			to give a more linear fade effect.  Fill the first and last elements of the array 
			first with 0 and then move toward the middle with higher values.
		*/
		fade_a[i] = (i * i); 			//Loads the 16 LSBs of the first half with increasing values
		fade_a[512-i] = (i * i);  		//Loads the 16 LSBs of the second half with decreasing values
		fade_b[i] = (i * i) << 16;  	//Loads the 16 MSBs of the first half with increasing values
		fade_b[512-i] = (i * i) << 16;  //Loads the 16 MSBs of the second half with decreasing values
	}
	
	/* 	Now load up the array for the control DMA channel; write to the .cc register of the PWM slice 
		These addresses are listed in section 4.5.3 of the RP2040 datasheet
	*/
	uint32_t pwm_set_level_locations[4];
	pwm_set_level_locations[0] = 0x4005000c; //CH0_CC pwm register
	pwm_set_level_locations[1] = 0x40050020; //CH1_CC pwm register
	pwm_set_level_locations[2] = 0x40050034; //CH2_CC pwm register
	pwm_set_level_locations[3] = 0x40050048; //CH3_CC pwm register

	// Setup the fade_a DMA channel, start with the default configuration structure
    dma_channel_config fade_dma_chan_a_config = dma_channel_get_default_config(fade_dma_chan_a);
	// Transfer 32-bits at a time
    channel_config_set_transfer_data_size(&fade_dma_chan_a_config, DMA_SIZE_32);
	// Increment read address so we pick up a new fade value each time
    channel_config_set_read_increment(&fade_dma_chan_a_config, true);
	// don't increment write address so we always transfer to the same PWM register
    channel_config_set_write_increment(&fade_dma_chan_a_config, false);
	// After block has been transferred, start control_dma_chan_b
	channel_config_set_chain_to(&fade_dma_chan_a_config, control_dma_chan_b); 
    // Transfer when the PWM slice asks for a new value
    channel_config_set_dreq(&fade_dma_chan_a_config, DREQ_PWM_WRAP0 + led_pwm_slice_num[0]);	
	// Set the address wrapping to 512 words (2^11 = 2048 bytes)
	channel_config_set_ring(&fade_dma_chan_a_config, false, 11);
	

	// Setup the fade_b DMA channel, start with the default configuration structure
    dma_channel_config fade_dma_chan_b_config = dma_channel_get_default_config(fade_dma_chan_b);
	// Transfer 32-bits at a time
    channel_config_set_transfer_data_size(&fade_dma_chan_b_config, DMA_SIZE_32);
	// Increment read address so we pick up a new fade value each time	
    channel_config_set_read_increment(&fade_dma_chan_b_config, true);
	// don't increment write address so we always transfer to the same PWM register	
    channel_config_set_write_increment(&fade_dma_chan_b_config, false);
	// After block has been transferred, start control_dma_chan_a
	channel_config_set_chain_to(&fade_dma_chan_b_config, control_dma_chan_a);
    // Transfer when the PWM slice asks for a new value
	channel_config_set_dreq(&fade_dma_chan_b_config, DREQ_PWM_WRAP0 + led_pwm_slice_num[0]);
	// Set the address wrapping to 512 words (2^11 = 2048 bytes)
	channel_config_set_ring(&fade_dma_chan_b_config, false, 11);	
	
	// Setup the control_dma_chan_a DMA channel, start with the default configuration structure
    dma_channel_config control_dma_chan_a_config = dma_channel_get_default_config(control_dma_chan_a);
	// Transfer 32-bits at a time
    channel_config_set_transfer_data_size(&control_dma_chan_a_config, DMA_SIZE_32);
	// Increment read address so we pick up a new fade value each time	
    channel_config_set_read_increment(&control_dma_chan_a_config, true);
	// don't increment write address so we always transfer to the same DMA register		
    channel_config_set_write_increment(&control_dma_chan_a_config, false);
	// After block has been transferred, start fade_dma_chan_a
	channel_config_set_chain_to(&control_dma_chan_a_config, fade_dma_chan_a);
	// Set the address wrapping to 4 words (2^4 = 16 bytes)	
    channel_config_set_ring(&control_dma_chan_a_config, false, 4);

	// Setup the control_dma_chan_b DMA channel, start with the default configuration structure
    dma_channel_config control_dma_chan_b_config = dma_channel_get_default_config(control_dma_chan_b);
	// Transfer 32-bits at a time
    channel_config_set_transfer_data_size(&control_dma_chan_b_config, DMA_SIZE_32);
	// Increment read address so we pick up a new fade value each time	
    channel_config_set_read_increment(&control_dma_chan_b_config, true);
	// don't increment write address so we always transfer to the same DMA register		
    channel_config_set_write_increment(&control_dma_chan_b_config, false);
	// After block has been transferred, start fade_dma_chan_b	
	channel_config_set_chain_to(&control_dma_chan_b_config, fade_dma_chan_b);
	// Set the address wrapping to 4 words (2^4 = 16 bytes)	
	channel_config_set_ring(&control_dma_chan_b_config, false, 4);

    // Link configuration structure to fade_dma_chan_a
    dma_channel_configure(
        fade_dma_chan_a, 							// Name of DMA channel
        &fade_dma_chan_a_config,					// Configuration structure
        &pwm_hw->slice[led_pwm_slice_num[0]].cc,	// Write to PWM counter compare register
        fade_a, 									// Read values from fade_a array
        512, 										// 512 words to transfer
        false 										// Don't start yet.
	);
	// Link configuration structure to fade_dma_chan_b
    dma_channel_configure(
        fade_dma_chan_b,							// Name of DMA channel
        &fade_dma_chan_b_config,					// Configuration structure
        &pwm_hw->slice[led_pwm_slice_num[0]].cc, 	// Write to PWM counter compare register
        fade_b, 									// Read values from fade_b array
        512, 										// 512 words to transfer
        false 										// Don't start yet.
    );
	// Link configuration structure to control_dma_chan_a
    dma_channel_configure(
        control_dma_chan_a,							// Name of DMA channel
        &control_dma_chan_a_config,					// Configuration structure		
        &dma_hw->ch[fade_dma_chan_a].al2_write_addr_trig, // Write PWM .cc register to 
		// fade_dma_chan_a Alias 2 write address and trigger
        &pwm_set_level_locations[0], 				// Read values from the list of PWM .cc registers
        1, 											// 1 word to transfer
        false 										// Don't start yet.
    );

	// Link configuration structure to control_dma_chan_b
    dma_channel_configure(
        control_dma_chan_b,							// Name of DMA channel
        &control_dma_chan_b_config,					// Configuration structure	
        &dma_hw->ch[fade_dma_chan_b].al2_write_addr_trig, // Write PWM .cc register to
		// fade_dma_chan_b Alias 2 write address and trigger		
        &pwm_set_level_locations[0], 				// Read values from the list of PWM .cc registers
        1, 											// 1 word to transfer
        false 										// Don't start yet.
    );
	
    // Everything is ready to go. Tell the control channel to load the first
    // control block. Everything is automatic from here.
    dma_start_channel_mask(1u << control_dma_chan_a);

    // Just catch a nap while the DMA steps through the LEDs
	while(true)
			sleep_ms(100000);	
}

