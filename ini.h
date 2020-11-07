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
	
	bool GetEntry( const std::string&, const std::string&, std::string& );									// エントリ読込み(文字列)
	bool SetEntry( const std::string&, const std::string&, const std::string&, const std::string&, ... );	// エントリ書込み(文字列)
	
	template <typename T> bool GetVal( const std::string&, const std::string&, T& );						// エントリ読込み
	template <typename T> bool SetVal( const std::string&, const std::string&, const std::string&, const T& );	// エントリ書込み
																											// エントリ書込み(書式付き)
	template <typename T, typename ... A> bool SetVal( const std::string& section, const std::string& entry, const std::string& comment, const std::string& format, const T& val, A const & ... args )
	{
		return SetEntry( section, entry, comment, format, val, args ... );
	}
	
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
