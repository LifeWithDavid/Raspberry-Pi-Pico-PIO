import time
import rp2
from machine import Pin

# This sets GPIO 6 through 11 as inputs pulled down)
for x in range(6, 12):       
  print(x)                        # prints the GPIO number for debugging
  Pin(x, Pin.IN, Pin.PULL_DOWN)

# this envokes the PIO assembler, reserving 4 out and 4 set pins, and the OSR shift direction is LSb out first
@rp2.asm_pio(set_init=(rp2.PIO.OUT_LOW,) * 4, out_init=(rp2.PIO.OUT_LOW,) *4, out_shiftdir=rp2.PIO.SHIFT_RIGHT)
             
def kb_scan():
    wrap_target()
    label("begin_again")
    pull(noblock)                 # 0b1000,1000,0100,0100,0010,0010,0001,0001
    mov(x,osr)                    # preserves the value of the last pull.  Pull(noblock) = mov(osr,x) if transmit FIFO is empty
    
# At this point, the OSR holds the values for the row selection doubled up in sequence (special sequencing word)
# (1000,1000,0100,0100,0010,0010,0001,0001 binary) The osr gets pulled twice for each row, once for row selection
# and once for row memorialization
# The 4 lsb's correspond to the GPIO for the rows
    
# In this section, turn on all the rows and look for a button push     
    label ("start_scan")
    set(pins, 15)  [31]           # turn on all rows in button matrix
    in_(pins, 6)   [31]           # move the column status into the ISR (GPIO 6, 7, 8, 9, 10, 11)
    mov(y, isr)    [31]           # shift irs into y register for zero sampling
    jmp(not_y, "start_scan") [31] # branch if y scratch register is zero, meaning no button is pushed

# button pushed, find out which one
# This section turns on each row, one at a time    
    label ("output_osr")
    jmp(not_osre,"check_row")     # Return to label if OSR still has data
    wrap()                        # osr is empty so start it again
    label("check_row")
    in_(null,32)   [31]           # clear the isr
    out(pins, 4)   [31]           # This outputs the 4 lsb's of the OSR to the GPIO, selecting the row
    in_(pins, 6)   [31]           # input the status of the buttons (columns)
    mov(y, isr)    [31]           # load the y register for zero test
    in_(osr,4)     [31]           # loads the isr with the row (0000,0000,0000,0000,0000,00CC,CCCC,RRRR) C=column, R=row
    out(pins,4)                   # this simply drains off the extra 4 lsb's
    jmp(not_y, "output_osr") [31] # branch if y scratch register is zero, meaning no button is pushed
#button was pushed, now process

    push()                        # output the ISR ro the main program 
    label ("check_button_off")
    set(pins, 15)  [31]           # turn on all rows in button matrix
    in_(pins, 6)   [31]           # move the column status into the ISR (GPIO 6, 7, 8, 9, 10, 11)
    mov(y, isr)    [31]           # shift irs into y register for zero sampling
    jmp(not_y, "begin_again") [31]# branch if y scratch register is zero, meaning no button is pushed so go to top
    jmp("check_button_off")       # button still pressed, so check again
    
# Instantiate a state machine with the kb_scan program, at 2000Hz, with set bound to Pin(12) (LED on the Pico board)
sm = rp2.StateMachine(0, kb_scan, freq=2000, set_base=Pin(12), in_base=Pin(6), out_base=Pin(12))

sm.active(1)                      # starts the state machine
sm.put(0b10001000010001000010001000010001) #The special sequencing word
while True:                       # continuous loop to print 
    time.sleep(.1)                # adds a little delay
    print("{:010b}".format(sm.get())) # prints the value of the isr in binary format(CCCCCC,RRRR)
