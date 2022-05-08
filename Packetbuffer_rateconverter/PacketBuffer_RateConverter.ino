/*
  Name:       PacketBuffer_RateConverter.ino
  Created:	5/4/2022 5:35:42 PM
  Author:     Tom Conrad
*/

/* The program receives bytes on one serial port (high speed) and transmits them over a second at a slower rate
The program relies on higher level protocols to avoid overrunning the buffers. 

TODO: add flags to determine buffer status

This version uses pointers, but it should be able to be done without them as well.

Serial1 is on D1 (txd) and D0 (rxd) (default config)
Serial2 is on D6 (txd) and D7 (rxd)

*/
#define debug 0
#define buffsize 8192  //buffer size is based on the amount of data needed to buffer based on window size

#define inputOverflowPin  9  //placeholder; need function to detect if input pointer has overrun the output pointer.  This pin will light if this happens.
#define outputOverflowPin 10  //placeholder

uint8_t rxbuff[buffsize]; //fixed buffer size greater than 32 packets @ 256 bytes per packet; adjust as needed!  If you change the mtu size in axports, you may need to adjust here (up bad, down is okay), currently set for MTU = 200, remainder of 252 is overhead
uint8_t *rxinptr;  //receive (radio) side pointer; initialize with memory address of rxbuff
uint8_t *rxoutptr; //receive (RPi) side pointer; initialize with memory address of rxbuff

uint8_t txbuff[buffsize]; //fixed buffer size greater than 32 packets
uint8_t *txinptr;  //receive (RPi) side pointer; initialize with memory address of txbuff
uint8_t *txoutptr; //receive (radio) side pointer; initialize with memory address of txbuff

//one state variable
uint8_t txstate = 0;  // by default, we are not transmitting; might use the other bits in this for FIFO flags?

//backoff timer to prevent crossing the streams 
uint32_t backofftimer = 0;
uint32_t backoff = 3; //greater than or equal to 3 bytes (10 bits ea) @ 9600 bits/second, in milliseconds


// The setup() function runs once each time the micro-controller starts
void setup()
{
  Serial.begin(115200);  //debug port
  
  //Serial 1 faces the RPi
  Serial1.begin(115200);
  
  //Serial 2 is the "radio" port, it talks to another serial port
  Serial2.begin(9600);

  rxinptr = rxbuff;
  rxoutptr = rxbuff;

  txinptr = txbuff;
  txoutptr = txbuff;
  
  backofftimer = millis();
}

// Add the main program code into the continuous loop() function
void loop()
{
  //Serial1 rx pin is txin  (receiving data from RPi)
  //Serial1 tx pin is rxout (sending data to Rpi)
  //Serial2 tx pin is txout (sending data to other unit)
  //Serial2 rx pin is rxin  (receiving data from other unit)
  //see if there's something on the input port 
  if (Serial1.available())
  {
    *txinptr = uint8_t(Serial1.read());
    txinptr++;
    if (txinptr == txbuff + buffsize)
    {
      if (debug) bufferstat();      
      txinptr = txbuff;  //wrap around to the beginning
    }
  }
  //see if there's something on the output port
  if (Serial2.available())
  {
    *rxinptr = uint8_t(Serial2.read());
    rxinptr++;
    if (rxinptr == rxbuff + buffsize)
    {
      if (debug) bufferstat();
      rxinptr = rxbuff;  //wrap around to the beginning
    }
    backofftimer = millis();  //reset backofftimer every time a new byte is received 
  }

  //the rx buffer is empty so we can transmit and I've waited 3 milliseconds to be sure there's nothing coming in AND I've got something to transmit
  if ((rxoutptr == rxinptr) && ((millis() - backofftimer) > backoff) && (txoutptr != txinptr))  // has to be empty and at longer than backoff milliseconds from last received byte
  {
	  if (debug) Serial.println("in transmit state");
    if (debug) Serial.print("timer value: "); 
    if (debug) Serial.println(millis()-backofftimer);
    if (debug) Serial.println(backoff);
    while (txoutptr != txinptr)  //check to see if there's data to be sent, the other side should be getting data in their rxbuffer, but what keeps them from transmitting?
	  {
	    Serial2.write(*txoutptr);  //this write the byte at txoutptr to Serial2
	    txoutptr++;  //increment the output pointer
	    if (txoutptr == txbuff + buffsize)
	    {
        if (debug) bufferstat();
	      txoutptr = txbuff;  //wrap around to the beginning  
	    }
	  }
  }
  //the rx buffer is not empty
  else if ((rxoutptr != rxinptr))
  {
    Serial.println("in receive state");
    while (rxoutptr != rxinptr)  //empty out whatever is in the receive buffer
    {
	    Serial1.write(*rxoutptr);  //this writes the byte at rxoutptr to Serial1
		  rxoutptr++;  //increment the output pointer
		  if (rxoutptr == rxbuff + buffsize) //zero index
		  {
        if (debug) bufferstat();
		    rxoutptr = rxbuff;  //wrap around to the beginning
		  }
    }
  }
  else
  {
    //we're idling

    // Serial.println( "idle");
  }
}

void bufferstat()
{
  Serial.println("buffer wrap");
  Serial.print("rxoutptr "); Serial.println((uint32_t)rxoutptr);
  Serial.print("rxinptr "); Serial.println((uint32_t)rxinptr);
  Serial.print("txoutptr "); Serial.println((uint32_t)txoutptr);
  Serial.print("txinptr "); Serial.println((uint32_t)txinptr);
}
