#ifndef INI_H_INCLUDED
#define INI_H_INCLUDED

#include <filesystem>
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
	cNode();					// コンストラクタ
	~cNode();					// デストラクタ
	
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
	std::filesystem::path IniPath;	// ファイルパス
	
	std::list<cNode>::iterator FindNode( const std::string&, const std::string& );							// ノード検索
	
public:
	cIni();						// コンストラクタ
	virtual ~cIni();			// デストラクタ
	
	void Init();																							// 初期化
	bool Read( const std::filesystem::path& );																// INIファイル読込み
	bool Write();																							// INIファイル書込み
	
	bool GetString( const std::string&, const std::string&, std::string&, const std::string& );				// 文字列読込み
	bool GetInt( const std::string&, const std::string&, int*, const int );									// 数値読込み
	bool GetTruth( const std::string&, const std::string&, bool*, const bool );								// YesNo読込み
	bool GetPath( const std::string&, const std::string&, std::filesystem::path&, const std::filesystem::path& );	// パス読込み
	bool PutEntry( const std::string&, const std::string&, const std::string&, const std::string&, ... );	// エントリ追加
	bool DeleteBefore( const std::string&, const std::string& );											// エントリ削除(前)
	bool DeleteAfter( const std::string&, const std::string& );												// エントリ削除(後)
	const std::filesystem::path& GetFilePath() const;														// ファイルパス取得
};



// どこでもSAVE用インターフェイス
struct IDoko
{
	virtual bool DokoLoad( cIni* ) = 0;
	virtual bool DokoSave( cIni* ) = 0;
};

#endif	// INI_H_INCLUDED
