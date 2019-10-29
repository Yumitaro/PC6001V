#ifndef PC6001V_H_INCLUDED
#define PC6001V_H_INCLUDED


/////////////////////////////////////////////////////////////////////////////
// ビルドオプション (不要な項目はコメントアウトする)
/////////////////////////////////////////////////////////////////////////////
//#define	NOMONITOR	// モニタモードなし
//#define	USEFMGEN	// PSGにfmgenを使う


/////////////////////////////////////////////////////////////////////////////
// バージョン情報,機種名など
/////////////////////////////////////////////////////////////////////////////
#define	VERSION					"1.26"
#define	APPNAME					"PC6001V"
                    			
#define	P60NAME					"PC-6001"
#define	P61NAME					"PC-6001A"
#define	P62NAME					"PC-6001mk2"
#define	P66NAME					"PC-6601"
#define	P64NAME					"PC-6001mk2SR"
#define	P68NAME					"PC-6601SR"


/////////////////////////////////////////////////////////////////////////////
// オプション 初期値
/////////////////////////////////////////////////////////////////////////////
#define	DEFAULT_MODEL			(0)					// 機種 60:PC-6001 61:PC-6001A 62:PC-6001mk2 66:PC-6601 64:PC-6001mk2SR 68:PC-6601SR (0:自動選定)
#define	DEFAULT_EXTRAM			(1)					// 拡張RAM 0:なし 1:あり
#define	DEFAULT_REPEAT			(70)				// キーリピートの間隔(単位:ms 0で無効)
#define	DEFAULT_SAMPLE_RATE		(44100)				// サンプリングレート
#define	DEFAULT_SOUND_BUFFER	(1)					// サウンドバッファ長倍率(基本長はVSYNC)
#define	DEFAULT_MASTERVOL		(70)				// マスター音量
#define	DEFAULT_PSGVOL			(60)				// PSG音量
#define	DEFAULT_PSGLPF			(0)					// PSG LPFカットオフ周波数(0で無効)
#define	DEFAULT_VOICEVOL		(100)				// 音声合成音量
#define	DEFAULT_TAPEVOL			(10)				// TAPEモニタ音量
#define	DEFAULT_TAPELPF			(1540)				// TAPE LPFカットオフ周波数(0で無効)
#define	DEFAULT_TURBO			(true)				// Turbo TAPE Yes:有効 No:無効
#define	DEFAULT_BOOST			(false)				// BoostUp Yes:有効 No:無効
#define DEFAULT_MAXBOOST60		(8)					// BoostUp最大倍率(N60モード)
#define DEFAULT_MAXBOOST62		(5)					// BoostUp最大倍率(N60m/N66モード)
#define	DEFAULT_BAUD			(1200)				// CMTボーレート
#define	DEFAULT_STOPBIT			(3)					// CMTストップビット数
#define	DEFAULT_FDD				(0)					// FDD接続台数
#define	DEFAULT_FDDWAIT			(true)				// FDDウェイト true:有効 false:無効
#define	DEFAULT_MODE4_COLOR		(1)					// モード４カラーモード 0:モノ 1:赤/青 2:青/赤 3:ピンク/緑 4:緑/ピンク
#define	DEFAULT_SCANLINE		(true) 				// スキャンライン true:あり false:なし
#define	DEFAULT_SCANLINEBR		(75)				// スキャンライン輝度 (0-100)%
#define	DEFAULT_FILTERING		(true) 				// フィルタリング true:アンチエイリアシング false:ニアレストネイバー
#define	DEFAULT_DISPNTSC		(true) 				// 4:3表示 true:有効 false:無効
#define	DEFAULT_FRAMESKIP		(0)					// フレームスキップ
#define	DEFAULT_WINDOWZOOM		(200)				// ウィンドウ表示倍率
#define	DEFAULT_OVERCLOCK		(100)				// オーバークロック率
#define	DEFAULT_CHECKCRC		(true) 				// CRCチェック
#define	DEFAULT_FULLSCREEN		(false) 			// フルスクリーン
#define	DEFAULT_DISPSTATUS		(true) 				// ステータスバー表示状態
#define	DEFAULT_AVIBPP			(24) 				// ビデオキャプチャ色深度
#define	DEFAULT_CKQUIT			(false) 			// 終了時確認
#define	DEFAULT_SAVEQUIT		(true) 				// 終了時INI保存
#define	DEFAULT_SOLDIER			(0) 				// 戦士のカートリッジ使うフラグ

#define	MIN_MODEL				(60)				// 機種 最小値
#define	MAX_MODEL				(68)				// 機種 最大値
#define	MIN_FDD					(0)					// FDD接続台数 最小値
#define	MAX_FDD					(2)					// FDD接続台数 最大値
#define	MIN_MAXBOOST			(1)					// BoostUp最大倍率 最小値
#define	MAX_MAXBOOST			(10)				// BoostUp最大倍率 最大値
#define	MIN_STOPBIT				(2)					// TAPEストップビット数 最小値
#define	MAX_STOPBIT				(10)				// TAPEストップビット数 最大値
#define	MIN_REPEAT				(10)				// キーリピートの間隔 最小値
#define	MAX_REPEAT				(100)				// キーリピートの間隔 最大値
#define	MIN_SAMPLE_RATE			(11025)				// サンプリングレート 最小値
#define	MAX_SAMPLE_RATE			(44100)				// サンプリングレート 最大値
#define	MIN_SOUNDBUFFER			(1)					// サウンドバッファ長倍率 最小値
#define	MAX_SOUNDBUFFER			(10)				// サウンドバッファ長倍率 最大値
#define	MIN_VOLUME				(0)					// 音量 最小値
#define	MAX_VOLUME				(100)				// 音量 最大値
#define	MIN_LPF					(0)					// LPFカットオフ周波数 最小値
#define	MAX_LPF					(20000)				// LPFカットオフ周波数 最大値
#define	MIN_MODE4_COLOR			(0)					// モード４カラーモード 最小値
#define	MAX_MODE4_COLOR			(4)					// モード４カラーモード 最大値
#define	MIN_SCANLINEBR			(0)					// スキャンライン輝度 最小値
#define	MAX_SCANLINEBR			(100)				// スキャンライン輝度 最大値
#define	MIN_FRAMESKIP			(0)					// フレームスキップ 最小値
#define	MAX_FRAMESKIP			(60)				// フレームスキップ 最大値
#define	MIN_WINDOWZOOM			(50)				// ウィンドウ表示倍率 最小値
#define	MAX_WINDOWZOOM			(400)				// ウィンドウ表示倍率 最大値
#define	MIN_OVERCLOCK			(1)					// オーバークロック率 最小値
#define	MAX_OVERCLOCK			(1000)				// オーバークロック率 最大値
#define	MIN_AVIBPP				(16)				// ビデオキャプチャ色深度 最小値
#define	MAX_AVIBPP				(32)				// ビデオキャプチャ色深度 最大値

#define	CPUM_CLOCK60			(3993600)			// メインCPUクロック(Hz) 60,62
#define	CPUM_CLOCK66			(4000000)			// メインCPUクロック(Hz) 66
#define	CPUM_CLOCK64			(3580000)			// メインCPUクロック(Hz) 64,68
#define	CPUS_CLOCK60			(CPUM_CLOCK60*2)	// サブCPUクロック(Hz) 60,62
#define	CPUS_CLOCK66			(CPUM_CLOCK66*2)	// サブCPUクロック(Hz) 66
#define	CPUS_CLOCK64			(CPUM_CLOCK64*2)	// サブCPUクロック(Hz) 64,68
#define	PSG_CLOCK60				(CPUM_CLOCK60/2)	// PSGクロック(Hz) 60,62
#define	PSG_CLOCK66				(CPUM_CLOCK66/2)	// PSGクロック(Hz) 66
#define	PSG_CLOCK64				(4000000)			// PSGクロック(Hz) 64,68 CPUクロックとは別

#define	VSYNC_HZ				(59.922)			// VSYNC周波数


/////////////////////////////////////////////////////////////////////////////
// 各種ディレクトリ名 初期値
/////////////////////////////////////////////////////////////////////////////
#define	DIR_ROM					"rom"				// ROMイメージ
#define	DIR_TAPE				"tape"				// TAPEイメージ
#define	DIR_DISK				"disk"				// DISKイメージ
#define	DIR_EXTROM				"extrom"			// 拡張ROMイメージ
#define	DIR_IMAGE				"img"				// スナップショット
#define	DIR_FONT				"font"				// フォント
#define	DIR_WAVE				"wave"				// WAVEファイル
#define	DIR_DOKO				"doko"				// どこでもSAVEファイル


/////////////////////////////////////////////////////////////////////////////
// 拡張子
/////////////////////////////////////////////////////////////////////////////
#define	EXT_IMG					"png"				// 画像ファイル拡張子
#define	EXT_RES					"resume"			// リプレイ途中保存用拡張子
#define	EXT_P6RAW				"p6"				// TAPEイメージ(RAW)
#define	EXT_CAS					"cas"				// TAPEイメージ(RAW)
#define	EXT_P6T					"p6t"				// TAPEイメージ(P6T)
#define	EXT_DISK				"d88"				// DISKイメージ
#define	EXT_ROM1				"rom"				// 拡張ROMイメージ
#define	EXT_ROM2				"bin"				// 拡張ROMイメージ
#define	EXT_DOKO				"dds"				// どこでもSAVE
#define	EXT_REPLAY				"ddr"				// リプレイ
#define	EXT_ATYPE1				"bas"				// 打ち込み代行
#define	EXT_ATYPE2				"txt"				// 打ち込み代行


/////////////////////////////////////////////////////////////////////////////
// 各種ファイル名 初期値
/////////////////////////////////////////////////////////////////////////////
#define	FILE_SNAP				"P6V"				// スナップショットプレフィックス
#define	FILE_CONFIG				"pc6001v.ini"		// 設定ファイル
#define	FILE_PRINTER			"printer.txt"		// プリンタ出力ファイル
#define	FILE_SERIAL				"serial.txt"		// シリアル出力ファイル
#define	FILE_SAVE				"_csave." EXT_P6T	// TAPE(CSAVE)ファイル
#define	FILE_FONTZ				"fontz12." EXT_IMG	// 半角フォントファイル
#define	FILE_FONTH				"fonth12." EXT_IMG	// 全角フォントファイル
                        
#define	SUBCPU60				"subcpu.60"			// サブCPU ROM(PC-6001)
#define	SUBCPU61				"subcpu.61"			// サブCPU ROM(PC-6001A)
#define	SUBCPU62				"subcpu.62"			// サブCPU ROM(PC-6001mk2)
#define	SUBCPU66				"subcpu.66"			// サブCPU ROM(PC-6601)
#define	SUBCPU64				"subcpu.64"			// サブCPU ROM(PC-6001mk2SR)
#define	SUBCPU68				"subcpu.68"			// サブCPU ROM(PC-6601SR)


#endif	// PC6001V_H_INCLUDED
