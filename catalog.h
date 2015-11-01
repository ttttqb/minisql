#pragma once
#include "btree.h"
#include "table.h"
#include "recmgr.h"

extern unsigned char nullfill[BlockSize];

class catalog :
	public recmgr
{
public:
	catalog();
	catalog(bufmgr &s) :recmgr(s){}
	~catalog();
	bool cmExistTable(const string &name);
	table cmCreateTable(const string &name, const vector <item> &data);
	table cmReadTable(const string &name);
	void cmDropTable(const string &name);
	bool cmExistIndex(const string &indexname);
	bool cmRegisterIndex(const string &tablename, const string &indexname, int itemIndex);
	pair<string, int> cmAskIndex(const string indexName);
	bool cmDeleteIndex(const string indexName);
};

