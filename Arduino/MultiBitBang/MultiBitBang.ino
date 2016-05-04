
// These values depend on which pins your 8 strings are connected to and what board you are using 
// More info on how to find these at http://www.arduino.cc/en/Reference/PortManipulation

// PORTD controls Digital Pins 0-7 on the Uno

// You'll need to look up the port/bit combination for other boards. 

// Note that you could also include the DigitalWriteFast header file to not need to to this lookup.

#define PIXEL_PORT  PORTD  // Port of the pin the pixels are connected to
#define PIXEL_DDR   DDRD   // Port of the pin the pixels are connected to

// These are the timing constraints taken mostly from the WS2812 datasheets 
// These are chosen to be conservative and avoid problems rather than for maximum throughput 

#define T1H  900    // Width of a 1 bit in ns
#define T1L  600    // Width of a 1 bit in ns

#define T0H  400    // Width of a 0 bit in ns
#define T0L  900    // Width of a 0 bit in ns

#define RES 6000    // Width of the low gap between bits to cause a frame to latch

// Here are some convience defines for using nanoseconds specs to generate actual CPU delays

#define NS_PER_SEC (1000000000L)          // Note that this has to be SIGNED since we want to be able to check for negative values of derivatives

#define CYCLES_PER_SEC (F_CPU)

#define NS_PER_CYCLE ( NS_PER_SEC / CYCLES_PER_SEC )

#define NS_TO_CYCLES(n) ( (n) / NS_PER_CYCLE )

// Actually send the next set of 8 WS2812B encoded bits to the 8 pins.
// We must to drop to asm to enusre that the complier does
// not reorder things and make it so the delay happens in the wrong place.

static inline __attribute__ ((always_inline)) void sendBitX8( uint8_t bits ) {

    const uint8_t onBits = 0xff;          // We need to send all bits on on all pins as the first 1/3 of the encoded bits
            
    asm volatile (
      
      "out %[port], %[onBits] \n\t"           // 1st step - send T0H high 
      
      ".rept %[T0HCycles] \n\t"               // Execute NOPs to delay exactly the specified number of cycles
        "nop \n\t"
      ".endr \n\t"
      
      "out %[port], %[bits] \n\t"             // set the output bits to thier values for T0H-T1H
      ".rept %[dataCycles] \n\t"               // Execute NOPs to delay exactly the specified number of cycles
      "nop \n\t"
      ".endr \n\t"
      
      "out %[port],__zero_reg__  \n\t"        // last step - T1L all bits low

      // Don't need an explicit delay here since the overhead that follows will always be long enough
    
      ::
      [port]    "I" (_SFR_IO_ADDR(PIXEL_PORT)),
      [bits]   "d" (bits),
      [onBits]   "d" (onBits),
      
      [T0HCycles]  "I" (NS_TO_CYCLES(T0H) - 2),           // 1-bit width less overhead  for the actual bit setting, note that this delay could be longer and everything would still work
      
      [dataCycles]   "I" (NS_TO_CYCLES((T1H-T0H)) - 2)     // Minimum interbit delay. Note that we probably don't need this at all since the loop overhead will be enough, but here for correctness

    );
                                  

    // Note that the inter-bit gap can be as long as you want as long as it doesn't exceed the reset timeout (which is A long time)
    
}  


// Each string is one bit in "row" represening on or off

static inline void __attribute__ ((always_inline)) sendPixelRow( uint8_t row ) {

  // Send the bit 24 times down every row. This is 100% white. Remember that each pixel is 24 bits wide (8 bits each for R,G, & B)
  
  uint8_t bit=24;       

  while (bit--) {

      sendBitX8( row );    
  }

}
    

// Just wait long enough without sending any bits to cause the pixels to latch and display the last sent frame

void show() {
  _delay_us( (RES / 1000UL) + 1);       // Round up since the delay must be _at_least_ this long (too short might not work, too long not a problem)
}



void setup() {
    
  PIXEL_DDR = 0xff;    // Set all row pins to output
  
}


void loop() {

  cli();                        // No time for interruptions!
  sendPixelRow( 0b10000000 );   // Send an interesting and challenging pattern 
  sendPixelRow( 0b01000000 );
  sendPixelRow( 0b00100000 );
  sendPixelRow( 0b00010000 );
  sendPixelRow( 0b00001000 );
  sendPixelRow( 0b00000100 );
  sendPixelRow( 0b00000010 );
  sendPixelRow( 0b00000001 );
  sendPixelRow( 0b00000010 );
  sendPixelRow( 0b00000100 );
  sendPixelRow( 0b00001000 );
  sendPixelRow( 0b00010000 );
  sendPixelRow( 0b00100000 );
  sendPixelRow( 0b01000000 );
  sendPixelRow( 0b10000000 );
  sendPixelRow( 0b00000000 );
  sendPixelRow( 0b01010101 );
  sendPixelRow( 0b10101010 );
  sendPixelRow( 0b01010101 );
  sendPixelRow( 0b10101010 );  
  sendPixelRow( 0x00000000 );
  sendPixelRow( 0b11111111 );  
  sendPixelRow( 0x00000000 );
  sendPixelRow( 0b11111111 );  
  sendPixelRow( 0x00000000 );
    
  sei();

  delay(10);      // Wait more than RESET timout to let latch into the LEDs
  return;  
}



