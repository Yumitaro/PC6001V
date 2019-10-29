#include <stdlib.h>

#include <fstream>
#include <algorithm>

#include "pc6001v.h"

#include "config.h"
#include "common.h"
#include "error.h"




const std::filesystem::path DummtPath = "";

static const std::vector<P6KeyName> P6KeyNameDef = {
	{ KP6_UNKNOWN,		"K6_UNKNOWN" },
	
	{ KP6_1,			"K6_1" },
	{ KP6_2,			"K6_2" },
	{ KP6_3,			"K6_3" },
	{ KP6_4,			"K6_4" },
	{ KP6_5,			"K6_5" },
	{ KP6_6,			"K6_6" },
	{ KP6_7,			"K6_7" },
	{ KP6_8,			"K6_8" },
	{ KP6_9,			"K6_9" },
	{ KP6_0,			"K6_0" },
	
	{ KP6_A,			"K6_A" },
	{ KP6_B,			"K6_B" },
	{ KP6_C,			"K6_C" },
	{ KP6_D,			"K6_D" },
	{ KP6_E,			"K6_E" },
	{ KP6_F,			"K6_F" },
	{ KP6_G,			"K6_G" },
	{ KP6_H,			"K6_H" },
	{ KP6_I,			"K6_I" },
	{ KP6_J,			"K6_J" },
	{ KP6_K,			"K6_K" },
	{ KP6_L,			"K6_L" },
	{ KP6_M,			"K6_M" },
	{ KP6_N,			"K6_N" },
	{ KP6_O,			"K6_O" },
	{ KP6_P,			"K6_P" },
	{ KP6_Q,			"K6_Q" },
	{ KP6_R,			"K6_R" },
	{ KP6_S,			"K6_S" },
	{ KP6_T,			"K6_T" },
	{ KP6_U,			"K6_U" },
	{ KP6_V,			"K6_V" },
	{ KP6_W,			"K6_W" },
	{ KP6_X,			"K6_X" },
	{ KP6_Y,			"K6_Y" },
	{ KP6_Z,			"K6_Z" },
	
	{ KP6_F1,			"K6_F1" },
	{ KP6_F2,			"K6_F2" },
	{ KP6_F3,			"K6_F3" },
	{ KP6_F4,			"K6_F4" },
	{ KP6_F5,			"K6_F5" },
	
	{ KP6_MINUS,		"K6_MINUS" },
	{ KP6_CARET,		"K6_CARET" },
	{ KP6_YEN,			"K6_YEN" },
	{ KP6_AT,			"K6_AT" },
	{ KP6_LBRACKET,		"K6_LBRACKET" },
	{ KP6_RBRACKET,		"K6_RBRACKET" },
	{ KP6_SEMICOLON,	"K6_SEMICOLON" },
	{ KP6_COLON,		"K6_COLON" },
	{ KP6_COMMA,		"K6_COMMA" },
	{ KP6_PERIOD,		"K6_PERIOD" },
	{ KP6_SLASH,		"K6_SLASH" },
	{ KP6_UNDERSCORE,	"K6_UNDERSCORE" },
	{ KP6_SPACE,		"K6_SPACE" },
	
	{ KP6_ESC,			"K6_ESC" },
	{ KP6_TAB,			"K6_TAB" },
	{ KP6_CTRL,			"K6_CTRL" },
	{ KP6_SHIFT,		"K6_SHIFT" },
	{ KP6_GRAPH,		"K6_GRAPH" },
	{ KP6_HOME,			"K6_HOME" },
	{ KP6_STOP,			"K6_STOP" },
	{ KP6_PAGE,			"K6_PAGE" },
	{ KP6_RETURN,		"K6_RETURN" },
	{ KP6_KANA,			"K6_KANA" },
	{ KP6_INS,			"K6_INS" },
	{ KP6_DEL,			"K6_DEL" },
	
	{ KP6_UP,			"K6_UP" },
	{ KP6_DOWN,			"K6_DOWN" },
	{ KP6_LEFT,			"K6_LEFT" },
	{ KP6_RIGHT,		"K6_RIGHT" },
	
	{ KP6_MODE,			"K6_MODE" },
	{ KP6_CAPS,			"K6_CAPS" },
	
	
	// テンキー部拡張
	{ KP6_P0,			"K6_P0" },
	{ KP6_P1,			"K6_P1" },
	{ KP6_P2,			"K6_P2" },
	{ KP6_P3,			"K6_P3" },
	{ KP6_P4,			"K6_P4" },
	{ KP6_P5,			"K6_P5" },
	{ KP6_P6,			"K6_P6" },
	{ KP6_P7,			"K6_P7" },
	{ KP6_P8,			"K6_P8" },
	{ KP6_P9,			"K6_P9" },
	{ KP6_PPLUS,		"K6_PPLUS" },
	{ KP6_PMINUS,		"K6_PMINUS" },
	{ KP6_PMULTIPLY,	"K6_PMULTIPLY" },
	{ KP6_PDIVIDE,		"K6_PDIVIDE" },
	{ KP6_PPERIOD,		"K6_PPERIOD" },
	{ KP6_PRETURN,		"K6_PRETURN" },
	
	
	// 各種機能キー
	{ KFN_1,			"K6_FN1" },
	{ KFN_2,			"K6_FN2" },
	{ KFN_3,			"K6_FN3" },
	{ KFN_4,			"K6_FN4" },
	{ KFN_5,			"K6_FN5" },
	{ KFN_6,			"K6_FN6" },
	{ KFN_7,			"K6_FN7" },
	{ KFN_8,			"K6_FN8" },
	{ KFN_9,			"K6_FN9" }
};


static const std::vector<PCKeyName> PCKeyNameDef = {
	{ KVC_UNKNOWN,		"K_UNKNOWN" },
	
	{ KVC_1,			"K_1" },
	{ KVC_2,			"K_2" },
	{ KVC_3,			"K_3" },
	{ KVC_4,			"K_4" },
	{ KVC_5,			"K_5" },
	{ KVC_6,			"K_6" },
	{ KVC_7,			"K_7" },
	{ KVC_8,			"K_8" },
	{ KVC_9,			"K_9" },
	{ KVC_0,			"K_0" },
	
	{ KVC_A,			"K_A" },
	{ KVC_B,			"K_B" },
	{ KVC_C,			"K_C" },
	{ KVC_D,			"K_D" },
	{ KVC_E,			"K_E" },
	{ KVC_F,			"K_F" },
	{ KVC_G,			"K_G" },
	{ KVC_H,			"K_H" },
	{ KVC_I,			"K_I" },
	{ KVC_J,			"K_J" },
	{ KVC_K,			"K_K" },
	{ KVC_L,			"K_L" },
	{ KVC_M,			"K_M" },
	{ KVC_N,			"K_N" },
	{ KVC_O,			"K_O" },
	{ KVC_P,			"K_P" },
	{ KVC_Q,			"K_Q" },
	{ KVC_R,			"K_R" },
	{ KVC_S,			"K_S" },
	{ KVC_T,			"K_T" },
	{ KVC_U,			"K_U" },
	{ KVC_V,			"K_V" },
	{ KVC_W,			"K_W" },
	{ KVC_X,			"K_X" },
	{ KVC_Y,			"K_Y" },
	{ KVC_Z,			"K_Z" },
	
	{ KVC_F1,			"K_F1" },
	{ KVC_F2,			"K_F2" },
	{ KVC_F3,			"K_F3" },
	{ KVC_F4,			"K_F4" },
	{ KVC_F5,			"K_F5" },
	{ KVC_F6,			"K_F6" },
	{ KVC_F7,			"K_F7" },
	{ KVC_F8,			"K_F8" },
	{ KVC_F9,			"K_F9" },
	{ KVC_F10,			"K_F10" },
	{ KVC_F11,			"K_F11" },
	{ KVC_F12,			"K_F12" },
	
	{ KVC_MINUS,		"K_MINUS" },
	{ KVC_CARET,		"K_CARET" },
	{ KVC_BACKSPACE,	"K_BACKSPACE" },
	{ KVC_AT,			"K_AT" },
	{ KVC_LBRACKET,		"K_LBRACKET" },
	{ KVC_SEMICOLON,	"K_SEMICOLON" },
	{ KVC_COLON,		"K_COLON" },
	{ KVC_COMMA,		"K_COMMA" },
	{ KVC_PERIOD,		"K_PERIOD" },
	{ KVC_SLASH,		"K_SLASH" },
	{ KVC_SPACE,		"K_SPACE" },
	
	{ KVC_ESC,			"K_ESC" },
	{ KVC_HANZEN,		"K_HANZEN" },
	{ KVC_TAB,			"K_TAB" },
	{ KVC_CAPSLOCK,		"K_CAPSLOCK" },
	{ KVC_ENTER,		"K_ENTER" },
	{ KVC_LCTRL,		"K_LCTRL" },
	{ KVC_RCTRL,		"K_RCTRL" },
	{ KVC_LSHIFT,		"K_LSHIFT" },
	{ KVC_RSHIFT,		"K_RSHIFT" },
	{ KVC_LALT,			"K_LALT" },
	{ KVC_RALT,			"K_RALT" },
	{ KVC_PRINT,		"K_PRINT" },
	{ KVC_SCROLLLOCK,	"K_SCROLLLOCK" },
	{ KVC_PAUSE,		"K_PAUSE" },
	{ KVC_INSERT,		"K_INSERT" },
	{ KVC_DELETE,		"K_DELETE" },
	{ KVC_HOME,			"K_HOME" },
	{ KVC_END,			"K_END" },
	{ KVC_PAGEUP,		"K_PAGEUP" },
	{ KVC_PAGEDOWN,		"K_PAGEDOWN" },
	
	{ KVC_UP,			"K_UP" },
	{ KVC_DOWN,			"K_DOWN" },
	{ KVC_LEFT,			"K_LEFT" },
	{ KVC_RIGHT,		"K_RIGHT" },
	
	{ KVC_P0,			"K_P_0" },
	{ KVC_P1,			"K_P_1" },
	{ KVC_P2,			"K_P_2" },
	{ KVC_P3,			"K_P_3" },
	{ KVC_P4,			"K_P_4" },
	{ KVC_P5,			"K_P_5" },
	{ KVC_P6,			"K_P_6" },
	{ KVC_P7,			"K_P_7" },
	{ KVC_P8,			"K_P_8" },
	{ KVC_P9,			"K_P_9" },
	{ KVC_NUMLOCK,		"K_NUMLOCK" },
	{ KVC_P_PLUS,		"K_P_PLUS" },
	{ KVC_P_MINUS,		"K_P_MINUS" },
	{ KVC_P_MULTIPLY,	"K_P_MULTIPLY" },
	{ KVC_P_DIVIDE,		"K_P_DIVIDE" },
	{ KVC_P_PERIOD,		"K_P_PERIOD" },
	{ KVC_P_ENTER,		"K_P_ENTER" },
	
	// 日本語キーボードのみ
	{ KVC_YEN,			"K_YEN" },
	{ KVC_RBRACKET,		"K_RBRACKET" },
	{ KVC_UNDERSCORE,	"K_UNDERSCORE" },
	{ KVC_MUHENKAN,		"K_MUHENKAN" },
	{ KVC_HENKAN,		"K_HENKAN" },
	{ KVC_HIRAGANA,		"K_HIRAGANA" },
	
	// 英語キーボードのみ
	{ KVE_BACKSLASH,	"K_BACKSLASH" },
	
	// 追加キー
	{ KVX_RMETA,		"K_RMETA" },
	{ KVX_LMETA,		"K_LMETA" },
	{ KVX_MENU,			"K_MENU" },
};


static const std::vector<VKeyConv> KeyIni = {	// 仮想キーコード -> P6キーコード定義初期値
	{ KVC_1,			KP6_1 },			// 1	!
	{ KVC_2,			KP6_2 },			// 2	"
	{ KVC_3,			KP6_3 },			// 3	#
	{ KVC_4,			KP6_4 },			// 4	$
	{ KVC_5,			KP6_5 },			// 5	%
	{ KVC_6,			KP6_6 },			// 6	&
	{ KVC_7,			KP6_7 },			// 7	'
	{ KVC_8,			KP6_8 },			// 8	(
	{ KVC_9,			KP6_9 },			// 9	)
	{ KVC_0,			KP6_0 },			// 0
	
	{ KVC_A,			KP6_A },			// a	A
	{ KVC_B,			KP6_B },			// b	B
	{ KVC_C,			KP6_C },			// c	C
	{ KVC_D,			KP6_D },			// d	D
	{ KVC_E,			KP6_E },			// e	E
	{ KVC_F,			KP6_F },			// f	F
	{ KVC_G,			KP6_G },			// g	G
	{ KVC_H,			KP6_H },			// h	H
	{ KVC_I,			KP6_I },			// i	I
	{ KVC_J,			KP6_J },			// j	J
	{ KVC_K,			KP6_K },			// k	K
	{ KVC_L,			KP6_L },			// l	L
	{ KVC_M,			KP6_M },			// m	M
	{ KVC_N,			KP6_N },			// n	N
	{ KVC_O,			KP6_O },			// o	O
	{ KVC_P,			KP6_P },			// p	P
	{ KVC_Q,			KP6_Q },			// q	Q
	{ KVC_R,			KP6_R },			// r	R
	{ KVC_S,			KP6_S },			// s	S
	{ KVC_T,			KP6_T },			// t	T
	{ KVC_U,			KP6_U },			// u	U
	{ KVC_V,			KP6_V },			// v	V
	{ KVC_W,			KP6_W },			// w	W
	{ KVC_X,			KP6_X },			// x	X
	{ KVC_Y,			KP6_Y },			// y	Y
	{ KVC_Z,			KP6_Z },			// z	Z
	
	{ KVC_F1,			KP6_F1 },			// F1
	{ KVC_F2,			KP6_F2 },			// F2
	{ KVC_F3,			KP6_F3 },			// F3
	{ KVC_F4,			KP6_F4 },			// F4
	{ KVC_F5,			KP6_F5 },			// F5
	
	{ KVC_MINUS,		KP6_MINUS },		// -	=
	{ KVC_CARET,		KP6_CARET },		// ^	~
	{ KVC_BACKSPACE,	KP6_DEL },			// BackSpace
	{ KVC_AT,			KP6_AT },			// @	`
	{ KVC_LBRACKET,		KP6_LBRACKET },		// [	{
	{ KVC_SEMICOLON,	KP6_SEMICOLON },	// ;	+
	{ KVC_COLON,		KP6_COLON },		// :	*
	{ KVC_COMMA,		KP6_COMMA },		// ,	<
	{ KVC_PERIOD,		KP6_PERIOD },		// .	>
	{ KVC_SLASH,		KP6_SLASH },		// /	?
	{ KVC_SPACE,		KP6_SPACE },		// Space
	
	{ KVC_ESC,			KP6_ESC },			// ESC
	{ KVC_HANZEN,		KP6_UNKNOWN },		// 半角/全角
	{ KVC_TAB,			KP6_TAB },			// Tab
	{ KVC_CAPSLOCK,		KP6_UNKNOWN },		// CapsLock
	{ KVC_ENTER,		KP6_RETURN },		// Enter
	{ KVC_LCTRL,		KP6_CTRL },			// L-Ctrl
	{ KVC_RCTRL,		KP6_CTRL },			// R-Ctrl
	{ KVC_LSHIFT,		KP6_SHIFT },		// L-Shift
	{ KVC_RSHIFT,		KP6_SHIFT },		// R-Shift
	{ KVC_LALT,			KP6_GRAPH },		// L-Alt
	{ KVC_RALT,			KP6_GRAPH },		// R-Alt
	{ KVC_PRINT,		KP6_UNKNOWN },		// PrintScreen
	{ KVC_SCROLLLOCK,	KP6_CAPS },			// ScrollLock
	{ KVC_PAUSE,		KP6_KANA },			// Pause
	{ KVC_INSERT,		KP6_INS },			// Insert
	{ KVC_DELETE,		KP6_DEL },			// Delete
	{ KVC_END,			KP6_STOP },			// End
	{ KVC_HOME,			KP6_HOME },			// Home
	{ KVC_PAGEUP,		KP6_PAGE },			// PageUp
	{ KVC_PAGEDOWN,		KP6_MODE },			// PageDown
	
	{ KVC_UP,			KP6_UP },			// ↑
	{ KVC_DOWN,			KP6_DOWN },			// ↓
	{ KVC_LEFT,			KP6_LEFT },			// ←
	{ KVC_RIGHT,		KP6_RIGHT },		// →
	
	{ KVC_P0,			KP6_P0 },			// [0]
	{ KVC_P1,			KP6_P1 },			// [1]
	{ KVC_P2,			KP6_P2 },			// [2]
	{ KVC_P3,			KP6_P3 },			// [3]
	{ KVC_P4,			KP6_P4 },			// [4]
	{ KVC_P5,			KP6_P5 },			// [5]
	{ KVC_P6,			KP6_P6 },			// [6]
	{ KVC_P7,			KP6_P7 },			// [7]
	{ KVC_P8,			KP6_P8 },			// [8]
	{ KVC_P9,			KP6_P9 },			// [9]
	{ KVC_NUMLOCK,		KP6_UNKNOWN },		// NumLock
	{ KVC_P_PLUS,		KP6_PPLUS },		// [+]
	{ KVC_P_MINUS,		KP6_PMINUS },		// [-]
	{ KVC_P_MULTIPLY,	KP6_PMULTIPLY },	// [*]
	{ KVC_P_DIVIDE,		KP6_PDIVIDE },		// [/]
	{ KVC_P_PERIOD,		KP6_PPERIOD },		// [.]
	{ KVC_P_ENTER,		KP6_PRETURN },		// [Enter]
	
	// 日本語キーボードのみ
	{ KVC_YEN,			KP6_YEN },			// ￥	|
	{ KVC_RBRACKET,		KP6_RBRACKET },		// ]	}
	{ KVC_UNDERSCORE,	KP6_UNDERSCORE },	// \	_
//	{ KVC_MUHENKAN,		KP6_UNKNOWN },		// 無変換
//	{ KVC_HENKAN,		KP6_UNKNOWN },		// 変換
//	{ KVC_HIRAGANA,		KP6_UNKNOWN },		// ひらがな
	
	// 英語キーボードのみ
	{ KVE_BACKSLASH,	KP6_YEN	 },			// BackSlash	|
	
	// 追加キー
	{ KVX_RMETA,		KP6_UNKNOWN },		// L-Meta
	{ KVX_LMETA,		KP6_UNKNOWN },		// R-Meta
	{ KVX_MENU,			KP6_UNKNOWN }		// Menu
	
	// 各種機能キー (今のところ無効)
//	{ KVC_F6,			KFN_1 },			// F6
//	{ KVC_F7,			KFN_2 },			// F7
//	{ KVC_F8,			KFN_3 },			// F8
//	{ KVC_F9,			KFN_4 },			// F9
//	{ KVC_F10,			KFN_5 },			// F10
//	{ KVC_F11,			KFN_6 },			// F11
//	{ KVC_F12,			KFN_7 }				// F12
};


static const std::vector<COLOR24> STDColor = {	// 標準カラーデータ ( R,G,B,0  0-255 )
				// システムカラー
				{   0,   0,   0, 0 },	// 00:
				{   0,   0, 128, 0 },	// 01:
				{   0, 128,   0, 0 },	// 02:
				{   0, 128, 128, 0 },	// 03:
				{ 128,   0,   0, 0 },	// 04:
				{ 128,   0, 128, 0 },	// 05:
				{ 128, 128,   0, 0 },	// 06:
				{ 128, 128, 128, 0 },	// 07:
				{  64,  64,  64, 0 },	// 08:
				{   0,   0, 255, 0 },	// 09:
				{   0, 255,   0, 0 },	// 10:
				{   0, 255, 255, 0 },	// 11:
				{ 255,   0,   0, 0 },	// 12:
				{ 255,   0, 255, 0 },	// 13:
				{ 255, 255,   0, 0 },	// 14:
				{ 255, 255, 255, 0 },	// 15:
				
				{  30,  30,  30, 0 },	// 16:黒(mode 1,2 ボーダー)
				
				// mode 1
				{  50, 238,  15, 0 },	// 17:緑(Set1)
				{   4, 106,   4, 0 },	// 18:深緑
				{ 252, 100,  35, 0 },	// 19:橙(Set2)
				{ 180,   6,   4, 0 },	// 20:深橙
				// mode 2
				{  50, 238,  15, 0 },	// 21:緑
				{ 234, 224,   4, 0 },	// 22:黄
				{  68,  10, 244, 0 },	// 23:青
				{ 226,   6,  12, 0 },	// 24:赤
				{ 214, 208, 246, 0 },	// 25:白
				{  52, 186, 236, 0 },	// 26:シアン
				{ 244,  10, 244, 0 },	// 27:マゼンタ
				{ 252,  90,   4, 0 },	// 28:橙
				// mode 3
				{  50, 238,  15, 0 },	// 29:緑
				{ 234, 224,   4, 0 },	// 30:黄
				{  68,  10, 244, 0 },	// 31:青
				{ 226,   6,  12, 0 },	// 32:赤
				{ 214, 208, 246, 0 },	// 33:白
				{  52, 186, 236, 0 },	// 34:シアン
				{ 244,  10, 244, 0 },	// 35:マゼンタ
				{ 252,  90,   4, 0 },	// 36:橙
				// mode 4
				{   4, 106,   4, 0 },	// 37:深緑(Set1)
				{  83, 242,  55, 0 },	// 38:緑
				{  30,  30,  30, 0 },	// 39:黒(Set2)
				{ 224, 255, 208, 0 },	// 40:白
				
				{ 146,  82,  13, 0 },	// 41:にじみ 赤(Set1)
				{  28, 202, 121, 0 },	// 42:にじみ 青(Set1)
				{ 120,  60,  95, 0 },	// 43:にじみ 桃(Set1)
				{  80, 190,  80, 0 },	// 44:にじみ 緑(Set1)
				
				{  81, 192,  14, 0 },	// 45:にじみ 明赤(Set1)
				{  69, 130,   9, 0 },	// 46:にじみ 暗赤(Set1)
				{  21, 221,  47, 0 },	// 47:にじみ 明青(Set1)
				{  14, 156,  50, 0 },	// 48:にじみ 暗青(Set1)
				
				{ 113, 210,  15, 0 },	// 49:にじみ 明桃(Set1)
				{  52, 132,  38, 0 },	// 50:にじみ 暗桃(Set1)
				{  70, 200,  60, 0 },	// 51:にじみ 明緑(Set1)
				{   4, 145,  47, 0 },	// 52:にじみ 暗緑(Set1)
				
				{ 255,  48,   0, 0 },	// 53:にじみ 赤(Set2)
				{  38, 201, 255, 0 },	// 54:にじみ 青(Set2)
				{ 255,  0,  236, 0 },	// 55:にじみ 桃(Set2)
				{  38, 255,  92, 0 },	// 56:にじみ 緑(Set2)
				
				{ 255, 140,  64, 0 },	// 57:にじみ 明赤(Set2)
				{ 150,  45,  00, 0 },	// 58:にじみ 暗赤(Set2)
				{ 119, 207, 255, 0 },	// 59:にじみ 明青(Set2)
				{  30,  80, 150, 0 },	// 60:にじみ 暗青(Set2)
				
				{ 255,  64, 131, 0 },	// 61:にじみ 明桃(Set2)
				{ 151,   0, 105, 0 },	// 62:にじみ 暗桃(Set2)
				{ 119, 255, 167, 0 },	// 63:にじみ 明緑(Set2)
				{  30, 151, 100, 0 },	// 64:にじみ 暗緑(Set2)
				
				// mk2
				{  20,  20,  20, 0 },	// 65:透明(黒)
				{ 255, 172,   0, 0 },	// 66:橙
				{   0, 255, 172, 0 },	// 67:青緑
				{ 172, 255,   0, 0 },	// 68:黄緑
				{ 172,   0, 255, 0 },	// 69:青紫
				{ 255,   0, 172, 0 },	// 70:赤紫
				{   0, 172, 255, 0 },	// 71:空色
				{ 172, 172, 172, 0 },	// 72:灰色
				{  20,  20,  20, 0 },	// 73:黒
				{ 255,   0,   0, 0 },	// 74:赤
				{   0, 255,   0, 0 },	// 75:緑
				{ 255, 255,   0, 0 },	// 76:黄
				{   0,   0, 255, 0 },	// 77:青
				{ 255,   0, 255, 0 },	// 78:マゼンタ
				{   0, 255, 255, 0 },	// 79:シアン
				{ 255, 255, 255, 0 },	// 80:白
			};




////////////////////////////////////////////////////////////////
// コンストラクタ
////////////////////////////////////////////////////////////////
CFG6::CFG6( void ) : Caption(""), DokoFile(""),
	RomPath(""), ExtRomPath(""), ExtRomFile(""), WavePath(""),
	TapePath(""), TapeFile(""), SaveFile(""), DiskPath(""),
	DiskFile1(""), DiskFile2(""), ImgPath(""), PrinterFile(""),
	DokoSavePath(""), FontPath("")
{
	// INIファイルのパスを設定
	IniPath = std::filesystem::u8path( FILE_CONFIG );
	AbsolutePath( IniPath );
}


////////////////////////////////////////////////////////////////
// デストラクタ
////////////////////////////////////////////////////////////////
CFG6::~CFG6( void )
{
}


////////////////////////////////////////////////////////////////
// 初期化(INIファイル読込み)
//
// 引数:	なし
// 返値:	bool	true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool CFG6::Init( void )
{
	// INIオブジェクト初期化
	cIni::Init();
	
	try{
		// INIファイルがないならデフォルトで作成
		if( !FileExist( IniPath ) ){
			std::fstream fs;
			
			if( !FSopen( fs, IniPath, std::ios_base::out ) ) throw Error::IniWriteFailed;
			
			// タイトル行を出力して一旦閉じる
			fs << GetText( TINI_TITLE ) << std::endl;
			fs.close();
			
			// INIファイルを開く
			if( !cIni::Read( IniPath ) ) throw Error::IniDefault;
			InitIni( true );	// INIオブジェクト初期値設定(全項目上書き)
			cIni::Write();
		}else{
			// INIファイルを開く
			if( !cIni::Read( IniPath ) ) throw Error::IniDefault;
			InitIni( false );	// INIオブジェクト初期値設定(不足分のみ追加)
		}
	}
	catch( Error::Errno i ){	// 例外発生
		Error::SetError( i );
		return false;
	}
	
	return true;
}


////////////////////////////////////////////////////////////////
// INIファイル書込み
//
// 引数:	なし
// 返値:	bool	true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool CFG6::Write( void )
{
	// INIファイルに書込み
	return cIni::Write();
}


////////////////////////////////////////////////////////////////
// メンバアクセス関数
////////////////////////////////////////////////////////////////

// [CONFIG] ----------------------------------------------------

// 機種取得
int CFG6::GetModel( void )
{
	int st = DEFAULT_MODEL;
	cIni::GetInt( "CONFIG", "Model", &st, st );
	return st;
}

// 機種設定
void CFG6::SetModel( int data )
{
	int st = min( max( MIN_MODEL, data ), MAX_MODEL );
	cIni::PutEntry( "CONFIG", GetText( TINI_Model ), "Model", "%02d", st );
}


// FDD接続台数取得
int CFG6::GetFddNum( void )
{
	int st = DEFAULT_FDD;
	cIni::GetInt( "CONFIG", "FDD", &st, st );
	return st;
}

// FDD接続台数設定
void CFG6::SetFddNum( int data )
{
	int st = min( max( MIN_FDD, data ), MAX_FDD );
	cIni::PutEntry( "CONFIG", GetText( TINI_FDD ), "FDD", "%d", st );
}


// 拡張RAMを使う取得
bool CFG6::GetUseExtRam( void )
{
	bool st = DEFAULT_EXTRAM;
	cIni::GetTruth( "CONFIG", "ExtRam", &st, st );
	return st;
}

// 拡張RAMを使う設定
void CFG6::SetUseExtRam( bool yn )
{
	cIni::PutEntry( "CONFIG", GetText( TINI_ExtRam ), "ExtRam", "%s", yn ? "Yes" : "No" );
}


// オーバークロック率取得
int CFG6::GetOverClock( void )
{
	int st = 100;
	cIni::GetInt( "CONFIG", "OverClock", &st, st );
	return st;
}

// オーバークロック率設定
void CFG6::SetOverClock( int data )
{
	int st = min( max( MIN_OVERCLOCK, data ), MAX_OVERCLOCK );
	cIni::PutEntry( "CONFIG", GetText( TINI_OverClock ), "OverClock", "%d", st );
}


// CRCチェック取得
bool CFG6::GetCheckCRC( void )
{
	bool st = true;
	cIni::GetTruth( "CONFIG", "CheckCRC", &st, st );
	return st;
}
// CRCチェック設定
void CFG6::SetCheckCRC( bool yn )
{
	cIni::PutEntry( "CONFIG", GetText( TINI_CheckCRC ), "CheckCRC", "%s", yn ? "Yes" : "No" );
}


// FDDウェイト有効フラグ取得
bool CFG6::GetFddWaitEnable( void )
{
	bool st = DEFAULT_FDDWAIT;
	cIni::GetTruth( "CONFIG", "FDDWait", &st, st );
	return st;
}

// FDDウェイト有効フラグ設定
void CFG6::SetFddWaitEnable( bool yn )
{
	cIni::PutEntry( "CONFIG", GetText( TINI_FDDWait ), "FDDWait", "%s", yn ? "Yes" : "No" );
}


// [CMT] -------------------------------------------------------

// Turbo TAPE 有効フラグ取得
bool CFG6::GetTurboTAPE( void )
{
	bool st = DEFAULT_TURBO;
	cIni::GetTruth( "CMT", "TurboTAPE", &st, st );
	return st;
}

// Turbo TAPE 有効フラグ設定
void CFG6::SetTurboTAPE( bool yn )
{
	cIni::PutEntry( "CMT", GetText( TINI_TurboTAPE ), "TurboTAPE", "%s", yn ? "Yes" : "No" );
}


// Boost Up 有効フラグ取得
bool CFG6::GetBoostUp( void )
{
	bool st = DEFAULT_BOOST;
	cIni::GetTruth( "CMT", "BoostUp", &st, st );
	return st;
}

// BoostUp 有効フラグ設定
void CFG6::SetBoostUp( bool yn )
{
	cIni::PutEntry( "CMT", GetText( TINI_BoostUp ), "BoostUp", "%s", yn ? "Yes" : "No" );
}


// BoostUp 最大倍率(N60モード)取得
int CFG6::GetMaxBoost1( void )
{
	int st = DEFAULT_MAXBOOST60;
	cIni::GetInt( "CMT", "MaxBoost60", &st, st );
	return st;
}

// BoostUp 最大倍率(N60モード)設定
void CFG6::SetMaxBoost1( int data )
{
	int st = min( max( MIN_MAXBOOST, data ), MAX_MAXBOOST );
	cIni::PutEntry( "CMT", GetText( TINI_MaxBoost60 ), "MaxBoost60", "%d", st );
}


// BoostUp 最大倍率(N60m/N66モード)取得
int CFG6::GetMaxBoost2( void )
{
	int st = DEFAULT_MAXBOOST62;
	cIni::GetInt( "CMT", "MaxBoost62", &st, st );
	return st;
}
// BoostUp 最大倍率(N60m/N66モード)設定
void CFG6::SetMaxBoost2( int data )
{
	int st = min( max( MIN_MAXBOOST, data ), MAX_MAXBOOST );
	cIni::PutEntry( "CMT", GetText( TINI_MaxBoost62 ), "MaxBoost62", "%d", st );
}


// TAPEストップビット数取得
int CFG6::GetStopBit( void )
{
	int st = DEFAULT_STOPBIT;
	cIni::GetInt( "CMT", "TapeStopBit", &st, st );
	return st;
}

// TAPEストップビット数設定
void CFG6::SetStopBit( int data )
{
	int st = min( max( MIN_STOPBIT, data ), MAX_STOPBIT );
	cIni::PutEntry( "CMT", GetText( TINI_StopBit ), "TapeStopBit", "%d", st );
}


// [DISPLAY] ---------------------------------------------------

// モード4カラーモード取得
int CFG6::GetMode4Color( void )
{
	int st = DEFAULT_MODE4_COLOR;
	cIni::GetInt( "DISPLAY", "Mode4Color", &st, st );
	return st;
}

// モード4カラーモード設定
void CFG6::SetMode4Color( int data )
{
	int st = min( max( MIN_MODE4_COLOR, data ), MAX_MODE4_COLOR );
	cIni::PutEntry( "DISPLAY", GetText( TINI_Mode4Color ), "Mode4Color", "%d", st );
}


// スキャンライン取得
bool CFG6::GetScanLine( void )
{
	bool st = DEFAULT_SCANLINE;
	cIni::GetTruth( "DISPLAY", "ScanLine", &st, st );
	return st;
}

// スキャンライン設定
void CFG6::SetScanLine( bool yn )
{
	cIni::PutEntry( "DISPLAY", GetText( TINI_ScanLine ), "ScanLine", "%s", yn ? "Yes" : "No" );
}


// スキャンライン輝度取得
int CFG6::GetScanLineBr( void )
{
	int st = DEFAULT_SCANLINEBR;
	cIni::GetInt( "DISPLAY", "ScanLineBr", &st, st );
	return st;
}

// スキャンライン輝度設定
void CFG6::SetScanLineBr( int data )
{
	int st = min( max( MIN_SCANLINEBR, data ), MAX_SCANLINEBR );
	cIni::PutEntry( "DISPLAY", GetText( TINI_ScanLineBr ), "ScanLineBr", "%d", st );
}


// フィルタリング取得
bool CFG6::GetFiltering( void )
{
	bool st = DEFAULT_FILTERING;
	cIni::GetTruth( "DISPLAY", "Filtering", &st, st );
	return st;
}

// フィルタリング設定
void CFG6::SetFiltering( bool yn )
{
	cIni::PutEntry( "DISPLAY", GetText( TINI_Filtering ), "Filtering", "%s", yn ? "Yes" : "No" );
}


// 4:3表示取得
bool CFG6::GetDispNTSC( void )
{
	bool st = DEFAULT_DISPNTSC;
	cIni::GetTruth( "DISPLAY", "DispNTSC", &st, st );
	return st;
}

// 4:3表示設定
void CFG6::SetDispNTSC( bool yn )
{
	cIni::PutEntry( "DISPLAY", GetText( TINI_DispNTSC ), "DispNTSC", "%s", yn ? "Yes" : "No" );
}


// フルスクリーン取得
bool CFG6::GetFullScreen( void )
{
	bool st = false;
	cIni::GetTruth( "DISPLAY", "FullScreen", &st, st );
	return st;
}

// フルスクリーン設定
void CFG6::SetFullScreen( bool yn )
{
	cIni::PutEntry( "DISPLAY", GetText( TINI_FullScreen ), "FullScreen", "%s", yn ? "Yes" : "No" );
}


// ウィンドウ表示倍率取得
int CFG6::GetWindowZoom( void )
{
	int st = DEFAULT_WINDOWZOOM;
	cIni::GetInt( "DISPLAY", "WindowZoom", &st, st );
	return st;
}

// ウィンドウ表示倍率設定
void CFG6::SetWindowZoom( int data )
{
	int st = min( max( MIN_WINDOWZOOM, data ), MAX_WINDOWZOOM );
	cIni::PutEntry( "DISPLAY", GetText( TINI_WindowZoom ), "WindowZoom", "%d", st );
}


// ステータスバー表示状態取得
bool CFG6::GetDispStat( void )
{
	bool st = true;
	cIni::GetTruth( "DISPLAY", "DispStatus", &st, st );
	return st;
}

// ステータスバー表示状態設定
void CFG6::SetDispStat( bool yn )
{
	cIni::PutEntry( "DISPLAY", GetText( TINI_DispStatus ), "DispStatus", "%s", yn ? "Yes" : "No" );
}


// フレームスキップ取得
int CFG6::GetFrameSkip( void )
{
	int st = DEFAULT_FRAMESKIP;
	cIni::GetInt( "DISPLAY", "FrameSkip", &st, st );
	return st;
}

// フレームスキップ設定
void CFG6::SetFrameSkip( int data )
{
	int st = min( max( MIN_FRAMESKIP, data ), MAX_FRAMESKIP );
	cIni::PutEntry( "DISPLAY", GetText( TINI_FrameSkip ), "FrameSkip", "%d", st );
}


// [SOUND] -----------------------------------------------------

// サンプリングレート取得
int CFG6::GetSampleRate( void )
{
	int st = DEFAULT_SAMPLE_RATE;
	cIni::GetInt( "SOUND", "SampleRate", &st, st );
	return st;
}

// サンプリングレート設定
void CFG6::SetSampleRate( int data )
{
	int st = min( max( MIN_SAMPLE_RATE, data ), MAX_SAMPLE_RATE );
	cIni::PutEntry( "SOUND", GetText( TINI_SampleRate ), "SampleRate", "%d", st );
}


// サウンドバッファ長倍率取得
int CFG6::GetSoundBuffer( void )
{
	int st = DEFAULT_SOUND_BUFFER;
	cIni::GetInt( "SOUND", "SoundBuffer", &st, st );
	return st;
}

// サウンドバッファ長倍率設定
void CFG6::SetSoundBuffer( int data )
{
	int st = min( max( MIN_SOUNDBUFFER, data ), MAX_SOUNDBUFFER );
	cIni::PutEntry( "SOUND", GetText( TINI_SoundBuffer ), "SoundBuffer", "%d", st );
}


// マスター音量取得
int CFG6::GetMasterVol( void )
{
	int st = DEFAULT_MASTERVOL;
	cIni::GetInt( "SOUND", "MasterVolume", &st, st );
	return st;
}

// マスター音量設定
void CFG6::SetMasterVol( int data )
{
	int st = min( max( MIN_VOLUME, data ), MAX_VOLUME );
	cIni::PutEntry( "SOUND", GetText( TINI_MasterVolume ), "MasterVolume", "%d", st );
}


// PSG音量取得
int CFG6::GetPsgVol( void )
{
	int st = DEFAULT_PSGVOL;
	cIni::GetInt( "SOUND", "PsgVolume", &st, st );
	return st;
}

// PSG音量設定
void CFG6::SetPsgVol( int data )
{
	int st = min( max( MIN_VOLUME, data ), MAX_VOLUME );
	cIni::PutEntry( "SOUND", GetText( TINI_PsgVolume ), "PsgVolume", "%d", st );
}


// PSG LPFカットオフ周波数取得
int CFG6::GetPsgLPF( void )
{
	int st = DEFAULT_PSGLPF;
	cIni::GetInt( "SOUND", "PsgLPF", &st, st );
	return st;
}

// PSG LPFカットオフ周波数設定
void CFG6::SetPsgLPF( int data )
{
	int st = min( max( MIN_LPF, data ), MAX_LPF );
	cIni::PutEntry( "SOUND", GetText( TINI_PsgLPF ),  "PsgLPF",  "%d", st );
}


// 音声合成音量取得
int CFG6::GetVoiceVol( void )
{
	int st = DEFAULT_VOICEVOL;
	cIni::GetInt( "SOUND", "VoiceVolume", &st, st );
	return st;
}

// 音声合成音量設定
void CFG6::SetVoiceVol( int data )
{
	int st = min( max( MIN_VOLUME, data ), MAX_VOLUME );
	cIni::PutEntry( "SOUND", GetText( TINI_VoiceVolume ), "VoiceVolume", "%d", st );
}


// TAPEモニタ音量取得
int CFG6::GetCmtVol( void )
{
	int st = DEFAULT_TAPEVOL;
	cIni::GetInt( "SOUND", "TapeVolume", &st, st );
	return st;
}

// TAPEモニタ音量設定
void CFG6::SetCmtVol( int data )
{
	int st = min( max( MIN_VOLUME, data ), MAX_VOLUME );
	cIni::PutEntry( "SOUND", GetText( TINI_TapeVolume ), "TapeVolume", "%d", st );
}


// TAPE LPFカットオフ周波数取得
int CFG6::GetCmtLPF( void )
{
	int st = DEFAULT_TAPELPF;
	cIni::GetInt( "SOUND", "TapeLPF", &st, st );
	return st;
}

// TAPE LPFカットオフ周波数設定
void CFG6::SetCmtLPF( int data )
{
	int st = min( max( MIN_LPF, data ), MAX_LPF );
	cIni::PutEntry( "SOUND", GetText( TINI_TapeLPF ),  "TapeLPF",  "%d", st );
}


// [MOVIE] -----------------------------------------------------

// ビデオキャプチャ色深度取得
int CFG6::GetAviBpp()
{
	int st = 24;
	cIni::GetInt( "MOVIE", "AviBpp", &st, st );
	return st;
}

// ビデオキャプチャ色深度設定
void CFG6::SetAviBpp( int data )
{
	int st = min( max( MIN_AVIBPP, data ), MAX_AVIBPP );
	cIni::PutEntry( "MOVIE", GetText( TINI_AviBpp ), "AviBpp", "%d", st );
}


// [FILES] -----------------------------------------------------

// 拡張ROMファイル名取得
const std::filesystem::path& CFG6::GetExtRomFile( void )
{
	cIni::GetPath( "FILES", "ExtRom", ExtRomFile, ExtRomFile );
	return ExtRomFile;
}

// 拡張ROMファイル名設定
void CFG6::SetExtRomFile( const std::filesystem::path& str )
{
	std::filesystem::path tpath = str;
	RelativePath( tpath );
	cIni::PutEntry( "FILES", GetText( TINI_ExtRom ), "ExtRom", tpath.u8string() );
}


// TAPEファイル名取得
const std::filesystem::path& CFG6::GetTapeFile( void )
{
	cIni::GetPath( "FILES", "tape", TapeFile, TapeFile );
	return TapeFile;
}

// TAPEファイル名設定
void CFG6::SetTapeFile( const std::filesystem::path& str )
{
	std::filesystem::path tpath = str;
	RelativePath( tpath );
	cIni::PutEntry( "FILES", GetText( TINI_tape ), "tape", tpath.u8string() );
}


// TAPE(SAVE)ファイル名取得
const std::filesystem::path& CFG6::GetSaveFile( void )
{
	cIni::GetPath( "FILES", "save", SaveFile, SaveFile );
	return SaveFile;
}

// TAPE(SAVE)ファイル名設定
void CFG6::SetSaveFile( const std::filesystem::path& str )
{
	std::filesystem::path tpath = str;
	RelativePath( tpath );
	cIni::PutEntry( "FILES", GetText( TINI_save ), "save", tpath.u8string() );
}


// DISKファイル名取得
const std::filesystem::path& CFG6::GetDiskFile( int drv )
{
	switch( drv & 1 ){
	case 1:	cIni::GetPath( "FILES", "disk1", DiskFile1, DiskFile1 );
			return DiskFile1;
	case 2: cIni::GetPath( "FILES", "disk2", DiskFile2, DiskFile2 );
			return DiskFile2;
	}
	return DummtPath;
}

// DISKファイル名設定
void CFG6::SetDiskFile( int drv, const std::filesystem::path& str )
{
	std::filesystem::path tpath = str;
	RelativePath( tpath );
	switch( drv ){
	case 1: cIni::PutEntry( "FILES", GetText( TINI_disk1 ), "disk1", tpath.u8string() ); break;
	case 2: cIni::PutEntry( "FILES", GetText( TINI_disk2 ), "disk2", tpath.u8string() ); break;
	}
}


// プリンタファイル名取得
const std::filesystem::path& CFG6::GetPrinterFile( void )
{
	cIni::GetPath( "FILES", "printer", PrinterFile, PrinterFile );
	return PrinterFile;
}

// プリンタファイル名設定
void CFG6::SetPrinterFile( const std::filesystem::path& str )
{
	std::filesystem::path tpath = str;
	RelativePath( tpath );
	cIni::PutEntry( "FILES", GetText( TINI_printer ), "printer", tpath.u8string() );
}


// **********************************************************
// フォント
// **********************************************************


// [PATH] ------------------------------------------------------

// ROMパス取得
const std::filesystem::path& CFG6::GetRomPath( void )
{
	cIni::GetPath( "PATH", "RomPath", RomPath, RomPath );
	AddDelimiter( RomPath );
	return RomPath;
}

// ROMパス設定
void CFG6::SetRomPath( const std::filesystem::path& str )
{
	std::filesystem::path tpath = str;
	DelDelimiter( tpath );
	RelativePath( tpath );
	cIni::PutEntry( "PATH", GetText( TINI_RomPath ), "RomPath", tpath.u8string() );
}


// TAPEパス取得
const std::filesystem::path& CFG6::GetTapePath( void )
{
	cIni::GetPath( "PATH", "TapePath", TapePath, TapePath );
	AddDelimiter( TapePath );
	return TapePath;
}

// TAPEパス設定
void CFG6::SetTapePath( const std::filesystem::path& str )
{
	std::filesystem::path tpath = str;
	DelDelimiter( tpath );
	RelativePath( tpath );
	cIni::PutEntry( "PATH", GetText( TINI_TapePath ), "TapePath", tpath.u8string() );
}


// DISKパス取得
const std::filesystem::path& CFG6::GetDiskPath( void )
{
	cIni::GetPath( "PATH", "DiskPath", DiskPath, DiskPath );
	AddDelimiter( DiskPath );
	return DiskPath;
}

// DISKパス設定
void CFG6::SetDiskPath( const std::filesystem::path& str )
{
	std::filesystem::path tpath = str;
	DelDelimiter( tpath );
	RelativePath( tpath );
	cIni::PutEntry( "PATH", GetText( TINI_DiskPath ), "DiskPath", tpath.u8string() );
}


// 拡張ROMパス取得
const std::filesystem::path& CFG6::GetExtRomPath( void )
{
	cIni::GetPath( "PATH", "ExtRomPath", ExtRomPath, ExtRomPath );
	AddDelimiter( ExtRomPath );
	return ExtRomPath;
}

// 拡張ROMパス設定
void CFG6::SetExtRomPath( const std::filesystem::path& str )
{
	std::filesystem::path tpath = str;
	DelDelimiter( tpath );
	RelativePath( tpath );
	cIni::PutEntry( "PATH", GetText( TINI_ExtRomPath ), "ExtRomPath", tpath.u8string() );
}


// スクリーンショット格納パス取得
const std::filesystem::path& CFG6::GetImgPath( void )
{
	cIni::GetPath( "PATH", "ImgPath", ImgPath, ImgPath );
	AddDelimiter( ImgPath );
	return ImgPath;
}

// スクリーンショット格納パス設定
void CFG6::SetImgPath( const std::filesystem::path& str )
{
	std::filesystem::path tpath = str;
	DelDelimiter( tpath );
	RelativePath( tpath );
	cIni::PutEntry( "PATH", GetText( TINI_ImgPath ), "ImgPath", tpath.u8string() );
}


// WAVEパス取得
const std::filesystem::path& CFG6::GetWavePath( void )
{
	cIni::GetPath( "PATH", "WavePath", WavePath, WavePath );
	AddDelimiter( WavePath );
	return WavePath;
}

// WAVEパス設定
void CFG6::SetWavePath( const std::filesystem::path& str )
{
	std::filesystem::path tpath = str;
	DelDelimiter( tpath );
	RelativePath( tpath );
	cIni::PutEntry( "PATH", GetText( TINI_WavePath ), "WavePath", tpath.u8string() );
}


// フォントパス取得
const std::filesystem::path& CFG6::GetFontPath( void )
{
	cIni::GetPath( "PATH", "FontPath", FontPath, FontPath );
	AddDelimiter( FontPath );
	return FontPath;
}

// フォントパス設定
void CFG6::SetFontPath( const std::filesystem::path& str )
{
	std::filesystem::path tpath = str;
	DelDelimiter( tpath );
	RelativePath( tpath );
	cIni::PutEntry( "PATH", GetText( TINI_FontPath ), "FontPath", tpath.u8string() );
}


// どこでもSAVEパス取得
const std::filesystem::path& CFG6::GetDokoSavePath( void )
{
	cIni::GetPath( "PATH", "DokoSavePath", DokoSavePath, DokoSavePath );
	AddDelimiter( DokoSavePath );
	return DokoSavePath;
}

// どこでもSAVEパス設定
void CFG6::SetDokoSavePath( const std::filesystem::path& str )
{
	std::filesystem::path tpath = str;
	DelDelimiter( tpath );
	RelativePath( tpath );
	cIni::PutEntry( "PATH", GetText( TINI_DokoSavePath ), "DokoSavePath", tpath.u8string() );
}


// [CHECK] -----------------------------------------------------

// 終了時確認取得
bool CFG6::GetCkQuit( void )
{
	bool st = false;
	cIni::GetTruth( "CHECK", "CkQuit", &st, st );
	return st;
}

// 終了時確認設定
void CFG6::SetCkQuit( bool yn )
{
	cIni::PutEntry( "CHECK", GetText( TINI_CkQuit ), "CkQuit", "%s", yn ? "Yes" : "No" );
}


// 終了時INI保存取得
bool CFG6::GetSaveQuit( void )
{
	bool st = false;
	cIni::GetTruth( "CHECK", "SaveQuit", &st, st );
	return st;
}

// 終了時INI保存設定
void CFG6::SetSaveQuit( bool yn )
{
	cIni::PutEntry( "CHECK", GetText( TINI_SaveQuit ), "SaveQuit", "%s", yn ? "Yes" : "No" );
}


// [OPTION] ----------------------------------------------------

// 戦士のカートリッジ使うフラグ取得
int CFG6::GetUseSoldier()
{
	int st = DEFAULT_SOLDIER;
	cIni::GetInt( "OPTION", "UseSoldier", &st, st );
	return st & 0x0f;
}

// 戦士のカートリッジ使うフラグ設定
void CFG6::SetUseSoldier( int sol )
{
	cIni::PutEntry( "OPTION", GetText( TINI_UseSoldier ), "UseSoldier", "%d", sol & 0x0f );
}


// [COLOR] -----------------------------------------------------

// カラーデータ取得
COLOR24 CFG6::GetColor( int num )
{
	COLOR24 col;
	
	try{
		std::string str = Stringf( "%02X%02X%02X", STDColor.at( num ).r, STDColor.at( num ).g, STDColor.at( num ).b );
		cIni::GetString( "COLOR", Stringf( "COL%03d", num ), str, str );
		int st = std::strtoul( str.c_str(), nullptr, 16 );
		col.r = (st>>16)&0xff;
		col.g = (st>> 8)&0xff;
		col.b = (st    )&0xff;
		col.a = 255;
	}
	catch( ... ){
		col.r = 0;
		col.g = 0;
		col.b = 0;
		col.a = 255;
	}
	return col;
}

// カラーデータ設定
void CFG6::SetColor( int num, const COLOR24& col )
{
	cIni::PutEntry( "COLOR", GetColorName( num-16 ), Stringf( "COL%03d", num ), "%02X%02X%02X", col.r, col.g, col.b );
}


// [KEY] -------------------------------------------------------

// キーリピート取得
int CFG6::GetKeyRepeat( void )
{
	int st = DEFAULT_REPEAT;
	cIni::GetInt( "KEY", "KeyRepeat", &st, st );
	return st;
}

// キーリピート設定
void CFG6::SetKeyRepeat( int data )
{
	int st = min( max( MIN_REPEAT, data ), MAX_REPEAT );
	cIni::PutEntry( "KEY", GetText( TINI_KeyRepeat ), "KeyRepeat", "%d", st );
}


// キー定義取得
P6KEYsym CFG6::GetVKey( PCKEYsym pcs )
{
	std::string str;
	
	// キーコードから名称取得
	cIni::GetString( "KEY", GetPCKeyName( pcs ), str, "" );
	
	return GetP6KeyCode( str );
}

// キー定義設定
void CFG6::SetVKey( PCKEYsym pcs, P6KEYsym p6s )
{
	cIni::PutEntry( "KEY", GetKeyName( pcs ), GetPCKeyName( pcs ), GetP6KeyName( p6s ) );
}


// キー定義配列取得
int CFG6::GetVKeyDef( std::vector<VKeyConv>& kdef )
{
	kdef.clear();
	for( auto &i : KeyIni ){
		VKeyConv key;
		key.PCKey = i.PCKey;
		key.P6Key = GetVKey( i.PCKey );
		kdef.emplace_back( key );
	}
	
	return kdef.size();
}


// その他 ------------------------------------------------------

// ウィンドウキャプション取得
const std::string& CFG6::GetCaption( void )
{
	switch( GetModel() ){	// 機種取得
	case 61: Caption = APPNAME " (" P61NAME ") Ver." VERSION; break;
	case 62: Caption = APPNAME " (" P62NAME ") Ver." VERSION; break;
	case 66: Caption = APPNAME " (" P66NAME ") Ver." VERSION; break;
	case 64: Caption = APPNAME " (" P64NAME ") Ver." VERSION; break;
	case 68: Caption = APPNAME " (" P68NAME ") Ver." VERSION; break;
	default: Caption = APPNAME " (" P60NAME ") Ver." VERSION; break;
	}
	return Caption;
}


// 一時保存のみ　INIファイルに書込まない
// どこでもSAVEファイル名取得
const std::filesystem::path CFG6::GetDokoFile( void )
{
	return DokoFile;
}

// どこでもSAVEファイル名設定
void CFG6::SetDokoFile( const std::filesystem::path& path )
{
	DokoFile = path;
	DelDelimiter( DokoFile );
}


////////////////////////////////////////////////////////////////
// INIオブジェクト初期値設定
//
// 引数:	over	true:上書き false:ノードが存在していたらパス
// 返値:	なし
////////////////////////////////////////////////////////////////
void CFG6::InitIni( bool over )
{
	std::string str;
	std::filesystem::path tpath;
	
	// [CONFIG] ------------------------------------------------
	// 機種
	if( over || !cIni::GetString( "CONFIG", "Model", str, "" ) )
		SetModel( DEFAULT_MODEL );
	
	// FDD
	if( over || !cIni::GetString( "CONFIG", "FDD", str, "" ) )
		SetFddNum( DEFAULT_FDD );
	
	// 拡張RAM使用
	if( over || !cIni::GetString( "CONFIG", "ExtRam", str, "" ) )
		SetUseExtRam( DEFAULT_EXTRAM );
	
	// オーバークロック率
	if( over || !cIni::GetString( "CONFIG", "OverClock", str, "" ) )
		SetOverClock( DEFAULT_OVERCLOCK );
	
	// CRCチェック
	if( over || !cIni::GetString( "CONFIG", "CheckCRC", str, "" ) )
		SetCheckCRC( DEFAULT_CHECKCRC );
	
	// FDDウェイト有効フラグ
	if( over || !cIni::GetString( "CONFIG", "FDDWait", str, "" ) )
		SetFddWaitEnable( DEFAULT_FDDWAIT );
	
	
	// [CMT] ---------------------------------------------------
	// Turbo TAPE
	if( over || !cIni::GetString( "CMT", "TurboTAPE", str, "" ) )
		SetTurboTAPE( DEFAULT_TURBO );
	
	// Boost Up
	if( over || !cIni::GetString( "CMT", "BoostUp", str, "" ) )
		SetBoostUp( DEFAULT_BOOST );
	
	// BoostUp 最大倍率(N60モード)
	if( over || !cIni::GetString( "CMT", "MaxBoost60", str, "" ) )
		SetMaxBoost1( DEFAULT_MAXBOOST60 );
	
	// BoostUp 最大倍率(N60m/N66モード)
	if( over || !cIni::GetString( "CMT", "MaxBoost62", str, "" ) )
		SetMaxBoost2( DEFAULT_MAXBOOST62 );
	
	// TAPEストップビット数
	if( over || !cIni::GetString( "CMT", "TapeStopBit", str, "" ) )
		SetStopBit( DEFAULT_STOPBIT );
	
	
	// [DISPLAY] -----------------------------------------------
	// MODE4カラー
	if( over || !cIni::GetString( "DISPLAY", "Mode4Color", str, "" ) )
		SetMode4Color( DEFAULT_MODE4_COLOR );
	
	// スキャンライン
	if( over || !cIni::GetString( "DISPLAY", "ScanLine", str, "" ) )
		SetScanLine( DEFAULT_SCANLINE );
	
	// スキャンライン輝度
	if( over || !cIni::GetString( "DISPLAY", "ScanLineBr", str, "" ) )
		SetScanLineBr( DEFAULT_SCANLINEBR );
	
	// フィルタリング
	if( over || !cIni::GetString( "DISPLAY", "Filtering", str, "" ) )
		SetFiltering( DEFAULT_FILTERING );
	
	// 4:3表示
	if( over || !cIni::GetString( "DISPLAY", "DispNTSC", str, "" ) )
		SetDispNTSC( DEFAULT_DISPNTSC );
	
	// フルスクリーン
	if( over || !cIni::GetString( "DISPLAY", "FullScreen", str, "" ) )
		SetFullScreen( DEFAULT_FULLSCREEN );
	
	// ウィンドウ表示倍率設定
	if( over || !cIni::GetString( "DISPLAY", "WindowZoom", str, "" ) )
		SetWindowZoom( DEFAULT_WINDOWZOOM );
	
	// ステータスバー表示状態
	if( over || !cIni::GetString( "DISPLAY", "DispStatus", str, "" ) )
		SetDispStat( DEFAULT_DISPSTATUS );
	
	// フレームスキップ
	if( over || !cIni::GetString( "DISPLAY", "FrameSkip", str, "" ) )
		SetFrameSkip( DEFAULT_FRAMESKIP );
	
	
	// [SOUND] -------------------------------------------------
	// サンプリングレート
	if( over || !cIni::GetString( "SOUND", "SampleRate", str, "" ) )
		SetSampleRate( DEFAULT_SAMPLE_RATE );
	
	// サウンドバッファ長倍率
	if( over || !cIni::GetString( "SOUND", "SoundBuffer", str, "" ) )
		SetSoundBuffer( DEFAULT_SOUND_BUFFER );
	
	// マスター音量
	if( over || !cIni::GetString( "SOUND", "MasterVolume", str, "" ) )
		SetMasterVol( DEFAULT_MASTERVOL );
	
	// PSG音量
	if( over || !cIni::GetString( "SOUND", "PsgVolume", str, "" ) )
		SetPsgVol( DEFAULT_PSGVOL );
	
	// PSG LPFカットオフ周波数
	if( over || !cIni::GetString( "SOUND", "PsgLPF", str, "" ) )
		SetPsgLPF( DEFAULT_PSGLPF );
	
	// 音声合成音量
	if( over || !cIni::GetString( "SOUND", "VoiceVolume", str, "" ) )
		SetVoiceVol( DEFAULT_VOICEVOL );
	
	// TAPEモニタ音量
	if( over || !cIni::GetString( "SOUND", "TapeVolume", str, "" ) )
		SetCmtVol( DEFAULT_TAPEVOL );
	
	// TAPE LPFカットオフ周波数
	if( over || !cIni::GetString( "SOUND", "TapeLPF", str, "" ) )
		SetCmtLPF( DEFAULT_TAPELPF );
	
	
	// [MOVIE] -------------------------------------------------
	// ビデオキャプチャ色深度
	if( over || !cIni::GetString( "MOVIE", "AviBpp", str, "" ) )
		SetAviBpp( DEFAULT_AVIBPP );
	
	
	// [FILES] -------------------------------------------------
	// 拡張ROMファイル名(起動時に自動マウント)
	if( over || !cIni::GetString( "FILES", "ExtRom", str, "" ) )
		SetExtRomFile( std::filesystem::u8path( "" ) );
	
	// TAPEファイル名(起動時に自動マウント)
	if( over || !cIni::GetString( "FILES", "tape", str, "" ) )
		SetTapeFile( std::filesystem::u8path( "" ) );
	
	// TAPE(SAVE)ファイル名(SAVE時に自動マウント)
	if( over || !cIni::GetString( "FILES", "save", str, "" ) ){
		AddPath( tpath, std::filesystem::u8path( DIR_TAPE ), std::filesystem::u8path( FILE_SAVE ) );
		AbsolutePath( tpath );
		SetSaveFile( tpath );
	}
	
	// DISK1ファイル名(起動時に自動マウント)
	if( over || !cIni::GetString( "FILES", "disk1", str, "" ) )
		SetDiskFile( 1, std::filesystem::u8path( "" ) );
	
	// DISK2ファイル名(起動時に自動マウント)
	if( over || !cIni::GetString( "FILES", "disk2", str, "" ) )
		SetDiskFile( 2, std::filesystem::u8path( "" ) );
	
	// プリンタファイル名
	if( over || !cIni::GetString( "FILES", "printer", str, "" ) ){
		tpath = std::filesystem::u8path( FILE_PRINTER );
		AbsolutePath( tpath );
		SetPrinterFile( tpath );
	}
	
	
	// [PATH] --------------------------------------------------
	// ROMパス
	if( over || !cIni::GetString( "PATH", "RomPath", str, "" ) ){
		tpath = std::filesystem::u8path( DIR_ROM );
		AbsolutePath( tpath );
		SetRomPath( tpath );
	}
	
	// TAPEパス
	if( over || !cIni::GetString( "PATH", "TapePath", str, "" ) ){
		tpath = std::filesystem::u8path( DIR_TAPE );
		AbsolutePath( tpath );
		SetTapePath( tpath );
	}
	
	// DISKパス
	if( over || !cIni::GetString( "PATH", "DiskPath", str, "" ) ){
		tpath = std::filesystem::u8path( DIR_DISK );
		AbsolutePath( tpath );
		SetDiskPath( tpath );
	}
	
	// 拡張ROMパス
	if( over || !cIni::GetString( "PATH", "ExtRomPath", str, "" ) ){
		tpath = std::filesystem::u8path( DIR_EXTROM );
		AbsolutePath( tpath );
		SetExtRomPath( tpath );
	}
	
	// IMGパス
	if( over || !cIni::GetString( "PATH", "ImgPath", str, "" ) ){
		tpath = std::filesystem::u8path( DIR_IMAGE );
		AbsolutePath( tpath );
		SetImgPath( tpath );
	}
	
	// WAVEパス
	if( over || !cIni::GetString( "PATH", "WavePath", str, "" ) ){
		tpath = std::filesystem::u8path( DIR_WAVE );
		AbsolutePath( tpath );
		SetWavePath( tpath );
	}
	
	// フォントパス設定
	if( over || !cIni::GetString( "PATH", "FontPath", str, "" ) ){
		tpath = std::filesystem::u8path( DIR_FONT );
		AbsolutePath( tpath );
		SetFontPath( tpath );
	}
	
	// どこでもSAVEパス
	if( over || !cIni::GetString( "PATH", "DokoSavePath", str, "" ) ){
		tpath = std::filesystem::u8path( DIR_DOKO );
		AbsolutePath( tpath );
		SetDokoSavePath( tpath );
	}
	
	
	// [CHECK] -------------------------------------------------
	// 終了時確認
	if( over || !cIni::GetString( "CHECK", "CkQuit", str, "" ) )
		SetCkQuit( DEFAULT_CKQUIT );
	
	// 終了時INI保存
	if( over || !cIni::GetString( "CHECK", "SaveQuit", str, "" ) )
		SetSaveQuit( DEFAULT_SAVEQUIT );
	
	
	// [OPTION] ------------------------------------------------
	// 戦士のカートリッジ使うフラグ
	if( over || !cIni::GetString( "OPTION", "UseSoldier", str, "" ) )
		SetUseSoldier( DEFAULT_SOLDIER );
	
	
	// [COLOR] -------------------------------------------------
	// パレット
	for( size_t i=16; i<STDColor.size(); i++ ){
		if( over || !cIni::GetString( "COLOR", Stringf( "COL%03d", i ), str, "" ) )
			SetColor( i, STDColor[i] );
	}
	
	
	// [KEY] ---------------------------------------------------
	// キーリピート
	if( over || !cIni::GetString( "KEY", "KeyRepeat", str, "" ) )
		SetKeyRepeat( DEFAULT_REPEAT );
	
	// キー定義
	for( auto &i : KeyIni )
		if( over || !cIni::GetString( "KEY", GetPCKeyName( i.PCKey ), str, "" ) )
			SetVKey( i.PCKey, i.P6Key );
}


////////////////////////////////////////////////////////////////
// 仮想キーコードから名称取得
//
// 引数:	sym		仮想キーコード
// 返値:	string&	名称文字列への参照(見つからなければUNKNOWN)
////////////////////////////////////////////////////////////////
const std::string& CFG6::GetPCKeyName( PCKEYsym sym )
{
	auto key = std::find_if( PCKeyNameDef.begin(), PCKeyNameDef.end(), [&]( PCKeyName n ){
				return( n.Key == sym );
			} );
	return key != PCKeyNameDef.end() ? key->Name : PCKeyNameDef[0].Name;
}


////////////////////////////////////////////////////////////////
// P6キーコードから名称取得
//
// 引数:	sym		P6キーコード
// 返値:	string&	名称文字列への参照(見つからなければUNKNOWN)
////////////////////////////////////////////////////////////////
const std::string& CFG6::GetP6KeyName( P6KEYsym sym )
{
	auto key = std::find_if( P6KeyNameDef.begin(), P6KeyNameDef.end(), [&]( P6KeyName n ){
				return( n.Key == sym );
			} );
	return key != P6KeyNameDef.end() ? key->Name : P6KeyNameDef[0].Name;
}


////////////////////////////////////////////////////////////////
// キー名称から仮想キーコードを取得
//
// 引数:	str			名称文字列への参照
// 返値:	PCKEYsym	仮想キーコード
////////////////////////////////////////////////////////////////
PCKEYsym CFG6::GetPCKeyCode( const std::string& str )
{
	auto key = std::find_if( PCKeyNameDef.begin(), PCKeyNameDef.end(), [&]( PCKeyName n ){
				return( n.Name == str );
			} );
	return key != PCKeyNameDef.end() ? key->Key : KVC_UNKNOWN;
}


////////////////////////////////////////////////////////////////
// キー名称からP6キーコードを取得
//
// 引数:	str			名称文字列への参照
// 返値:	P6KEYsym	P6キーコード
////////////////////////////////////////////////////////////////
P6KEYsym CFG6::GetP6KeyCode( const std::string& str )
{
	auto key = std::find_if( P6KeyNameDef.begin(), P6KeyNameDef.end(), [&]( P6KeyName n ){
				return( n.Name == str );
			} );
	return key != P6KeyNameDef.end() ? key->Key : KP6_UNKNOWN;
}


////////////////////////////////////////////////////////////////
// どこでもSAVE
//
// 引数:	ini		INIオブジェクトポインタ
// 返値:	bool	true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool CFG6::DokoSave( cIni* ini )
{
	if( !ini ) return false;
	
	// 共通
	ini->PutEntry( "GLOBAL", "", "Version",		VERSION );
	ini->PutEntry( "GLOBAL", "", "Model",		"%02d",	GetModel() );
	ini->PutEntry( "GLOBAL", "", "FDD",			"%d",	GetFddNum() );
	ini->PutEntry( "GLOBAL", "", "ExtRam",		"%s",	GetUseExtRam()  ? "Yes" : "No" );
	// OPTION
	ini->PutEntry( "OPTION", "", "UseSoldier",	"%d",	GetUseSoldier() );
	
	return true;
}


////////////////////////////////////////////////////////////////
// どこでもLOAD
//
// 引数:	ini		INIオブジェクトポインタ
// 返値:	bool	true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool CFG6::DokoLoad( cIni* ini )
{
	int st;
	bool yn;
	std::string strva;
	
	if( !ini ) return false;
	
	// 共通
	ini->GetString( "GLOBAL", "Version", strva, "" );
	if( strva != VERSION ){
		Error::SetError( Error::DokoDiffVersion );
		return false;
	}
	
	ini->GetInt(   "GLOBAL", "Model",      &st, GetModel() );		SetModel( st );
	ini->GetInt(   "GLOBAL", "FDD",        &st, GetFddNum() );		SetFddNum( st );
	ini->GetTruth( "GLOBAL", "ExtRam",     &yn, GetUseExtRam() );	SetUseExtRam( yn );
	ini->GetInt(   "OPTION", "UseSoldier", &st, GetUseSoldier() );	SetUseSoldier( st );
	
	return true;
}
