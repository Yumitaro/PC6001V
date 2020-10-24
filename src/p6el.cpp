#include <algorithm>
#include <cctype>
#include <new>
#include <string>
#include <utility>

#include "pc6001v.h"
#include "id_menu.h"

#include "breakpoint.h"
#include "common.h"
#include "config.h"
#include "cpum.h"
#include "cpus.h"
#include "debug.h"
#include "disk.h"
#include "error.h"
#include "graph.h"
#include "intr.h"
#include "joystick.h"
#include "keyboard.h"
#include "log.h"
#include "memory.h"
#include "osd.h"
#include "p6el.h"
#include "pio.h"
#include "psgfm.h"
#include "schedule.h"
#include "sound.h"
#include "status.h"
#include "tape.h"
#include "vdg.h"
#include "voice.h"
#include "vsurface.h"


#define	FRAMERATE	((double)VSYNC_HZ/(double)(cfg->GetFrameSkip()+1))

int EL6::Speed = 100;



////////////////////////////////////////////////////////////////
// コンストラクタ
////////////////////////////////////////////////////////////////
EL6::EL6( void ) : cfg( nullptr ), vm( nullptr ), sche( nullptr ), snd( nullptr ),
	joy( nullptr ), graph( nullptr ), staw( nullptr ),
	#ifndef NOMONITOR	// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	regw( nullptr ), memw( nullptr ), monw( nullptr ),
	#endif				// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	UpDateFPSID( 0 ), FSkipCount( 0 ), MMotion( true ),
	TapePathUI( "" ), DiskPathUI( "" ), ExRomPathUI( "" ), DokoPathUI( "" )
{
	PRINTD( CONST_LOG, "[[EL6]]\n" );
}


////////////////////////////////////////////////////////////////
// デストラクタ
////////////////////////////////////////////////////////////////
EL6::~EL6( void )
{
	PRINTD( CONST_LOG, "[[~EL6]]\n" );
}



////////////////////////////////////////////////////////////////
// スレッド関数(オーバーライド)
////////////////////////////////////////////////////////////////
void EL6::OnThread( void* inst )
{
	EL6* p6 = STATIC_CAST( EL6*, inst );	// 自分自身のオブジェクトポインタ取得
	int st  = 0;
	
	
	#ifndef NOMONITOR	// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	if( p6->vm->IsMonitor() ){
	// モニタモード
		while( !this->cThread::IsCancel() ){
			// 画面更新
			if( p6->ScreenUpdate() ) OSD_PushEvent( EV_RENDER );
			
			// ウェイト
			p6->Wait();
		}
	}else
	// 通常モード
		if( p6->vm->bp->GetNum() ){
			// ブレークポイントあり
			while( !this->cThread::IsCancel() ){
				// ポーズ中なら画面更新のみ
				if( p6->sche->GetPauseEnable() ){
					// 画面更新
					if( p6->ScreenUpdate() ) OSD_PushEvent( EV_RENDER );
					
					// ウェイト
					p6->Wait();
				}else{
					st = p6->Emu();		// 1命令実行
					
					// ブレークポイントチェック(バスリクエスト期間中はチェックしない)
					if( st > 0 && ( p6->vm->bp->Check( BPoint::BP_PC, p6->vm->cpum->GetPC() ) || p6->vm->bp->GetReqNum() ) ){
						p6->vm->bp->Reset();
						OSD_PushEvent( EV_DEBUGMODEBP, p6->vm->cpum->GetPC() );
						break;	// ブレーク条件にヒットしたらスレッド抜ける
					}
					
					if( p6->vm->evsc->IsVSYNC() ){
						p6->vm->key->ScanMatrix();	// キーマトリクススキャン
						
						// サウンド更新
						p6->SoundUpdate( 0 );
						// 画面更新
						if( p6->ScreenUpdate() ) OSD_PushEvent( EV_RENDER );
						
						// 自動キー入力
						if( IsAutoKey() ){
							BYTE key = GetAutoKey();
							if( key ){
								if( key == 0x14 ) p6->vm->cpus->ReqKeyIntr( 6, GetAutoKey() );
								else			  p6->vm->cpus->ReqKeyIntr( 0, key );
							}
						}
						
						// ウェイト
						p6->Wait();
					}
				}
			}
		}else
	#endif				// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	{	// 通常実行
		while( !this->cThread::IsCancel() ){
			// ポーズ中なら画面更新のみ
			if( p6->sche->GetPauseEnable() ){
				// 画面更新
				if( p6->ScreenUpdate() ) OSD_PushEvent( EV_RENDER );
			}else{
				// キーマトリクススキャン
				bool matchg = p6->vm->key->ScanMatrix();
				
				// リプレイ記録中
				if( REPLAY::GetStatus() == REP_RECORD )
					REPLAY::ReplayWriteFrame( p6->vm->key->GetMatrix2(), matchg );
				
				// リプレイ再生中
				if( REPLAY::GetStatus() == REP_REPLAY )
					REPLAY::ReplayReadFrame( p6->vm->key->GetMatrix() );
				
				p6->EmuVSYNC();			// 1画面分実行
				
				// ビデオキャプチャ中?
				if( AVI6::IsAVI() ){
					// ビデオキャプチャ中ならここでAVI保存処理
					// サウンド更新
					p6->SoundUpdate( 0, AVI6::GetAudioBuffer() );
					// 画面更新されたら AVI1画面保存
					if( p6->ScreenUpdate() ) OSD_PushEvent( EV_CAPTURE );
				}else{
					// ビデオキャプチャ中でないなら通常の更新
					// サウンド更新
					p6->SoundUpdate( 0 );
					// 画面更新
					if( p6->ScreenUpdate() ) OSD_PushEvent( EV_RENDER );
				}
				
				// 自動キー入力
				if( IsAutoKey() ){
					BYTE key = GetAutoKey();
					if( key ){
						if( key == 0x14 ) p6->vm->cpus->ReqKeyIntr( 6, GetAutoKey() );
						else			  p6->vm->cpus->ReqKeyIntr( 0, key );
					}
				}
			}
			
			// ウェイト
			p6->Wait();
		}
	}
}


////////////////////////////////////////////////////////////////
// Wait
////////////////////////////////////////////////////////////////
void EL6::Wait( void )
{
	if( sche->GetWaitEnable() && (!cfg->GetTurboTAPE() || (vm->cpus->GetCmtStatus() == CMTCLOSE)) )
		sche->VWait();
	vm->evsc->ReVSYNC();
}


////////////////////////////////////////////////////////////////
// 1命令実行
////////////////////////////////////////////////////////////////
int EL6::Emu( void )
{
	int st = vm->Emu();				// VM 1命令実行
	int ste = st <= 0 ? 1 : st;
	vm->evsc->Update( ste );		// イベント更新
	sche->Update( ste );
	
	return st;
}


////////////////////////////////////////////////////////////////
// 1画面分実行
////////////////////////////////////////////////////////////////
int EL6::EmuVSYNC( void )
{
	int state = 0;
	
	// VSYNCが発生するまで繰返し
	while( !vm->evsc->IsVSYNC() ){
		int st = vm->Emu();		// VM 1命令実行
		if( st <= 0 ) st = 1;
		vm->evsc->Update( st );	// イベント更新
		sche->Update( st );
		state += st;
	}
	
	return state;
}


#ifndef NOMONITOR	// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
////////////////////////////////////////////////////////////////
// 指定ステート数実行
////////////////////////////////////////////////////////////////
void EL6::Exec( int st )
{
	int State = st;
	
	while( State > 0 ) State -= Emu();
}


////////////////////////////////////////////////////////////////
// モニタモード切替え
////////////////////////////////////////////////////////////////
void EL6::ToggleMonitor( void )
{
	// VM停止
	Stop();
	
	// モニタウィンドウ表示状態切換え
	vm->SetMonitor( !vm->IsMonitor() );
	
	// スクリーンサイズ変更
	graph->ResizeScreen();
	
	// VM再開
	Start();
}

#endif				// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@




////////////////////////////////////////////////////////////////
// 初期化
////////////////////////////////////////////////////////////////
bool EL6::Init( const std::shared_ptr<CFG6>& config )
{
	// エラーメッセージ初期値
	Error::SetError( Error::InitFailed );
	
	// 念の為
	StopFPSTimer();
	ak.Buffer.clear();
	
	if( !config ) return false;
	cfg = config;
	
	// パレット設定
	SetPalette();
	
	try{
		// 機種別 VM確保
		switch( cfg->GetModel() ){
		case 61: vm = std::make_unique<VM61>();	break;
		case 62: vm = std::make_unique<VM62>();	break;
		case 66: vm = std::make_unique<VM66>();	break;
		case 64: vm = std::make_unique<VM64>();	break;
		case 68: vm = std::make_unique<VM68>();	break;
		default: vm = std::make_unique<VM60>();
		}
		
		// 各種オブジェクト確保
		sche  = std::make_unique<SCH6>();			// スケジューラ
		snd   = std::make_unique<SND6>();			// サウンド
		joy   = std::make_unique<JOY6>();			// ジョイスティック
		graph = std::make_unique<DSP6>( this );		// 画面描画
		staw  = std::make_unique<cWndStat>();		// ステータスバー
		#ifndef NOMONITOR	// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
		regw  = std::make_unique<cWndReg>( vm );	// レジスタウィンドウ
		memw  = std::make_unique<cWndMem>( vm );	// メモリウィンドウ
		monw  = std::make_unique<cWndMon>( vm );	// モニタウィンドウ
		#endif				// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
		
		
		// VM初期化
		if( !vm->Init( cfg ) ) throw Error::GetError();
		
		// スケジューラ -----
		sche->SetMasterClock( vm->evsc->GetMasterClock() );
		
		// サウンド -----
		if( !snd->Init( this, EL6::StreamUpdate, cfg->GetSampleRate(), cfg->GetSoundBuffer() ) ) throw Error::GetError();
		snd->SetVolume( cfg->GetMasterVol() );
		
		// 画面描画 -----
		if( !graph->Init() ) throw Error::GetError();
		graph->SetIcon( cfg->GetModel() );	// アイコン設定
		
		// ジョイスティック -----
		if( !joy->Init() ) throw Error::GetError();
		
		// ステータスバー -----
		if( !staw->Init( graph->ScreenX(), cfg->GetFddNum() ) ) throw Error::GetError();
		
		#ifndef NOMONITOR	// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
		// レジスタウィンドウ　-----
		if( !regw->Init() ) throw Error::GetError();
		
		// メモリウィンドウ　-----
		if( !memw->Init() ) throw Error::GetError();
		
		// モニタウィンドウ　-----
		if( !monw->Init() ) throw Error::GetError();
		#endif				// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
		
		// ビデオキャプチャ -----
		if( !AVI6::Init() ) throw Error::GetError();
		
		// リプレイ -----
		if( !REPLAY::Init() ) throw Error::GetError();
		
		// スクリーンサイズ変更
		graph->ResizeScreen();
		
		// ストリーム接続
		snd->ConnectStream( vm->psg );		// PSG/OPN
		snd->ConnectStream( vm->cmtl );		// CMT(LOAD)
		snd->ConnectStream( vm->voice );	// 音声合成
		
		
		// TAPE挿入
		if( !cfg->GetTapeFile().empty() ) TapeMount( cfg->GetTapeFile() );
		
		// ドライブ1,2にDISK挿入
		if( !cfg->GetDiskFile( 1 ).empty() ) DiskMount( 0, cfg->GetDiskFile( 1 ) );
		if( !cfg->GetDiskFile( 2 ).empty() ) DiskMount( 1, cfg->GetDiskFile( 2 ) );
		
		// リセット
		UI_Reset();
		
	}
	catch( std::bad_alloc& ){	// new に失敗した場合
		Error::SetError( Error::MemAllocFailed );
		return false;
	}
	catch( Error::Errno i ){	// 例外発生
		return false;
	}
	
	// エラーなし
	Error::Reset();
	
	return true;
}


////////////////////////////////////////////////////////////////
// 動作開始
////////////////////////////////////////////////////////////////
bool EL6::Start( void )
{
	// 実行速度を復元
	while( sche->GetSpeedRatio() != Speed ){
		sche->SetSpeedRatio( Speed > 100 ? 1 : -1 );
	}
	
	FSkipCount = 0;
	
	// スレッド生成
	if( !this->cThread::BeginThread( this ) ) return false;
	
	sche->Start();
	snd->Play();
	
	// FPS表示タイマ開始
	StartFPSTimer();
	
	return true;
}


////////////////////////////////////////////////////////////////
// 動作停止
////////////////////////////////////////////////////////////////
void EL6::Stop( void )
{
	// FPS表示タイマ停止
	StopFPSTimer();
	
	// 実行速度を退避
	Speed = sche->GetSpeedRatio();
	
	if( !this->cThread::IsCancel() ){
		this->cThread::Cancel();	// スレッド終了フラグ立てる
		this->cThread::Waiting();	// スレッド終了まで待つ
	}
	snd->Pause();
	sche->Stop();
}


////////////////////////////////////////////////////////////////
// イベントループ
////////////////////////////////////////////////////////////////
EL6::ReturnCode EL6::EventLoop( void )
{
	Event event;
	std::string str;
	
	// イベントキュークリア
	OSD_FlushEvents();
	
	// ResizeScreen()でリサイズしたかチェック 空読みしてリセット
	graph->CheckResize();
	
	while( OSD_GetEvent( &event ) ){
		switch( event.type ){
		case EV_FPSUPDATE:		// FPS表示
			str = cfg->GetCaption();
			if( sche->GetPauseEnable() )
				str += " === PAUSE ===";
			else{
				str += Stringf( " (%4d%%  %5.2f/%5.2f fps)", sche->GetRatio(), sche->GetFPS(), FRAMERATE );
				if( sche->GetSpeedRatio() != 100 )
					str += Stringf( " [x%3.1f]", (double)sche->GetSpeedRatio()/100 );
			}
			OSD_SetWindowCaption( GetWindowHandle(), str );
			
			// フルスクリーン時にマウスを一定時間動かさなかったらカーソルを消す
			if( cfg->GetFullScreen() ){
				if( MMotion ){ MMotion = false; }
				else         { OSD_ShowCursor( false ); }
			}
			break;
			
		case EV_KEYDOWN:		// キー入力
			#ifndef NOMONITOR	// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
			// モニタモード?
			if( vm->IsMonitor() ){
				CheckMonKey( event.key.sym, event.key.unicode, event.key.mod & KVM_SHIFT ? true : false );
				break;
			}
			#endif				// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
			// 各種機能キーチェック
			if( CheckFuncKey( event.key.sym, event.key.mod & KVM_ALT ? true : false ) )
				break;
			
			// リプレイ再生中 or 自動キー入力実行中でなければ
			if( REPLAY::GetStatus() != REP_REPLAY && !IsAutoKey() )
				// キーマトリクス更新(キー)
				vm->key->UpdateMatrixKey( event.key.sym, true );
			break;
			
		case EV_KEYUP:
			// リプレイ再生中 or 自動キー入力実行中でなければ
			if( REPLAY::GetStatus() != REP_REPLAY && !IsAutoKey() )
				// キーマトリクス更新(キー)
				vm->key->UpdateMatrixKey( event.key.sym, false );
			break;
			
		case EV_JOYAXISMOTION:
		case EV_JOYBUTTONDOWN:
		case EV_JOYBUTTONUP:
			// リプレイ再生中 or 自動キー入力実行中でなければ
			if( REPLAY::GetStatus() != REP_REPLAY && !IsAutoKey() )
				// キーマトリクス更新(ジョイスティック)
				vm->key->UpdateMatrixJoy( joy->GetJoyState( 0 ), joy->GetJoyState( 1 ) );
			break;
			
		#ifndef NOMONITOR	// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
		case EV_DEBUGMODEBP:		// モニタモード変更(ブレークポイント到達時)
			monw->BreakIn( event.bp.addr );		// ブレークポイントの情報を表示
			[[fallthrough]];
			
		case EV_DEBUGMODETOGGLE:	// モニタモード変更(モニタモードから通常モードへの復帰)
			ToggleMonitor();
			break;
			
		#endif				// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
		case EV_MOUSEMOTION:	// マウスカーソル動いた
			if( cfg->GetFullScreen() ){
				OSD_ShowCursor( true );
				MMotion = true;
			}
			break;
			
		case EV_MOUSEBUTTONUP:	// マウスボタンクリック(離した時)
			switch( event.mousebt.button ){
			case MBT_LEFT:		// 等速
				sche->SetSpeedRatio( 0 );
				break;
				
			case MBT_MIDDLE:
				break;
				
			case MBT_RIGHT:		// ポップアップメニュー表示
				Stop();
				ShowPopupMenu( event.mousebt.x, event.mousebt.y );
				Start();
				break;
				
			default:
				break;
			}
			break;
			
		case EV_MOUSEWHEEL:	// マウスホイール
			if( event.mousewh.y > 0 )	// スピードアップ
				sche->SetSpeedRatio( 1 );
			
			if( event.mousewh.y < 0 )	// スピードダウン
				sche->SetSpeedRatio( -1 );
			break;
			
		case EV_RENDER:				// 画面描画
			graph->DrawScreen();
			break;
			
		case EV_CAPTURE:			// ビデオキャプチャ
			graph->DrawScreen();
			AVI6::AVIWriteFrame( GetWindowHandle() );
			break;
			
//		case EV_WINDOWRESIZED:{		// ウィンドウサイズ変更
		case EV_WINDOWSIZECHANGED:{	// ウィンドウサイズ変更
			// ResizeScreen()でリサイズしたなら何もしないで戻る
			if( graph->CheckResize() ) break;
			
			int zoom = (int)((double)event.window.w / (double)GetVideoInfo().w * (double)((cfg->GetDispNTSC() ? GetVideoInfo().ratio : 100.0) + 0.5));
			
			Stop();
			cfg->SetWindowZoom( zoom );	// ウィンドウサイズに合わせて倍率再設定
			graph->ResizeScreen();		// スクリーンサイズ変更
			Start();
			break;
		}
		
		case EV_WINDOWEVENT_RESTORED:
			Stop();
			graph->ResizeScreen();		// スクリーンサイズ変更
			Start();
			break;
			
		case EV_QUIT:			// 終了
			if( cfg->GetCkQuit() )
				if( OSD_Message( GetWindowHandle(), GetText( T_QUIT ), GetText( T_QUITC ), OSDM_YESNO | OSDM_ICONQUESTION ) != OSDR_YES )
					break;
			return Quit;
			
		case EV_RESTART:		// 再起動
			return Restart;
			
		case EV_DOKOLOAD:		// どこでもLOAD
			return Dokoload;
			
		case EV_REPLAY:			// リプレイ再生
			return Replay;
			
		case EV_DROPFILE:{		// Drag & Drop
			P6VPATH tpath = P6VSTR2PATH( event.drop.file );
			// ファイル名を開放
			delete [] event.drop.file;
			
			// 拡張子取得(小文字)
			std::string ext = OSD_GetFileNameExt( tpath );
			std::transform( ext.begin(), ext.end(), ext.begin(), ::tolower );
			
			if( ext == EXT_P6RAW || ext == EXT_CAS || ext == EXT_P6T ){
				UI_TapeInsert( tpath );
			}else if( ext == EXT_DISK ){
				UI_DiskInsert( 0, tpath );
			}else if( ext == EXT_ROM1 || ext == EXT_ROM2 ){
				UI_RomInsert( tpath );
			}else if( ext == EXT_DOKO ){
				UI_DokoLoad( tpath );
			}else if( ext == EXT_REPLAY ){
				UI_ReplayLoad( tpath );
			}else if( ext == EXT_ATYPE1 || ext == EXT_ATYPE2 ){
				UI_AutoType( tpath );
			}
			
			break;
		}
		
		default:
			break;
		}
		
		// エラー処理
		switch( Error::GetError() ){
		case Error::NoError:
			break;
			
		default:
			OSD_Message( GetWindowHandle(), Error::GetErrorText(), GetText( TERR_ERROR ), OSDR_OK | OSDM_ICONERROR );
			Error::Reset();
		}
	}
	return Quit;
}


////////////////////////////////////////////////////////////////
// 各種機能キーチェック
////////////////////////////////////////////////////////////////
bool EL6::CheckFuncKey( int kcode, bool OnALT )
{
	switch( kcode ){	// キーコード
	case KVC_F6:		// モニタモード変更 or スクリーンモード変更
		// ビデオキャプチャ中は無効
		if( AVI6::IsAVI() ) return false;
		
		if( OnALT ){
			Stop();
			cfg->SetFullScreen( cfg->GetFullScreen() ? false : true );
			graph->ResizeScreen();	// スクリーンサイズ変更
			Start();
		}else{
			#ifndef NOMONITOR	// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
			ToggleMonitor();
			#endif				// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
		}
		break;
		
	case KVC_F7:			// スキャンラインモード変更
		// ビデオキャプチャ中は無効
		if( AVI6::IsAVI() ) return false;
		
		Stop();
		if( OnALT ){
			cfg->SetDispNTSC( cfg->GetDispNTSC() ? false : true );
			graph->ResizeScreen();	// スクリーンサイズ変更
		}else{
			cfg->SetScanLine( cfg->GetScanLine() ? false : true );
		}
		Start();
		break;
		
	case KVC_F8:			// モード4カラー変更 or ステータスバー表示状態変更
		if( OnALT ){
			Stop();
			cfg->SetDispStat( cfg->GetDispStat() ? false : true );
			graph->ResizeScreen();	// スクリーンサイズ変更
			Start();
		}else{
			int c = vm->vdg->GetMode4Color();
			if( ++c > 4 ) c = 0;
			vm->vdg->SetMode4Color( c );
		}
		break;
		
	case KVC_F9:			// ポーズ有効無効変更
		if( OnALT ){
		}else{
			sche->SetPauseEnable( sche->GetPauseEnable() ? false : true );
		}
		break;
		
	case KVC_F10:			// Wait有効無効変更
		if( OnALT ){
		}else{
			sche->SetWaitEnable( sche->GetWaitEnable() ? false : true );
		}
		break;
		
	case KVC_F11:			// リセット or 再起動
		if( OnALT ){
			OSD_PushEvent( EV_RESTART );
		}else{
			UI_Reset();
		}
		break;
		
	case KVC_F12:			// スナップショット
		if( OnALT ){
		}else{
			graph->SnapShot( cfg->GetImgPath() );
		}
		break;
		
	case KVX_MENU:			// ポップアップメニュー表示
		Stop();
		ShowPopupMenu( 0, 0 );
		Start();
		break;
		
/*
	case KVC_MUHENKAN:      // どこでもSAVE
		Stop();
		if( REPLAY::GetStatus() == REP_RECORD ){
			UI_ReplayDokoSave();
		}else{
			P6VPATH tpath = P6VSTR2PATH( Stringf( "%s/.1.dds", cfg->GetDokoSavePath() );
			DokoDemoSave( tpath );
			
			cIni save;
			if( save.Init( tpath ) ){
				// 一旦キー入力を無効化する(LOAD時にキーが押しっぱなしになるのを防ぐため)
				save.PutEntry( "KEY", "P6Matrix", "", "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF" );
				save.PutEntry( "KEY", "P6Mtrx",   "", "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF" );
			}
		}
		Start();
		break;
		
	case KVC_HENKAN:      // どこでもLOAD
		Stop();
		if( REPLAY::GetStatus() == REP_RECORD ){
			UI_ReplayDokoLoad();
		} else {
			P6VPATH tpath = P6VSTR2PATH( Stringf( "%s/.1.dds", cfg->GetDokoSavePath() ) );
			if( OSD_FileExist( tpath ) ){
				cfg->SetModel( GetDokoModel( tpath ) );
				cfg->SetDokoFile( tpath );
				OSD_PushEvent( EV_DOKOLOAD );
			}
		}
		Start();
		break;
*/
		
	default:				// どれでもない
		return false;
	}
	return true;
}


#ifndef NOMONITOR	// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
////////////////////////////////////////////////////////////////
// モニタモードキーチェック
////////////////////////////////////////////////////////////////
void EL6::CheckMonKey( int kcode, int ccode, bool OnSHIFT )
{
	switch( kcode ){		// キーコード
	case KVC_F6:			// モニタモード変更
		ToggleMonitor();
		break;
		
	// メモリウィンドウ
	case KVC_PAGEDOWN:		// PageDown
		memw->SetAddress( memw->GetAddress() + ( OnSHIFT ? 2048 : 16 ) );
		break;
		
	case KVC_PAGEUP:		// PageUp
		memw->SetAddress( memw->GetAddress() - ( OnSHIFT ? 2048 : 16 ) );
		break;
		
	default:
		monw->KeyIn( kcode, ccode );
	}
}
#endif				// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@


////////////////////////////////////////////////////////////////
// 画面更新
////////////////////////////////////////////////////////////////
bool EL6::ScreenUpdate( void )
{
	// 画面更新時期を迎えた?(ビデオキャプチャ中は無視)
	if( !AVI6::IsAVI() && !sche->IsScreenUpdate() ) return false;
	
	// フレームスキップチェック
	if( FSkipCount++ < cfg->GetFrameSkip() ) return false;
	
	
	// ここではバックバッファの更新のみ
	// 実際に画面に反映するには「メインスレッドから」graph->DrawScreen()を呼ぶ
	// (SDLの制約による。この手のフレームワークでは大体同じ制約があるらしい。)
	
	// バックバッファ更新
	vm->vdg->UpdateBackBuf();
	
	// ステータスバー更新
	staw->SetReplayStatus( REPLAY::GetStatus() );	// リプレイステータス
	staw->Update( vm );
	
	#ifndef NOMONITOR	// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	// モニタモード画面更新
	if( vm->IsMonitor() ){
		regw->Update();
		memw->Update();
		monw->Update();
	}
	#endif				// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	
	// FPSカウントアップ
	sche->FPSUpdate();
	
	FSkipCount = 0;
	
	return true;
}


////////////////////////////////////////////////////////////////
// サウンド更新
//
// 引数:	samples		更新するサンプル数(0:処理クロック分)
//			exbuf		外部バッファ
// 返値:	int			更新したサンプル数
////////////////////////////////////////////////////////////////
int EL6::SoundUpdate( int samples, cRing* exbuf )
{
	// PSG更新
	int size = vm->psg->SoundUpdate( samples );
	
	// CMT(LOAD)更新
	vm->cmtl->SoundUpdate( size );
	
	// 音声合成更新
	vm->voice->SoundUpdate( size );
	
	// サウンドバッファ更新
	int ret = snd->PreUpdate( size, exbuf );
	
	return ret;
}


////////////////////////////////////////////////////////////////
// ストリーム更新 コールバック関数
//
// 引数:	userdata	コールバック関数に渡すデータ(自分自身へのオブジェクトポインタ)
//			stream		ストリーム書込みバッファへのポインタ
//			len			バッファサイズ(バイト単位)
// 返値:	なし
////////////////////////////////////////////////////////////////
void EL6::StreamUpdate( void* userdata, BYTE* stream, int len )
{
	EL6* p6 = STATIC_CAST( EL6*, userdata );	// 自分自身のオブジェクトポインタ取得
	
	// サウンドバッファ更新
	//  もしサンプル数が足りなければここで追加
	//  ただしビデオキャプチャ中,ポーズ中,モニタモードの場合は無視
	int addsam = len/sizeof(int16_t) - p6->snd->cRing::ReadySize();
	
	if( addsam > 0 && !p6->AVI6::IsAVI() && !p6->sche->GetPauseEnable()
		#ifndef NOMONITOR	// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
		&& !p6->vm->IsMonitor()
		#endif				// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
																		){
		p6->SoundUpdate( addsam );
	}
	p6->snd->Update( stream, len/sizeof(int16_t) );
}


////////////////////////////////////////////////////////////////
// FPS表示タイマ コールバック関数
////////////////////////////////////////////////////////////////
DWORD EL6::UpDateFPS( DWORD interval, void* obj )
{
	// obj未使用
	
	OSD_PushEvent( EV_FPSUPDATE );
	
	return interval;
}


////////////////////////////////////////////////////////////////
// FPS表示タイマ開始
////////////////////////////////////////////////////////////////
bool EL6::StartFPSTimer( void )
{
	// タイマ稼動中なら一旦停止
	StopFPSTimer();
	
	// タイマ設定
	UpDateFPSID = OSD_AddTimer( 1000, EL6::UpDateFPS, nullptr );
	
	return UpDateFPSID ? true : false;
}


////////////////////////////////////////////////////////////////
// FPS表示タイマ停止
////////////////////////////////////////////////////////////////
void EL6::StopFPSTimer( void )
{
	if( UpDateFPSID ){
		OSD_DelTimer( UpDateFPSID );
		UpDateFPSID = 0;
	}
}


////////////////////////////////////////////////////////////////
// 自動キー入力実行中?
//
// 引数:	なし
// 返値:	bool	true:実行中 false:実行中でない
////////////////////////////////////////////////////////////////
bool EL6::IsAutoKey( void )
{
	return !ak.Buffer.empty();
}


////////////////////////////////////////////////////////////////
// 自動キー入力1文字取得
//   (VSYNC=1/60sec毎に呼ばれる)
//
// 引数:	なし
// 返値:	BYTE	P6のキーコード
////////////////////////////////////////////////////////////////
char EL6::GetAutoKey( void )
{
	// リレーON待ち
	if( ak.RelayOn ){
		if( vm->cmtl->IsRelay() ) ak.RelayOn = false;
		else                      return 0;
	}
	
	// リレーOFF待ち
	if( ak.Relay ){
		if( !vm->cmtl->IsRelay() ) ak.Relay = false;
		else                       return 0;
	}
	
	// 待ち?
	if( ak.Wait > 0 ){
		ak.Wait--;
		return 0;
	}
	
	
	// バッファが空なら終了
	if( ak.Buffer.empty() )
		return 0;
	
	// 次の文字を取得
	BYTE dat = ak.Buffer.front();
	ak.Buffer.erase( ak.Buffer.begin() );
	
	switch( dat ){
	case 0x17:	// '\w' ウェイト設定
		if( !ak.Buffer.empty() ){
			ak.Wait += (BYTE)ak.Buffer.front();	// 待ち回数設定
			ak.Buffer.erase( ak.Buffer.begin() );
		}
		return 0;
		
	case 0x0a:	// '\r' リレーOFF待ち
		ak.Relay   = true;
		ak.RelayOn = true;
		dat = 0x0d;
		[[fallthrough]];
		
	case 0x0d:	// '\n' 改行?
		ak.Wait = 9;	// 待ち9回(=150msec)
		break;
		
	default:	// 一般の文字
		ak.Wait = 0;	// 待ちなし
	}
	return dat;
}


////////////////////////////////////////////////////////////////
// 自動キー入力文字列設定
//
// 引数:	str		文字列への参照
// 返値:	bool	true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool EL6::SetAutoKey( const std::string& str )
{
	ak.Buffer.clear();
	
	ak.Buffer  = str;
	ak.Wait    = 60;	// 待ち回数カウンタ(初回は1sec)
	ak.Relay   = false;	// リレースイッチOFF待ちフラグ
	ak.RelayOn = false;	// リレースイッチON待ちフラグ
	
	return true;
}


////////////////////////////////////////////////////////////////
// 自動キー入力文字列設定(ファイルから)
//
// 引数:	filepath	入力ファイルパス
// 返値:	bool		true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool EL6::SetAutoKeyFile( const P6VPATH& filepath )
{
	std::fstream fs;
	char lbuf[1024];
	
	if( !OSD_FSopen( fs, filepath, std::ios_base::in ) ) return false;
	
	ak.Buffer.clear();
	
	// 文字列を読込み
	// データが無くなるまで繰り返し
	// 最初の1行読込む
	fs.getline( lbuf, sizeof(lbuf) );
	while( !fs.eof() ){
		Sjis2P6( ak.Buffer, lbuf );	// SJIS -> P6
		ak.Buffer += 0x0d;			// '\n'追加
		// 次の1行読込む
		fs.getline( lbuf, sizeof(lbuf) );
	}
	fs.close();
	
	ak.Wait    = 60;	// 待ち回数カウンタ(初回は1sec)
	ak.Relay   = false;	// リレースイッチOFF待ちフラグ
	ak.RelayOn = false;	// リレースイッチON待ちフラグ
	
	return true;
}


////////////////////////////////////////////////////////////////
// オートスタート文字列設定
//
// 引数:	なし
// 返値:	なし
////////////////////////////////////////////////////////////////
void EL6::SetAutoStart( void )
{
	std::string kbuf;
	
	if( !(vm->cmtl->IsMount() && vm->cmtl->IsAutoStart()) ) return;
	
	const P6TAUTOINFO& ainf = vm->cmtl->GetAutoStartInfo();
	
	// キーバッファに書込み
	switch( cfg->GetModel() ){
	case 60:	// PC-6001
	case 61:	// PC-6001A
		kbuf = Stringf( "%c%c", ainf.Page+'0', 0x0d );
		break;
		
	case 64:	// PC-6001mk2SR
		if( ainf.BASIC == 6 ){
			if( vm->disk->GetDrives() )	// ??? 実際は?
				kbuf = Stringf( "%c%c%c%c%c%c%c%c", 0x17, 50, ainf.BASIC+'0', 0x17, 20, 0x0d, 0x17, 10 );
			else
				kbuf = Stringf( "%c%c%c%c%c%c%c",   0x17, 10, ainf.BASIC+'0', 0x17, 20,       0x17, 10 );
			break;
		}
		[[fallthrough]];
		
	case 62:	// PC-6001mk2
		switch( ainf.BASIC ){
		case 3:
		case 4:
		case 5:
			if( vm->disk->GetDrives() )	// ??? 実際は?
				kbuf = Stringf( "%c%c%c%c%c%c%c%c%c%c", 0x17, 50, ainf.BASIC+'0', 0x17, 30, 0x0d, ainf.Page+'0', 0x0d, 0x17, 120 );
			else
				kbuf = Stringf( "%c%c%c%c%c%c%c%c%c",   0x17, 50, ainf.BASIC+'0', 0x17, 30,       ainf.Page+'0', 0x0d, 0x17, 120 );
			break;
		default:
			kbuf = Stringf( "%c%c%c%c%c%c%c%c%c", 0x17, 50, ainf.BASIC+'0', 0x17, 30, ainf.Page+'0', 0x0d, 0x17, 20 );
		}
		break;
		
	case 68:	// PC-6601SR
		if( ainf.BASIC == 6 ){
			if( vm->disk->IsMount( 0 ) )
				kbuf = Stringf( "%c%c%c%c%c%c%c%c%c%c%c", 0x17, 240, 0x17, 60, 0x14, 0xf4, 0x17, 30, 0x0d, 0x17, 10 );
			else
				kbuf = Stringf( "%c%c%c%c%c%c%c%c",   0x17, 240, 0x14, 0xf4, 0x17, 30,       0x17, 10 );
			break;
		}else{
			kbuf = Stringf( "%c%c%c%c%c%c%c%c", 0x17, 240, 0x17, vm->disk->IsMount( 0 ) ? 60 : 1, 0x17, vm->disk->IsMount( 1 ) ? 60 : 1, 0x14, 0xf3 );
		}
		[[fallthrough]];
		
	case 66:	// PC-6601
		switch( ainf.BASIC ){
		case 3:
		case 4:
		case 5:
			if( vm->disk->IsMount( 0 ) )
				kbuf += Stringf( "%c%c%c%c%c%c%c%c%c%c", 0x17, 50, ainf.BASIC+'0', 0x17, 30, 0x0d, ainf.Page+'0', 0x0d, 0x17, 110 );
			else
				kbuf += Stringf( "%c%c%c%c%c%c%c%c%c",   0x17, 50, ainf.BASIC+'0', 0x17, 30,       ainf.Page+'0', 0x0d, 0x17, 110 );
			break;
		default:
			kbuf += Stringf( "%c%c%c%c%c%c%c%c%c", 0x17, 50, ainf.BASIC+'0', 0x17, 30, ainf.Page+'0', 0x0d, 0x17, 10 );
		}
		break;
		
	}
	kbuf += ainf.ask.data();
	
	// 自動キー入力設定
	if( !kbuf.empty() ) SetAutoKey( kbuf );
}


////////////////////////////////////////////////////////////////
// パレット設定
//
// 引数:	なし
// 返値:	なし
////////////////////////////////////////////////////////////////
void EL6::SetPalette( void )
{
	if( !cfg ) return;
	
	for( int i=0; i<128; i++ )
		VSurface::SetColor( i, COL2DW( cfg->GetColor( i ) ) );
}


////////////////////////////////////////////////////////////////
// バックバッファ取得
//
// 引数:	なし
// 返値:	std::shared_ptr<VSurface>	バックバッファ
////////////////////////////////////////////////////////////////
std::shared_ptr<VSurface> EL6::GetBackBuffer( void )
{
	return vm->vdg;
}


////////////////////////////////////////////////////////////////
// 画面情報取得
//
// 引数:	なし
// 返値:	VDGInfo&	画面情報
////////////////////////////////////////////////////////////////
const VDGInfo& EL6::GetVideoInfo( void ) const
{
	return vm->vdg->GetVideoInfo();
}


////////////////////////////////////////////////////////////////
// ウィンドウハンドル取得
//
// 引数:	なし
// 返値:	HWINDOW		ウィンドウハンドル
////////////////////////////////////////////////////////////////
HWINDOW EL6::GetWindowHandle( void )
{
	return graph ? (HWINDOW)graph->GetWindowHandle() : nullptr;
}


////////////////////////////////////////////////////////////////
// どこでもSAVE
//
// 引数:	path		ファイルパスへの参照
// 返値:	bool		true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool EL6::DokoDemoSave( const P6VPATH& path )
{
	PRINTD( VM_LOG, "[EL6][DokoDemoSave]\n" );
	
	cIni ini;
	
	// エラーをリセット
	Error::Reset();
	try{
		std::fstream fs;
		
		if( !OSD_FSopen( fs, path, std::ios_base::out ) ) throw Error::DokoWriteFailed;
		
		// タイトル行を出力して一旦閉じる
		fs << GetText( TDOK_TITLE ) << std::endl;
		fs.close();
		
		// どこでもSAVEファイルを開く
		if( !ini.Read( path ) ) throw Error::DokoWriteFailed;
		
		// 各オブジェクトのパラメータ書込み
		if( !cfg->DokoSave( &ini )      ||
			!vm->evsc->DokoSave( &ini ) ||
			!vm->intr->DokoSave( &ini ) ||
			!vm->cpum->DokoSave( &ini ) ||
			!vm->cpus->DokoSave( &ini ) ||
			!vm->mem->DokoSave( &ini )  ||
			!vm->vdg->DokoSave( &ini )  ||
			!vm->psg->DokoSave( &ini )  ||
			!vm->pio->DokoSave( &ini )  ||
			!vm->key->DokoSave( &ini )  ||
			!vm->cmtl->DokoSave( &ini ) ||
			!vm->disk->DokoSave( &ini ) ||
			!vm->voice->DokoSave( &ini )
		) throw Error::GetError();
		
		ini.PutEntry( "KEY", "AK_Wait",		"", "%d",	ak.Wait );
		ini.PutEntry( "KEY", "AK_Relay",	"", "%s",	ak.Relay   ? "Yes" : "No" );
		ini.PutEntry( "KEY", "AK_RelayOn",	"", "%s",	ak.RelayOn ? "Yes" : "No" );
		
		std::string strva;
		int nn=0;
		
		for( size_t i=0; i<ak.Buffer.length(); i++ ){
			strva += Stringf( "%02X", ak.Buffer[i] );
			if( !((i+1)&63) ){
				ini.PutEntry( "KEY", Stringf( "AKBuf_%02X", nn++ ), "", "%s", strva.c_str() );
				strva.clear();
			}
		};
		if( !ak.Buffer.empty() )
			ini.PutEntry( "KEY", Stringf( "AKBuf_%02X", nn ), "", "%s", strva.c_str() );
		
		ini.Write();
	}
	catch( Error::Errno i ){	// 例外発生
		Error::SetError( i );
		return false;
	}
	
	return true;
}


////////////////////////////////////////////////////////////////
// どこでもLOAD
//
// 引数:	path		ファイルパスへの参照
// 返値:	bool		true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool EL6::DokoDemoLoad( const P6VPATH& path )
{
	PRINTD( VM_LOG, "[EL6][DokoDemoLoad]\n" );
	
	cIni ini;
	
	// エラーをリセット
	Error::Reset();
	try{
		// どこでもLOADファイルを開く
		if( !ini.Read( path ) ) throw Error::DokoReadFailed;
		
		// PC6001Vのバージョン確認と主要構成情報を読込み
		// (機種,FDD台数,拡張RAM,戦士のカートリッジ)
		if( !cfg->DokoLoad( &ini ) ) throw Error::GetError();
		
		// VM再初期化
		Init( cfg );
		
		// 各オブジェクトのパラメータ読込み
		if(	!vm->evsc->DokoLoad( &ini ) ||
			!vm->intr->DokoLoad( &ini ) ||
			!vm->cpum->DokoLoad( &ini ) ||
			!vm->cpus->DokoLoad( &ini ) ||
			!vm->mem->DokoLoad( &ini )  ||
			!vm->vdg->DokoLoad( &ini )  ||
			!vm->psg->DokoLoad( &ini )  ||
			!vm->pio->DokoLoad( &ini )  ||
			!vm->key->DokoLoad( &ini )  ||
			!vm->cmtl->DokoLoad( &ini ) ||
			!vm->disk->DokoLoad( &ini ) ||
			!vm->voice->DokoLoad( &ini )
		) throw Error::GetError();
		
		ini.GetInt(   "KEY", "AK_Wait",		&ak.Wait,		ak.Wait );
		ini.GetTruth( "KEY", "AK_Relay",	&ak.Relay,		ak.Relay );
		ini.GetTruth( "KEY", "AK_RelayOn",	&ak.RelayOn,	ak.RelayOn );
		
		std::string strva;
		int nn=0;
		
		ak.Buffer.clear();
		while( ini.GetString( "KEY", Stringf( "AKBuf_%02X", nn++ ), strva, "" ) ){
			while( strva.length() >= 2 ){
				ak.Buffer += std::stoul( strva.substr( 0, 2 ), nullptr, 16 );
				strva.erase( strva.begin() );
				strva.erase( strva.begin() );
			}
		}
		
		// ディスクドライブ数によってスクリーンサイズ変更
		if( !staw->Init( -1, vm->disk->GetDrives() ) ) throw Error::GetError();
		if( !graph->ResizeScreen() ) throw Error::GetError();
	}
	catch( Error::Errno i ){	// 例外発生
		Error::SetError( i );
		return false;
	}
	
	return true;
}


////////////////////////////////////////////////////////////////
// どこでもLOADファイルから機種名読込
//
// 引数:	path		ファイルパスへの参照
// 返値:	int			機種名(60,61,62,66)
////////////////////////////////////////////////////////////////
int EL6::GetDokoModel( const P6VPATH& path )
{
	cIni ini;
	int st;
	
	try{
		// どこでもLOADファイルを開く
		if( !ini.Read( path ) ) throw Error::DokoReadFailed;
		
		// 機種取得
		ini.GetInt( "GLOBAL", "Model",	&st, 0 );
	}
	catch( Error::Errno i ){	// 例外発生
		Error::SetError( i );
		return 0;
	}
	
	return st;
}


////////////////////////////////////////////////////////////////
// TAPE マウント
//
// 引数:	path		ファイルパス
// 返値:	bool		true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool EL6::TapeMount( const P6VPATH& path )
{
	if( !vm->cmtl->Mount( path ) ) return false;
	
	vm->cmtl->SetStopBit( cfg->GetStopBit() );		// ストップビット数
	return true;
}


////////////////////////////////////////////////////////////////
// TAPE アンマウント
//
// 引数:	なし
// 返値:	なし
////////////////////////////////////////////////////////////////
void EL6::TapeUnmount( void )
{
	vm->cmtl->Unmount();
}


////////////////////////////////////////////////////////////////
// DISK マウント
//
// 引数:	drv			ドライブ番号
//			path		ファイルパスへの参照
// 返値:	bool		true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool EL6::DiskMount( int drv, const P6VPATH& path )
{
	if( !vm->disk->Mount( drv, path ) ) return false;
	return true;
}


////////////////////////////////////////////////////////////////
// DISK アンマウント
//
// 引数:	drv			ドライブ番号
// 返値:	なし
////////////////////////////////////////////////////////////////
void EL6::DiskUnmount( int drv )
{
	vm->disk->Unmount( drv );
}






////////////////////////////////////////////////////////////////
// リプレイ保存開始
//
// 引数:	path		ファイルパス
// 返値:	bool		true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool EL6::ReplayRecStart( const P6VPATH& path )
{
	return REPLAY::StartRecord( path );
}


////////////////////////////////////////////////////////////////
// リプレイ保存再開
//
// 引数:	path		ファイルパスへの参照
// 返値:	bool		true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool EL6::ReplayRecResume( const P6VPATH& path )
{
	// 途中セーブファイルを探す
	P6VPATH tpath = path;
	OSD_ChangeFileNameExt( tpath, EXT_RES );	// 拡張子を差替え
	
	if( OSD_FileExist( tpath ) ){
		cIni save;
		save.Read( tpath );
		int frame = 0;
		save.GetInt( "REPLAY", "frame", &frame, frame );
		if( frame == 0 ) return false;
		
		DokoDemoLoad( tpath );
		return REPLAY::ResumeRecord( path, frame );
	}
	return false;
}


////////////////////////////////////////////////////////////////
// リプレイ中どこでもLOAD
//
// 引数:	なし
// 返値:	bool		true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool EL6::ReplayRecDokoLoad( void )
{
	if( REPLAY::GetStatus() == REP_RECORD ){
		P6VPATH tpath = REPLAY::cIni::GetFilePath();
		REPLAY::StopRecord();
		return ReplayRecResume( tpath );
	}else{
		return false;
	}
}


////////////////////////////////////////////////////////////////
// リプレイ中どこでもSAVE
//
// 引数:	なし
// 返値:	bool		true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool EL6::ReplayRecDokoSave( void )
{
	if( REPLAY::GetStatus() == REP_RECORD ){
		// 途中セーブファイルを保存
		P6VPATH tpath = REPLAY::cIni::GetFilePath();
		OSD_ChangeFileNameExt( tpath, EXT_RES );	// 拡張子を差替え
		if( !DokoDemoSave( tpath ) ) return false;
		
		// 途中セーブ情報を追記
		cIni save;
		if( !save.Read( tpath ) ) return false;
		save.PutEntry( "REPLAY", "frame", "", "%d", REPLAY::RepFrm );
		// 一旦キー入力を無効化する(LOAD時にキーが押しっぱなしになるのを防ぐため)
		save.PutEntry( "KEY", "P6Matrix", "", "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF" );
		save.PutEntry( "KEY", "P6Mtrx",   "", "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF" );
		
		save.Write();
		
		return true;
	}else{
		return false;
	}
}


////////////////////////////////////////////////////////////////
// リプレイ保存停止
//
// 引数:	なし
// 返値:	なし
////////////////////////////////////////////////////////////////
void EL6::ReplayRecStop( void )
{
	ReplayRecDokoSave();
	REPLAY::StopRecord();
}


////////////////////////////////////////////////////////////////
// リプレイ再生開始
//
// 引数:	path		ファイルパスへの参照
// 返値:	なし
////////////////////////////////////////////////////////////////
void EL6::ReplayPlayStart( const P6VPATH& path )
{
	cfg->SetModel( GetDokoModel( path ) );
	cfg->SetDokoFile( path );
	OSD_PushEvent( EV_REPLAY );
}


////////////////////////////////////////////////////////////////
// リプレイ再生停止
//
// 引数:	なし
// 返値:	なし
////////////////////////////////////////////////////////////////
void EL6::ReplayPlayStop( void )
{
	REPLAY::StopReplay();
}







////////////////////////////////////////////////////////////////
// UI:TAPE 挿入
//
// 引数:	path		ファイルパスへの参照
// 返値:	なし
////////////////////////////////////////////////////////////////
void EL6::UI_TapeInsert( const P6VPATH& path )
{
	P6VPATH fpath = path;
	
	if( fpath.empty() ){
		if( !OSD_FileExist( TapePathUI ) )
			TapePathUI = cfg->GetTapePath();
		OSD_FileSelect( GetWindowHandle(), FD_TapeLoad, fpath, TapePathUI );
	}
	if( fpath.empty() ) return;
	
	if( !TapeMount( fpath ) ) Error::SetError( Error::TapeMountFailed );
}


////////////////////////////////////////////////////////////////
// UI:DISK 挿入
//
// 引数:	drv			ドライブ番号
//			path		ファイルパスへの参照
// 返値:	なし
////////////////////////////////////////////////////////////////
void EL6::UI_DiskInsert( int drv, const P6VPATH& path )
{
	P6VPATH fpath = path;
	
	if( fpath.empty() ){
		if( !OSD_FileExist( DiskPathUI ) )
			DiskPathUI = cfg->GetDiskPath();
		OSD_FileSelect( GetWindowHandle(), FD_Disk, fpath, DiskPathUI );
	}
	if( fpath.empty() ) return;
	
	if( !DiskMount( drv, fpath ) ) Error::SetError( Error::DiskMountFailed );
}


////////////////////////////////////////////////////////////////
// UI:拡張ROM 挿入
//
// 引数:	path		ファイルパスへの参照
// 返値:	なし
////////////////////////////////////////////////////////////////
void EL6::UI_RomInsert( const P6VPATH& path )
{
	P6VPATH fpath = path;
	
	if( fpath.empty() ){
		if( !OSD_FileExist( ExRomPathUI ) )
			ExRomPathUI = cfg->GetExtRomPath();
		OSD_FileSelect( GetWindowHandle(), FD_ExtRom, fpath, ExRomPathUI );
	}
	if( fpath.empty() ) return;
	
	// リセットを伴うのでメッセージ表示
	OSD_Message( GetWindowHandle(), GetText( T_RESETI ), GetText( T_RESETC ), OSDM_OK | OSDM_ICONINFO );
	if( !vm->mem->MountExtRom( fpath ) )
		Error::SetError( Error::ExtRomMountFailed );
	else
		UI_Reset();
}


////////////////////////////////////////////////////////////////
// UI:拡張ROM 排出
//
// 引数:	なし
// 返値:	なし
////////////////////////////////////////////////////////////////
void EL6::UI_RomEject( void )
{
	// リセットを伴うのでメッセージ表示
	OSD_Message( GetWindowHandle(), GetText( T_RESETE ), GetText( T_RESETC ), OSDM_OK | OSDM_ICONINFO );
	vm->mem->UnmountExtRom();
	UI_Reset();
}


////////////////////////////////////////////////////////////////
// UI:どこでもSAVE
//
// 引数:	path		ファイルパスへの参照
// 返値:	なし
////////////////////////////////////////////////////////////////
void EL6::UI_DokoSave( const P6VPATH& path )
{
	P6VPATH fpath = path;
	
	if( fpath.empty() ){
		if( !OSD_FileExist( DokoPathUI ) )
			DokoPathUI = cfg->GetDokoSavePath();
		OSD_FileSelect( GetWindowHandle(), FD_DokoSave, fpath, DokoPathUI );
	}
	if( fpath.empty() ) return;
	
	if( !DokoDemoSave( fpath ) ) Error::SetError( Error::DokoWriteFailed );
}


////////////////////////////////////////////////////////////////
// UI:どこでもLOAD
//
// 引数:	path		ファイルパスへの参照
// 返値:	なし
////////////////////////////////////////////////////////////////
void EL6::UI_DokoLoad( const P6VPATH& path )
{
	P6VPATH fpath = path;
	
	if( fpath.empty() ){
		if( !OSD_FileExist( DokoPathUI ) )
			DokoPathUI = cfg->GetDokoSavePath();
		OSD_FileSelect( GetWindowHandle(), FD_DokoLoad, fpath, DokoPathUI );
	}
	if( fpath.empty() ) return;
	
	cfg->SetModel( GetDokoModel( fpath ) );
	cfg->SetDokoFile( fpath );
	OSD_PushEvent( EV_DOKOLOAD );
}


////////////////////////////////////////////////////////////////
// UI:リプレイ保存
//
// 引数:	path		ファイルパスへの参照
// 返値:	なし
////////////////////////////////////////////////////////////////
void EL6::UI_ReplaySave( const P6VPATH& path )
{
	P6VPATH fpath = path;
	
	if( REPLAY::GetStatus() == REP_IDLE ){
		if( fpath.empty() ){
			if( !OSD_FileExist( DokoPathUI ) )
				DokoPathUI = cfg->GetDokoSavePath();
			OSD_FileSelect( GetWindowHandle(), FD_RepSave, fpath, DokoPathUI );
		}
		if( fpath.empty() ) return;
		
		if( !DokoDemoSave( fpath ) || !ReplayRecStart( fpath ) )
			Error::SetError( Error::ReplayRecError );
		
	}else if( REPLAY::GetStatus() == REP_RECORD ){
		ReplayRecStop();
	}
}


////////////////////////////////////////////////////////////////
// UI:リプレイ再開
//
// 引数:	path		ファイルパスへの参照
// 返値:	なし
////////////////////////////////////////////////////////////////
void EL6::UI_ReplayResumeSave( const P6VPATH& path )
{
	P6VPATH fpath = path;
	
	if( REPLAY::GetStatus() == REP_IDLE ){
		if( fpath.empty() ){
			if( !OSD_FileExist( DokoPathUI ) )
				DokoPathUI = cfg->GetDokoSavePath();
			OSD_FileSelect( GetWindowHandle(), FD_RepSave, fpath, DokoPathUI );
		}
		if( fpath.empty() ) return;
		
		if( !ReplayRecResume( fpath ) ) Error::SetError( Error::ReplayRecError );
	}
}


////////////////////////////////////////////////////////////////
// UI:リプレイ中どこでもLOAD
//
// 引数:	なし
// 返値:	なし
////////////////////////////////////////////////////////////////
void EL6::UI_ReplayDokoLoad()
{
	ReplayRecDokoLoad();
}


////////////////////////////////////////////////////////////////
// UI:リプレイ中どこでもSAVE
//
// 引数:	なし
// 返値:	なし
////////////////////////////////////////////////////////////////
void EL6::UI_ReplayDokoSave()
{
	ReplayRecDokoSave();
}


////////////////////////////////////////////////////////////////
// UI:リプレイ再生
//
// 引数:	path		ファイルパスへの参照
// 返値:	なし
////////////////////////////////////////////////////////////////
void EL6::UI_ReplayLoad( const P6VPATH& path )
{
	P6VPATH fpath = path;
	
	if( REPLAY::GetStatus() == REP_IDLE ){
		if( fpath.empty() ){
			if( !OSD_FileExist( DokoPathUI ) )
				DokoPathUI = cfg->GetDokoSavePath();
			OSD_FileSelect( GetWindowHandle(), FD_RepLoad, fpath, DokoPathUI );
		}
	}else if( REPLAY::GetStatus() == REP_REPLAY ){
		ReplayPlayStop();
	}
	
	if( fpath.empty() ) return;
	
	ReplayPlayStart( fpath );
}


////////////////////////////////////////////////////////////////
// UI:ビデオキャプチャ
//
// 引数:	なし
// 返値:	なし
////////////////////////////////////////////////////////////////
void EL6::UI_AVISave( void )
{
	P6VPATH fpath;
	P6VPATH mpath = OSD_GetConfigPath();
	
	if( !AVI6::IsAVI() ){
		if( OSD_FileSelect( GetWindowHandle(), FD_AVISave, fpath, mpath ) ){
			AVI6::StartAVI( fpath, graph->ScreenX(), graph->ScreenY(), FRAMERATE, cfg->GetSampleRate(), cfg->GetAviBpp() );
		}
	}else{
		AVI6::StopAVI();
	}
}


////////////////////////////////////////////////////////////////
// UI:打込み代行
//
// 引数:	path		ファイルパスへの参照
// 返値:	なし
////////////////////////////////////////////////////////////////
void EL6::UI_AutoType( const P6VPATH& path )
{
	P6VPATH fpath = path;
	P6VPATH mpath = OSD_GetConfigPath();
	
	if( fpath.empty() ){
		OSD_FileSelect( GetWindowHandle(), FD_LoadAll, fpath, mpath );
	}
	if( fpath.empty() ) return;
	
	if( !SetAutoKeyFile( fpath ) ) Error::SetError( Error::Unknown );
}


////////////////////////////////////////////////////////////////
// UI:リセット
//
// 引数:	なし
// 返値:	なし
////////////////////////////////////////////////////////////////
void EL6::UI_Reset( void )
{
	bool can = this->cThread::IsCancel();	// スレッド停止済み?
	
	if( !can ) Stop();	// スレッド動いてたら一旦止める
	
	// システムディスクが入っていたらTAPEのオートスタート無効
	if( !vm->disk->IsSystem(0) && !vm->disk->IsSystem(1) )
		SetAutoStart();
	
	vm->Reset();
	
	if( !can ) Start();	// 元々スレッドが動いていたら再始動
}


////////////////////////////////////////////////////////////////
// UI:再起動
//
// 引数:	なし
// 返値:	なし
////////////////////////////////////////////////////////////////
void EL6::UI_Restart( void )
{
	OSD_PushEvent( EV_RESTART );
}


////////////////////////////////////////////////////////////////
// UI:終了
//
// 引数:	なし
// 返値:	なし
////////////////////////////////////////////////////////////////
void EL6::UI_Quit( void )
{
	OSD_PushEvent( EV_QUIT );
}


////////////////////////////////////////////////////////////////
// UI:Wait変更
//
// 引数:	なし
// 返値:	なし
////////////////////////////////////////////////////////////////
void EL6::UI_NoWait( void )
{
	sche->SetWaitEnable( sche->GetWaitEnable() ? false : true );
}


////////////////////////////////////////////////////////////////
// UI:Turbo TAPE変更
//
// 引数:	なし
// 返値:	なし
////////////////////////////////////////////////////////////////
void EL6::UI_TurboTape( void )
{
	cfg->SetTurboTAPE( cfg->GetTurboTAPE() ? false : true );
}


////////////////////////////////////////////////////////////////
// UI:Boost Up変更
//
// 引数:	なし
// 返値:	なし
////////////////////////////////////////////////////////////////
void EL6::UI_BoostUp( void )
{
	cfg->SetBoostUp( cfg->GetBoostUp() ? false : true );
	vm->cmtl->SetBoost( vm->cmtl->IsBoostUp() ? false : true );
}


////////////////////////////////////////////////////////////////
// UI:フルスクリーン変更
//
// 引数:	なし
// 返値:	なし
////////////////////////////////////////////////////////////////
void EL6::UI_FullScreen( void )
{
	// ビデオキャプチャ中は無効
	if( !AVI6::IsAVI() ){
		cfg->SetFullScreen( cfg->GetFullScreen() ? false : true );
		graph->ResizeScreen();	// スクリーンサイズ変更
	}
}


////////////////////////////////////////////////////////////////
// UI:ウィンドウ表示倍率変更
//
// 引数:	zoom		表示倍率(%)
// 返値:	なし
////////////////////////////////////////////////////////////////
void EL6::UI_WindowZoom( int zoom )
{
	// ビデオキャプチャ中は無効
	if( !AVI6::IsAVI() ){
		cfg->SetWindowZoom( zoom );
		graph->ResizeScreen();	// スクリーンサイズ変更
	}
}


////////////////////////////////////////////////////////////////
// UI:ステータスバー表示状態変更
//
// 引数:	なし
// 返値:	なし
////////////////////////////////////////////////////////////////
void EL6::UI_StatusBar( void )
{
	cfg->SetDispStat( cfg->GetDispStat() ? false : true );
	graph->ResizeScreen();	// スクリーンサイズ変更
}


////////////////////////////////////////////////////////////////
// UI:4:3表示変更
//
// 引数:	なし
// 返値:	なし
////////////////////////////////////////////////////////////////
void EL6::UI_Disp43( void )
{
	// ビデオキャプチャ中は無効
	if( !AVI6::IsAVI() ){
		cfg->SetDispNTSC( cfg->GetDispNTSC() ? false : true );
		graph->ResizeScreen();	// スクリーンサイズ変更
	}
}


////////////////////////////////////////////////////////////////
// UI:スキャンラインモード変更
//
// 引数:	なし
// 返値:	なし
////////////////////////////////////////////////////////////////
void EL6::UI_ScanLine( void )
{
	// ビデオキャプチャ中は無効
	if( !AVI6::IsAVI() ){
		cfg->SetScanLine( cfg->GetScanLine() ? false : true );
	}
}


////////////////////////////////////////////////////////////////
// UI:フィルタリング変更
//
// 引数:	なし
// 返値:	なし
////////////////////////////////////////////////////////////////
void EL6::UI_Filtering( void )
{
	// ビデオキャプチャ中は無効
	if( !AVI6::IsAVI() ){
		cfg->SetFiltering( cfg->GetFiltering() ? false : true );
		graph->ResizeScreen();	// スクリーンサイズ変更
	}
}


////////////////////////////////////////////////////////////////
// UI:MODE4カラー変更
//
// 引数:	col			0:モノクロ 1:赤/青 2:青/赤 3:ピンク/緑 4:緑/ピンク
// 返値:	なし
////////////////////////////////////////////////////////////////
void EL6::UI_Mode4Color( int col )
{
	cfg->SetMode4Color( col );
	vm->vdg->SetMode4Color( col );
}


////////////////////////////////////////////////////////////////
// UI:フレームスキップ変更
//
// 引数:	sk			フレームスキップ数
// 返値:	なし
////////////////////////////////////////////////////////////////
void EL6::UI_FrameSkip( int sk )
{
	if( !AVI6::IsAVI() ) cfg->SetFrameSkip( sk );
}


////////////////////////////////////////////////////////////////
// UI:サンプリングレート変更
//
// 引数:	rate		サンプリングレート(Hz)
// 返値:	なし
////////////////////////////////////////////////////////////////
void EL6::UI_SampleRate( int rate )
{
	cfg->SetSampleRate( rate );
	snd->SetSampleRate( rate );
}


////////////////////////////////////////////////////////////////
// UI:環境設定
//
// 引数:	なし
// 返値:	なし
////////////////////////////////////////////////////////////////
void EL6::UI_Config( void )
{
	cfg->Write();
	if( OSD_ConfigDialog( GetWindowHandle() ) > 0 ){
		CFG6 ccfg;
		
		ccfg.Init();	// 変更したINIを読込み(比較用)
		bool reb =  cfg->GetModel()       != ccfg.GetModel()       ||	// 機種取得
					cfg->GetFddNum()      != ccfg.GetFddNum()      ||	// FDD接続台数取得
					cfg->GetUseExtRam()   != ccfg.GetUseExtRam()   ||	// 拡張RAMを使う取得
					cfg->GetOverClock()   != ccfg.GetOverClock()   ||	// オーバークロック率
					cfg->GetUseSoldier()  != ccfg.GetUseSoldier()  ||	// 戦士のカートリッジ使うフラグ取得
					cfg->GetExtRomFile()  != ccfg.GetExtRomFile()  ||	// 拡張ROMファイル名取得
					cfg->GetTapeFile()    != ccfg.GetTapeFile()    ||	// TAPE(LOAD)ファイル名
					cfg->GetDiskFile( 1 ) != ccfg.GetDiskFile( 1 ) ||	// DISK1ファイル名
					cfg->GetDiskFile( 2 ) != ccfg.GetDiskFile( 2 );		// DISK2ファイル名
		
		cfg->Init();	// 変更したINIを読込み(オリジナル)
		
		// 再起動?
		if( reb && OSD_Message( GetWindowHandle(), GetText( T_RESTART ), GetText( T_RESTARTC ), OSDM_YESNO | OSDM_ICONQUESTION ) == OSDR_YES ){
			OSD_PushEvent( EV_RESTART );
			return;
		}
		
		// 設定反映
		// [CONFIG] ----------------------------------------------------
		vm->cmtl->SetBoost( cfg->GetBoostUp() );					// BoostUp 有効フラグ
		vm->cmtl->SetMaxBoost( cfg->GetMaxBoost1(), cfg->GetMaxBoost2() );	// BoostUp 最大倍率
		vm->disk->WaitEnable( cfg->GetFddWaitEnable() );			// FDDウェイト有効フラグ
		vm->cmtl->SetStopBit( cfg->GetStopBit() );					// ストップビット数
		
		// [DISPLAY] ---------------------------------------------------
		vm->vdg->SetMode4Color( cfg->GetMode4Color() );				// モード4カラーモード
		
		// [SOUND] -----------------------------------------------------
		snd->SetSampleRate( cfg->GetSampleRate(), cfg->GetSoundBuffer() );	// サンプリングレート, サウンドバッファ長倍率
		snd->SetVolume( cfg->GetMasterVol() );						// マスター音量
		vm->psg->SetVolume( cfg->GetPsgVol() );						// PSG/OPN音量
		vm->psg->SetLPF( cfg->GetPsgLPF() );						// PSG/OPN LPFカットオフ周波数
		vm->voice->SetVolume( cfg->GetVoiceVol() );					// 音声合成音量
		vm->cmtl->SetVolume( cfg->GetCmtVol() );					// TAPEモニタ音量取得
		vm->cmtl->SetLPF( cfg->GetCmtLPF() );						// TAPE LPFカットオフ周波数取得
		
		// [FILES] -----------------------------------------------------
		vm->pio->cPRT::SetFile( cfg->GetPrinterFile() );			// プリンタファイル名取得
		
		// [PATH] ------------------------------------------------------
		vm->voice->SetPath( cfg->GetWavePath() );					// WAVEパス取得
		
		// [KEY] -------------------------------------------------------
		OSD_SetKeyRepeat( cfg->GetKeyRepeat() );					// キーリピート
		
		// [COLOR] -----------------------------------------------------
		SetPalette();												// パレット設定
		
		graph->ResizeScreen();	// スクリーンサイズ変更
	}
}


////////////////////////////////////////////////////////////////
// メニュー選択項目実行
//
// 引数:	id		選択したメニューID
// 返値:	なし
////////////////////////////////////////////////////////////////
void EL6::ExecMenu( int id )
{
	// 項目ごとの処理
	switch( id ){
	case ID_TAPEINSERT:		UI_TapeInsert();						break;	// TAPE 挿入
	case ID_TAPEEJECT:		TapeUnmount();							break;	// TAPE 排出
	case ID_DISKINSERT1:													// DISK 挿入
	case ID_DISKINSERT2:	UI_DiskInsert( id - ID_DISKINSERT1 );	break;
	case ID_DISKEJECT1:														// DISK 排出
	case ID_DISKEJECT2:		DiskUnmount( id - ID_DISKEJECT1 );		break;
	case ID_ROMINSERT:		UI_RomInsert();							break;	// 拡張ROM 挿入
	case ID_ROMEJECT:		UI_RomEject();							break;	// 拡張ROM 排出
	case ID_JOY100:															// ジョイスティック1
	case ID_JOY101:
	case ID_JOY102:
	case ID_JOY103:
	case ID_JOY104:
	case ID_JOY105:			joy->Connect( 0, id - ID_JOY101 );		break;
	case ID_JOY200:															// ジョイスティック2
	case ID_JOY201:
	case ID_JOY202:
	case ID_JOY203:
	case ID_JOY204:
	case ID_JOY205:			joy->Connect( 1, id - ID_JOY201 );		break;
	case ID_CONFIG:			UI_Config();							break;	// 環境設定
	case ID_RESET:			UI_Reset();								break;	// リセット
	case ID_RESTART:		UI_Restart();							break;	// 再起動
	case ID_DOKOSAVE:		UI_DokoSave();							break;	// どこでもSAVE
	case ID_DOKOLOAD:		UI_DokoLoad();							break;	// どこでもLOAD
	case ID_REPLAYSAVE:		UI_ReplaySave();						break;	// リプレイ保存
	case ID_REPLAYRESUME:	UI_ReplayResumeSave();					break;	// リプレイ保存再開
	case ID_REPLAYDOKOLOAD:	UI_ReplayDokoLoad();					break;	// リプレイ中どこでもLOAD
	case ID_REPLAYDOKOSAVE:	UI_ReplayDokoSave();					break;	// リプレイ中どこでもSAVE
	case ID_REPLAYLOAD:		UI_ReplayLoad();						break;	// リプレイ再生
	case ID_AVISAVE:		UI_AVISave();							break;	// ビデオキャプチャ
	case ID_AUTOTYPE:		UI_AutoType();							break;	// 打込み代行
	case ID_QUIT:			UI_Quit();								break;	// 終了
	case ID_NOWAIT:			UI_NoWait();							break;	// Wait有効無効変更
	case ID_TURBO:			UI_TurboTape();							break;	// Turbo TAPE
	case ID_BOOST:			UI_BoostUp();							break;	// Boost Up
	case ID_FULLSCRN:		UI_FullScreen();						break;	// フルスクリーン変更
	case ID_ZOOM100:														// ウィンドウ表示倍率100%
	case ID_ZOOM200:														// ウィンドウ表示倍率200%
	case ID_ZOOM300:		UI_WindowZoom( (id-ID_ZOOM100+1)*100 );	break;	// ウィンドウ表示倍率300%
	case ID_STATUS:			UI_StatusBar();							break;	// ステータスバー表示状態変更
	case ID_DISP43:			UI_Disp43();							break;	// 4:3表示変更
	case ID_SCANLINE:		UI_ScanLine();							break;	// スキャンラインモード変更
	case ID_FILTERING:		UI_Filtering();							break;	// フィルタリング変更
	case ID_M4MONO:															// MODE4カラー モノクロ
	case ID_M4RDBL:															// MODE4カラー 赤/青
	case ID_M4BLRD:															// MODE4カラー 青/赤
	case ID_M4PKGR:															// MODE4カラー ピンク/緑
	case ID_M4GRPK:			UI_Mode4Color( id - ID_M4MONO );		break;	// MODE4カラー 緑/ピンク
	case ID_FSKP0:															// フレームスキップ なし
	case ID_FSKP1:															// フレームスキップ 1
	case ID_FSKP2:															// フレームスキップ 2
	case ID_FSKP3:															// フレームスキップ 3
	case ID_FSKP4:															// フレームスキップ 4
	case ID_FSKP5:			UI_FrameSkip( id - ID_FSKP0 );			break;	// フレームスキップ 5
	case ID_SPR44:															// サンプリングレート 44100Hz
	case ID_SPR22:															// サンプリングレート 22050Hz
	case ID_SPR11:			UI_SampleRate( 44100 >> (id - ID_SPR44 ) );	break;	// サンプリングレート 11025Hz
	case ID_VERSION:		OSD_VersionDialog( GetWindowHandle(), cfg->GetModel() );	break;	// バージョン情報
	#ifndef NOMONITOR	// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	case ID_MONITOR:		ToggleMonitor();						break;	// モニターモード
	#endif				// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	}
}
