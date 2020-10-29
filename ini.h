#ifndef INI_H_INCLUDED
#define INI_H_INCLUDED

#include <string>
#include <list>

#include "typedef.h"


////////////////////////////////////////////////////////////////
// クラス定義
////////////////////////////////////////////////////////////////
class cNode {
public:
	enum NodeType{ NODE_NONE, NODE_COMMENT, NODE_SECTION, NODE_ENTRY };
	
public:
	cNode();
	~cNode();
	
	void SetMember( NodeType, const std::string& );
	
	NodeType NodeID;		// Node ID
	std::string Comment;	// Comments
	std::string Section;	// Sections
	std::string Entry;		// Entries
	std::string Value;
};

class cIni {
protected:
	std::list<cNode> IniNode;
	P6VPATH IniPath;		// ファイルパス
	
	std::list<cNode>::iterator FindNode( const std::string&, const std::string& );							// ノード検索
	
public:
	cIni();
	virtual ~cIni();
	
	void Init();																							// 初期化
	bool Read( const P6VPATH& );																			// INIファイル読込み
	bool Write();																							// INIファイル書込み
	
	bool GetEntry( const std::string&, const std::string&, std::string& );									// 文字列読込み
    template<typename T>bool GetValue( const std::string&, const std::string&, T& );						// 数値読込み
	bool GetYesNo( const std::string&, const std::string&, bool& );											// YesNo読込み
	bool GetPath ( const std::string&, const std::string&, P6VPATH& );										// パス読込み
	bool PutEntry( const std::string&, const std::string&, const std::string&, const std::string&, ... );	// エントリ追加
	bool PutValue( const std::string&, const std::string&, const std::string&, const int, const std::string& = "%d" );	// エントリ追加 数値
	bool PutYesNo( const std::string&, const std::string&, const std::string&, const bool );				// エントリ追加 YesNo
	bool PutPath ( const std::string&, const std::string&, const std::string&, const P6VPATH& );			// エントリ追加 パス
	bool DeleteBefore( const std::string&, const std::string& );											// エントリ削除(前)
	bool DeleteAfter( const std::string&, const std::string& );												// エントリ削除(後)
	const P6VPATH& GetFilePath() const;																		// ファイルパス取得
};



// どこでもSAVE用インターフェイス
struct IDoko
{
	virtual bool DokoLoad( cIni* ) = 0;
	virtual bool DokoSave( cIni* ) = 0;
};

#endif	// INI_H_INCLUDED
