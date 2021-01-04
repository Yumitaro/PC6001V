// ----------------------------------------------------------------------------
//	M88 - PC-8801 series emulator
//	Copyright (C) cisc 1999.
//  をベースにゆみたろが細工したものです
// ----------------------------------------------------------------------------
//	$Id: error.cpp,v 1.6 2002/04/07 05:40:08 cisc Exp $

#include "error.h"
#include "common.h"


const TextID ErrorText[Error::EndofErrors] =
{
	TERR_NoError,			// NoError
	TERR_Unknown,			// Unknown
	TERR_MemAllocFailed,	// MemAllocFailed
	TERR_RomChange,			// RomChange
	TERR_NoRom,				// NoRom
	TERR_RomSizeNG,			// RomSizeNG
	TERR_RomCrcNG,			// RomCrcNG
	TERR_LibInitFailed,		// LibInitFailed
	TERR_InitFailed,		// InitFailed
	TERR_FontLoadFailed,	// FontLoadFailed
	TERR_FontCreateFailed,	// FontCreateFailed
	TERR_IniDefault,		// IniDefault
	TERR_IniReadFailed,		// IniReadFailed
	TERR_IniWriteFailed,	// IniWriteFailed
	TERR_TapeMountFailed,	// TapeMountFailed
	TERR_DiskMountFailed,	// DiskMountFailed
	TERR_ExtRomMountFailed,	// ExtRomMountFailed
	TERR_DokoReadFailed,	// DokoReadFailed
	TERR_DokoWriteFailed,	// DokoWriteFailed
	TERR_DokoDiffVersion,	// DokoDiffVersion
	TERR_ReplayPlayError,	// ReplayPlayError
	TERR_ReplayRecError,	// ReplayRecError
	TERR_NoReplayData,		// NoReplayData
	TERR_CaptureFailed		// CaptureFailed

};

Error::Errno Error::err = Error::NoError;

void Error::SetError( Errno e )
{
	err = e;
}

Error::Errno Error::GetError( void )
{
	return err;
}

const std::string& Error::GetErrorText()
{
	return GetText( ErrorText[err] );
}

void Error::Reset( void )
{
	err = NoError;
}

