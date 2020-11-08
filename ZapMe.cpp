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

#include "ZapMe.h"

void ZapMe::sendTiming(uint16_t* timings) {
  if (!timings)
    return;

  /* We start at low */
  bool tlev = false;

  /* Loop until we hit NULL in the timings data */
  for (uint32_t tidx = 0; timings[tidx]; ++tidx, tlev=!tlev) {
    digitalWrite(mTransmitPin, tlev ? HIGH : LOW);
    delayMicroseconds(timings[tidx]);
  }

  /* Always pull down before leaving */
  digitalWrite(mTransmitPin, LOW);
}

void CH8803::sendShock(uint8_t strength, uint16_t duration) {
  DEBUG_ZAPME_MSGLN("CH8803::sendShock");
  sendCommand(1, strength, duration);
}

void CH8803::sendVibration(uint8_t strength, uint16_t duration) {
  DEBUG_ZAPME_MSGLN("CH8803::sendVibration");
  sendCommand(2, strength, duration);
}

void CH8803::sendAudio(uint8_t strength /* unused */, uint16_t duration) {
  DEBUG_ZAPME_MSGLN("CH8803::sendAudio");
  sendCommand(3, 0, duration);
}

void CH8803::sendCommand(uint8_t func, uint8_t strength, uint16_t duration) {
  /* Calculate checksum */
  uint8_t checksum = 0;
  checksum += (uint8_t)(mId >> 8);
  checksum += (uint8_t)(mId & 0xFF);
  checksum += mChannel;
  checksum += func;
  checksum += strength;

  const uint16_t pulseLength = 1016;
  const uint16_t zeroLength = 292;
  const uint16_t oneLength = 804;

  uint16_t transmitIdx = 0;

  /* This is the sync preamble */
  mTransmitTimings[transmitIdx++] = 840;
  mTransmitTimings[transmitIdx++] = 1440;
  mTransmitTimings[transmitIdx++] = pulseLength - zeroLength;

  /*
   * We can now translate bits into timings as follows:
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

  TRBITS(mId, 16);
  TRBITS(mChannel, 4);
  TRBITS(func, 4);
  TRBITS(strength, 8);
  TRBITS(checksum, 8);

  // Trail, which is 3 zeros, but the third zero has a longer pulse
  TRBITS((uint8_t)0, 2);
  mTransmitTimings[transmitIdx++] = zeroLength;
  mTransmitTimings[transmitIdx++] = 1476;

  if (mMaxTransmitTimings <= transmitIdx) {
    // This should never happen
    DEBUG_ZAPME_MSG("ERROR: Exceeding maximum allocated transmit timings: ");
    DEBUG_ZAPME_MSG(transmitIdx);
    DEBUG_ZAPME_MSGLN(mMaxTransmitTimings);
    return;
  }

  /* Null terminate */
  mTransmitTimings[transmitIdx++] = 0;

  uint32_t startTime = millis();

  #ifdef DEBUG_ZAPME
  DEBUG_ZAPME_MSG("The following timings will be transmitted: ");
  for (uint32_t tidx = 0; mTransmitTimings[tidx]; ++tidx) {
    DEBUG_ZAPME_MSG(mTransmitTimings[tidx]);
    DEBUG_ZAPME_MSG(",");
  }
  DEBUG_ZAPME_MSGLN("0");
  #endif

  DEBUG_ZAPME_MSG("Starting transmission...");

  do {
    /* Transmit timings */
    sendTiming(mTransmitTimings);
    DEBUG_ZAPME_MSG(".");
  } while(millis() - startTime < duration);

  DEBUG_ZAPME_MSGLN(" complete.");
}
