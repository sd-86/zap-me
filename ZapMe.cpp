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

void DogTronic::sendShock(uint8_t strength, uint16_t duration) {
  DEBUG_ZAPME_MSGLN("DogTronic::sendShock");
  sendCommand(0, strength, duration);
}

void DogTronic::sendVibration(uint8_t strength /* unused */, uint16_t duration) {
  DEBUG_ZAPME_MSGLN("DogTronic::sendVibration");
  sendCommand(0, 0, duration);
}

void DogTronic::sendAudio(uint8_t strength /* unused */, uint16_t duration) {
  DEBUG_ZAPME_MSGLN("DogTronic::sendAudio");
  sendCommand(0, 0, duration);
}

void DogTronic::sendCommand(uint8_t func /* unused */, uint8_t strength, uint16_t duration) {
  const uint16_t pulseLength = 2212;
  const uint16_t oneGap = 8144;
  const uint16_t zeroGap = 4012;
  const uint16_t endGap = 64000;

  const uint16_t checksum_base = 4; // 0b0100
  const uint8_t unknown_const = 2;  // 0b10

  uint16_t transmitIdx = 0;

  /* This is the sync preamble */
  mTransmitTimings[transmitIdx++] = 240;
  mTransmitTimings[transmitIdx++] = 1700;
  for (uint8_t pi = 0; pi < 14; ++pi) {
    mTransmitTimings[transmitIdx++] = 240;
    mTransmitTimings[transmitIdx++] = 776;
  }
  mTransmitTimings[transmitIdx++] = 388;
  mTransmitTimings[transmitIdx++] = pulseLength;

  /* Translate l bits to timings */
  #define TRBITS_DT(b,l)                              \
    for (uint8_t k = l - 1; k <= l - 1; --k) {        \
      uint16_t bitGap = zeroGap;                      \
      if (((b) & ( 1 << k )) >> k)                    \
        bitGap = oneGap;                              \
      mTransmitTimings[transmitIdx++] = bitGap;       \
      mTransmitTimings[transmitIdx++] = pulseLength;  \
    };

  uint16_t command = 0;

  /*
     b15 b14 b13 b12 b11 b10 b9 b8 b7 b6 b5 b4 b3 b2 b1 b0
     |-------- id ---------| |-- str --| |-2-| |-- chk --|
  */

  /* First 6 bits are the ID */
  command |= (mId << 10);

  /* We now need to convert the strength from MSB to LSB */
  if (strength & 1)
    command |= 1 << 9;

  if (strength & (1 << 1))
    command |= 1 << 8;

  if (strength & (1 << 2))
    command |= 1 << 7;

  if (strength & (1 << 3))
    command |= 1 << 6;

  /* Now we have the unknown 2-bit constant */
  command |= unknown_const << 4;

  /* Calculate the checksum from the base and strength */
  uint16_t checksum = (checksum_base + strength) % 16;

  /* If an overflow happened, feed the overflow back into the sum */
  checksum += (checksum_base + strength) >> 4;

  /* Now write the checksum with bits flipped pairwise */
  if (checksum & 1)
    command |= 1 << 1;

  if (checksum & (1 << 1))
    command |= 1 << 0;

  if (checksum & (1 << 2))
    command |= 1 << 3;

  if (checksum & (1 << 3))
    command |= 1 << 2;


  DEBUG_ZAPME_MSG("The following command will be encoded: ");
  DEBUG_ZAPME_MSGLN(command);

  // Translate bits to timings
  TRBITS_DT(command, 16);

  mTransmitTimings[transmitIdx++] = endGap;

  /* Null terminate */
  mTransmitTimings[transmitIdx++] = 0;

#ifdef DEBUG_ZAPME
  DEBUG_ZAPME_MSG("The following timings will be transmitted: ");
  for (uint32_t tidx = 0; mTransmitTimings[tidx]; ++tidx) {
    DEBUG_ZAPME_MSG(mTransmitTimings[tidx]);
    DEBUG_ZAPME_MSG(",");
  }
  DEBUG_ZAPME_MSGLN("0");
#endif

  DEBUG_ZAPME_MSG("Starting transmission...");

  uint32_t startTime = millis();

  do {
    /* Transmit timings */
    sendTiming(mTransmitTimings);
    DEBUG_ZAPME_MSG(".");
  } while(millis() - startTime < duration);

  DEBUG_ZAPME_MSGLN(" complete.");
}


void PaiPaitek::sendShock(uint8_t strength, uint16_t duration) {
  DEBUG_ZAPME_MSGLN("PaiPaitek::sendShock");
  sendCommand(1, strength, duration);
}

void PaiPaitek::sendVibration(uint8_t strength, uint16_t duration) {
  DEBUG_ZAPME_MSGLN("PaiPaitek::sendVibration");
  sendCommand(2, strength, duration);
}

void PaiPaitek::sendAudio(uint8_t strength /* unused */, uint16_t duration) {
  DEBUG_ZAPME_MSGLN("PaiPaitek::sendAudio");
  sendCommand(4, 0, duration);
}

void PaiPaitek::sendCommand(uint8_t func, uint8_t strength, uint16_t duration) {
  const uint16_t pulseLength = 1000;
  const uint16_t zeroLength = 250;
  const uint16_t oneLength = 750;

  uint16_t transmitIdx = 0;
  uint8_t chSum = 0;

  /* This is the sync preamble */
  mTransmitTimings[transmitIdx++] = 4000;
  mTransmitTimings[transmitIdx++] = 1440;
  mTransmitTimings[transmitIdx++] = 980;

  // Channel translation
  switch (mChannel) {
    case 1: TRBITS(8, 4); chSum = 14; break; // Ch1
    case 2: TRBITS(15, 4); chSum = 0; break; // Ch2
    case 3: TRBITS(10, 4); chSum = 5; break; // Ch3
  }

  TRBITS(func, 4);
  TRBITS(mId, 16);
  TRBITS(strength, 8);

  // Function Checksum
  switch (func) {
    case 1: TRBITS(7, 4); break;  // Shock
    case 2: TRBITS(11, 4); break; // Vibration
    case 4: TRBITS(13, 4); break; // Sound
  }

  // Channel checksum
  TRBITS(chSum, 4);

  // Trail
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

