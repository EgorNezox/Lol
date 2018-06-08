#ifndef GKEYCODE_H
#define GKEYCODE_H
/*************************************************************

  Keycodes for special keys returned by GSimKbGet()

  The bits GK_ALT, GK_SHIFT and GK_CTRL indicates if these
  modifier keys where pressed at the time the basic key where
  pressed.
  The modifier code is or'ed with the normal ASCII key or special key.

   <Alt><F1> = GK_F1|GK_ALT
   <Ctrl><a> = 'a'|GK_CTRL

   Note that the shift state is included with the shifted ASCII value:
   Lower case 'a'
   <a> = 'a'  or = 'a' & GK_KEY_MASK
   Upper case 'A'
   <Shift><a> = 'A'|GK_SHIFT or = 'A' & GK_KEY_MASK

   --

   Note the keyboard scan values may change over time depending
   on changes in the PC keyboard type / OS system version.

   Therefore you may use the LCD simulator to check the key press
   values below with the specific PC (windows version).
   In the simulator <Setup> menu tick 'Show Key Codes' to see
   key code values in the Debug Info window.

   Revision date:     04-12-2017
   Revision Purpose:  Keycode update for lcdserv.exe version 3.1
                      Separate value for main keys and keypad keys
                      now passed through.

   Version number: 2.5
   Copyright (c) RAMTEX International Aps 2001-2017
   Web site, support and upgrade: www.ramtex.dk


************************************************************/

/* Return keycode when no keys is pending */
#define  GK_NOKEY           0x0000

/* Special keys */
#define  GK_PAGEUP          0x016E
#define  GK_PAGEDOWN        0x016F
#define  GK_END             0x0138
#define  GK_HOME            0x0139
#define  GK_ARROW_LEFT      0x013A
#define  GK_ARROW_UP        0x013B
#define  GK_ARROW_RIGHT     0x013C
#define  GK_ARROW_DOWN      0x013D
#define  GK_DELETE          0x007F
#define  GK_INSERT          0x0142

#define  GK_PAD_HOME        0x0177
#define  GK_PAD_ARROW_LEFT  0x0178
#define  GK_PAD_ARROW_UP    0x0179
#define  GK_PAD_ARROW_RIGHT 0x017A
#define  GK_PAD_ARROW_DOWN  0x017B
#define  GK_PAD_PAGEUP      0x017C
#define  GK_PAD_PAGEDOWN    0x017D
#define  GK_PAD_END         0x017E

#define  GK_PAD_INC         0x0180
#define  GK_PAD_DEL         0x0181
#define  GK_PAD_MULT        0x0183
#define  GK_PAD_PLUS        0x0184
#define  GK_PAD_MINUS       0x0186
#define  GK_PAD_SLASH       0x0188

#define  GK_ENTER           0x000D
#define  GK_PAD_ENTER       0x0172

#define  GK_ESC             0x001B
#define  GK_TAB             0x0009

#define  GK_F1              0x0154
#define  GK_F2              0x0155
#define  GK_F3              0x0156
#define  GK_F4              0x0157
#define  GK_F5              0x0158
#define  GK_F6              0x0159
#define  GK_F7              0x015A
#define  GK_F8              0x015B
#define  GK_F9              0x015C
#define  GK_F10             0x015D
#define  GK_F11             0x015E
#define  GK_F12             0x015F

/* Key modifiers, OR'ed to the basic key codes,
   One or more key modifiers can be active at a time */
#define  GK_ALT             0x4000
#define  GK_CTRL            0x2000
#define  GK_SHIFT           0x1000

/* Key code mask, remove modifiers */
#define  GK_KEY_MASK        0x0FFF


#endif
