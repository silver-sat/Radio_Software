/**
 * @file packetfinder_test.ino
 * @author Tom Conrad (tom@silversat.org)
 * @brief tests for the packetfinder functions
 * @version 1.0.1
 * @date 2022-10-23
 *
 * What we're testing:
 * Align packet to first 0xC0
 * Compute the packet size correctly
 * Handle partial packets
 * Handle repeated calls without corrupting the buffer
 * Handle a "short packet" of just 2 C0s and still process the next packet.  
 *
 */

#define  DEBUG

#ifdef DEBUG
#define debug_printf printf
#else
#define debug_printf(...)
#endif

#include <LibPrintf.h>
#include <CircularBuffer.h>
#include "packetfinder.h" //need to figure this out!!

#define CMDBUFFSIZE 1024
#define DATABUFFSIZE 8192

CircularBuffer<unsigned char, CMDBUFFSIZE> cmdbuffer;
CircularBuffer<unsigned char, DATABUFFSIZE> databuffer;
CircularBuffer<unsigned char, CMDBUFFSIZE> cmd2buffer;  //extra buffer for scenario 3

void setup()
{
    Serial.begin(57600);
    while (!Serial);
    //delay(3000);
    debug_printf("building buffers \n");
    //parameters
    uint8_t c0position = 10;
    int packetend = 30; //where the end of the first packet should be in the buffer at the before processing
    int testbuffersize = 40; //extra bytes after the buffer end; represents a new but not complete packet

    //Scenario 1 -----------------------------------------------------
    debug_printf("Scenario 1: first packet is complete (size is %u, starting at %u, with a second partial packet following, using cmdbuffer \n", packetend-c0position+1, c0position);
	  /* push something that looks like data into the buffer */
    for (uint8_t j = 0; j < c0position; j++){
      cmdbuffer.push(j);
    }
    cmdbuffer.push(uint8_t(0xC0));
    for (uint8_t j=c0position + 1; j < packetend; j++){
      cmdbuffer.push(j);
    }
    cmdbuffer.push(uint8_t(0xC0));
    cmdbuffer.push(uint8_t(0xC0));  //start a new, but not complete packet...note the two 0xC0's
    for (uint8_t j=packetend + 2; j < testbuffersize; j++){
      cmdbuffer.push(j);
    }

    //report sizes before processing; print out initial buffer contents
    debug_printf("command buffer size before processing: %u \n", cmdbuffer.size());
    for(int i=0; i< cmdbuffer.size(); i++){
      debug_printf("index %u : value %x \n", i, cmdbuffer[i]);
    }

    //process dem buffers
    debug_printf("processing cmd buffer \n");
    uint8_t packetsize = processbuff(cmdbuffer);
    debug_printf("processbuff returned %u \n", packetsize);
    debug_printf("the answer should be %u \n", packetend-c0position+1);

    //report size after processing; print out modified buffers
    debug_printf("command buffer size after processing: %u \n", cmdbuffer.size());
    for(int i=0; i< cmdbuffer.size(); i++){
      debug_printf("index %u : value %x \n", i, cmdbuffer[i]);
    }

    //Scenario 2 -----------------------------------------------------
    /* push something that looks like data into the buffer */
    //this one here just to show how a longer different packet size would work
    debug_printf("Scenario 2: first packet is complete, different size of packet %u and buffer (using databuffer which is bigger) \n", packetend-c0position+1);
    c0position = 20;
    for (uint8_t j = 0; j < c0position; j++){
      databuffer.push(j);
    }
    databuffer.push(uint8_t(0xC0));
    for (uint8_t j=c0position + 1; j < testbuffersize; j++){
      databuffer.push(j);
    }
    databuffer.push(uint8_t(0xC0));

    //report sizes before processing; print out initial buffer contents
    debug_printf("data buffer size before processing: %u \n", databuffer.size());
    for(int i=0; i< databuffer.size(); i++){
      debug_printf("index %u : value %x \n", i, databuffer[i]);
    }

    //process dem buffers
    debug_printf("processing data buffer \n");
    packetsize = processbuff(databuffer);
    debug_printf("processbuff returned %u \n", packetsize);


    //report result
    debug_printf("data buffer size after processing: %u \n", databuffer.size());
    for(int i=0; i< databuffer.size(); i++){
      debug_printf("index %u : value %x \n", i, databuffer[i]);
    }

    //Scenario 3 -----------------------------------------------------
    // incomplete packet, only one C0 is in buffer
    debug_printf("Scenario 3: incomplete packet \n");
    c0position = 20;
    for (uint8_t j = 0; j < c0position; j++){
      cmd2buffer.push(j);
    }
    cmd2buffer.push(uint8_t(0xC0));
    for (uint8_t j=c0position + 1; j < testbuffersize; j++){
      cmd2buffer.push(j);
    }

    //report sizes before processing; print out initial buffer contents
    debug_printf("cmd2 buffer size before processing: %u \n", cmd2buffer.size());
    for(int i=0; i< cmd2buffer.size(); i++){
      debug_printf("index %u : value %x \n", i, cmd2buffer[i]);
    }

    //process dem buffers
    debug_printf("processing cmd2 buffer \n");
    packetsize = processbuff(cmd2buffer);
    debug_printf("processbuff returned %u \n", packetsize);

    //report result
    debug_printf("cmd2 buffer size after processing: %u \n", cmd2buffer.size());
    for(int i=0; i< cmd2buffer.size(); i++){
      debug_printf("index %u : value %x \n", i, cmd2buffer[i]);
    }

    //Scenario 4 --------------------------------------------------------
    debug_printf("Scenario 4: now processing cmd2 (from Scenario 3) buffer one more time (shouldn't do anything to the buffer!) \n");

    //report sizes before processing; print out initial buffer contents
    debug_printf("command buffer size before processing: %u \n", cmd2buffer.size());
    for(int i=0; i< cmd2buffer.size(); i++){
      debug_printf("index %u : value %x \n", i, cmd2buffer[i]);
    }

    //process dem buffers
    debug_printf("processing cmd buffer \n");
    packetsize = processbuff(cmd2buffer);
    debug_printf("processbuff returned %u \n", packetsize);

    //report size after processing; print out modified buffers
    debug_printf("command buffer size after processing: %u \n", cmd2buffer.size());
    for(int i=0; i< cmd2buffer.size(); i++){
      debug_printf("index %u : value %x \n", i, cmd2buffer[i]);
    }

    //Scenario 5 --------------------------------------------------------
    debug_printf("Scenario 5: load 2 C0s, process the packet, then load data containing a complete packet (having junk at the beginning).  Does it report the size correctly? \n");
    /* need to start with an empty buffer */
    cmdbuffer.clear();

	  /* push 2 C0's into the buffer */
    for (uint8_t j = 0; j < 2; j++){
      cmdbuffer.push(0xC0);
    }
    
    //report sizes before processing; print out initial buffer contents
    debug_printf("command buffer size before processing: %u \n", cmdbuffer.size());
    for(int i=0; i< cmdbuffer.size(); i++){
      debug_printf("index %u : value %x \n", i, cmdbuffer[i]);
    }

    //process dem buffers
    debug_printf("processing cmd buffer \n");
    packetsize = processbuff(cmdbuffer);
    debug_printf("processbuff returned %u \n", packetsize);
    debug_printf("current size of data in buffer %u \n", cmdbuffer.size());
        
    /* push something that looks like data into the buffer */
    for (uint8_t j = 0; j < c0position; j++){
      cmdbuffer.push(j);
    }
    cmdbuffer.push(uint8_t(0xC0));
    for (uint8_t j=c0position + 1; j < packetend; j++){
      cmdbuffer.push(j);
    }
    //cmdbuffer.push(uint8_t(0xC0));  taking this out to see what junk characters prior to the first C0 would do.
    cmdbuffer.push(uint8_t(0xC0));  //start a new, but not complete packet...note the two 0xC0's
    for (uint8_t j=packetend + 2; j < testbuffersize; j++){
      cmdbuffer.push(j);
    }

    //report sizes before processing; print out initial buffer contents
    debug_printf("command buffer size before processing: %u \n", cmdbuffer.size());
    for(int i=0; i< cmdbuffer.size(); i++){
      debug_printf("index %u : value %x \n", i, cmdbuffer[i]);
    }

    //process dem buffers
    debug_printf("processing cmd buffer \n");
    packetsize = processbuff(cmdbuffer);
    debug_printf("processbuff returned %u \n", packetsize);

    //report size after processing; print out modified buffers
    debug_printf("command buffer size after processing: %u \n", cmdbuffer.size());
    for(int i=0; i< cmdbuffer.size(); i++){
      debug_printf("index %u : value %x \n", i, cmdbuffer[i]);
    }

    debug_printf("and now let's process that again!");

    //process dem buffers
    debug_printf("processing cmd buffer \n");
    packetsize = processbuff(cmdbuffer);
    debug_printf("processbuff returned %u \n", packetsize);

    //report size after processing; print out modified buffers
    debug_printf("command buffer size after processing: %u \n", cmdbuffer.size());
    for(int i=0; i< cmdbuffer.size(); i++){
      debug_printf("index %u : value %x \n", i, cmdbuffer[i]);
    }

    //Scenario 6 --------------------------------------------------------
    debug_printf("Scenario 6: load 3 C0s, process the packet, then load a complete packet.  Does it report the size correctly? \n");
    /* need to start with an empty buffer */
    cmdbuffer.clear();

    /* push 2 C0's into the buffer */
    for (uint8_t j = 0; j < 3; j++){
      cmdbuffer.push(0xC0);
    }
    
    //report sizes before processing; print out initial buffer contents
    debug_printf("command buffer size before processing: %u \n", cmdbuffer.size());
    for(int i=0; i< cmdbuffer.size(); i++){
      debug_printf("index %u : value %x \n", i, cmdbuffer[i]);
    }

    //process dem buffers
    debug_printf("processing cmd buffer \n");
    packetsize = processbuff(cmdbuffer);
    debug_printf("processbuff returned %u \n", packetsize);
    debug_printf("current size of data in buffer %u \n", cmdbuffer.size());
        
    /* push something that looks like data into the buffer */
    for (uint8_t j = 0; j < c0position; j++){
      cmdbuffer.push(j);
    }
    cmdbuffer.push(uint8_t(0xC0));
    for (uint8_t j=c0position + 1; j < packetend; j++){
      cmdbuffer.push(j);
    }
    //cmdbuffer.push(uint8_t(0xC0));  taking this out to see what junk characters prior to the first C0 would do.
    cmdbuffer.push(uint8_t(0xC0));  //start a new, but not complete packet...note the two 0xC0's
    for (uint8_t j=packetend + 2; j < testbuffersize; j++){
      cmdbuffer.push(j);
    }

    //report sizes before processing; print out initial buffer contents
    debug_printf("command buffer size before processing: %u \n", cmdbuffer.size());
    for(int i=0; i< cmdbuffer.size(); i++){
      debug_printf("index %u : value %x \n", i, cmdbuffer[i]);
    }

    //process dem buffers
    debug_printf("processing cmd buffer \n");
    packetsize = processbuff(cmdbuffer);
    debug_printf("processbuff returned %u \n", packetsize);

    //report size after processing; print out modified buffers
    debug_printf("command buffer size after processing: %u \n", cmdbuffer.size());
    for(int i=0; i< cmdbuffer.size(); i++){
      debug_printf("index %u : value %x \n", i, cmdbuffer[i]);
    }

    debug_printf("and now let's process that again!");

    //process dem buffers
    debug_printf("processing cmd buffer \n");
    packetsize = processbuff(cmdbuffer);
    debug_printf("processbuff returned %u \n", packetsize);

    //report size after processing; print out modified buffers
    debug_printf("command buffer size after processing: %u \n", cmdbuffer.size());
    for(int i=0; i< cmdbuffer.size(); i++){
      debug_printf("index %u : value %x \n", i, cmdbuffer[i]);
    }

    //Scenario 7 --------------------------------------------------------
    debug_printf("Scenario 7: load 2 C0s, process the packet, then load a good complete packet.  Does it report the size correctly? \n");
    /* need to start with an empty buffer */
    cmdbuffer.clear();

    /* push 2 C0's into the buffer */
    for (uint8_t j = 0; j < 2; j++){
      cmdbuffer.push(0xC0);
    }
    
    //report sizes before processing; print out initial buffer contents
    debug_printf("command buffer size before processing: %u \n", cmdbuffer.size());
    for(int i=0; i< cmdbuffer.size(); i++){
      debug_printf("index %u : value %x \n", i, cmdbuffer[i]);
    }

    //process dem buffers
    debug_printf("processing cmd buffer \n");
    packetsize = processbuff(cmdbuffer);
    debug_printf("processbuff returned %u \n", packetsize);
    debug_printf("current size of data in buffer %u \n", cmdbuffer.size());
        
    /* push a good packet into the buffer */
   
    cmdbuffer.push(uint8_t(0xC0));
    for (uint8_t j=c0position + 1; j < packetend; j++){
      cmdbuffer.push(j);
    }
    cmdbuffer.push(uint8_t(0xC0));
    cmdbuffer.push(uint8_t(0xC0));  //start a new, but not complete packet...note the two 0xC0's
    for (uint8_t j=packetend + 2; j < testbuffersize; j++){
      cmdbuffer.push(j);
    }

    //report sizes before processing; print out initial buffer contents
    debug_printf("command buffer size before processing: %u \n", cmdbuffer.size());
    for(int i=0; i< cmdbuffer.size(); i++){
      debug_printf("index %u : value %x \n", i, cmdbuffer[i]);
    }

    //process dem buffers
    debug_printf("processing cmd buffer \n");
    packetsize = processbuff(cmdbuffer);
    debug_printf("processbuff returned %u \n", packetsize);

    //report size after processing; print out modified buffers
    debug_printf("command buffer size after processing: %u \n", cmdbuffer.size());
    for(int i=0; i< cmdbuffer.size(); i++){
      debug_printf("index %u : value %x \n", i, cmdbuffer[i]);
    }

    debug_printf("and now let's process that again!");

    //process dem buffers
    debug_printf("processing cmd buffer \n");
    packetsize = processbuff(cmdbuffer);
    debug_printf("processbuff returned %u \n", packetsize);

    //report size after processing; print out modified buffers
    debug_printf("command buffer size after processing: %u \n", cmdbuffer.size());
    for(int i=0; i< cmdbuffer.size(); i++){
      debug_printf("index %u : value %x \n", i, cmdbuffer[i]);
    }

    debug_printf("and that's all folks!");
}

void loop()
{

	  /* add main program code here, this code starts again each time it ends */

}
