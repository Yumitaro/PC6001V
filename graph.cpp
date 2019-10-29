#include "pc6001v.h"

#include "common.h"
#include "config.h"
#include "debug.h"
#include "graph.h"
#include "log.h"
#include "osd.h"
#include "p6el.h"
#include "status.h"
#include "vdg.h"


// スクリーン表示倍率(%)
#define	WSCALE		vm->el->cfg->GetWindowZoom()

// スクリーンサイズ(標準)
#define	P6WINW		vm->vdg->GetW()
#define	P6WINH		vm->vdg->GetH()


#ifndef NOMONITOR	// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// モニタモード スクリーン表示マージン
#define	P6WINMGN	4
// モニタモードウィンドウサイズ
#define	P6DEBUGW	(max(P6WINW+P6WINMGN*2,vm->el->monw->Width())+max(vm->el->regw->Width(),vm->el->memw->Width()))
#define	P6DEBUGH	(max(P6WINH+P6WINMGN*2+vm->el->monw->Height(),vm->el->regw->Height()+vm->el->memw->Height()))
#endif				// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@


// モニタモード時はフルスクリーン，スキャンライン，4:3表示禁止
// フルスクリーンモード時はステータスバー表示禁止
#ifndef NOMONITOR	// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
#define	DISPMON		vm->el->IsMonitor()
#define	DISPFULL	(!DISPMON && vm->el->cfg->GetFullScreen())
#define	DISPSCAN	(!DISPMON && vm->el->cfg->GetScanLine())
#define	DISPNTSC	(!DISPMON && vm->el->cfg->GetDispNTSC())
#else
#define	DISPFULL	vm->el->cfg->GetFullScreen()
#define	DISPSCAN	vm->el->cfg->GetScanLine()
#define	DISPNTSC	vm->el->cfg->GetDispNTSC()
#endif				// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

#define	DISPSTAT	(!DISPFULL && vm->el->cfg->GetDispStat())


////////////////////////////////////////////////////////////////
// コンストラクタ
////////////////////////////////////////////////////////////////
DSP6::DSP6( VM6 *pvm ) : vm(pvm), Wh(nullptr) {}


////////////////////////////////////////////////////////////////
// デストラクタ
////////////////////////////////////////////////////////////////
DSP6::~DSP6( void )
{
	if( Wh ) OSD_DestroyWindow( Wh );
}


////////////////////////////////////////////////////////////////
// 初期化
//
// 引数:	なし
// 返値:	bool	true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool DSP6::Init( void )
{
	PRINTD( GRP_LOG, "[GRP][Init]\n" );
	
	// スクリーンサーフェス作成
	if( !SetScreenSurface() ) return false;
	
	return true;
}


////////////////////////////////////////////////////////////////
// 初期化
//
// 引数:	model	機種 60,62,66
// 返値:	なし
////////////////////////////////////////////////////////////////
void DSP6::SetIcon( const int model )
{
	OSD_SetIcon( Wh, model );
}


////////////////////////////////////////////////////////////////
// スクリーンサーフェス作成
//
// 引数:	なし
// 返値:	bool	true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool DSP6::SetScreenSurface( void )
{
	PRINTD( GRP_LOG, "[GRP][SetScreenSurface]\n" );
	
	int x, y;
	
	#ifndef NOMONITOR	// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	if( DISPMON ){	// モニタモード?
		x = P6DEBUGW;
		y = P6DEBUGH;
		
		PRINTD( GRP_LOG, " -> Monitor Mode ( X:%d Y:%d )\n", x, y );
	}else
	#endif				// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	{
		x = ScreenX();
		y = ScreenY() + (DISPSTAT ? vm->el->staw->Height() : 0);
		
		PRINTD( GRP_LOG, " -> %s ( X:%d Y:%d )\n", DISPFULL ? "FullScreen" : "Window", x, y );
	}
	
	// ウィンドウ作成
	OSD_CreateWindow( &Wh, x, y, P6WINW, P6WINH, DISPFULL, vm->el->cfg->GetFiltering(), vm->el->cfg->GetScanLineBr() );
	
	PRINTD( GRP_LOG, " -> %s ( %d x %d )\n", Wh ? "OK" : "Failed", Wh ? OSD_GetWindowWidth( Wh ) : 0, Wh ? OSD_GetWindowHeight( Wh ) : 0 );
	
	if( !Wh ) return false;
	
	// フルスクリーンの時はマウスカーソルを非表示
	OSD_ShowCursor( !DISPFULL );
	
	return true;
}


////////////////////////////////////////////////////////////////
// スクリーンサイズ変更
//
// 引数:	なし
// 返値:	bool	true:成功 false:失敗
////////////////////////////////////////////////////////////////
bool DSP6::ResizeScreen( void )
{
	PRINTD( GRP_LOG, "[GRP][ResizeScreen]\n" );
	
	int x, y;
	
	// ウィンドウサイズチェック
	#ifndef NOMONITOR	// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	if( DISPMON ){	// モニタモード?
		x = P6DEBUGW;
		y = P6DEBUGH;
	}else
	#endif				// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	{
		x = ScreenX();
		y = ScreenY() + (DISPSTAT ? vm->el->staw->Height() : 0);
	}
	
	// ウィンドウサイズ変更 or フィルタリング変更なら作り直す
	if( !Wh || (x != OSD_GetWindowWidth( Wh )) || (y != OSD_GetWindowHeight( Wh )) || (OSD_IsFullScreen( Wh ) != DISPFULL) || OSD_IsFiltering( Wh ) != vm->el->cfg->GetFiltering() ){
		if( !SetScreenSurface() ) return false;
		vm->el->staw->Init( OSD_GetWindowWidth( Wh ) );	// ステータスバーも
	}else
		// 作り直さない場合は現在のスクリーンサーフェスをクリア
		OSD_ClearWindow( Wh );
	
	return true;
}


////////////////////////////////////////////////////////////////
// 画面更新
//
// 引数:	なし
// 返値:	なし
////////////////////////////////////////////////////////////////
void DSP6::DrawScreen( void )
{
	PRINTD( GRP_LOG, "[GRP][DrawScreen]\n" );
	
	VRect pos;
	VSurface* BBuf = vm->vdg;		// バックバッファへのポインタ取得
	
	if( !Wh || !BBuf ) return;
	
	// スクリーンサーフェスにblit
	PRINTD( GRP_LOG, " -> Blit" );
	
	#ifndef NOMONITOR	// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	if( DISPMON ){	// モニタモード?
		PRINTD( GRP_LOG, " -> Monitor" );
		
		pos.x = P6WINMGN;
		pos.y = P6WINMGN;
		pos.w = P6WINW;
		pos.h = P6WINH;
		
		// モニタウィンドウ描画
		OSD_BlitToWindow( Wh, vm->el->monw, 0, vm->el->regw->Height()+vm->el->memw->Height()-vm->el->monw->Height() );
		
		// レジスタウィンドウ描画
		OSD_BlitToWindow( Wh, vm->el->regw, max( P6WINW+P6WINMGN * 2, vm->el->monw->Width() ), 0 );
		
		// メモリウィンドウ描画
		OSD_BlitToWindow( Wh, vm->el->memw, max( P6WINW+P6WINMGN * 2, vm->el->monw->Width() ), vm->el->regw->Height() );
	}else
	#endif				// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	{
		pos.x = pos.y = 0;
		pos.w = ScreenX();
		pos.h = ScreenY();
		
		// ステータスバー
		if( DISPSTAT ){
			PRINTD( GRP_LOG, " -> Statusbar" );
			
			// ステータスバー更新/描画
			// スクリーンサーフェス下端に位置を合わせてblit
			vm->el->staw->Update();
			OSD_BlitToWindow( Wh, vm->el->staw, 0, OSD_GetWindowHeight( Wh ) - vm->el->staw->Height() );
		}
	}
	
	// バックバッファ描画
	OSD_BlitToWindowEx( Wh, BBuf, &pos, DISPSCAN );
	
	PRINTD( GRP_LOG, " -> OK\n" );
	
	// 描画反映
	OSD_RenderWindow( Wh );
}


////////////////////////////////////////////////////////////////
// 有効スクリーン幅取得
//
// 引数:	なし
// 返値:	int		有効スクリーン幅ピクセル数
////////////////////////////////////////////////////////////////
int DSP6::ScreenX( void ) const
{
	return (int)((double)(P6WINW * WSCALE) / (DISPNTSC ? vm->vdg->GetVratio() : 100.0) + 0.5);
}


////////////////////////////////////////////////////////////////
// 有効スクリーン高さ取得
//
// 引数:	なし
// 返値:	int		有効スクリーン高さピクセル数
////////////////////////////////////////////////////////////////
int DSP6::ScreenY( void ) const
{
	return P6WINH * WSCALE / 100;
}


////////////////////////////////////////////////////////////////
// ウィンドウハンドル取得
//
// 引数:	なし
// 返値:	HWINDOW*	ウィンドウハンドル
////////////////////////////////////////////////////////////////
HWINDOW DSP6::GetWindowHandle( void )
{
	return (HWINDOW)Wh;
}


////////////////////////////////////////////////////////////////
// スナップショット
//
// 引数:	path	スクリーンショット格納パスへの参照
// 返値:	なし
////////////////////////////////////////////////////////////////
void DSP6::SnapShot( const std::filesystem::path& path )
{
	PRINTD( GRP_LOG, "[GRP][SnapShot]\n" );
	
	std::filesystem::path tpath;
	int Index = 0;
	
	// スナップショット格納フォルダがなければフォルダを作成
	if( !FileExist( path ) ){
		if( !CreateFolder( path ) ) return;
	}
	
	// スナップショットファイル名を決める
	do{
		tpath = path / std::filesystem::u8path( Stringf( "%s%03d.%s", FILE_SNAP, ++Index, EXT_IMG ) );
	}while( FileExist( tpath ) || (Index > 999) );
	
	// 連番が有効なら画像ファイル保存
	if( !(Index > 999) ){
		VRect scr;
		
		scr.x = scr.y = 0;
		scr.w = ScreenX();
		scr.h = ScreenY();
		
		BYTE* pixels = new BYTE[( (scr.w * 24 + 31) / 32 ) * sizeof(DWORD) * scr.h];
		if( !pixels ) return;
		
		if( !OSD_GetWindowImage( Wh, (void**)(&pixels), &scr, 24 ) ){
			delete [] pixels;
			return;
		}
		SaveImgData( tpath, pixels, 24, scr.w, scr.h, nullptr );
		
		delete [] pixels;
	}
}
