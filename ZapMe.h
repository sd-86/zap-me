/*
  ZapMe - Arduino libary for remote controlled electric shock collars.

  BSD 2-Clause License

  Copyright (c) 2020, sd_ <sd@smjg.org>
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice, this
     list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _ZapMe_h
#define _ZapMe_h

#if defined(ARDUINO) && ARDUINO >= 100
    #include "Arduino.h"
#endif

#include <stdint.h>

/* To enable debugging, uncomment the next line.
 * You are responsible for enabling Serial communication yourself, e.g.
 * put `Serial.begin(9600);` into your setup routine.
 */
//#define DEBUG_ZAPME 1

#ifdef DEBUG_ZAPME
#define DEBUG_ZAPME_MSGLN(msg) \
  do {                         \
    Serial.println(msg);       \
  } while(false);
#define DEBUG_ZAPME_MSG(msg) \
  do {                       \
    Serial.print(msg);       \
  } while(false);
#else
#define DEBUG_ZAPME_MSGLN(msg) do {} while(false);
#define DEBUG_ZAPME_MSG(msg) do {} while(false);
#endif

class ZapMe {
  public:
    ZapMe(uint8_t transmitPin) : mTransmitPin(transmitPin), mDebug(false) {
      pinMode(mTransmitPin, OUTPUT);
    }

    virtual void sendShock(uint8_t strength, uint16_t duration) = 0;
    virtual void sendVibration(uint8_t strength, uint16_t duration) = 0;
    virtual void sendAudio(uint8_t strength, uint16_t duration) = 0;

    void sendTiming(uint16_t* timings);

  protected:
    uint8_t mTransmitPin;
    bool mDebug;
};


class CH8803 : public ZapMe {
  /*
   * Chinese shockcollar 880-3 (Euro) uses 5 bytes of data per command:
   *
   * | ID (16-bit) | Channel (4 bit) | Function (4 bit) | Strength (8 bit) | Checksum (8 bit) |
   *
   * The ID is simply an identifier randomly assigned to each remote. Collars
   * can be set to learning mode to be associated with a new ID.
   *
   * The channel is an integer that is either 0, 1 or 2 mapping to CH1, CH2 or CH3 on the remote.
   *
   * The function is an integer that is either 1 (Shock), 2 (Vibration) or 3 (Sound).
   *
   * The strength is an integer in range 0 to 99. It is always 0 for sound.
   *
   * The checksum is simply an unsigned sum of all the previous bytes.
   */


  public:
    CH8803(uint8_t transmitPin, uint16_t id)
      : ZapMe(transmitPin), mId(id), mChannel(0) {
        /*
         * In transmit timings, we need 3 timings for the sync preamble,
         * then 64 timings to send 40 bit of data, then 3 trailing zeros
         * and one for the terminating null byte we are going to use as
         * a length indicator.
         */
        mMaxTransmitTimings = 3 + 2*40 + 2*3 + 1;
        mTransmitTimings = new uint16_t[mMaxTransmitTimings];
      }

    void setId(uint16_t id) { mId = id; }
    void setId(uint8_t hid, uint8_t lid) { mId = (hid << 8) + lid; }

    void setChannel(uint8_t ch) { mChannel = ch; }

    virtual void sendShock(uint8_t strength, uint16_t duration);
    virtual void sendVibration(uint8_t strength, uint16_t duration);
    virtual void sendAudio(uint8_t strength, uint16_t duration);

  protected:

    void sendCommand(uint8_t func, uint8_t strength, uint16_t duration);

    uint16_t mId;
    uint8_t mChannel;
    uint16_t* mTransmitTimings;
    uint16_t mMaxTransmitTimings;
};

#endif
