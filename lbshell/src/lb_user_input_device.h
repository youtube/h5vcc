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

#ifndef SRC_LB_SHELL_LB_USER_INPUT_DEVICE_H_
#define SRC_LB_SHELL_LB_USER_INPUT_DEVICE_H_

#include "Platform.h"  // Needed for the OS() macro in KeyboardCodes.h
#include "external/chromium/third_party/WebKit/Source/WebCore/platform/chromium/KeyboardCodes.h"

using namespace WebCore;

// see comments within KeyboardCodes.h, but several of these
// had to be captured by debugging test_shell in windows
static const uint8_t AsciiToWindowsKeyCode[128] = {
  /* 00 NUL */ 0,               /* 01 SOH */ 0,
  /* 02 STX */ 0,               /* 03 ETX */ 0,
  /* 04 EOT */ 0,               /* 05 ENQ */ 0,
  /* 06 ACK */ 0,               /* 07 BEL */ 0,
  /* 08 BS  */ VKEY_BACK,       /* 09 TAB */ VKEY_TAB,
  /* 0A LF  */ VKEY_RETURN,     /* 0B VT  */  0,
  /* 0C FF  */ 0,               /* 0D CR  */ VKEY_RETURN,
  /* 0E SO  */ 0,               /* OF SI  */ 0,

  /* 10 DLE */ 0,               /* 11 DC1 */ 0,
  /* 12 DC2 */ 0,               /* 13 DC3 */ 0,
  /* 14 DC4 */ 0,               /* 15 NAK */ 0,
  /* 16 SYN */ 0,               /* 17 ETB */ 0,
  /* 18 CAN */ 0,               /* 19 EM  */ 0,
  /* 1A SUB */ 0,               /* 1B ESC */ VKEY_ESCAPE,
  /* 1C FS  */ 0,               /* 1D GS  */ 0,
  /* 1E RS  */ 0,               /* 1F US  */ 0,

  /* 20 SPC */ VKEY_SPACE,      /* 21 !   */ VKEY_1,
  /* 22 "   */ VKEY_OEM_7,      /* 23 #   */ VKEY_3,
  /* 24 $   */ VKEY_4,          /* 25 %   */ VKEY_5,
  /* 26 &   */ VKEY_7,          /* 27 '   */ VKEY_OEM_7,
  /* 28 (   */ VKEY_9,          /* 29 )   */ VKEY_0,
  /* 2A *   */ VKEY_8,          /* 2B +   */ VKEY_OEM_PLUS,
  /* 2C ,   */ VKEY_OEM_COMMA,  /* 2D -   */ VKEY_OEM_MINUS,
  /* 2E .   */ VKEY_OEM_PERIOD, /* 2F /   */ VKEY_OEM_2,

  /* 30 0   */ VKEY_0,          /* 31 1   */ VKEY_1,
  /* 32 2   */ VKEY_2,          /* 33 3   */ VKEY_3,
  /* 34 4   */ VKEY_4,          /* 35 5   */ VKEY_5,
  /* 36 6   */ VKEY_6,          /* 37 7   */ VKEY_7,
  /* 38 8   */ VKEY_8,          /* 39 9   */ VKEY_9,
  /* 3A :   */ VKEY_OEM_1,      /* 3B ;   */ VKEY_OEM_1,
  /* 3C <   */ VKEY_OEM_COMMA,  /* 3D =   */ VKEY_OEM_PLUS,
  /* 3E >   */ VKEY_OEM_PERIOD, /* 3F ?   */ VKEY_OEM_2,

  /* 40 @   */ VKEY_2,          /* 41 A   */ VKEY_A,
  /* 42 B   */ VKEY_B,          /* 43 C   */ VKEY_C,
  /* 44 D   */ VKEY_D,          /* 45 E   */ VKEY_E,
  /* 46 F   */ VKEY_F,          /* 47 G   */ VKEY_G,
  /* 48 H   */ VKEY_H,          /* 49 I   */ VKEY_I,
  /* 4A J   */ VKEY_J,          /* 4B K   */ VKEY_K,
  /* 4C L   */ VKEY_L,          /* 4D M   */ VKEY_M,
  /* 4E N   */ VKEY_N,          /* 4F O   */ VKEY_O,

  /* 50 P   */ VKEY_P,          /* 51 Q   */ VKEY_Q,
  /* 52 R   */ VKEY_R,          /* 53 S   */ VKEY_S,
  /* 54 T   */ VKEY_T,          /* 55 U   */ VKEY_U,
  /* 56 V   */ VKEY_V,          /* 57 W   */ VKEY_W,
  /* 58 X   */ VKEY_X,          /* 59 Y   */ VKEY_Y,
  /* 5A Z   */ VKEY_Z,          /* 5B [   */ VKEY_OEM_4,
  /* 5C \   */ VKEY_OEM_5,      /* 5D ]   */ VKEY_OEM_6,
  /* 5E ^   */ VKEY_6,          /* 5F _   */ VKEY_OEM_MINUS,

  /* 60 `   */ VKEY_OEM_3,      /* 61 a   */ VKEY_A,
  /* 62 b   */ VKEY_B,          /* 63 c   */ VKEY_C,
  /* 64 d   */ VKEY_D,          /* 65 e   */ VKEY_E,
  /* 66 f   */ VKEY_F,          /* 67 g   */ VKEY_G,
  /* 68 h   */ VKEY_H,          /* 69 i   */ VKEY_I,
  /* 6A j   */ VKEY_J,          /* 6B k   */ VKEY_K,
  /* 6C l   */ VKEY_L,          /* 6D m   */ VKEY_M,
  /* 6E n   */ VKEY_N,          /* 6F o   */ VKEY_O,

  /* 70 p   */ VKEY_P,          /* 71 q   */ VKEY_Q,
  /* 72 r   */ VKEY_R,          /* 73 s   */ VKEY_S,
  /* 74 t   */ VKEY_T,          /* 75 u   */ VKEY_U,
  /* 76 v   */ VKEY_V,          /* 77 w   */ VKEY_W,
  /* 78 x   */ VKEY_X,          /* 79 y   */ VKEY_Y,
  /* 7A z   */ VKEY_Z,          /* 7B {   */ VKEY_OEM_4,
  /* 7C |   */ VKEY_OEM_5,      /* 7D }   */ VKEY_OEM_6,
  /* 7E ~   */ VKEY_OEM_3,      /* 7F DEL */ VKEY_DELETE
};

#endif  // SRC_PLATFORM_WIIU_LB_SHELL_LB_USER_INPUT_DEVICE_H_
