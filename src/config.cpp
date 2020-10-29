#include <fstream>
#include <algorithm>
#include <map>

#include "config.h"
#include "common.h"
#include "error.h"
#include "log.h"
#include "osd.h"
#include "pc6001v.h"


// 設定項目定義
typedef struct{
	std::string Section;	// セクション
	std::string Entry;		// ノード
	TextID Comment;			// コメント
	int Default;			// 初期値
	int Max;				// 最大値
	int Min;				// 最小値
} CfgSetValue;

typedef struct{
	std::string Section;	// セクション
	std::string Entry;		// ノード
	TextID Comment;			// コメント
	bool Default;			// 初期値
} CfgSetBool;

typedef struct{
	std::string Section;	// セクション
	std::string Entry;		// ノード
	TextID Comment;			// コメント
	P6VPATH Default;		// 初期値
	bool IsFile;			// true:ファイル false:パス
} CfgSetPath;


static const std::map<TConfig, const CfgSetValue> ConfigValue = {
	{ CF_Model,			{ "CONFIG",		"Model",		TINI_Model,			DEFAULT_MODEL,		MAX_MODEL,		MIN_MODEL		} },
	{ CF_FDD,			{ "CONFIG",		"FDD",			TINI_FDD,			DEFAULT_FDD,		MAX_FDD,		MIN_FDD			} },
	{ CF_OverClock,		{ "CONFIG",		"OverClock",	TINI_OverClock,		DEFAULT_OVERCLOCK,	MAX_OVERCLOCK,	MIN_OVERCLOCK	} },
	{ CF_MaxBoost60,	{ "CMT",		"MaxBoost60",	TINI_MaxBoost60,	DEFAULT_MAXBOOST60,	MAX_BOOST,		MIN_BOOST		} },
	{ CF_MaxBoost62,	{ "CMT",		"MaxBoost62",	TINI_MaxBoost62,	DEFAULT_MAXBOOST62,	MAX_BOOST,		MIN_BOOST		} },
	{ CF_StopBit,		{ "CMT",		"StopBit",		TINI_StopBit,		DEFAULT_STOPBIT,	MAX_STOPBIT,	MIN_STOPBIT		} },
	{ CF_Mode4Color,	{ "DISPLAY",	"Mode4Color",	TINI_Mode4Color,	DEFAULT_MODE4COLOR,	MAX_MODE4COLOR,	MIN_MODE4COLOR	} },
	{ CF_ScanLineBr,	{ "DISPLAY",	"ScanLineBr",	TINI_ScanLineBr,	DEFAULT_SCANLINEBR,	MAX_SCANLINEBR,	MIN_SCANLINEBR	} },
	{ CF_WindowZoom,	{ "DISPLAY",	"WindowZoom",	TINI_WindowZoom,	DEFAULT_WINDOWZOOM,	MAX_WINDOWZOOM,	MIN_WINDOWZOOM	} },
	{ CF_FrameSkip,		{ "DISPLAY",	"FrameSkip",	TINI_FrameSkip,		DEFAULT_FRAMESKIP,	MAX_FRAMESKIP,	MIN_FRAMESKIP	} },
	{ CF_SampleRate,	{ "SOUND",		"SampleRate",	TINI_SampleRate,	DEFAULT_SAMPLERATE,	MAX_SAMPLERATE,	MIN_SAMPLERATE	} },
	{ CF_SoundBuffer,	{ "SOUND",		"SoundBuffer",	TINI_SoundBuffer,	DEFAULT_SOUNDBUF,	MAX_SOUNDBUF,	MIN_SOUNDBUF	} },
	{ CF_MasterVolume,	{ "SOUND",		"MasterVolume",	TINI_MasterVolume,	DEFAULT_MASTERVOL,	MAX_VOLUME,		MIN_VOLUME		} },
	{ CF_PsgVolume,		{ "SOUND",		"PsgVolume",	TINI_PsgVolume,		DEFAULT_PSGVOL,		MAX_VOLUME,		MIN_VOLUME		} },
	{ CF_PsgLPF,		{ "SOUND",		"PsgLPF",		TINI_PsgLPF,		DEFAULT_PSGLPF,		MAX_LPF,		MIN_LPF			} },
	{ CF_VoiceVolume,	{ "SOUND",		"VoiceVolume",	TINI_VoiceVolume,	DEFAULT_VOICEVOL,	MAX_VOLUME,		MIN_VOLUME		} },
	{ CF_TapeVolume,	{ "SOUND",		"TapeVolume",	TINI_TapeVolume,	DEFAULT_TAPEVOL,	MAX_VOLUME,		MIN_VOLUME		} },
	{ CF_TapeLPF,		{ "SOUND",		"TapeLPF",		TINI_TapeLPF,		DEFAULT_TAPELPF,	MAX_LPF,		MIN_LPF			} },
	{ CF_AviBpp,		{ "MOVIE",		"AviBpp",		TINI_AviBpp,		DEFAULT_AVIBPP,		MAX_AVIBPP,		MIN_AVIBPP		} },
	{ CF_UseSoldier,	{ "OPTION",		"UseSoldier",	TINI_UseSoldier,	DEFAULT_SOLDIER,	0x0f,			0				} },
	{ CF_KeyRepeat,		{ "KEY",		"KeyRepeat",	TINI_KeyRepeat,		DEFAULT_REPEAT,		MAX_REPEAT,		MIN_REPEAT		} }
};

static const std::map<TConfig, const CfgSetBool> ConfigBool = {
	{ CF_ExtRam,		{ "CONFIG",		"ExtRam",		TINI_ExtRam,		DEFAULT_EXTRAM		} },
	{ CF_CheckCRC,		{ "CONFIG",		"CheckCRC",		TINI_CheckCRC,		DEFAULT_CHECKCRC	} },
	{ CF_FDDWait,		{ "CONFIG",		"FDDWait",		TINI_FDDWait,		DEFAULT_FDDWAIT		} },
	{ CF_TurboTAPE,		{ "CMT",		"TurboTAPE",	TINI_TurboTAPE,		DEFAULT_TURBO		} },
	{ CF_BoostUp,		{ "CMT",		"BoostUp",		TINI_BoostUp,		DEFAULT_BOOST		} },
	{ CF_ScanLine,		{ "DISPLAY",	"ScanLine",		TINI_ScanLine,		DEFAULT_SCANLINE	} },
	{ CF_Filtering,		{ "DISPLAY",	"Filtering",	TINI_Filtering,		DEFAULT_FILTERING	} },
	{ CF_DispNTSC,		{ "DISPLAY",	"DispNTSC",		TINI_DispNTSC,		DEFAULT_DISPNTSC	} },
	{ CF_FullScreen,	{ "DISPLAY",	"FullScreen",	TINI_FullScreen,	DEFAULT_FULLSCREEN	} },
	{ CF_DispStatus,	{ "DISPLAY",	"DispStatus",	TINI_DispStatus,	DEFAULT_DISPSTATUS	} },
	{ CF_CkQuit,		{ "CHECK",		"CkQuit",		TINI_CkQuit,		DEFAULT_CKQUIT		} },
	{ CF_SaveQuit,		{ "CHECK",		"SaveQuit",		TINI_SaveQuit,		DEFAULT_SAVEQUIT	} }
};

static const std::map<TConfig, const CfgSetPath> ConfigPath = {
	{ CF_ExtRom,		{ "FILES",		"ExtRom",		TINI_ExtRom,		"",							true	} },
	{ CF_tape,			{ "FILES",		"tape",			TINI_tape,			"",							true	} },
	{ CF_save,			{ "FILES",		"save",			TINI_save,			DIR_TAPE "\\" FILE_SAVE,	true	} },
	{ CF_disk1,			{ "FILES",		"disk1",		TINI_disk1,			"",							true	} },
	{ CF_disk2,			{ "FILES",		"disk2",		TINI_disk2,			"",							true	} },
	{ CF_printer,		{ "FILES",		"printer",		TINI_printer,		FILE_PRINT,					true	} },
	{ CF_RomPath,		{ "PATH",		"RomPath",		TINI_RomPath,		DIR_ROM,					false	} },
	{ CF_TapePath,		{ "PATH",		"TapePath",		TINI_TapePath,		DIR_TAPE,					false	} },
	{ CF_DiskPath,		{ "PATH",		"DiskPath",		TINI_DiskPath,		DIR_DISK,					false	} },
	{ CF_ExtRomPath,	{ "PATH",		"ExtRomPath",	TINI_ExtRomPath,	DIR_EXTROM,					false	} },
	{ CF_ImgPath,		{ "PATH",		"ImgPath",		TINI_ImgPath,		DIR_IMAGE,					false	} },
	{ CF_WavePath,		{ "PATH",		"WavePath",		TINI_WavePath,		DIR_WAVE,					false	} },
	{ CF_FontPath,		{ "PATH",		"FontPath",		TINI_FontPath,		DIR_FONT,					false	} },
	{ CF_DokoPath,		{ "PATH",		"DokoPath",		TINI_DokoPath,		DIR_DOKO,					false	} }
};


static const std::vector<P6KeyName> P6KeyNameDef = {
	{ KP6_UNKNOWN,		"K6_UNKNOWN"	},
	
	{ KP6_1,			"K6_1"			},
	{ KP6_2,			"K6_2"			},
	{ KP6_3,			"K6_3"			},
	{ KP6_4,			"K6_4"			},
	{ KP6_5,			"K6_5"			},
	{ KP6_6,			"K6_6"			},
	{ KP6_7,			"K6_7"			},
	{ KP6_8,			"K6_8"			},
	{ KP6_9,			"K6_9"			},
	{ KP6_0,			"K6_0"			},
	
	{ KP6_A,			"K6_A"			},
	{ KP6_B,			"K6_B"			},
	{ KP6_C,			"K6_C"			},
	{ KP6_D,			"K6_D"			},
	{ KP6_E,			"K6_E"			},
	{ KP6_F,			"K6_F"			},
	{ KP6_G,			"K6_G"			},
	{ KP6_H,			"K6_H"			},
	{ KP6_I,			"K6_I"			},
	{ KP6_J,			"K6_J"			},
	{ KP6_K,			"K6_K"			},
	{ KP6_L,			"K6_L"			},
	{ KP6_M,			"K6_M"			},
	{ KP6_N,			"K6_N"			},
	{ KP6_O,			"K6_O"			},
	{ KP6_P,			"K6_P"			},
	{ KP6_Q,			"K6_Q"			},
	{ KP6_R,			"K6_R"			},
	{ KP6_S,			"K6_S"			},
	{ KP6_T,			"K6_T"			},
	{ KP6_U,			"K6_U"			},
	{ KP6_V,			"K6_V"			},
	{ KP6_W,			"K6_W"			},
	{ KP6_X,			"K6_X"			},
	{ KP6_Y,			"K6_Y"			},
	{ KP6_Z,			"K6_Z"			},
	
	{ KP6_F1,			"K6_F1"			},
	{ KP6_F2,			"K6_F2"			},
	{ KP6_F3,			"K6_F3"			},
	{ KP6_F4,			"K6_F4"			},
	{ KP6_F5,			"K6_F5"			},
	
	{ KP6_MINUS,		"K6_MINUS"		},
	{ KP6_CARET,		"K6_CARET"		},
	{ KP6_YEN,			"K6_YEN"		},
	{ KP6_AT,			"K6_AT"			},
	{ KP6_LBRACKET,		"K6_LBRACKET"	},
	{ KP6_RBRACKET,		"K6_RBRACKET"	},
	{ KP6_SEMICOLON,	"K6_SEMICOLON"	},
	{ KP6_COLON,		"K6_COLON"		},
	{ KP6_COMMA,		"K6_COMMA"		},
	{ KP6_PERIOD,		"K6_PERIOD"		},
	{ KP6_SLASH,		"K6_SLASH"		},
	{ KP6_UNDERSCORE,	"K6_UNDERSCORE"	},
	{ KP6_SPACE,		"K6_SPACE"		},
	
	{ KP6_ESC,			"K6_ESC"		},
	{ KP6_TAB,			"K6_TAB"		},
	{ KP6_CTRL,			"K6_CTRL"		},
	{ KP6_SHIFT,		"K6_SHIFT"		},
	{ KP6_GRAPH,		"K6_GRAPH"		},
	{ KP6_HOME,			"K6_HOME"		},
	{ KP6_STOP,			"K6_STOP"		},
	{ KP6_PAGE,			"K6_PAGE"		},
	{ KP6_RETURN,		"K6_RETURN"		},
	{ KP6_KANA,			"K6_KANA"		},
	{ KP6_INS,			"K6_INS"		},
	{ KP6_DEL,			"K6_DEL"		},
	
	{ KP6_UP,			"K6_UP"			},
	{ KP6_DOWN,			"K6_DOWN"		},
	{ KP6_LEFT,			"K6_LEFT"		},
	{ KP6_RIGHT,		"K6_RIGHT"		},
	
	{ KP6_MODE,			"K6_MODE"		},
	{ KP6_CAPS,			"K6_CAPS"		},
	
	
	// テンキー部拡張
	{ KP6_P0,			"K6_P0"			},
	{ KP6_P1,			"K6_P1"			},
	{ KP6_P2,			"K6_P2"			},
	{ KP6_P3,			"K6_P3"			},
	{ KP6_P4,			"K6_P4"			},
	{ KP6_P5,			"K6_P5"			},
	{ KP6_P6,			"K6_P6"			},
	{ KP6_P7,			"K6_P7"			},
	{ KP6_P8,			"K6_P8"			},
	{ KP6_P9,			"K6_P9"			},
	{ KP6_PPLUS,		"K6_PPLUS"		},
	{ KP6_PMINUS,		"K6_PMINUS"		},
	{ KP6_PMULTIPLY,	"K6_PMULTIPLY"	},
	{ KP6_PDIVIDE,		"K6_PDIVIDE"	},
	{ KP6_PPERIOD,		"K6_PPERIOD"	},
	{ KP6_PRETURN,		"K6_PRETURN"	},
	
	
	// 各種機能キー
	{ KFN_1,			"K6_FN1"		},
	{ KFN_2,			"K6_FN2"		},
	{ KFN_3,			"K6_FN3"		},
	{ KFN_4,			"K6_FN4"		},
	{ KFN_5,			"K6_FN5"		},
	{ KFN_6,			"K6_FN6"		},
	{ KFN_7,			"K6_FN7"		},
	{ KFN_8,			"K6_FN8"		},
	{ KFN_9,			"K6_FN9"		}
};


static const std::vector<PCKeyName> PCKeyNameDef = {
	{ KVC_UNKNOWN,		"K_UNKNOWN"		},
	
	{ KVC_1,			"K_1"			},
	{ KVC_2,			"K_2"			},
	{ KVC_3,			"K_3"			},
	{ KVC_4,			"K_4"			},
	{ KVC_5,			"K_5"			},
	{ KVC_6,			"K_6"			},
	{ KVC_7,			"K_7"			},
	{ KVC_8,			"K_8"			},
	{ KVC_9,			"K_9"			},
	{ KVC_0,			"K_0"			},
	
	{ KVC_A,			"K_A"			},
	{ KVC_B,			"K_B"			},
	{ KVC_C,			"K_C"			},
	{ KVC_D,			"K_D"			},
	{ KVC_E,			"K_E"			},
	{ KVC_F,			"K_F"			},
	{ KVC_G,			"K_G"			},
	{ KVC_H,			"K_H"			},
	{ KVC_I,			"K_I"			},
	{ KVC_J,			"K_J"			},
	{ KVC_K,			"K_K"			},
	{ KVC_L,			"K_L"			},
	{ KVC_M,			"K_M"			},
	{ KVC_N,			"K_N"			},
	{ KVC_O,			"K_O"			},
	{ KVC_P,			"K_P"			},
	{ KVC_Q,			"K_Q"			},
	{ KVC_R,			"K_R"			},
	{ KVC_S,			"K_S"			},
	{ KVC_T,			"K_T"			},
	{ KVC_U,			"K_U"			},
	{ KVC_V,			"K_V"			},
	{ KVC_W,			"K_W"			},
	{ KVC_X,			"K_X"			},
	{ KVC_Y,			"K_Y"			},
	{ KVC_Z,			"K_Z"			},
	
	{ KVC_F1,			"K_F1"			},
	{ KVC_F2,			"K_F2"			},
	{ KVC_F3,			"K_F3"			},
	{ KVC_F4,			"K_F4"			},
	{ KVC_F5,			"K_F5"			},
	{ KVC_F6,			"K_F6"			},
	{ KVC_F7,			"K_F7"			},
	{ KVC_F8,			"K_F8"			},
	{ KVC_F9,			"K_F9"			},
	{ KVC_F10,			"K_F10"			},
	{ KVC_F11,			"K_F11"			},
	{ KVC_F12,			"K_F12"			},
	
	{ KVC_MINUS,		"K_MINUS"		},
	{ KVC_CARET,		"K_CARET"		},
	{ KVC_BACKSPACE,	"K_BACKSPACE"	},
	{ KVC_AT,			"K_AT"			},
	{ KVC_LBRACKET,		"K_LBRACKET"	},
	{ KVC_SEMICOLON,	"K_SEMICOLON"	},
	{ KVC_COLON,		"K_COLON"		},
	{ KVC_COMMA,		"K_COMMA"		},
	{ KVC_PERIOD,		"K_PERIOD"		},
	{ KVC_SLASH,		"K_SLASH"		},
	{ KVC_SPACE,		"K_SPACE"		},
	
	{ KVC_ESC,			"K_ESC"			},
	{ KVC_HANZEN,		"K_HANZEN"		},
	{ KVC_TAB,			"K_TAB"			},
	{ KVC_CAPSLOCK,		"K_CAPSLOCK"	},
	{ KVC_ENTER,		"K_ENTER"		},
	{ KVC_LCTRL,		"K_LCTRL"		},
	{ KVC_RCTRL,		"K_RCTRL"		},
	{ KVC_LSHIFT,		"K_LSHIFT"		},
	{ KVC_RSHIFT,		"K_RSHIFT"		},
	{ KVC_LALT,			"K_LALT"		},
	{ KVC_RALT,			"K_RALT"		},
	{ KVC_PRINT,		"K_PRINT"		},
	{ KVC_SCROLLLOCK,	"K_SCROLLLOCK"	},
	{ KVC_PAUSE,		"K_PAUSE"		},
	{ KVC_INSERT,		"K_INSERT"		},
	{ KVC_DELETE,		"K_DELETE"		},
	{ KVC_HOME,			"K_HOME"		},
	{ KVC_END,			"K_END"			},
	{ KVC_PAGEUP,		"K_PAGEUP"		},
	{ KVC_PAGEDOWN,		"K_PAGEDOWN"	},
	
	{ KVC_UP,			"K_UP"			},
	{ KVC_DOWN,			"K_DOWN"		},
	{ KVC_LEFT,			"K_LEFT"		},
	{ KVC_RIGHT,		"K_RIGHT"		},
	
	{ KVC_P0,			"K_P_0"			},
	{ KVC_P1,			"K_P_1"			},
	{ KVC_P2,			"K_P_2"			},
	{ KVC_P3,			"K_P_3"			},
	{ KVC_P4,			"K_P_4"			},
	{ KVC_P5,			"K_P_5"			},
	{ KVC_P6,			"K_P_6"			},
	{ KVC_P7,			"K_P_7"			},
	{ KVC_P8,			"K_P_8"			},
	{ KVC_P9,			"K_P_9"			},
	{ KVC_NUMLOCK,		"K_NUMLOCK"		},
	{ KVC_P_PLUS,		"K_P_PLUS"		},
	{ KVC_P_MINUS,		"K_P_MINUS"		},
	{ KVC_P_MULTIPLY,	"K_P_MULTIPLY"	},
	{ KVC_P_DIVIDE,		"K_P_DIVIDE"	},
	{ KVC_P_PERIOD,		"K_P_PERIOD"	},
	{ KVC_P_ENTER,		"K_P_ENTER"		},
	
	// 日本語キーボードのみ
	{ KVC_YEN,			"K_YEN"			},
	{ KVC_RBRACKET,		"K_RBRACKET"	},
	{ KVC_UNDERSCORE,	"K_UNDERSCORE"	},
	{ KVC_MUHENKAN,		"K_MUHENKAN"	},
	{ KVC_HENKAN,		"K_HENKAN"		},
	{ KVC_HIRAGANA,		"K_HIRAGANA"	},
	
	// 英語キーボードのみ
	{ KVE_BACKSLASH,	"K_BACKSLASH"	},
	
	// 追加キー
	{ KVX_RMETA,		"K_RMETA"		},
	{ KVX_LMETA,		"K_LMETA"		},
	{ KVX_MENU,			"K_MENU"		}
};


static const std::vector<VKeyConv> KeyIni = {	// 仮想キーコード -> P6キーコード定義初期値
	{ KVC_1,			KP6_1			},	// 1	!
	{ KVC_2,			KP6_2			},	// 2	"
	{ KVC_3,			KP6_3			},	// 3	#
	{ KVC_4,			KP6_4			},	// 4	$
	{ KVC_5,			KP6_5			},	// 5	%
	{ KVC_6,			KP6_6			},	// 6	&
	{ KVC_7,			KP6_7			},	// 7	'
	{ KVC_8,			KP6_8			},	// 8	(
	{ KVC_9,			KP6_9			},	// 9	)
	{ KVC_0,			KP6_0			},	// 0
	
	{ KVC_A,			KP6_A			},	// a	A
	{ KVC_B,			KP6_B			},	// b	B
	{ KVC_C,			KP6_C			},	// c	C
	{ KVC_D,			KP6_D			},	// d	D
	{ KVC_E,			KP6_E			},	// e	E
	{ KVC_F,			KP6_F			},	// f	F
	{ KVC_G,			KP6_G			},	// g	G
	{ KVC_H,			KP6_H			},	// h	H
	{ KVC_I,			KP6_I			},	// i	I
	{ KVC_J,			KP6_J			},	// j	J
	{ KVC_K,			KP6_K			},	// k	K
	{ KVC_L,			KP6_L			},	// l	L
	{ KVC_M,			KP6_M			},	// m	M
	{ KVC_N,			KP6_N			},	// n	N
	{ KVC_O,			KP6_O			},	// o	O
	{ KVC_P,			KP6_P			},	// p	P
	{ KVC_Q,			KP6_Q			},	// q	Q
	{ KVC_R,			KP6_R			},	// r	R
	{ KVC_S,			KP6_S			},	// s	S
	{ KVC_T,			KP6_T			},	// t	T
	{ KVC_U,			KP6_U			},	// u	U
	{ KVC_V,			KP6_V			},	// v	V
	{ KVC_W,			KP6_W			},	// w	W
	{ KVC_X,			KP6_X			},	// x	X
	{ KVC_Y,			KP6_Y			},	// y	Y
	{ KVC_Z,			KP6_Z			},	// z	Z
	
	{ KVC_F1,			KP6_F1			},	// F1
	{ KVC_F2,			KP6_F2			},	// F2
	{ KVC_F3,			KP6_F3			},	// F3
	{ KVC_F4,			KP6_F4			},	// F4
	{ KVC_F5,			KP6_F5			},	// F5
	
	{ KVC_MINUS,		KP6_MINUS		},	// -	=
	{ KVC_CARET,		KP6_CARET		},	// ^	~
	{ KVC_BACKSPACE,	KP6_DEL			},	// BackSpace
	{ KVC_AT,			KP6_AT			},	// @	`
	{ KVC_LBRACKET,		KP6_LBRACKET	},	// [	{
	{ KVC_SEMICOLON,	KP6_SEMICOLON	},	// ;	+
	{ KVC_COLON,		KP6_COLON		},	// :	*
	{ KVC_COMMA,		KP6_COMMA		},	// ,	<
	{ KVC_PERIOD,		KP6_PERIOD		},	// .	>
	{ KVC_SLASH,		KP6_SLASH		},	// /	?
	{ KVC_SPACE,		KP6_SPACE		},	// Space
	
	{ KVC_ESC,			KP6_ESC			},	// ESC
	{ KVC_HANZEN,		KP6_UNKNOWN		},	// 半角/全角
	{ KVC_TAB,			KP6_TAB			},	// Tab
	{ KVC_CAPSLOCK,		KP6_UNKNOWN		},	// CapsLock
	{ KVC_ENTER,		KP6_RETURN		},	// Enter
	{ KVC_LCTRL,		KP6_CTRL		},	// L-Ctrl
	{ KVC_RCTRL,		KP6_CTRL		},	// R-Ctrl
	{ KVC_LSHIFT,		KP6_SHIFT		},	// L-Shift
	{ KVC_RSHIFT,		KP6_SHIFT		},	// R-Shift
	{ KVC_LALT,			KP6_GRAPH		},	// L-Alt
	{ KVC_RALT,			KP6_GRAPH		},	// R-Alt
	{ KVC_PRINT,		KP6_UNKNOWN		},	// PrintScreen
	{ KVC_SCROLLLOCK,	KP6_CAPS		},	// ScrollLock
	{ KVC_PAUSE,		KP6_KANA		},	// Pause
	{ KVC_INSERT,		KP6_INS			},	// Insert
	{ KVC_DELETE,		KP6_DEL			},	// Delete
	{ KVC_END,			KP6_STOP		},	// End
	{ KVC_HOME,			KP6_HOME		},	// Home
	{ KVC_PAGEUP,		KP6_PAGE		},	// PageUp
	{ KVC_PAGEDOWN,		KP6_MODE		},	// PageDown
	
	{ KVC_UP,			KP6_UP			},	// ↑
	{ KVC_DOWN,			KP6_DOWN		},	// ↓
	{ KVC_LEFT,			KP6_LEFT		},	// ←
	{ KVC_RIGHT,		KP6_RIGHT		},	// →
	
	{ KVC_P0,			KP6_P0			},	// [0]
	{ KVC_P1,			KP6_P1			},	// [1]
	{ KVC_P2,			KP6_P2			},	// [2]
	{ KVC_P3,			KP6_P3			},	// [3]
	{ KVC_P4,			KP6_P4			},	// [4]
	{ KVC_P5,			KP6_P5			},	// [5]
	{ KVC_P6,			KP6_P6			},	// [6]
	{ KVC_P7,			KP6_P7			},	// [7]
	{ KVC_P8,			KP6_P8			},	// [8]
	{ KVC_P9,			KP6_P9			},	// [9]
	{ KVC_NUMLOCK,		KP6_UNKNOWN		},	// NumLock
	{ KVC_P_PLUS,		KP6_PPLUS		},	// [+]
	{ KVC_P_MINUS,		KP6_PMINUS		},	// [-]
	{ KVC_P_MULTIPLY,	KP6_PMULTIPLY	},	// [*]
	{ KVC_P_DIVIDE,		KP6_PDIVIDE		},	// [/]
	{ KVC_P_PERIOD,		KP6_PPERIOD		},	// [.]
	{ KVC_P_ENTER,		KP6_PRETURN		},	// [Enter]
	
	// 日本語キーボードのみ
	{ KVC_YEN,			KP6_YEN },			// ￥	|
	{ KVC_RBRACKET,		KP6_RBRACKET	},	// ]	}
	{ KVC_UNDERSCORE,	KP6_UNDERSCORE	},	// \	_
//	{ KVC_MUHENKAN,		KP6_UNKNOWN		},	// 無変換
//	{ KVC_HENKAN,		KP6_UNKNOWN		},	// 変換
//	{ KVC_HIRAGANA,		KP6_UNKNOWN		},	// ひらがな
	
	// 英語キーボードのみ
	{ KVE_BACKSLASH,	KP6_YEN			},	// BackSlash	|
	
	// 追加キー
	{ KVX_RMETA,		KP6_UNKNOWN		},	// L-Meta
	{ KVX_LMETA,		KP6_UNKNOWN		},	// R-Meta
	{ KVX_MENU,			KP6_UNKNOWN		}	// Menu
	
	// 各種機能キー (今のところ無効)
//	{ KVC_F6,			KFN_1			},	// F6
//	{ KVC_F7,			KFN_2			},	// F7
//	{ KVC_F8,			KFN_3			},	// F8
//	{ KVC_F9,			KFN_4			},	// F9
//	{ KVC_F10,			KFN_5			},	// F10
//	{ KVC_F11,			KFN_6			},	// F11
//	{ KVC_F12,			KFN_7			}	// F12
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
	{ 255, 255, 255, 0 }	// 80:白
};




////////////////////////////////////////////////////////////////
// コンストラクタ
////////////////////////////////////////////////////////////////
CFG6::CFG6( void ) : DokoFile(""), Caption("")
{
	PRINTD( CONST_LOG, "[[CFG6]]\n" );
	
	// INIファイルのパスを設定
	IniPath = P6VSTR2PATH( FILE_CONFIG );
	OSD_AbsolutePath( IniPath );
}


////////////////////////////////////////////////////////////////
// デストラクタ
////////////////////////////////////////////////////////////////
CFG6::~CFG6( void )
{
	PRINTD( CONST_LOG, "[[~CFG6]]\n" );
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
		if( !OSD_FileExist( IniPath ) ){
			std::fstream fs;
			
			if( !OSD_FSopen( fs, IniPath, std::ios_base::out ) ) throw Error::IniWriteFailed;
			
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

// 値 取得
int CFG6::GetValue( TConfig tc )
{
	try{
		CfgSetValue cs = ConfigValue.at( tc );
		int st = cs.Default;
		
		cIni::GetValue( cs.Section, cs.Entry, st );
		return st;
	}
	catch( std::out_of_range& ){}
	
	return 0;
}


// 値 設定
void CFG6::SetValue( TConfig tc , int val )
{
	try{
		CfgSetValue cs = ConfigValue.at( tc );
		int st = val;
		
		if( cs.Max != cs.Min ){
			st = min( max( cs.Min, st ), cs.Max );
		}
		cIni::PutValue( cs.Section, cs.Entry, GetText( cs.Comment ), st );
	}
	catch( std::out_of_range& ){}
}


// bool 取得
bool CFG6::GetYesNo( TConfig tc )
{
	try{
		CfgSetBool cs = ConfigBool.at( tc );
		bool st = cs.Default;
		
		cIni::GetYesNo( cs.Section, cs.Entry, st );
		return st;
	}
	catch( std::out_of_range& ){}
	
	return false;
}


// bool 設定
void CFG6::SetYesNo( TConfig tc , bool yn )
{
	try{
		CfgSetBool cs = ConfigBool.at( tc );
		
		cIni::PutYesNo( cs.Section, cs.Entry, GetText( cs.Comment ), yn );
	}
	catch( std::out_of_range& ){}
}


// path 取得
P6VPATH CFG6::GetPath( TConfig tc )
{
	try{
		CfgSetPath cs = ConfigPath.at( tc );
		P6VPATH st = cs.Default;
		
		cIni::GetPath( cs.Section, cs.Entry, st );
		if( !cs.IsFile ){
			OSD_AddDelimiter( st );
		}
		
		return st;
	}
	catch( std::out_of_range& ){}
	
	return "";
}


// path 設定
void CFG6::SetPath( TConfig tc , const P6VPATH& path )
{
	try{
		CfgSetPath cs = ConfigPath.at( tc );
		
		P6VPATH tpath = path;
		if( !cs.IsFile ){
			OSD_DelDelimiter( tpath );
		}
		OSD_RelativePath( tpath );
		
		cIni::PutPath( cs.Section, cs.Entry, GetText( cs.Comment ), tpath );
	}
	catch( std::out_of_range& ){}
}


// 初期値設定
void CFG6::SetDefault( TConfig tc, bool ow )
{
	std::string str;
	
	try{
		if( ConfigValue.count( tc ) ){
			CfgSetValue cs = ConfigValue.at( tc );
			if( ow || !cIni::GetEntry( cs.Section, cs.Entry, str ) ){
				cIni::PutValue( cs.Section, cs.Entry, GetText( cs.Comment ), cs.Default );
			}
		}else if( ConfigBool.count( tc )  ){
			CfgSetBool cs = ConfigBool.at( tc );
			if( ow || !cIni::GetEntry( cs.Section, cs.Entry, str ) ){
				cIni::PutYesNo( cs.Section, cs.Entry, GetText( cs.Comment ), cs.Default );
			}
		}else if( ConfigPath.count( tc )  ){
			CfgSetPath cs = ConfigPath.at( tc );
			if( ow || !cIni::GetEntry( cs.Section, cs.Entry, str ) ){
				cIni::PutPath( cs.Section, cs.Entry, GetText( cs.Comment ), cs.Default );
			}
		}
	}
	catch( std::out_of_range& ){}
}


// [COLOR] -----------------------------------------------------

// カラーデータ取得
COLOR24 CFG6::GetColor( int num )
{
	COLOR24 col;
	
	try{
		std::string str = Stringf( "%02X%02X%02X", STDColor.at( num ).r, STDColor.at( num ).g, STDColor.at( num ).b );
		cIni::GetEntry( "COLOR", Stringf( "COL%03d", num ), str );
		int st = std::stoul( str, nullptr, 16 );
		col.r = (st>>16)&0xff;
		col.g = (st>> 8)&0xff;
		col.b = (st    )&0xff;
		col.a = 255;
	}
	catch( std::out_of_range& ){
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
	cIni::PutValue( "COLOR", Stringf( "COL%03d", num ), GetColorName( num-16 ), (col.r<<16)|(col.g<<8)|col.b, "%06X" );
}


// [KEY] -------------------------------------------------------

// キー定義取得
P6KEYsym CFG6::GetVKey( PCKEYsym pcs )
{
	std::string str;
	
	// キーコードから名称取得
	cIni::GetEntry( "KEY", GetPCKeyName( pcs ), str );
	
	return GetP6KeyCode( str );
}

// キー定義設定
void CFG6::SetVKey( PCKEYsym pcs, P6KEYsym p6s )
{
	cIni::PutEntry( "KEY", GetPCKeyName( pcs ), GetKeyName( pcs ), GetP6KeyName( p6s ) );
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
	switch( GetValue( CF_Model ) ){	// 機種取得
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
const P6VPATH CFG6::GetDokoFile( void )
{
	return DokoFile;
}

// どこでもSAVEファイル名設定
void CFG6::SetDokoFile( const P6VPATH& path )
{
	DokoFile = path;
	OSD_DelDelimiter( DokoFile );
}


////////////////////////////////////////////////////////////////
// INIオブジェクト初期値設定
//
// 引数:	over	true:上書き false:ノードが存在していたらパス
// 返値:	なし
////////////////////////////////////////////////////////////////
void CFG6::InitIni( bool over )
{
	std::vector<TConfig> allcf = {
		// [CONFIG] ------------------------------------------------
		CF_Model,			// 機種
		CF_FDD,				// FDD
		CF_ExtRam,			// 拡張RAM使用
		CF_OverClock,		// オーバークロック率
		CF_CheckCRC,		// CRCチェック
		CF_FDDWait,			// FDDウェイト有効フラグ
		
		// [CMT] ---------------------------------------------------
		CF_TurboTAPE,		// Turbo TAPE
		CF_BoostUp,			// Boost Up
		CF_MaxBoost60,		// BoostUp 最大倍率(N60モード)
		CF_MaxBoost62,		// BoostUp 最大倍率(N60m/N66モード)
		CF_StopBit,			// TAPEストップビット数
		
		// [DISPLAY] -----------------------------------------------
		CF_Mode4Color,		// MODE4カラー
		CF_ScanLine,		// スキャンライン
		CF_ScanLineBr,		// スキャンライン輝度
		CF_Filtering,		// フィルタリング
		CF_DispNTSC,		// 4:3表示
		CF_FullScreen,		// フルスクリーン
		CF_WindowZoom,		// ウィンドウ表示倍率設定
		CF_DispStatus,		// ステータスバー表示状態
		CF_FrameSkip,		// フレームスキップ
		
		// [SOUND] -------------------------------------------------
		CF_SampleRate,		// サンプリングレート
		CF_SoundBuffer,		// サウンドバッファ長倍率
		CF_MasterVolume,	// マスター音量
		CF_PsgVolume,		// PSG音量
		CF_PsgLPF,			// PSG LPFカットオフ周波数
		CF_VoiceVolume,		// 音声合成音量
		CF_TapeVolume,		// TAPEモニタ音量
		CF_TapeLPF,			// TAPE LPFカットオフ周波数
		
		// [MOVIE] -------------------------------------------------
		CF_AviBpp,			// ビデオキャプチャ色深度
		
		// [FILES] -------------------------------------------------
		CF_ExtRom,			// 拡張ROMファイル名(起動時に自動マウント)
		CF_tape,			// TAPEファイル名(起動時に自動マウント)
		CF_save,			// TAPE(SAVE)ファイル名(SAVE時に自動マウント)
		CF_disk1,			// DISK1ファイル名(起動時に自動マウント)
		CF_disk2,			// DISK2ファイル名(起動時に自動マウント)
		CF_printer,			// プリンタファイル名
		
		// [PATH] --------------------------------------------------
		CF_RomPath,			// ROMパス
		CF_TapePath,		// TAPEパス
		CF_DiskPath,		// DISKパス
		CF_ExtRomPath,		// 拡張ROMパス
		CF_ImgPath,			// IMGパス
		CF_WavePath,		// WAVEパス
		CF_FontPath,		// フォントパス設定
		CF_DokoPath,		// どこでもSAVEパス
		
		// [CHECK] -------------------------------------------------
		CF_CkQuit,			// 終了時確認
		CF_SaveQuit,		// 終了時INI保存
		
		// [OPTION] ------------------------------------------------
		CF_UseSoldier,		// 戦士のカートリッジ使うフラグ
		
		// [KEY] ---------------------------------------------------
		CF_KeyRepeat,		// キーリピート
	};
	
	std::string str;
	
	for( TConfig x : allcf ){
		SetDefault( x, over );
	}



/*
	// [FILES] -------------------------------------------------
	// 拡張ROMファイル名(起動時に自動マウント)
	if( over || !cIni::GetEntry( "FILES", "ExtRom", str ) )
		SetPath( CF_ExtRom, P6VSTR2PATH( "" ) );
	
	// TAPEファイル名(起動時に自動マウント)
	if( over || !cIni::GetEntry( "FILES", "tape", str ) )
		SetPath( CF_tape, P6VSTR2PATH( "" ) );
	
	// TAPE(SAVE)ファイル名(SAVE時に自動マウント)
	if( over || !cIni::GetEntry( "FILES", "save", str ) ){
		OSD_AddPath( tpath, P6VSTR2PATH( DIR_TAPE ), P6VSTR2PATH( FILE_SAVE ) );
		OSD_AbsolutePath( tpath );
		SetPath( CF_save, tpath );
	}
	
	// DISK1ファイル名(起動時に自動マウント)
	if( over || !cIni::GetEntry( "FILES", "disk1", str ) )
		SetPath( CF_disk1, P6VSTR2PATH( "" ) );
	
	// DISK2ファイル名(起動時に自動マウント)
	if( over || !cIni::GetEntry( "FILES", "disk2", str ) )
		SetPath( CF_disk2, P6VSTR2PATH( "" ) );
	
	// プリンタファイル名
	if( over || !cIni::GetEntry( "FILES", "printer", str ) ){
		tpath = P6VSTR2PATH( FILE_PRINT );
		OSD_AbsolutePath( tpath );
		SetPath( CF_printer, tpath );
	}
	
	
	// [PATH] --------------------------------------------------
	// ROMパス
	if( over || !cIni::GetEntry( "PATH", "RomPath", str ) ){
		tpath = P6VSTR2PATH( DIR_ROM );
		OSD_AbsolutePath( tpath );
		SetPath( CF_RomPath, tpath );
	}
	
	// TAPEパス
	if( over || !cIni::GetEntry( "PATH", "TapePath", str ) ){
		tpath = P6VSTR2PATH( DIR_TAPE );
		OSD_AbsolutePath( tpath );
		SetPath( CF_TapePath, tpath );
	}
	
	// DISKパス
	if( over || !cIni::GetEntry( "PATH", "DiskPath", str ) ){
		tpath = P6VSTR2PATH( DIR_DISK );
		OSD_AbsolutePath( tpath );
		SetPath( CF_DiskPath, tpath );
	}
	
	// 拡張ROMパス
	if( over || !cIni::GetEntry( "PATH", "ExtRomPath", str ) ){
		tpath = P6VSTR2PATH( DIR_EXTROM );
		OSD_AbsolutePath( tpath );
		SetPath( CF_ExtRomPath, tpath );
	}
	
	// IMGパス
	if( over || !cIni::GetEntry( "PATH", "ImgPath", str ) ){
		tpath = P6VSTR2PATH( DIR_IMAGE );
		OSD_AbsolutePath( tpath );
		SetPath( CF_ImgPath, tpath );
	}
	
	// WAVEパス
	if( over || !cIni::GetEntry( "PATH", "WavePath", str ) ){
		tpath = P6VSTR2PATH( DIR_WAVE );
		OSD_AbsolutePath( tpath );
		SetPath( CF_WavePath, tpath );
	}
	
	// フォントパス設定
	if( over || !cIni::GetEntry( "PATH", "FontPath", str ) ){
		tpath = P6VSTR2PATH( DIR_FONT );
		OSD_AbsolutePath( tpath );
		SetPath( CF_FontPath, tpath );
	}
	
	// どこでもSAVEパス
	if( over || !cIni::GetEntry( "PATH", "DokoPath", str ) ){
		tpath = P6VSTR2PATH( DIR_DOKO );
		OSD_AbsolutePath( tpath );
		SetPath( CF_DokoPath, tpath );
	}
*/
	
	
	// [COLOR] -------------------------------------------------
	// パレット
	for( size_t i=16; i<STDColor.size(); i++ ){
		if( over || !cIni::GetEntry( "COLOR", Stringf( "COL%03d", i ), str ) )
			SetColor( i, STDColor[i] );
	}
	
	
	// [KEY] ---------------------------------------------------
	// キー定義
	for( auto &i : KeyIni )
		if( over || !cIni::GetEntry( "KEY", GetPCKeyName( i.PCKey ), str ) )
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
	ini->PutEntry( "GLOBAL", "Version",		"", VERSION );
	ini->PutValue( "GLOBAL", "Model",		"", GetValue( CF_Model ) );
	ini->PutValue( "GLOBAL", "FDD",			"", GetValue( CF_FDD ) );
	ini->PutYesNo( "GLOBAL", "ExtRam",		"", GetYesNo( CF_ExtRam ) );
	// OPTION
	ini->PutValue( "OPTION", "UseSoldier",	"", GetValue( CF_UseSoldier ) );
	
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
	ini->GetEntry( "GLOBAL", "Version", strva );
	if( strva != VERSION ){
		Error::SetError( Error::DokoDiffVersion );
		return false;
	}
	
	if( ini->GetValue( "GLOBAL", "Model",      st ) )	SetValue( CF_Model, st );
	if( ini->GetValue( "GLOBAL", "FDD",        st ) )	SetValue( CF_FDD, st );
	if( ini->GetYesNo( "GLOBAL", "ExtRam",     yn ) )	SetYesNo( CF_ExtRam, yn );
	if( ini->GetValue( "OPTION", "UseSoldier", st ) )	SetValue( CF_UseSoldier, st );
	
	return true;
}
