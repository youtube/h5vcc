/*
 * Copyright 2012 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SRC_LB_USER_INPUT_DEVICE_H_
#define SRC_LB_USER_INPUT_DEVICE_H_

#include <inttypes.h>

#include "Platform.h"  // Needed for the OS() macro in KeyboardCodes.h
#include "external/chromium/base/time.h"
#include "external/chromium/third_party/WebKit/Source/WebCore/platform/chromium/KeyboardCodes.h"

class LBWebViewHost;

// see comments within KeyboardCodes.h, but several of these
// had to be captured by debugging test_shell in windows
static const uint8_t AsciiToWindowsKeyCode[128] = {
  /* 00 NUL */ 0,                        /* 01 SOH */ 0,
  /* 02 STX */ 0,                        /* 03 ETX */ 0,
  /* 04 EOT */ 0,                        /* 05 ENQ */ 0,
  /* 06 ACK */ 0,                        /* 07 BEL */ 0,
  /* 08 BS  */ WebCore::VKEY_BACK,       /* 09 TAB */ WebCore::VKEY_TAB,
  /* 0A LF  */ WebCore::VKEY_RETURN,     /* 0B VT  */  0,
  /* 0C FF  */ 0,                        /* 0D CR  */ WebCore::VKEY_RETURN,
  /* 0E SO  */ 0,                        /* OF SI  */ 0,

  /* 10 DLE */ 0,                        /* 11 DC1 */ 0,
  /* 12 DC2 */ 0,                        /* 13 DC3 */ 0,
  /* 14 DC4 */ 0,                        /* 15 NAK */ 0,
  /* 16 SYN */ 0,                        /* 17 ETB */ 0,
  /* 18 CAN */ 0,                        /* 19 EM  */ 0,
  /* 1A SUB */ 0,                        /* 1B ESC */ WebCore::VKEY_ESCAPE,
  /* 1C FS  */ 0,                        /* 1D GS  */ 0,
  /* 1E RS  */ 0,                        /* 1F US  */ 0,

  /* 20 SPC */ WebCore::VKEY_SPACE,      /* 21 !   */ WebCore::VKEY_1,
  /* 22 "   */ WebCore::VKEY_OEM_7,      /* 23 #   */ WebCore::VKEY_3,
  /* 24 $   */ WebCore::VKEY_4,          /* 25 %   */ WebCore::VKEY_5,
  /* 26 &   */ WebCore::VKEY_7,          /* 27 '   */ WebCore::VKEY_OEM_7,
  /* 28 (   */ WebCore::VKEY_9,          /* 29 )   */ WebCore::VKEY_0,
  /* 2A *   */ WebCore::VKEY_8,          /* 2B +   */ WebCore::VKEY_OEM_PLUS,
  /* 2C ,   */ WebCore::VKEY_OEM_COMMA,  /* 2D -   */ WebCore::VKEY_OEM_MINUS,
  /* 2E .   */ WebCore::VKEY_OEM_PERIOD, /* 2F /   */ WebCore::VKEY_OEM_2,

  /* 30 0   */ WebCore::VKEY_0,          /* 31 1   */ WebCore::VKEY_1,
  /* 32 2   */ WebCore::VKEY_2,          /* 33 3   */ WebCore::VKEY_3,
  /* 34 4   */ WebCore::VKEY_4,          /* 35 5   */ WebCore::VKEY_5,
  /* 36 6   */ WebCore::VKEY_6,          /* 37 7   */ WebCore::VKEY_7,
  /* 38 8   */ WebCore::VKEY_8,          /* 39 9   */ WebCore::VKEY_9,
  /* 3A :   */ WebCore::VKEY_OEM_1,      /* 3B ;   */ WebCore::VKEY_OEM_1,
  /* 3C <   */ WebCore::VKEY_OEM_COMMA,  /* 3D =   */ WebCore::VKEY_OEM_PLUS,
  /* 3E >   */ WebCore::VKEY_OEM_PERIOD, /* 3F ?   */ WebCore::VKEY_OEM_2,

  /* 40 @   */ WebCore::VKEY_2,          /* 41 A   */ WebCore::VKEY_A,
  /* 42 B   */ WebCore::VKEY_B,          /* 43 C   */ WebCore::VKEY_C,
  /* 44 D   */ WebCore::VKEY_D,          /* 45 E   */ WebCore::VKEY_E,
  /* 46 F   */ WebCore::VKEY_F,          /* 47 G   */ WebCore::VKEY_G,
  /* 48 H   */ WebCore::VKEY_H,          /* 49 I   */ WebCore::VKEY_I,
  /* 4A J   */ WebCore::VKEY_J,          /* 4B K   */ WebCore::VKEY_K,
  /* 4C L   */ WebCore::VKEY_L,          /* 4D M   */ WebCore::VKEY_M,
  /* 4E N   */ WebCore::VKEY_N,          /* 4F O   */ WebCore::VKEY_O,

  /* 50 P   */ WebCore::VKEY_P,          /* 51 Q   */ WebCore::VKEY_Q,
  /* 52 R   */ WebCore::VKEY_R,          /* 53 S   */ WebCore::VKEY_S,
  /* 54 T   */ WebCore::VKEY_T,          /* 55 U   */ WebCore::VKEY_U,
  /* 56 V   */ WebCore::VKEY_V,          /* 57 W   */ WebCore::VKEY_W,
  /* 58 X   */ WebCore::VKEY_X,          /* 59 Y   */ WebCore::VKEY_Y,
  /* 5A Z   */ WebCore::VKEY_Z,          /* 5B [   */ WebCore::VKEY_OEM_4,
  /* 5C \   */ WebCore::VKEY_OEM_5,      /* 5D ]   */ WebCore::VKEY_OEM_6,
  /* 5E ^   */ WebCore::VKEY_6,          /* 5F _   */ WebCore::VKEY_OEM_MINUS,

  /* 60 `   */ WebCore::VKEY_OEM_3,      /* 61 a   */ WebCore::VKEY_A,
  /* 62 b   */ WebCore::VKEY_B,          /* 63 c   */ WebCore::VKEY_C,
  /* 64 d   */ WebCore::VKEY_D,          /* 65 e   */ WebCore::VKEY_E,
  /* 66 f   */ WebCore::VKEY_F,          /* 67 g   */ WebCore::VKEY_G,
  /* 68 h   */ WebCore::VKEY_H,          /* 69 i   */ WebCore::VKEY_I,
  /* 6A j   */ WebCore::VKEY_J,          /* 6B k   */ WebCore::VKEY_K,
  /* 6C l   */ WebCore::VKEY_L,          /* 6D m   */ WebCore::VKEY_M,
  /* 6E n   */ WebCore::VKEY_N,          /* 6F o   */ WebCore::VKEY_O,

  /* 70 p   */ WebCore::VKEY_P,          /* 71 q   */ WebCore::VKEY_Q,
  /* 72 r   */ WebCore::VKEY_R,          /* 73 s   */ WebCore::VKEY_S,
  /* 74 t   */ WebCore::VKEY_T,          /* 75 u   */ WebCore::VKEY_U,
  /* 76 v   */ WebCore::VKEY_V,          /* 77 w   */ WebCore::VKEY_W,
  /* 78 x   */ WebCore::VKEY_X,          /* 79 y   */ WebCore::VKEY_Y,
  /* 7A z   */ WebCore::VKEY_Z,          /* 7B {   */ WebCore::VKEY_OEM_4,
  /* 7C |   */ WebCore::VKEY_OEM_5,      /* 7D }   */ WebCore::VKEY_OEM_6,
  /* 7E ~   */ WebCore::VKEY_OEM_3,      /* 7F DEL */ WebCore::VKEY_DELETE
};

// this abstract base class represents character-driven user input to the system
class LBUserInputDevice {
 public:
  explicit LBUserInputDevice(LBWebViewHost* view) : view_(view) {}
  virtual ~LBUserInputDevice() {}
  // tells the device to update state and send any appropriate events
  virtual void Poll() = 0;

 protected:
  LBWebViewHost * view_;
};

#endif  // SRC_LB_USER_INPUT_DEVICE_H_
