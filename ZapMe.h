/*
  ZapMe - Arduino libary for remote controlled electric shock collars.

  BSD 2-Clause License

  Copyright (c) 2020, sd_ <sd@smjg.org> and others.
  Please see the AUTHORS file in the main source directory for details.
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

/*
 * Common macro to translate bits into timings as follows:
 *
 * Each bit is a pulse of fixed-width (`pulseLength` microseconds), where
 * a zero has a raise of `zeroLength` microseconds at the beginning and
 * a one has a raise of `oneLength` microseconds at the beginning.
 */

 /* Translate l bits to timings */
 #define TRBITS(b,l) \
   for (uint8_t k = l - 1; k <= l - 1; --k) {                   \
     uint16_t bitLength = zeroLength;                           \
     if (((b) & ( 1 << k )) >> k)                               \
       bitLength = oneLength;                                   \
     mTransmitTimings[transmitIdx++] = bitLength;               \
     mTransmitTimings[transmitIdx++] = pulseLength - bitLength; \
   };

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

class DogTronic : public ZapMe {
  /*
   * DogTronic uses a sync preamble of short pulses followed by a relatively
   * slow series of constant length pulses. The gaps inbetween the pulses
   * encode bits (short gap is 0, long gap is 1) and each command has 16 bits.
   *
   * For a command c[15:0], we have:
   *
   * c[15:10] - 6-bit ID (presumably)
   * c[9:6]   - 4-bit shock strength (LSB first)
   * c[5:4]   - 2-bit constant 0b10, unknown purpose
   * c[3:0]   - 4-bit checksum (see below)
   *
   * The checksum c[3:0] is particularly weird of construction:
   *
   * This is a 4-bit value, but the bits are reordered, the proper number
   * is reconstructed as c[2] c[3] c[0] c[1], so this is MSB but the bits
   * have been pairwise swapped. Apart from that, it is a simple sum that
   * does not cover the ID, but only the strength. Since the constant at
   * c[5:4] never changes, we can't tell if it is part of the sum or not.
   * The sum starts at an offset of 0b0100 and then the strength is added.
   * However, if an overflow beyond 4-bit occurs, the sum is not simply
   * truncated (mod 16), but the overflow bit is fed back into the register
   * from the LSB side, so the checksum is:
   *
   *   (0b0100 + strength) mod 16 + (0b0100 + strength) >> 4.
   *
   * Internally, this is probably a 4-bit full adder with the carry-out
   * hard-wired back to the carry-in.
   *
   * IMPORTANT: So far, with this implementation, the collar only accepts
   *            4 different IDs as valid, these are: 14, 23, 44 and 53.
   *            We don't know, why this is the case, it could be tied to
   *            the checksum somehow but without more samples, it is hard
   *            to tell.
   */

  public:
    DogTronic(uint8_t transmitPin, uint8_t id)
      : ZapMe(transmitPin), mId(id) {
        /*
         * In transmit timings, we need 32 timings for the sync preamble,
         * then 32 timings to send 16 bit of data, finally 1 timing for
         * the larger gap after the message and 1 timing for the null byte.
         */
        mMaxTransmitTimings = 32*2 + 2;
        mTransmitTimings = new uint16_t[mMaxTransmitTimings];
      }

    void setId(uint8_t id) { mId = id; }

    virtual void sendShock(uint8_t strength, uint16_t duration);

    /*
     * Dogtronic has no separate vibration and audio, the function is
     * the same but there are two different versions of the collar,
     * one with vibration and one with audio. So it doesn't really matter
     * which of the two here is used, they map to the same functionality.
     */
    virtual void sendVibration(uint8_t strength, uint16_t duration);
    virtual void sendAudio(uint8_t strength, uint16_t duration);

  protected:

    void sendCommand(uint8_t func, uint8_t strength, uint16_t duration);

    uint8_t mId;
    uint16_t* mTransmitTimings;
    uint16_t mMaxTransmitTimings;
};

#endif

class PaiPaitek : public ZapMe {
  /*
   * PaiPaiTek shockcollar (Euro) uses 5 bytes of data per command:
   *
   *  | Channel (4 bit) | Function (4 bit) | ID (16-bit) | Strength (8 bit) | Checksum (8 bit) |
   *
   * The ID is simply an identifier randomly assigned to each remote.
   * Collars can be set to learning mode to be associated with a new ID.
   *
   * The channel is either 8, 15 or 10, mapping to CH1, CH2 or CH3 on the remote.
   * For the `setChannel` API, we map these to 1, 2 and 3 for simplicity.
   *
   * The function is an integer that is either
   *   1 (Shock)
   *   2 (Vibration) or
   *   4 (Sound).
   *
   * The strength is an integer in range 1 to 100. It is always 0 for sound.
   *
   * The checksum consists of 4 bits for the function and 4 bits for channel.
   * Function (upper 4 bit) sum is 11 for Vib, 13 for Sound or 7 for Shock.
   * Channel (lower 4 bit) sum is 14 for CH1, 0 for CH2 and 5 for CH3.
   *
   * The Preambel is 4000 long low / 1480 High / 960 Low.
   *
   * The Ending is 240 High / 2000 Low.
   *
   * The Timing is 250 -> 0 / 750 -> 1 / Complete 1000.
   */

  public:
    PaiPaitek(uint8_t transmitPin, uint16_t id)
      : ZapMe(transmitPin), mId(id), mChannel(0) {
        /*
         * In transmit timings, we need 3 timings for the sync preamble,
         * then 64 timings to send 40 bit of data, then 3 trailing zeros
         * and one for the terminating null byte we are going to use as
         * a length indicator.
         */
        mMaxTransmitTimings = 3 + 2*40 + 3 + 1;
        mTransmitTimings = new uint16_t[mMaxTransmitTimings];
      }

    void setId(uint16_t id) { mId = id; }
    void setId(uint8_t hid, uint8_t lid) { mId = (hid << 8) + lid; }

    void setChannel(uint8_t ch) { mChannel = ch; }

    virtual void sendShock(uint8_t strength, uint16_t duration);
    virtual void sendVibration(uint8_t strength, uint16_t duration);
    virtual void sendAudio(uint8_t strength, uint16_t duration);
    virtual void sendtest(uint16_t duration);

  protected:

    void sendCommand(uint8_t func, uint8_t strength, uint16_t duration);

    uint16_t mId;
    uint8_t mChannel;
    uint16_t* mTransmitTimings;
    uint16_t mMaxTransmitTimings;
};
