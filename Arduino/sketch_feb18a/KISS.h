// [Modified from GitHub josefmtd/kiss-aprs/KISS.h
// https://github.com/josefmtd/kiss-aprs/blob/88945ebbd482d26bc564d0dac64841bf0b622dbb/KISS.h
// by isaac-silversat of SilverSat Limited (https://www.silversat.org)]

#ifndef KISS_H
#define KISS_H

#include <Arduino.h>

#define FEND      0xC0
#define FESC      0xDB
#define TFEND     0xDC
#define TFESC     0xDD

#define CMD_DATA      0x00
#define CMD_TXDELAY   0x01
#define CMD_PERSIST   0x02
#define CMD_SLOTTIME  0x03
#define CMD_TXTAIL    0x04
#define CMD_DUPLEX    0x05
#define CMD_HARDWARE  0x06
#define CMD_RETURN    0xFF

// SilverSat-specific commands
// These commands have not yet been incuded to contain them within SilverSat Limited;
// such that they may not be used to gain unauthorized control of the satellite.

// [KISS class commented]
// class KISSClass : public Stream {
//   public:
//     KISSClass(HardwareSerial& hwSerial);

//     virtual void begin(unsigned long baudRate);
//     virtual void begin(unsigned long baudRate, uint16_t config);
//     virtual void end();
//     virtual int available();
//     virtual int peek();
//     virtual int read(void);
//     virtual void flush();
//     virtual size_t write(uint8_t byte);
//     using Print::write;

//     void sendData(const char *buffer, int bufferSize);
//     void sendTxDelay(const uint8_t txDelay);
//     void sendPersist(const uint8_t persist);
//     void sendTimeSlot(const uint8_t timeSlot);
//     void sendTxTail(const uint8_t txTail);
//     void sendDuplex(const uint8_t duplex);

//   private:
//     HardwareSerial* _serial;
//     unsigned long _baudRate;
//     uint16_t _config;

//     uint8_t _txDelay;
//     uint8_t _persist;
//     uint8_t _timeSlot;
//     uint8_t _txTail;
//     uint8_t _duplex;
//     void escapedWrite(uint8_t bufferByte);
// };

#endif