/**
 * @file commands.cpp
 * @author Tom Conrad (tom@silversat.org)
 * @brief command processor for Silversat
 * @version 1.0.1
 * @date 2022-11-08
 * 
 * what's in the command buffer is KISS encoded, but guaranteed by design to be ASCII printable characters, so there's no need to decode it.
 * a command packet has multiple parts (but we don't really care about a lot of it)
 * KISS delimiter, KISS command byte, maybe some data, KISS delimiter
 * length in most cases will be 1 (should test that too) 
 * 
 */

#include "commands.h"

#ifdef DEBUG
#define debug_printf printf
#else
#define debug_printf(...)
#endif

void processcmdbuff(CircularBuffer<unsigned char, CMDBUFFSIZE>& mybuffer, CircularBuffer<unsigned char, DATABUFFSIZE>& txbuffer, int packetlength, ax_config& config)
{
  unsigned char response[240]; //for command responses..set for max size of one packet

  mybuffer.shift();  //remove the seal... C0
  unsigned char commandcode = mybuffer.shift(); //and the command code
  debug_printf("command code is: %x \n", commandcode);

  unsigned char commanddata[MAXCOMMANDSIZE]; //to hold the command data or response
  unsigned int length;

  switch (commandcode)
  {
    case 0xAA:  //catching if the command is also AA.
    case 0:  //nothing to see here, it's not for me...forward to the other end, so copy this over to the tx buffer
      txbuffer.push(0xC0);
      //so for commands or responses bound for the other side, I'm adding a new command code back on.  However, why not just make this the address byte?
      //I am using an 0xAA to indicate it's for Serial0 (Avionics)...normal data from Payload will likely have 0x00, where ? is the Port index nibble, which
      //should be 0 since it's port AX0.  I'm using 0xAA to be even safe since that's not a valid port (only valid ports are 0-9).
      txbuffer.push(0xAA);  
      for (int i=2; i < packetlength; i++)
      {  //you're starting at the second byte of the total packet
        txbuffer.push(mybuffer.shift());  //shift it out of mybuffer and push it into txbuffer, don't need to push a final 0xC0 because it's still part of the packet
      }

      debug_printf("what's in the tx buffer? \n");
      for (int i=0; i< txbuffer.size(); i++)
      {
        debug_printf("index %x , value %x \n", i, txbuffer[i]);
      }

      break;

    case 0x07:
      //beacon
      sendACK(commandcode);
      //only beacon has data attached to the command
      for (int i=0; i<MAXCOMMANDSIZE; i++)
      {
        commanddata[i] = (unsigned char)mybuffer.shift(); //pull out the data bytes in the buffer (command data or response)
      }
      mybuffer.shift();  //remove the last C0
      //sendbeacon(commanddata, config);  //give it the whole string and let the function take what it needs, the action function will send the response?
      sendbeacon((unsigned char&)commanddata, config);
      break;

    case 0x08:
      //deploy the antenna and report if successful
      sendACK(commandcode);
      length = deployantenna(*response); //how long should this take?
      sendResponse(*response, length);
      mybuffer.shift();  //remove the last C0
      break;

    case 0x09:
      //status
      sendACK(commandcode);
      length = reportstatus(*response); //the status should just be written to a string somewhere, or something like that.
      sendResponse(*response, length);
      mybuffer.shift();  //remove the last C0
      break;

    case 0x0A:
      //halt radio transmissions, revert to RX state and report if successful
      sendACK(commandcode);
      haltradio();
      mybuffer.shift();  //remove the last C0
      break;

    default:
      sendNACK(commandcode);
      mybuffer.shift();  //remove the last C0
      break;
  }
}

void sendACK(unsigned char code)
{
  //create an ACK packet and send it out Serial1 - for testing at this moment just sent it to Serial
  //note that acks always go to Serial1
  debug_printf("ACK!! \n");

  unsigned char ackpacket[] = {0xC0, 0x00, 0x41, 0x43, 0x4B, 0x00, 0xC0};  //generic form of ack packet
  ackpacket[5] = code;  //replace code byte with the received command code

  Serial0.write(ackpacket, 7);  //TODO: change to Serial1 later

}

void sendNACK(unsigned char code)
{
  //create an NACK packet and put it in the CMD TX queue
  //nacks always go to Serial1
  debug_printf("NACK!! \n"); //bad code, no cookie!
  unsigned char nackpacket[] = {0xC0, 0x00, 0x4E, 0x41, 0x43, 0x4B, 0x00, 0xC0};  //generic form of nack packet
  nackpacket[6] = code;  //replace code byte with the received command code

  Serial0.write(nackpacket, 7);
}


//send a response.  currently assumed that response is a character string
void sendResponse(unsigned char& response, int responselen)
{
  debug_printf("Sending the response \n");
  
  //responses are KISS with cmd byte = 0x00, and always start with 'RES'
  unsigned char responsepacket[] = {0xC0, 0x00, 0x52, 0x45, 0x53};  
  
  //write it to Serial1 in parts
  Serial0.write(responsepacket,5);                // first header
  Serial0.write(&response, responselen); //now the actual data
  Serial0.write(0xC0);                            //and finish the KISS packet
}


unsigned int deployantenna(unsigned char& response)
{
    //this should probably be its own class.  There's a lot going on here...
    //this command is only local, and it's only useful on the satellite
    debug_printf("deploying the antenna and generating a report \n");
    //strcpy((char*)response, "antenna status report");
    String responseString = "antenna status report";
    responseString.getBytes(&response, responseString.length());
    return responseString.length();
}


unsigned int reportstatus(unsigned char& response)
{
    //WASSUP!
    debug_printf("generating the status report \n");
    //eFuse 5V status
    //PA temperature
    //FEC?
    String responseString = "status report";
    responseString.getBytes(&response, responseString.length());
    return responseString.length();
    //strcpy((char*)response, "status report");
    //return strlen((char*)response);
}


void haltradio()
{
    //lot going on here too!
    debug_printf("halting all radio activity \n");
}