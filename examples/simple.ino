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

#include <ZapMe.h>

// Create a new collar using transmission pin 10 and a zero id
CH8803 collar = CH8803(10, 0);

void setup() {
  // Set an ID. You can either randomly hardcode a unique 16-bit ID here
  // or you sniff the ID of your remote and add the two bytes here.
  //
  // Since this device can be learned to listen for your ID, there is no
  // real need to sniff your remote ID though, unless you want to use your
  // hardware remote at the same time as the Arduino remote.
  collar.setId(13, 37);
}

void loop() {
  // Send weakest and strongest vibration pulse for 2 seconds each
  collar.sendVibration(1, 2000);
  collar.sendVibration(99, 2000);

  // Send one audio warning (250ms is enough for one beep iteration)
  collar.sendAudio(0, 250);
  delay(2000);

  // Send shock on level 1
  collar.sendShock(1, 500);
  delay(3000);
}
