// Exmaple how to make Arduino as a SPI Slave
//
// Helper links:
// https://flyingcarsandstuff.com/2017/02/creating-a-sensor-or-peripheral-with-an-spi-slave-interface/
// http://www.gammon.com.au/forum/?id=10892
// https://electronics.stackexchange.com/questions/42197/making-two-arduinos-talk-over-spi

#include <PinChangeInt.h>

#define BUF_SIZE 100

char buf [BUF_SIZE];
volatile byte pos;
volatile boolean process_it;

#define STORAGE_SIZE 256

char storage[STORAGE_SIZE];

// A global flag to keep track of our use of the MISO line
bool chipSelected = false;

// Start of transaction
void chipSelectedChanged ()
{
    if (digitalRead(SS)) // If HIGH (true) the chip is not selected
    {
        // SS line *not* selected
        
        // We *were* using MISO, previously,
        // so put the slave-out line back
        // into high-z
        pinMode(MISO, INPUT);
        chipSelected = false;
    }      
    else
    {
        // The SS line is LOW, so we *are* selected
        pinMode(MISO, OUTPUT);        
        pos = 0;
        process_it = false;
        chipSelected = true;        
    }
}

#define SPI_MODE_MASK 0x0C  // CPOL = bit 3, CPHA = bit 2 on SPCR
#define SPI_LSBFIRST 0
#define SPI_MSBFIRST 1
#define SPI_MODE_0 0x00
#define SPI_MODE_1 0x04
#define SPI_MODE_2 0x08
#define SPI_MODE_3 0x0C

void setSpiBitOrder(uint8_t bitOrder) 
{
    if (bitOrder == SPI_LSBFIRST) 
       SPCR |= _BV(DORD);
    else 
       SPCR &= ~(_BV(DORD));
}
  
void setSpiDataMode(uint8_t dataMode)
{
    SPCR = (SPCR & ~SPI_MODE_MASK) | dataMode;
}

// Setup procedure
void setup (void)
{
    Serial.begin (115200);
    Serial.println("Wainting for SPI data...\n\r");

    for (int i=0; i<STORAGE_SIZE; i++)
      storage[i] = 0;
    
    // Initialize SPI pins.
    // the clock, master-in and select 
    // are obviously all inputs
    //pinMode(SCK, INPUT);
    //pinMode(MISO, INPUT);
    //pinMode(SS, INPUT);
  
    // Our output is, initially, put
    // in high-z mode by setting it
    // as an input, too, so we allow
    // other slaves on the SPI bus
    // to work.
    //pinMode(MOSI, INPUT); 
    DDRB = (1 << 6);
    
    setSpiBitOrder(SPI_MSBFIRST);
    setSpiDataMode(SPI_MODE_2);
     
    // Get ready for an interrupt 
    pos = 0;   // buffer empty
    process_it = false;
  
    // Turn on SPI in slave mode and interrupts
    SPCR |= (1 << SPE) | (1 << SPIE);

    // Interrupt for SS chnaged edge
    attachPinChangeInterrupt(10, chipSelectedChanged, CHANGE);    

    interrupts();
}

// SPI interrupt routine
ISR (SPI_STC_vect)
{      
    if (chipSelected)
    {  
        while(!(SPSR)&(1<<SPIF));
            
        byte c = SPDR;  // Grab byte from SPI Data Register
        
        if (pos < BUF_SIZE - 1)
        {
          Serial.print(c, HEX);
            buf [pos++] = c;
            if (c == '\n')
              process_it = true;
        }
    }
}

// Main loop for sending data over serial when requested
void loop (void)
{
    if (process_it)
    {
        //for (int i=0; i<pos; i++)
        //{
        //  Serial.print((byte) buf[i], HEX);
        //  Serial.print(" ");
        //}
        Serial.print("\n\rData:");
        buf [pos] = 0;
        Serial.println(buf);
        pos = 0;
        process_it = false;
    }
}
