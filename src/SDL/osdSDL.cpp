/////////////////////////////////////////////////////////////////////////////
//  P C 6 0 0 1 V
//  Copyright 1999,2021 Yumitaro
/////////////////////////////////////////////////////////////////////////////
// SDL依存ルーチン
/////////////////////////////////////////////////////////////////////////////
#include <cstdarg>
#include <cstring>
#include <string>
#include <unordered_map>

#include <SDL.h>
#include <SDL_syswm.h>

#include "../common.h"
#include "../log.h"
#include "../osd.h"
#include "../vsurface.h"

#include "icons2.h"


/////////////////////////////////////////////////////////////////////////////
// SDL関連
/////////////////////////////////////////////////////////////////////////////
//#define USESDLMESSAGEBOX			// SDLのMESSAGEBOXを使用

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
#define	AUDIOFORMAT	AUDIO_S16MSB	// 16ビット符号あり
#else
#define	AUDIOFORMAT	AUDIO_S16		// 16ビット符号あり
#endif

// Renderer,Texture作成用オプション
#define SDLOP_SCREEN	(SDL_RENDERER_ACCELERATED|SDL_RENDERER_TARGETTEXTURE)


/////////////////////////////////////////////////////////////////////////////
// スタティック変数
/////////////////////////////////////////////////////////////////////////////
static SDL_Texture* sdl_texwx;						// 汎用Texture
static SDL_Texture* sdl_texbb;						// バックバッファ用Texture
static SDL_Texture* sdl_texsl;						// スキャンライン用Texture
static DWORD sdl_format = SDL_PIXELFORMAT_UNKNOWN;	// Renderer,Textureフォーマット
static SDL_AudioDeviceID sdl_adev;					// オーディオデバイス
static DWORD UEVnum = -1;							// 確保済みユーザー定義イベント数
//static P6VPATH ConfigPath = "";					// 設定ファイルパス保存用



/////////////////////////////////////////////////////////////////////////////
// 定数
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// SDLスキャンコード  -> 仮想キーコード 変換テーブル
/////////////////////////////////////////////////////////////////////////////
static std::unordered_map<SDL_Scancode, PCKEYsym> VKMapTable =
{
	{ SDL_SCANCODE_UNKNOWN,			KVC_UNKNOWN },
	
	{ SDL_SCANCODE_1,				KVC_1 },			// 1	!
	{ SDL_SCANCODE_2,				KVC_2 },			// 2	"
	{ SDL_SCANCODE_3,				KVC_3 },			// 3	#
	{ SDL_SCANCODE_4,				KVC_4 },			// 4	$
	{ SDL_SCANCODE_5,				KVC_5 },			// 5	%
	{ SDL_SCANCODE_6,				KVC_6 },			// 6	&
	{ SDL_SCANCODE_7,				KVC_7 },			// 7	'
	{ SDL_SCANCODE_8,				KVC_8 },			// 8	(
	{ SDL_SCANCODE_9,				KVC_9 },			// 9	)
	{ SDL_SCANCODE_0,				KVC_0 },			// 0
	
	{ SDL_SCANCODE_A,				KVC_A },			// a	A
	{ SDL_SCANCODE_B,				KVC_B },			// b	B
	{ SDL_SCANCODE_C,				KVC_C },			// c	C
	{ SDL_SCANCODE_D,				KVC_D },			// d	D
	{ SDL_SCANCODE_E,				KVC_E },			// e	E
	{ SDL_SCANCODE_F,				KVC_F },			// f	F
	{ SDL_SCANCODE_G,				KVC_G },			// g	G
	{ SDL_SCANCODE_H,				KVC_H },			// h	H
	{ SDL_SCANCODE_I,				KVC_I },			// i	I
	{ SDL_SCANCODE_J,				KVC_J },			// j	J
	{ SDL_SCANCODE_K,				KVC_K },			// k	K
	{ SDL_SCANCODE_L,				KVC_L },			// l	L
	{ SDL_SCANCODE_M,				KVC_M },			// m	M
	{ SDL_SCANCODE_N,				KVC_N },			// n	N
	{ SDL_SCANCODE_O,				KVC_O },			// o	O
	{ SDL_SCANCODE_P,				KVC_P },			// p	P
	{ SDL_SCANCODE_Q,				KVC_Q },			// q	Q
	{ SDL_SCANCODE_R,				KVC_R },			// r	R
	{ SDL_SCANCODE_S,				KVC_S },			// s	S
	{ SDL_SCANCODE_T,				KVC_T },			// t	T
	{ SDL_SCANCODE_U,				KVC_U },			// u	U
	{ SDL_SCANCODE_V,				KVC_V },			// v	V
	{ SDL_SCANCODE_W,				KVC_W },			// w	W
	{ SDL_SCANCODE_X,				KVC_X },			// x	X
	{ SDL_SCANCODE_Y,				KVC_Y },			// y	Y
	{ SDL_SCANCODE_Z,				KVC_Z },			// z	Z
	
	{ SDL_SCANCODE_F1,				KVC_F1 },			// F1
	{ SDL_SCANCODE_F2,				KVC_F2 },			// F2
	{ SDL_SCANCODE_F3,				KVC_F3 },			// F3
	{ SDL_SCANCODE_F4,				KVC_F4 },			// F4
	{ SDL_SCANCODE_F5,				KVC_F5 },			// F5
	{ SDL_SCANCODE_F6,				KVC_F6 },			// F6
	{ SDL_SCANCODE_F7,				KVC_F7 },			// F7
	{ SDL_SCANCODE_F8,				KVC_F8 },			// F8
	{ SDL_SCANCODE_F9,				KVC_F9 },			// F9
	{ SDL_SCANCODE_F10,				KVC_F10 },			// F10
	{ SDL_SCANCODE_F11,				KVC_F11 },			// F11
	{ SDL_SCANCODE_F12,				KVC_F12 },			// F12
	
	{ SDL_SCANCODE_MINUS,			KVC_MINUS },		// -	=
	{ SDL_SCANCODE_EQUALS,			KVC_CARET },		// ^	~
	{ SDL_SCANCODE_BACKSPACE,		KVC_BACKSPACE },	// BackSpace
	{ SDL_SCANCODE_LEFTBRACKET,		KVC_AT },			// @	`
	{ SDL_SCANCODE_RIGHTBRACKET,	KVC_LBRACKET },		// [	{
	{ SDL_SCANCODE_BACKSLASH,		KVC_RBRACKET },		// ]	}
	{ SDL_SCANCODE_SEMICOLON,		KVC_SEMICOLON },	// ;	+
	{ SDL_SCANCODE_APOSTROPHE,		KVC_COLON },		// :	*
	{ SDL_SCANCODE_COMMA,			KVC_COMMA },		// ,	<
	{ SDL_SCANCODE_PERIOD,			KVC_PERIOD },		// .	>
	{ SDL_SCANCODE_SLASH,			KVC_SLASH },		// /	?
	{ SDL_SCANCODE_SPACE,			KVC_SPACE },		// Space
	
	{ SDL_SCANCODE_ESCAPE,			KVC_ESC },			// ESC
	{ SDL_SCANCODE_TAB,				KVC_TAB },			// Tab
	{ SDL_SCANCODE_CAPSLOCK,		KVC_CAPSLOCK },		// CapsLock
	{ SDL_SCANCODE_RETURN,			KVC_ENTER },		// Enter
	{ SDL_SCANCODE_LCTRL,			KVC_LCTRL },		// L-Ctrl
	{ SDL_SCANCODE_RCTRL,			KVC_RCTRL },		// R-Ctrl
	{ SDL_SCANCODE_LSHIFT,			KVC_LSHIFT },		// L-Shift
	{ SDL_SCANCODE_RSHIFT,			KVC_RSHIFT },		// R-Shift
	{ SDL_SCANCODE_LALT,			KVC_LALT },			// L-Alt
	{ SDL_SCANCODE_RALT,			KVC_RALT },			// R-Alt
	{ SDL_SCANCODE_PRINTSCREEN,		KVC_PRINT },		// PrintScreen
	{ SDL_SCANCODE_SCROLLLOCK,		KVC_SCROLLLOCK },	// ScrollLock
	{ SDL_SCANCODE_PAUSE,			KVC_PAUSE },		// Pause
	{ SDL_SCANCODE_INSERT,			KVC_INSERT },		// Insert
	{ SDL_SCANCODE_DELETE,			KVC_DELETE },		// Delete
	{ SDL_SCANCODE_END,				KVC_END },			// End
	{ SDL_SCANCODE_HOME,			KVC_HOME },			// Home
	{ SDL_SCANCODE_PAGEUP,			KVC_PAGEUP },		// PageUp
	{ SDL_SCANCODE_PAGEDOWN,		KVC_PAGEDOWN },		// PageDown
	
	{ SDL_SCANCODE_UP,				KVC_UP },			// ↑
	{ SDL_SCANCODE_DOWN,			KVC_DOWN },			// ↓
	{ SDL_SCANCODE_LEFT,			KVC_LEFT },			// ←
	{ SDL_SCANCODE_RIGHT,			KVC_RIGHT },		// →
	
	{ SDL_SCANCODE_KP_0,			KVC_P0 },			// [0]
	{ SDL_SCANCODE_KP_1,			KVC_P1 },			// [1]
	{ SDL_SCANCODE_KP_2,			KVC_P2 },			// [2]
	{ SDL_SCANCODE_KP_3,			KVC_P3 },			// [3]
	{ SDL_SCANCODE_KP_4,			KVC_P4 },			// [4]
	{ SDL_SCANCODE_KP_5,			KVC_P5 },			// [5]
	{ SDL_SCANCODE_KP_6,			KVC_P6 },			// [6]
	{ SDL_SCANCODE_KP_7,			KVC_P7 },			// [7]
	{ SDL_SCANCODE_KP_8,			KVC_P8 },			// [8]
	{ SDL_SCANCODE_KP_9,			KVC_P9 },			// [9]
	{ SDL_SCANCODE_NUMLOCKCLEAR,	KVC_NUMLOCK },		// NumLock
	{ SDL_SCANCODE_KP_PLUS,			KVC_P_PLUS },		// [+]
	{ SDL_SCANCODE_KP_MINUS,		KVC_P_MINUS },		// [-]
	{ SDL_SCANCODE_KP_MULTIPLY,		KVC_P_MULTIPLY },	// [*]
	{ SDL_SCANCODE_KP_DIVIDE,		KVC_P_DIVIDE },		// [/]
	{ SDL_SCANCODE_KP_PERIOD,		KVC_P_PERIOD },		// [.]
	{ SDL_SCANCODE_KP_ENTER,		KVC_P_ENTER },		// [Enter]
	
	// 日本語キーボードのみ
	{ SDL_SCANCODE_NONUSBACKSLASH,	KVC_UNDERSCORE },	// ￥	_	(半角モード時?)
	{ SDL_SCANCODE_INTERNATIONAL1,	KVC_UNDERSCORE },	// ￥	_	(全角モード時?)
	{ SDL_SCANCODE_INTERNATIONAL2,	KVC_HIRAGANA },		// ひらがな
	{ SDL_SCANCODE_INTERNATIONAL3,	KVC_YEN },			// ￥	|
	{ SDL_SCANCODE_INTERNATIONAL4,	KVC_HENKAN },		// 変換
	{ SDL_SCANCODE_INTERNATIONAL5,	KVC_MUHENKAN },		// 無変換
	{ SDL_SCANCODE_GRAVE,			KVC_HANZEN },		// 半角/全角
	
	// 追加キー
	{ SDL_SCANCODE_LGUI,			KVX_LMETA },		// L-Meta
	{ SDL_SCANCODE_RGUI,			KVX_RMETA },		// R-Meta
	{ SDL_SCANCODE_APPLICATION,		KVX_MENU }			// Menu
};


/////////////////////////////////////////////////////////////////////////////
// 仮想イベントタイプ -> SDLイベントタイプ 変換テーブル
/////////////////////////////////////////////////////////////////////////////
static std::unordered_map<EventType, DWORD> EvConv =
{
	{ EV_QUIT,					SDL_QUIT						},	// User-requested quit
	{ EV_DROPFILE,				SDL_DROPFILE					},	// File dropped
	{ EV_KEYDOWN,				SDL_KEYDOWN						},	// Keys pressed
	{ EV_KEYUP,					SDL_KEYUP						},	// Keys released
	{ EV_MOUSEMOTION,			SDL_MOUSEMOTION					},	// Mouse moved
	{ EV_MOUSEBUTTONDOWN,		SDL_MOUSEBUTTONDOWN				},	// Mouse button pressed
	{ EV_MOUSEBUTTONUP,			SDL_MOUSEBUTTONUP				},	// Mouse button released
	{ EV_MOUSEWHEEL,			SDL_MOUSEWHEEL					},	// Mouse wheel motion
	{ EV_JOYAXISMOTION,			SDL_JOYAXISMOTION				},	// Joystick axis motion
	{ EV_JOYBUTTONDOWN,			SDL_JOYBUTTONDOWN				},	// Joystick button pressed
	{ EV_JOYBUTTONUP,			SDL_JOYBUTTONUP					},	// Joystick button released
	{ EV_WINDOWRESIZED,			SDL_WINDOWEVENT_RESIZED			},	// Window resized
	{ EV_WINDOWSIZECHANGED,		SDL_WINDOWEVENT_SIZE_CHANGED	},	// Window size changed
	{ EV_WINDOWEVENT_MINIMIZED,	SDL_WINDOWEVENT_MINIMIZED		},	// Window minimized
	{ EV_WINDOWEVENT_MAXIMIZED,	SDL_WINDOWEVENT_MAXIMIZED		},	// Window maximized
	{ EV_WINDOWEVENT_RESTORED,	SDL_WINDOWEVENT_RESTORED		}	// Window restored to normal size and position
};


/////////////////////////////////////////////////////////////////////////////
// イベント変換(仮想 -> SDL)
//
// 引数:	ev				イベントタイプ
// 返値:	DWORD			SDLイベントタイプ
/////////////////////////////////////////////////////////////////////////////
static DWORD ConvEventOSD2SDL( EventType ev )
{
	DWORD type;
	
	switch( ev ){
	case EV_RESTART:
	case EV_DOKOLOAD:
	case EV_REPLAYPLAY:
	case EV_REPLAYRESUME:
	case EV_REPLAYMOVIE:
	case EV_FPSUPDATE:
	case EV_RENDER:
	case EV_CAPTURE:
	#ifndef NOMONITOR	// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	case EV_DEBUGMODEBP:
	case EV_DEBUGMODETOGGLE:
	#endif				// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
		type = ev + UEVnum;
		break;
		
	default:
		try{
			type = EvConv.at( ev );
		}
		catch( std::out_of_range& ){
			type = SDL_FIRSTEVENT;
		}
	}
	
	return type;
}




/////////////////////////////////////////////////////////////////////////////
// 論理座標を絶対座標に変換する(マウスボタンイベント用)
//
// 引数:	winid			ウィンドウID
//			x				X座標
//			y				Y座標
// 返値:	なし
/////////////////////////////////////////////////////////////////////////////
static void ConvertLogicalToAbsolute( DWORD winid, int* x, int* y )
{
	int wa, ha, wl, hl;
	
	SDL_GetWindowSize( SDL_GetWindowFromID( winid ), &wa, &ha );
	SDL_RenderGetLogicalSize( SDL_GetRenderer( SDL_GetWindowFromID( winid ) ), &wl, &hl );
	*x = (*x * wa) / wl;
	*y = (*y * ha) / hl;
}






/////////////////////////////////////////////////////////////////////////////
// 初期化Sub(ライブラリ依存処理等)
//
// 引数:	なし
// 返値:	bool			true:成功 false:失敗
/////////////////////////////////////////////////////////////////////////////
bool OSD_Init_Sub( void )
{
	// SDL初期化
	if( SDL_Init( SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_JOYSTICK ) )
		return false;
	
	// ユーザー定義イベント確保
	UEVnum = SDL_RegisterEvents( EV_QUIT );
	if( UEVnum == (DWORD)-1 ){
		SDL_Quit();
		return false;
	}
	
	return true;
}


/////////////////////////////////////////////////////////////////////////////
// 終了処理Sub(ライブラリ依存処理等)
//
// 引数:	なし
// 返値:	なし
/////////////////////////////////////////////////////////////////////////////
void OSD_Quit_Sub( void )
{
	// SDL終了
	SDL_Quit();
}




/*
/////////////////////////////////////////////////////////////////////////////
// 設定ファイルパス取得
//
// 引数:	なし
// 返値:	std::string&	取得した文字列への参照(UTF-8)
/////////////////////////////////////////////////////////////////////////////
const P6VPATH& OSD_GetConfigPath( void )
{
	if( ConfigPath.empty() ){
		char* str = SDL_GetPrefPath( DIR_CONFIG, DIR_CONFIG );	// 末尾には必ずデリミタがつく
		if( str ){
			ConfigPath = STR2P6VPATH( str );
			delete [] str;
		}
	}
	
	return ConfigPath;
}
*/


/////////////////////////////////////////////////////////////////////////////
// メッセージ表示
//
// 引数:	hwnd			親のウィンドウハンドル
//			mes				メッセージ文字列へのポインタ(UTF-8)
//			cap				ウィンドウキャプション文字列へのポインタ(UTF-8)
//			type			表示形式指示のフラグ
// 返値:	int				押されたボタンの種類
//								OSDR_OK:     OKボタン
//								OSDR_CANCEL: CANCELボタン
//								OSDR_YES:    YESボタン
//								OSDR_NO:     NOボタン
/////////////////////////////////////////////////////////////////////////////
/*
int OSD_Message( HWINDOW hwnd, const char* mes, const char* cap, int type )
{
	const SDL_MessageBoxButtonData bb_OKCANCEL[] = {
		{ SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, IDOK,     "OK"    },
		{ SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, IDCANCEL, "Cancel" }
	};
	const SDL_MessageBoxButtonData bb_YESNOCANCEL[] = {
		{ SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, IDYES,    "Yes"    },
		{ 0,                                       IDNO,     "No"     },
		{ SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, IDCANCEL, "Cancel" }
	};
	
	SDL_MessageBoxData mb;
    int res;
	
	// メッセージボックスのタイプ
	switch( type&0x000f ){
	case OSDM_OKCANCEL:		mb.numbuttons = 2;	mb.buttons = bb_OKCANCEL;		break;
	case OSDM_YESNO:		mb.numbuttons = 2;	mb.buttons = bb_YESNOCANCEL;	break;
	case OSDM_YESNOCANCEL:	mb.numbuttons = 3;	mb.buttons = bb_YESNOCANCEL;	break;
	default:				mb.numbuttons = 1;	mb.buttons = bb_OKCANCEL;
	}
	
	// メッセージボックスのアイコンタイプ
	switch( type&0x00f0 ){
	case OSDM_ICONERROR:	mb.flags = SDL_MESSAGEBOX_ERROR;		break;
//	case OSDM_ICONQUESTION:	mb.flags = MB_ICONQUESTION;	break;
	case OSDM_ICONWARNING:	mb.flags = SDL_MESSAGEBOX_WARNING;		break;
	case OSDM_ICONINFO:		mb.flags = SDL_MESSAGEBOX_INFORMATION;	break;
	default:				mb.flags = 0;
	}
	
	mb.window      = hwnd;
	mb.colorScheme = nullptr;
	mb.title       = cap;
	mb.message     = mes;
	
    SDL_ShowMessageBox( &mb, &res);
	
	switch( res ){
	case IDOK:	return OSDR_OK;
	case IDYES:	return OSDR_YES;
	case IDNO:	return OSDR_NO;
	default:	return OSDR_CANCEL;
	}
}
*/





/////////////////////////////////////////////////////////////////////////////
// キーリピート設定
//
// 引数:	repeat			キーリピートの間隔(ms) 0で無効
// 返値:	なし
/////////////////////////////////////////////////////////////////////////////
void OSD_SetKeyRepeat( int repeat )
{
	// SDL2.0にはキーリピートの設定がないらしい
	// 後で考える
}


/////////////////////////////////////////////////////////////////////////////
// 利用可能なジョイスティック数取得
//
// 引数:	なし
// 返値:	int				利用可能なジョイスティック数
/////////////////////////////////////////////////////////////////////////////
int OSD_GetJoyNum( void )
{
	return SDL_NumJoysticks();
}


/////////////////////////////////////////////////////////////////////////////
// ジョイスティック名取得
//
// 引数:	int				インデックス
// 返値:	std::string		ジョイスティック名文字列(UTF-8)
/////////////////////////////////////////////////////////////////////////////
const std::string OSD_GetJoyName( int index )
{
	const char* name = SDL_JoystickNameForIndex( index );
	std::string tname = name ? name : "(Unknown)";
	
	return tname;
}


/////////////////////////////////////////////////////////////////////////////
// ジョイスティックオープンされてる？
//
// 引数:	HJOYINFO		ジョイスティック情報へのポインタ
// 返値:	bool			true:OPEN false:CLOSE
/////////////////////////////////////////////////////////////////////////////
bool OSD_OpenedJoy( HJOYINFO joy )
{
	return joy && SDL_JoystickGetAttached( (SDL_Joystick*)joy ) ? true : false;
}


/////////////////////////////////////////////////////////////////////////////
// ジョイスティックオープン
//
// 引数:	int				インデックス
// 返値:	HJOYINFO		ジョイスティック情報へのポインタ
/////////////////////////////////////////////////////////////////////////////
HJOYINFO OSD_OpenJoy( int index )
{
	return (HJOYINFO)SDL_JoystickOpen( index );
}


/////////////////////////////////////////////////////////////////////////////
// ジョイスティッククローズ
//
// 引数:	int				インデックス
// 返値:	なし
/////////////////////////////////////////////////////////////////////////////
void OSD_CloseJoy( HJOYINFO jinfo )
{
	SDL_JoystickClose( (SDL_Joystick*)jinfo );
}


/////////////////////////////////////////////////////////////////////////////
// ジョイスティックの軸の数取得
//
// 引数:	HJOYINFO		ジョイスティック情報へのポインタ
// 返値:	int				軸の数
/////////////////////////////////////////////////////////////////////////////
int OSD_GetJoyNumAxes( HJOYINFO jinfo )
{
	return SDL_JoystickNumAxes( (SDL_Joystick*)jinfo );
}


/////////////////////////////////////////////////////////////////////////////
// ジョイスティックのボタンの数取得
//
// 引数:	HJOYINFO		ジョイスティック情報へのポインタ
// 返値:	int				ボタンの数
/////////////////////////////////////////////////////////////////////////////
int OSD_GetJoyNumButtons( HJOYINFO jinfo )
{
	return SDL_JoystickNumButtons( (SDL_Joystick*)jinfo );
}


/////////////////////////////////////////////////////////////////////////////
// ジョイスティックの軸の状態取得
//
// 引数:	HJOYINFO		ジョイスティック情報へのポインタ
//			int				軸の番号
// 返値:	int				軸の状態(-32768～32767)
/////////////////////////////////////////////////////////////////////////////
int OSD_GetJoyAxis( HJOYINFO jinfo, int num )
{
	return SDL_JoystickGetAxis( (SDL_Joystick*)jinfo, num );
}


/////////////////////////////////////////////////////////////////////////////
// ジョイスティックのボタンの状態取得
//
// 引数:	HJOYINFO		ジョイスティック情報へのポインタ
//			int				ボタンの番号
// 返値:	bool			ボタンの状態 true:ON false:OFF
/////////////////////////////////////////////////////////////////////////////
bool OSD_GetJoyButton( HJOYINFO jinfo, int num )
{
	return SDL_JoystickGetButton( (SDL_Joystick*)jinfo, num ) ? true : false;
}




/////////////////////////////////////////////////////////////////////////////
// オーディオデバイスオープン
//
// 引数:	obj				自分自身へのオブジェクトポインタ
//			callback		コールバック関数へのポインタ
//			rate			サンプリングレート
//			sample			バッファサイズ(サンプル数)
// 返値:	bool			true:成功 false:失敗
/////////////////////////////////////////////////////////////////////////////
bool OSD_OpenAudio( void* obj, CBF_SND callback, int rate, int samples )
{
	SDL_AudioSpec ASpec, AHave;				// オーディオスペック
	
	ASpec.freq     = rate;				// サンプリングレート
	ASpec.format   = AUDIOFORMAT;		// フォーマット
	ASpec.channels = 1;					// モノラル
	ASpec.samples  = samples;			// バッファサイズ(サンプル数)
	ASpec.callback = callback;			// コールバック関数の指定
	ASpec.userdata = obj;				// コールバック関数に自分自身のオブジェクトポインタを渡す
	
	// オーディオデバイスを開く
	if( !(sdl_adev = SDL_OpenAudioDevice( nullptr, 0, &ASpec, &AHave, 0 )) ) return false;
	
	return true;
}


/////////////////////////////////////////////////////////////////////////////
// オーディオデバイスクローズ
//
// 引数:	なし
// 返値:	なし
/////////////////////////////////////////////////////////////////////////////
void OSD_CloseAudio( void )
{
	SDL_CloseAudioDevice( sdl_adev );
	sdl_adev = 0;
}


/////////////////////////////////////////////////////////////////////////////
// 再生開始
//
// 引数:	なし
// 返値:	なし
/////////////////////////////////////////////////////////////////////////////
void OSD_StartAudio( void )
{
	SDL_PauseAudioDevice( sdl_adev, 0 );
}


/////////////////////////////////////////////////////////////////////////////
// 再生停止
//
// 引数:	なし
// 返値:	なし
/////////////////////////////////////////////////////////////////////////////
void OSD_StopAudio( void )
{
	SDL_PauseAudioDevice( sdl_adev, 1 );
}


/////////////////////////////////////////////////////////////////////////////
// 再生状態取得
//
// 引数:	なし
// 返値:	bool			true:再生中 false:停止中
/////////////////////////////////////////////////////////////////////////////
bool OSD_AudioPlaying( void )
{
	return SDL_GetAudioStatus() == SDL_AUDIO_PLAYING ? true : false;
}


/////////////////////////////////////////////////////////////////////////////
// Waveファイル読込み
// 　対応形式は 22050Hz以上,符号付き16bit,1ch
//
// 引数:	filepath		入力ファイルパス
//			buf				バッファポインタ格納ポインタ
//			len				ファイル長さ格納ポインタ
//			freq			サンプリングレート格納ポインタ
// 返値:	bool			true:成功 false:失敗
/////////////////////////////////////////////////////////////////////////////
bool OSD_LoadWAV( const P6VPATH& filepath, BYTE** buf, DWORD* len, int* freq )
{
	SDL_AudioSpec ws;
	
	if( !SDL_LoadWAV( P6VPATH2STR( filepath ).c_str(), &ws, buf, (Uint32*)len ) ) return false;
	
	if( ws.freq < 22050 || ws.format != AUDIO_S16 || ws.channels != 1 ){
		SDL_FreeWAV( *buf );
		return false;
	}
	
	*freq = ws.freq;
	
	return true;
}


/////////////////////////////////////////////////////////////////////////////
// Waveファイル開放
//
// 引数:	buf				バッファへのポインタ
// 返値:	なし
/////////////////////////////////////////////////////////////////////////////
void OSD_FreeWAV( BYTE* buf )
{
	SDL_FreeWAV( buf );
}


/////////////////////////////////////////////////////////////////////////////
// オーディオをロックする
//
// 引数:	なし
// 返値:	なし
/////////////////////////////////////////////////////////////////////////////
void OSD_LockAudio( void )
{
	SDL_LockAudioDevice( sdl_adev );
}


/////////////////////////////////////////////////////////////////////////////
// オーディオをアンロックする
//
// 引数:	なし
// 返値:	なし
/////////////////////////////////////////////////////////////////////////////
void OSD_UnlockAudio( void )
{
	SDL_UnlockAudioDevice( sdl_adev );
}




/////////////////////////////////////////////////////////////////////////////
// 指定時間待機
//
// 引数:	tms				待機時間(ms)
// 返値:	なし
/////////////////////////////////////////////////////////////////////////////
void OSD_Delay( DWORD tms )
{
	SDL_Delay( tms );
}


/////////////////////////////////////////////////////////////////////////////
// プロセス開始からの経過時間取得
//
// 引数:	なし
// 返値:	DWORD			経過時間(ms)
/////////////////////////////////////////////////////////////////////////////
DWORD OSD_GetTicks( void )
{
	return SDL_GetTicks();
}


/////////////////////////////////////////////////////////////////////////////
// タイマ追加
//
// 引数:	interval		割込み間隔(ms)
//			callback		コールバック関数
//			param			コールバック関数に渡す引数
// 返値:	TIMERID			タイマID(失敗したら0)
/////////////////////////////////////////////////////////////////////////////
TIMERID OSD_AddTimer( DWORD interval, CBF_TMR callback, void* param )
{
	return (TIMERID)SDL_AddTimer( interval, (SDL_TimerCallback)callback, param );
}


/////////////////////////////////////////////////////////////////////////////
// タイマ削除
//
// 引数:	id				タイマID
// 返値:	bool			true:成功 false:失敗
/////////////////////////////////////////////////////////////////////////////
bool OSD_DelTimer( TIMERID id )
{
	return SDL_RemoveTimer( (SDL_TimerID)id ) == SDL_TRUE ? true : false;
}




/////////////////////////////////////////////////////////////////////////////
// ウィンドウ作成
//
// 引数:	hwnd			ウィンドウハンドルへのポインタ
//			w				ウィンドウの幅
//			h				ウィンドウの高さ
//			sw				スクリーンの幅
//			sh				スクリーンの高さ
//			fsflag			true:フルスクリーン false:ウィンドウ
//			filter			true:拡縮時にフィルタリングする false:しない
//			scanbr			スキャンライン輝度
// 返値:	bool			true:成功 false:失敗
/////////////////////////////////////////////////////////////////////////////
bool OSD_CreateWindow( HWINDOW* hwnd, const int w, const int h, const int sw, const int sh, const bool fsflag, const bool filter, const int scanbr )
{
	PRINTD( OSD_LOG, "[OSD][OSD_CreateWindow] w:%d h:%d %s(w:%d h:%d)\n", w, h, fsflag ? "Full screen" : "Window", sw, sh );
	
	SDL_Renderer* rend;
	SDL_RendererInfo info;
	const char* RendSQstr[] = { "nearest", "linear", "best" };	// 0:nearest / 1:linear / 2:best
	
	
	// ウィンドウ作成
	if( !*hwnd ){
		*hwnd = (HWINDOW)SDL_CreateWindow( "", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, SDL_WINDOW_SHOWN );
//		*hwnd = (HWINDOW)SDL_CreateWindow( "", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE );
		if( !*hwnd ) return false;
	}
	
	// ハードウェアRenderer作成
	rend = SDL_GetRenderer( (SDL_Window*)*hwnd );
	if( !rend ){
		rend = SDL_CreateRenderer( (SDL_Window*)*hwnd, -1, SDLOP_SCREEN );
		if( !rend ){
			SDL_DestroyWindow( (SDL_Window*)*hwnd );
			*hwnd = nullptr;
			return false;
		}
	}
	SDL_RenderSetLogicalSize( rend, w, h );
	
	// 作成済みのTextureは一旦破棄
	if( sdl_texwx ) SDL_DestroyTexture( sdl_texwx );
	if( sdl_texbb ) SDL_DestroyTexture( sdl_texbb );
	if( sdl_texsl ) SDL_DestroyTexture( sdl_texsl );
	
	// Renderer,Textureのフォーマットを選定する
	SDL_GetRendererInfo( rend, &info );
	for( int i=(int)info.num_texture_formats-1; i>=0; i-- ){
		DWORD tx = info.texture_formats[i];
		if( SDL_BYTESPERPIXEL(tx) == 4 && (SDL_BITSPERPIXEL(tx) == 32 || SDL_BITSPERPIXEL(tx) == 24) &&
			(SDL_PIXELORDER(tx) == SDL_PACKEDORDER_ARGB || SDL_PIXELORDER(tx) == SDL_PACKEDORDER_XRGB) ){
			sdl_format = tx;
		}
	}
	
	// フルスクリーン？
	if( fsflag ){
		SDL_DisplayMode mode;
		
		mode.format       = sdl_format;
		mode.w            = w;
		mode.h            = h;
		mode.refresh_rate = 0;
		mode.driverdata   = 0;
		
		if( SDL_GetClosestDisplayMode( SDL_GetWindowDisplayIndex( (SDL_Window*)*hwnd ), &mode, &mode ) ){
			SDL_SetWindowDisplayMode( (SDL_Window*)*hwnd, &mode );
			SDL_SetWindowFullscreen( (SDL_Window*)*hwnd, SDL_WINDOW_FULLSCREEN_DESKTOP );
		}
	}else{
		SDL_SetWindowFullscreen( (SDL_Window*)*hwnd, 0 );
		SDL_SetWindowSize( (SDL_Window*)*hwnd, w, h );
	}
	
	OSD_ClearWindow( (SDL_Window*)*hwnd );
	
	
	// フィルタリング設定
	int rendsq = !filter															  ? 0
			   : (std::strncmp( "direct3d", info.name, std::strlen(info.name) ) >= 0) ? 2
			   : (std::strncmp( "opengl",   info.name, std::strlen(info.name) ) >= 0) ? 1
			   :																		0;
	SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, RendSQstr[rendsq] );
	
	// 汎用Texture作成
	sdl_texwx = SDL_CreateTexture( rend, sdl_format, SDL_TEXTUREACCESS_STREAMING, w,  h );
	
	// バックバッファ用Texture作成
	// SR高解像度に備えて幅は2倍する
	sdl_texbb = SDL_CreateTexture( rend, sdl_format, SDL_TEXTUREACCESS_STREAMING, sw * 2, sh );
	
	// スキャンライン用Texture作成
	sdl_texsl = SDL_CreateTexture( rend, sdl_format, SDL_TEXTUREACCESS_TARGET,    sw, sh * 2 );
	SDL_SetTextureBlendMode( sdl_texsl, SDL_BLENDMODE_BLEND );
	SDL_SetTextureAlphaMod( sdl_texsl, ((100 - scanbr) * 255)/100 );
	
	SDL_SetRenderTarget( rend, sdl_texsl );
	SDL_SetRenderDrawColor( rend, 0, 0, 0, 0 );		// 無色透明
	SDL_RenderClear( rend );
	SDL_SetRenderDrawColor( rend, 0, 0, 0, 255 );	// 黒 + alpha
	for( int yy=0; yy < sh*2; yy += 2 ){
		SDL_RenderDrawLine( rend, 0, yy, sw-1, yy );
	}
	SDL_SetRenderTarget( rend, nullptr );
	
	return *hwnd ? true : false;
}


/////////////////////////////////////////////////////////////////////////////
// ウィンドウ破棄
//
// 引数:	hwnd			ウィンドウハンドル
// 返値:	なし
/////////////////////////////////////////////////////////////////////////////
void OSD_DestroyWindow( HWINDOW hwnd )
{
	PRINTD( OSD_LOG, "[OSD][OSD_DestroyWindow]\n" );
	
	if( sdl_texsl ){ SDL_DestroyTexture( sdl_texsl );	sdl_texsl = nullptr; }
	if( sdl_texbb ){ SDL_DestroyTexture( sdl_texbb );	sdl_texbb = nullptr; }
	if( sdl_texwx ){ SDL_DestroyTexture( sdl_texwx );	sdl_texwx = nullptr; }
	
	SDL_Renderer* rend = SDL_GetRenderer( (SDL_Window*)hwnd );
	if( rend ) SDL_DestroyRenderer( rend );
	if( hwnd ) SDL_DestroyWindow( (SDL_Window*)hwnd );
}


/////////////////////////////////////////////////////////////////////////////
// ウィンドウの幅を取得
//
// 引数:	hwnd			ウィンドウハンドル
// 返値:	int				幅
/////////////////////////////////////////////////////////////////////////////
int OSD_GetWindowWidth( HWINDOW hwnd )
{
	int res = 0;
	if( hwnd ) SDL_GetWindowSize( (SDL_Window*)hwnd, &res, nullptr );
	return res;
}


/////////////////////////////////////////////////////////////////////////////
// ウィンドウの高さを取得
//
// 引数:	hwnd			ウィンドウハンドル
// 返値:	int				高さ
/////////////////////////////////////////////////////////////////////////////
int OSD_GetWindowHeight( HWINDOW hwnd )
{
	int res = 0;
	if( hwnd ) SDL_GetWindowSize( (SDL_Window*)hwnd, nullptr, &res );
	return res;
}


/////////////////////////////////////////////////////////////////////////////
// フルスクリーン?
//
// 引数:	hwnd			ウィンドウハンドル
// 返値:	bool			true:フルスクリーン表示 false:ウィンドウ表示
/////////////////////////////////////////////////////////////////////////////
bool OSD_IsFullScreen( HWINDOW hwnd )
{
	return (SDL_GetWindowFlags( (SDL_Window*)hwnd ) & SDL_WINDOW_FULLSCREEN);
}


/////////////////////////////////////////////////////////////////////////////
// フィルタリング有効?
//
// 引数:	hwnd			ウィンドウハンドル
// 返値:	bool			true:有効 false:無効
/////////////////////////////////////////////////////////////////////////////
bool OSD_IsFiltering( HWINDOW hwnd )
{
	const char* hint = SDL_GetHint( SDL_HINT_RENDER_SCALE_QUALITY );
	
	return ( hint && std::strncmp( hint, "nearest", 7 ) ) ? true : false;
}


/////////////////////////////////////////////////////////////////////////////
// ウィンドウのサイズ変更可否設定
//
// 引数:	hwnd			ウィンドウハンドル
// 返値:	bool			true:変更可 false:変更不可
/////////////////////////////////////////////////////////////////////////////
void OSD_SetWindowResizable( HWINDOW hwnd, bool resize )
{
	SDL_SetWindowResizable( (SDL_Window*)hwnd, (SDL_bool)resize );
}



/////////////////////////////////////////////////////////////////////////////
// ウィンドウクリア
//  色は0(黒)で決め打ち
//
// 引数:	hwnd			ウィンドウハンドル
// 返値:	なし
/////////////////////////////////////////////////////////////////////////////
void OSD_ClearWindow( HWINDOW hwnd )
{
	PRINTD( OSD_LOG, "[OSD][OSD_ClearWindow]\n" );
	
	SDL_Renderer* rend = SDL_GetRenderer( (SDL_Window*)hwnd );
	if( !rend ) return;
	
	SDL_SetRenderDrawColor( rend, 0, 0, 0, SDL_ALPHA_OPAQUE );
	SDL_RenderClear( rend );
	SDL_RenderPresent( rend );
}


/////////////////////////////////////////////////////////////////////////////
// ウィンドウ反映
//
// 引数:	hwnd			ウィンドウハンドル
// 返値:	なし
/////////////////////////////////////////////////////////////////////////////
void OSD_RenderWindow( HWINDOW hwnd )
{
	PRINTD( OSD_LOG, "[OSD][OSD_RenderWindow]\n" );
	
	SDL_Renderer* rend = SDL_GetRenderer( (SDL_Window*)hwnd );
	if( !rend ) return;
	
	SDL_RenderPresent( rend );
}


/////////////////////////////////////////////////////////////////////////////
// ウィンドウに転送(等倍)
//
// 引数:	hwnd			ウィンドウハンドル
//			src				転送元サーフェス
//			x				転送先x座標
//			y				転送先y座標
// 返値:	なし
/////////////////////////////////////////////////////////////////////////////
void OSD_BlitToWindow( HWINDOW hwnd, VSurface* src, const int x, const int y )
{
	PRINTD( OSD_LOG, "[OSD][OSD_BlitToWindow] x:%d y:%d\n", x, y );
	
	SDL_Rect src1,drc1;
	
	if( !hwnd || !src ) return;
	
	SDL_Renderer* rend = SDL_GetRenderer( (SDL_Window*)hwnd );
	if( !rend ) return;
	
	// 転送元VSurface範囲設定
	// ウィンドウからはみ出す部分はトリム
	SDL_RenderGetLogicalSize( rend, &src1.w, &src1.h );
	src1.x = max( 0, -x );
	src1.y = max( 0, -y );
	src1.w = min( src->Width()  + x, src1.w) - max( 0, x );
	src1.h = min( src->Height() + y, src1.h) - max( 0, y );
	
	if( src1.w <= 0 || src1.h <= 0 ) return;
	
	// 転送先Render範囲設定
	drc1.x = x;
	drc1.y = y;
	drc1.w = src1.w;
	drc1.h = src1.h;
	
	BYTE* psrc = (BYTE*)src->GetPixels().data() + src1.y * src->Pitch() + src1.x;
	int spp    = src->Pitch();
	
	void* ptex;
	int tpp;
	
	SDL_LockTexture( sdl_texwx, &drc1, &ptex, &tpp );
	if( !ptex ) return;
	
	for( int yy=0; yy < src1.h; yy++ ){
		BYTE*  tps =                 psrc + yy * spp;
		DWORD* tpd = (DWORD*)((BYTE*)ptex + yy * tpp);
		for( int xx=0; xx < src1.w; xx++ )
			*tpd++ = VSurface::GetColor( *tps++ );
	}
	
	SDL_UnlockTexture( sdl_texwx );
	SDL_RenderCopy( rend, sdl_texwx, &drc1, &drc1 );
}


/////////////////////////////////////////////////////////////////////////////
// ウィンドウに転送(拡大等)
//
// 引数:	hwnd			ウィンドウハンドル
//			src				転送元サーフェス
//			pos				転送先 座標/サイズ
//			scanen			スキャンラインフラグ
// 返値:	なし
/////////////////////////////////////////////////////////////////////////////
void OSD_BlitToWindowEx( HWINDOW hwnd, VSurface* src, const VRect* pos, const bool scanen )
{
	PRINTD( OSD_LOG, "[OSD][OSD_BlitToWindowEx] x:%d y:%d\n", pos ? pos->x : 0, pos ? pos->y : 0  );
	
	SDL_Rect src1;
	
	if( !hwnd || !src ) return;
	
	SDL_Renderer* rend = SDL_GetRenderer( (SDL_Window*)hwnd );
	if( !rend ) return;
	
	// 転送元/Texture範囲設定
	// バックバッファからはみ出す部分はトリム
	SDL_QueryTexture( sdl_texbb, nullptr, nullptr, &src1.w, &src1.h );
	src1.x = 0;
	src1.y = 0;
	src1.w = min( src->Width() , src1.w );
	src1.h = min( src->Height(), src1.h );
	
	if( src1.w <= 0 || src1.h <= 0 ) return;
	
	BYTE* psrc = (BYTE*)src->GetPixels().data();
	int spp    = src->Pitch();
	
	void* ptex;
	int tpp;
	SDL_LockTexture( sdl_texbb, &src1, &ptex, &tpp );
	if( !ptex ) return;
	
	for( int yy=0; yy < src1.h; yy++ ){
		BYTE*  tps =                 psrc + yy * spp;
		DWORD* tpd = (DWORD*)((BYTE*)ptex + yy * tpp);
		for( int xx=0; xx < src1.w; xx++ )
			*tpd++ = VSurface::GetColor( *tps++ );
	}
	
	SDL_UnlockTexture( sdl_texbb );
	SDL_RenderCopy( rend, sdl_texbb, &src1, (SDL_Rect*)pos );
	
	if( scanen ){
		src1.h *= 2;
		SDL_RenderCopy( rend, sdl_texsl, &src1, (SDL_Rect*)pos );
	}

}


/////////////////////////////////////////////////////////////////////////////
// ウィンドウのイメージデータ取得
//
// 引数:	hwnd			ウィンドウハンドル
//			pixels			転送先配列への参照
//			pos				保存する領域情報へのポインタ
//			pxfmt			ピクセルフォーマット
// 返値:	bool			true:成功 false:失敗
/////////////////////////////////////////////////////////////////////////////
bool OSD_GetWindowImage( HWINDOW hwnd, std::vector<BYTE>& pixels, VRect* pos, PixelFMT pxfmt )
{
	VRect src1;
	int fmt, dpt;
	
	if( !hwnd ) return false;
	
	SDL_Renderer* rend = SDL_GetRenderer( (SDL_Window*)hwnd );
	if( !rend ) return false;
	
	SDL_RenderGetLogicalSize( rend, &src1.w, &src1.h );
	dpt = ( pos ? pos->w : src1.w );
	switch( pxfmt ){
	case PX16RGB:
		fmt  = SDL_PIXELFORMAT_RGB555;
		dpt  = ((dpt * 16 + 31) / 32 ) * sizeof(DWORD);
		break;
		
	case PX24RGB:
		fmt  = SDL_PIXELFORMAT_RGB24;
		dpt  = ((dpt * 24 + 31) / 32 ) * sizeof(DWORD);
		break;
		
	case PX24BGR:
		fmt  = SDL_PIXELFORMAT_BGR24;
		dpt  = ((dpt * 24 + 31) / 32 ) * sizeof(DWORD);
		break;
		
	case PX32ARGB:
	default:
		fmt  = SDL_PIXELFORMAT_ARGB32;
		dpt *= sizeof(DWORD);
	}
	
	if( SDL_RenderReadPixels( rend, (SDL_Rect*)pos, fmt, (void*)(&pixels[0]), dpt ) )
		return false;
	
	return true;
}




/////////////////////////////////////////////////////////////////////////////
// アイコン設定
//
// 引数:	hwnd			ウィンドウハンドル
//			model			機種 60,61,62,66,64,68
// 返値:	なし
/////////////////////////////////////////////////////////////////////////////
void OSD_SetIcon( HWINDOW hwnd, int model )
{
	BYTE* ipix;
	switch( model ){
	case 61: ipix = (BYTE*)p61pix; break;
	case 62: ipix = (BYTE*)p62pix; break;
	case 66: ipix = (BYTE*)p66pix; break;
	case 64: ipix = (BYTE*)p64pix; break;
	case 68: ipix = (BYTE*)p68pix; break;
	default: ipix = (BYTE*)p60pix;
	}
	SDL_Surface* p6icon = SDL_CreateRGBSurfaceFrom( ipix, 32, 32, 32, 32 * 4, RMASK32, GMASK32, BMASK32, AMASK32 );
	SDL_SetWindowIcon( (SDL_Window*)hwnd, p6icon );
	SDL_FreeSurface( p6icon );
}


/////////////////////////////////////////////////////////////////////////////
// キャプション設定
//
// 引数:	hwnd			ウィンドウハンドル
//			str				キャプション文字列への参照
// 返値:	なし
/////////////////////////////////////////////////////////////////////////////
void OSD_SetWindowCaption( HWINDOW hwnd, const std::string& str )
{
	PRINTD( OSD_LOG, "[OSD][OSD_SetWindowCaption] %s\n", str.c_str() );
	
	if( hwnd ){
		SDL_SetWindowTitle( (SDL_Window*)hwnd, str.c_str() );
	}
}


/////////////////////////////////////////////////////////////////////////////
// マウスカーソル表示/非表示
//
// 引数:	disp			true:表示 false:非表示
// 返値:	なし
/////////////////////////////////////////////////////////////////////////////
void OSD_ShowCursor( bool disp )
{
	PRINTD( OSD_LOG, "[OSD][OSD_ShowCursor] %s\n", disp ? "Show" : "Hide" );
	
	SDL_ShowCursor( disp ? SDL_ENABLE : SDL_DISABLE );
}


/////////////////////////////////////////////////////////////////////////////
// OS依存のウィンドウハンドルを取得
//
// 引数:	hwnd			ウィンドウハンドル
// 返値:	void*			OS依存のウィンドウハンドル
/////////////////////////////////////////////////////////////////////////////
void* OSD_GetWindowHandle( HWINDOW hwnd )
{
	SDL_SysWMinfo WinInfo;
	
	SDL_VERSION( &WinInfo.version );
	if( SDL_GetWindowWMInfo( (SDL_Window*)hwnd, &WinInfo ) )
		return WinInfo.info.win.window;
	else
		return nullptr;
}


/////////////////////////////////////////////////////////////////////////////
// イベントキュークリア
//
// 引数:	なし
// 返値:	なし
/////////////////////////////////////////////////////////////////////////////
void OSD_FlushEvents( void )
{
	SDL_PumpEvents();
	SDL_FlushEvents( SDL_FIRSTEVENT, SDL_LASTEVENT );
}


/////////////////////////////////////////////////////////////////////////////
// イベント取得(イベントが発生するまで待つ)
//
// 引数:	ev				イベント情報共用体へのポインタ
// 返値:	bool			true:成功 false:失敗
/////////////////////////////////////////////////////////////////////////////
bool OSD_GetEvent( Event* ev )
{
	SDL_Event event;
	
	if( !SDL_WaitEvent( &event ) ) return false;
	
	switch( event.type ){
	case SDL_KEYDOWN:
		ev->type			= EV_KEYDOWN;
		ev->key.state		= true;
		ev->key.sym			= VKMapTable[ event.key.keysym.scancode ];	// 例外防止の為にatは使わない
		ev->key.mod			= (PCKEYmod)(
							  ( event.key.keysym.mod & KMOD_LSHIFT ? KVM_LSHIFT : KVM_NONE )
							| ( event.key.keysym.mod & KMOD_RSHIFT ? KVM_RSHIFT : KVM_NONE )
							| ( event.key.keysym.mod & KMOD_LCTRL  ? KVM_LCTRL  : KVM_NONE )
							| ( event.key.keysym.mod & KMOD_RCTRL  ? KVM_RCTRL  : KVM_NONE )
							| ( event.key.keysym.mod & KMOD_LALT   ? KVM_LALT   : KVM_NONE )
							| ( event.key.keysym.mod & KMOD_RALT   ? KVM_RALT   : KVM_NONE )
							| ( event.key.keysym.mod & KMOD_LGUI   ? KVM_LMETA  : KVM_NONE )
							| ( event.key.keysym.mod & KMOD_RGUI   ? KVM_RMETA  : KVM_NONE )
							| ( event.key.keysym.mod & KMOD_NUM    ? KVM_NUM    : KVM_NONE )
							| ( event.key.keysym.mod & KMOD_CAPS   ? KVM_CAPS   : KVM_NONE )
							| ( event.key.keysym.mod & KMOD_MODE   ? KVM_MODE   : KVM_NONE ) );
		ev->key.unicode		= GetKeyChar( ev->key.sym, ev->key.mod & KVM_SHIFT );
		break;
		
	case SDL_KEYUP:
		ev->type			= EV_KEYUP;
		ev->key.state		= false;
		ev->key.sym			= VKMapTable[ event.key.keysym.scancode ];	// 例外防止の為にatは使わない
		ev->key.mod			= (PCKEYmod)(
							  ( event.key.keysym.mod & KMOD_LSHIFT ? KVM_LSHIFT : KVM_NONE )
							| ( event.key.keysym.mod & KMOD_RSHIFT ? KVM_RSHIFT : KVM_NONE )
							| ( event.key.keysym.mod & KMOD_LCTRL  ? KVM_LCTRL  : KVM_NONE )
							| ( event.key.keysym.mod & KMOD_RCTRL  ? KVM_RCTRL  : KVM_NONE )
							| ( event.key.keysym.mod & KMOD_LALT   ? KVM_LALT   : KVM_NONE )
							| ( event.key.keysym.mod & KMOD_RALT   ? KVM_RALT   : KVM_NONE )
							| ( event.key.keysym.mod & KMOD_LGUI   ? KVM_LMETA  : KVM_NONE )
							| ( event.key.keysym.mod & KMOD_RGUI   ? KVM_RMETA  : KVM_NONE )
							| ( event.key.keysym.mod & KMOD_NUM    ? KVM_NUM    : KVM_NONE )
							| ( event.key.keysym.mod & KMOD_CAPS   ? KVM_CAPS   : KVM_NONE )
							| ( event.key.keysym.mod & KMOD_MODE   ? KVM_MODE   : KVM_NONE ) );
		ev->key.unicode		= GetKeyChar( ev->key.sym, ev->key.mod & KVM_SHIFT );
		break;
		
	case SDL_MOUSEMOTION:
		ev->type			= EV_MOUSEMOTION;
		break;
		
	case SDL_MOUSEBUTTONDOWN:
		ev->type			= EV_MOUSEBUTTONDOWN;
		ev->mousebt.button	= event.button.button == SDL_BUTTON_LEFT      ? MBT_LEFT      :
							  event.button.button == SDL_BUTTON_MIDDLE    ? MBT_MIDDLE    :
							  event.button.button == SDL_BUTTON_RIGHT     ? MBT_RIGHT     :
							  MBT_NONE;
		ev->mousebt.state	= true;
		ev->mousebt.x		= event.button.x;	// 論理座標
		ev->mousebt.y		= event.button.y;	// 論理座標
		// 絶対座標に変換
		ConvertLogicalToAbsolute( event.button.windowID, &ev->mousebt.x, &ev->mousebt.y );
		break;
		
	case SDL_MOUSEBUTTONUP:
		ev->type			= EV_MOUSEBUTTONUP;
		ev->mousebt.button	= event.button.button == SDL_BUTTON_LEFT      ? MBT_LEFT      :
							  event.button.button == SDL_BUTTON_MIDDLE    ? MBT_MIDDLE    :
							  event.button.button == SDL_BUTTON_RIGHT     ? MBT_RIGHT     :
							  MBT_NONE;
		ev->mousebt.state	= false;
		ev->mousebt.x		= event.button.x;	// 論理座標
		ev->mousebt.y		= event.button.y;	// 論理座標
		// 絶対座標に変換
		ConvertLogicalToAbsolute( event.button.windowID, &ev->mousebt.x, &ev->mousebt.y );
		break;
		
	case SDL_MOUSEWHEEL:
		ev->type			= EV_MOUSEWHEEL;
		ev->mousewh.x		= event.wheel.direction == SDL_MOUSEWHEEL_NORMAL ? event.wheel.x : -event.wheel.x;
		ev->mousewh.y		= event.wheel.direction == SDL_MOUSEWHEEL_NORMAL ? event.wheel.y : -event.wheel.y;
		break;
		
	case SDL_JOYAXISMOTION:
		ev->type			= EV_JOYAXISMOTION;
		ev->joyax.idx		= event.jaxis.which;
		ev->joyax.axis		= event.jaxis.axis;
		ev->joyax.value		= event.jaxis.value;
		break;
		
	case SDL_JOYBUTTONDOWN:
		ev->type			= EV_JOYBUTTONDOWN;
		ev->joybt.idx		= event.jbutton.which;
		ev->joybt.button	= event.jbutton.button;
		ev->joybt.state		= true;
		break;
		
	case SDL_JOYBUTTONUP:
		ev->type			= EV_JOYBUTTONUP;
		ev->joybt.idx		= event.jbutton.which;
		ev->joybt.button	= event.jbutton.button;
		ev->joybt.state		= false;
		break;
		
	case SDL_JOYDEVICEADDED:
		ev->type			= EV_JOYDEVICEADDED;
		ev->joydev.idx		= event.jdevice.which;
		break;
		
	case SDL_JOYDEVICEREMOVED:
		ev->type			= EV_JOYDEVICEREMOVED;
		ev->joydev.idx		= event.jdevice.which;
		break;
		
	case SDL_DROPFILE:
		ev->type			= event.drop.file ? EV_DROPFILE : EV_NOEVENT;
		ev->drop.file		= event.drop.file;
		break;
		
	case SDL_WINDOWEVENT:
		switch( event.window.event ){
		case SDL_WINDOWEVENT_RESIZED:
			ev->type		= EV_WINDOWRESIZED;
			ev->window.w	= event.window.data1;
			ev->window.h	= event.window.data2;
			break;
			
		case SDL_WINDOWEVENT_SIZE_CHANGED:
			ev->type		= EV_WINDOWSIZECHANGED;
			ev->window.w	= event.window.data1;
			ev->window.h	= event.window.data2;
			break;
			
		case SDL_WINDOWEVENT_MINIMIZED:
			ev->type		= EV_WINDOWEVENT_MINIMIZED;
			break;
			
		case SDL_WINDOWEVENT_MAXIMIZED:
			ev->type		= EV_WINDOWEVENT_MAXIMIZED;
			break;
			
		case SDL_WINDOWEVENT_RESTORED:
			ev->type		= EV_WINDOWEVENT_RESTORED;
			break;
			
		default:
			ev->type		= EV_NOEVENT;
		}
		break;
		
	case SDL_QUIT:
		ev->type			= EV_QUIT;
		break;
		
	default:
		if( event.type >= UEVnum && event.type < (UEVnum + SDL_QUIT) ){
			// ユーザー定義イベント
			ev->type = (EventType)(event.type - UEVnum);
			
			switch( ev->type ){
			case EV_RESTART:
			case EV_DOKOLOAD:
			case EV_REPLAYPLAY:
			case EV_REPLAYRESUME:
			case EV_REPLAYMOVIE:
			case EV_FPSUPDATE:
				break;
				
			#ifndef NOMONITOR	// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
			case EV_DEBUGMODEBP:
				ev->bp.addr			= event.user.code;
				break;
			#endif				// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
			default:
				break;
			}
		}else{
			ev->type		= EV_NOEVENT;
		}
	}
	
	return true;
}


/////////////////////////////////////////////////////////////////////////////
// イベントをキューにプッシュする
//
// 引数:	ev				イベントタイプ
//			...				イベントタイプに応じた引数
// 返値:	bool			true:成功 false:失敗
/////////////////////////////////////////////////////////////////////////////
bool OSD_PushEvent( EventType ev, ... )
{
	SDL_Event event;
	std::va_list args;
	
	event.type = ConvEventOSD2SDL( ev );
	if( event.type == SDL_FIRSTEVENT ){
		return false;
	}
	
	switch( ev ){
	#ifndef NOMONITOR	// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	case EV_DEBUGMODEBP:
		// C的可変長引数展開
		va_start( args, ev );
		event.user.code	= va_arg( args, int );
		va_end( args );
		break;
	case EV_DEBUGMODETOGGLE:
	#endif				// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	default:;
	}
	
	return SDL_PushEvent( &event ) < 0 ? false : true;
}


/////////////////////////////////////////////////////////////////////////////
// キューに指定のイベントが存在するか調査する
//
// 引数:	ev				イベントタイプ
// 返値:	bool			true:ある false:ない
/////////////////////////////////////////////////////////////////////////////
bool OSD_HasEvent( EventType ev )
{
	return SDL_HasEvent( ConvEventOSD2SDL( ev ) ) == SDL_TRUE ? true : false;
}


/////////////////////////////////////////////////////////////////////////////
// イベント処理の状態を種類ごとに設定する
//
// 引数:	ev				イベントタイプ
//			st				ステータス
// 返値:	bool			変更前の状態 true:有効 false:無効
/////////////////////////////////////////////////////////////////////////////
bool OSD_EventState( EventType ev, EventState st )
{
	int state;
	
	switch( st ){
	case EVS_QUERY:		state = SDL_QUERY;		break;
	case EVS_DISABLE:	state = SDL_DISABLE;	break;
	case EVS_ENABLE:	state = SDL_ENABLE;		break;
	default:
		return false;
	}
	
	return SDL_EventState( ConvEventOSD2SDL( ev ), state ) == SDL_ENABLE  ? true : false;
}

